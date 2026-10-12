/* OpenGL 3.3 drawing backend for the PS1 GPU; the interface and the coherence model are described
 * in pc_gpu_gl.h.
 *
 * Primitives are batched into one triangle stream (lines become thin quads) and drawn with a
 * shader that does the PS1 texture lookup (4/8-bit CLUT or 15-bit direct, texture window),
 * modulation and the semi-transparency modes. Every vertex carries its own texture page, CLUT
 * and blend mode, so a batch only ends at a drawing-area change, at a B-F primitive (which needs
 * another blend equation) or when VRAM has to be synchronised. The colour target is RGBA8 with
 * the mask bit in alpha; no dithering, since nothing is reduced to 15 bits for display. */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glad/gl.h>
#define SDL_MAIN_HANDLED
#include <SDL.h>

#include "pc_gpu.h"
#include "pc_gpu_gl.h"
#include "pc_gte.h"
#include "pc_render3d.h"
#include "pc_gpu_gl_shaders.h"

#define VRAM_W 1024
#define VRAM_H 512
#define CELL 16
#define CELLS_X (VRAM_W / CELL)
#define CELLS_Y (VRAM_H / CELL)

/* cell states */
#define CPU_STALE   1   /* the render target has newer pixels than the CPU VRAM */
#define FBO_STALE   2   /* the CPU VRAM is newer than the render target */
#define ATLAS_STALE 4   /* the CPU VRAM is newer than the 16-bit texture atlas */

typedef PC_GlVert GVert;

/* GVert.info (PC_GL_I_* in pc_gpu_gl.h) */
#define I_TEXTURED 0x200
#define I_RAW      0x400
#define I_SEMI     0x800
#define I_MASK     0x1000
#define I_DEPTH    PC_GL_I_DEPTH
#define I_VIEW     PC_GL_I_VIEW
#define I_SKY      PC_GL_I_SKY

#define MAX_VERTS (64 * 1024)

static int s_active, s_scale = 1;
static GLuint s_draw_prog, s_copy_prog, s_present_prog;
static GLint s_draw_u_pass, s_draw_u_dpass, s_draw_u_proj, s_copy_u_scale;
static GLuint s_vao, s_vbo, s_copy_vao, s_copy_vbo, s_present_vao, s_present_vbo;
static GLuint s_atlas;                 /* R16UI 1024x512: textures and CLUTs */
static GLuint s_fbo, s_fbo_tex;        /* RGBA8 (1024*scale)x(512*scale): drawing */
static GLuint s_depth_rb;              /* DEPTH32F, same size: native 3D layers (pc_render3d.c) */
static GLuint s_rb_fbo, s_rb_tex;      /* RGBA8 1024x512: readback at scale > 1 */
static GLuint s_disp_tex;              /* RGBA8 1024x512: 24-bit display modes */
static int s_filter;
static int s_precise, s_perspective;   /* THPS_GL_PRECISE, THPS_GL_PERSPECTIVE (default on) */

static uint8_t s_cell[CELLS_Y][CELLS_X];
static int s_n_stale[8];               /* cells with each state bit set, by bit value */

/* THPS_GLSTATS=1: per-second counts of draw calls, readbacks and uploads */
static struct { int on, draws, readbacks, rb_cells, atlas_ups, fbo_ups, frames; } st;

static GVert s_verts[MAX_VERTS];
static int s_nv;
static int s_batch_sub;                /* the batch is B-F semi-transparent */
static int s_bclip[4] = { 0, 0, -1, -1 };
static int s_bbox[4];                  /* area the pending batch draws on (inclusive) */

/* A native 3D layer being drawn (PC_GlLayer*): its vertices, and the area it draws on. While it
 * is active, emulated primitives with a precise depth are depth-tested against it. */
static int s_layer;
static int s_lclip[4];
static GLuint s_vao3, s_vbo3;

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

static void mark_all(int bits);

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

