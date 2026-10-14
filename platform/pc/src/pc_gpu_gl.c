/* OpenGL 3.3 drawing backend; the interface and the render targets are described in
 * pc_gpu_gl.h.
 *
 * Primitives are batched into one triangle stream (lines become thin quads) and drawn with a
 * shader that does the PS1 texture lookup (4/8-bit CLUT or 15-bit direct, texture window),
 * modulation and the semi-transparency modes. Every vertex carries its own texture page, CLUT
 * and blend mode, so a batch only ends at a drawing-area change, at a B-F primitive (which needs
 * another blend equation) or when the textures or a screen's pixels change. The colour target is RGBA8 with
 * the mask bit in alpha; no dithering, since nothing is reduced to 15 bits for display. */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glad/gl.h>
#define SDL_MAIN_HANDLED
#include <SDL.h>

#include "pc_dctex.h"
#include "pc_gpu_gl.h"
#include "pc_precise.h"
#include "pc_render3d.h"
#include "pc_runtime.h"
#include "pc_texstore.h"
#include "pc_gpu_gl_shaders.h"

#define VRAM_W 1024
#define VRAM_H 512
#define CELL 16
#define CELLS_X (VRAM_W / CELL)
#define CELLS_Y (VRAM_H / CELL)


typedef PC_GlVert GVert;

/* GVert.info (PC_GL_I_* in pc_gpu_gl.h) */
#define I_TEXTURED 0x200
#define I_RAW      0x400
#define I_SEMI     0x800
#define I_MASK     0x1000
#define I_DEPTH    PC_GL_I_DEPTH
#define I_VIEW     PC_GL_I_VIEW
#define I_SKY      PC_GL_I_SKY
#define I_HD       0x0180   /* colour depth 3: an HD texture (pc_dctex.h), clut = its handle */

#define MAX_HD 8192

#define MAX_VERTS (64 * 1024)

/* A render target for a VRAM area: the base target is all of VRAM at the PS1's resolution, a
 * display surface one screen buffer at the size it is shown at (see PC_GlNoteScreen). Each VRAM
 * pixel belongs to the surface covering it, else to the base; drawing goes to every target its
 * area touches, so the base under a surface is never read.
 *
 * Widescreen: a gameplay surface also has margins left and right of its VRAM area, in drawing
 * coordinates beyond the buffer (x < 0, or past it where VRAM holds textures). Only primitives
 * drawn with the widened drawing area (PC_GlWideClip; PC_GlPrim.wide) reach them; to everything
 * else, and to the cell bookkeeping, the surface is its VRAM area alone. */
typedef struct {
    int x, y, w, h;           /* drawing area: the VRAM area plus the margins */
    int rx, rw;               /* the VRAM area's columns (its rows are the drawing area's) */
    int pw, ph;               /* size in pixels */
    float sx, sy;             /* pixels per VRAM pixel */
    GLuint tex, depth, fbo;   /* RGBA8 colour, DEPTH32F for the native 3D layers (pc_render3d.c) */
} Target;

#define MAX_SURFACES 4

static int s_active;
static GLuint s_draw_prog, s_copy_prog, s_present_prog;
static GLint s_draw_u_pass, s_draw_u_dpass, s_draw_u_proj, s_draw_u_target, s_draw_u_shift,
    s_draw_u_uvfix, s_copy_u_target, s_copy_u_map, s_copy_u_src;
static GLuint s_vao, s_vbo, s_copy_vao, s_copy_vbo, s_present_vao, s_present_vbo;
static GLuint s_blit;                  /* R16UI 1024x512: pixels on their way to a screen */
static GLuint s_texstore;              /* R16UI 1024x512: textures and CLUTs (pc_texstore.h) */
static Target s_base;                  /* 1024x512 */
static Target s_surf[MAX_SURFACES];    /* display surfaces, oldest first; they never overlap */
static int s_nsurf;
static int s_fixed_scale;              /* THPS_GL_SCALE: surfaces at n times the PS1 resolution */
static int s_out_w, s_out_h;           /* the window's drawable size (pixels) */
static int s_widescreen;               /* PC_GlSetWidescreen */
static int s_pad;                      /* widescreen margin of gameplay screens, in PS1 pixels */
int pc_wide_pad;                       /* s_pad, for the game's view frustum (pc_gpu_gl.h) */
static GLint s_max_tex;
static GLuint s_rb_fbo, s_rb_tex;      /* RGBA8 1024x512: readback */
static GLuint s_disp_tex;              /* RGBA8 1024x512: pictures shown instead (movies) */
static int s_filter;
static int s_precise, s_perspective;   /* THPS_GL_PRECISE, THPS_GL_PERSPECTIVE (default on) */

static uint8_t s_drawn[CELLS_Y][CELLS_X];   /* 16x16 cells primitives drew on */

/* THPS_GLSTATS=1: per-second counts of draw calls, readbacks and uploads */
static struct { int on, draws, readbacks, tex_ups, blits, frames; } st;

static GVert s_verts[MAX_VERTS];
static int s_nv;
static int s_batch_sub;                /* the batch is B-F semi-transparent */
static int s_bwide;                    /* its drawing area is widened (PC_GlPrim.wide) */
static int s_bclip[4] = { 0, 0, -1, -1 };
static int s_bbox[4];                  /* area the pending batch draws on (inclusive) */

/* A native 3D layer being drawn (PC_GlLayer*): its vertices, and the area it draws on. While it
 * is active, emulated primitives with a precise depth are depth-tested against it. */
static int s_layer;
static int s_lclip[4];
static int s_lmark[4];                 /* the union of the areas the layer drew in */
static GLuint s_vao3, s_vbo3;
static GLuint s_hd[MAX_HD];           /* HD textures by handle (pc_dctex.h), made when first drawn */
static uint16_t s_bhd;                 /* the pending batch's HD texture */

/* ---------------------------------------------------------------------------------------------
 * GL objects */

static GLuint compile(GLenum type, const char *src)
{
    GLuint s = glCreateShader(type);
    GLint ok;
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetShaderInfoLog(s, sizeof log, NULL, log);
        fprintf(stderr, "[gl] shader compile error:\n%s\n", log);
    }
    return s;
}

static GLuint program(const char *vs, const char *fs)
{
    GLuint p = glCreateProgram(), v = compile(GL_VERTEX_SHADER, vs),
           f = compile(GL_FRAGMENT_SHADER, fs);
    GLint ok;
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    glDeleteShader(v);
    glDeleteShader(f);
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetProgramInfoLog(p, sizeof log, NULL, log);
        fprintf(stderr, "[gl] program link error:\n%s\n", log);
        glDeleteProgram(p);
        return 0;
    }
    return p;
}

static GLuint texture(GLint ifmt, int w, int h, GLenum fmt, GLenum type, GLint filter)
{
    GLuint t;
    glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D, t);
    glTexImage2D(GL_TEXTURE_2D, 0, ifmt, w, h, 0, fmt, type, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return t;
}

static GLuint framebuffer(GLuint tex)
{
    GLuint f;
    glGenFramebuffers(1, &f);
    glBindFramebuffer(GL_FRAMEBUFFER, f);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        f = 0;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return f;
}

static void target_destroy(Target *t)
{
    glDeleteFramebuffers(1, &t->fbo);
    glDeleteRenderbuffers(1, &t->depth);
    glDeleteTextures(1, &t->tex);
    memset(t, 0, sizeof *t);
}

/* VRAM area x,y w*h, with pad columns of margin on either side, drawn at pw*ph pixels. 0 on
 * failure. */
static int target_create(Target *t, int x, int y, int w, int h, int pad, int pw, int ph)
{
    memset(t, 0, sizeof *t);
    if (pw < 1 || ph < 1 || pw > s_max_tex || ph > s_max_tex)
        return 0;
    t->rx = x;
    t->rw = w;
    t->x = x - pad;
    t->y = y;
    t->w = w + 2 * pad;
    t->h = h;
    w = t->w;
    t->pw = pw;
    t->ph = ph;
    t->sx = (float)pw / (float)w;
    t->sy = (float)ph / (float)h;
    t->tex = texture(GL_RGBA8, pw, ph, GL_RGBA, GL_UNSIGNED_BYTE, GL_NEAREST);
    t->fbo = framebuffer(t->tex);
    glGenRenderbuffers(1, &t->depth);
    glBindRenderbuffer(GL_RENDERBUFFER, t->depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, pw, ph);
    if (t->fbo) {
        glBindFramebuffer(GL_FRAMEBUFFER, t->fbo);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, t->depth);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            target_destroy(t);
            return 0;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    if (!t->fbo || glGetError() != GL_NO_ERROR) {
        target_destroy(t);
        return 0;
    }
    return 1;
}

