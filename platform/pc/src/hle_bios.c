/* The PS1 BIOS as the game sees it: the A0/B0/C0 function tables (reached from the Psy-Q stubs,
 * which mips2c turns into PC_BiosCall), the syscalls, a heap in PS1 RAM, and events. */
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "pc_hle.h"
#include "pc_recomp.h"
#include "pc_runtime.h"

/* ---- heap: first-fit free list inside PS1 RAM ------------------------------------------------ */

/* Block header (8 bytes, in PS1 RAM): size of the block including the header, and a used flag. */
typedef struct {
    uint32_t size;
    uint32_t used;
} HeapBlock;

static uint32_t s_heapStart, s_heapEnd;

void PC_HeapInit(uint32_t addr, uint32_t size) {
    HeapBlock *b;
    addr = (addr + 7) & ~7u;
    size &= ~7u;
    s_heapStart = addr;
    s_heapEnd = addr + size;
    b = PSX_PTR(addr);
    b->size = size;
    b->used = 0;
}

uint32_t PC_Malloc(uint32_t size) {
    uint32_t a = s_heapStart, need = ((size + 7) & ~7u) + 8;
    while (a < s_heapEnd) {
        HeapBlock *b = PSX_PTR(a);
        if (!b->size)
            break;
        if (!b->used) {
            /* merge following free blocks */
            while (a + b->size < s_heapEnd) {
                HeapBlock *n = PSX_PTR(a + b->size);
                if (n->used || !n->size)
                    break;
                b->size += n->size;
            }
            if (b->size >= need) {
                if (b->size - need >= 16) {
                    HeapBlock *rest = PSX_PTR(a + need);
                    rest->size = b->size - need;
                    rest->used = 0;
                    b->size = need;
                }
                b->used = 1;
                return a + 8;
            }
        }
        a += b->size;
    }
    PC_Log("BIOS malloc(%u) failed", size);
    return 0;
}

void PC_Free(uint32_t addr) {
    if (addr >= s_heapStart + 8 && addr < s_heapEnd)
        ((HeapBlock *)PSX_PTR(addr - 8))->used = 0;
}

/* ---- events ----------------------------------------------------------------------------------- */

#define MAX_EVENTS 32
typedef struct {
    uint32_t cls, spec, mode, func;
    int enabled, ready, used;
} Event;
static Event s_events[MAX_EVENTS];

static uint32_t OpenEvent(uint32_t cls, uint32_t spec, uint32_t mode, uint32_t func) {
    int i;
    for (i = 0; i < MAX_EVENTS; i++)
        if (!s_events[i].used) {
            Event *e = &s_events[i];
            memset(e, 0, sizeof *e);
            e->used = 1;
            e->cls = cls;
            e->spec = spec;
            e->mode = mode;
            e->func = func;
            PC_Trace("OpenEvent(%08x, %04x, %04x, %08x) -> %d", cls, spec, mode, func, i);
            return 0xf1000000u | i;
        }
    return 0xffffffffu;
}

static Event *Ev(uint32_t h) {
    uint32_t i = h & 0xffff;
    return (h >> 24) == 0xf1 && i < MAX_EVENTS && s_events[i].used ? &s_events[i] : NULL;
}

void PC_DeliverEvent(uint32_t cls, uint32_t spec) {
    int i;
    for (i = 0; i < MAX_EVENTS; i++) {
        Event *e = &s_events[i];
        if (!e->used || !e->enabled || e->cls != cls || e->spec != spec)
            continue;
        if (e->mode == 0x1000 && e->func) /* EvMdINTR: call the handler */
            ((void (*)(void))PSX_PTR(e->func))();
        else
            e->ready = 1;
    }
}

static int TestEvent(uint32_t h) {
    Event *e = Ev(h);
    PC_TickVBlank();
    if (e && e->ready) {
        e->ready = 0;
        return 1;
    }
    return 0;
}

/* ---- the BIOS function tables ------------------------------------------------------------------ */

static uint32_t s_critical;

static int BiosPrintf(void) {
    /* printf(fmt, ...) with the arguments in a1..a3 and on the PS1 stack */
    const char *fmt = PSX_PTR(pc_r.r[4]);
    uint32_t args[16];
    char out[1024];
    int i;
    args[0] = pc_r.r[5];
    args[1] = pc_r.r[6];
    args[2] = pc_r.r[7];
    for (i = 3; i < 16; i++)
        args[i] = MEM32(pc_r.r[29] + 16 + (i - 3) * 4);
    snprintf(out, sizeof out, fmt, args[0], args[1], args[2], args[3], args[4], args[5], args[6],
             args[7], args[8], args[9], args[10], args[11], args[12], args[13], args[14],
             args[15]);
    fputs(out, stdout);
    fflush(stdout);
    return (int)strlen(out);
}

static const char *BiosName(uint32_t vec, uint32_t fn) {
    static char buf[32];
    snprintf(buf, sizeof buf, "%c0:%02x", vec == 0xa0 ? 'A' : vec == 0xb0 ? 'B' : 'C', fn);
    return buf;
}

