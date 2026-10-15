/* Boot: crash and hang diagnostics, the host devices, then the game's main(). See
 * platform/pc/README.md. */
#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#else
#define _GNU_SOURCE
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <ucontext.h>
#include <unistd.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SDL_MAIN_HANDLED /* this file has main: SDL_SetMainReady in PlatformInit */
#include <SDL2/SDL.h>

#include "pc_assets.h"
#include "pc_gamedata.h"
#include "pc_geom.h"
#include "pc_input.h"
#include "pc_platform.h"
#include "pc_render3d.h"
#include "pc_runtime.h"
#include "pc_sound.h"
#include "pc_video.h"

typedef uint32_t u32;


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

/* The game's own debug output (its printf and puts calls; build.py NATIVE_RENAME): THPS_TRACE=1. */
int PC_GameDebugf(const char *fmt, ...) {
    va_list ap;
    int n;
    if (!pc_trace)
        return 0;
    va_start(ap, fmt);
    n = vfprintf(stdout, fmt, ap);
    va_end(ap);
    fflush(stdout);
    return n;
}

int PC_GameDebugPuts(const char *s) {
    if (pc_trace)
        puts(s);
    return 0;
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
        PC_ErrorBox(buf);
    PC_ExitProcess(1);
}

extern const char *PC_SymbolName(uint32_t addr); /* generated: names.c */

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

/* ---- C++ delete (see link.py, "destructor thunks") -------------------------------------------- */

void *pc_delete_marked, *pc_delete_freed;

void pc_vdelete(void *vtable, int slot, void *obj) {
    /* an egcs vtable: {delta, index, function} entries after an all-zero one */
    const struct {
        int16_t delta, index;
        void (*fn)(void *, int);
    } *e = (const void *)vtable;
    e[1 + slot].fn((uint8_t *)obj + e[1 + slot].delta, 3);
}

void pc_delete_mark(void *p) {
    pc_delete_marked = p;
    pc_delete_freed = NULL;
}

/* operator delete for a class without an allocator of its own. Retail's (libsn) freed into the
 * BIOS heap, which the game never allocates from: there's nothing to free. */
void pc_delete_unowned(void *p) {
    if (p)
        PC_Trace("delete %p: no allocator owns it", p);
}

/* ---- memory map ----------------------------------------------------------------------------- */

/* The PS1's scratchpad, the 1 KB of fast RAM the game uses as working memory (game/scratchpad.h);
 * 4 KB, as code that ran past its end on the PS1 reached I/O registers. */
unsigned char Scratchpad[4096] __attribute__((aligned(4096))); /* aligned as the PS1's (0x1f800000): the game's code relies on it */
/* the pointers the game keeps in the scratchpad, by retail offset / 4 (game/scratchpad.h) */
uintptr_t ScratchPointers[1024 / 4];

/* (16 MB in a 64-bit build: its pointers, and so its structures and packets, are bigger) */
#define GAME_HEAP_SIZE ((sizeof(void *) == 8 ? 16u : 8u) << 20)

void *PC_GameHeap(uint32_t *size)
{
    static void *heap;
    if (!heap && !(heap = calloc(1, GAME_HEAP_SIZE)))
        PC_Fatal("cannot allocate the game's %u MB of memory", GAME_HEAP_SIZE >> 20);
    *size = GAME_HEAP_SIZE;
    return heap;
}

const char *PC_DataDir(void) { return s_dataDir; }

void (*pc_hires_before_draw)(void);