/* The pixels of a target that VRAM columns (or rows) a..b cover: those whose centres lie inside,
 * as GL rasterises an edge at a * scale. p1 is exclusive. */
static void span(float s, int origin, int a, int b, int *p0, int *p1)
{
    *p0 = (int)ceilf((float)(a - origin) * s - 0.5f);
    *p1 = (int)ceilf((float)(b + 1 - origin) * s - 0.5f);
}

/* Bind a target for drawing (the whole target is the viewport). */
static void use_target(const Target *t)
{
    glBindFramebuffer(GL_FRAMEBUFFER, t->fbo);
    glViewport(0, 0, t->pw, t->ph);
}

/* Scissor to the area x0,y0..x1,y1 (inclusive) of a target: of its VRAM area, or with wide of
 * its whole drawing area (margins included); 0 if none of it is there. */
static int scissor(const Target *t, int x0, int y0, int x1, int y1, int wide)
{
    int px0, px1, py0, py1;
    int l = wide ? t->x : t->rx, r = wide ? t->x + t->w - 1 : t->rx + t->rw - 1;
    if (x0 < l) x0 = l;
    if (y0 < t->y) y0 = t->y;
    if (x1 > r) x1 = r;
    if (y1 > t->y + t->h - 1) y1 = t->y + t->h - 1;
    if (x1 < x0 || y1 < y0)
        return 0;
    span(t->sx, t->x, x0, x1, &px0, &px1);
    span(t->sy, t->y, y0, y1, &py0, &py1);
    glEnable(GL_SCISSOR_TEST);
    glScissor(px0, py0, px1 - px0, py1 - py0);
    return px1 > px0 && py1 > py0;
}

/* The draw program's uniforms for a target. At the PS1's resolution the geometry moves by half a
 * pixel, scaled up only texture coordinates do (see pc_gpu_gl_shaders.h). At a fractional scale
 * a pixel centre can lie exactly on a primitive's top or left edge, where the corner is exactly
 * a texel boundary: stopping 1/64 pixel short keeps it on the primitive's own texel. */
static void draw_uniforms(const Target *t)
{
    int one = t->sx == 1.0f && t->sy == 1.0f;
    float ex = t->sx == floorf(t->sx) ? 0.0f : 1.0f / 64, ey = t->sy == floorf(t->sy) ? 0.0f : 1.0f / 64;
    glUniform4f(s_draw_u_target, (float)t->x, (float)t->y, (float)t->w, (float)t->h);
    glUniform1f(s_draw_u_shift, one ? 0.5f : 0.0f);
    glUniform2f(s_draw_u_uvfix, one ? 0.0f : 0.5f * t->sx - ex, one ? 0.0f : 0.5f * t->sy - ey);
}

/* The targets the VRAM area x0,y0..x1,y1 (inclusive) is drawn on: the surfaces whose VRAM areas
 * it overlaps, and the base unless a single surface holds all of it. Returns how many (base
 * last). */
static int targets_for(int x0, int y0, int x1, int y1, Target **out)
{
    int n = 0, inside = 0;
    for (int i = 0; i < s_nsurf; i++) {
        Target *t = &s_surf[i];
        if (t->rx > x1 || x0 >= t->rx + t->rw || t->y > y1 || y0 >= t->y + t->h)
            continue;
        out[n++] = t;
        if (x0 >= t->rx && x1 < t->rx + t->rw && y0 >= t->y && y1 < t->y + t->h)
            inside = 1;
    }
    if (!inside)
        out[n++] = &s_base;
    return n;
}

/* The targets of a drawing: a widened one (PC_GlPrim.wide = surface + 1) only its surface. */
static int draw_targets(int wide, int x0, int y0, int x1, int y1, Target **out)
{
    if (wide > 0 && wide <= s_nsurf) {
        out[0] = &s_surf[wide - 1];
        return 1;
    }
    return targets_for(x0, y0, x1, y1, out);
}

/* Mark the cells under a drawn area (inclusive, may reach outside VRAM) as drawn on: for a
 * widened drawing only its surface's VRAM area. */
static void mark_drawn(int wide, int x0, int y0, int x1, int y1)
{
    int l = 0, r = VRAM_W - 1;
    if (wide > 0 && wide <= s_nsurf) {
        l = s_surf[wide - 1].rx;
        r = l + s_surf[wide - 1].rw - 1;
    }
    if (x0 < l) x0 = l;
    if (x1 > r) x1 = r;
    if (y0 < 0) y0 = 0;
    if (y1 > VRAM_H - 1) y1 = VRAM_H - 1;
    for (int cy = y0 / CELL; x0 <= x1 && y0 <= y1 && cy <= y1 / CELL; cy++)
        for (int cx = x0 / CELL; cx <= x1 / CELL; cx++)
            s_drawn[cy][cx] = 1;
}

/* The GVert layout, for the bound VAO and array buffer. */
static void vert_attribs(void)
{
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GVert), (void *)offsetof(GVert, x));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(GVert), (void *)offsetof(GVert, r));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GVert), (void *)offsetof(GVert, u));
    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(3, 2, GL_UNSIGNED_SHORT, sizeof(GVert), (void *)offsetof(GVert, clut));
    glEnableVertexAttribArray(4);
    glVertexAttribIPointer(4, 4, GL_UNSIGNED_BYTE, sizeof(GVert), (void *)offsetof(GVert, tw));
    glEnableVertexAttribArray(5);
    glVertexAttribIPointer(5, 4, GL_UNSIGNED_BYTE, sizeof(GVert), (void *)offsetof(GVert, uvr));
}

int PC_GlInit(int fixed_scale, int out_w, int out_h)
{
    if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)) {
        fprintf(stderr, "[gl] cannot load the OpenGL functions\n");
        return -1;
    }
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &s_max_tex);
    s_fixed_scale = fixed_scale > 0 ? fixed_scale : 0;
    s_out_w = out_w > 0 ? out_w : 640;
    s_out_h = out_h > 0 ? out_h : 480;
    s_nsurf = 0;

    s_draw_prog = program(gl_draw_vs, gl_draw_fs);
    s_copy_prog = program(gl_copy_vs, gl_copy_fs);
    s_present_prog = program(gl_present_vs, gl_present_fs);
    if (!s_draw_prog || !s_copy_prog || !s_present_prog)
        return -1;
    s_draw_u_pass = glGetUniformLocation(s_draw_prog, "u_pass");
    s_draw_u_proj = glGetUniformLocation(s_draw_prog, "u_proj");
    s_draw_u_dpass = glGetUniformLocation(s_draw_prog, "u_dpass");
    s_draw_u_target = glGetUniformLocation(s_draw_prog, "u_target");
    s_draw_u_shift = glGetUniformLocation(s_draw_prog, "u_shift");
    s_draw_u_uvfix = glGetUniformLocation(s_draw_prog, "u_uvfix");
    s_copy_u_target = glGetUniformLocation(s_copy_prog, "u_target");
    s_copy_u_map = glGetUniformLocation(s_copy_prog, "u_map");
    s_copy_u_src = glGetUniformLocation(s_copy_prog, "u_src");
    glUseProgram(s_draw_prog);
    glUniform1i(glGetUniformLocation(s_draw_prog, "u_vram"), 0);
    glUniform1i(glGetUniformLocation(s_draw_prog, "u_hd"), 1);
    glUseProgram(s_copy_prog);
    glUniform1i(glGetUniformLocation(s_copy_prog, "u_vram"), 0);
    glUseProgram(s_present_prog);
    glUniform1i(glGetUniformLocation(s_present_prog, "u_tex"), 0);
    glUseProgram(0);

    glGenVertexArrays(1, &s_vao);
    glGenBuffers(1, &s_vbo);
    glBindVertexArray(s_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof s_verts, NULL, GL_STREAM_DRAW);
    vert_attribs();
    glGenVertexArrays(1, &s_vao3);
    glGenBuffers(1, &s_vbo3);
    glBindVertexArray(s_vao3);
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo3);
    vert_attribs();

    glGenVertexArrays(1, &s_copy_vao);
    glGenBuffers(1, &s_copy_vbo);
    glBindVertexArray(s_copy_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_copy_vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);

    glGenVertexArrays(1, &s_present_vao);
    glGenBuffers(1, &s_present_vbo);
    glBindVertexArray(s_present_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_present_vbo);
    glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(float), NULL, GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glBindVertexArray(0);

    const char *f = getenv("THPS_FILTER");
    s_filter = f && f[0] == '1';
    s_blit = texture(GL_R16UI, VRAM_W, VRAM_H, GL_RED_INTEGER, GL_UNSIGNED_SHORT, GL_NEAREST);
    s_texstore = texture(GL_R16UI, VRAM_W, VRAM_H, GL_RED_INTEGER, GL_UNSIGNED_SHORT, GL_NEAREST);
    s_rb_tex = texture(GL_RGBA8, VRAM_W, VRAM_H, GL_RGBA, GL_UNSIGNED_BYTE, GL_NEAREST);
    s_disp_tex = texture(GL_RGBA8, VRAM_W, VRAM_H, GL_RGBA, GL_UNSIGNED_BYTE,
                         s_filter ? GL_LINEAR : GL_NEAREST);
    s_rb_fbo = framebuffer(s_rb_tex);
    if (!target_create(&s_base, 0, 0, VRAM_W, VRAM_H, 0, VRAM_W, VRAM_H) || !s_rb_fbo) {
        fprintf(stderr, "[gl] framebuffer setup failed\n");
        return -1;
    }
    if (glGetError() != GL_NO_ERROR) {
        fprintf(stderr, "[gl] setup failed\n");
        return -1;
    }

    memset(s_drawn, 0, sizeof s_drawn);
    s_nv = 0;
    st.on = getenv("THPS_GLSTATS") && atoi(getenv("THPS_GLSTATS"));
    s_precise = !getenv("THPS_GL_PRECISE") || atoi(getenv("THPS_GL_PRECISE"));
    s_perspective = !getenv("THPS_GL_PERSPECTIVE") || atoi(getenv("THPS_GL_PERSPECTIVE"));
    PC_PreciseEnable(s_precise || s_perspective);
    s_active = 1;
    R3d_Init(1);
    if (s_fixed_scale)
        fprintf(stderr, "[gl] OpenGL renderer: %s, screens at %dx the PS1 resolution\n",
                (const char *)glGetString(GL_RENDERER), s_fixed_scale);
    else
        fprintf(stderr, "[gl] OpenGL renderer: %s, screens at %dx%d\n",
                (const char *)glGetString(GL_RENDERER), s_out_w, s_out_h);
    return 0;
}