int PC_GlInit(int scale)
{
    if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)) {
        fprintf(stderr, "[gl] cannot load the OpenGL functions\n");
        return -1;
    }
    GLint max_tex = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_tex);
    if (scale < 1)
        scale = 1;
    while (scale > 1 && VRAM_W * scale > max_tex)
        scale--;
    s_scale = scale;

    s_draw_prog = program(gl_draw_vs, gl_draw_fs);
    s_copy_prog = program(gl_copy_vs, gl_copy_fs);
    s_present_prog = program(gl_present_vs, gl_present_fs);
    if (!s_draw_prog || !s_copy_prog || !s_present_prog)
        return -1;
    s_draw_u_pass = glGetUniformLocation(s_draw_prog, "u_pass");
    s_draw_u_proj = glGetUniformLocation(s_draw_prog, "u_proj");
    s_draw_u_dpass = glGetUniformLocation(s_draw_prog, "u_dpass");
    s_copy_u_scale = glGetUniformLocation(s_copy_prog, "u_scale");
    glUseProgram(s_draw_prog);
    glUniform1i(glGetUniformLocation(s_draw_prog, "u_vram"), 0);
    glUniform1f(glGetUniformLocation(s_draw_prog, "u_shift"), s_scale == 1 ? 0.5f : 0.0f);
    glUniform1f(glGetUniformLocation(s_draw_prog, "u_uvfix"), s_scale == 1 ? 0.0f : 0.5f * s_scale);
    glUseProgram(s_copy_prog);
    glUniform1i(glGetUniformLocation(s_copy_prog, "u_vram"), 0);
    glUniform1f(s_copy_u_scale, (float)s_scale);
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
    s_atlas = texture(GL_R16UI, VRAM_W, VRAM_H, GL_RED_INTEGER, GL_UNSIGNED_SHORT, GL_NEAREST);
    s_fbo_tex = texture(GL_RGBA8, VRAM_W * s_scale, VRAM_H * s_scale, GL_RGBA, GL_UNSIGNED_BYTE,
                        s_filter ? GL_LINEAR : GL_NEAREST);
    s_rb_tex = texture(GL_RGBA8, VRAM_W, VRAM_H, GL_RGBA, GL_UNSIGNED_BYTE, GL_NEAREST);
    s_disp_tex = texture(GL_RGBA8, VRAM_W, VRAM_H, GL_RGBA, GL_UNSIGNED_BYTE,
                         s_filter ? GL_LINEAR : GL_NEAREST);
    s_fbo = framebuffer(s_fbo_tex);
    s_rb_fbo = framebuffer(s_rb_tex);
    if (s_fbo) {
        glGenRenderbuffers(1, &s_depth_rb);
        glBindRenderbuffer(GL_RENDERBUFFER, s_depth_rb);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, VRAM_W * s_scale,
                              VRAM_H * s_scale);
        glBindFramebuffer(GL_FRAMEBUFFER, s_fbo);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, s_depth_rb);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            s_fbo = 0;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    if (!s_fbo || !s_rb_fbo) {
        fprintf(stderr, "[gl] framebuffer setup failed\n");
        return -1;
    }
    if (glGetError() != GL_NO_ERROR) {
        fprintf(stderr, "[gl] setup failed\n");
        return -1;
    }

    /* everything comes from the CPU VRAM at first */
    memset(s_cell, 0, sizeof s_cell);
    memset(s_n_stale, 0, sizeof s_n_stale);
    mark_all(FBO_STALE | ATLAS_STALE);
    s_nv = 0;
    st.on = getenv("THPS_GLSTATS") && atoi(getenv("THPS_GLSTATS"));
    s_precise = !getenv("THPS_GL_PRECISE") || atoi(getenv("THPS_GL_PRECISE"));
    s_perspective = !getenv("THPS_GL_PERSPECTIVE") || atoi(getenv("THPS_GL_PERSPECTIVE"));
    PC_GtePreciseEnable(s_precise || s_perspective);
    s_active = 1;
    R3d_Init(1);
    fprintf(stderr, "[gl] OpenGL renderer: %s, %dx internal resolution\n",
            (const char *)glGetString(GL_RENDERER), s_scale);
    return 0;
}

