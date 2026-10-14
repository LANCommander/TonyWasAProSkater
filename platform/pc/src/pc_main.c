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

void *PC_Data(uint32_t addr) {
    static int n = -1;
    int lo = 0, hi;
    if (n < 0)
        for (n = 0; pc_data[n].start; n++)
            ;
    hi = n - 1;
    while (lo <= hi) {
        int m = (lo + hi) / 2;
        if (addr < pc_data[m].start)
            hi = m - 1;
        else if (addr >= pc_data[m].end)
            lo = m + 1;
        else
            return pc_data[m].p + (addr - pc_data[m].start);
    }
    return NULL;
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
    uint8_t *e = (uint8_t *)vtable + 8 + 8 * slot;
    void (*dtor)(void *, int) = *(void (**)(void *, int))(e + 4);
    dtor((uint8_t *)obj + *(int16_t *)e, 3);
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
unsigned char Scratchpad[4096] __attribute__((aligned(64)));

#define GAME_HEAP_SIZE (8u << 20)

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

/* The game's static constructors, once (what libsn's __main does on the PS1): the 19 entries of
 * the retail constructor table. */
void PC_RunStaticCtors(void) {
    static int done;
    int i;
    if (done)
        return;
    done = 1;
    for (i = 0; i < 19; i++)
        ((void (**)(void))PC_Data(0x800d101cu))[i]();
}

/* ---- crash diagnostics ---------------------------------------------------------------------- */

/* The interrupted thread's registers, as the fault handlers below see them (Windows CONTEXT,
 * POSIX ucontext): eax ecx edx ebx esp ebp esi edi in x86 encoding order. */
typedef struct {
    uint32_t *r[8];
    uint32_t *eip, *eflags;
} Cpu;

#ifdef _WIN32
static void CpuOf(CONTEXT *c, Cpu *cpu) {
    cpu->r[0] = (uint32_t *)&c->Eax;
    cpu->r[1] = (uint32_t *)&c->Ecx;
    cpu->r[2] = (uint32_t *)&c->Edx;
    cpu->r[3] = (uint32_t *)&c->Ebx;
    cpu->r[4] = (uint32_t *)&c->Esp;
    cpu->r[5] = (uint32_t *)&c->Ebp;
    cpu->r[6] = (uint32_t *)&c->Esi;
    cpu->r[7] = (uint32_t *)&c->Edi;
    cpu->eip = (uint32_t *)&c->Eip;
    cpu->eflags = (uint32_t *)&c->EFlags;
}
#else
static void CpuOf(ucontext_t *uc, Cpu *cpu) {
    greg_t *g = uc->uc_mcontext.gregs;
    cpu->r[0] = (uint32_t *)&g[REG_EAX];
    cpu->r[1] = (uint32_t *)&g[REG_ECX];
    cpu->r[2] = (uint32_t *)&g[REG_EDX];
    cpu->r[3] = (uint32_t *)&g[REG_EBX];
    cpu->r[4] = (uint32_t *)&g[REG_UESP];
    cpu->r[5] = (uint32_t *)&g[REG_EBP];
    cpu->r[6] = (uint32_t *)&g[REG_ESI];
    cpu->r[7] = (uint32_t *)&g[REG_EDI];
    cpu->eip = (uint32_t *)&g[REG_EIP];
    cpu->eflags = (uint32_t *)&g[REG_EFL];
}
#endif

/* On the PS1, 0x00000000-0x001fffff is main RAM too, so the game's NULL-ish pointers read and
 * write harmless low memory. Such an access faults here; it is redone against s_lowpage (64 KB,
 * zero until written) by adding its address to the instruction's base (or index) register,
 * single-stepping it, and taking the bias back off. Each site is logged once. */
#define LOW_PAGE 0x10000u
static uint8_t *s_lowpage;
static struct {
    int active;
    int reg;
    uint32_t bias;
} s_fix;

/* Opcodes (one byte) that take a ModRM byte. */
static int HasModRM1(uint8_t op) {
    if (op < 0x40)
        return (op & 7) < 4;
    return (op >= 0x62 && op <= 0x63) || op == 0x69 || op == 0x6b || (op >= 0x80 && op <= 0x8f) ||
           (op >= 0xc0 && op <= 0xc1) || (op >= 0xc6 && op <= 0xc7) || (op >= 0xd0 && op <= 0xd3) ||
           (op >= 0xd8 && op <= 0xdf) || op == 0xf6 || op == 0xf7 || op == 0xfe || op == 0xff;
}

static int FixLowAccess(Cpu *c, uint32_t addr) {
    uint8_t *p = (uint8_t *)(uintptr_t)*c->eip;
    uint8_t op, modrm;
    int mod, rm, reg, base = -1, index = -1, scale = 1, writesReg = 0, two = 0;
    uint32_t *br;
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
    if (base >= 0 && *c->r[base] < LOW_PAGE)
        s_fix.reg = base, s_fix.bias = (uint32_t)(uintptr_t)s_lowpage;
    else if (index >= 0 && *c->r[index] * (uint32_t)scale < LOW_PAGE)
        s_fix.reg = index, s_fix.bias = (uint32_t)(uintptr_t)s_lowpage / (uint32_t)scale;
    else
        return 0;
    br = c->r[s_fix.reg];
    *br += s_fix.bias;
    s_fix.active = 1;
    if (writesReg && reg == s_fix.reg)
        s_fix.active = 2; /* the instruction replaces the register: nothing to undo */
    *c->eflags |= 0x100;  /* trap flag: come back after this one instruction */
    {
        static uint32_t seen[256];
        static int nseen;
        int i, known = 0;
        for (i = 0; i < nseen; i++)
            known |= seen[i] == *c->eip;
        if (!known && nseen < 256) {
            seen[nseen++] = *c->eip;
            uint32_t *bp = (uint32_t *)(uintptr_t)*c->r[5];
            char callers[64] = "";
            for (i = 0; i < 4 && bp && PC_Readable(bp, 8); i++, bp = (uint32_t *)(uintptr_t)bp[0])
                snprintf(callers + strlen(callers), sizeof callers - strlen(callers), " %08x", bp[1]);
            PC_Log("low-memory access at eip=%08x (address %08x) redirected; callers%s", *c->eip,
                   addr, callers);
        }
    }
    return 1;
}

/* The single step after FixLowAccess: take the bias back off. */
static void FixLowAccessDone(Cpu *c) {
    if (s_fix.active == 1)
        *c->r[s_fix.reg] -= s_fix.bias;
    s_fix.active = 0;
    *c->eflags &= ~0x100u;
}

/* A breakpoint target for debuggers: called once for a genuine crash, before the report. */
void __attribute__((noinline)) PC_CrashHook(void) { __asm__ volatile(""); }

/* The 32-bit operand of the instruction whose ModRM byte is at m. */
static uint32_t Operand(Cpu *c, const uint8_t *m) {
    int mod = m[0] >> 6, rm = m[0] & 7;
    const uint8_t *q = m + 1;
    uint32_t ea = 0, d;
    if (mod == 3)
        return *c->r[rm];
    if (rm == 4) {
        uint8_t sib = *q++;
        int index = (sib >> 3) & 7, base = sib & 7;
        if (index != 4)
            ea += *c->r[index] << (sib >> 6);
        if (base == 5 && mod == 0) {
            memcpy(&d, q, 4);
            ea += d;
            q += 4;
        } else {
            ea += *c->r[base];
        }
    } else if (rm == 5 && mod == 0) {
        memcpy(&ea, q, 4);
        q += 4;
    } else {
        ea = *c->r[rm];
    }
    if (mod == 1) {
        ea += (uint32_t)(int32_t)(int8_t)*q;
    } else if (mod == 2) {
        memcpy(&d, q, 4);
        ea += d;
    }
    return *(const uint32_t *)(uintptr_t)ea;
}

/* The R3000 never traps on division: `div` by zero leaves LO = (dividend < 0 ? 1 : -1) (or all
 * ones unsigned) and HI = dividend, and INT_MIN / -1 gives LO = INT_MIN, HI = 0. The game relies
 * on that (retail uses unchecked div). x86 div/idiv trap instead: give the MIPS result in
 * eax/edx and step over the instruction. overflow: -1 if the caller can't tell a division by
 * zero from INT_MIN / -1 (POSIX reports both as FPE_INTDIV). Each site is logged once. */
static int FixDivide(Cpu *c, int overflow) {
    uint8_t *p = (uint8_t *)(uintptr_t)*c->eip, modrm;
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
    if (overflow < 0)
        overflow = Operand(c, p + 1) != 0;
    {
        uint32_t dividend = *c->r[0];
        if (overflow) {
            *c->r[0] = 0x80000000u;
            *c->r[2] = 0;
        } else {
            *c->r[0] = sgn ? ((int32_t)dividend < 0 ? 1u : 0xffffffffu) : 0xffffffffu;
            *c->r[2] = dividend;
        }
    }
    {
        static uint32_t seen[64];
        static int nseen;
        int i, known = 0;
        for (i = 0; i < nseen; i++)
            known |= seen[i] == *c->eip;
        if (!known && nseen < 64) {
            seen[nseen++] = *c->eip;
            PC_Log("division by zero at eip=%08x given the MIPS result", *c->eip);
        }
    }
    *c->eip = (uint32_t)(uintptr_t)(p + len);
    return 1;
}

/* The crash report: registers and a frame-pointer walk (everything is built with
 * -fno-omit-frame-pointer). */
static void CrashReport(Cpu *c) {
    uint32_t *bp = (uint32_t *)(uintptr_t)*c->r[5];
    int i;
    fprintf(stderr, "\n    eax=%08x ebx=%08x ecx=%08x edx=%08x esi=%08x edi=%08x\n"
                    "    esp=%08x ebp=%08x\n",
            *c->r[0], *c->r[3], *c->r[1], *c->r[2], *c->r[6], *c->r[7], *c->r[4], *c->r[5]);
    fprintf(stderr, "    backtrace (run tools/symbolize.py on these):\n      %08x\n", *c->eip);
    for (i = 0; i < 32 && bp && PC_Readable(bp, 8); i++) {
        fprintf(stderr, "      %08x\n", bp[1]);
        bp = (uint32_t *)(uintptr_t)bp[0];
    }
    fflush(stderr);
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
        FixLowAccessDone(&cpu);
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    if (code == EXCEPTION_ACCESS_VIOLATION && r->ExceptionInformation[0] != 8 &&
        r->ExceptionInformation[1] < LOW_PAGE && !s_fix.active &&
        FixLowAccess(&cpu, (uint32_t)r->ExceptionInformation[1]))
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
    CrashReport(&cpu);
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
        FixLowAccess(&cpu, (uint32_t)(uintptr_t)si->si_addr))
        return;
    PC_CrashHook();
    fprintf(stderr, "\n*** crash: signal %d (%s) at eip=%08x", sig, strsignal(sig), *cpu.eip);
    if (sig == SIGSEGV || sig == SIGBUS)
        fprintf(stderr, " (%s %08x)",
                uc->uc_mcontext.gregs[REG_ERR] & 0x10  ? "execute"
                : uc->uc_mcontext.gregs[REG_ERR] & 2 ? "write"
                                                     : "read",
                (uint32_t)(uintptr_t)si->si_addr);
    CrashReport(&cpu);
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
    uint32_t *bp = (uint32_t *)__builtin_frame_address(0);
    int i;
    fprintf(stderr, "*** %s\n    backtrace:\n", why);
    for (i = 0; i < 24 && bp && PC_Readable(bp, 8); i++) {
        fprintf(stderr, "      %08x\n", bp[1]);
        bp = (uint32_t *)(uintptr_t)bp[0];
    }
    fflush(stderr);
}

