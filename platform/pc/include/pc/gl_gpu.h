/* The OpenGL 3.3 drawing backend (src/pc_gpu_gl.cpp). pc_draw.cpp parses the
 * game's primitives and holds the drawing state; it hands polygons, rectangles, lines and fills
 * here. They are drawn into render targets: the screens (display surfaces, see GlGpu::NoteScreen)
 * at the size their picture is shown at, anything else at the PS1's resolution. Textures come from
 * the texture store (pc/texture_store.h). The screens' pixels live only in the render targets: the
 * CPU writes them with GlGpu::Blit and reads them with GlGpu::Read (at the PS1's resolution).
 * Here: render targets and display surfaces, the batched triangle stream
 * drawn with the PS1 texture/CLUT shader, the native 3D layers, HD textures, readback and
 * presentation with widescreen margins. */
#pragma once

#include <stdint.h>

#include <glad/gl.h>

/* Widescreen margin columns of the gameplay screens, for the game's view frustum (game/src/bit.cpp). */
extern "C" int pc_wide_pad;

namespace pc {

/* A vertex of an emulated primitive. */
struct GlVertex {
    int x, y;        /* VRAM position, drawing offset applied */
    uint8_t r, g, b;
    int u, v;        /* texel coordinates */
    float px, py, w; /* precise position (offset applied) and view depth; w = 0: unknown */
    float z;         /* > 0: the view depth of an effect made in screen space (PC_DRAW_DEPTH) */
};

/* An emulated primitive's drawing state. */
struct GlPrim {
    int textured, raw, semi, abr;
    int depth, tp_x, tp_y, clut_x, clut_y;
    uint8_t tw_and_x, tw_and_y, tw_or_x, tw_or_y;
    int mask_set, mask_check;
    int clip_l, clip_t, clip_r, clip_b;   /* drawing area, inclusive */
    int wide;                             /* the area is widened (GlGpu::WideClip) */
    float z;   /* > 0: a rectangle's view depth (PC_DRAW_DEPTH; polygons have GlVertex.z) */
};


/* ---- Native 3D layers (pc_render3d.cpp) ----
 * A vertex as the shader takes it. Emulated primitives use x, y in VRAM coordinates and z as
 * the perspective divisor (1 = affine). With PC_GL_I_VIEW, x, y, z are a view-space position
 * that the layer's projection maps to the screen, with a depth buffer. info = texture page bits
 * 0-3 (x/64) and 4 (y/256), blend mode 5-6, colour depth 7-8, then the flags below; clut is the
 * GPU's CLUT halfword; tw = texture window and_x, and_y, or_x, or_y; uvr = the texels the
 * face's texture coordinates span (u min, v min, u max, v max), which its samples stay inside.
 * Colour depth 3 (info 0x180) with PC_GL_I_TEXTURED: an HD texture (pc_dctex.h), clut = its
 * handle, u, v in 256ths of one repeat. */
struct GlVert {
    float x, y, z;
    uint8_t r, g, b, a;   /* a: PC_GL_A_SHADOWED, else 0 */
    float u, v;
    uint16_t clut, info;
    uint8_t tw[4];
    uint8_t uvr[4];
};

#define PC_GL_I_TEXTURED 0x0200
#define PC_GL_I_SEMI     0x0800
#define PC_GL_I_DEPTH    0x2000   /* emulated primitive with a view depth in z (set internally) */
#define PC_GL_I_VIEW     0x4000   /* view-space position, projected by the layer */
#define PC_GL_I_SKY      0x8000   /* behind everything: depth 1, never written */

/* GlVert.a of a native face that the Dreamcast's skater shadows fall on (the level, not models) */
#define PC_GL_A_SHADOWED 255

/* A skater's shadow over a native 3D layer (the Dreamcast's, pc_render3d.h R3d_ShadowQuad): its
 * blobs, drawn into a texture of its own, and the planes that map a view-space point p onto it:
 * texture s = dot(s.xyz, p) + s.w, likewise t, and the height above the ground towards the light
 * h (h.xyz is the light's direction). */
struct GlShadow {
    static constexpr int kQuads = 32;
    float s[4], t[4], h[4];
    float top;        /* the height of the skater's highest part: nothing above it is shadowed */
    float size;       /* half the texture's width, in world units */
    float strength;   /* the darkening at the shadow's centre */
    int nquads;
    float quad[kQuads][4][2];   /* the blobs' corners in the texture (0..1): -e-f, +e-f, -e+f, +e+f */
};

/* A native 3D layer's screen and projection. */
struct GlLayerParams {
    int ofs_x, ofs_y;                     /* drawing offset */
    int clip_l, clip_t, clip_r, clip_b;   /* drawing area, inclusive */
    float h, ofx, ofy;                    /* projection: screen = of + h * xy / z (GTE H, OFX, OFY) */
    float hither;                         /* near clip plane (view z) */
    int wide;                             /* the drawing area is widened (GlGpu::WideClip) */
};

/* A render target for a VRAM area: the base target is all of VRAM at the PS1's resolution, a
 * display surface one screen buffer at the size it is shown at (see GlGpu::NoteScreen). Each VRAM
 * pixel belongs to the surface covering it, else to the base; drawing goes to every target its
 * area touches, so the base under a surface is never read.
 *
 * Widescreen: a gameplay surface also has margins left and right of its VRAM area, in drawing
 * coordinates beyond the buffer (x < 0, or past it where VRAM holds textures). Only primitives
 * drawn with the widened drawing area (GlGpu::WideClip; GlPrim.wide) reach them; to everything
 * else, and to the cell bookkeeping, the surface is its VRAM area alone. */
struct RenderTarget {
    int x, y, w, h;           /* drawing area: the VRAM area plus the margins */
    int rx, rw;               /* the VRAM area's columns (its rows are the drawing area's) */
    int pw, ph;               /* size in pixels */
    float sx, sy;             /* pixels per VRAM pixel */
    GLuint tex, depth, fbo;   /* RGBA8 colour, DEPTH32F for the native 3D layers (pc_render3d.cpp) */
    /* Antialiasing (GlGpu::SetSamples): with samples > 0 drawing goes to the multisampled ms_fbo
     * (colour ms_color, and depth), and tex/fbo hold its resolved pixels, brought up to date by
     * Resolve before anything reads them. */
    int samples;
    GLuint ms_color, ms_fbo;
    mutable bool stale;       /* ms_fbo was drawn on since the last Resolve */

