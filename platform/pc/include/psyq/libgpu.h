/* Clean-room Psy-Q libgpu interface for the PC port.
 *
 * Primitives are GP0 command packets (the PS1 GPU's words, which the port's packet walker draws,
 * pc_draw.h) behind a native tag: the next packet of the ordering table (a pointer) and the number
 * of command words. On the PS1 the tag was one DMA word with a 24-bit link; here packets and
 * ordering tables can be anywhere in memory. An ordering-table entry is a tag with no words.
 *
 * Written from the public description of the API and psx-spx. The structures and macros are what
 * the port uses (the game's packets, DRAWENV, DISPENV); the functions the game calls are the
 * port's own: src/pc_libgpu.c, and pc_draw.h / pc_screen.h / pc_texstore.h for the GPU's work. */
#ifndef PSYQ_LIBGPU_H
#define PSYQ_LIBGPU_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- VRAM rectangles and environments ---- */

typedef struct {
    short x, y;
    short w, h;
} RECT;

typedef struct {
    int x, y;
    int w, h;
} RECT32;

/* ---- primitive tags ---- */

typedef struct P_TAG {
    struct P_TAG *next; /* the next packet; NULL ends the ordering table */
    u_long len;         /* command words after the tag */
} P_TAG;

/* A packet's command words (they follow its tag). */
#define P_WORDS(p) ((u_long *)((P_TAG *)(p) + 1))

typedef struct {
    P_TAG tag;
    u_long code[15];
} DR_ENV;

typedef struct {
    RECT clip;          /* drawing area */
    short ofs[2];       /* drawing offset */
    RECT tw;            /* texture window */
    u_short tpage;      /* initial texture page */
    u_char dtd;         /* dithering on */
    u_char dfe;         /* drawing to the displayed area allowed */
    u_char isbg;        /* clear the drawing area when the environment is put */
    u_char r0, g0, b0;  /* clear colour */
    DR_ENV dr_env;      /* packet built by SetDrawEnv */
} DRAWENV;

typedef struct {
    RECT disp;          /* displayed VRAM area */
    RECT screen;        /* position/size on the TV screen (0 = defaults) */
    u_char isinter;     /* interlace */
    u_char isrgb24;     /* 24-bit colour */
    u_char pad0, pad1;
} DISPENV;

typedef struct {
    u_char r0, g0, b0, code;
} P_CODE;

/* ---- polygons ---- */

typedef struct {
    P_TAG tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    short x1, y1;
    short x2, y2;
} POLY_F3;

typedef struct {
    P_TAG tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    short x1, y1;
    short x2, y2;
    short x3, y3;
} POLY_F4;

typedef struct {
    P_TAG tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    u_char u0, v0; u_short clut;
    short x1, y1;
    u_char u1, v1; u_short tpage;
    short x2, y2;
    u_char u2, v2; u_short pad1;
} POLY_FT3;

typedef struct {
    P_TAG tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    u_char u0, v0; u_short clut;
    short x1, y1;
    u_char u1, v1; u_short tpage;
    short x2, y2;
    u_char u2, v2; u_short pad1;
    short x3, y3;
    u_char u3, v3; u_short pad2;
} POLY_FT4;

typedef struct {
    P_TAG tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    u_char r1, g1, b1, pad1;
    short x1, y1;
    u_char r2, g2, b2, pad2;
    short x2, y2;
} POLY_G3;

typedef struct {
    P_TAG tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    u_char r1, g1, b1, pad1;
    short x1, y1;
    u_char r2, g2, b2, pad2;
    short x2, y2;
    u_char r3, g3, b3, pad3;
    short x3, y3;
} POLY_G4;

typedef struct {
    P_TAG tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    u_char u0, v0; u_short clut;
    u_char r1, g1, b1, p1;
    short x1, y1;
    u_char u1, v1; u_short tpage;
    u_char r2, g2, b2, p2;
    short x2, y2;
    u_char u2, v2; u_short pad2;
} POLY_GT3;

typedef struct {
    P_TAG tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    u_char u0, v0; u_short clut;
    u_char r1, g1, b1, p1;
    short x1, y1;
    u_char u1, v1; u_short tpage;
    u_char r2, g2, b2, p2;
    short x2, y2;
    u_char u2, v2; u_short pad2;
    u_char r3, g3, b3, p3;
    short x3, y3;
    u_char u3, v3; u_short pad3;
} POLY_GT4;

