/* Debug_RunAnim3__6CBruceiiiii (0x8002fde4) */
#include "names.h"
void CBruce_MaybeFlipAnimationTransition(int a);
void CSuper_RunAnim(int a, int b, int c, int d, int e);

void CBruce_Debug_RunAnim3(int a, int b, int c, int d, int e, int line) {
	CBruce_MaybeFlipAnimationTransition(a);
	CSuper_RunAnim(a, b, c, d, e);
}