    /* VRAM area x,y w*h, with pad columns of margin on either side, drawn at pw*ph pixels (at
     * most max_tex), with samples samples per pixel (0: single). false on failure. */
    bool Create(int x, int y, int w, int h, int pad, int pw, int ph, int samples, GLint max_tex);
    void Destroy();
    /* Bind for drawing (the whole target is the viewport). */
    void Use() const;
    /* tex/fbo as drawn (leaves no framebuffer bound and the scissor test off). */
    void Resolve() const;
    /* The pixels px0,py0..px1,py1 (exclusive) were written to fbo: copy them to ms_fbo too. */
    void Written(int px0, int py0, int px1, int py1) const;
    /* Scissor to the area x0,y0..x1,y1 (inclusive): of the VRAM area, or with wide of the whole
     * drawing area (margins included); false if none of it is here. */
    bool Scissor(int x0, int y0, int x1, int y1, int wide) const;
};

class GlGpu {
  public:
    static constexpr int VRAM_W = 1024;
    static constexpr int VRAM_H = 512;
    static constexpr int CELL = 16;
    static constexpr int CELLS_X = VRAM_W / CELL;
    static constexpr int CELLS_Y = VRAM_H / CELL;
    static constexpr int MAX_SURFACES = 4;
    static constexpr int MAX_HD = 8192;
    static constexpr int MAX_VERTS = 64 * 1024;
    static constexpr int kShadows = 2;          /* per layer: one for each player, as the Dreamcast */
    static constexpr int kShadowSize = 128;     /* a shadow's texture, as the Dreamcast's */