#ifndef _WIN32
/* $XDG_DATA_HOME/Neversoft/Tony Hawk's Pro Skater (~/.local/share/...) */
const char *PC_UserDir(void) {
    static char dir[MAX_PATH];
    if (!dir[0]) {
        const char *xdg = getenv("XDG_DATA_HOME"), *home = getenv("HOME");
        if (xdg && xdg[0] == '/')
            snprintf(dir, sizeof dir, "%s/Neversoft/Tony Hawk's Pro Skater", xdg);
        else
            snprintf(dir, sizeof dir, "%s/.local/share/Neversoft/Tony Hawk's Pro Skater",
                     home && *home ? home : ".");
        PC_MakeDirs(dir);
    }
    return dir;
}
#else
const char *PC_UserDir(void) {
    static char dir[MAX_PATH];
    if (!dir[0]) {
        /* FOLDERID_SavedGames, spelled out so we don't depend on libuuid */
        static const GUID kSavedGames = {0x4c5c32ff, 0xbb9d, 0x43b0,
                                         {0xb5, 0xb4, 0x2d, 0x72, 0xe5, 0x4e, 0xaa, 0xa4}};
        char home[MAX_PATH], *s;
        PWSTR wide = NULL;
        home[0] = 0;
        if (SHGetKnownFolderPath(&kSavedGames, KF_FLAG_CREATE, NULL, &wide) == S_OK &&
            !WideCharToMultiByte(CP_ACP, 0, wide, -1, home, sizeof home, NULL, NULL))
            home[0] = 0;
        CoTaskMemFree(wide);
        if (!home[0]) {
            if (getenv("USERPROFILE"))
                snprintf(home, sizeof home, "%s\\Saved Games", getenv("USERPROFILE"));
            else if (SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, home) == S_OK)
                strncat(home, "\\Saved Games", sizeof home - strlen(home) - 1);
            else
                strcpy(home, ".");
        }
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
#endif

/* The game's static constructors, once (what libsn's __main does on the PS1): the constructor
 * entries of the retail table. */
void PC_RunStaticCtors(void) {
    static int done;
    int i;
    if (done)
        return;
    done = 1;
    for (i = 0; i < GAME_CTORS; i++)
        GameCtorList[i]();
}

/* ---- crash diagnostics ---------------------------------------------------------------------- */

/* The interrupted thread's registers, as the fault handlers below see them (Windows CONTEXT,
 * POSIX ucontext), in x86 encoding order: ax cx dx bx sp bp si di (r8-r15 on x64). */
typedef uintptr_t Reg;
#ifdef __x86_64__
#define NREGS 16
#else
#define NREGS 8
#endif
typedef struct {
    Reg *r[NREGS];
    Reg *ip;
    uint32_t *flags;
} Cpu;

#ifdef _WIN32
#ifdef __x86_64__
#define CTX_IP(c) ((c)->Rip)
#define CTX_BP(c) ((c)->Rbp)
#else
#define CTX_IP(c) ((c)->Eip)
#define CTX_BP(c) ((c)->Ebp)
#endif
static void CpuOf(CONTEXT *c, Cpu *cpu) {
#ifdef __x86_64__
    DWORD64 *g[NREGS] = {&c->Rax, &c->Rcx, &c->Rdx, &c->Rbx, &c->Rsp, &c->Rbp, &c->Rsi, &c->Rdi,
                         &c->R8,  &c->R9,  &c->R10, &c->R11, &c->R12, &c->R13, &c->R14, &c->R15};
#else
    DWORD *g[NREGS] = {&c->Eax, &c->Ecx, &c->Edx, &c->Ebx, &c->Esp, &c->Ebp, &c->Esi, &c->Edi};
#endif
    int i;
    for (i = 0; i < NREGS; i++)
        cpu->r[i] = (Reg *)g[i];
    cpu->ip = (Reg *)&CTX_IP(c);
    cpu->flags = (uint32_t *)&c->EFlags;
}
#else
#ifdef __x86_64__
#define UC_IP REG_RIP
#define UC_BP REG_RBP
static const int kRegs[NREGS] = {REG_RAX, REG_RCX, REG_RDX, REG_RBX, REG_RSP, REG_RBP,
                                 REG_RSI, REG_RDI, REG_R8,  REG_R9,  REG_R10, REG_R11,
                                 REG_R12, REG_R13, REG_R14, REG_R15};
#else
#define UC_IP REG_EIP
#define UC_BP REG_EBP
static const int kRegs[NREGS] = {REG_EAX, REG_ECX, REG_EDX, REG_EBX,
                                 REG_UESP, REG_EBP, REG_ESI, REG_EDI};
#endif
static void CpuOf(ucontext_t *uc, Cpu *cpu) {
    greg_t *g = uc->uc_mcontext.gregs;
    int i;
    for (i = 0; i < NREGS; i++)
        cpu->r[i] = (Reg *)&g[kRegs[i]];
    cpu->ip = (Reg *)&g[UC_IP];
    cpu->flags = (uint32_t *)&g[REG_EFL];
}
#endif

/* An instruction's prefixes: legacy ones skipped, REX (x64) decoded. */
typedef struct {
    const uint8_t *p; /* the opcode */
    int opsize16, rex;
} Prefixes;

static Prefixes ReadPrefixes(const uint8_t *p) {
    Prefixes x = {0, 0, 0};
    while (*p == 0x66 || *p == 0x67 || *p == 0xf2 || *p == 0xf3 || *p == 0x2e || *p == 0x3e ||
           *p == 0x26 || *p == 0x36 || *p == 0x64 || *p == 0x65) {
        if (*p == 0x66)
            x.opsize16 = 1;
        p++;
    }
#ifdef __x86_64__
    if ((*p & 0xf0) == 0x40)
        x.rex = *p++;
#endif
    x.p = p;
    return x;
}

/* A ModRM memory operand: base and index registers (-1: none), scale, displacement, whether it
 * is RIP-relative, and the length of ModRM + SIB + displacement. */
typedef struct {
    int base, index, scale, riprel, len;
    int32_t disp;
} MemOperand;

static int DecodeModRM(const uint8_t *m, int rex, MemOperand *o) {
    int mod = m[0] >> 6, rm = m[0] & 7;
    const uint8_t *q = m + 1;
    o->base = o->index = -1;
    o->scale = 1;
    o->riprel = 0;
    o->disp = 0;
    if (mod == 3) {
        o->base = rm | (rex & 1) << 3;
        o->len = 1;
        return 0; /* a register */
    }
    if (rm == 4) {
        uint8_t sib = *q++;
        int index = ((sib >> 3) & 7) | (rex & 2) << 2, base = sib & 7;
        o->scale = 1 << (sib >> 6);
        if (index != 4)
            o->index = index;
        if (base == 5 && mod == 0) {
            memcpy(&o->disp, q, 4);
            q += 4;
        } else {
            o->base = base | (rex & 1) << 3;
        }
    } else if (rm == 5 && mod == 0) {
#ifdef __x86_64__
        o->riprel = 1;
#endif
        memcpy(&o->disp, q, 4);
        q += 4;
    } else {
        o->base = rm | (rex & 1) << 3;
    }
    if (mod == 1) {
        o->disp = (int8_t)*q++;
    } else if (mod == 2) {
        memcpy(&o->disp, q, 4);
        q += 4;
    }
    o->len = (int)(q - m);
    return 1;
}

/* The return addresses up the stack, for symbolize.py: a frame-pointer walk (everything is built
 * with -fno-omit-frame-pointer), or on 64-bit Windows, whose frame pointers make no chain, the
 * unwind tables from the context `wctx` (a CONTEXT *; NULL: walk from ip/fp). The first is ip.
 * -> how many. */
static int StackWalk(Reg ip, Reg fp, void *wctx, Reg *out, int max) {
    Reg *bp = (Reg *)fp;
    int n = 0;
#if defined(_WIN32) && defined(__x86_64__)
    if (wctx) {
        CONTEXT c = *(CONTEXT *)wctx;
        while (n < max && c.Rip) {
            DWORD64 base, frame;
            PRUNTIME_FUNCTION f;
            void *data;
            out[n++] = c.Rip;
            f = RtlLookupFunctionEntry(c.Rip, &base, NULL);
            if (!f) { /* a leaf function: the return address is on top of the stack */
                if (!PC_Readable((void *)c.Rsp, 8))
                    break;
                c.Rip = *(DWORD64 *)c.Rsp;
                c.Rsp += 8;
                continue;
            }
            RtlVirtualUnwind(UNW_FLAG_NHANDLER, base, c.Rip, f, &c, &data, &frame, NULL);
        }
        return n;
    }
#else
    (void)wctx;
#endif
    if (ip && n < max)
        out[n++] = ip;
    while (n < max && bp && PC_Readable(bp, 2 * sizeof(Reg))) {
        out[n++] = bp[1];
        bp = (Reg *)bp[0];
    }
    return n;
}

static void Backtrace(Reg ip, Reg fp, int depth, void *wctx) {
    Reg a[64];
    int i, n = StackWalk(ip, fp, wctx, a, depth + 1 < 64 ? depth + 1 : 64);
    for (i = 0; i < n; i++)
        fprintf(stderr, "      %08llx\n", (unsigned long long)a[i]);
    fflush(stderr);
}

/* On the PS1, 0x00000000-0x001fffff is main RAM too, so the game's NULL-ish pointers read and
 * write harmless low memory. Such an access faults here; it is redone against s_lowpage (64 KB,
 * zero until written) by adding its address to the instruction's base (or index) register,
 * single-stepping it, and taking the bias back off. Each site is logged once. */
#define LOW_PAGE 0x10000u
static uint8_t *s_lowpage;
static struct {
    int active;
    int reg;
    Reg bias;
} s_fix;

/* Opcodes (one byte) that take a ModRM byte. */
static int HasModRM1(uint8_t op) {
    if (op < 0x40)
        return (op & 7) < 4;
    return (op >= 0x62 && op <= 0x63) || op == 0x69 || op == 0x6b || (op >= 0x80 && op <= 0x8f) ||
           (op >= 0xc0 && op <= 0xc1) || (op >= 0xc6 && op <= 0xc7) || (op >= 0xd0 && op <= 0xd3) ||
           (op >= 0xd8 && op <= 0xdf) || op == 0xf6 || op == 0xf7 || op == 0xfe || op == 0xff;
}

static int FixLowAccess(Cpu *c, uintptr_t addr, void *wctx) {
    Prefixes x = ReadPrefixes((const uint8_t *)*c->ip);
    const uint8_t *p = x.p;
    uint8_t op;
    int reg, writesReg = 0, two = 0;
    MemOperand mo;
    Reg *br;
    op = *p++;
    if (op == 0x0f) {
        two = 1;
        op = *p++;
    } else if (!HasModRM1(op)) {
        return 0;
    }
    reg = ((p[0] >> 3) & 7) | (x.rex & 4) << 1;
    if (!DecodeModRM(p, x.rex, &mo) || mo.riprel)
        return 0;
    /* loads into a general register overwrite `reg`; don't restore it then */
    if (!two)
        writesReg = op == 0x8a || op == 0x8b || (op < 0x40 && (op & 7) >= 2 && (op & 7) <= 3 &&
                                                 (op & 0x38) != 0x38);
    else
        writesReg = op == 0xb6 || op == 0xb7 || op == 0xbe || op == 0xbf || op == 0xaf;
    if (mo.base >= 0 && *c->r[mo.base] < LOW_PAGE)
        s_fix.reg = mo.base, s_fix.bias = (Reg)s_lowpage;
    else if (mo.index >= 0 && *c->r[mo.index] * (Reg)mo.scale < LOW_PAGE)
        s_fix.reg = mo.index, s_fix.bias = (Reg)s_lowpage / (Reg)mo.scale;
    else
        return 0;
    br = c->r[s_fix.reg];
    *br += s_fix.bias;
    s_fix.active = 1;
    if (writesReg && reg == s_fix.reg)
        s_fix.active = 2; /* the instruction replaces the register: nothing to undo */
    *c->flags |= 0x100;   /* trap flag: come back after this one instruction */
    {
        static Reg seen[256];
        static int nseen;
        int i, known = 0;
        for (i = 0; i < nseen; i++)
            known |= seen[i] == *c->ip;
        if (!known && nseen < 256) {
            Reg a[5];
            char callers[128] = "";
            int n = StackWalk(*c->ip, *c->r[5], wctx, a, 5);
            seen[nseen++] = *c->ip;
            for (i = 1; i < n; i++)
                snprintf(callers + strlen(callers), sizeof callers - strlen(callers), " %08llx",
                         (unsigned long long)a[i]);
            PC_Log("low-memory access at ip=%08llx (address %08llx) redirected; callers%s",
                   (unsigned long long)*c->ip, (unsigned long long)addr, callers);
        }
    }
    return 1;
}

/* The single step after FixLowAccess: take the bias back off. */
static void FixLowAccessDone(Cpu *c) {
    if (s_fix.active == 1)
        *c->r[s_fix.reg] -= s_fix.bias;
    s_fix.active = 0;
    *c->flags &= ~0x100u;
}

/* A breakpoint target for debuggers: called once for a genuine crash, before the report. */
void __attribute__((noinline)) PC_CrashHook(void) { __asm__ volatile(""); }

/* The R3000 never traps on division: `div` by zero leaves LO = (dividend < 0 ? 1 : -1) (or all
 * ones unsigned) and HI = dividend, and INT_MIN / -1 gives LO = INT_MIN, HI = 0. The game relies
 * on that (retail uses unchecked div). x86 div/idiv trap instead: give the MIPS result in
 * eax/edx (rax/rdx for a 64-bit division) and step over the instruction. overflow: -1 if the
 * caller can't tell a division by zero from INT_MIN / -1 (POSIX reports both as FPE_INTDIV). Each
 * site is logged once. */
static int FixDivide(Cpu *c, int overflow) {
    Prefixes x = ReadPrefixes((const uint8_t *)*c->ip);
    const uint8_t *p = x.p;
    int sgn, wide = (x.rex & 8) != 0;
    MemOperand mo;
    if (*p != 0xf7 || x.opsize16)
        return 0;
    if (((p[1] >> 3) & 7) != 6 && ((p[1] >> 3) & 7) != 7)
        return 0;
    sgn = ((p[1] >> 3) & 7) == 7;
    DecodeModRM(p + 1, x.rex, &mo);
    if (overflow < 0) {
        /* the divisor: a register or memory */
        uint64_t d;
        if ((p[1] >> 6) == 3) {
            d = *c->r[mo.base];
        } else {
            uintptr_t ea = (uintptr_t)(intptr_t)mo.disp;
            if (mo.riprel)
                ea += (uintptr_t)(p + 1 + mo.len);
            if (mo.base >= 0)
                ea += *c->r[mo.base];
            if (mo.index >= 0)
                ea += *c->r[mo.index] * mo.scale;
            d = wide ? *(const uint64_t *)ea : *(const uint32_t *)ea;
        }
        overflow = (wide ? d : (uint32_t)d) != 0;
    }
    if (wide) {
        uint64_t dividend = *c->r[0];
        if (overflow) {
            *c->r[0] = (Reg)1 << (8 * sizeof(Reg) - 1);
            *c->r[2] = 0;
        } else {
            *c->r[0] = sgn && (int64_t)dividend < 0 ? 1 : ~(uint64_t)0;
            *c->r[2] = dividend;
        }
    } else {
        /* (32-bit results zero the upper halves, as x64 does) */
        uint32_t dividend = (uint32_t)*c->r[0];
        if (overflow) {
            *c->r[0] = 0x80000000u;
            *c->r[2] = 0;
        } else {
            *c->r[0] = sgn ? ((int32_t)dividend < 0 ? 1u : 0xffffffffu) : 0xffffffffu;
            *c->r[2] = dividend;
        }
    }
    {
        static Reg seen[64];
        static int nseen;
        int i, known = 0;
        for (i = 0; i < nseen; i++)
            known |= seen[i] == *c->ip;
        if (!known && nseen < 64) {
            seen[nseen++] = *c->ip;
            PC_Log("division by zero at ip=%08llx given the MIPS result",
                   (unsigned long long)*c->ip);
        }
    }
    *c->ip = (Reg)(p + 1 + mo.len);
    return 1;
}

/* The crash report: registers and a backtrace. */
static void CrashReport(Cpu *c, void *wctx) {
    static const char *const names[16] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di",
                                          "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"};
    int i;
    fprintf(stderr, "\n   ");
    for (i = 0; i < NREGS; i++)
        fprintf(stderr, " %s=%0*llx%s", names[i], (int)(2 * sizeof(Reg)),
                (unsigned long long)*c->r[i], i % 6 == 5 ? "\n   " : "");
    fprintf(stderr, "\n    backtrace (run tools/symbolize.py on these):\n");
    Backtrace(*c->ip, *c->r[5], 32, wctx);
    if (!getenv("THPS_HEADLESS"))
        PC_ErrorBox("The game crashed; see the console log.");
    PC_ExitProcess(3);
}