void PC_GlShutdown(void)
{
    if (!s_active)
        return;
    PC_GtePreciseEnable(0);
    glDeleteProgram(s_draw_prog);
    glDeleteProgram(s_copy_prog);
    glDeleteProgram(s_present_prog);
    GLuint vaos[4] = { s_vao, s_copy_vao, s_present_vao, s_vao3 };
    GLuint bufs[4] = { s_vbo, s_copy_vbo, s_present_vbo, s_vbo3 };
    GLuint texs[4] = { s_atlas, s_fbo_tex, s_rb_tex, s_disp_tex };
    GLuint fbos[2] = { s_fbo, s_rb_fbo };
    glDeleteVertexArrays(4, vaos);
    glDeleteBuffers(4, bufs);
    glDeleteRenderbuffers(1, &s_depth_rb);
    glDeleteTextures(4, texs);
    glDeleteFramebuffers(2, fbos);
    s_active = 0;
}

int PC_GlActive(void)
{
    return s_active;
}

int PC_GlScale(void)
{
    return s_scale;
}

/* ---------------------------------------------------------------------------------------------
 * Cell bookkeeping */

static void cell_set(int cx, int cy, int set, int clear)
{
    uint8_t *c = &s_cell[cy][cx];
    uint8_t n = (uint8_t)((*c & ~clear) | set);
    for (int b = 1; b <= 4; b <<= 1)
        if ((n ^ *c) & b)
            s_n_stale[b] += (n & b) ? 1 : -1;
    *c = n;
}

static void mark_all(int bits)
{
    for (int cy = 0; cy < CELLS_Y; cy++)
        for (int cx = 0; cx < CELLS_X; cx++)
            cell_set(cx, cy, bits, 0);
}

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

static int any_cell(int x0, int y0, int x1, int y1, int bits)
{
    if (!s_n_stale[bits])
        return 0;
    for (int cy = y0 / CELL; cy <= y1 / CELL; cy++)
        for (int cx = x0 / CELL; cx <= x1 / CELL; cx++)
            if (s_cell[cy][cx] & bits)
                return 1;
    return 0;
}

static int overlaps(const int *a, int x0, int y0, int x1, int y1)
{
    return a[0] <= x1 && x0 <= a[2] && a[1] <= y1 && y0 <= a[3];
}

/* ---------------------------------------------------------------------------------------------
 * Uploads (CPU VRAM -> atlas -> render target) */

static void upload_atlas(void)
{
    if (!s_n_stale[ATLAS_STALE])
        return;
    glBindTexture(GL_TEXTURE_2D, s_atlas);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, VRAM_W);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
    const uint16_t *vram = PC_GpuVram();
    for (int cy = 0; cy < CELLS_Y; cy++) {
        for (int cx = 0; cx < CELLS_X;) {
            if (!(s_cell[cy][cx] & ATLAS_STALE)) {
                cx++;
                continue;
            }
            int run = cx;
            while (run < CELLS_X && (s_cell[cy][run] & ATLAS_STALE))
                cell_set(run++, cy, 0, ATLAS_STALE);
            int x = cx * CELL, y = cy * CELL;
            st.atlas_ups++;
            glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, (run - cx) * CELL, CELL, GL_RED_INTEGER,
                            GL_UNSIGNED_SHORT, vram + y * VRAM_W + x);
            cx = run;
        }
    }
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
}

static void upload_fbo(void)
{
    static float quads[CELLS_X * CELLS_Y * 12];
    int n = 0;
    if (!s_n_stale[FBO_STALE])
        return;
    upload_atlas();
    for (int cy = 0; cy < CELLS_Y; cy++) {
        for (int cx = 0; cx < CELLS_X;) {
            if (!(s_cell[cy][cx] & FBO_STALE)) {
                cx++;
                continue;
            }
            int run = cx;
            while (run < CELLS_X && (s_cell[cy][run] & FBO_STALE))
                cell_set(run++, cy, 0, FBO_STALE);
            float x0 = (float)(cx * CELL), x1 = (float)(run * CELL);
            float y0 = (float)(cy * CELL), y1 = (float)(cy * CELL + CELL);
            float q[12] = { x0, y0, x1, y0, x0, y1, x1, y0, x1, y1, x0, y1 };
            memcpy(&quads[n * 2], q, sizeof q);
            n += 6;
            st.fbo_ups++;
            cx = run;
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, s_fbo);
    glViewport(0, 0, VRAM_W * s_scale, VRAM_H * s_scale);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glUseProgram(s_copy_prog);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s_atlas);
    glBindVertexArray(s_copy_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_copy_vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)n * 2 * sizeof(float), quads, GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, n);
}

