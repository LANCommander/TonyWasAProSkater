/* Ob_QuickAI__FPP5CBody (0x80044620) */
/* C++ (OB.CPP). Fuzzy demo twin; locals from its MAIN.SYM. Runs AI on every body in the list;
   bodies flagged 0x40 are deleted on the second pass (0x80 marks the first). */
#include "game/ob.h"

void Ob_QuickAI(CBody **ppBody) {
	CBody *pBody;
	CBody *pNext;

	/* the exit test loads pNext (probably a list-walk macro), so cc1 can't rotate it into a
	   simple bottom test and `delete` keeps its null check */
	for (pBody = *ppBody; pBody ? (pNext = (CBody *)pBody->mNextItem, 1) : 0; pBody = pNext) {
		if (pBody->mCBodyFlags & 0x40) {
			if (pBody->mCBodyFlags & 0x80)
				delete pBody;
			else
				pBody->mCBodyFlags |= 0x80;
		} else {
			pBody->AI();
		}
	}
}
