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

#include <vector>

#include <glad/gl.h>
#define SDL_MAIN_HANDLED
#include <SDL.h>

#include "pc_dctex.h"
#include "pc_render3d.h"
#include "pc_runtime.h"
#include "pc_texstore.h"
#include "pc/gl_shaders.h"
#include "pc/gl_gpu.h"
#include "pc/texture_store.h"
#include "pc/dc_textures.h"
#include "pc/precise.h"
#include "pc/render3d.h"

int pc_wide_pad;   /* GlGpu::m_pad, for the game's view frustum (pc/gl_gpu.h) */

namespace pc {

GlGpu g_glGpu;

namespace {

typedef GlVert GVert;
typedef RenderTarget Target;

constexpr int VRAM_W = GlGpu::VRAM_W;
constexpr int VRAM_H = GlGpu::VRAM_H;
constexpr int CELL = GlGpu::CELL;
constexpr int CELLS_X = GlGpu::CELLS_X;
constexpr int CELLS_Y = GlGpu::CELLS_Y;
constexpr int MAX_SURFACES = GlGpu::MAX_SURFACES;
constexpr int MAX_HD = GlGpu::MAX_HD;
constexpr int MAX_VERTS = GlGpu::MAX_VERTS;

/* GVert.info (PC_GL_I_* in pc_gpu_gl.h) */
constexpr uint16_t I_TEXTURED = 0x200;
constexpr uint16_t I_RAW = 0x400;
constexpr uint16_t I_SEMI = 0x800;
constexpr uint16_t I_MASK = 0x1000;
constexpr uint16_t I_DEPTH = PC_GL_I_DEPTH;
[[maybe_unused]] constexpr uint16_t I_VIEW = PC_GL_I_VIEW;
[[maybe_unused]] constexpr uint16_t I_SKY = PC_GL_I_SKY;
[[maybe_unused]] constexpr uint16_t I_HD = 0x0180;   /* colour depth 3: an HD texture (pc_dctex.h), clut = its handle */

/* HiResImage's texture has the last handle (pc_dctex.cpp leaves it free). */
constexpr int HIRES_SLOT = MAX_HD - 1;

#define MIN3(a, b, c) ((a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))
#define MAX3(a, b, c) ((a) > (b) ? ((a) > (c) ? (a) : (c)) : ((b) > (c) ? (b) : (c)))

/* ---------------------------------------------------------------------------------------------
 * GL objects */

GLuint Compile(GLenum type, const char *src)
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

GLuint Program(const char *vs, const char *fs)
{
    GLuint p = glCreateProgram(), v = Compile(GL_VERTEX_SHADER, vs),
           f = Compile(GL_FRAGMENT_SHADER, fs);
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

GLuint Texture(GLint ifmt, int w, int h, GLenum fmt, GLenum type, GLint filter)
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

GLuint Framebuffer(GLuint tex)
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

/* The pixels of a target that VRAM columns (or rows) a..b cover: those whose centres lie inside,
 * as GL rasterises an edge at a * scale. p1 is exclusive. */
void Span(float s, int origin, int a, int b, int *p0, int *p1)
{
    *p0 = (int)ceilf((float)(a - origin) * s - 0.5f);
    *p1 = (int)ceilf((float)(b + 1 - origin) * s - 0.5f);
}

/* The GVert layout, for the bound VAO and array buffer. */
void VertAttribs()
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

/* Call fn(x0, y0, x1, y1) (inclusive) for each in-VRAM part of a rectangle that may wrap at the
 * right and bottom edges. */
template <typename Fn>
void EachPart(int x, int y, int w, int h, Fn fn)
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

bool Overlaps(const int *a, int x0, int y0, int x1, int y1)
{
    return a[0] <= x1 && x0 <= a[2] && a[1] <= y1 && y0 <= a[3];
}

uint16_t PrimInfo(const GlPrim *p)
{
    uint16_t info = (uint16_t)(((p->tp_x / 64) & 15) | ((p->tp_y / 256) & 1) << 4 |
                               (p->abr & 3) << 5 | (p->depth & 3) << 7);
    if (p->textured) info |= I_TEXTURED;
    if (p->raw) info |= I_RAW;
    if (p->semi) info |= I_SEMI;
    if (p->mask_set) info |= I_MASK;
    return info;
}

void Put(GVert *o, const GlPrim *p, uint16_t info, float x, float y, uint8_t r, uint8_t g,
         uint8_t b, int u, int v)
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

/* The texels the PS1 samples on a triangle with whole-pixel corners: its texture coordinates at
 * the first and last pixel corners it covers (x0..x1-1, y0..y1-1), rounded, and never past its
 * vertices'. Scaled up, pixel centres also land between the PS1's last corner and the edge, which
 * would round onto the texel beyond it: 2D quads give the exclusive u0+w, v0+h there (a font's
 * neighbouring glyph showed as a line beside scaled text). The half of a rectangle's quad is
 * measured over the whole rectangle, its other half has the same gradients. */
void UvRange(GVert *o, const GlVertex *a, const GlVertex *b, const GlVertex *c,
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

/* Clip an area to VRAM; false if nothing is left. */
bool ClipArea(int *x, int *y, int *w, int *h)
{
    if (*x < 0) { *w += *x; *x = 0; }
    if (*y < 0) { *h += *y; *y = 0; }
    if (*x + *w > VRAM_W) *w = VRAM_W - *x;
    if (*y + *h > VRAM_H) *h = VRAM_H - *y;
    return *w > 0 && *h > 0;
}

/* Copy a target's pixels of the VRAM area x, y, w, h into another target (nearest). */
void CopyArea(const Target *from, const Target *to, int x, int y, int w, int h)
{
    int fx0, fx1, fy0, fy1, tx0, tx1, ty0, ty1;
    Span(from->sx, from->x, x, x + w - 1, &fx0, &fx1);
    Span(from->sy, from->y, y, y + h - 1, &fy0, &fy1);
    Span(to->sx, to->x, x, x + w - 1, &tx0, &tx1);
    Span(to->sy, to->y, y, y + h - 1, &ty0, &ty1);
    glDisable(GL_SCISSOR_TEST);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, from->fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, to->fbo);
    glBlitFramebuffer(fx0, fy0, fx1, fy1, tx0, ty0, tx1, ty1, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

}   // namespace

/* ---------------------------------------------------------------------------------------------
 * Render targets */

void RenderTarget::Destroy()
{
    glDeleteFramebuffers(1, &fbo);
    glDeleteRenderbuffers(1, &depth);
    glDeleteTextures(1, &tex);
    *this = RenderTarget{};
}

bool RenderTarget::Create(int x_, int y_, int w_, int h_, int pad, int pw_, int ph_, GLint max_tex)
{
    *this = RenderTarget{};
    if (pw_ < 1 || ph_ < 1 || pw_ > max_tex || ph_ > max_tex)
        return false;
    rx = x_;
    rw = w_;
    x = x_ - pad;
    y = y_;
    w = w_ + 2 * pad;
    h = h_;
    pw = pw_;
    ph = ph_;
    sx = (float)pw_ / (float)w;
    sy = (float)ph_ / (float)h_;
    tex = Texture(GL_RGBA8, pw_, ph_, GL_RGBA, GL_UNSIGNED_BYTE, GL_NEAREST);
    fbo = Framebuffer(tex);
    glGenRenderbuffers(1, &depth);
    glBindRenderbuffer(GL_RENDERBUFFER, depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, pw_, ph_);
    if (fbo) {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            Destroy();
            return false;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    if (!fbo || glGetError() != GL_NO_ERROR) {
        Destroy();
        return false;
    }
    return true;
}

void RenderTarget::Use() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, pw, ph);
}

bool RenderTarget::Scissor(int x0, int y0, int x1, int y1, int wide) const
{
    int px0, px1, py0, py1;
    int l = wide ? x : rx, r = wide ? x + w - 1 : rx + rw - 1;
    if (x0 < l) x0 = l;
    if (y0 < y) y0 = y;
    if (x1 > r) x1 = r;
    if (y1 > y + h - 1) y1 = y + h - 1;
    if (x1 < x0 || y1 < y0)
        return false;
    Span(sx, x, x0, x1, &px0, &px1);
    Span(sy, y, y0, y1, &py0, &py1);
    glEnable(GL_SCISSOR_TEST);
    glScissor(px0, py0, px1 - px0, py1 - py0);
    return px1 > px0 && py1 > py0;
}

/* The draw program's uniforms for a target. At the PS1's resolution the geometry moves by half a
 * pixel, scaled up only texture coordinates do (see pc/gl_shaders.h). At a fractional scale
 * a pixel centre can lie exactly on a primitive's top or left edge, where the corner is exactly
 * a texel boundary: stopping 1/64 pixel short keeps it on the primitive's own texel. */
void GlGpu::DrawUniforms(const Target *t)
{
    int one = t->sx == 1.0f && t->sy == 1.0f;
    float ex = t->sx == floorf(t->sx) ? 0.0f : 1.0f / 64, ey = t->sy == floorf(t->sy) ? 0.0f : 1.0f / 64;
    glUniform4f(m_drawUTarget, (float)t->x, (float)t->y, (float)t->w, (float)t->h);
    glUniform1f(m_drawUShift, one ? 0.5f : 0.0f);
    glUniform2f(m_drawUUvfix, one ? 0.0f : 0.5f * t->sx - ex, one ? 0.0f : 0.5f * t->sy - ey);
}

/* The targets the VRAM area x0,y0..x1,y1 (inclusive) is drawn on: the surfaces whose VRAM areas
 * it overlaps, and the base unless a single surface holds all of it. Returns how many (base
 * last). */
int GlGpu::TargetsFor(int x0, int y0, int x1, int y1, Target **out)
{
    int n = 0, inside = 0;
    for (int i = 0; i < m_nsurf; i++) {
        Target *t = &m_surf[i];
        if (t->rx > x1 || x0 >= t->rx + t->rw || t->y > y1 || y0 >= t->y + t->h)
            continue;
        out[n++] = t;
        if (x0 >= t->rx && x1 < t->rx + t->rw && y0 >= t->y && y1 < t->y + t->h)
            inside = 1;
    }
    if (!inside)
        out[n++] = &m_base;
    return n;
}

/* The targets of a drawing: a widened one (pc::GlPrim.wide = surface + 1) only its surface. */
int GlGpu::DrawTargets(int wide, int x0, int y0, int x1, int y1, Target **out)
{
    if (wide > 0 && wide <= m_nsurf) {
        out[0] = &m_surf[wide - 1];
        return 1;
    }
    return TargetsFor(x0, y0, x1, y1, out);
}

/* Mark the cells under a drawn area (inclusive, may reach outside VRAM) as drawn on: for a
 * widened drawing only its surface's VRAM area. */
void GlGpu::MarkDrawn(int wide, int x0, int y0, int x1, int y1)
{
    int l = 0, r = VRAM_W - 1;
    if (wide > 0 && wide <= m_nsurf) {
        l = m_surf[wide - 1].rx;
        r = l + m_surf[wide - 1].rw - 1;
    }
    if (x0 < l) x0 = l;
    if (x1 > r) x1 = r;
    if (y0 < 0) y0 = 0;
    if (y1 > VRAM_H - 1) y1 = VRAM_H - 1;
    for (int cy = y0 / CELL; x0 <= x1 && y0 <= y1 && cy <= y1 / CELL; cy++)
        for (int cx = x0 / CELL; cx <= x1 / CELL; cx++)
            m_drawn[cy][cx] = 1;
}

bool GlGpu::AnyDrawn(int x0, int y0, int x1, int y1) const
{
    for (int cy = y0 / CELL; cy <= y1 / CELL; cy++)
        for (int cx = x0 / CELL; cx <= x1 / CELL; cx++)
            if (m_drawn[cy][cx])
                return true;
    return false;
}

int GlGpu::Init(int fixed_scale, int out_w, int out_h)
{
    if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)) {
        fprintf(stderr, "[gl] cannot load the OpenGL functions\n");
        return -1;
    }
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &m_maxTex);
    m_fixedScale = fixed_scale > 0 ? fixed_scale : 0;
    m_outW = out_w > 0 ? out_w : 640;
    m_outH = out_h > 0 ? out_h : 480;
    m_nsurf = 0;