#ifdef _WIN32
static LONG CALLBACK CrashHandler(EXCEPTION_POINTERS *ep) {
    EXCEPTION_RECORD *r = ep->ExceptionRecord;
    CONTEXT *c = ep->ContextRecord;
    DWORD code = r->ExceptionCode;
    Cpu cpu;
    CpuOf(c, &cpu);
    if ((code == EXCEPTION_INT_DIVIDE_BY_ZERO || code == EXCEPTION_INT_OVERFLOW) &&
        FixDivide(&cpu, code == EXCEPTION_INT_OVERFLOW))
        return EXCEPTION_CONTINUE_EXECUTION;
    if (code == EXCEPTION_SINGLE_STEP && (c->Dr6 & 1)) {
        /* THPS_WATCH hit: log the write */
        static int hits;
        c->Dr6 = 0;
        if (hits++ < 200) {
            fprintf(stderr, "watch: [%08llx] = %08x at ip=%08llx, frame %u; callers\n",
                    (unsigned long long)c->Dr0, *(uint32_t *)c->Dr0,
                    (unsigned long long)CTX_IP(c), pc_heartbeat);
            Backtrace(0, CTX_BP(c), 6, NULL);
        }
        if (!s_fix.active)
            return EXCEPTION_CONTINUE_EXECUTION;
    }
    if (code == EXCEPTION_SINGLE_STEP && s_fix.active) {
        FixLowAccessDone(&cpu);
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    if (code == EXCEPTION_ACCESS_VIOLATION && r->ExceptionInformation[0] != 8 &&
        r->ExceptionInformation[1] < LOW_PAGE && !s_fix.active &&
        FixLowAccess(&cpu, (uintptr_t)r->ExceptionInformation[1], c))
        return EXCEPTION_CONTINUE_EXECUTION;
    if (code != EXCEPTION_ACCESS_VIOLATION && code != EXCEPTION_ILLEGAL_INSTRUCTION &&
        code != EXCEPTION_INT_DIVIDE_BY_ZERO && code != EXCEPTION_STACK_OVERFLOW &&
        code != EXCEPTION_PRIV_INSTRUCTION)
        return EXCEPTION_CONTINUE_SEARCH;
    PC_CrashHook();
    fprintf(stderr, "\n*** crash: exception %08lx at ip=%08llx", code,
            (unsigned long long)CTX_IP(c));
    if (code == EXCEPTION_ACCESS_VIOLATION)
        fprintf(stderr, " (%s %08llx)", r->ExceptionInformation[0] == 8   ? "execute"
                                        : r->ExceptionInformation[0] ? "write"
                                                                     : "read",
                (unsigned long long)r->ExceptionInformation[1]);
    CrashReport(&cpu, c);
    return EXCEPTION_CONTINUE_SEARCH;
}
#else
static void CrashSignal(int sig, siginfo_t *si, void *ctx) {
    ucontext_t *uc = ctx;
    Cpu cpu;
    CpuOf(uc, &cpu);
    if (sig == SIGFPE && (si->si_code == FPE_INTDIV || si->si_code == FPE_INTOVF) &&
        FixDivide(&cpu, si->si_code == FPE_INTOVF ? 1 : -1))
        return;
    if (sig == SIGTRAP && s_fix.active) {
        FixLowAccessDone(&cpu);
        return;
    }
    /* page-fault error code bit 4: instruction fetch */
    if ((sig == SIGSEGV || sig == SIGBUS) && !(uc->uc_mcontext.gregs[REG_ERR] & 0x10) &&
        (uintptr_t)si->si_addr < LOW_PAGE && !s_fix.active &&
        FixLowAccess(&cpu, (uintptr_t)si->si_addr, NULL))
        return;
    PC_CrashHook();
    fprintf(stderr, "\n*** crash: signal %d (%s) at ip=%08llx", sig, strsignal(sig),
            (unsigned long long)*cpu.ip);
    if (sig == SIGSEGV || sig == SIGBUS)
        fprintf(stderr, " (%s %08llx)",
                uc->uc_mcontext.gregs[REG_ERR] & 0x10  ? "execute"
                : uc->uc_mcontext.gregs[REG_ERR] & 2 ? "write"
                                                     : "read",
                (unsigned long long)(uintptr_t)si->si_addr);
    CrashReport(&cpu, NULL);
}

static void InstallCrashHandler(void) {
    static uint8_t altstack[64 * 1024]; /* the report still runs after a stack overflow */
    stack_t ss;
    struct sigaction sa;
    int sigs[] = {SIGSEGV, SIGBUS, SIGFPE, SIGILL, SIGTRAP};
    unsigned i;
    ss.ss_sp = altstack;
    ss.ss_size = sizeof altstack;
    ss.ss_flags = 0;
    sigaltstack(&ss, NULL);
    memset(&sa, 0, sizeof sa);
    sa.sa_sigaction = CrashSignal;
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_NODEFER;
    sigemptyset(&sa.sa_mask);
    for (i = 0; i < sizeof sigs / sizeof sigs[0]; i++)
        sigaction(sigs[i], &sa, NULL);
}
#endif

/* Print the caller's native backtrace (frame-pointer walk) for symbolize.py. */
void PC_Backtrace(const char *why) {
    fprintf(stderr, "*** %s\n    backtrace:\n", why);
#if defined(_WIN32) && defined(__x86_64__)
    {
        CONTEXT c;
        RtlCaptureContext(&c);
        Backtrace(0, 0, 24, &c);
    }
#else
    Backtrace(0, (Reg)__builtin_frame_address(0), 24, NULL);
#endif
}

/* ---- hang watchdog ----------------------------------------------------------------------------- */

volatile uint32_t pc_heartbeat; /* bumped by every presented frame */

static void DumpStack(Reg ip, Reg fp, void *wctx) {
    fprintf(stderr, "    ip=%08llx\n    backtrace:\n", (unsigned long long)ip);
    Backtrace(ip, fp, 24, wctx);
}

#ifdef _WIN32
static HANDLE s_mainThread;

static void DumpMainThread(void) {
    CONTEXT c;
    memset(&c, 0, sizeof c);
    c.ContextFlags = CONTEXT_FULL;
    SuspendThread(s_mainThread);
    if (GetThreadContext(s_mainThread, &c))
        DumpStack(CTX_IP(&c), CTX_BP(&c), &c);
    ResumeThread(s_mainThread);
}
#else
static pthread_t s_mainThread;

/* SIGUSR2 to the main thread: it prints where it is itself */
static void DumpSignal(int sig, siginfo_t *si, void *ctx) {
    ucontext_t *uc = ctx;
    (void)sig;
    (void)si;
    DumpStack((Reg)uc->uc_mcontext.gregs[UC_IP], (Reg)uc->uc_mcontext.gregs[UC_BP], NULL);
}

static void DumpMainThread(void) { pthread_kill(s_mainThread, SIGUSR2); }
#endif

static void Watchdog(void) {
    uint32_t last = 0;
    int stalls = 0;
    for (;;) {
        PC_SleepMs(5000);
        if (pc_heartbeat != last) {
            last = pc_heartbeat;
            stalls = 0;
            continue;
        }
        if (++stalls > 3)
            continue;
        fprintf(stderr, "\n*** no frame presented for %d s (frame %u)\n", stalls * 5, last);
        DumpMainThread();
    }
}

#ifdef _WIN32
static DWORD WINAPI WatchdogThread(LPVOID arg) {
    (void)arg;
    Watchdog();
    return 0;
}

static void StartWatchdog(void) {
    DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &s_mainThread, 0,
                    FALSE, DUPLICATE_SAME_ACCESS);
    CreateThread(NULL, 0, WatchdogThread, NULL, 0, NULL);
}
#else
static void *WatchdogThread(void *arg) {
    (void)arg;
    Watchdog();
    return NULL;
}

