/* libgpu for translated retail code (build.py --mips only: THPS_MIPS, THPS_SNAP). The game's own
 * code calls the port's drawing, screen and texture services from its PC_PORT branches; retail
 * code translated from MIPS still calls libgpu at its addresses, so in a --mips build these
 * entry points pass those calls on. libgpu's pure helpers (SetDefDrawEnv ...) run as translated
 * retail code; the MDEC's (libpress) are not provided: the game's movie code is always C. */
#ifdef PC_MIPS
#include <stdint.h>

#include <sys/types.h>
#include <libgpu.h>

#include "pc_draw.h"
#include "pc_frame.h"
#include "pc_runtime.h"
#include "pc_screen.h"
#include "pc_texstore.h"

/* VRAM held both: textures where the texture store has some, the screens elsewhere */
static int InTextures(const RECT *r) {
    const uint8_t *cell = PC_TexStoreCells();
    for (int y = r->y / PC_TEX_CELL; y <= (r->y + r->h - 1) / PC_TEX_CELL && y < PC_TEX_H / PC_TEX_CELL; y++)
        for (int x = r->x / PC_TEX_CELL; x <= (r->x + r->w - 1) / PC_TEX_CELL && x < PC_TEX_W / PC_TEX_CELL; x++)
            if (cell[y * (PC_TEX_W / PC_TEX_CELL) + x] & PC_TEX_USED)
                return 1;
    return 0;
}

int hle_8008c1a0(int mode) { /* ResetGraph */
    if ((mode & 3) != 1) {
        PC_DrawReset();
        PC_ScreenReset();
    }
    return 0;
}

int hle_8008c314(int level) { /* SetGraphDebug */
    (void)level;
    return 0;
}

uint32_t hle_8008c370(uint32_t fn) { /* DrawSyncCallback */
    PC_DrawSetDone((void (*)(void))(uintptr_t)fn);   /* its address is callable (trampoline) */
    return 0;
}

void hle_8008c3d0(int on) { /* SetDispMask */
    PC_ScreenEnable(on);
}

int hle_8008c468(int mode) { /* DrawSync */
    (void)mode;
    return 0;
}

int hle_8008c5ec(RECT *r, int red, int green, int blue) { /* ClearImage */
    uint32_t rgb = (uint32_t)(red & 255) | (uint32_t)(green & 255) << 8 | (uint32_t)(blue & 255) << 16;
    PC_ScreenFill(r->x, r->y, r->w, r->h, rgb);
    PC_TexFill(r->x, r->y, r->w, r->h, rgb);
    return 0;
}

int hle_8008c67c(RECT *r, int red, int green, int blue) { /* ClearImage2 */
    return hle_8008c5ec(r, red, green, blue);
}

int hle_8008c714(RECT *r, u_long *p) { /* LoadImage */
    PC_TexUpload(r->x, r->y, r->w, r->h, p);
    PC_ScreenBlit(r->x, r->y, r->w, r->h, p);
    return 0;
}

int hle_8008c774(RECT *r, u_long *p) { /* StoreImage */
    if (InTextures(r))
        PC_TexRead(r->x, r->y, r->w, r->h, p);
    else
        PC_ScreenRead(r->x, r->y, r->w, r->h, p);
    return 0;
}

u_long *hle_8008c7d4(u_long *ot, int n) { /* ClearOTagR */
    PC_DrawClearOTR(ot, n);
    return ot;
}

void hle_8008c880(void *p) { /* DrawPrim */
    PC_DrawPacket(p);
}

void hle_8008c8dc(u_long *ot) { /* DrawOTag */
    PC_DrawOT(ot);
}

DRAWENV *hle_8008c94c(DRAWENV *env) { /* PutDrawEnv */
    PC_DrawSetEnv(env);
    return env;
}

DISPENV *hle_8008ca0c(DISPENV *env) { /* PutDispEnv */
    PC_ScreenShow(env);
    return env;
}

int hle_8008cf04(void) { /* GetODE */
    PC_FramePoll();
    return (int)(PC_FrameCount() & 1);
}
#endif