    m_drawProg = Program(gl_draw_vs, gl_draw_fs);
    m_copyProg = Program(gl_copy_vs, gl_copy_fs);
    m_presentProg = Program(gl_present_vs, gl_present_fs);
    if (!m_drawProg || !m_copyProg || !m_presentProg)
        return -1;
    m_drawUPass = glGetUniformLocation(m_drawProg, "u_pass");
    m_drawUProj = glGetUniformLocation(m_drawProg, "u_proj");
    m_drawUDpass = glGetUniformLocation(m_drawProg, "u_dpass");
    m_drawUTarget = glGetUniformLocation(m_drawProg, "u_target");
    m_drawUShift = glGetUniformLocation(m_drawProg, "u_shift");
    m_drawUUvfix = glGetUniformLocation(m_drawProg, "u_uvfix");
    m_copyUTarget = glGetUniformLocation(m_copyProg, "u_target");
    m_copyUMap = glGetUniformLocation(m_copyProg, "u_map");
    m_copyUSrc = glGetUniformLocation(m_copyProg, "u_src");
    glUseProgram(m_drawProg);
    glUniform1i(glGetUniformLocation(m_drawProg, "u_vram"), 0);
    glUniform1i(glGetUniformLocation(m_drawProg, "u_hd"), 1);
    glUseProgram(m_copyProg);
    glUniform1i(glGetUniformLocation(m_copyProg, "u_vram"), 0);
    glUseProgram(m_presentProg);
    glUniform1i(glGetUniformLocation(m_presentProg, "u_tex"), 0);
    glUseProgram(0);

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof m_batch.verts, NULL, GL_STREAM_DRAW);
    VertAttribs();
    glGenVertexArrays(1, &m_layer.vao);
    glGenBuffers(1, &m_layer.vbo);
    glBindVertexArray(m_layer.vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_layer.vbo);
    VertAttribs();

    glGenVertexArrays(1, &m_copyVao);
    glGenBuffers(1, &m_copyVbo);
    glBindVertexArray(m_copyVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_copyVbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);

    glGenVertexArrays(1, &m_presentVao);
    glGenBuffers(1, &m_presentVbo);
    glBindVertexArray(m_presentVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_presentVbo);
    glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(float), NULL, GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glBindVertexArray(0);

    const char *f = getenv("THPS_FILTER");
    m_filter = f && f[0] == '1';
    m_blit = Texture(GL_R16UI, VRAM_W, VRAM_H, GL_RED_INTEGER, GL_UNSIGNED_SHORT, GL_NEAREST);
    m_texstore = Texture(GL_R16UI, VRAM_W, VRAM_H, GL_RED_INTEGER, GL_UNSIGNED_SHORT, GL_NEAREST);
    m_rbTex = Texture(GL_RGBA8, VRAM_W, VRAM_H, GL_RGBA, GL_UNSIGNED_BYTE, GL_NEAREST);
    m_dispTex = Texture(GL_RGBA8, VRAM_W, VRAM_H, GL_RGBA, GL_UNSIGNED_BYTE,
                        m_filter ? GL_LINEAR : GL_NEAREST);
    m_rbFbo = Framebuffer(m_rbTex);
    if (!m_base.Create(0, 0, VRAM_W, VRAM_H, 0, VRAM_W, VRAM_H, m_maxTex) || !m_rbFbo) {
        fprintf(stderr, "[gl] framebuffer setup failed\n");
        return -1;
    }
    if (glGetError() != GL_NO_ERROR) {
        fprintf(stderr, "[gl] setup failed\n");
        return -1;
    }

    memset(m_drawn, 0, sizeof m_drawn);
    m_batch.nv = 0;
    m_st.on = getenv("THPS_GLSTATS") && atoi(getenv("THPS_GLSTATS"));
    m_precise = !getenv("THPS_GL_PRECISE") || atoi(getenv("THPS_GL_PRECISE"));
    m_perspective = !getenv("THPS_GL_PERSPECTIVE") || atoi(getenv("THPS_GL_PERSPECTIVE"));
    g_precise.Enable(m_precise || m_perspective);
    m_active = 1;
    g_renderer3d.Init(1);
    if (m_fixedScale)
        fprintf(stderr, "[gl] OpenGL renderer: %s, screens at %dx the PS1 resolution\n",
                (const char *)glGetString(GL_RENDERER), m_fixedScale);
    else
        fprintf(stderr, "[gl] OpenGL renderer: %s, screens at %dx%d\n",
                (const char *)glGetString(GL_RENDERER), m_outW, m_outH);
    return 0;
}