/* Make the render target and the atlas reflect every CPU write so far. */
static void prepare(void)
{
    upload_fbo();
    upload_atlas();
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
    glBindFramebuffer(GL_FRAMEBUFFER, s_fbo);
    glViewport(0, 0, VRAM_W * s_scale, VRAM_H * s_scale);
    glEnable(GL_SCISSOR_TEST);
    glScissor(s_bclip[0] * s_scale, s_bclip[1] * s_scale, (s_bclip[2] - s_bclip[0] + 1) * s_scale,
              (s_bclip[3] - s_bclip[1] + 1) * s_scale);
    glUseProgram(s_draw_prog);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s_atlas);
    glBindVertexArray(s_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof s_verts, NULL, GL_STREAM_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)s_nv * sizeof(GVert), s_verts);
    if (s_layer) {
        /* inside a native 3D layer: test (don't write) the depth of GTE-projected primitives */
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);
    }
    if (s_batch_sub) {
        set_blend(1);
        glDrawArrays(GL_TRIANGLES, 0, s_nv);
        set_blend(2);
    } else {
        set_blend(0);
    }
    glDrawArrays(GL_TRIANGLES, 0, s_nv);
    st.draws++;
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);

    /* the drawn area is newer here than in the CPU VRAM */
    int x0 = s_bbox[0] < s_bclip[0] ? s_bclip[0] : s_bbox[0];
    int y0 = s_bbox[1] < s_bclip[1] ? s_bclip[1] : s_bbox[1];
    int x1 = s_bbox[2] > s_bclip[2] ? s_bclip[2] : s_bbox[2];
    int y1 = s_bbox[3] > s_bclip[3] ? s_bclip[3] : s_bbox[3];
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > VRAM_W - 1) x1 = VRAM_W - 1;
    if (y1 > VRAM_H - 1) y1 = VRAM_H - 1;
    for (int cy = y0 / CELL; x0 <= x1 && cy <= y1 / CELL; cy++)
        for (int cx = x0 / CELL; cx <= x1 / CELL; cx++)
            cell_set(cx, cy, CPU_STALE, 0);
    s_nv = 0;
}

static void readback(int x0, int y0, int x1, int y1);

/* Footprint of a textured primitive's texel reads: its texture page area and CLUT row. Pixels
 * drawn since the last readback there (also by the pending batch) are read back first. */
