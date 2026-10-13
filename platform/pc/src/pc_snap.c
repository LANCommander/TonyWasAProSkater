/* THPS_SNAP: record the machine state at the entry of chosen functions, for the differential
 * tests that check C rewrites of hand-written assembly and library code against the retail MIPS
 * (platform/pc/tests/test_asm.c replays each snapshot through tests/mips_ref.c and through the
 * C, and compares what they leave behind).
 *
 *   THPS_SNAP=addr[:count[:every]],...   e.g. THPS_SNAP=80086f40:8:500,8008b254:4
 *   THPS_SNAPDIR=dir                     (default snaps)
 *
 * takes `count` snapshots (default 4) of each function, one every `every` calls (default 1).
 * A snapshot is the RAM the game uses, the scratchpad, the GTE registers, the first 12 argument
 * words and the native stack above them (decompiled callers pass pointers to their locals),
 * written to <dir>/<addr>_<n>.snp (read by tests/test_asm.c). The function then runs as usual. */
#ifdef _WIN32
#include <windows.h>
#else
#define _GNU_SOURCE
#include <pthread.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pc_gte.h"
#include "pc_platform.h"
#include "pc_recomp.h"
#include "pc_runtime.h"

#define MAX_SNAP 256
#define SNAP_ARGS 12
#define SNAP_STACK 0x40000u /* at most this much of the native stack, from the arguments up */

typedef uint32_t (*Fn12)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                         uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);

static struct {
    uint32_t addr;
    Fn12 fn;
    int left, every, calls, taken;
} s_snap[MAX_SNAP];
static int s_nsnap;
static char s_dir[MAX_PATH];

uint32_t PC_SnapHandler(uint32_t addr, const uint32_t *args);

/* Each hooked address jumps to `mov eax, addr; jmp pc_snap_entry`; this passes the address and
 * a pointer to the caller's argument words on to PC_SnapHandler. */
__asm__(".text\n"
        PC_ASM_SYM("pc_snap_entry") ":\n"
        "\tpushl %ebp\n"
        "\tmovl %esp, %ebp\n"
        "\tleal 8(%ebp), %ecx\n"
        "\tpushl %ecx\n"
        "\tpushl %eax\n"
        "\tcall " PC_ASM_SYM("PC_SnapHandler") "\n"
        "\tleave\n"
        "\tret\n");
extern void pc_snap_entry(void);

/* The (exclusive) top of the calling thread's stack. */
static uint32_t StackTop(void) {
#ifdef _WIN32
    return (uint32_t)(uintptr_t)((NT_TIB *)NtCurrentTeb())->StackBase;
#else
    pthread_attr_t at;
    void *base = NULL;
    size_t size = 0;
    if (!pthread_getattr_np(pthread_self(), &at)) {
        pthread_attr_getstack(&at, &base, &size);
        pthread_attr_destroy(&at);
    }
    return (uint32_t)((uintptr_t)base + size);
#endif
}

static void Write(int k, const uint32_t *args) {
    char path[MAX_PATH + 32];
    uint32_t hdr[8 + SNAP_ARGS], ram = pc_ram_top - PSX_RAM_BASE;
    int gs = PC_GteStateSize();
    void *gte = malloc((size_t)gs);
    FILE *f;
    snprintf(path, sizeof path, "%s" PC_SEP "%08x_%d.snp", s_dir, s_snap[k].addr, s_snap[k].taken);
    f = fopen(path, "wb");
    if (!f) {
        PC_Log("THPS_SNAP: cannot write %s", path);
        free(gte);
        return;
    }
    memcpy(&hdr[0], "TSNP", 4);
    hdr[1] = 2;
    hdr[2] = s_snap[k].addr;
    hdr[3] = SNAP_ARGS;
    hdr[4] = ram;
    hdr[5] = (uint32_t)gs;
    hdr[6] = pc_r.r[29];
    hdr[7] = pc_r.r[28];
    memcpy(&hdr[8], args, 4 * SNAP_ARGS);
    PC_GteSaveState(gte);
    fwrite(hdr, 4, 8 + SNAP_ARGS, f);
    fwrite(PSX_PTR(PSX_RAM_BASE), 1, ram, f);
    fwrite(PSX_PTR(PSX_SCRATCH_BASE), 1, 1024, f);
    fwrite(gte, 1, (size_t)gs, f);
    {
        /* the native stack the arguments are on: callers pass pointers to their locals */
        uint32_t lo = ((uint32_t)(uintptr_t)args & ~0xfffu) - 0x1000u;
        uint32_t top = StackTop(), sz;
        if (top - lo > SNAP_STACK)
            top = lo + SNAP_STACK;
        sz = top - lo;
        fwrite(&lo, 4, 1, f);
        fwrite(&sz, 4, 1, f);
        fwrite((void *)(uintptr_t)lo, 1, sz, f);
    }
    fclose(f);
    free(gte);
    PC_Log("THPS_SNAP: %s", path);
}

uint32_t PC_SnapHandler(uint32_t addr, const uint32_t *args) {
    int k;
    for (k = 0; k < s_nsnap && s_snap[k].addr != addr; k++)
        ;
    if (k == s_nsnap)
        PC_Fatal("THPS_SNAP: no hook for %08x", addr);
    if (s_snap[k].left > 0 && s_snap[k].calls++ % s_snap[k].every == 0) {
        Write(k, args);
        s_snap[k].taken++;
        s_snap[k].left--;
    }
    return s_snap[k].fn(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7],
                        args[8], args[9], args[10], args[11]);
}

/* After the trampolines are in: route the chosen functions through the recorder. */
void PC_SnapInstall(void) {
    const char *spec = getenv("THPS_SNAP");
    uint8_t *stubs;
    if (!spec || !*spec)
        return;
    snprintf(s_dir, sizeof s_dir, "%s", getenv("THPS_SNAPDIR") ? getenv("THPS_SNAPDIR") : "snaps");
    PC_MakeDirs(s_dir);
    stubs = PC_AllocExec(4096);
    while (*spec && s_nsnap < MAX_SNAP) {
        char *e;
        uint32_t a = strtoul(spec, &e, 16);
        int count = 4, every = 1;
        if (*e == ':')
            count = (int)strtol(e + 1, &e, 10);
        if (*e == ':')
            every = (int)strtol(e + 1, &e, 10);
        if (PC_NativeOf(a)) {
            uint8_t *s = stubs + 16 * s_nsnap, *p = PSX_PTR(a);
            int32_t rel;
            s_snap[s_nsnap].addr = a;
            s_snap[s_nsnap].fn = (Fn12)PC_NativeOf(a);
            s_snap[s_nsnap].left = count;
            s_snap[s_nsnap].every = every > 0 ? every : 1;
            s[0] = 0xb8; /* mov eax, imm32 */
            memcpy(s + 1, &a, 4);
            s[5] = 0xe9; /* jmp rel32 */
            rel = (int32_t)((uintptr_t)pc_snap_entry - ((uintptr_t)s + 10));
            memcpy(s + 6, &rel, 4);
            p[0] = 0xe9;
            rel = (int32_t)((uintptr_t)s - ((uintptr_t)p + 5));
            memcpy(p + 1, &rel, 4);
            s_nsnap++;
        } else {
            PC_Log("THPS_SNAP: %08x is not a function", a);
        }
        while (*e && *e != ',')
            e++;
        spec = *e == ',' ? e + 1 : e;
    }
#ifdef _WIN32
    FlushInstructionCache(GetCurrentProcess(), NULL, 0);
#endif
    PC_Log("THPS_SNAP: %d functions hooked, snapshots to %s", s_nsnap, s_dir);
}