void GlGpu::Shutdown()
{
    if (!m_active)
        return;
    g_precise.Enable(0);
    glDeleteProgram(m_drawProg);
    glDeleteProgram(m_copyProg);
    glDeleteProgram(m_presentProg);
    GLuint vaos[4] = { m_vao, m_copyVao, m_presentVao, m_layer.vao };
    GLuint bufs[4] = { m_vbo, m_copyVbo, m_presentVbo, m_layer.vbo };
    GLuint texs[4] = { m_blit, m_texstore, m_rbTex, m_dispTex };
    glDeleteVertexArrays(4, vaos);
    glDeleteBuffers(4, bufs);
    glDeleteTextures(4, texs);
    glDeleteFramebuffers(1, &m_rbFbo);
    for (int i = 0; i < m_nsurf; i++)
        m_surf[i].Destroy();
    m_nsurf = 0;
    m_base.Destroy();
    m_active = 0;
}

/* ---------------------------------------------------------------------------------------------
 * Textures */

/* The texture store's changed cells to its texture. */
void GlGpu::UploadTexStore()
{
    const uint16_t *tex = g_textureStore.Data();
    uint8_t *cell = g_textureStore.Cells();
    int any = 0;
    for (int cy = 0; cy < CELLS_Y; cy++) {
        uint8_t *row = cell + cy * CELLS_X;
        for (int cx = 0; cx < CELLS_X;) {
            if (!(row[cx] & kTexDirty)) {
                cx++;
                continue;
            }
            int run = cx;
            while (run < CELLS_X && (row[run] & kTexDirty))
                row[run++] &= (uint8_t)~kTexDirty;
            if (!any) {
                any = 1;
                glBindTexture(GL_TEXTURE_2D, m_texstore);
                glPixelStorei(GL_UNPACK_ROW_LENGTH, VRAM_W);
                glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
            }
            int x = cx * CELL, y = cy * CELL;
            m_st.tex_ups++;
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

/* Make the textures reflect every write so far. */
void GlGpu::Prepare()
{
    UploadTexStore();
}

void GlGpu::TexturesChanging(int x, int y, int w, int h)
{
    if (!m_active)
        return;
    Flush();
    /* what was drawn there is replaced by texels */
    for (int cy = y / CELL; cy <= (y + h - 1) / CELL && cy < CELLS_Y; cy++)
        for (int cx = x / CELL; cx <= (x + w - 1) / CELL && cx < CELLS_X; cx++)
            m_drawn[cy][cx] = 0;
}

/* ---------------------------------------------------------------------------------------------
 * HD textures (pc_dctex.h) */

/* Bind an HD texture to unit 1 (made from its image the first time), then make unit 0 active. */
void GlGpu::HdBind(uint16_t h)
{
    glActiveTexture(GL_TEXTURE1);
    if (h < MAX_HD && !m_hd[h]) {
        int w = 1, ht = 1;
        uint8_t *rgba = NULL;
        static const uint8_t missing[4] = { 255, 0, 255, 255 };
        glGenTextures(1, &m_hd[h]);
        glBindTexture(GL_TEXTURE_2D, m_hd[h]);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        if (g_dcTextures.Decode(h, &w, &ht, &rgba)) {
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
        glBindTexture(GL_TEXTURE_2D, h < MAX_HD ? m_hd[h] : 0);
    }
    glActiveTexture(GL_TEXTURE0);
}

void GlGpu::HiResImage(int x, int y, int w, int h, const uint16_t *src, int sw, int sh)
{
    std::vector<uint8_t> rgba((size_t)sw * sh * 4);
    for (int i = 0; i < sw * sh; i++) {
        uint16_t p = src[i];
        rgba[i * 4] = (uint8_t)((p & 31) * 255 / 31);
        rgba[i * 4 + 1] = (uint8_t)((p >> 5 & 31) * 255 / 31);
        rgba[i * 4 + 2] = (uint8_t)((p >> 10 & 31) * 255 / 31);
        rgba[i * 4 + 3] = 255;   /* every pixel is written, as by LoadImage */
    }
    Flush();
    if (!m_hd[HIRES_SLOT])
        glGenTextures(1, &m_hd[HIRES_SLOT]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_hd[HIRES_SLOT]);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, sw, sh, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glActiveTexture(GL_TEXTURE0);

    /* a raw textured rectangle over the area, its texture spanning 0..256 */
    GlPrim p;
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
    GVert *o = BeginPrim(&p, 6, x, y, x + w - 1, y + h - 1);
    if (o) {
        uint16_t info = PrimInfo(&p);
        Put(&o[0], &p, info, (float)x, (float)y, 128, 128, 128, 0, 0);
        Put(&o[1], &p, info, (float)(x + w), (float)y, 128, 128, 128, 256, 0);
        Put(&o[2], &p, info, (float)x, (float)(y + h), 128, 128, 128, 0, 256);
        o[3] = o[1];
        Put(&o[4], &p, info, (float)(x + w), (float)(y + h), 128, 128, 128, 256, 256);
        o[5] = o[2];
    }
    Flush();
}

void GlGpu::HdForget(uint16_t h)
{
    if (!m_active || h >= MAX_HD || !m_hd[h])
        return;
    if (m_batch.nv && m_batch.hd == h)
        Flush();
    glDeleteTextures(1, &m_hd[h]);
    m_hd[h] = 0;
}

/* ---------------------------------------------------------------------------------------------
 * Batches */

void GlGpu::SetBlend(int pass)
{
    glEnable(GL_BLEND);
    if (pass == 2) {
        glBlendEquationSeparate(GL_FUNC_REVERSE_SUBTRACT, GL_FUNC_ADD);
        glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ZERO);
    } else {
        glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
        glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC1_ALPHA, GL_ONE, GL_ZERO);
    }
    glUniform1i(m_drawUPass, pass);
}

void GlGpu::Flush()
{
    Batch &b = m_batch;
    if (!b.nv)
        return;
    Prepare();

    /* the area drawn on */
    int x0 = b.bbox[0] < b.clip[0] ? b.clip[0] : b.bbox[0];
    int y0 = b.bbox[1] < b.clip[1] ? b.clip[1] : b.bbox[1];
    int x1 = b.bbox[2] > b.clip[2] ? b.clip[2] : b.bbox[2];
    int y1 = b.bbox[3] > b.clip[3] ? b.clip[3] : b.bbox[3];

    glUseProgram(m_drawProg);
    if (b.hd)
        HdBind(b.hd);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texstore);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof b.verts, NULL, GL_STREAM_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)b.nv * sizeof(GVert), b.verts);
    if (m_layer.active) {
        /* inside a native 3D layer: test (don't write) the depth of projected primitives */
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);
    }
    Target *ts[MAX_SURFACES + 1];
    int nt = x0 <= x1 && y0 <= y1 ? DrawTargets(b.wide, x0, y0, x1, y1, ts) : 0;
    for (int i = 0; i < nt; i++) {
        ts[i]->Use();
        if (!ts[i]->Scissor(b.clip[0], b.clip[1], b.clip[2], b.clip[3], b.wide))
            continue;
        DrawUniforms(ts[i]);
        if (b.sub) {
            SetBlend(1);
            glDrawArrays(GL_TRIANGLES, 0, b.nv);
            SetBlend(2);
        } else {
            SetBlend(0);
        }
        glDrawArrays(GL_TRIANGLES, 0, b.nv);
        m_st.draws++;
    }
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);

    /* drawn on (the render-to-texture check, CheckTexture) */
    MarkDrawn(b.wide, x0, y0, x1, y1);
    b.nv = 0;
}

