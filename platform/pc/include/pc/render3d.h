/* The native 3D renderer for the world and models (src/pc_render3d.cpp; the outline and the C
 * API the game calls are in pc_render3d.h). */
#pragma once

#include <stdint.h>

#include "pc/gl_gpu.h"
#include "pc_render3d.h"

struct P_TAG; /* libgpu.h */

namespace pc {

class Renderer3D {
public:
    /* After the OpenGL backend started: gl enables the native renderer (unless THPS_RENDER3D=psx). */
    void Init(int gl);
    bool Native() const { return m_native; }

    /* The entire level (pc_render3d.h R3d_FarBegin/R3d_FarEnd): the item transform in double
     * precision, with no far plane. */
    void FarBegin(const double m[3][3], const double t[3], int fogstart, int fogshift);
    void FarEnd();

    /* Runs transform fn of that kind (the entire level's far path instead), keeping its vertices
     * unrounded; the result's outcodes against a widened viewport. */
    int Transform(int kind, R3dTransformFn fn, void *v, int n, void *o);
    /* ProcessPolys (fast = 0) and ProcessPolysFast (fast = 1): 1 if the native renderer drew the
     * faces, 0 if the game's C should. */
    int ProcessPolys(int fast, uint32_t *list, int count);
    /* M3d_RenderSetup(camera, viewport, ot), after the game's: a 3D layer starts at ot. */
    void RenderSetup(const void *viewport, P_TAG *ot);

    /* PC_DrawOT, for each packet before it is drawn: true if it was a layer marker (its words are
     * not GPU commands). ChainEnd at the end of the chain. */
    bool ChainPacket(const P_TAG *p);
    void ChainEnd();
    /* THPS_R3D_DUMP: the faces of the layer drawn last, to a text file (debugging). */
    void DumpLast(const char *path);

private:
    static constexpr int kPvSlots = 512;   /* ProjectedVertices: 400 slots before RotatedVertices (+3200) */
    static constexpr int kSpSlots = 128;   /* scratchpad cache of the fast paths (1 KB) */
    static constexpr int kBatches = 8;

    /* A float copy of a projected-vertex cache slot: the view-space position, and the screen word
     * retail stored in the slot when this was made. */
    struct Shadow {
        float x, y, z;
        uint32_t tag;
    };

    struct Face {
        uint16_t bucket;
        uint8_t cls, nv;
        uint32_t first;
        float zmin, zmax;   /* view depth range of its corners */
        uint16_t hd;        /* its HD texture (pc_dctex.h), 0 if none */
        uint32_t src;       /* the face's address in the model (THPS_R3D_DUMP) */
    };

    /* The faces of one M3d_RenderSetup's layer. */
    struct Batch {
        uint32_t gen;
        const P_TAG *ot;    /* the layer's ordering table (4096 entries) */
        int valid, sorted, next;
        float h, ofx, ofy, hither;
        Face *f, *sf;
        int nf, capf;
        GlVert *v, *sv;
        int nv, capv;

        GlVert *AllocVerts(int n);
        void AddFace(int bucket, int cls, int count, uint32_t first, uint16_t hd, uint32_t src);
        void Sort();
    };

    struct Water;   /* the water plane of a call that tests it */

    struct Stats {
        uint32_t faces, culled, badtag, layers, draws, calls, water, fade, fb_fade, fb_nolayer;
        uint32_t water_checked, water_bad;
        uint32_t vchecked[8], vbad[8], sign_checked, sign_bad, near, far;
    };

    Shadow *ShadowAt(uintptr_t a);
    void SetShadow(uintptr_t a, float x, float y, float z);
    void VerifySlot(int kind, uintptr_t slot, uintptr_t rot);
    uint32_t FarTransform(uintptr_t a0, int n);
    uint32_t TransformVertices(int kind, R3dTransformFn fn, void *verts, int count, void *o);

    uint32_t Dpcs(uint32_t rgbc);
    uint32_t Dpcl(uint32_t rgbc, uint32_t light);
    void Probe(const uintptr_t *sl, int fast, uint32_t f, const char *what, uint32_t a, uint32_t c,
               uint32_t z);
    void Faces(Batch *b, uintptr_t face, int n, int fast, const Water *water);

    void DrawUpto(uint32_t bucket);
    void LayerEnd();
    void LayerStart(uint32_t id);
    void StatsTick();

    /* options (environment, Init) */
    bool m_native = false, m_verify = false, m_showStats = false, m_nearDrop = false;
    bool m_retailFade = false, m_nclip = false;
    Stats m_st = {};

    /* float vertices */
    Shadow m_pv[kPvSlots] = {}, m_sp[kSpSlots] = {};
    uintptr_t m_pvBase = 0;
    int m_lastKind = -1;   /* the transform the next ProcessPolys draws from */
    float m_cap[1024][3] = {};
    int m_verifyShown = 0;

    /* the entire level (FarBegin) */
    struct {
        int on;
        double m[3][3], t[3];
        int fogstart, fogshift;
    } m_far = {};

    /* batches */
    Batch m_b[kBatches] = {};
    int m_cur = -1;
    uint32_t m_gen = 0;

    /* the face walk */
    uint32_t m_src = 0;   /* the face being added */
    int16_t m_fog = 0;    /* the fog (0..4096) towards the far colour the colours get, as retail's */
    int m_probeOn = -1, m_probeFrame = 0, m_probeX = 0, m_probeY = 0;   /* THPS_R3D_PROBE */

    /* drawing */
    Batch *m_layer = nullptr;
    uint32_t m_bucket = 0;
    Batch *m_lastDrawn = nullptr;
    int m_lastOfs[2] = {};
    uint32_t m_statsFrame = 0;
};

extern Renderer3D g_renderer3d;

} // namespace pc
