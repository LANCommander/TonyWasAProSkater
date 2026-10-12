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

#pragma interface /* see class.h */

extern "C" {
#include <sys/types.h>
#include <libgte.h>
#include <libgpu.h>
}

#include "game/types.h"
#include "game/class.h"

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

	SlicedImage(char *pPixels, int w, int h, int sw, int sh, int depth, ushort clutid); /* 0x80061ef4 */
	virtual ~SlicedImage();                                     /* slot 0: 0x800620f0 */
	int pack();                                                 /* 0x800621c0 */
	void packAt(int x, int y);                                  /* 0x800627ec (FUN_; THPS1-only) */
	void unpack();                                              /* 0x80062d00 */
	void removeFromMemory();                                    /* 0x80062de8 */
	void draw(int x, int y, int otFront, int r, int g, int b, int semiTrans, int drawNow); /* 0x80062e20 */
	void rotateDraw(int x, int y);                              /* 0x80063474 */
	void removeBlack();                                         /* 0x800637d0 */
};

class Sprite : public CClass {
  public:
	int m_w;              /* 0x04 */
	int m_h;              /* 0x08 */
	int m_packed;         /* 0x0c */
	int m_bpp;            /* 0x10 */
	SlicedImage *m_image; /* 0x14 */
	/* sizeof(Sprite) == 0x18 */

	Sprite(char *pName, int packed, int maxw, int maxh);                  /* 0x80063830 */
	Sprite(int x, int y, int w, int h, int packed, int maxw, int maxh);   /* 0x80063928 */
	virtual ~Sprite();                                                    /* slot 0: 0x80063b64 */
	void draw(int x, int y, int otFront, int r, int g, int b);            /* 0x80063be4 */
	void prepare();          /* 0x80063c48 (FUN_; name from front/boardselect*: FUN_80063290(m_image)) */
	void drawFaded(int x, int y, int otFront, int r, int g, int b);       /* 0x80063c6c */
	void pack();                                                          /* 0x80063cb4 */
	void unpack();                                                        /* 0x80063cec */
	void removeFromMemory();                                              /* 0x80063d24 */
};

class EraseSprite : public CClass {
  public:
	RECT mR;      /* 0x04 */
	ulong *mData; /* 0x0c */
	/* sizeof(EraseSprite) == 0x10 */

	EraseSprite(int x, int y, int w, int h); /* 0x80063d48 */
	virtual ~EraseSprite();                  /* slot 0: 0x80063db4 */
	void restore(int x, int y);              /* 0x80063e10 */
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

	PadBuffer(int pad); /* 0x80063e34 */
	void update();      /* 0x80063f58 */
	int getPress();     /* 0x80064274 */
	void reset();       /* 0x800642c0 */
};

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
	int item_spacing;           /* 0x004 */
	int font_scale;             /* 0x008 */
	int font_scale_big;         /* 0x00c */
	int num_items;              /* 0x010 */
	int old_selected;           /* 0x014 */
	int selected;               /* 0x018 */
	VerticalMenuItem list[12];  /* 0x01c */
	int tick;                   /* 0x31c */
	Sprite *mTri1;              /* 0x320 */
	Sprite *mTri2;              /* 0x324 */
	Sprite *mEraseTri1;         /* 0x328 */
	Sprite *mEraseTri2;         /* 0x32c */
	int mDontDeleteSprites;     /* 0x330 */
	/* sizeof(VerticalMenu) == 0x334 */

	VerticalMenu(int top, int bottom, int spacing, char **items, int num, int *xlist, int *ylist,
	             int selection);            /* 0x800642e8 */
	void setString(int i, char *pString);   /* 0x800646b0 */
	virtual ~VerticalMenu();                /* slot 0: 0x80064720 */
	virtual void draw();                    /* slot 1: 0x80064990 */
	void erase();                           /* 0x80064f9c */
	void select(int i);                     /* 0x80065120 */
	int getSelected();                      /* 0x8006516c */
	void up();                              /* 0x80065178 */
	void down();                            /* 0x80065230 */
};

class VerticalDialog : public VerticalMenu {
  public:
	/* sizeof(VerticalDialog) == 0x334 */
	VerticalDialog(int top, int bottom, int spacing, char **items, int num, int *xlist, int *ylist,
	               int selection);          /* 0x800652e4 (FUN_; C source) */
	virtual ~VerticalDialog();              /* slot 0: 0x80065340 */
	virtual void draw();                    /* slot 1: 0x80065394 */
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

	FileLoader();                         /* 0x80065520 (FUN_; C source) */
	virtual ~FileLoader();                /* slot 0: 0x8006553c (FUN_; C source) */
	void setFiles(char **names, int num); /* 0x80065594 */
	void unload(int i);                   /* 0x80065e04 (FUN_; C source) */
};

#endif