static void sync_texture(const PC_GlPrim *p, int umin, int vmin, int umax, int vmax)
{
    int rects[2][4], n = 0;
    if (p->tw_and_x != 0xff || p->tw_and_y != 0xff || umin < 0 || vmin < 0 || umax > 255 ||
        vmax > 255) {
        umin = vmin = 0;
        umax = vmax = 255;
    }
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
        if (any_cell(r[0], r[1], r[2], r[3], CPU_STALE)) {
            flush();
            readback(r[0], r[1], r[2], r[3]);
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
    if (s_nv && (sub != s_batch_sub || s_nv + nverts > MAX_VERTS ||
                 s_bclip[0] != p->clip_l || s_bclip[1] != p->clip_t ||
                 s_bclip[2] != p->clip_r || s_bclip[3] != p->clip_b))
        flush();
    if (!s_nv) {
        s_batch_sub = sub;
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
                               (p->abr & 3) << 5 | (p->depth > 2 ? 2 : p->depth) << 7);
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

void PC_GlTriangle(const PC_GlPrim *p, const PC_GlVertex *a, const PC_GlVertex *b,
                   const PC_GlVertex *c)
{
    if (p->textured)
        sync_texture(p, MIN3(a->u, b->u, c->u), MIN3(a->v, b->v, c->v), MAX3(a->u, b->u, c->u),
                     MAX3(a->v, b->v, c->v));
    int precise = a->w > 0 && b->w > 0 && c->w > 0;
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
}

void PC_GlRect(const PC_GlPrim *p, int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b,
               int u0, int v0, int flip_x, int flip_y)
{
    if (w <= 0 || h <= 0)
        return;
    /* the texel at pixel x+i is u0+i, or u0-i when flipped (the shader rounds, see there) */
    int ul = u0, ur = flip_x ? u0 - w : u0 + w;
    int vt = v0, vb = flip_y ? v0 - h : v0 + h;
    if (p->textured) {
        int umin = flip_x ? u0 - w + 1 : u0, vmin = flip_y ? v0 - h + 1 : v0;
        sync_texture(p, umin & 255, vmin & 255, (umin & 255) + w - 1, (vmin & 255) + h - 1);
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
    glScissor(x0 * s_scale, y0 * s_scale, (x1 - x0 + 1) * s_scale, (y1 - y0 + 1) * s_scale);
    glClear(GL_COLOR_BUFFER_BIT);
    for (int cy = y0 / CELL; cy <= y1 / CELL; cy++)
        for (int cx = x0 / CELL; cx <= x1 / CELL; cx++)
            cell_set(cx, cy, CPU_STALE, 0);
}

void PC_GlFill(int x, int y, int w, int h, uint32_t rgb24)
{
    flush();
    prepare();
    glBindFramebuffer(GL_FRAMEBUFFER, s_fbo);
    glEnable(GL_SCISSOR_TEST);
    /* the fill colour is reduced to 15 bits, like everything else in VRAM */
    glClearColor((float)((rgb24 >> 3) & 31) / 31.0f, (float)((rgb24 >> 11) & 31) / 31.0f,
                 (float)((rgb24 >> 19) & 31) / 31.0f, 0.0f);
    each_part(x, y, w, h, fill_part);
    glDisable(GL_SCISSOR_TEST);
}

/* ---------------------------------------------------------------------------------------------
 * Native 3D layers */

static PC_GlLayerParams s_lp;

void PC_GlLayerSyncTexture(uint16_t tpage, uint16_t clut)
{
    PC_GlPrim p;
    memset(&p, 0, sizeof p);
    p.textured = 1;
    p.depth = (tpage >> 7) & 3;
    p.tp_x = (tpage & 15) * 64;
    p.tp_y = ((tpage >> 4) & 1) * 256;
    p.clut_x = (clut & 63) * 16;
    p.clut_y = (clut >> 6) & 511;
    p.tw_and_x = p.tw_and_y = 0xff;
    sync_texture(&p, 0, 0, 255, 255);
}

static void layer_scissor(void)
{
    glEnable(GL_SCISSOR_TEST);
    glScissor(s_lclip[0] * s_scale, s_lclip[1] * s_scale, (s_lclip[2] - s_lclip[0] + 1) * s_scale,
              (s_lclip[3] - s_lclip[1] + 1) * s_scale);
}

void PC_GlLayerBegin(const PC_GlLayerParams *lp, const PC_GlVert *v, int nv)
{
    flush();
    prepare();
    s_lp = *lp;
    s_lclip[0] = lp->clip_l < 0 ? 0 : lp->clip_l;
    s_lclip[1] = lp->clip_t < 0 ? 0 : lp->clip_t;
    s_lclip[2] = lp->clip_r > VRAM_W - 1 ? VRAM_W - 1 : lp->clip_r;
    s_lclip[3] = lp->clip_b > VRAM_H - 1 ? VRAM_H - 1 : lp->clip_b;
    if (s_lclip[2] < s_lclip[0] || s_lclip[3] < s_lclip[1])
        return;
    glBindVertexArray(s_vao3);
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo3);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)nv * sizeof(GVert), v, GL_STREAM_DRAW);
    glBindFramebuffer(GL_FRAMEBUFFER, s_fbo);
    layer_scissor();
    glDepthMask(GL_TRUE);
    glClearDepth(1.0);
    glClear(GL_DEPTH_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
    s_layer = 1;
}

void PC_GlLayerDraw(int first, int count, int cls)
{
    if (!s_layer || count <= 0)
        return;
    flush();        /* emulated primitives submitted before these are behind them */
    prepare();
    glBindFramebuffer(GL_FRAMEBUFFER, s_fbo);
    glViewport(0, 0, VRAM_W * s_scale, VRAM_H * s_scale);
    layer_scissor();
    glUseProgram(s_draw_prog);
    glUniform4f(s_draw_u_proj, s_lp.ofs_x + s_lp.ofx, s_lp.ofs_y + s_lp.ofy, s_lp.h, s_lp.hither);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s_atlas);
    glBindVertexArray(s_vao3);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glEnable(GL_CLIP_DISTANCE0);
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
    for (int cy = s_lclip[1] / CELL; cy <= s_lclip[3] / CELL; cy++)
        for (int cx = s_lclip[0] / CELL; cx <= s_lclip[2] / CELL; cx++)
            cell_set(cx, cy, CPU_STALE, 0);
}