/* ---- lines (poly-lines end with a 0x55555555 terminator word in `pad`) ---- */

typedef struct {
    P_TAG tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    short x1, y1;
} LINE_F2;

typedef struct {
    P_TAG tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    u_char r1, g1, b1, p1;
    short x1, y1;
} LINE_G2;

typedef struct {
    P_TAG tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    short x1, y1;
    short x2, y2;
    u_long pad;
} LINE_F3;

typedef struct {
    P_TAG tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    u_char r1, g1, b1, p1;
    short x1, y1;
    u_char r2, g2, b2, p2;
    short x2, y2;
    u_long pad;
} LINE_G3;

typedef struct {
    P_TAG tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    short x1, y1;
    short x2, y2;
    short x3, y3;
    u_long pad;
} LINE_F4;

typedef struct {
    P_TAG tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    u_char r1, g1, b1, p1;
    short x1, y1;
    u_char r2, g2, b2, p2;
    short x2, y2;
    u_char r3, g3, b3, p3;
    short x3, y3;
    u_long pad;
} LINE_G4;

/* ---- sprites and tiles ---- */

typedef struct {
    P_TAG tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    u_char u0, v0; u_short clut;
    short w, h;
} SPRT;

typedef struct {
    P_TAG tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    u_char u0, v0; u_short clut;
} SPRT_16;

typedef struct {
    P_TAG tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    u_char u0, v0; u_short clut;
} SPRT_8;

typedef struct {
    P_TAG tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    short w, h;
} TILE;

typedef struct {
    P_TAG tag;
    u_char r0, g0, b0, code;
    short x0, y0;
} TILE_16;

typedef struct {
    P_TAG tag;
    u_char r0, g0, b0, code;
    short x0, y0;
} TILE_8;

typedef struct {
    P_TAG tag;
    u_char r0, g0, b0, code;
    short x0, y0;
} TILE_1;

/* ---- state-change packets ---- */

typedef struct { P_TAG tag; u_long code[2]; } DR_MODE;
typedef struct { P_TAG tag; u_long code[2]; } DR_TWIN;
typedef struct { P_TAG tag; u_long code[2]; } DR_AREA;
typedef struct { P_TAG tag; u_long code[2]; } DR_OFFSET;
typedef struct { P_TAG tag; u_long code[5]; } DR_MOVE;
typedef struct { P_TAG tag; u_long code[3]; u_long p[13]; } DR_LOAD;
typedef struct { P_TAG tag; u_long code[1]; } DR_TPAGE;
typedef struct { P_TAG tag; u_long code[2]; } DR_STP;

/* ---- TIM descriptor ---- */

typedef struct {
    u_long mode;
    RECT *crect;
    u_long *caddr;
    RECT *prect;
    u_long *paddr;
} TIM_IMAGE;

#ifdef PSYQ_LIBGTE_H
/* Generic TMD primitive (needs SVECTOR, so only when libgte.h came first, as with Psy-Q). */
typedef struct {
    u_long id;
    u_char r0, g0, b0, p0;
    u_char r1, g1, b1, p1;
    u_char r2, g2, b2, p2;
    u_char r3, g3, b3, p3;
    u_short tpage, clut;
    u_char u0, v0, u1, v1;
    u_char u2, v2, u3, v3;
    SVECTOR x0, x1, x2, x3;
    SVECTOR n0, n1, n2, n3;
    SVECTOR *v_ofs;
    SVECTOR *n_ofs;
    u_short vert0, vert1;
    u_short vert2, vert3;
    u_short norm0, norm1;
    u_short norm2, norm3;
} TMD_PRIM;
#endif

#define FNT_MAX_ID   8
#define FNT_MAX_SPRT 1024

/* ---- macros: rectangles and primitive fields ---- */

#define setRECT(r, _x, _y, _w, _h) \
    ((r)->x = (_x), (r)->y = (_y), (r)->w = (_w), (r)->h = (_h))

#define setRGB0(p, _r, _g, _b) ((p)->r0 = (_r), (p)->g0 = (_g), (p)->b0 = (_b))
#define setRGB1(p, _r, _g, _b) ((p)->r1 = (_r), (p)->g1 = (_g), (p)->b1 = (_b))
#define setRGB2(p, _r, _g, _b) ((p)->r2 = (_r), (p)->g2 = (_g), (p)->b2 = (_b))
#define setRGB3(p, _r, _g, _b) ((p)->r3 = (_r), (p)->g3 = (_g), (p)->b3 = (_b))