void PC_GlShutdown(void)
{
    if (!s_active)
        return;
    PC_PreciseEnable(0);
    glDeleteProgram(s_draw_prog);
    glDeleteProgram(s_copy_prog);
    glDeleteProgram(s_present_prog);
    GLuint vaos[4] = { s_vao, s_copy_vao, s_present_vao, s_vao3 };
    GLuint bufs[4] = { s_vbo, s_copy_vbo, s_present_vbo, s_vbo3 };
    GLuint texs[4] = { s_blit, s_texstore, s_rb_tex, s_disp_tex };
    glDeleteVertexArrays(4, vaos);
    glDeleteBuffers(4, bufs);
    glDeleteTextures(4, texs);
    glDeleteFramebuffers(1, &s_rb_fbo);
    for (int i = 0; i < s_nsurf; i++)
        target_destroy(&s_surf[i]);
    s_nsurf = 0;
    target_destroy(&s_base);
    s_active = 0;
}

/* ---------------------------------------------------------------------------------------------
 * Areas */

/* Call fn for each in-VRAM part of a rectangle that may wrap at the right and bottom edges. */
typedef void (*RectFn)(int x0, int y0, int x1, int y1);   /* inclusive */

static void each_part(int x, int y, int w, int h, RectFn fn)
{
    if (w <= 0 || h <= 0)
        return;
    if (w > VRAM_W) w = VRAM_W;
    if (h > VRAM_H) h = VRAM_H;
    x &= VRAM_W - 1;
    y &= VRAM_H - 1;
    int w0 = x + w > VRAM_W ? VRAM_W - x : w, h0 = y + h > VRAM_H ? VRAM_H - y : h;
    fn(x, y, x + w0 - 1, y + h0 - 1);
    if (w0 < w)
        fn(0, y, w - w0 - 1, y + h0 - 1);
    if (h0 < h)
        fn(x, 0, x + w0 - 1, h - h0 - 1);
    if (w0 < w && h0 < h)
        fn(0, 0, w - w0 - 1, h - h0 - 1);
}

static int any_drawn(int x0, int y0, int x1, int y1)
{
    for (int cy = y0 / CELL; cy <= y1 / CELL; cy++)
        for (int cx = x0 / CELL; cx <= x1 / CELL; cx++)
            if (s_drawn[cy][cx])
                return 1;
    return 0;
}

static int overlaps(const int *a, int x0, int y0, int x1, int y1)
{
    return a[0] <= x1 && x0 <= a[2] && a[1] <= y1 && y0 <= a[3];
}

/* ---------------------------------------------------------------------------------------------
 * Textures */

/* The texture store's changed cells to its texture. */
static void upload_texstore(void)
{
    const uint16_t *tex = PC_TexStoreData();
    uint8_t *cell = PC_TexStoreCells();
    int any = 0;
    for (int cy = 0; cy < CELLS_Y; cy++) {
        uint8_t *row = cell + cy * CELLS_X;
        for (int cx = 0; cx < CELLS_X;) {
            if (!(row[cx] & PC_TEX_DIRTY)) {
                cx++;
                continue;
            }
            int run = cx;
            while (run < CELLS_X && (row[run] & PC_TEX_DIRTY))
                row[run++] &= (uint8_t)~PC_TEX_DIRTY;
            if (!any) {
                any = 1;
                glBindTexture(GL_TEXTURE_2D, s_texstore);
                glPixelStorei(GL_UNPACK_ROW_LENGTH, VRAM_W);
                glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
            }
            int x = cx * CELL, y = cy * CELL;
            st.tex_ups++;
            glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, (run - cx) * CELL, CELL, GL_RED_INTEGER,
                            GL_UNSIGNED_SHORT, tex + y * VRAM_W + x);
            cx = run;
        }
    }
    if (any) {
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    }
}

static void flush(void);

/* Make the textures reflect every write so far. */
static void prepare(void)
{
    upload_texstore();
}

void PC_GlTexturesChanging(int x, int y, int w, int h)
{
    if (!s_active)
        return;
    flush();
    /* what was drawn there is replaced by texels */
    for (int cy = y / CELL; cy <= (y + h - 1) / CELL && cy < CELLS_Y; cy++)
        for (int cx = x / CELL; cx <= (x + w - 1) / CELL && cx < CELLS_X; cx++)
            s_drawn[cy][cx] = 0;
}

/* ---------------------------------------------------------------------------------------------
 * HD textures (pc_dctex.h) */

/* Bind an HD texture to unit 1 (made from its image the first time), then make unit 0 active. */
static void flush(void);

static void hd_bind(uint16_t h)
{
    glActiveTexture(GL_TEXTURE1);
    if (h < MAX_HD && !s_hd[h]) {
        int w = 1, ht = 1;
        uint8_t *rgba = NULL;
        static const uint8_t missing[4] = { 255, 0, 255, 255 };
        glGenTextures(1, &s_hd[h]);
        glBindTexture(GL_TEXTURE_2D, s_hd[h]);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        if (PC_HdTexDecode(h, &w, &ht, &rgba)) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, ht, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
            free(rgba);
        } else {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, missing);
        }
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    } else {
        glBindTexture(GL_TEXTURE_2D, h < MAX_HD ? s_hd[h] : 0);
    }
    glActiveTexture(GL_TEXTURE0);
}

/* PC_GlHiResImage's texture has the last handle (pc_dctex.c leaves it free). */
#define HIRES_SLOT (MAX_HD - 1)

static GVert *begin_prim(const PC_GlPrim *p, int nverts, int bx0, int by0, int bx1, int by1);
static uint16_t prim_info(const PC_GlPrim *p);
static void put(GVert *o, const PC_GlPrim *p, uint16_t info, float x, float y, uint8_t r,
                uint8_t g, uint8_t b, int u, int v);

