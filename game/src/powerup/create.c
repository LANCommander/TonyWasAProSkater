/* PowerUp_Create__FUsRC7CVectorUiiR7CVector (0x80020b18) */
#include "game/classsizes.h"
#include "names.h"

void *__nw__5CItemUi(unsigned);
void *CPowerUp_ctor(void *, unsigned short, void *, void *, unsigned, int);

/* the new power-up (0 if there was no memory) */
void *PowerUp_Create(unsigned short type, void *pos, unsigned a, int b, void *v) {
	void *p = __nw__5CItemUi(SIZEOF_CPOWERUP);

	if (p)
		CPowerUp_ctor(p, type, pos, v, a, b);

	return p;
}