u32 PC_BiosCall(u32 vec, u32 fn) {
    u32 a0 = pc_r.r[4], a1 = pc_r.r[5], a2 = pc_r.r[6], a3 = pc_r.r[7];
    if (vec == 0xa0) {
        switch (fn) {
        case 0x13: return 0; /* setjmp */
        case 0x17: return (u32)strcmp(PSX_PTR(a0), PSX_PTR(a1));
        case 0x18: return (u32)strncmp(PSX_PTR(a0), PSX_PTR(a1), a2);
        case 0x19: strcpy(PSX_PTR(a0), PSX_PTR(a1)); return a0;
        case 0x1b: return (u32)strlen(PSX_PTR(a0));
        case 0x25: return (u32)toupper((int)a0);
        case 0x28: memset(PSX_PTR(a0), 0, a1); return a0; /* bzero */
        case 0x2a: memcpy(PSX_PTR(a0), PSX_PTR(a1), a2); return a0;
        case 0x2b: memset(PSX_PTR(a0), (int)a1, a2); return a0;
        case 0x2c: memmove(PSX_PTR(a0), PSX_PTR(a1), a2); return a0;
        case 0x2e: { const void *p = memchr(PSX_PTR(a0), (int)a1, a2); return p ? (u32)(uintptr_t)p : 0; }
        case 0x33: return PC_Malloc(a0);
        case 0x34: PC_Free(a0); return 0;
        case 0x37: { u32 p = PC_Malloc(a0 * a1); if (p) memset(PSX_PTR(p), 0, a0 * a1); return p; }
        case 0x39: PC_HeapInit(a0, a1); return 0;
        case 0x3e: fputs(PSX_PTR(a0), stdout); fputc('\n', stdout); fflush(stdout); return 0;
        case 0x3f: return (u32)BiosPrintf();
        case 0x44: return 0; /* FlushCache */
        case 0x49: return 0; /* GPU_cw: handled by the libgpu HLE */
        case 0x72: return 0; /* CdRemove */
        case 0x9f: return 0; /* SetMem */
        case 0xab: return 1; /* _card_info: answered via events (hle_libcard.c) */
        case 0xac: return 1; /* _card_load */
        }
    } else if (vec == 0xb0) {
        switch (fn) {
        case 0x07: return 1; /* DeliverEvent */
        case 0x08: return OpenEvent(a0, a1, a2, a3);
        case 0x09: { Event *e = Ev(a0); if (e) e->used = 0; return 1; }
        case 0x0a: { Event *e = Ev(a0); while (e && !e->ready) { PC_TickVBlank(); } if (e) e->ready = 0; return 1; }
        case 0x0b: return (u32)TestEvent(a0);
        case 0x0c: { Event *e = Ev(a0); if (e) e->enabled = 1; return 1; }
        case 0x0d: { Event *e = Ev(a0); if (e) e->enabled = 0; return 1; }
        case 0x12: return 1; /* InitPAD */
        case 0x13: return 1; /* StartPAD */
        case 0x14: return 1; /* StopPAD */
        case 0x17: return 0; /* ReturnFromException */
        case 0x18: return 0; /* ResetEntryInt */
        case 0x19: return 0; /* HookEntryInt */
        case 0x20: { Event *e = Ev(a0); if (e) e->ready = 0; return 1; } /* UnDeliverEvent */
        /* file I/O: the memory card (hle_libcard.c) */
        case 0x32: return (u32)hle_open(PSX_PTR(a0), (int)a1);
        case 0x33: return (u32)hle_lseek((int)a0, (int)a1, (int)a2);
        case 0x34: return (u32)hle_read((int)a0, PSX_PTR(a1), (int)a2);
        case 0x35: return (u32)hle_write((int)a0, PSX_PTR(a1), (int)a2);
        case 0x36: return (u32)hle_close((int)a0);
        case 0x3d: return 0; /* putchar */
        case 0x3f: fputs(PSX_PTR(a0), stdout); fflush(stdout); return 0; /* puts */
        case 0x41: return (u32)hle_format(PSX_PTR(a0));
        case 0x42: return (u32)(uintptr_t)hle_firstfile(PSX_PTR(a0), PSX_PTR(a1));
        case 0x43: return (u32)(uintptr_t)hle_nextfile(PSX_PTR(a0));
        case 0x45: return (u32)hle_erase(PSX_PTR(a0));
        case 0x4a: return 1; /* InitCARD */
        case 0x4b: return 1; /* StartCARD */
        case 0x4c: return 1; /* StopCARD */
        case 0x4d: return 0; /* _card_info_subfunc */
        case 0x4e: return 1; /* _card_write */
        case 0x4f: return 1; /* _card_read */
        case 0x50: return 1; /* _new_card */
        case 0x51: return 0; /* Krom2RawAdd */
        case 0x56: return 0; /* GetC0Table */
        case 0x57: return 0; /* GetB0Table */
        case 0x58: return 0; /* _card_chan */
        case 0x5b: return 0; /* ChangeClearPad */
        case 0x5c: return 0; /* _card_status */
        case 0x5d: return 0; /* _card_wait */
        }
    } else if (vec == 0xc0) {
        switch (fn) {
        case 0x00: return 0; /* EnqueueTimerAndVblankIrqs */
        case 0x01: return 0; /* EnqueueSyscallHandler */
        case 0x02: return 0; /* SysEnqIntRP */
        case 0x03: return 0; /* SysDeqIntRP */
        case 0x0a: return 0; /* ChangeClearRCnt */
        case 0x12: return 0; /* InstallDevices */
        }
    }
    PC_Log("unhandled BIOS call %s (a0=%08x a1=%08x a2=%08x a3=%08x)", BiosName(vec, fn), a0, a1, a2,
           a3);
    return 0;
}

/* syscall: a0 = 1 EnterCriticalSection, 2 ExitCriticalSection */
void PC_Syscall(void) {
    u32 a0 = pc_r.r[4];
    if (a0 == 1) {
        pc_r.r[2] = s_critical ? 0 : 1;
        s_critical = 1;
    } else if (a0 == 2) {
        s_critical = 0;
        pc_r.r[2] = 0;
    } else {
        pc_r.r[2] = 0;
    }
}