void PC_GlHiResImage(int x, int y, int w, int h, const uint16_t *src, int sw, int sh)
{
    uint8_t *rgba = malloc((size_t)sw * sh * 4);
    for (int i = 0; i < sw * sh; i++) {
        uint16_t p = src[i];
        rgba[i * 4] = (uint8_t)((p & 31) * 255 / 31);
        rgba[i * 4 + 1] = (uint8_t)((p >> 5 & 31) * 255 / 31);
        rgba[i * 4 + 2] = (uint8_t)((p >> 10 & 31) * 255 / 31);
        rgba[i * 4 + 3] = 255;   /* every pixel is written, as by LoadImage */
    }
    flush();
    if (!s_hd[HIRES_SLOT])
        glGenTextures(1, &s_hd[HIRES_SLOT]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, s_hd[HIRES_SLOT]);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, sw, sh, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glActiveTexture(GL_TEXTURE0);
    free(rgba);

    /* a raw textured rectangle over the area, its texture spanning 0..256 */
    PC_GlPrim p;
    memset(&p, 0, sizeof p);
    p.textured = 1;
    p.raw = 1;
    p.depth = 3;
    p.clut_x = (HIRES_SLOT & 63) * 16;
    p.clut_y = HIRES_SLOT >> 6;
    p.tw_and_x = p.tw_and_y = 0xff;
    p.clip_l = x;
    p.clip_t = y;
    p.clip_r = x + w - 1;
    p.clip_b = y + h - 1;
    GVert *o = begin_prim(&p, 6, x, y, x + w - 1, y + h - 1);
    if (o) {
        uint16_t info = prim_info(&p);
        put(&o[0], &p, info, (float)x, (float)y, 128, 128, 128, 0, 0);
        put(&o[1], &p, info, (float)(x + w), (float)y, 128, 128, 128, 256, 0);
        put(&o[2], &p, info, (float)x, (float)(y + h), 128, 128, 128, 0, 256);
        o[3] = o[1];
        put(&o[4], &p, info, (float)(x + w), (float)(y + h), 128, 128, 128, 256, 256);
        o[5] = o[2];
    }
    flush();
}

void PC_GlHdForget(uint16_t h)
{
    if (!s_active || h >= MAX_HD || !s_hd[h])
        return;
    if (s_nv && s_bhd == h)
        flush();
    glDeleteTextures(1, &s_hd[h]);
    s_hd[h] = 0;
}

/* ---------------------------------------------------------------------------------------------
 * Batches */

static void set_blend(int pass)
{
    glEnable(GL_BLEND);
    if (pass == 2) {
        glBlendEquationSeparate(GL_FUNC_REVERSE_SUBTRACT, GL_FUNC_ADD);
        glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ZERO);
    } else {
        glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
        glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC1_ALPHA, GL_ONE, GL_ZERO);
    }
    glUniform1i(s_draw_u_pass, pass);
}

static void flush(void)
{
    if (!s_nv)
        return;
    prepare();

    /* the area drawn on */
    int x0 = s_bbox[0] < s_bclip[0] ? s_bclip[0] : s_bbox[0];
    int y0 = s_bbox[1] < s_bclip[1] ? s_bclip[1] : s_bbox[1];
    int x1 = s_bbox[2] > s_bclip[2] ? s_bclip[2] : s_bbox[2];
    int y1 = s_bbox[3] > s_bclip[3] ? s_bclip[3] : s_bbox[3];

    glUseProgram(s_draw_prog);
    if (s_bhd)
        hd_bind(s_bhd);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s_texstore);
    glBindVertexArray(s_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof s_verts, NULL, GL_STREAM_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)s_nv * sizeof(GVert), s_verts);
    if (s_layer) {
        /* inside a native 3D layer: test (don't write) the depth of projected primitives */
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);
    }
    Target *ts[MAX_SURFACES + 1];
    int nt = x0 <= x1 && y0 <= y1 ? draw_targets(s_bwide, x0, y0, x1, y1, ts) : 0;
    for (int i = 0; i < nt; i++) {
        use_target(ts[i]);
        if (!scissor(ts[i], s_bclip[0], s_bclip[1], s_bclip[2], s_bclip[3], s_bwide))
            continue;
        draw_uniforms(ts[i]);
        if (s_batch_sub) {
            set_blend(1);
            glDrawArrays(GL_TRIANGLES, 0, s_nv);
            set_blend(2);
        } else {
            set_blend(0);
        }
        glDrawArrays(GL_TRIANGLES, 0, s_nv);
        st.draws++;
    }
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);

    /* drawn on (the render-to-texture check, check_texture) */
    mark_drawn(s_bwide, x0, y0, x1, y1);
    s_nv = 0;
}

/* Footprint of a textured primitive's texel reads: its texture page area and CLUT row. Textures
 * come from the texture store, which nothing draws into: drawing there would be the game using
 * what it drew as a texture, which THPS doesn't do (logged once with THPS_TRACE). */
static void check_texture(const PC_GlPrim *p, int umin, int vmin, int umax, int vmax)
{
    int rects[2][4], n = 0;
    if (!pc_trace || p->tw_and_x != 0xff || p->tw_and_y != 0xff || umin < 0 || vmin < 0 ||
        umax > 255 || vmax > 255)
        return;   /* only exact texel ranges: a whole page also covers what others drew next to it */
    int shift = p->depth == 0 ? 2 : p->depth == 1 ? 1 : 0;
    rects[n][0] = p->tp_x + (umin >> shift);
    rects[n][1] = p->tp_y + vmin;
    rects[n][2] = p->tp_x + (umax >> shift);
    rects[n][3] = p->tp_y + vmax;
    n++;
    if (p->depth < 2) {
        rects[n][0] = p->clut_x;
        rects[n][1] = p->clut_y;
        rects[n][2] = p->clut_x + (p->depth == 0 ? 15 : 255);
        rects[n][3] = p->clut_y;
        n++;
    }
    for (int i = 0; i < n; i++) {
        int *r = rects[i];
        if (r[2] > VRAM_W - 1) r[2] = VRAM_W - 1;   /* pages near the right edge wrap; rare */
        if (r[3] > VRAM_H - 1) r[3] = VRAM_H - 1;
        if (s_nv && overlaps(s_bbox, r[0], r[1], r[2], r[3]))
            flush();
        if (any_drawn(r[0], r[1], r[2], r[3])) {
            static int warned;
            if (!warned) {
                warned = 1;
                PC_Trace("[gl] a texture at %d,%d-%d,%d was drawn into: render-to-texture?", r[0],
                         r[1], r[2], r[3]);
            }
        }
    }
}

/* Start or continue a batch for one primitive covering bx0..bx1, by0..by1; NULL if it is clipped
 * away entirely. */
static GVert *begin_prim(const PC_GlPrim *p, int nverts, int bx0, int by0, int bx1, int by1)
{
    static int warned_mask;
    if (p->clip_r < p->clip_l || p->clip_b < p->clip_t)
        return NULL;
    if (bx1 < p->clip_l || bx0 > p->clip_r || by1 < p->clip_t || by0 > p->clip_b)
        return NULL;
    if (bx0 < p->clip_l) bx0 = p->clip_l;
    if (by0 < p->clip_t) by0 = p->clip_t;
    if (bx1 > p->clip_r) bx1 = p->clip_r;
    if (by1 > p->clip_b) by1 = p->clip_b;
    if (p->mask_check && !warned_mask) {
        warned_mask = 1;
        fprintf(stderr, "[gl] mask-bit test is not implemented (drawing ignores it)\n");
    }
    int sub = p->semi && p->abr == 2;
    uint16_t hd = p->textured && p->depth == 3 ? (uint16_t)((p->clut_x / 16) | (p->clut_y << 6)) : 0;
    if (s_nv && (sub != s_batch_sub || hd != s_bhd || p->wide != s_bwide ||
                 s_nv + nverts > MAX_VERTS ||
                 s_bclip[0] != p->clip_l || s_bclip[1] != p->clip_t ||
                 s_bclip[2] != p->clip_r || s_bclip[3] != p->clip_b))
        flush();
    if (!s_nv) {
        s_batch_sub = sub;
        s_bhd = hd;
        s_bwide = p->wide;
        s_bclip[0] = p->clip_l;
        s_bclip[1] = p->clip_t;
        s_bclip[2] = p->clip_r;
        s_bclip[3] = p->clip_b;
        s_bbox[0] = bx0;
        s_bbox[1] = by0;
        s_bbox[2] = bx1;
        s_bbox[3] = by1;
    } else {
        if (bx0 < s_bbox[0]) s_bbox[0] = bx0;
        if (by0 < s_bbox[1]) s_bbox[1] = by0;
        if (bx1 > s_bbox[2]) s_bbox[2] = bx1;
        if (by1 > s_bbox[3]) s_bbox[3] = by1;
    }
    GVert *v = &s_verts[s_nv];
    s_nv += nverts;
    return v;
}

