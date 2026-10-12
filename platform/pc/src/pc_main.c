/* Boot: map PS1 memory at its own addresses, load the retail image, point every PS1 function
 * address at its native implementation, then run the game's main(). See platform/pc/README.md. */
#include <windows.h>
#include <shlobj.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pc_gpu.h"
#include "pc_gte.h"
#include "pc_hle.h"
#include "pc_recomp.h"
#include "pc_runtime.h"

#define EXE_GAME_MAIN 0x80026d44u
#define EXE_GP 0x800d1080u
#define EXE_BSS_START 0x800d21c8u
#define EXE_BSS_END 0x800dc94cu
#define EXE_STACK_TOP_VAR 0x800d0548u /* the linker's _stacktop / _stacksize words */
#define EXE_STACK_SIZE_VAR 0x800d0544u

PcRegs pc_r;
extern volatile uint32_t pc_heartbeat;
int pc_trace;

static char s_dataDir[MAX_PATH];

/* ---- logging ------------------------------------------------------------------------------- */

void PC_Log(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    fflush(stderr);
}

void PC_Trace(const char *fmt, ...) {
    va_list ap;
    if (!pc_trace)
        return;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    fflush(stderr);
}

void PC_Fatal(const char *fmt, ...) {
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    fprintf(stderr, "FATAL: %s\n", buf);
    fflush(stderr);
    if (!getenv("THPS_HEADLESS"))
        MessageBoxA(NULL, buf, "Tony Hawk's Pro Skater", MB_ICONERROR | MB_OK);
    ExitProcess(1);
}

/* ---- function table ------------------------------------------------------------------------- */

typedef struct {
    uint32_t addr;
    void *fn;
    const char *kind;
} FnEntry;

static FnEntry *s_fns;
static int s_nfns;

static int CmpFn(const void *a, const void *b) {
    uint32_t x = ((const FnEntry *)a)->addr, y = ((const FnEntry *)b)->addr;
    return x < y ? -1 : x > y;
}

static void AddTable(const PcFunc *t, const char *kind, int *cap) {
    for (; t->addr; t++) {
        if (s_nfns == *cap) {
            *cap = *cap ? *cap * 2 : 4096;
            s_fns = realloc(s_fns, sizeof *s_fns * *cap);
        }
        s_fns[s_nfns].addr = t->addr;
        s_fns[s_nfns].fn = t->fn;
        s_fns[s_nfns].kind = kind;
        s_nfns++;
    }
}

static FnEntry *FindFn(uint32_t addr) {
    FnEntry key = {addr, 0, 0};
    return bsearch(&key, s_fns, s_nfns, sizeof *s_fns, CmpFn);
}

void *PC_NativeOf(uint32_t addr) {
    FnEntry *e = FindFn(addr);
    return e ? e->fn : NULL;
}

extern const char *PC_SymbolName(uint32_t addr); /* generated: names.c */
extern const PcFunc pc_override_funcs[];

const char *PC_NameOf(uint32_t addr) {
    static char buf[4][64];
    static int n;
    const char *s = PC_SymbolName(addr);
    if (s)
        return s;
    n = (n + 1) & 3;
    snprintf(buf[n], sizeof buf[n], "FUN_%08x", addr);
    return buf[n];
}

extern const PcFunc pc_recomp_game_funcs[];

/* THPS_MIPS: run game functions from the retail MIPS (translated) instead of the decompiled C,
 * for A/B debugging. A comma list applied left to right: "all", an address, a range "lo-hi", each
 * optionally prefixed with '!' to go back to C. E.g. THPS_MIPS=all,!80080000-80085000. */
static int MipsSelected(uint32_t addr, const char *spec) {
    int sel = 0;
    const char *p = spec;
    while (*p) {
        int neg = 0, hit;
        uint32_t lo, hi;
        char *e;
        if (*p == '!') {
            neg = 1;
            p++;
        }
        if (!strncmp(p, "all", 3)) {
            hit = 1;
            p += 3;
        } else {
            lo = hi = strtoul(p, &e, 16);
            p = e;
            if (*p == '-') {
                hi = strtoul(p + 1, &e, 16);
                p = e;
            }
            hit = addr >= lo && addr <= hi;
        }
        if (hit)
            sel = !neg;
        while (*p && *p != ',')
            p++;
        if (*p == ',')
            p++;
    }
    return sel;
}

