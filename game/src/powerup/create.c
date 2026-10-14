/* PowerUp_Create__FUsRC7CVectorUiiR7CVector (0x80020b18) */
#include "names.h"

void *__nw__5CItemUi(unsigned);
void *CPowerUp_ctor(void *, unsigned short, void *, void *, unsigned, int);

void PowerUp_Create(unsigned short type, void *pos, unsigned a, int b, void *v) {
	void *p = __nw__5CItemUi(356);

	if (p)
		CPowerUp_ctor(p, type, pos, v, a, b);
}