static uint16_t prim_info(const PC_GlPrim *p)
{
    uint16_t info = (uint16_t)(((p->tp_x / 64) & 15) | ((p->tp_y / 256) & 1) << 4 |
                               (p->abr & 3) << 5 | (p->depth & 3) << 7);
    if (p->textured) info |= I_TEXTURED;
    if (p->raw) info |= I_RAW;
    if (p->semi) info |= I_SEMI;
    if (p->mask_set) info |= I_MASK;
    return info;
}

static void put(GVert *o, const PC_GlPrim *p, uint16_t info, float x, float y, uint8_t r,
                uint8_t g, uint8_t b, int u, int v)
{
    o->x = x;
    o->y = y;
    o->z = 1.0f;
    o->r = r;
    o->g = g;
    o->b = b;
    o->a = 0;
    o->u = (float)u;
    o->v = (float)v;
    o->clut = (uint16_t)((p->clut_x / 16) | (p->clut_y << 6));
    o->info = info;
    o->tw[0] = p->tw_and_x;
    o->tw[1] = p->tw_and_y;
    o->tw[2] = p->tw_or_x;
    o->tw[3] = p->tw_or_y;
    o->uvr[0] = o->uvr[1] = 0;
    o->uvr[2] = o->uvr[3] = 255;
}

static void put_v(GVert *o, const PC_GlPrim *p, uint16_t info, const PC_GlVertex *s, int precise)
{
    if (!precise) {
        put(o, p, info, (float)s->x, (float)s->y, s->r, s->g, s->b, s->u, s->v);
        return;
    }
    put(o, p, info, s_precise ? s->px : (float)s->x, s_precise ? s->py : (float)s->y, s->r, s->g,
        s->b, s->u, s->v);
    if (s_perspective)
        o->z = s->w;
}

/* ---------------------------------------------------------------------------------------------
 * Primitives */

#define MIN3(a, b, c) ((a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))
#define MAX3(a, b, c) ((a) > (b) ? ((a) > (c) ? (a) : (c)) : ((b) > (c) ? (b) : (c)))

/* The texels the PS1 samples on a triangle with whole-pixel corners: its texture coordinates at
 * the first and last pixel corners it covers (x0..x1-1, y0..y1-1), rounded, and never past its
 * vertices'. Scaled up, pixel centres also land between the PS1's last corner and the edge, which
 * would round onto the texel beyond it: 2D quads give the exclusive u0+w, v0+h there (a font's
 * neighbouring glyph showed as a line beside scaled text). The half of a rectangle's quad is
 * measured over the whole rectangle, its other half has the same gradients. */
static void uv_range(GVert *o, const PC_GlVertex *a, const PC_GlVertex *b, const PC_GlVertex *c,
                     int x0, int y0, int x1, int y1)
{
    double det = (double)(b->x - a->x) * (c->y - a->y) - (double)(c->x - a->x) * (b->y - a->y);
    if (det == 0.0 || x1 <= x0 || y1 <= y0)
        return;
    int uv[2][3] = { { a->u, b->u, c->u }, { a->v, b->v, c->v } };
    uint8_t r[4];
    for (int k = 0; k < 2; k++) {
        const int *t = uv[k];
        double dx = ((double)(t[1] - t[0]) * (c->y - a->y) - (double)(t[2] - t[0]) * (b->y - a->y)) / det;
        double dy = ((double)(t[2] - t[0]) * (b->x - a->x) - (double)(t[1] - t[0]) * (c->x - a->x)) / det;
        double base = t[0] + dx * (x0 - a->x) + dy * (y0 - a->y);
        double lo = base + (dx < 0 ? dx * (x1 - 1 - x0) : 0) + (dy < 0 ? dy * (y1 - 1 - y0) : 0);
        double hi = base + (dx > 0 ? dx * (x1 - 1 - x0) : 0) + (dy > 0 ? dy * (y1 - 1 - y0) : 0);
        int mn = (int)floor(lo + 0.5 - 1e-3), mx = (int)floor(hi + 0.5 + 1e-3);
        int vmn = MIN3(t[0], t[1], t[2]), vmx = MAX3(t[0], t[1], t[2]);
        r[k] = (uint8_t)(mn > vmn ? (mn < vmx ? mn : vmx) : vmn);
        r[k + 2] = (uint8_t)(mx < vmx ? (mx > r[k] ? mx : r[k]) : vmx);
    }
    for (int i = 0; i < 3; i++)
        memcpy(o[i].uvr, r, 4);
}

void PC_GlTriangle(const PC_GlPrim *p, const PC_GlVertex *a, const PC_GlVertex *b,
                   const PC_GlVertex *c)
{
    if (p->textured && p->depth != 3)
        check_texture(p, MIN3(a->u, b->u, c->u), MIN3(a->v, b->v, c->v), MAX3(a->u, b->u, c->u),
                     MAX3(a->v, b->v, c->v));
    int precise = a->w > 0 && b->w > 0 && c->w > 0;
    PC_GlVertex wv[3];
    if (p->wide > 0 && p->wide <= s_nsurf && !p->textured && !precise) {
        /* widescreen: a flat 2D polygon across the whole screen (the pause menu's dimming) spans
         * the margins as well: its corners on the screen's sides move to the margins' */
        const Target *t = &s_surf[p->wide - 1];
        int l = t->rx, r = t->rx + t->rw;
        if (MIN3(a->x, b->x, c->x) <= l && MAX3(a->x, b->x, c->x) >= r) {
            wv[0] = *a;
            wv[1] = *b;
            wv[2] = *c;
            for (int i = 0; i < 3; i++)
                wv[i].x = wv[i].x <= l ? t->x : wv[i].x >= r ? t->x + t->w : wv[i].x;
            a = &wv[0];
            b = &wv[1];
            c = &wv[2];
        }
    }
    int bx0 = MIN3(a->x, b->x, c->x), by0 = MIN3(a->y, b->y, c->y);
    int bx1 = MAX3(a->x, b->x, c->x), by1 = MAX3(a->y, b->y, c->y);
    if (precise && s_precise) {
        /* the hardware's rounded positions can be several pixels off near the camera */
        bx0 = (int)floorf(MIN3(a->px, b->px, c->px));
        by0 = (int)floorf(MIN3(a->py, b->py, c->py));
        bx1 = (int)ceilf(MAX3(a->px, b->px, c->px));
        by1 = (int)ceilf(MAX3(a->py, b->py, c->py));
    }
    GVert *o = begin_prim(p, 3, bx0, by0, bx1, by1);
    if (!o)
        return;
    uint16_t info = prim_info(p);
    if (precise && s_layer && s_perspective)
        info |= I_DEPTH;
    put_v(&o[0], p, info, a, precise);
    put_v(&o[1], p, info, b, precise);
    put_v(&o[2], p, info, c, precise);
    if (p->textured && p->depth != 3 && !(precise && s_precise))
        uv_range(o, a, b, c, bx0, by0, bx1, by1);
}

void PC_GlRect(const PC_GlPrim *p, int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b,
               int u0, int v0, int flip_x, int flip_y)
{
    if (w <= 0 || h <= 0)
        return;
    /* the texel at pixel x+i is u0+i, or u0-i when flipped (the shader rounds, see there) */
    int ul = u0, ur = flip_x ? u0 - w : u0 + w;
    int vt = v0, vb = flip_y ? v0 - h : v0 + h;
    if (p->textured && p->depth != 3) {
        int umin = flip_x ? u0 - w + 1 : u0, vmin = flip_y ? v0 - h + 1 : v0;
        check_texture(p, umin & 255, vmin & 255, (umin & 255) + w - 1, (vmin & 255) + h - 1);
    }
    if (p->wide > 0 && p->wide <= s_nsurf && !p->textured) {
        /* widescreen: a flat rectangle across the whole screen (a fade, the pause menu's dimming,
         * a flash) spans the margins as well */
        const Target *t = &s_surf[p->wide - 1];
        if (x <= t->rx && x + w >= t->rx + t->rw) {
            x = t->x;
            w = t->w;
        }
    }
    GVert *o = begin_prim(p, 6, x, y, x + w - 1, y + h - 1);
    if (!o)
        return;
    uint16_t info = prim_info(p);
    float x0 = (float)x, x1 = (float)(x + w), y0 = (float)y, y1 = (float)(y + h);
    put(&o[0], p, info, x0, y0, r, g, b, ul, vt);
    put(&o[1], p, info, x1, y0, r, g, b, ur, vt);
    put(&o[2], p, info, x0, y1, r, g, b, ul, vb);
    o[3] = o[1];
    put(&o[4], p, info, x1, y1, r, g, b, ur, vb);
    o[5] = o[2];
}

