/* Debug_RunAnim1__6CBruceiii (0x8002fd28) */
#include "names.h"
void CBruce_MaybeFlipAnimationTransition(int a);
void CSuper_RunAnim(int a, int b, int c, int d, int e);

void CBruce_Debug_RunAnim1(int a, int b, int c, int line) {
	CBruce_MaybeFlipAnimationTransition(a);
	CSuper_RunAnim(a, b, c, -1, -1);
}
