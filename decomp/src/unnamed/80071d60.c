/* FUN_80071d60 (0x80071d60) */
#include "names.h"
#ifdef PC_PORT
#include "pc_frame.h"
#include "pc_screen.h"
#include "pc_texstore.h"
#include "pc_draw.h"
#endif

typedef struct {
	short x, y, w, h;
} R;

extern char Vertices[];
extern char *ProjectedVertices;
extern char *RotatedVertices;
extern R DAT_800d1f8c[];

void FUN_80047870(void);
int FUN_80096b84(void);
void FUN_80014d5c(int a);
void Pad_InitAtStart(void);
void FUN_800971a4(void);
void FUN_8008c1a0(int a);
void FUN_8008c314(int a);
void FUN_800491a0(void);
void ClearImage(R *r, int a, int b, int c);
void FUN_80096c14(void (*a)(void));
void FUN_8008c3d0(int a);
void FUN_800785cc(int a);
void SFX_InitAtStart(void);
void FileIO_Init(void);
void FUN_80022b4c(void);

void FUN_80071d60(void) {
	R r;

	ProjectedVertices = Vertices;
	RotatedVertices = Vertices + 3200;
#ifndef PC_PORT /* PC: no interrupt callbacks to reset; the frame handler is set below */
	FUN_80096b84();
#endif
	FUN_80014d5c(1);
	Pad_InitAtStart();
	FUN_800971a4();
#ifdef PC_PORT
	PC_DrawReset();
	PC_ScreenReset();
#else
	FUN_8008c1a0(0);
#endif
#ifndef PC_PORT /* PC: no libgpu debug level */
	FUN_8008c314(0);
#endif
	FUN_800491a0();

	r = DAT_800d1f8c[0];
#ifdef PC_PORT
	/* all of the PS1's VRAM: the screens and the textures */
	PC_ScreenFill(r.x, r.y, r.w, r.h, 0);
	PC_TexFill(r.x, r.y, r.w, r.h, 0);
	PC_SetFrameHandler(FUN_80047870); /* MyVSync: once a frame (pc_frame.h) */
	PC_ScreenEnable(1);
#else
	ClearImage(&r, 0, 0, 0);
	FUN_80096c14(FUN_80047870);
	FUN_8008c3d0(1);
#endif
	FUN_800785cc(0x9f6ad);
	SFX_InitAtStart();
	FileIO_Init();
	FUN_80022b4c();
}