static void StartWatchdog(void) {
    struct sigaction sa;
    pthread_t th;
    sigset_t block, old;
    s_mainThread = pthread_self();
    memset(&sa, 0, sizeof sa);
    sa.sa_sigaction = DumpSignal;
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR2, &sa, NULL);
    /* the watchdog takes no signals meant for the game thread */
    sigfillset(&block);
    pthread_sigmask(SIG_BLOCK, &block, &old);
    if (!pthread_create(&th, NULL, WatchdogThread, NULL))
        pthread_detach(th);
    pthread_sigmask(SIG_SETMASK, &old, NULL);
}
#endif

/* Bring up the host devices before the game runs: the extracted data (from the disc on first
 * startup), window/GPU, input, sound. */
static void PlatformInit(void) {
    int scale = getenv("THPS_SCALE") ? atoi(getenv("THPS_SCALE")) : 3;
    SDL_SetMainReady();
    PC_AssetsInit();
    PC_VideoInit(scale > 0 ? scale : 3);
    if (PC_AssetsDreamcast() && !R3d_Native())
        PC_Log("note: the Dreamcast models and textures need the native 3D renderer (not "
               "THPS_RENDER3D=psx): they will look wrong");
    PC_InputInit();
    PC_SoundInit();
}

/* ---- entry ---------------------------------------------------------------------------------- */