static void SelectMips(void) {
    const char *spec = getenv("THPS_MIPS");
    const PcFunc *t;
    int n = 0;
    if (!spec || !*spec)
        return;
    for (t = pc_recomp_game_funcs; t->addr; t++) {
        FnEntry *e;
        if (!MipsSelected(t->addr, spec) || !(e = FindFn(t->addr)))
            continue;
        if (!strcmp(e->kind, "c")) {
            e->fn = t->fn;
            e->kind = "mips";
            n++;
        }
    }
    PC_Log("THPS_MIPS: %d game functions run as translated retail code", n);
}

/* Later tables take precedence: stubs < translated MIPS < HLE < decompiled C. */
static void BuildFunctionTable(void) {
    int cap = 0, i, j;
    AddTable(pc_stub_funcs, "stub", &cap);
    AddTable(pc_recomp_funcs, "mips", &cap);
    AddTable(pc_hle_funcs, "hle", &cap);
    AddTable(pc_game_funcs, "c", &cap);
    AddTable(pc_override_funcs, "hle", &cap);
    /* stable sort by address keeping the last (highest-precedence) entry per address */
    for (i = 1; i < s_nfns; i++) {
        FnEntry e = s_fns[i];
        for (j = i - 1; j >= 0 && s_fns[j].addr > e.addr; j--)
            s_fns[j + 1] = s_fns[j];
        s_fns[j + 1] = e;
    }
    for (i = 0, j = 0; i < s_nfns; i++) {
        if (j && s_fns[j - 1].addr == s_fns[i].addr)
            s_fns[j - 1] = s_fns[i];
        else
            s_fns[j++] = s_fns[i];
    }
    s_nfns = j;
    SelectMips();
}

static uint32_t pc_count_addr[32];
static volatile uint32_t pc_count[32];
static int pc_ncount;

/* Called once per vblank: print and reset THPS_COUNT counters. */
void PC_CountReport(uint32_t frame) {
    char line[1024];
    int i, n = 0, any = 0;
    for (i = 0; i < pc_ncount; i++) {
        n += snprintf(line + n, sizeof line - n, " %s=%u", PC_NameOf(pc_count_addr[i]),
                      pc_count[i]);
        any |= pc_count[i] != 0;
        pc_count[i] = 0;
    }
    if (any)
        PC_Log("count f%u:%s", frame, line);
}

