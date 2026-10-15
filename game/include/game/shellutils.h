/* Front-end utility classes (SHELLUTILS.CPP / SHELLUTILS.H): Sprite, SlicedImage,
   EraseSprite, PadBuffer, VerticalMenu(+Item), VerticalDialog, FileLoader. PREManager is in
   game/pre.h.

   All derive from CClass (game/class.h; vptr at +0), as in the demo's MAIN.SYM. The "CItem" that
   front/*, rfront/memcardhiresdisplay and shellutils/* TUs declare as these classes' base
   (`virtual ~CItem() {}` + operator new 0x800241dc, vtable 0x800a2310) is CClass itself: 0x800241dc
   is CClass::operator new and 0x800a2310 is one more per-TU copy of CClass's vtable (its slot 0,
   FUN_80068aac, stores 0x800a2310 and calls CClass::operator delete, like the other ~CClass
   copies). There is no separate front-end CItem class. See build/hdr_notes/shellutils.md.

   Retail vtables: Sprite 0x800a2190, SlicedImage 0x800a21a0, EraseSprite 0x800a2180, PadBuffer
   0x800a2170, VerticalMenu 0x800a2158, VerticalDialog 0x800a2140, FileLoader 0x800a2130;
   SHELLUTILS's own CClass vtable copy is 0x800a21b0. */
#ifndef GAME_SHELLUTILS_H
#define GAME_SHELLUTILS_H

#ifndef GAME_DATA_TU /* (the data files: full debug types) */
#pragma interface /* see class.h */
#endif

extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/classsizes.h"
#include "game/types.h"
#include "game/class.h"
class PREManager;

class SlicedImage : public CClass {
  public:
	int width;                 /* 0x04 */
	int height;                /* 0x08 */
	int slice_width;           /* 0x0c */
	int slice_height;          /* 0x10 */
	ushort *slicedBuffer;      /* 0x14 (demo: char *) */
	RECT *sliceTab;            /* 0x18 */
	int num_slices_x;          /* 0x1c */
	int num_slices_y;          /* 0x20 */
	int bpp;                   /* 0x24 */
	ushort clut;               /* 0x28 */
	int is_packed;             /* 0x2c */
	int m_removed_from_memory; /* 0x30 */
	/* sizeof(SlicedImage) == 0x34 */

	SlicedImage(char *pPixels, int w, int h, int sw, int sh, int depth, ushort clutid);
	virtual ~SlicedImage();
	int pack();
	void packAt(int x, int y); /* FUN_; THPS1-only */
	void unpack();
	void removeFromMemory();
	void draw(int x, int y, int otFront, int r, int g, int b, int semiTrans, int drawNow);
	/* draw, scaled to w x h pixels (the size a Sprite is laid out at; platform/pc) */
	void drawScaled(int x, int y, int w, int h, int otFront, int r, int g, int b, int semiTrans,
	                int drawNow);
	void rotateDraw(int x, int y);
	void removeBlack();
};
CHECK_CLASS_SIZE(SlicedImage, SIZEOF_SLICEDIMAGE);

class Sprite : public CClass {
  public:
	int m_w;              /* 0x04 */
	int m_h;              /* 0x08 */
	int m_packed;         /* 0x0c */
	int m_bpp;            /* 0x10 */
	SlicedImage *m_image; /* 0x14 */
	/* sizeof(Sprite) == 0x18 */

	Sprite(char *pName, int packed, int maxw, int maxh);
	Sprite(int x, int y, int w, int h, int packed, int maxw, int maxh);
	virtual ~Sprite();
	void draw(int x, int y, int otFront, int r, int g, int b);
	void prepare(); /* FUN_; name from front/boardselect*: FUN_80063290(m_image) */
	void drawFaded(int x, int y, int otFront, int r, int g, int b);
	void pack();
	void unpack();
	void removeFromMemory();
};
CHECK_CLASS_SIZE(Sprite, SIZEOF_SPRITE);

class EraseSprite : public CClass {
  public:
	RECT mR;      /* 0x04 */
	ulong *mData; /* 0x0c */
	/* sizeof(EraseSprite) == 0x10 */

	EraseSprite(int x, int y, int w, int h);
	virtual ~EraseSprite();
	void restore(int x, int y);
};

