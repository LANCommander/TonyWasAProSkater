/* The port's runtime: the game's memory and data, logging, diagnostics. */
#ifndef PC_RUNTIME_H
#define PC_RUNTIME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The memory the game's allocator manages (Init_ResetMemory: its heaps, Mem_NewX): one block from
 * the system, 8 MB (the PS1 had 2 MB for everything; the port's packets and the Dreamcast data's
 * files are bigger). */
void *PC_GameHeap(uint32_t *size);

/* A 32-bit address as a native pointer (runtime code that still holds the game's pointers as
 * words). */
#define PSX_PTR(a) ((void *)(uintptr_t)(uint32_t)(a))

/* The game's data and .bss are native globals: the build's table of them, by retail address, ends
 * with {0, 0, 0}. */
typedef struct {
    uint32_t start, end;
    char *p;
} PcData;
extern const PcData pc_data[];

/* The global data at a retail address: where the native global holding it is (NULL if none). For
 * runtime code that reads a game global by address. */
void *PC_Data(uint32_t addr);
/* The same as a number for the PSX_PTR arithmetic runtime code does, looked up once per use. */
#define PSX_DATA_ADDR(addr)                                                                        \
    ({                                                                                             \
        static uint32_t pc_data_addr_;                                                             \
        if (!pc_data_addr_)                                                                        \
            pc_data_addr_ = (uint32_t)(uintptr_t)PC_Data(addr);                                    \
        pc_data_addr_;                                                                             \
    })

/* Logging: PC_Log always prints; PC_Trace only with THPS_TRACE=1. */
void PC_Log(const char *fmt, ...);
void PC_Trace(const char *fmt, ...);
extern int pc_trace;
void PC_Fatal(const char *fmt, ...);
/* Bang(r, g, b), the game's fatal-error handler, which on the PS1 fills the screen with a colour and
 * spins: reports which heap ran out (and its free list) and exits instead of hanging silently. */
void PC_Bang(int r, int g, int b) __attribute__((noreturn));

/* Called by generated stubs for functions with no implementation yet. Returns 0. */
uint32_t PC_MissingFunction(uint32_t addr);

/* Name of a PS1 code address, for logs ("FUN_8002ab20", "DrawOTag", ...). */
const char *PC_NameOf(uint32_t addr);

/* Directory holding the disc image / game files, and the executable path. */
const char *PC_DataDir(void);
/* The player's folder, Saved Games\Neversoft\Tony Hawk's Pro Skater (Linux:
 * $XDG_DATA_HOME/Neversoft/Tony Hawk's Pro Skater, default ~/.local/share/...; created if missing):
 * Settings.ini and the Saves folder. */
const char *PC_UserDir(void);

/* Called by the hi-res front end's frame end (FUN_8006b26c) between its wait for the next field
 * and DrawOTag: a screen that puts back part of its background itself does it here, so the
 * background and the frame's text are shown together. */
extern void (*pc_hires_before_draw)(void);

/* Per-frame hooks (pc_frame.c). */
void PC_PumpEvents(void);
int PC_QuitRequested(void);
void PC_RequestQuit(void);

#ifdef __cplusplus
}
#endif

#endif