/* A 5-byte `jmp rel32` over the first bytes of each function's (unused) MIPS code. */
static void InstallTrampolines(void) {
    int i, counts[4] = {0};
    for (i = 0; i < s_nfns; i++) {
        uint8_t *p = PSX_PTR(s_fns[i].addr);
        int32_t rel = (int32_t)((uintptr_t)s_fns[i].fn - ((uintptr_t)p + 5));
        p[0] = 0xe9;
        memcpy(p + 1, &rel, 4);
        if (!strcmp(s_fns[i].kind, "c"))
            counts[0]++;
        else if (!strcmp(s_fns[i].kind, "hle"))
            counts[1]++;
        else if (!strcmp(s_fns[i].kind, "mips"))
            counts[2]++;
        else
            counts[3]++;
    }
    {
        /* THPS_COUNT=addr,addr,...: count calls per frame (PC_CountReport prints them) by
         * routing those trampolines through `inc [counter]; jmp native` stubs */
        const char *spec = getenv("THPS_COUNT");
        static uint8_t *stubs;
        int k = 0;
        if (spec && !stubs)
            stubs = VirtualAlloc(NULL, 4096, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
        while (spec && *spec && k < 32) {
            char *e;
            uint32_t a = strtoul(spec, &e, 16);
            FnEntry *f = FindFn(a);
            if (f) {
                uint8_t *s = stubs + 16 * k, *p = PSX_PTR(a);
                int32_t rel;
                pc_count_addr[k] = a;
                s[0] = 0xff;
                s[1] = 0x05;
                *(uint32_t *)(s + 2) = (uint32_t)(uintptr_t)&pc_count[k];
                s[6] = 0xe9;
                rel = (int32_t)((uintptr_t)f->fn - ((uintptr_t)s + 11));
                memcpy(s + 7, &rel, 4);
                rel = (int32_t)((uintptr_t)s - ((uintptr_t)p + 5));
                memcpy(p + 1, &rel, 4);
                k++;
            }
            spec = *e == ',' ? e + 1 : e;
        }
        pc_ncount = k;
    }
    FlushInstructionCache(GetCurrentProcess(), PSX_PTR(PSX_EXE_BASE), PSX_RAM_SIZE - 0x10000);
    PC_Log("functions: %d decompiled C, %d HLE, %d translated MIPS, %d missing (stubs)", counts[0],
           counts[1], counts[2], counts[3]);
}

uint32_t PC_MissingFunction(uint32_t addr) {
    static uint32_t seen[512];
    static int nseen;
    int i;
    for (i = 0; i < nseen; i++)
        if (seen[i] == addr)
            return 0;
    if (nseen < 512)
        seen[nseen++] = addr;
    PC_Log("missing function %08x (%s) called; returning 0", addr, PC_NameOf(addr));
    if (getenv("THPS_STRICT"))
        PC_Fatal("missing function %08x (%s)", addr, PC_NameOf(addr));
    return 0;
}

/* ---- translated-MIPS support ---------------------------------------------------------------- */

u32 rc_enter(void (*body)(void), u32 a0, u32 a1, u32 a2, u32 a3, const u32 *stack) {
    PcRegs save = pc_r;
    u32 sp = (save.r[29] - 16 - 4 * RC_STACK_ARGS - 32) & ~7u, v0, v1;
    int i;
    pc_r.r[29] = sp;
    pc_r.r[4] = a0;
    pc_r.r[5] = a1;
    pc_r.r[6] = a2;
    pc_r.r[7] = a3;
    for (i = 0; i < RC_STACK_ARGS; i++)
        MEM32(sp + 16 + 4 * i) = stack[i];
    pc_r.r[28] = EXE_GP;
    pc_r.r[31] = 0xfffffff0u;
    body();
    v0 = pc_r.r[2];
    v1 = pc_r.r[3];
    pc_r = save;
    pc_r.r[2] = v0;
    pc_r.r[3] = v1;
    return v0;
}

typedef uint64_t (*NativeFn)(u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32);

static void CallNative(u32 target, int indirect) {
    u32 sp = pc_r.r[29];
    void *fn = PC_NativeOf(target);
    uint64_t r;
    if (!fn) {
        PC_Log("%s call to %08x, which is not a known function", indirect ? "indirect" : "direct",
               target);
        pc_r.r[2] = 0;
        return;
    }
    r = ((NativeFn)fn)(pc_r.r[4], pc_r.r[5], pc_r.r[6], pc_r.r[7], MEM32(sp + 16), MEM32(sp + 20),
                       MEM32(sp + 24), MEM32(sp + 28), MEM32(sp + 32), MEM32(sp + 36),
                       MEM32(sp + 40), MEM32(sp + 44));
    pc_r.r[2] = (u32)r;
    pc_r.r[3] = (u32)(r >> 32);
}

void rc_call_native(u32 target) { CallNative(target, 0); }
void rc_call_indirect(u32 target) {
    if (target == 0xa0 || target == 0xb0 || target == 0xc0) {
        /* direct BIOS table call: function number in t1 */
        pc_r.r[2] = PC_BiosCall(target, pc_r.r[9]);
        return;
    }
    CallNative(target, 1);
}

u32 rc_cop0_read(int reg) {
    (void)reg;
    return 0;
}

void rc_break(u32 pc) { PC_Log("MIPS break at %08x (%s)", pc, PC_NameOf(pc)); }

void rc_bad_jump(u32 pc, u32 target) {
    PC_Fatal("translated code at %08x jumped to %08x outside its function", pc, target);
}

void rc_untranslatable(u32 entry) { PC_Fatal("function %08x could not be translated", entry); }

/* ---- C++ delete (see link.py, "destructor thunks") -------------------------------------------- */

void *pc_delete_marked, *pc_delete_freed;

void pc_vdelete(void *vtable, int slot, void *obj) {
    uint8_t *e = (uint8_t *)vtable + 8 + 8 * slot;
    void (*dtor)(void *, int) = *(void (**)(void *, int))(e + 4);
    dtor((uint8_t *)obj + *(int16_t *)e, 3);
}

void pc_delete_mark(void *p) {
    pc_delete_marked = p;
    pc_delete_freed = NULL;
}

/* ---- memory map ----------------------------------------------------------------------------- */

uint32_t pc_ram_top = PSX_RAM_BASE + PSX_RAM_SIZE;

static void MapPsxMemory(void) {
    HANDLE sec = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_EXECUTE_READWRITE, 0,
                                    PSX_RAM_MAPPED, NULL);
    void *kseg0, *kseg1, *scratch;
    if (!sec)
        PC_Fatal("CreateFileMapping failed (%lu)", GetLastError());
    /* 8 MB: the PS1's 2 MB, then what the game gets with the Dreamcast data (pc_ram_top). With
     * the PS1 data nothing above 2 MB is used: the stack top the executable was linked with (an
     * 8 MB dev-kit address) is moved down by Crt0. */
    kseg0 = MapViewOfFileEx(sec, FILE_MAP_ALL_ACCESS | FILE_MAP_EXECUTE, 0, 0, PSX_RAM_MAPPED,
                            PSX_PTR(PSX_RAM_BASE));
    kseg1 = MapViewOfFileEx(sec, FILE_MAP_ALL_ACCESS | FILE_MAP_EXECUTE, 0, 0, PSX_RAM_MAPPED,
                            PSX_PTR(0xa0000000u));
    {
        /* KUSEG: the game strips bit 31 off some buffer pointers (primitive buffers), so main RAM
         * is also needed at 0x00010000-0x001fffff. The first 64 KB can't be mapped on Windows;
         * accesses there go through the fault handler. */
        uint32_t a, missing = 0;
        for (a = 0x10000; a < PSX_RAM_SIZE; a += 0x10000)
            if (!MapViewOfFileEx(sec, FILE_MAP_ALL_ACCESS | FILE_MAP_EXECUTE, 0, a, 0x10000,
                                 PSX_PTR(a))) {
                MEMORY_BASIC_INFORMATION mi;
                if (!missing++ && VirtualQuery(PSX_PTR(a), &mi, sizeof mi))
                    PC_Log("note: KUSEG %08x busy (state %lx, allocation %p, error %lu)", a,
                           mi.State, mi.AllocationBase, GetLastError());
            }
        if (missing) {
            PC_Log("note: %u of 31 KUSEG RAM chunks could not be mapped", missing);
            for (a = 0; pc_trace && a < 0x400000;) {
                MEMORY_BASIC_INFORMATION mi;
                if (!VirtualQuery(PSX_PTR(a), &mi, sizeof mi))
                    break;
                if (mi.State != MEM_FREE)
                    PC_Log("  %08x-%08x state %lx type %lx", a, a + (uint32_t)mi.RegionSize,
                           mi.State, mi.Type);
                a += (uint32_t)mi.RegionSize;
            }
        }
    }
    if (kseg0 != PSX_PTR(PSX_RAM_BASE))
        PC_Fatal("cannot map PS1 RAM at 0x80000000 (%lu): the executable must be 32-bit and "
                 "large-address-aware",
                 GetLastError());
    if (kseg1 != PSX_PTR(0xa0000000u))
        PC_Log("note: no KSEG1 mirror at 0xa0000000 (%lu)", GetLastError());
    /* scratchpad (1 KB) and the I/O register page behind it: plain memory */
    scratch = VirtualAlloc(PSX_PTR(PSX_SCRATCH_BASE), 0x10000, MEM_RESERVE | MEM_COMMIT,
                           PAGE_READWRITE);
    if (scratch != PSX_PTR(PSX_SCRATCH_BASE))
        PC_Fatal("cannot map the scratchpad at 0x1f800000 (%lu)", GetLastError());
    memset(PSX_PTR(PSX_RAM_BASE), 0, PSX_RAM_MAPPED);
}