/* ---------------------------------------------------------------------------------------------
 * Coherence */

/* 8-bit channel -> 5 bits: exact for every value a 5-bit one was stored as (n * 255 / 31,
 * rounded), and the PS1's truncation for the in-between values drawing produces. */
static uint8_t s_to5[256];

/* Copy the CPU_STALE cells of an inclusive area back into the CPU VRAM. */
static void readback(int x0, int y0, int x1, int y1)
{
    static uint8_t buf[VRAM_W * VRAM_H * 4];
    if (!s_to5[255])
        for (int i = 0; i < 256; i++)
            s_to5[i] = (uint8_t)((i * 31 + 16) / 255);
    int cx0 = CELLS_X, cy0 = CELLS_Y, cx1 = -1, cy1 = -1;
    for (int cy = y0 / CELL; cy <= y1 / CELL; cy++)
        for (int cx = x0 / CELL; cx <= x1 / CELL; cx++)
            if (s_cell[cy][cx] & CPU_STALE) {
                if (cx < cx0) cx0 = cx;
                if (cy < cy0) cy0 = cy;
                if (cx > cx1) cx1 = cx;
                if (cy > cy1) cy1 = cy;
            }
    if (cx1 < 0)
        return;
    flush();
    int px = cx0 * CELL, py = cy0 * CELL, pw = (cx1 - cx0 + 1) * CELL, ph = (cy1 - cy0 + 1) * CELL;
    if (s_scale > 1) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, s_fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, s_rb_fbo);
        glBlitFramebuffer(px * s_scale, py * s_scale, (px + pw) * s_scale, (py + ph) * s_scale,
                          px, py, px + pw, py + ph, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, s_rb_fbo);
    } else {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, s_fbo);
    }
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glReadPixels(px, py, pw, ph, GL_RGBA, GL_UNSIGNED_BYTE, buf);
    st.readbacks++;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    uint16_t *vram = PC_GpuVram();
    for (int cy = cy0; cy <= cy1; cy++)
        for (int cx = cx0; cx <= cx1; cx++) {
            if (!(s_cell[cy][cx] & CPU_STALE))
                continue;
            for (int j = 0; j < CELL; j++) {
                int y = cy * CELL + j;
                const uint8_t *s = buf + ((size_t)(y - py) * pw + (cx * CELL - px)) * 4;
                uint16_t *d = vram + y * VRAM_W + cx * CELL;
                for (int i = 0; i < CELL; i++, s += 4)
                    d[i] = (uint16_t)(s_to5[s[0]] | s_to5[s[1]] << 5 | s_to5[s[2]] << 10 |
                                      (s[3] >= 128 ? 0x8000 : 0));
            }
            cell_set(cx, cy, ATLAS_STALE, CPU_STALE);
            st.rb_cells++;
        }
}

static void sync_read_part(int x0, int y0, int x1, int y1)
{
    if (s_nv && overlaps(s_bbox, x0, y0, x1, y1))
        flush();
    if (any_cell(x0, y0, x1, y1, CPU_STALE))
        readback(x0, y0, x1, y1);
}

void PC_GlSyncRead(int x, int y, int w, int h)
{
    if (s_active)
        each_part(x, y, w, h, sync_read_part);
}

/* Cells the write covers only partly keep their other pixels: read those back first. */
static void before_write_part(int x0, int y0, int x1, int y1)
{
    int ix0 = (x0 + CELL - 1) / CELL, iy0 = (y0 + CELL - 1) / CELL;   /* fully covered cells */
    int ix1 = (x1 + 1) / CELL - 1, iy1 = (y1 + 1) / CELL - 1;
    for (int cy = y0 / CELL; cy <= y1 / CELL; cy++)
        for (int cx = x0 / CELL; cx <= x1 / CELL; cx++)
            if ((s_cell[cy][cx] & CPU_STALE) && (cx < ix0 || cx > ix1 || cy < iy0 || cy > iy1))
                readback(cx * CELL, cy * CELL, cx * CELL + CELL - 1, cy * CELL + CELL - 1);
}