/* ---- hang watchdog ----------------------------------------------------------------------------- */

volatile uint32_t pc_heartbeat; /* bumped by every presented frame */

static void DumpStack(uint32_t eip, uint32_t ebp) {
    uint32_t *bp = (uint32_t *)(uintptr_t)ebp;
    int i;
    fprintf(stderr, "    eip=%08x\n    backtrace:\n      %08x\n", eip, eip);
    for (i = 0; i < 24 && bp && PC_Readable(bp, 8); i++) {
        fprintf(stderr, "      %08x\n", bp[1]);
        bp = (uint32_t *)(uintptr_t)bp[0];
    }
    fflush(stderr);
}

#ifdef _WIN32
static HANDLE s_mainThread;

static void DumpMainThread(void) {
    CONTEXT c;
    memset(&c, 0, sizeof c);
    c.ContextFlags = CONTEXT_FULL;
    SuspendThread(s_mainThread);
    if (GetThreadContext(s_mainThread, &c))
        DumpStack(c.Eip, c.Ebp);
    ResumeThread(s_mainThread);
}
#else
static pthread_t s_mainThread;

/* SIGUSR2 to the main thread: it prints where it is itself */
static void DumpSignal(int sig, siginfo_t *si, void *ctx) {
    ucontext_t *uc = ctx;
    (void)sig;
    (void)si;
    DumpStack((uint32_t)uc->uc_mcontext.gregs[REG_EIP], (uint32_t)uc->uc_mcontext.gregs[REG_EBP]);
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
        c.Dr0 = strtoul(getenv("THPS_WATCH"), NULL, 16) & ~3u;
        c.Dr7 = 1 | (1u << 16) | (3u << 18); /* L0, write, 4 bytes */
        if (!SetThreadContext(GetCurrentThread(), &c))
            PC_Log("THPS_WATCH: SetThreadContext failed (%lu)", GetLastError());
        else
            PC_Log("watching writes to %08lx", c.Dr0);
#else
        PC_Log("THPS_WATCH: only on Windows (use a gdb watchpoint)");
#endif
    }
    PC_Log("starting the game");
    psx_80026d44();
    PC_Log("game main returned");
    return 0;
}