/* Footprint of a textured primitive's texel reads: its texture page area and CLUT row. Textures
 * come from the texture store, which nothing draws into: drawing there would be the game using
 * what it drew as a texture, which THPS doesn't do (logged once with THPS_TRACE). */
void GlGpu::CheckTexture(const GlPrim *p, int umin, int vmin, int umax, int vmax)
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
        if (m_batch.nv && Overlaps(m_batch.bbox, r[0], r[1], r[2], r[3]))
            Flush();
        if (AnyDrawn(r[0], r[1], r[2], r[3])) {
            if (!m_warnedRtt) {
                m_warnedRtt = 1;
                PC_Trace("[gl] a texture at %d,%d-%d,%d was drawn into: render-to-texture?", r[0],
                         r[1], r[2], r[3]);
            }
        }
    }
}

/* Start or continue a batch for one primitive covering bx0..bx1, by0..by1; NULL if it is clipped
 * away entirely. */
GVert *GlGpu::BeginPrim(const GlPrim *p, int nverts, int bx0, int by0, int bx1, int by1)
{
    Batch &b = m_batch;
    if (p->clip_r < p->clip_l || p->clip_b < p->clip_t)
        return NULL;
    if (bx1 < p->clip_l || bx0 > p->clip_r || by1 < p->clip_t || by0 > p->clip_b)
        return NULL;
    if (bx0 < p->clip_l) bx0 = p->clip_l;
    if (by0 < p->clip_t) by0 = p->clip_t;
    if (bx1 > p->clip_r) bx1 = p->clip_r;
    if (by1 > p->clip_b) by1 = p->clip_b;
    if (p->mask_check && !m_warnedMask) {
        m_warnedMask = 1;
        fprintf(stderr, "[gl] mask-bit test is not implemented (drawing ignores it)\n");
    }
    int sub = p->semi && p->abr == 2;
    uint16_t hd = p->textured && p->depth == 3 ? (uint16_t)((p->clut_x / 16) | (p->clut_y << 6)) : 0;
    if (b.nv && (sub != b.sub || hd != b.hd || p->wide != b.wide ||
                 b.nv + nverts > MAX_VERTS ||
                 b.clip[0] != p->clip_l || b.clip[1] != p->clip_t ||
                 b.clip[2] != p->clip_r || b.clip[3] != p->clip_b))
        Flush();
    if (!b.nv) {
        b.sub = sub;
        b.hd = hd;
        b.wide = p->wide;
        b.clip[0] = p->clip_l;
        b.clip[1] = p->clip_t;
        b.clip[2] = p->clip_r;
        b.clip[3] = p->clip_b;
        b.bbox[0] = bx0;
        b.bbox[1] = by0;
        b.bbox[2] = bx1;
        b.bbox[3] = by1;
    } else {
        if (bx0 < b.bbox[0]) b.bbox[0] = bx0;
        if (by0 < b.bbox[1]) b.bbox[1] = by0;
        if (bx1 > b.bbox[2]) b.bbox[2] = bx1;
        if (by1 > b.bbox[3]) b.bbox[3] = by1;
    }
    GVert *v = &b.verts[b.nv];
    b.nv += nverts;
    return v;
}

