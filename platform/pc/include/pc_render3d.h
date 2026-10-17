/* Native 3D renderer for the world and models (src/pc_render3d.cpp).
 *
 * THPS1 draws its 3D with hand-written assembly (M3DASM): vertex transforms that project through
 * the GTE into a cache, then ProcessPolys, which lights, culls and subdivides each face and links
 * GPU packets into the ordering table. With THPS_RENDER3D=native (the default with OpenGL) the
 * transforms still run, but their view-space vertices are kept unrounded, and ProcessPolys is
 * replaced: faces are drawn by OpenGL from those vertices with a projection and a depth buffer,
 * at the place in the ordering table the PS1 would have drawn them. THPS_RENDER3D=psx keeps the
 * PS1 path. */
#ifndef PC_RENDER3D_H
#define PC_RENDER3D_H

#include <stdint.h>

struct P_TAG; /* libgpu.h */

#ifdef __cplusplus
extern "C" {
#endif

/* After the OpenGL backend started: gl = 1 enables the native renderer (unless THPS_RENDER3D=psx). */
int R3d_Native(void);

/* Drawing the entire level (Options > Graphics > Draw Distance, pc_video.h; M3d_Render): the PS1
 * transforms work in 16-bit view space, so items beyond it are drawn from here. Between these,
 * the item transform (M3dAsm_TransformAndOutcodeItemVertices) works in double precision: view =
 * m * vertex + t (m: the view matrix times the item's rotation, t: the item's view-space
 * position), with no far plane; its faces are fogged on their true depth (fog from fogstart,
 * INT_MAX for none, with fogshift as DpqShift). */
void R3d_FarBegin(const double m[3][3], const double t[3], int fogstart, int fogshift);
void R3d_FarEnd(void);

/* The renderer's hooks in the game's code (game/src/m3dasm and M3d_RenderSetup). */

/* The vertex transforms, by the kind of model they project. */
enum {
    R3D_T_ITEM,        /* M3dAsm_TransformAndOutcodeItemVertices */
    R3D_T_SUPER,       /* ...SuperVertices */
    R3D_T_SUPER_DW,    /* ...SuperVerticesDewibbled */
    R3D_T_SFAST,       /* ...SuperVerticesFast */
    R3D_T_SFAST_DW,    /* ...SuperVerticesFastDewibbled */
    R3D_T_FAST,        /* ...VerticesFast */
    R3D_T_BLOCK,       /* ...BlockVertices */
    R3D_T_BG,          /* ...BackgroundVertices */
    R3D_T_COUNT
};
/* A transform's C (verts, count, the offset vector of FAST and BLOCK); returns the AND of the
 * outcodes. */
typedef int (*R3dTransformFn)(void *v, int n, void *o);
/* Runs transform fn of that kind (the entire level's far path instead, see R3d_FarBegin), keeping
 * its vertices unrounded; the result's outcodes against a widened viewport. */
int R3d_Transform(int kind, R3dTransformFn fn, void *v, int n, void *o);

/* ProcessPolys (fast = 0) and ProcessPolysFast (fast = 1): 1 if the native renderer drew the
 * faces, 0 if the game's C should (THPS_RENDER3D=psx, no layer, subtractive fade). */
int R3d_ProcessPolys(int fast, uint32_t *list, int count);
/* M3d_RenderSetup(camera, viewport, ot), after the game's: a 3D layer starts at ot. */
void R3d_RenderSetup(const void *viewport, struct P_TAG *ot);

/* The Dreamcast's skater shadows (Options > Graphics > Shadows, pc_video.h PC_DrawDcShadows).
 * The PS1 draws a skater's shadow as one dark quad per body part, its ellipsoid projected along
 * the light onto the ground plane under the skater. The Dreamcast draws the same quads, soft
 * (bodyshadow.pvr), into a 128x128 texture of the skater's own each frame, and lays that over the
 * level faces around the skater (projected along the light), so the shadow follows ramps, steps
 * and rails. M3dElips_RenderShadow hands each quad here: centre +- e +- f, relative to the camera
 * on the world axes; light = towards the light (4096 = 1); part = the part's centre; grey = the
 * PS1's shadow colour (128 = full, fainter higher up); key = the skater's (its mShadowPos); view =
 * the camera's view matrix (4096 = 1). 1 if taken, 0 if the game should draw the PS1's quad. */
int R3d_ShadowQuad(const void *key, const short view[3][3], const int centre[3], const int e[3],
                   const int f[3], const int light[3], const int part[3], int grey);

#ifdef __cplusplus
}
#endif

#endif
