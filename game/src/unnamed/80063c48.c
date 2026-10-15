/* prepare__6Sprite (0x80063c48) */
#include "names.h"

/* Sprite (game/shellutils.h) */
struct Sprite {
	void *vt;
	int m_w, m_h, m_packed, m_bpp;
	struct SlicedImage *m_image;
};

int FUN_80063290(struct SlicedImage *);

int Sprite_prepare(struct Sprite *t) {
	return FUN_80063290(t->m_image);
}