static int FileExists(const char *p) {
    DWORD a = GetFileAttributesA(p);
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

const char *PC_DataDir(void) { return s_dataDir; }

void (*pc_hires_before_draw)(void);

const char *PC_UserDir(void) {
    static char dir[MAX_PATH];
    if (!dir[0]) {
        char home[MAX_PATH], *s;
        if (getenv("USERPROFILE"))
            snprintf(home, sizeof home, "%s", getenv("USERPROFILE"));
        else if (SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, home) != S_OK)
            strcpy(home, ".");
        snprintf(dir, sizeof dir, "%s\\Neversoft\\Tony Hawk's Pro Skater", home);
        for (s = dir; *s; s++)
            if (*s == '/')
                *s = '\\';
        for (s = strchr(dir + 3, '\\'); s; s = strchr(s + 1, '\\')) {
            *s = 0;
            CreateDirectoryA(dir, NULL);
            *s = '\\';
        }
        CreateDirectoryA(dir, NULL);
    }
    return dir;
}

/* The retail executable: THPS_EXE, else SLUS_008.60 next to the exe, in ./game, or the repo's
 * extern/thps1_retail (development layout). */
static void LoadExecutable(void) {
    char exeDir[MAX_PATH], path[MAX_PATH], *slash;
    const char *cands[] = {"%s\\SLUS_008.60", "%s\\game\\SLUS_008.60",
                           "%s\\..\\..\\extern\\thps1_retail\\SLUS_008.60"};
    FILE *f;
    uint8_t hdr[0x800];
    uint32_t taddr, tsize;
    int i, found = 0;
    GetModuleFileNameA(NULL, exeDir, sizeof exeDir);
    slash = strrchr(exeDir, '\\');
    if (slash)
        *slash = 0;
    if (getenv("THPS_EXE")) {
        snprintf(path, sizeof path, "%s", getenv("THPS_EXE"));
        found = FileExists(path);
    }
    for (i = 0; !found && i < 3; i++) {
        snprintf(path, sizeof path, cands[i], exeDir);
        found = FileExists(path);
    }
    if (!found)
        PC_Fatal("SLUS_008.60 not found. Put the retail executable from your Tony Hawk's Pro "
                 "Skater (USA) disc next to thps.exe, or set THPS_EXE.");
    snprintf(s_dataDir, sizeof s_dataDir, "%s", path);
    slash = strrchr(s_dataDir, '\\');
    if (slash)
        *slash = 0;
    f = fopen(path, "rb");
    if (!f || fread(hdr, 1, sizeof hdr, f) != sizeof hdr || memcmp(hdr, "PS-X EXE", 8))
        PC_Fatal("%s is not a PS-X executable", path);
    memcpy(&taddr, hdr + 0x18, 4);
    memcpy(&tsize, hdr + 0x1c, 4);
    if (taddr != PSX_EXE_BASE || tsize != 0xcd000)
        PC_Fatal("%s is not the THPS1 USA executable (SLUS-00860)", path);
    if (fread(PSX_PTR(taddr), 1, tsize, f) != tsize)
        PC_Fatal("%s is truncated", path);
    fclose(f);
    PC_Log("loaded %s", path);
}