#define setXY0(p, _x0, _y0) ((p)->x0 = (_x0), (p)->y0 = (_y0))
#define setXY2(p, _x0, _y0, _x1, _y1) \
    ((p)->x0 = (_x0), (p)->y0 = (_y0), (p)->x1 = (_x1), (p)->y1 = (_y1))
#define setXY3(p, _x0, _y0, _x1, _y1, _x2, _y2) \
    ((p)->x0 = (_x0), (p)->y0 = (_y0), (p)->x1 = (_x1), (p)->y1 = (_y1), \
     (p)->x2 = (_x2), (p)->y2 = (_y2))
#define setXY4(p, _x0, _y0, _x1, _y1, _x2, _y2, _x3, _y3) \
    ((p)->x0 = (_x0), (p)->y0 = (_y0), (p)->x1 = (_x1), (p)->y1 = (_y1), \
     (p)->x2 = (_x2), (p)->y2 = (_y2), (p)->x3 = (_x3), (p)->y3 = (_y3))
#define setXYWH(p, _x0, _y0, _w, _h) \
    ((p)->x0 = (_x0), (p)->y0 = (_y0), \
     (p)->x1 = (_x0) + (_w), (p)->y1 = (_y0), \
     (p)->x2 = (_x0), (p)->y2 = (_y0) + (_h), \
     (p)->x3 = (_x0) + (_w), (p)->y3 = (_y0) + (_h))
#define setWH(p, _w, _h) ((p)->w = (_w), (p)->h = (_h))

#define setUV0(p, _u0, _v0) ((p)->u0 = (_u0), (p)->v0 = (_v0))
#define setUV3(p, _u0, _v0, _u1, _v1, _u2, _v2) \
    ((p)->u0 = (_u0), (p)->v0 = (_v0), (p)->u1 = (_u1), (p)->v1 = (_v1), \
     (p)->u2 = (_u2), (p)->v2 = (_v2))
#define setUV4(p, _u0, _v0, _u1, _v1, _u2, _v2, _u3, _v3) \
    ((p)->u0 = (_u0), (p)->v0 = (_v0), (p)->u1 = (_u1), (p)->v1 = (_v1), \
     (p)->u2 = (_u2), (p)->v2 = (_v2), (p)->u3 = (_u3), (p)->v3 = (_v3))
#define setUVWH(p, _u0, _v0, _w, _h) \
    ((p)->u0 = (_u0), (p)->v0 = (_v0), \
     (p)->u1 = (_u0) + (_w), (p)->v1 = (_v0), \
     (p)->u2 = (_u0), (p)->v2 = (_v0) + (_h), \
     (p)->u3 = (_u0) + (_w), (p)->v3 = (_v0) + (_h))

/* Texture page / CLUT words (tp: 0 = 4-bit, 1 = 8-bit, 2 = 15-bit; abr: blend mode 0-3). */
#define getTPage(tp, abr, x, y) \
    ((((tp) & 0x3) << 7) | (((abr) & 0x3) << 5) | (((y) & 0x100) >> 4) | \
     (((x) & 0x3ff) >> 6) | (((y) & 0x200) << 2))
#define getClut(x, y) (((y) << 6) | (((x) >> 4) & 0x3f))
#define setTPage(p, tp, abr, x, y) ((p)->tpage = getTPage(tp, abr, x, y))
#define setClut(p, x, y) ((p)->clut = getClut(x, y))

/* ---- macros: tags and ordering tables ---- */

#define setlen(p, _len)   (((P_TAG *)(p))->len = (u_char)(_len))
#define setaddr(p, _addr) (((P_TAG *)(p))->next = (P_TAG *)(_addr))
#define setcode(p, _code) (((u_char *)P_WORDS(p))[3] = (u_char)(_code))
#define getlen(p)         ((u_char)(((P_TAG *)(p))->len))
#define getcode(p)        (((u_char *)P_WORDS(p))[3])
#define getaddr(p)        (((P_TAG *)(p))->next)

#define nextPrim(p)  ((void *)((P_TAG *)(p))->next)
#define isendprim(p) (((P_TAG *)(p))->next == 0)

