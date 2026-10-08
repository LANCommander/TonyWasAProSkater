/* Debug_RunAnim__6CBruceii (0x8002fcdc) */
#include "names.h"
void CBruce_MaybeFlipAnimationTransition(int a);
void CSuper_RunAnim(int a, int b, int c, int d, int e);

void CBruce_Debug_RunAnim(int a, int b, int line) {
	CBruce_MaybeFlipAnimationTransition(a);
	CSuper_RunAnim(a, b, 0, -1, -1);
}
