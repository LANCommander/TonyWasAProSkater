/* Debug_RunAnim2__6CBruceiiii (0x8002fd80) */
#include "names.h"
void CBruce_MaybeFlipAnimationTransition(int a);
void CSuper_RunAnim(int a, int b, int c, int d, int e);

void CBruce_Debug_RunAnim2(int a, int b, int c, int d, int line) {
	CBruce_MaybeFlipAnimationTransition(a);
	CSuper_RunAnim(a, b, c, d, -1);
}