void PC_GlLine(const PC_GlPrim *p, const PC_GlVertex *a, const PC_GlVertex *b)
{
    int dx = b->x - a->x, dy = b->y - a->y;
    if ((dx < 0 ? -dx : dx) >= 1024 || (dy < 0 ? -dy : dy) >= 512)
        return;   /* the GPU drops over-long lines */
    const PC_GlVertex *s = a, *e = b;
    int xmajor = (dx < 0 ? -dx : dx) >= (dy < 0 ? -dy : dy);
    if ((xmajor && dx < 0) || (!xmajor && dy < 0)) {
        s = b;
        e = a;
    }
    GVert *o = begin_prim(p, 6, s->x < e->x ? s->x : e->x, s->y < e->y ? s->y : e->y,
                          s->x > e->x ? s->x : e->x, s->y > e->y ? s->y : e->y);
    if (!o)
        return;
    /* a one-pixel-wide quad along the major axis, both end points included */
    uint16_t info = prim_info(p);
    float sx = (float)s->x, sy = (float)s->y, ex = (float)e->x, ey = (float)e->y;
    if (xmajor) {
        put(&o[0], p, info, sx, sy, s->r, s->g, s->b, 0, 0);
        put(&o[1], p, info, ex + 1, ey, e->r, e->g, e->b, 0, 0);
        put(&o[2], p, info, sx, sy + 1, s->r, s->g, s->b, 0, 0);
        put(&o[4], p, info, ex + 1, ey + 1, e->r, e->g, e->b, 0, 0);
    } else {
        put(&o[0], p, info, sx, sy, s->r, s->g, s->b, 0, 0);
        put(&o[1], p, info, sx + 1, sy, s->r, s->g, s->b, 0, 0);
        put(&o[2], p, info, ex, ey + 1, e->r, e->g, e->b, 0, 0);
        put(&o[4], p, info, ex + 1, ey + 1, e->r, e->g, e->b, 0, 0);
    }
    o[3] = o[1];
    o[5] = o[2];
}

static void fill_part(int x0, int y0, int x1, int y1)
{
    Target *ts[MAX_SURFACES + 1];
    int nt = targets_for(x0, y0, x1, y1, ts);
    for (int i = 0; i < nt; i++) {
        const Target *t = ts[i];
        int all = t != &s_base && x0 == t->rx && x1 == t->rx + t->rw - 1 && y0 == t->y &&
                  y1 == t->y + t->h - 1;   /* the whole screen: with the margins */
        use_target(t);
        if (scissor(t, all ? t->x : x0, y0, all ? t->x + t->w - 1 : x1, y1, all))
            glClear(GL_COLOR_BUFFER_BIT);
    }
}

void PC_GlFill(int x, int y, int w, int h, uint32_t rgb24)
{
    static int dbg = -1;
    if (dbg < 0)
        dbg = getenv("THPS_DBG_FILL") ? (int)strtoul(getenv("THPS_DBG_FILL"), NULL, 16) : 0;
    if (dbg && w >= 256 && h >= 200)
        rgb24 = (uint32_t)dbg;   /* THPS_DBG_FILL=bbggrr: screen clears in a colour gaps show */
    flush();
    prepare();
    /* the fill colour is reduced to 15 bits, like everything else in VRAM */
    glClearColor((float)((rgb24 >> 3) & 31) / 31.0f, (float)((rgb24 >> 11) & 31) / 31.0f,
                 (float)((rgb24 >> 19) & 31) / 31.0f, 0.0f);
    each_part(x, y, w, h, fill_part);
    glDisable(GL_SCISSOR_TEST);
}

/* ---------------------------------------------------------------------------------------------
 * Native 3D layers */

static PC_GlLayerParams s_lp;

static int s_lwide;   /* the layer's drawing area is widened (PC_GlLayerParams.wide) */

static int layer_targets(Target **ts)
{
    return draw_targets(s_lwide, s_lclip[0], s_lclip[1], s_lclip[2], s_lclip[3], ts);
}

static void layer_area(int ofs_x, int ofs_y, const int clip[4], int wide)
{
    s_lp.ofs_x = ofs_x;
    s_lp.ofs_y = ofs_y;
    s_lwide = wide > 0 && wide <= s_nsurf ? wide : 0;
    s_lclip[0] = clip[0] < 0 && !s_lwide ? 0 : clip[0];
    s_lclip[1] = clip[1] < 0 ? 0 : clip[1];
    s_lclip[2] = clip[2] > VRAM_W - 1 && !s_lwide ? VRAM_W - 1 : clip[2];
    s_lclip[3] = clip[3] > VRAM_H - 1 ? VRAM_H - 1 : clip[3];
}

void PC_GlLayerArea(int ofs_x, int ofs_y, const int clip[4], int wide)
{
    if (!s_layer)
        return;
    layer_area(ofs_x, ofs_y, clip, wide);
    if (s_lclip[2] < s_lclip[0] || s_lclip[3] < s_lclip[1])
        return;
    for (int i = 0; i < 2; i++) {
        if (s_lclip[i] < s_lmark[i])
            s_lmark[i] = s_lclip[i];
        if (s_lclip[i + 2] > s_lmark[i + 2])
            s_lmark[i + 2] = s_lclip[i + 2];
    }
}

void PC_GlLayerBegin(const PC_GlLayerParams *lp, const PC_GlVert *v, int nv)
{
    flush();
    prepare();
    s_lp = *lp;
    {
        int clip[4] = { lp->clip_l, lp->clip_t, lp->clip_r, lp->clip_b };
        layer_area(lp->ofs_x, lp->ofs_y, clip, lp->wide);
    }
    if (s_lclip[2] < s_lclip[0] || s_lclip[3] < s_lclip[1])
        return;
    memcpy(s_lmark, s_lclip, sizeof s_lmark);
    glBindVertexArray(s_vao3);
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo3);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)nv * sizeof(GVert), v, GL_STREAM_DRAW);
    glDepthMask(GL_TRUE);
    glClearDepth(1.0);
    Target *ts[MAX_SURFACES + 1];
    int nt = layer_targets(ts);
    for (int i = 0; i < nt; i++) {
        /* all of it: the drawing area can change while the layer draws (PC_GlLayerArea) */
        use_target(ts[i]);
        glDisable(GL_SCISSOR_TEST);
        glClear(GL_DEPTH_BUFFER_BIT);
    }
    s_layer = 1;
}

void PC_GlLayerDraw(int first, int count, int cls, uint16_t hd)
{
    if (!s_layer || count <= 0)
        return;
    flush();        /* emulated primitives submitted before these are behind them */
    prepare();
    glUseProgram(s_draw_prog);
    if (hd)
        hd_bind(hd);
    glUniform4f(s_draw_u_proj, s_lp.ofs_x + s_lp.ofx, s_lp.ofs_y + s_lp.ofy, s_lp.h, s_lp.hither);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s_texstore);
    glBindVertexArray(s_vao3);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CLIP_DISTANCE0);
    Target *ts[MAX_SURFACES + 1];
    int nt = layer_targets(ts);
    for (int i = 0; i < nt; i++) {
        use_target(ts[i]);
        if (!scissor(ts[i], s_lclip[0], s_lclip[1], s_lclip[2], s_lclip[3], s_lwide))
            continue;
        draw_uniforms(ts[i]);
        glDepthMask(GL_FALSE);
        glUniform1i(s_draw_u_dpass, 0);
        if (cls == 2) {
            set_blend(1);
            glDrawArrays(GL_TRIANGLES, first, count);
            set_blend(2);
        } else {
            set_blend(0);
        }
        glDrawArrays(GL_TRIANGLES, first, count);
        st.draws++;
        if (cls == 0) {
            /* opaque: now the true depth (see the shaders) */
            glUniform1i(s_draw_u_dpass, 1);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            glDepthMask(GL_TRUE);
            glDrawArrays(GL_TRIANGLES, first, count);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glUniform1i(s_draw_u_dpass, 0);
        }
    }
    glDisable(GL_CLIP_DISTANCE0);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
}