int psx_80026d44(void); /* the game's main() */

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    pc_trace = getenv("THPS_TRACE") && atoi(getenv("THPS_TRACE"));
#ifdef _WIN32
    /* a release build has no console: started from Explorer, the log (crash backtraces too) goes
     * to thps.log in the player's folder */
    if (!GetStdHandle(STD_ERROR_HANDLE)) {
        char log[MAX_PATH];
        snprintf(log, sizeof log, "%s\\thps.log", PC_UserDir());
        freopen(log, "w", stderr);
    }
    AddVectoredExceptionHandler(1, CrashHandler);
#else
    InstallCrashHandler();
#endif
    StartWatchdog();
    PC_ExeDir(s_dataDir, sizeof s_dataDir);
    {
        /* the low page: aligned to its size, so the index-register bias divides exactly */
        uint8_t *p = calloc(2, LOW_PAGE);
        if (!p)
            PC_Fatal("out of memory");
        s_lowpage = (uint8_t *)(((uintptr_t)p + LOW_PAGE - 1) & ~(uintptr_t)(LOW_PAGE - 1));
    }
    PC_GeomReset();
    PlatformInit();
    if (getenv("THPS_WATCH")) {
#ifdef _WIN32
        /* hardware write watchpoint on a 4-byte word (debug register 0) */
        CONTEXT c;
        memset(&c, 0, sizeof c);
        c.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        c.Dr0 = strtoull(getenv("THPS_WATCH"), NULL, 16) & ~(uintptr_t)3;
        c.Dr7 = 1 | (1u << 16) | (3u << 18); /* L0, write, 4 bytes */
        if (!SetThreadContext(GetCurrentThread(), &c))
            PC_Log("THPS_WATCH: SetThreadContext failed (%lu)", GetLastError());
        else
            PC_Log("watching writes to %08llx", (unsigned long long)c.Dr0);
#else
        PC_Log("THPS_WATCH: only on Windows (use a gdb watchpoint)");
#endif
    }
    PC_Log("starting the game");
    psx_80026d44();
    PC_Log("game main returned");
    return 0;
}