/* d-pad repeat queue. The destructor is implicit (demo: SHELLUTILS.H; retail 0x80066524). */
class PadBuffer : public CClass {
  public:
	int entries;     /* 0x004 */
	int down;        /* 0x008 */
	int buffer[64];  /* 0x00c */
	int first;       /* 0x10c */
	int last;        /* 0x110 */
	int current;     /* 0x114 */
	uint mWhich_pad; /* 0x118: unsigned (`mPad < 2` is sltiu) */
	/* sizeof(PadBuffer) == 0x11c */

	PadBuffer(int pad);
	void update();
	int getPress();
	void reset();
};
CHECK_CLASS_SIZE(PadBuffer, SIZEOF_PADBUFFER);

struct VerticalMenuItem {
	Sprite *erase_sprite; /* 0x00 */
	char string[32];      /* 0x04 */
	int x;                /* 0x24 */
	int y;                /* 0x28 */
	int w;                /* 0x2c */
	int h;                /* 0x30 */
	int baseline;         /* 0x34 */
	int grayed;           /* 0x38 */
	int erase_sprite_x;   /* 0x3c */
};

class VerticalMenu : public CClass {
  public:
	int item_spacing;          /* 0x004 */
	int font_scale;            /* 0x008 */
	int font_scale_big;        /* 0x00c */
	int num_items;             /* 0x010 */
	int old_selected;          /* 0x014 */
	int selected;              /* 0x018 */
	VerticalMenuItem list[12]; /* 0x01c */
	int tick;                  /* 0x31c */
	Sprite *mTri1;             /* 0x320 */
	Sprite *mTri2;             /* 0x324 */
	Sprite *mEraseTri1;        /* 0x328 */
	Sprite *mEraseTri2;        /* 0x32c */
	int mDontDeleteSprites;    /* 0x330 */
	/* sizeof(VerticalMenu) == 0x334 */

	VerticalMenu(int top, int bottom, int spacing, char **items, int num, int *xlist, int *ylist,
	             int selection);
	void setString(int i, char *pString);
	virtual ~VerticalMenu();
	void setTriangles(Sprite *tri1, Sprite *tri2);
	virtual void draw();
	void erase();
	void select(int i);
	int getSelected();
	void up();
	void down();
};
CHECK_CLASS_SIZE(VerticalMenu, SIZEOF_VERTICALMENU);

class VerticalDialog : public VerticalMenu {
  public:
	/* sizeof(VerticalDialog) == 0x334 */
	VerticalDialog(int top, int bottom, int spacing, char **items, int num, int *xlist, int *ylist,
	               int selection); /* FUN_; C source */
	virtual ~VerticalDialog();
	virtual void draw();
};

/* a front-end screen's files (DAT_800be478[15]; FrontLoader_SetFiles hands one to FileLoader::setFiles)
   (THPS1-only, no demo twin) */
struct SScreenFiles {
	char *pName[32]; /* 0x00 */
	int Num;         /* 0x80 */
};

/* one file of a FileLoader set (THPS1-only, no demo twin; names from fileloader_setfiles) */
struct SLoadEntry {
	void *pData;   /* 0x00 */
	char Name[16]; /* 0x04 */
	int Handle;    /* 0x14 */
	int Type;      /* 0x18 */
	int Size;      /* 0x1c */
	int Loaded;    /* 0x20 */
};

class FileLoader : public CClass {
  public:
	int mMode;             /* 0x004 */
	int mNum;              /* 0x008 */
	int mCur;              /* 0x00c */
	SLoadEntry mEntry[32]; /* 0x010 */
	/* sizeof(FileLoader) == 0x490 */

	FileLoader(); /* FUN_; C source */
	virtual ~FileLoader(); /* FUN_; C source */
	void setFiles(char **names, int num);
	void unload(int i); /* FUN_; C source */
};

/* functions */
int Load8BitBMP2(char *file, char **out, int *pw, int *ph, unsigned short *pclut);
void DownloadBMP16x2(char *pData);
void DrawColoredBox(int x, int y, int w, int h, u_char r, u_char g, u_char b);
void SetClippingBox(int x, int y, int w, int h);
void ResetOT();
void SetOTPlayer(int a);
void PadBuffer_SetNumPlayers(int a);
void FUN_800648c0(VerticalMenu *m, int i, int on);
char **FUN_800652e4(char **p, int a, int b, int c, char **d, int e, int *f, int *g, int h);
FileLoader *FUN_80065520(FileLoader *p);
void FUN_8006553c(char **p, int f);
void FUN_80065fec(FileLoader *s);
PREManager *FUN_8006613c(PREManager *p);
void FUN_80066214(void *pm, char *name);

/* globals */
extern int Pack_NoBottom;
extern char *pBMR;
#endif