void GlGpu::PutV(GVert *o, const GlPrim *p, uint16_t info, const GlVertex *s, int precise)
{
    if (!precise) {
        Put(o, p, info, (float)s->x, (float)s->y, s->r, s->g, s->b, s->u, s->v);
        return;
    }
    Put(o, p, info, m_precise ? s->px : (float)s->x, m_precise ? s->py : (float)s->y, s->r, s->g,
        s->b, s->u, s->v);
    if (m_perspective)
        o->z = s->w;
}

/* An effect made in screen space with view depths (PC_DRAW_DEPTH: zs, one per vertex): inside a
 * native 3D layer it is depth-tested there, as projected primitives are. The depth is also the w
 * of its corners, which leaves their positions as they are. */
void GlGpu::FlatDepth(GVert *o, int n, const float *zs)
{
    if (!m_layer.active)
        return;
    for (int i = 0; i < n; i++)
        if (zs[i] <= 0)
            return;
    for (int i = 0; i < n; i++) {
        o[i].z = zs[i];
        o[i].info |= I_DEPTH;
    }
}

/* ---------------------------------------------------------------------------------------------
 * Primitives */

void GlGpu::Triangle(const GlPrim *p, const GlVertex *a, const GlVertex *b,
                     const GlVertex *c)
{
    if (p->textured && p->depth != 3)
        CheckTexture(p, MIN3(a->u, b->u, c->u), MIN3(a->v, b->v, c->v), MAX3(a->u, b->u, c->u),
                     MAX3(a->v, b->v, c->v));
    int precise = a->w > 0 && b->w > 0 && c->w > 0;
    GlVertex wv[3];
    if (p->wide > 0 && p->wide <= m_nsurf && !p->textured && !precise) {
        /* widescreen: a flat 2D polygon across the whole screen (the pause menu's dimming) spans
         * the margins as well: its corners on the screen's sides move to the margins' */
        const Target *t = &m_surf[p->wide - 1];
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
    if (precise && m_precise) {
        /* the hardware's rounded positions can be several pixels off near the camera */
        bx0 = (int)floorf(MIN3(a->px, b->px, c->px));
        by0 = (int)floorf(MIN3(a->py, b->py, c->py));
        bx1 = (int)ceilf(MAX3(a->px, b->px, c->px));
        by1 = (int)ceilf(MAX3(a->py, b->py, c->py));
    }
    GVert *o = BeginPrim(p, 3, bx0, by0, bx1, by1);
    if (!o)
        return;
    uint16_t info = PrimInfo(p);
    if (precise && m_layer.active && m_perspective)
        info |= I_DEPTH;
    PutV(&o[0], p, info, a, precise);
    PutV(&o[1], p, info, b, precise);
    PutV(&o[2], p, info, c, precise);
    if (!precise) {
        float zs[3] = { a->z, b->z, c->z };
        FlatDepth(o, 3, zs);
    }
    if (p->textured && p->depth != 3 && !(precise && m_precise))
        UvRange(o, a, b, c, bx0, by0, bx1, by1);
}

void GlGpu::TriangleF(const GlPrim *p, const float xy[3][2], const uint8_t rgb[3][3])
{
    int bx0 = (int)floorf(MIN3(xy[0][0], xy[1][0], xy[2][0]));
    int by0 = (int)floorf(MIN3(xy[0][1], xy[1][1], xy[2][1]));
    int bx1 = (int)ceilf(MAX3(xy[0][0], xy[1][0], xy[2][0]));
    int by1 = (int)ceilf(MAX3(xy[0][1], xy[1][1], xy[2][1]));
    GVert *o = BeginPrim(p, 3, bx0, by0, bx1, by1);
    if (!o)
        return;
    uint16_t info = PrimInfo(p);
    for (int i = 0; i < 3; i++)
        Put(&o[i], p, info, xy[i][0], xy[i][1], rgb[i][0], rgb[i][1], rgb[i][2], 0, 0);
}

void GlGpu::Rect(const GlPrim *p, int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b,
                 int u0, int v0, int flip_x, int flip_y)
{
    if (w <= 0 || h <= 0)
        return;
    /* the texel at pixel x+i is u0+i, or u0-i when flipped (the shader rounds, see there) */
    int ul = u0, ur = flip_x ? u0 - w : u0 + w;
    int vt = v0, vb = flip_y ? v0 - h : v0 + h;
    if (p->textured && p->depth != 3) {
        int umin = flip_x ? u0 - w + 1 : u0, vmin = flip_y ? v0 - h + 1 : v0;
        CheckTexture(p, umin & 255, vmin & 255, (umin & 255) + w - 1, (vmin & 255) + h - 1);
    }
    if (p->wide > 0 && p->wide <= m_nsurf && !p->textured) {
        /* widescreen: a flat rectangle across the whole screen (a fade, the pause menu's dimming,
         * a flash) spans the margins as well */
        const Target *t = &m_surf[p->wide - 1];
        if (x <= t->rx && x + w >= t->rx + t->rw) {
            x = t->x;
            w = t->w;
        }
    }
    GVert *o = BeginPrim(p, 6, x, y, x + w - 1, y + h - 1);
    if (!o)
        return;
    uint16_t info = PrimInfo(p);
    float x0 = (float)x, x1 = (float)(x + w), y0 = (float)y, y1 = (float)(y + h);
    Put(&o[0], p, info, x0, y0, r, g, b, ul, vt);
    Put(&o[1], p, info, x1, y0, r, g, b, ur, vt);
    Put(&o[2], p, info, x0, y1, r, g, b, ul, vb);
    o[3] = o[1];
    Put(&o[4], p, info, x1, y1, r, g, b, ur, vb);
    o[5] = o[2];
    {
        float zs[6] = { p->z, p->z, p->z, p->z, p->z, p->z };
        FlatDepth(o, 6, zs);
    }
}

void GlGpu::Line(const GlPrim *p, const GlVertex *a, const GlVertex *b)
{
    int dx = b->x - a->x, dy = b->y - a->y;
    if ((dx < 0 ? -dx : dx) >= 1024 || (dy < 0 ? -dy : dy) >= 512)
        return;   /* the GPU drops over-long lines */
    const GlVertex *s = a, *e = b;
    int xmajor = (dx < 0 ? -dx : dx) >= (dy < 0 ? -dy : dy);
    if ((xmajor && dx < 0) || (!xmajor && dy < 0)) {
        s = b;
        e = a;
    }
    GVert *o = BeginPrim(p, 6, s->x < e->x ? s->x : e->x, s->y < e->y ? s->y : e->y,
                         s->x > e->x ? s->x : e->x, s->y > e->y ? s->y : e->y);
    if (!o)
        return;
    /* a one-pixel-wide quad along the major axis, both end points included */
    uint16_t info = PrimInfo(p);
    float sx = (float)s->x, sy = (float)s->y, ex = (float)e->x, ey = (float)e->y;
    if (xmajor) {
        Put(&o[0], p, info, sx, sy, s->r, s->g, s->b, 0, 0);
        Put(&o[1], p, info, ex + 1, ey, e->r, e->g, e->b, 0, 0);
        Put(&o[2], p, info, sx, sy + 1, s->r, s->g, s->b, 0, 0);
        Put(&o[4], p, info, ex + 1, ey + 1, e->r, e->g, e->b, 0, 0);
    } else {
        Put(&o[0], p, info, sx, sy, s->r, s->g, s->b, 0, 0);
        Put(&o[1], p, info, sx + 1, sy, s->r, s->g, s->b, 0, 0);
        Put(&o[2], p, info, ex, ey + 1, e->r, e->g, e->b, 0, 0);
        Put(&o[4], p, info, ex + 1, ey + 1, e->r, e->g, e->b, 0, 0);
    }
    o[3] = o[1];
    o[5] = o[2];
}

void GlGpu::FillPart(int x0, int y0, int x1, int y1)
{
    Target *ts[MAX_SURFACES + 1];
    int nt = TargetsFor(x0, y0, x1, y1, ts);
    for (int i = 0; i < nt; i++) {
        const Target *t = ts[i];
        int all = t != &m_base && x0 == t->rx && x1 == t->rx + t->rw - 1 && y0 == t->y &&
                  y1 == t->y + t->h - 1;   /* the whole screen: with the margins */
        t->Use();
        if (t->Scissor(all ? t->x : x0, y0, all ? t->x + t->w - 1 : x1, y1, all))
            glClear(GL_COLOR_BUFFER_BIT);
    }
}

void GlGpu::Fill(int x, int y, int w, int h, uint32_t rgb24)
{
    if (!m_dbgFillRead) {
        m_dbgFillRead = true;
        m_dbgFill = getenv("THPS_DBG_FILL") ? (int)strtoul(getenv("THPS_DBG_FILL"), NULL, 16) : 0;
    }
    if (m_dbgFill && w >= 256 && h >= 200)
        rgb24 = (uint32_t)m_dbgFill;   /* THPS_DBG_FILL=bbggrr: screen clears in a colour gaps show */
    Flush();
    Prepare();
    /* the fill colour is reduced to 15 bits, like everything else in VRAM */
    glClearColor((float)((rgb24 >> 3) & 31) / 31.0f, (float)((rgb24 >> 11) & 31) / 31.0f,
                 (float)((rgb24 >> 19) & 31) / 31.0f, 0.0f);
    EachPart(x, y, w, h, [this](int x0, int y0, int x1, int y1) { FillPart(x0, y0, x1, y1); });
    glDisable(GL_SCISSOR_TEST);
}

/* ---------------------------------------------------------------------------------------------
 * Native 3D layers */

int GlGpu::LayerTargets(Target **ts)
{
    return DrawTargets(m_layer.wide, m_layer.clip[0], m_layer.clip[1], m_layer.clip[2],
                       m_layer.clip[3], ts);
}

void GlGpu::SetLayerArea(int ofs_x, int ofs_y, const int clip[4], int wide)
{
    Layer &l = m_layer;
    l.lp.ofs_x = ofs_x;
    l.lp.ofs_y = ofs_y;
    l.wide = wide > 0 && wide <= m_nsurf ? wide : 0;
    l.clip[0] = clip[0] < 0 && !l.wide ? 0 : clip[0];
    l.clip[1] = clip[1] < 0 ? 0 : clip[1];
    l.clip[2] = clip[2] > VRAM_W - 1 && !l.wide ? VRAM_W - 1 : clip[2];
    l.clip[3] = clip[3] > VRAM_H - 1 ? VRAM_H - 1 : clip[3];
}

void GlGpu::LayerArea(int ofs_x, int ofs_y, const int clip[4], int wide)
{
    Layer &l = m_layer;
    if (!l.active)
        return;
    SetLayerArea(ofs_x, ofs_y, clip, wide);
    if (l.clip[2] < l.clip[0] || l.clip[3] < l.clip[1])
        return;
    for (int i = 0; i < 2; i++) {
        if (l.clip[i] < l.mark[i])
            l.mark[i] = l.clip[i];
        if (l.clip[i + 2] > l.mark[i + 2])
            l.mark[i + 2] = l.clip[i + 2];
    }
}

void GlGpu::LayerBegin(const GlLayerParams *lp, const GlVert *v, int nv)
{
    Layer &l = m_layer;
    Flush();
    Prepare();
    l.lp = *lp;
    {
        int clip[4] = { lp->clip_l, lp->clip_t, lp->clip_r, lp->clip_b };
        SetLayerArea(lp->ofs_x, lp->ofs_y, clip, lp->wide);
    }
    if (l.clip[2] < l.clip[0] || l.clip[3] < l.clip[1])
        return;
    memcpy(l.mark, l.clip, sizeof l.mark);
    glBindVertexArray(l.vao);
    glBindBuffer(GL_ARRAY_BUFFER, l.vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)nv * sizeof(GVert), v, GL_STREAM_DRAW);
    glDepthMask(GL_TRUE);
    glClearDepth(1.0);
    Target *ts[MAX_SURFACES + 1];
    int nt = LayerTargets(ts);
    for (int i = 0; i < nt; i++) {
        /* all of it: the drawing area can change while the layer draws (LayerArea) */
        ts[i]->Use();
        glDisable(GL_SCISSOR_TEST);
        glClear(GL_DEPTH_BUFFER_BIT);
    }
    l.active = 1;
}