    /* The interface of pc_gpu_gl.h, one method per PC_Gl* function (see there). */
    int Init(int fixed_scale, int out_w, int out_h);
    void Shutdown();
    void NoteScreen(int x, int y, int w, int h);
    void SetWidescreen(int on) { m_widescreen = on; }
    /* Multisample antialiasing of the screens: samples per pixel (0 or 1: off; at most what the
     * driver offers). Remakes the display surfaces when it changes. */
    void SetSamples(int n);
    int WideClip(int clip[4]);
    void Triangle(const GlPrim *p, const GlVertex *a, const GlVertex *b,
                  const GlVertex *c);
    void TriangleF(const GlPrim *p, const float xy[3][2], const uint8_t rgb[3][3]);
    void Rect(const GlPrim *p, int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b,
              int u0, int v0, int flip_x, int flip_y);
    void Line(const GlPrim *p, const GlVertex *a, const GlVertex *b);
    void Fill(int x, int y, int w, int h, uint32_t rgb24);
    void HiResImage(int x, int y, int w, int h, const uint16_t *src, int sw, int sh);
    /* The skaters' shadows the next layer's faces get (n <= kShadows; 0: none). */
    void LayerShadows(const GlShadow *s, int n);
    void LayerBegin(const GlLayerParams *lp, const GlVert *v, int nv);
    void LayerArea(int ofs_x, int ofs_y, const int clip[4], int wide);
    void LayerDraw(int first, int count, int cls, uint16_t hd);
    void LayerEnd();
    void HdForget(uint16_t h);
    void TexturesChanging(int x, int y, int w, int h);
    void Blit(int x, int y, int w, int h, const uint16_t *src);
    void Read(int x, int y, int w, int h, uint16_t *out);
    void Present(void *sdl_window, int x, int y, int w, int h, int enabled,
                 const uint32_t *image, int iw, int ih);
    uint32_t *ReadArea(int x, int y, int w, int h, int *out_w, int *out_h);

  private:
    /* the pending batch of emulated primitives (one triangle stream) */
    struct Batch {
        int nv;
        int sub;          /* the batch is B-F semi-transparent */
        int wide;         /* its drawing area is widened (GlPrim.wide) */
        int clip[4];      /* its drawing area (inclusive) */
        int bbox[4];      /* area the pending batch draws on (inclusive) */
        uint16_t hd;      /* its HD texture */
        GlVert verts[MAX_VERTS];
    };

    /* A native 3D layer being drawn (PC_GlLayer*): its vertices, and the area it draws on. While
     * it is active, emulated primitives with a precise depth are depth-tested against it. */
    struct Layer {
        int active;
        int clip[4];
        int mark[4];      /* the union of the areas the layer drew in */
        int wide;         /* the layer's drawing area is widened (GlLayerParams.wide) */
        GlLayerParams lp;
        GLuint vao, vbo;
    };

    /* THPS_GLSTATS=1: per-second counts of draw calls, readbacks and uploads */
    struct Stats {
        int on, draws, readbacks, tex_ups, blits, frames;
    };

    /* targets */
    void DrawUniforms(const RenderTarget *t);
    int TargetsFor(int x0, int y0, int x1, int y1, RenderTarget **out);
    int DrawTargets(int wide, int x0, int y0, int x1, int y1, RenderTarget **out);
    void MarkDrawn(int wide, int x0, int y0, int x1, int y1);
    bool AnyDrawn(int x0, int y0, int x1, int y1) const;
    /* textures */
    void UploadTexStore();
    void Prepare();
    void HdBind(uint16_t h);
    /* batches */
    void SetBlend(int pass);
    void Flush();
    void CheckTexture(const GlPrim *p, int umin, int vmin, int umax, int vmax);
    GlVert *BeginPrim(const GlPrim *p, int nverts, int bx0, int by0, int bx1, int by1);
    void PutV(GlVert *o, const GlPrim *p, uint16_t info, const GlVertex *s, int precise);
    void FlatDepth(GlVert *o, int n, const float *zs);
    void FillPart(int x0, int y0, int x1, int y1);
    /* layers */
    int LayerTargets(RenderTarget **ts);
    bool ShadowSetup();
    void ShadowUniforms(int n);
    void SetLayerArea(int ofs_x, int ofs_y, const int clip[4], int wide);
    /* screens' pixels */
    void ReadPixels(int x0, int y0, int x1, int y1, uint16_t *out);
    RenderTarget *SurfaceFor(int x, int y, int w, int h);
    const RenderTarget *ShownArea(int *x, int *w, int y, int h);
    /* display surfaces */
    int ScreenPad(int w, int h) const { return w == 512 && h == 240 ? m_pad : 0; }
    void PictureRect(int w, int rw, int *r) const;
    void SurfaceSize(int w, int h, int pad, int *pw, int *ph) const;
    void SurfaceRetire(int i);
    void SurfaceAdd(int x, int y, int w, int h);
    int WidePad(int ww, int wh) const;
    void SetOutputSize(int w, int h);

