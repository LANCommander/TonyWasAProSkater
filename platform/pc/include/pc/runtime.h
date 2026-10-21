/* The port's runtime proper (pc_main.cpp): boot, the game's memory and folders, its static
 * constructors and missing functions (Runtime), and the crash, low-memory fault and hang
 * diagnostics (Diagnostics). The C interface is pc_runtime.h; logging (PC_Log, PC_Trace,
 * PC_Fatal) stays free functions there. */
#pragma once

#include <stdint.h>

#ifndef _WIN32
#include <pthread.h>
#endif

#include "pc/platform.h"

namespace pc {

class Runtime {
public:
    /* (16 MB in a 64-bit build: its pointers, and so its structures and packets, are bigger) */
    static constexpr uint32_t kGameHeapSize = (sizeof(void *) == 8 ? 16u : 8u) << 20;
    /* more when mods add skaters or levels (pc/mod_content.h) */
    static constexpr uint32_t kModHeapSize = (sizeof(void *) == 8 ? 16u : 8u) << 20;

    /* The process: diagnostics, the host devices, then the game's Runtime::Main(). */
    int Main();

    /* The memory the game's allocator manages (Init_ResetMemory: its heaps, Mem_NewX): one block
     * from the system, allocated on first use. */
    void *GameHeap(uint32_t *size);
    /* Directory holding the disc image / game files: the executable's. */
    const char *DataDir() const { return m_dataDir; }
    /* The player's folder (created if missing): Settings.ini and the Saves folder. */
    const char *UserDir();
    /* The game's static constructors, once (what libsn's __main does on the PS1). */
    void RunStaticCtors();
    /* main()'s arguments, for Restart. */
    void SetArgs(char **argv) { m_argv = argv; }
    /* PC_RequestRestart: quit at the end of the frame, then start the game again (Restart). */
    void RequestRestart();
    bool RestartRequested() const { return m_restart; }
    void Restart() __attribute__((noreturn));
    /* A generated stub for a function with no implementation was called: log it (once). */
    uint32_t MissingFunction(uint32_t addr);
    /* Name of a PS1 code address, for logs ("FUN_8002ab20", "DrawOTag", ...). */
    const char *NameOf(uint32_t addr);

private:
    void PlatformInit();

    char m_dataDir[MAX_PATH] = "";
    char **m_argv = nullptr;
    bool m_restart = false;
    char m_userDir[MAX_PATH] = "";
    void *m_heap = nullptr;
    bool m_ctorsDone = false;
    uint32_t m_missingSeen[512] = {};
    int m_missingCount = 0;
    char m_nameBuf[4][64] = {};
    int m_nameSlot = 0;
};

extern Runtime g_runtime;

/* The interrupted thread's registers, as the fault handlers see them (pc_main.cpp). */
struct CpuState;

/* Crash reports, the PS1 behaviours the game relies on that fault on x86 (low-memory accesses,
 * division by zero), THPS_WATCH, and the hang watchdog. Its state is touched from the fault
 * handlers and the watchdog thread: raw storage only. */
class Diagnostics {
public:
    using Reg = uintptr_t;
    /* the redirected low memory: 0 - LOW_PAGE */
    static constexpr uintptr_t kLowPage = 0x10000u;

    /* The fault handler (Windows: a vectored exception handler; POSIX: signal handlers on an
     * alternate stack). */
    void InstallCrashHandler();
    /* The hang watchdog thread: a backtrace of the Runtime::Main thread when no frame is presented. */
    void StartWatchdog();
    /* The page low-memory accesses are redirected to. */
    void AllocLowPage();
    /* THPS_WATCH=address: a hardware write watchpoint (Windows). */
    void Watch(const char *addr);
    /* Print the caller's native backtrace for symbolize.py. */
    void Backtrace(const char *why);

    /* The system's handler entries (pc_main.cpp forwards to these). */
#ifdef _WIN32
    long OnException(void *exceptionPointers);
#else
    void OnSignal(int sig, void *siginfo, void *ucontext);
    void OnDumpSignal(void *ucontext);
#endif
    void WatchdogLoop();

private:
    bool FixLowAccess(CpuState &c, uintptr_t addr, void *wctx);
    void FixLowAccessDone(CpuState &c);
    bool FixDivide(CpuState &c, int overflow);
    void CrashReport(CpuState &c, void *wctx);
    void DumpMainThread();

    /* the low page (64 KB, zero until written), and the access being single-stepped against it */
    uint8_t *m_lowPage = nullptr;
    /* THPS_LOWMEM_FATAL=1: report a low-memory access as a crash instead of redirecting it */
    bool m_lowFatal = false;
    struct {
        int active;
        int reg;
        Reg bias;
    } m_fix = {};
    /* sites logged once */
    Reg m_lowSeen[256] = {};
    int m_lowSeenCount = 0;
    Reg m_divSeen[64] = {};
    int m_divSeenCount = 0;
    int m_watchHits = 0;
#ifdef _WIN32
    void *m_mainThread = nullptr; /* a HANDLE */
#else
    pthread_t m_mainThread{};
    uint8_t m_altStack[64 * 1024] = {}; /* the report still runs after a stack overflow */
#endif
};

extern Diagnostics g_diagnostics;

/* THPS_TRACE_RENDER=1: per-frame counts for the environment renderer (pc_debug.cpp). */
class RenderTrace {
public:
    void Count(const char *what, uint32_t ret);

private:
    uint32_t m_frame = ~0u, m_calls[2] = {}, m_culled = 0;
    int m_on = -1;
};

extern RenderTrace g_renderTrace;

} // namespace pc