void PC_GlLayerEnd(void)
{
    if (!s_layer)
        return;
    flush();
    s_layer = 0;
    mark_drawn(s_lwide, s_lmark[0], s_lmark[1], s_lmark[2], s_lmark[3]);
}

/* ---------------------------------------------------------------------------------------------
 * Reading and writing screens' pixels */

/* 8-bit channel -> 5 bits: exact for every value a 5-bit one was stored as (n * 255 / 31,
 * rounded), and the PS1's truncation for the in-between values drawing produces. */
static uint8_t s_to5[256];

/* The pixels of an area (inclusive, inside VRAM) as drawn, at the PS1's resolution, into out
 * (rows of x1 - x0 + 1, 15-bit with the mask bit). Each pixel comes from the target it belongs
 * to: surfaces are scaled down (nearest), whole 16x16 cells at a time. */
static void read_area(int x0, int y0, int x1, int y1, uint16_t *out)
{
    static uint8_t buf[VRAM_W * VRAM_H * 4];
    if (!s_to5[255])
        for (int i = 0; i < 256; i++)
            s_to5[i] = (uint8_t)((i * 31 + 16) / 255);
    flush();
    int px = x0 / CELL * CELL, py = y0 / CELL * CELL;
    int pw = (x1 / CELL + 1) * CELL - px, ph = (y1 / CELL + 1) * CELL - py;
    /* the base, then the surfaces over it */
    Target *ts[MAX_SURFACES + 1];
    int nt = targets_for(px, py, px + pw - 1, py + ph - 1, ts);
    glDisable(GL_SCISSOR_TEST);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, s_rb_fbo);
    for (int i = nt - 1; i >= 0; i--) {
        const Target *t = ts[i];
        int ax0 = px > t->rx ? px : t->rx, ay0 = py > t->y ? py : t->y;
        int ax1 = px + pw < t->rx + t->rw ? px + pw : t->rx + t->rw;
        int ay1 = py + ph < t->y + t->h ? py + ph : t->y + t->h;
        int sx0, sx1, sy0, sy1;
        if (ax1 <= ax0 || ay1 <= ay0)
            continue;
        span(t->sx, t->x, ax0, ax1 - 1, &sx0, &sx1);
        span(t->sy, t->y, ay0, ay1 - 1, &sy0, &sy1);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, t->fbo);
        glBlitFramebuffer(sx0, sy0, sx1, sy1, ax0, ay0, ax1, ay1, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
    glBindFramebuffer(GL_READ_FRAMEBUFFER, s_rb_fbo);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glReadPixels(px, py, pw, ph, GL_RGBA, GL_UNSIGNED_BYTE, buf);
    st.readbacks++;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    int w = x1 - x0 + 1;
    for (int y = y0; y <= y1; y++) {
        const uint8_t *s = buf + ((size_t)(y - py) * pw + (x0 - px)) * 4;
        uint16_t *d = out + (size_t)(y - y0) * w;
        for (int i = 0; i < w; i++, s += 4)
            d[i] = (uint16_t)(s_to5[s[0]] | s_to5[s[1]] << 5 | s_to5[s[2]] << 10 |
                              (s[3] >= 128 ? 0x8000 : 0));
    }
}

/* Clip an area to VRAM; 0 if nothing is left. */
static int clip_area(int *x, int *y, int *w, int *h)
{
    if (*x < 0) { *w += *x; *x = 0; }
    if (*y < 0) { *h += *y; *y = 0; }
    if (*x + *w > VRAM_W) *w = VRAM_W - *x;
    if (*y + *h > VRAM_H) *h = VRAM_H - *y;
    return *w > 0 && *h > 0;
}

void PC_GlRead(int x, int y, int w, int h, uint16_t *out)
{
    int ox = x, oy = y, ow = w;
    if (!s_active || !clip_area(&x, &y, &w, &h))
        return;
    if (ox == x && oy == y && ow == w) {
        read_area(x, y, x + w - 1, y + h - 1, out);
        return;
    }
    uint16_t *tmp = malloc((size_t)w * h * 2);
    read_area(x, y, x + w - 1, y + h - 1, tmp);
    for (int j = 0; j < h; j++)
        memcpy(out + (size_t)(y - oy + j) * ow + (x - ox), tmp + (size_t)j * w, (size_t)w * 2);
    free(tmp);
}

void PC_GlBlit(int x, int y, int w, int h, const uint16_t *src)
{
    int sw = w;
    if (!s_active)
        return;
    {
        int ox = x, oy = y;
        if (!clip_area(&x, &y, &w, &h))
            return;
        src += (size_t)(y - oy) * sw + (x - ox);
    }
    flush();   /* primitives already sent draw before these pixels arrive */
    glBindTexture(GL_TEXTURE_2D, s_blit);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, sw);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RED_INTEGER, GL_UNSIGNED_SHORT, src);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    float x0 = (float)x, x1 = (float)(x + w), y0 = (float)y, y1 = (float)(y + h);
    float q[12] = { x0, y0, x1, y0, x0, y1, x1, y0, x1, y1, x0, y1 };
    glDisable(GL_BLEND);
    glUseProgram(s_copy_prog);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s_blit);
    glBindVertexArray(s_copy_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_copy_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof q, q, GL_STREAM_DRAW);
    glUniform2f(s_copy_u_src, (float)x, (float)y);
    Target *ts[MAX_SURFACES + 1];
    int nt = targets_for(x, y, x + w - 1, y + h - 1, ts);
    for (int i = 0; i < nt; i++) {
        const Target *t = ts[i];
        use_target(t);
        if (!scissor(t, x, y, x + w - 1, y + h - 1, 0))
            continue;
        glUniform4f(s_copy_u_target, (float)t->x, (float)t->y, (float)t->w, (float)t->h);
        glUniform4f(s_copy_u_map, (float)t->x, (float)t->y, t->sx, t->sy);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    glDisable(GL_SCISSOR_TEST);
    st.blits++;
}

