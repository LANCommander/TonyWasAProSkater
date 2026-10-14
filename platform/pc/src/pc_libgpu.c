/* The pure helpers of Psy-Q's libgpu that the game calls, as the port's own C: they fill in the
 * game's structures (drawing and display environments, primitive packets) and compute texture
 * page and CLUT words, nothing more. The game calls them by their Psy-Q names; the build points
 * those calls here (build.py NATIVE_AT: SetDefDrawEnv -> PC_SetDefDrawEnv ...).
 *
 * What libgpu did with the GPU itself is the port's own: drawing is pc_draw.h (DrawOTag ->
 * PC_DrawOT ...), the screens and the display pc_screen.h, the textures pc_texstore.h; the game
 * calls those. */
#include <stdint.h>

#include <sys/types.h>
#include <libgpu.h>

static int ClampI(int v, int lo, int hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

static u_long WordArea(uint32_t cmd, int x, int y)
{
    x = ClampI(x, 0, 1023);
    y = ClampI(y, 0, 511);
    return cmd | ((u_long)(y & 0x3ff) << 10) | (u_long)(x & 0x3ff);
}

static void TagLen(void *p, int n)
{
    ((P_TAG *)p)->len = (u_long)n;
}

DRAWENV *PC_SetDefDrawEnv(DRAWENV *env, int x, int y, int w, int h)
{
    env->clip.x = (short)x;
    env->clip.y = (short)y;
    env->clip.w = (short)w;
    env->clip.h = (short)h;
    env->ofs[0] = (short)x;
    env->ofs[1] = (short)y;
    env->tw.x = env->tw.y = env->tw.w = env->tw.h = 0;
    env->tpage = 0x0a;
    env->dtd = 1;
    env->dfe = (h > 256) ? 1 : 0;
    env->isbg = 0;
    env->r0 = env->g0 = env->b0 = 0;
    return env;
}

DISPENV *PC_SetDefDispEnv(DISPENV *env, int x, int y, int w, int h)
{
    env->disp.x = (short)x;
    env->disp.y = (short)y;
    env->disp.w = (short)w;
    env->disp.h = (short)h;
    env->screen.x = env->screen.y = env->screen.w = env->screen.h = 0;
    env->isinter = 0;
    env->isrgb24 = 0;
    env->pad0 = env->pad1 = 0;
    return env;
}

void PC_SetDrawArea(DR_AREA *p, RECT *r)
{
    TagLen(p, 2);
    p->code[0] = WordArea(0xe3000000u, r->x, r->y);
    p->code[1] = WordArea(0xe4000000u, r->x + r->w - 1, r->y + r->h - 1);
}

void PC_SetDrawTPage(DR_TPAGE *p, int dfe, int dtd, int tpage)
{
    TagLen(p, 1);
    p->code[0] = _get_mode(dfe, dtd, tpage);
}

void PC_SetPolyF4(POLY_F4 *p)
{
    setPolyF4(p);
}

u_short PC_GetTPage(int tp, int abr, int x, int y)
{
    return (u_short)getTPage(tp, abr, x, y);
}

u_short PC_GetClut(int x, int y)
{
    return (u_short)getClut(x, y);
}

/* Nothing is ever still being drawn: drawing is done by the time the call returns. */
int PC_DrawSync(int mode)
{
    (void)mode;
    return 0;
}