#define addPrim(ot, p)        (setaddr(p, getaddr(ot)), setaddr(ot, p))
#define addPrims(ot, p0, p1)  (setaddr(p1, getaddr(ot)), setaddr(ot, p0))
#define catPrim(p0, p1)       setaddr(p0, p1)
#define termPrim(p)           setaddr(p, 0)

#define setSemiTrans(p, abe) \
    ((abe) ? setcode(p, getcode(p) | 0x02) : setcode(p, getcode(p) & ~0x02))
#define setShadeTex(p, tge) \
    ((tge) ? setcode(p, getcode(p) | 0x01) : setcode(p, getcode(p) & ~0x01))

/* ---- macros: state packets ---- */

#define _get_mode(dfe, dtd, tpage) \
    (0xe1000000u | ((dtd) ? 0x0200 : 0) | ((dfe) ? 0x0400 : 0) | ((tpage) & 0x9ff))
#define _get_tw(tw) \
    ((tw) ? (0xe2000000u | ((((tw)->y & 0xff) >> 3) << 15) | ((((tw)->x & 0xff) >> 3) << 10) | \
             (((~((tw)->h - 1) & 0xff) >> 3) << 5) | ((~((tw)->w - 1) & 0xff) >> 3)) : 0)
#define _get_len(rect) ((((rect)->w * (rect)->h + 1) / 2) + 4)

#define setDrawTPage(p, dfe, dtd, tpage) \
    (setlen(p, 1), P_WORDS(p)[0] = _get_mode(dfe, dtd, tpage))
#define setTexWindow(p, tw) \
    (setlen(p, 2), P_WORDS(p)[0] = _get_tw(tw), P_WORDS(p)[1] = 0)
#define setDrawStp(p, pbw) \
    (setlen(p, 2), P_WORDS(p)[0] = 0xe6000000u | ((pbw) ? 1 : 0), P_WORDS(p)[1] = 0)
#define setDrawLoad(pt, rect) \
    ((_get_len(rect) <= 16) ? \
        (setlen(pt, _get_len(rect)), (pt)->code[0] = 0xa0000000u, \
         (pt)->code[1] = *((u_long *)&(rect)->x), (pt)->code[2] = *((u_long *)&(rect)->w), \
         (pt)->p[_get_len(rect) - 4] = 0x01000000u) : \
        (setlen(pt, 0)))

/* ---- macros: primitive initialisers (length, command code) ---- */

#define setPolyF3(p)  (setlen(p, 4), setcode(p, 0x20))
#define setPolyFT3(p) (setlen(p, 7), setcode(p, 0x24))
#define setPolyG3(p)  (setlen(p, 6), setcode(p, 0x30))
#define setPolyGT3(p) (setlen(p, 9), setcode(p, 0x34))
#define setPolyF4(p)  (setlen(p, 5), setcode(p, 0x28))
#define setPolyFT4(p) (setlen(p, 9), setcode(p, 0x2c))
#define setPolyG4(p)  (setlen(p, 8), setcode(p, 0x38))
#define setPolyGT4(p) (setlen(p, 12), setcode(p, 0x3c))

#define setSprt8(p)   (setlen(p, 3), setcode(p, 0x74))
#define setSprt16(p)  (setlen(p, 3), setcode(p, 0x7c))
#define setSprt(p)    (setlen(p, 4), setcode(p, 0x64))

#define setTile1(p)   (setlen(p, 2), setcode(p, 0x68))
#define setTile8(p)   (setlen(p, 2), setcode(p, 0x70))
#define setTile16(p)  (setlen(p, 2), setcode(p, 0x78))
#define setTile(p)    (setlen(p, 3), setcode(p, 0x60))

#define setLineF2(p)  (setlen(p, 3), setcode(p, 0x40))
#define setLineG2(p)  (setlen(p, 4), setcode(p, 0x50))
#define setLineF3(p)  (setlen(p, 5), setcode(p, 0x48), (p)->pad = 0x55555555)
#define setLineG3(p)  (setlen(p, 7), setcode(p, 0x58), (p)->pad = 0x55555555, (p)->p2 = 0)
#define setLineF4(p)  (setlen(p, 6), setcode(p, 0x4c), (p)->pad = 0x55555555)
#define setLineG4(p)  (setlen(p, 9), setcode(p, 0x5c), (p)->pad = 0x55555555, \
                       (p)->p2 = 0, (p)->p3 = 0)