/* Copy a target's pixels of the VRAM area x, y, w, h into another target (nearest). */
static void copy_area(const Target *from, const Target *to, int x, int y, int w, int h)
{
    int fx0, fx1, fy0, fy1, tx0, tx1, ty0, ty1;
    span(from->sx, from->x, x, x + w - 1, &fx0, &fx1);
    span(from->sy, from->y, y, y + h - 1, &fy0, &fy1);
    span(to->sx, to->x, x, x + w - 1, &tx0, &tx1);
    span(to->sy, to->y, y, y + h - 1, &ty0, &ty1);
    glDisable(GL_SCISSOR_TEST);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, from->fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, to->fbo);
    glBlitFramebuffer(fx0, fy0, fx1, fy1, tx0, ty0, tx1, ty1, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/* The display surface holding all of a VRAM area, or NULL. */
static Target *surface_for(int x, int y, int w, int h)
{
    for (int i = 0; i < s_nsurf; i++) {
        Target *t = &s_surf[i];
        if (x >= t->rx && y >= t->y && x + w <= t->rx + t->rw && y + h <= t->y + t->h)
            return t;
    }
    return NULL;
}

/* The VRAM area of a display (x, y, w, h) as shown: a widened surface's margins too. */
static const Target *shown_area(int *x, int *w, int y, int h)
{
    const Target *t = surface_for(*x, y, *w, h);
    if (!t)
        return &s_base;
    if (*x == t->rx && *w == t->rw) {
        *x = t->x;
        *w = t->w;
    }
    return t;
}

uint32_t *PC_GlReadArea(int x, int y, int w, int h, int *out_w, int *out_h)
{
    if (!s_active || w <= 0 || h <= 0 || x < 0 || y < 0 || x + w > VRAM_W || y + h > VRAM_H)
        return NULL;
    const Target *t = shown_area(&x, &w, y, h);
    int px0, px1, py0, py1;
    span(t->sx, t->x, x, x + w - 1, &px0, &px1);
    span(t->sy, t->y, y, y + h - 1, &py0, &py1);
    int sw = px1 - px0, sh = py1 - py0;
    uint32_t *out = (uint32_t *)malloc((size_t)sw * sh * 4);
    if (!out)
        return NULL;
    flush();
    prepare();
    glBindFramebuffer(GL_READ_FRAMEBUFFER, t->fbo);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glReadPixels(px0, py0, sw, sh, GL_BGRA, GL_UNSIGNED_BYTE, out);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    for (int i = 0; i < sw * sh; i++)
        out[i] |= 0xff000000u;
    *out_w = sw;
    *out_h = sh;
    return out;
}

/* ---------------------------------------------------------------------------------------------
 * Display surfaces */

/* Widescreen margins of a w*h screen: only gameplay's 512x240 buffers have them. */
static int screen_pad(int w, int h)
{
    return w == 512 && h == 240 ? s_pad : 0;
}

/* The picture of a screen whose drawing area is w wide (its VRAM area rw wide) in the window:
 * 4:3 widened by the margins, as large as fits. */
static void picture_rect(int w, int rw, int *r)
{
    double a = 4.0 / 3.0 * w / rw;
    int dw = s_out_w, dh = s_out_h;
    if (s_out_w > s_out_h * a)
        dw = (int)(s_out_h * a + 0.5);
    else
        dh = (int)(s_out_w / a + 0.5);
    if (dw > s_out_w)
        dw = s_out_w;
    if (dh > s_out_h)
        dh = s_out_h;
    r[0] = (s_out_w - dw) / 2;
    r[1] = (s_out_h - dh) / 2;
    r[2] = dw;
    r[3] = dh;
}

/* Size of a surface for a w*h screen with pad columns of margin, in pixels. */
static void surface_size(int w, int h, int pad, int *pw, int *ph)
{
    if (s_fixed_scale) {
        *pw = (w + 2 * pad) * s_fixed_scale;
        *ph = h * s_fixed_scale;
    } else {
        int r[4];
        picture_rect(w + 2 * pad, w, r);
        *pw = r[2];
        *ph = r[3];
    }
}

/* Give surface i's pixels back to the base and drop it. */
static void surface_retire(int i)
{
    Target *t = &s_surf[i];
    copy_area(t, &s_base, t->rx, t->y, t->rw, t->h);
    target_destroy(t);
    memmove(&s_surf[i], &s_surf[i + 1], (size_t)(s_nsurf - i - 1) * sizeof(Target));
    s_nsurf--;
}

/* A surface for a VRAM area, with what the base has there. */
static void surface_add(int x, int y, int w, int h)
{
    int pw, ph, pad = screen_pad(w, h);
    if (s_nsurf == MAX_SURFACES)
        surface_retire(0);
    surface_size(w, h, pad, &pw, &ph);
    if (!target_create(&s_surf[s_nsurf], x, y, w, h, pad, pw, ph)) {
        fprintf(stderr, "[gl] no %dx%d surface for the screen at %d,%d\n", pw, ph, x, y);
        return;
    }
    copy_area(&s_base, &s_surf[s_nsurf], x, y, w, h);
    s_nsurf++;
    if (st.on)
        fprintf(stderr, "[gl] surface for the screen at %d,%d %dx%d (+%d each side): %dx%d pixels\n",
                x, y, w, h, pad, pw, ph);
}

void PC_GlNoteScreen(int x, int y, int w, int h)
{
    /* (at 1x the base is the same thing, and the 1x comparisons stay exact) */
    if (!s_active || s_fixed_scale == 1 || w < 256 || h < 192 || x < 0 || y < 0 ||
        x + w > VRAM_W || y + h > VRAM_H)
        return;
    for (int i = 0; i < s_nsurf; i++)
        if (s_surf[i].rx == x && s_surf[i].y == y && s_surf[i].rw == w && s_surf[i].h == h)
            return;
    flush();
    for (int i = s_nsurf - 1; i >= 0; i--) {
        const Target *t = &s_surf[i];
        if (t->rx < x + w && x < t->rx + t->rw && t->y < y + h && y < t->y + t->h)
            surface_retire(i);
    }
    surface_add(x, y, w, h);
}

int PC_GlWideClip(int clip[4])
{
    for (int i = 0; i < s_nsurf; i++) {
        const Target *t = &s_surf[i];
        if (t->w != t->rw && clip[0] == t->rx && clip[2] == t->rx + t->rw - 1 && clip[1] == t->y &&
            clip[3] == t->y + t->h - 1) {
            clip[0] = t->x;
            clip[2] = t->x + t->w - 1;
            return i + 1;
        }
    }
    return 0;
}

void PC_GlSetWidescreen(int on)
{
    s_widescreen = on;
}

/* Widescreen margins for a window: gameplay's 512x240 (shown 4:3) widened to the window's shape,
 * when that is wider than 4:3. Only with the native 3D renderer, which draws the margins. */
static int wide_pad(int ww, int wh)
{
    if (!s_widescreen || !R3d_Native() || ww * 3 <= wh * 4)
        return 0;
    int pad = (int)((512.0 * ww / wh * 0.75 - 512.0) / 2 + 0.5);
    return pad > 400 ? 400 : pad;   /* GPU coordinates end at -1024 and 1023 */
}

/* The window's size (or the widescreen setting) changed: make the surfaces again to suit. */
static void set_output_size(int w, int h)
{
    int pad = wide_pad(w, h);
    if (w < 1 || h < 1 || (w == s_out_w && h == s_out_h && pad == s_pad))
        return;
    s_out_w = w;
    s_out_h = h;
    if (s_fixed_scale && pad == s_pad)
        return;
    s_pad = pad;
    pc_wide_pad = pad;
    flush();
    for (int n = s_nsurf; n > 0; n--) {
        Target t = s_surf[0];
        surface_retire(0);
        surface_add(t.x, t.y, t.w, t.h);
    }
}

/* ---------------------------------------------------------------------------------------------
 * Presentation */

void PC_GlPresent(void *sdl_window, int x, int y, int w, int h, int enabled,
                  const uint32_t *image, int iw, int ih)
{
    SDL_Window *win = (SDL_Window *)sdl_window;
    if (!s_active || !win)
        return;
    flush();

    int ww, wh, pic[4];
    SDL_GL_GetDrawableSize(win, &ww, &wh);
    set_output_size(ww, wh);
    picture_rect(1, 1, pic);   /* 4:3 (movies) */
    prepare();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, ww, wh);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    if (image || (enabled && w > 0 && h > 0)) {
        float u0, v0, u1, v1;
        glActiveTexture(GL_TEXTURE0);
        if (image) {
            /* a picture of its own (movies, pc_screen.h) */
            if (iw > VRAM_W)
                iw = VRAM_W;
            if (ih > VRAM_H)
                ih = VRAM_H;
            glBindTexture(GL_TEXTURE_2D, s_disp_tex);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, iw, ih, GL_BGRA, GL_UNSIGNED_BYTE, image);
            u0 = 0;
            v0 = 0;
            u1 = (float)iw / VRAM_W;
            v1 = (float)ih / VRAM_H;
        } else {
            int rw = w;
            const Target *t = shown_area(&x, &w, y, h);
            int px0, px1, py0, py1;
            picture_rect(w, rw, pic);
            span(t->sx, t->x, x, x + w - 1, &px0, &px1);
            span(t->sy, t->y, y, y + h - 1, &py0, &py1);
            /* a surface the window's size is copied pixel for pixel */
            int exact = px1 - px0 == pic[2] && py1 - py0 == pic[3];
            int smooth = s_filter || (t != &s_base && !s_fixed_scale);
            GLint filter = !exact && smooth ? GL_LINEAR : GL_NEAREST;
            glBindTexture(GL_TEXTURE_2D, t->tex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
            u0 = (float)px0 / (float)t->pw;
            v0 = (float)py0 / (float)t->ph;
            u1 = (float)px1 / (float)t->pw;
            v1 = (float)py1 / (float)t->ph;
        }
        /* 4:3, letterboxed; VRAM row 0 is the top of the picture */
        glViewport(pic[0], wh - pic[1] - pic[3], pic[2], pic[3]);
        float q[16] = { -1, 1, u0, v0, 1, 1, u1, v0, -1, -1, u0, v1, 1, -1, u1, v1 };
        glUseProgram(s_present_prog);
        glBindVertexArray(s_present_vao);
        glBindBuffer(GL_ARRAY_BUFFER, s_present_vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof q, q);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
    SDL_GL_SwapWindow(win);
    if (st.on && ++st.frames == 60) {
        fprintf(stderr, "[gl] last 60 frames: %d draw calls, %d readbacks, %d texture uploads, "
                        "%d blits\n",
                st.draws, st.readbacks, st.tex_ups, st.blits);
        memset(&st, 0, sizeof st);
        st.on = 1;
    }
}