void GlGpu::LayerDraw(int first, int count, int cls, uint16_t hd)
{
    Layer &l = m_layer;
    if (!l.active || count <= 0)
        return;
    Flush();        /* emulated primitives submitted before these are behind them */
    Prepare();
    glUseProgram(m_drawProg);
    if (hd)
        HdBind(hd);
    glUniform4f(m_drawUProj, l.lp.ofs_x + l.lp.ofx, l.lp.ofs_y + l.lp.ofy, l.lp.h, l.lp.hither);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texstore);
    glBindVertexArray(l.vao);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CLIP_DISTANCE0);
    Target *ts[MAX_SURFACES + 1];
    int nt = LayerTargets(ts);
    for (int i = 0; i < nt; i++) {
        ts[i]->Use();
        if (!ts[i]->Scissor(l.clip[0], l.clip[1], l.clip[2], l.clip[3], l.wide))
            continue;
        DrawUniforms(ts[i]);
        glDepthMask(GL_FALSE);
        glUniform1i(m_drawUDpass, 0);
        if (cls == 2) {
            SetBlend(1);
            glDrawArrays(GL_TRIANGLES, first, count);
            SetBlend(2);
        } else {
            SetBlend(0);
        }
        glDrawArrays(GL_TRIANGLES, first, count);
        m_st.draws++;
        if (cls == 0) {
            /* opaque: now the true depth (see the shaders) */
            glUniform1i(m_drawUDpass, 1);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            glDepthMask(GL_TRUE);
            glDrawArrays(GL_TRIANGLES, first, count);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glUniform1i(m_drawUDpass, 0);
        }
    }
    glDisable(GL_CLIP_DISTANCE0);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
}

