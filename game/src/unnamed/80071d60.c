/* FUN_80071d60 (0x80071d60) */
#include "names.h"
#include "pc_frame.h"
#include "pc_screen.h"
#include "pc_texstore.h"
#include "pc_draw.h"

typedef struct {
	short x, y, w, h;
} R;

extern char Vertices[];
extern char *ProjectedVertices;
extern char *RotatedVertices;
extern R DAT_800d1f8c;

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
	FUN_80014d5c(1);
	Pad_InitAtStart();
	FUN_800971a4();
	PC_DrawReset();
	PC_ScreenReset();
	FUN_800491a0();

	r = DAT_800d1f8c;
	/* all of the PS1's VRAM: the screens and the textures */
	PC_ScreenFill(r.x, r.y, r.w, r.h, 0);
	PC_TexFill(r.x, r.y, r.w, r.h, 0);
	PC_SetFrameHandler(FUN_80047870); /* MyVSync: once a frame (pc_frame.h) */
	PC_ScreenEnable(1);
	FUN_800785cc(0x9f6ad);
	SFX_InitAtStart();
	FileIO_Init();
	FUN_80022b4c();
}
