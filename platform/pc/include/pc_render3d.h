/* Native 3D renderer for the world and models (src/pc_render3d.c).
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

#ifdef __cplusplus
extern "C" {
#endif

/* After the OpenGL backend started: gl = 1 enables the native renderer (unless THPS_RENDER3D=psx). */
void R3d_Init(int gl);
int R3d_Native(void);

/* PC_DrawOT, for each packet before it is drawn: 1 if it was a layer marker
 * (its words are not GPU commands). R3d_ChainEnd at the end of the chain. */
int R3d_ChainPacket(const uint32_t *p, uint32_t len);
void R3d_ChainEnd(void);
/* THPS_R3D_DUMP: the faces of the layer drawn last, to a text file (debugging). */
void R3d_DumpLast(const char *path);

/* Drawing the entire level (Options > Graphics > Draw Distance, pc_video.h; M3d_Render): the PS1
 * transforms work in 16-bit view space, so items beyond it are drawn from here. Between these,
 * the item transform (M3dAsm_TransformAndOutcodeItemVertices) works in double precision: view =
 * m * vertex + t (m: the view matrix times the item's rotation, t: the item's view-space
 * position), with no far plane; its faces are fogged on their true depth (fog from fogstart,
 * INT_MAX for none, with fogshift as DpqShift). */
void R3d_FarBegin(const double m[3][3], const double t[3], int fogstart, int fogshift);
void R3d_FarEnd(void);

#ifdef __cplusplus
}
#endif

#endif