void GlGpu::LayerEnd()
{
    Layer &l = m_layer;
    if (!l.active)
        return;
    Flush();
    l.active = 0;
    MarkDrawn(l.wide, l.mark[0], l.mark[1], l.mark[2], l.mark[3]);
}

/* ---------------------------------------------------------------------------------------------
 * Reading and writing screens' pixels */

/* The pixels of an area (inclusive, inside VRAM) as drawn, at the PS1's resolution, into out
 * (rows of x1 - x0 + 1, 15-bit with the mask bit). Each pixel comes from the target it belongs
 * to: surfaces are scaled down (nearest), whole 16x16 cells at a time. */
void GlGpu::ReadPixels(int x0, int y0, int x1, int y1, uint16_t *out)
{
    uint8_t *buf = m_readBuf;
    if (!m_to5[255])
        for (int i = 0; i < 256; i++)
            m_to5[i] = (uint8_t)((i * 31 + 16) / 255);
    Flush();
    int px = x0 / CELL * CELL, py = y0 / CELL * CELL;
    int pw = (x1 / CELL + 1) * CELL - px, ph = (y1 / CELL + 1) * CELL - py;
    /* the base, then the surfaces over it */
    Target *ts[MAX_SURFACES + 1];
    int nt = TargetsFor(px, py, px + pw - 1, py + ph - 1, ts);
    glDisable(GL_SCISSOR_TEST);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_rbFbo);
    for (int i = nt - 1; i >= 0; i--) {
        const Target *t = ts[i];
        int ax0 = px > t->rx ? px : t->rx, ay0 = py > t->y ? py : t->y;
        int ax1 = px + pw < t->rx + t->rw ? px + pw : t->rx + t->rw;
        int ay1 = py + ph < t->y + t->h ? py + ph : t->y + t->h;
        int sx0, sx1, sy0, sy1;
        if (ax1 <= ax0 || ay1 <= ay0)
            continue;
        Span(t->sx, t->x, ax0, ax1 - 1, &sx0, &sx1);
        Span(t->sy, t->y, ay0, ay1 - 1, &sy0, &sy1);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, t->fbo);
        glBlitFramebuffer(sx0, sy0, sx1, sy1, ax0, ay0, ax1, ay1, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_rbFbo);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glReadPixels(px, py, pw, ph, GL_RGBA, GL_UNSIGNED_BYTE, buf);
    m_st.readbacks++;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    int w = x1 - x0 + 1;
    for (int y = y0; y <= y1; y++) {
        const uint8_t *s = buf + ((size_t)(y - py) * pw + (x0 - px)) * 4;
        uint16_t *d = out + (size_t)(y - y0) * w;
        for (int i = 0; i < w; i++, s += 4)
            d[i] = (uint16_t)(m_to5[s[0]] | m_to5[s[1]] << 5 | m_to5[s[2]] << 10 |
                              (s[3] >= 128 ? 0x8000 : 0));
    }
}

void GlGpu::Read(int x, int y, int w, int h, uint16_t *out)
{
    int ox = x, oy = y, ow = w;
    if (!m_active || !ClipArea(&x, &y, &w, &h))
        return;
    if (ox == x && oy == y && ow == w) {
        ReadPixels(x, y, x + w - 1, y + h - 1, out);
        return;
    }
    std::vector<uint16_t> tmp((size_t)w * h);
    ReadPixels(x, y, x + w - 1, y + h - 1, tmp.data());
    for (int j = 0; j < h; j++)
        memcpy(out + (size_t)(y - oy + j) * ow + (x - ox), tmp.data() + (size_t)j * w, (size_t)w * 2);
}

void GlGpu::Blit(int x, int y, int w, int h, const uint16_t *src)
{
    int sw = w;
    if (!m_active)
        return;
    {
        int ox = x, oy = y;
        if (!ClipArea(&x, &y, &w, &h))
            return;
        src += (size_t)(y - oy) * sw + (x - ox);
    }
    Flush();   /* primitives already sent draw before these pixels arrive */
    glBindTexture(GL_TEXTURE_2D, m_blit);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, sw);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RED_INTEGER, GL_UNSIGNED_SHORT, src);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    float x0 = (float)x, x1 = (float)(x + w), y0 = (float)y, y1 = (float)(y + h);
    float q[12] = { x0, y0, x1, y0, x0, y1, x1, y0, x1, y1, x0, y1 };
    glDisable(GL_BLEND);
    glUseProgram(m_copyProg);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_blit);
    glBindVertexArray(m_copyVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_copyVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof q, q, GL_STREAM_DRAW);
    glUniform2f(m_copyUSrc, (float)x, (float)y);
    Target *ts[MAX_SURFACES + 1];
    int nt = TargetsFor(x, y, x + w - 1, y + h - 1, ts);
    for (int i = 0; i < nt; i++) {
        const Target *t = ts[i];
        t->Use();
        if (!t->Scissor(x, y, x + w - 1, y + h - 1, 0))
            continue;
        glUniform4f(m_copyUTarget, (float)t->x, (float)t->y, (float)t->w, (float)t->h);
        glUniform4f(m_copyUMap, (float)t->x, (float)t->y, t->sx, t->sy);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    glDisable(GL_SCISSOR_TEST);
    m_st.blits++;
}

/* The display surface holding all of a VRAM area, or NULL. */
Target *GlGpu::SurfaceFor(int x, int y, int w, int h)
{
    for (int i = 0; i < m_nsurf; i++) {
        Target *t = &m_surf[i];
        if (x >= t->rx && y >= t->y && x + w <= t->rx + t->rw && y + h <= t->y + t->h)
            return t;
    }
    return NULL;
}

/* The VRAM area of a display (x, y, w, h) as shown: a widened surface's margins too. */
const Target *GlGpu::ShownArea(int *x, int *w, int y, int h)
{
    const Target *t = SurfaceFor(*x, y, *w, h);
    if (!t)
        return &m_base;
    if (*x == t->rx && *w == t->rw) {
        *x = t->x;
        *w = t->w;
    }
    return t;
}