/* What the retail entry point (__SN_ENTRY_POINT) does before main: clear .bss and hand the
 * memory between .bss and the stack to the heap. */
static void Crt0(void) {
    /* the linker's stack top is an 8 MB dev-kit address; on a 2 MB console it is mirrored: the
     * stack goes at the top of the RAM the game uses */
    u32 ram = pc_ram_top - PSX_RAM_BASE;
    u32 stackTop = (((MEM32(EXE_STACK_TOP_VAR) - 1) & (ram - 1)) + 1) | 0x80000000u;
    u32 stackSize = MEM32(EXE_STACK_SIZE_VAR);
    u32 sp = (stackTop - 8) | 0x80000000u;
    u32 heap = EXE_BSS_END;
    u32 size = (stackTop - 8) - stackSize - heap;
    memset(PSX_PTR(EXE_BSS_START), 0, EXE_BSS_END - EXE_BSS_START);
    pc_r.r[29] = sp;
    pc_r.r[28] = EXE_GP;
    PC_Log("heap %08x size %08x, PS1 stack %08x", heap + 4, size, sp);
    PC_HeapInit(heap + 4, size);
}

/* ---- crash diagnostics ---------------------------------------------------------------------- */

/* KUSEG: on the PS1, 0x00000000-0x001fffff is main RAM too, so NULL-ish pointers read and write
 * harmless low memory. Windows can't map those pages, so such an access faults here; the fault is
 * redone against the RAM at 0x80000000 by adding 0x80000000 to the instruction's base (or index)
 * register, single-stepping it, and taking the bias back off. Each site is logged once. */
static DWORD *RegPtr(CONTEXT *c, int r) {
    switch (r) {
    case 0: return &c->Eax;
    case 1: return &c->Ecx;
    case 2: return &c->Edx;
    case 3: return &c->Ebx;
    case 4: return &c->Esp;
    case 5: return &c->Ebp;
    case 6: return &c->Esi;
    default: return &c->Edi;
    }
}

static struct {
    int active;
    int reg;
    DWORD bias;
} s_fix;