/* ---- functions ---- */

/* FntPrint takes an optional leading stream id before the format; it is declared without a
 * prototype in C (as Psy-Q does) so both call forms compile. */
#ifndef PC_LIBGPU_NO_FNTPRINT_DECL
#if defined(__cplusplus) || (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L)
int FntPrint(...);
#else
int FntPrint();
#endif
#endif

DISPENV *GetDispEnv(DISPENV *env);
DISPENV *PutDispEnv(DISPENV *env);
DISPENV *SetDefDispEnv(DISPENV *env, int x, int y, int w, int h);
DRAWENV *GetDrawEnv(DRAWENV *env);
DRAWENV *PutDrawEnv(DRAWENV *env);
DRAWENV *SetDefDrawEnv(DRAWENV *env, int x, int y, int w, int h);

int ResetGraph(int mode);
int SetGraphDebug(int level);
int GetGraphDebug(void);
u_long DrawSyncCallback(void (*func)(void));
void SetDispMask(int mask);
int DrawSync(int mode);
int GetODE(void);
int GetVideoMode(void);
int SetVideoMode(int mode);

int ClearImage(RECT *rect, u_char r, u_char g, u_char b);
int ClearImage2(RECT *rect, u_char r, u_char g, u_char b);
int LoadImage(RECT *rect, u_long *p);
int StoreImage(RECT *rect, u_long *p);
int MoveImage(RECT *rect, int x, int y);

P_TAG *ClearOTag(P_TAG *ot, int n);
P_TAG *ClearOTagR(P_TAG *ot, int n);
void DrawOTag(P_TAG *p);
void DrawOTagEnv(P_TAG *p, DRAWENV *env);
void DrawPrim(void *p);

void AddPrim(void *ot, void *p);
void AddPrims(void *ot, void *p0, void *p1);
void CatPrim(void *p0, void *p1);
void TermPrim(void *p);
void *NextPrim(void *p);
int IsEndPrim(void *p);

u_short GetTPage(int tp, int abr, int x, int y);
u_short GetClut(int x, int y);
u_short LoadTPage(u_long *pix, int tp, int abr, int x, int y, int w, int h);
u_short LoadClut(u_long *clut, int x, int y);
u_short LoadClut2(u_long *clut, int x, int y);

void SetDrawArea(DR_AREA *p, RECT *r);
void SetDrawEnv(DR_ENV *dr_env, DRAWENV *env);
void SetDrawMode(DR_MODE *p, int dfe, int dtd, int tpage, RECT *tw);
void SetDrawTPage(DR_TPAGE *p, int dfe, int dtd, int tpage);
void SetDrawOffset(DR_OFFSET *p, u_short *ofs);
void SetDrawMove(DR_MOVE *p, RECT *rect, int x, int y);
void SetDrawLoad(DR_LOAD *p, RECT *rect);
void SetDrawStp(DR_STP *p, int pbw);
void SetTexWindow(DR_TWIN *p, RECT *tw);

void SetPolyF3(POLY_F3 *p);
void SetPolyF4(POLY_F4 *p);
void SetPolyFT3(POLY_FT3 *p);
void SetPolyFT4(POLY_FT4 *p);
void SetPolyG3(POLY_G3 *p);
void SetPolyG4(POLY_G4 *p);
void SetPolyGT3(POLY_GT3 *p);
void SetPolyGT4(POLY_GT4 *p);
void SetLineF2(LINE_F2 *p);
void SetLineF3(LINE_F3 *p);
void SetLineF4(LINE_F4 *p);
void SetLineG2(LINE_G2 *p);
void SetLineG3(LINE_G3 *p);
void SetLineG4(LINE_G4 *p);
void SetSprt(SPRT *p);
void SetSprt16(SPRT_16 *p);
void SetSprt8(SPRT_8 *p);
void SetTile(TILE *p);
void SetTile1(TILE_1 *p);
void SetTile8(TILE_8 *p);
void SetTile16(TILE_16 *p);
void SetSemiTrans(void *p, int abe);
void SetShadeTex(void *p, int tge);

void FntLoad(int tx, int ty);
int FntOpen(int x, int y, int w, int h, int isbg, int n);
u_long *FntFlush(int id);
void SetDumpFnt(int id);

#ifdef __cplusplus
}
#endif

#endif