    int m_active = 0;
    GLuint m_drawProg = 0, m_copyProg = 0, m_presentProg = 0;
    GLint m_drawUPass = 0, m_drawUDpass = 0, m_drawUProj = 0, m_drawUTarget = 0,
          m_drawUShift = 0, m_drawUUvfix = 0, m_copyUTarget = 0, m_copyUMap = 0, m_copyUSrc = 0;
    GLuint m_vao = 0, m_vbo = 0, m_copyVao = 0, m_copyVbo = 0, m_presentVao = 0, m_presentVbo = 0;
    GLuint m_blit = 0;                     /* R16UI 1024x512: pixels on their way to a screen */
    GLuint m_texstore = 0;                 /* R16UI 1024x512: textures and CLUTs (pc/texture_store.h) */
    RenderTarget m_base = {};              /* 1024x512 */
    RenderTarget m_surf[MAX_SURFACES] = {};   /* display surfaces, oldest first; they never overlap */
    int m_nsurf = 0;
    int m_fixedScale = 0;                  /* THPS_GL_SCALE: surfaces at n times the PS1 resolution */
    int m_outW = 0, m_outH = 0;            /* the window's drawable size (pixels) */
    int m_widescreen = 0;                  /* SetWidescreen */
    int m_pad = 0;                         /* widescreen margin of gameplay screens, in PS1 pixels */
    int m_samples = 0;                     /* SetSamples: the display surfaces' samples (0: off) */
    GLint m_maxSamples = 0;
    GLint m_maxTex = 0;
    GLuint m_rbFbo = 0, m_rbTex = 0;       /* RGBA8 1024x512: readback */
    GLuint m_dispTex = 0;                  /* RGBA8 1024x512: pictures shown instead (movies) */
    int m_filter = 0;
    int m_precise = 0, m_perspective = 0;  /* THPS_GL_PRECISE, THPS_GL_PERSPECTIVE (default on) */
    Stats m_st = {};
    int m_warnedMask = 0;                  /* the mask-bit test was logged */
    int m_warnedRtt = 0;                   /* render-to-texture was logged */
    int m_dbgFill = 0;                     /* THPS_DBG_FILL, once m_dbgFillRead */
    bool m_dbgFillRead = false;
    Layer m_layer = {};
    /* the skaters' shadows (LayerShadows): the blob image, a texture per shadow, the program that
     * draws the blobs into them, and the draw program's uniforms */
    int m_shadowState = 0;                 /* 0: not set up yet, 1: ready, -1: failed */
    GLuint m_shadowProg = 0, m_shadowVao = 0, m_shadowVbo = 0, m_shadowBlob = 0;
    GLuint m_shadowTex[kShadows] = {}, m_shadowFbo[kShadows] = {};
    GLint m_drawUShN = 0, m_drawUShS = 0, m_drawUShT = 0, m_drawUShH = 0, m_drawUShP = 0;
    GLuint m_hd[MAX_HD] = {};              /* HD textures by handle (pc_dctex.h), made when first drawn */
    /* 8-bit channel -> 5 bits: exact for every value a 5-bit one was stored as (n * 255 / 31,
     * rounded), and the PS1's truncation for the in-between values drawing produces. */
    uint8_t m_to5[256] = {};
    uint8_t m_drawn[CELLS_Y][CELLS_X] = {};   /* 16x16 cells primitives drew on */
    Batch m_batch = {};
    uint8_t m_readBuf[VRAM_W * VRAM_H * 4] = {};   /* ReadPixels' readback */
};

extern GlGpu g_glGpu;

}   // namespace pc