/* Opcodes (one byte) that take a ModRM byte. */
static int HasModRM1(uint8_t op) {
    if (op < 0x40)
        return (op & 7) < 4;
    return (op >= 0x62 && op <= 0x63) || op == 0x69 || op == 0x6b || (op >= 0x80 && op <= 0x8f) ||
           (op >= 0xc0 && op <= 0xc1) || (op >= 0xc6 && op <= 0xc7) || (op >= 0xd0 && op <= 0xd3) ||
           (op >= 0xd8 && op <= 0xdf) || op == 0xf6 || op == 0xf7 || op == 0xfe || op == 0xff;
}

static int FixLowAccess(EXCEPTION_POINTERS *ep) {
    CONTEXT *c = ep->ContextRecord;
    uint8_t *p = (uint8_t *)c->Eip;
    uint8_t op, modrm;
    int mod, rm, reg, base = -1, index = -1, scale = 1, writesReg = 0, two = 0;
    DWORD *br;
    while (*p == 0x66 || *p == 0x67 || *p == 0xf2 || *p == 0xf3 || *p == 0x2e || *p == 0x3e ||
           *p == 0x26 || *p == 0x36 || *p == 0x64 || *p == 0x65)
        p++;
    op = *p++;
    if (op == 0x0f) {
        two = 1;
        op = *p++;
    } else if (!HasModRM1(op)) {
        return 0;
    }
    modrm = *p++;
    mod = modrm >> 6;
    reg = (modrm >> 3) & 7;
    rm = modrm & 7;
    if (mod == 3)
        return 0;
    if (rm == 4) {
        uint8_t sib = *p++;
        scale = 1 << (sib >> 6);
        index = (sib >> 3) & 7;
        base = sib & 7;
        if (index == 4)
            index = -1;
        if (base == 5 && mod == 0)
            base = -1;
    } else if (rm == 5 && mod == 0) {
        base = -1;
    } else {
        base = rm;
    }
    /* loads into a general register overwrite `reg`; don't restore it then */
    if (!two)
        writesReg = op == 0x8a || op == 0x8b || (op < 0x40 && (op & 7) >= 2 && (op & 7) <= 3 &&
                                                 (op & 0x38) != 0x38);
    else
        writesReg = op == 0xb6 || op == 0xb7 || op == 0xbe || op == 0xbf || op == 0xaf;
    if (base >= 0 && *RegPtr(c, base) < PSX_RAM_SIZE)
        s_fix.reg = base, s_fix.bias = 0x80000000u;
    else if (index >= 0 && *RegPtr(c, index) * (DWORD)scale < PSX_RAM_SIZE)
        s_fix.reg = index, s_fix.bias = 0x80000000u / (DWORD)scale;
    else
        return 0;
    br = RegPtr(c, s_fix.reg);
    *br += s_fix.bias;
    s_fix.active = 1;
    if (writesReg && reg == s_fix.reg)
        s_fix.active = 2; /* the instruction replaces the register: nothing to undo */
    c->EFlags |= 0x100;   /* trap flag: come back after this one instruction */
    {
        static DWORD seen[256];
        static int nseen;
        int i, known = 0;
        for (i = 0; i < nseen; i++)
            known |= seen[i] == c->Eip;
        if (!known && nseen < 256) {
            seen[nseen++] = c->Eip;
            PC_Log("low-memory access at eip=%08lx (address %08lx) redirected to PS1 RAM", c->Eip,
                   (unsigned long)ep->ExceptionRecord->ExceptionInformation[1]);
        }
    }
    return 1;
}

/* A breakpoint target for debuggers: called once for a genuine crash, before the report. */
void __attribute__((noinline)) PC_CrashHook(void) { __asm__ volatile(""); }

/* The R3000 never traps on division: `div` by zero leaves LO = (dividend < 0 ? 1 : -1) (or all
 * ones unsigned) and HI = dividend, and INT_MIN / -1 gives LO = INT_MIN, HI = 0. The game relies
 * on that (retail uses unchecked div). x86 div/idiv trap instead: give the MIPS result in
 * eax/edx and step over the instruction. Each site is logged once. */