uint32_t *GlGpu::ReadArea(int x, int y, int w, int h, int *out_w, int *out_h)
{
    if (!m_active || w <= 0 || h <= 0 || x < 0 || y < 0 || x + w > VRAM_W || y + h > VRAM_H)
        return NULL;
    const Target *t = ShownArea(&x, &w, y, h);
    int px0, px1, py0, py1;
    Span(t->sx, t->x, x, x + w - 1, &px0, &px1);
    Span(t->sy, t->y, y, y + h - 1, &py0, &py1);
    int sw = px1 - px0, sh = py1 - py0;
    uint32_t *out = (uint32_t *)malloc((size_t)sw * sh * 4);
    if (!out)
        return NULL;
    Flush();
    Prepare();
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

/* The picture of a screen whose drawing area is w wide (its VRAM area rw wide) in the window:
 * 4:3 widened by the margins, as large as fits. */
void GlGpu::PictureRect(int w, int rw, int *r) const
{
    double a = 4.0 / 3.0 * w / rw;
    int dw = m_outW, dh = m_outH;
    if (m_outW > m_outH * a)
        dw = (int)(m_outH * a + 0.5);
    else
        dh = (int)(m_outW / a + 0.5);
    if (dw > m_outW)
        dw = m_outW;
    if (dh > m_outH)
        dh = m_outH;
    r[0] = (m_outW - dw) / 2;
    r[1] = (m_outH - dh) / 2;
    r[2] = dw;
    r[3] = dh;
}

/* Size of a surface for a w*h screen with pad columns of margin, in pixels. */
void GlGpu::SurfaceSize(int w, int h, int pad, int *pw, int *ph) const
{
    if (m_fixedScale) {
        *pw = (w + 2 * pad) * m_fixedScale;
        *ph = h * m_fixedScale;
    } else {
        int r[4];
        PictureRect(w + 2 * pad, w, r);
        *pw = r[2];
        *ph = r[3];
    }
}

/* Give surface i's pixels back to the base and drop it. */
void GlGpu::SurfaceRetire(int i)
{
    Target *t = &m_surf[i];
    CopyArea(t, &m_base, t->rx, t->y, t->rw, t->h);
    t->Destroy();
    memmove(&m_surf[i], &m_surf[i + 1], (size_t)(m_nsurf - i - 1) * sizeof(Target));
    m_nsurf--;
}

/* A surface for a VRAM area, with what the base has there. */
void GlGpu::SurfaceAdd(int x, int y, int w, int h)
{
    int pw, ph, pad = ScreenPad(w, h);
    if (m_nsurf == MAX_SURFACES)
        SurfaceRetire(0);
    SurfaceSize(w, h, pad, &pw, &ph);
    if (!m_surf[m_nsurf].Create(x, y, w, h, pad, pw, ph, m_maxTex)) {
        fprintf(stderr, "[gl] no %dx%d surface for the screen at %d,%d\n", pw, ph, x, y);
        return;
    }
    CopyArea(&m_base, &m_surf[m_nsurf], x, y, w, h);
    m_nsurf++;
    if (m_st.on)
        fprintf(stderr, "[gl] surface for the screen at %d,%d %dx%d (+%d each side): %dx%d pixels\n",
                x, y, w, h, pad, pw, ph);
}

void GlGpu::NoteScreen(int x, int y, int w, int h)
{
    /* (at 1x the base is the same thing, and the 1x comparisons stay exact) */
    if (!m_active || m_fixedScale == 1 || w < 256 || h < 192 || x < 0 || y < 0 ||
        x + w > VRAM_W || y + h > VRAM_H)
        return;
    for (int i = 0; i < m_nsurf; i++)
        if (m_surf[i].rx == x && m_surf[i].y == y && m_surf[i].rw == w && m_surf[i].h == h)
            return;
    Flush();
    for (int i = m_nsurf - 1; i >= 0; i--) {
        const Target *t = &m_surf[i];
        if (t->rx < x + w && x < t->rx + t->rw && t->y < y + h && y < t->y + t->h)
            SurfaceRetire(i);
    }
    SurfaceAdd(x, y, w, h);
}

int GlGpu::WideClip(int clip[4])
{
    for (int i = 0; i < m_nsurf; i++) {
        const Target *t = &m_surf[i];
        if (t->w != t->rw && clip[0] == t->rx && clip[2] == t->rx + t->rw - 1 && clip[1] == t->y &&
            clip[3] == t->y + t->h - 1) {
            clip[0] = t->x;
            clip[2] = t->x + t->w - 1;
            return i + 1;
        }
    }
    return 0;
}

/* Widescreen margins for a window: gameplay's 512x240 (shown 4:3) widened to the window's shape,
 * when that is wider than 4:3. Only with the native 3D renderer, which draws the margins. */
int GlGpu::WidePad(int ww, int wh) const
{
    if (!m_widescreen || !R3d_Native() || ww * 3 <= wh * 4)
        return 0;
    int pad = (int)((512.0 * ww / wh * 0.75 - 512.0) / 2 + 0.5);
    return pad > 400 ? 400 : pad;   /* GPU coordinates end at -1024 and 1023 */
}

/* The window's size (or the widescreen setting) changed: make the surfaces again to suit. */
void GlGpu::SetOutputSize(int w, int h)
{
    int pad = WidePad(w, h);
    if (w < 1 || h < 1 || (w == m_outW && h == m_outH && pad == m_pad))
        return;
    m_outW = w;
    m_outH = h;
    if (m_fixedScale && pad == m_pad)
        return;
    m_pad = pad;
    pc_wide_pad = pad;
    Flush();
    for (int n = m_nsurf; n > 0; n--) {
        Target t = m_surf[0];
        SurfaceRetire(0);
        SurfaceAdd(t.x, t.y, t.w, t.h);
    }
}

/* ---------------------------------------------------------------------------------------------
 * Presentation */

void GlGpu::Present(void *sdl_window, int x, int y, int w, int h, int enabled,
                    const uint32_t *image, int iw, int ih)
{
    SDL_Window *win = (SDL_Window *)sdl_window;
    if (!m_active || !win)
        return;
    Flush();

    int ww, wh, pic[4];
    SDL_GL_GetDrawableSize(win, &ww, &wh);
    SetOutputSize(ww, wh);
    PictureRect(1, 1, pic);   /* 4:3 (movies) */
    Prepare();
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
            glBindTexture(GL_TEXTURE_2D, m_dispTex);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, iw, ih, GL_BGRA, GL_UNSIGNED_BYTE, image);
            u0 = 0;
            v0 = 0;
            u1 = (float)iw / VRAM_W;
            v1 = (float)ih / VRAM_H;
        } else {
            int rw = w;
            const Target *t = ShownArea(&x, &w, y, h);
            int px0, px1, py0, py1;
            PictureRect(w, rw, pic);
            Span(t->sx, t->x, x, x + w - 1, &px0, &px1);
            Span(t->sy, t->y, y, y + h - 1, &py0, &py1);
            /* a surface the window's size is copied pixel for pixel */
            int exact = px1 - px0 == pic[2] && py1 - py0 == pic[3];
            int smooth = m_filter || (t != &m_base && !m_fixedScale);
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
        glUseProgram(m_presentProg);
        glBindVertexArray(m_presentVao);
        glBindBuffer(GL_ARRAY_BUFFER, m_presentVbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof q, q);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
    SDL_GL_SwapWindow(win);
    if (m_st.on && ++m_st.frames == 60) {
        fprintf(stderr, "[gl] last 60 frames: %d draw calls, %d readbacks, %d texture uploads, "
                        "%d blits\n",
                m_st.draws, m_st.readbacks, m_st.tex_ups, m_st.blits);
        m_st = Stats{};
        m_st.on = 1;
    }
}

}   // namespace pc