void PC_GlBeforeWrite(int x, int y, int w, int h)
{
    if (!s_active)
        return;
    flush();   /* primitives already sent draw before the CPU's pixels arrive */
    each_part(x, y, w, h, before_write_part);
}

static void written_part(int x0, int y0, int x1, int y1)
{
    for (int cy = y0 / CELL; cy <= y1 / CELL; cy++)
        for (int cx = x0 / CELL; cx <= x1 / CELL; cx++)
            cell_set(cx, cy, FBO_STALE | ATLAS_STALE, CPU_STALE);
}

void PC_GlWritten(int x, int y, int w, int h)
{
    if (!s_active)
        return;
    flush();
    each_part(x, y, w, h, written_part);
}

uint32_t *PC_GlReadArea(int x, int y, int w, int h)
{
    if (!s_active || w <= 0 || h <= 0 || x < 0 || y < 0 || x + w > VRAM_W || y + h > VRAM_H)
        return NULL;
    int sw = w * s_scale, sh = h * s_scale;
    uint32_t *out = (uint32_t *)malloc((size_t)sw * sh * 4);
    if (!out)
        return NULL;
    flush();
    prepare();
    glBindFramebuffer(GL_READ_FRAMEBUFFER, s_fbo);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glReadPixels(x * s_scale, y * s_scale, sw, sh, GL_BGRA, GL_UNSIGNED_BYTE, out);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    for (int i = 0; i < sw * sh; i++)
        out[i] |= 0xff000000u;
    return out;
}

/* ---------------------------------------------------------------------------------------------
 * Presentation */

void PC_GlPresent(void *sdl_window, int x, int y, int w, int h, int rgb24, int enabled)
{
    SDL_Window *win = (SDL_Window *)sdl_window;
    static uint32_t argb[VRAM_W * VRAM_H];
    if (!s_active || !win)
        return;
    flush();
    prepare();

    int ww, wh;
    SDL_GL_GetDrawableSize(win, &ww, &wh);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, ww, wh);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    if (enabled && w > 0 && h > 0) {
        float u0, v0, u1, v1;
        glActiveTexture(GL_TEXTURE0);
        if (rgb24) {
            /* 24-bit pixels (movies) are not drawn by the GPU: convert them on the CPU
             * (PC_GpuDisplayToARGB reads drawn pixels back first) */
            int dw, dh;
            PC_GpuDisplayToARGB(argb, VRAM_W, VRAM_W, VRAM_H, &dw, &dh);
            glBindTexture(GL_TEXTURE_2D, s_disp_tex);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, VRAM_W);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, dw, dh, GL_BGRA, GL_UNSIGNED_BYTE, argb);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
            u0 = 0;
            v0 = 0;
            u1 = (float)dw / VRAM_W;
            v1 = (float)dh / VRAM_H;
        } else {
            glBindTexture(GL_TEXTURE_2D, s_fbo_tex);
            u0 = (float)x / VRAM_W;
            v0 = (float)y / VRAM_H;
            u1 = (float)(x + w) / VRAM_W;
            v1 = (float)(y + h) / VRAM_H;
        }
        /* 4:3, letterboxed; VRAM row 0 is the top of the picture */
        int dw = ww, dh = wh;
        if (ww * 3 > wh * 4)
            dw = wh * 4 / 3;
        else
            dh = ww * 3 / 4;
        glViewport((ww - dw) / 2, (wh - dh) / 2, dw, dh);
        float q[16] = { -1, 1, u0, v0, 1, 1, u1, v0, -1, -1, u0, v1, 1, -1, u1, v1 };
        glUseProgram(s_present_prog);
        glBindVertexArray(s_present_vao);
        glBindBuffer(GL_ARRAY_BUFFER, s_present_vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof q, q);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
    SDL_GL_SwapWindow(win);
    if (st.on && ++st.frames == 60) {
        fprintf(stderr, "[gl] last 60 frames: %d draw calls, %d readbacks (%d cells), "
                        "%d atlas and %d render-target uploads\n",
                st.draws, st.readbacks, st.rb_cells, st.atlas_ups, st.fbo_ups);
        memset(&st, 0, sizeof st);
        st.on = 1;
    }
}