static int FixDivide(EXCEPTION_POINTERS *ep) {
    CONTEXT *c = ep->ContextRecord;
    uint8_t *p = (uint8_t *)c->Eip, modrm;
    int mod, rm, len, sgn;
    int opsize16 = 0;
    while (*p == 0x66 || *p == 0x67 || *p == 0x2e || *p == 0x3e || *p == 0x26 || *p == 0x36 ||
           *p == 0x64 || *p == 0x65) {
        if (*p == 0x66)
            opsize16 = 1;
        p++;
    }
    if (*p != 0xf7 || opsize16)
        return 0;
    modrm = p[1];
    if (((modrm >> 3) & 7) != 6 && ((modrm >> 3) & 7) != 7)
        return 0;
    sgn = ((modrm >> 3) & 7) == 7;
    mod = modrm >> 6;
    rm = modrm & 7;
    len = 2;
    if (mod != 3 && rm == 4)
        len++; /* SIB */
    if (mod == 1)
        len += 1;
    else if (mod == 2 || (mod == 0 && rm == 5))
        len += 4;
    if (mod == 0 && rm == 4 && (p[2] & 7) == 5)
        len += 4; /* SIB with no base: disp32 */
    {
        uint32_t dividend = c->Eax;
        if (ep->ExceptionRecord->ExceptionCode == EXCEPTION_INT_OVERFLOW) {
            c->Eax = 0x80000000u;
            c->Edx = 0;
        } else {
            c->Eax = sgn ? ((int32_t)dividend < 0 ? 1u : 0xffffffffu) : 0xffffffffu;
            c->Edx = dividend;
        }
    }
    {
        static DWORD seen[64];
        static int nseen;
        int i, known = 0;
        for (i = 0; i < nseen; i++)
            known |= seen[i] == c->Eip;
        if (!known && nseen < 64) {
            seen[nseen++] = c->Eip;
            PC_Log("division by zero at eip=%08lx given the MIPS result", c->Eip);
        }
    }
    c->Eip = (DWORD)(uintptr_t)(p + len);
    return 1;
}

static LONG CALLBACK CrashHandler(EXCEPTION_POINTERS *ep) {
    EXCEPTION_RECORD *r = ep->ExceptionRecord;
    CONTEXT *c = ep->ContextRecord;
    DWORD code = r->ExceptionCode;
    if ((code == EXCEPTION_INT_DIVIDE_BY_ZERO || code == EXCEPTION_INT_OVERFLOW) && FixDivide(ep))
        return EXCEPTION_CONTINUE_EXECUTION;
    if (code == EXCEPTION_SINGLE_STEP && (c->Dr6 & 1)) {
        /* THPS_WATCH hit: log the write */
        static int hits;
        DWORD *bp = (DWORD *)c->Ebp;
        int i;
        c->Dr6 = 0;
        if (hits++ < 200) {
            fprintf(stderr, "watch: [%08lx] = %08x at eip=%08lx, frame %u; callers", c->Dr0,
                    *(uint32_t *)c->Dr0, c->Eip, pc_heartbeat);
            for (i = 0; i < 6 && bp && !IsBadReadPtr(bp, 8); i++) {
                fprintf(stderr, " %08lx", bp[1]);
                bp = (DWORD *)bp[0];
            }
            fputc('\n', stderr);
        }
        if (!s_fix.active)
            return EXCEPTION_CONTINUE_EXECUTION;
    }
    if (code == EXCEPTION_SINGLE_STEP && s_fix.active) {
        if (s_fix.active == 1)
            *RegPtr(c, s_fix.reg) -= s_fix.bias;
        s_fix.active = 0;
        c->EFlags &= ~0x100u;
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    if (code == EXCEPTION_ACCESS_VIOLATION && r->ExceptionInformation[0] != 8 &&
        r->ExceptionInformation[1] < PSX_RAM_SIZE && !s_fix.active && FixLowAccess(ep))
        return EXCEPTION_CONTINUE_EXECUTION;
    if (code != EXCEPTION_ACCESS_VIOLATION && code != EXCEPTION_ILLEGAL_INSTRUCTION &&
        code != EXCEPTION_INT_DIVIDE_BY_ZERO && code != EXCEPTION_STACK_OVERFLOW &&
        code != EXCEPTION_PRIV_INSTRUCTION)
        return EXCEPTION_CONTINUE_SEARCH;
    PC_CrashHook();
    fprintf(stderr, "\n*** crash: exception %08lx at eip=%08lx", code, c->Eip);
    if (code == EXCEPTION_ACCESS_VIOLATION)
        fprintf(stderr, " (%s %08lx)", r->ExceptionInformation[0] == 8   ? "execute"
                                       : r->ExceptionInformation[0] ? "write"
                                                                    : "read",
                (unsigned long)r->ExceptionInformation[1]);
    fprintf(stderr, "\n    eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx esi=%08lx edi=%08lx\n"
                    "    esp=%08lx ebp=%08lx\n",
            c->Eax, c->Ebx, c->Ecx, c->Edx, c->Esi, c->Edi, c->Esp, c->Ebp);
    {
        /* frame-pointer walk (everything is built with -fno-omit-frame-pointer) */
        DWORD *bp = (DWORD *)c->Ebp;
        int i;
        fprintf(stderr, "    backtrace (run tools/symbolize.py on these):\n      %08lx\n", c->Eip);
        for (i = 0; i < 32 && bp && !IsBadReadPtr(bp, 8); i++) {
            fprintf(stderr, "      %08lx\n", bp[1]);
            bp = (DWORD *)bp[0];
        }
    }
    fflush(stderr);
    if (!getenv("THPS_HEADLESS"))
        MessageBoxA(NULL, "The game crashed; see the console log.", "Tony Hawk's Pro Skater",
                    MB_ICONERROR);
    ExitProcess(3);
    return EXCEPTION_CONTINUE_SEARCH;
}

/* Print the caller's native backtrace (frame-pointer walk) for symbolize.py. */
void PC_Backtrace(const char *why) {
    DWORD *bp = (DWORD *)__builtin_frame_address(0);
    int i;
    fprintf(stderr, "*** %s\n    backtrace:\n", why);
    for (i = 0; i < 24 && bp && !IsBadReadPtr(bp, 8); i++) {
        fprintf(stderr, "      %08lx\n", bp[1]);
        bp = (DWORD *)bp[0];
    }
    fflush(stderr);
}

/* ---- hang watchdog ----------------------------------------------------------------------------- */

volatile uint32_t pc_heartbeat; /* bumped by every presented frame */
static HANDLE s_mainThread;

static void DumpThread(HANDLE th) {
    CONTEXT c;
    DWORD *bp;
    int i;
    memset(&c, 0, sizeof c);
    c.ContextFlags = CONTEXT_FULL;
    SuspendThread(th);
    if (GetThreadContext(th, &c)) {
        fprintf(stderr, "    eip=%08lx\n    backtrace:\n      %08lx\n", c.Eip, c.Eip);
        bp = (DWORD *)c.Ebp;
        for (i = 0; i < 24 && bp && !IsBadReadPtr(bp, 8); i++) {
            fprintf(stderr, "      %08lx\n", bp[1]);
            bp = (DWORD *)bp[0];
        }
    }
    fflush(stderr);
    ResumeThread(th);
}

static DWORD WINAPI Watchdog(LPVOID arg) {
    uint32_t last = 0;
    int stalls = 0;
    (void)arg;
    for (;;) {
        Sleep(5000);
        if (pc_heartbeat != last) {
            last = pc_heartbeat;
            stalls = 0;
            continue;
        }
        if (++stalls > 3)
            continue;
        fprintf(stderr, "\n*** no frame presented for %d s (frame %u)\n", stalls * 5, last);
        DumpThread(s_mainThread);
    }
    return 0;
}

/* ---- entry ---------------------------------------------------------------------------------- */

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    pc_trace = getenv("THPS_TRACE") && atoi(getenv("THPS_TRACE"));
    AddVectoredExceptionHandler(1, CrashHandler);
    DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &s_mainThread, 0,
                    FALSE, DUPLICATE_SAME_ACCESS);
    CreateThread(NULL, 0, Watchdog, NULL, 0, NULL);
    MapPsxMemory();
    LoadExecutable();
    BuildFunctionTable();
    InstallTrampolines();
    PC_GteReset();
    PC_HleInit();
    Crt0();
    if (getenv("THPS_WATCH")) {
        /* hardware write watchpoint on a 4-byte word (debug register 0) */
        CONTEXT c;
        memset(&c, 0, sizeof c);
        c.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        c.Dr0 = strtoul(getenv("THPS_WATCH"), NULL, 16) & ~3u;
        c.Dr7 = 1 | (1u << 16) | (3u << 18); /* L0, write, 4 bytes */
        if (!SetThreadContext(GetCurrentThread(), &c))
            PC_Log("THPS_WATCH: SetThreadContext failed (%lu)", GetLastError());
        else
            PC_Log("watching writes to %08lx", c.Dr0);
    }
    PC_Log("starting game main at %08x", EXE_GAME_MAIN);
    ((int (*)(void))PSX_PTR(EXE_GAME_MAIN))();
    PC_Log("game main returned");
    return 0;
}
