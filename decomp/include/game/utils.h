/* Odds and ends of UTILS.CPP and DB.CPP's render call: ground height, the explosion stack,
   pausing. */
#ifndef GAME_UTILS_H
#define GAME_UTILS_H

#include "game/types.h"
#include "game/vector.h"

/* functions */
void Pause(int n);                                             /* 0x800478f4 */
void Utils_ClearExplosionStack();                              /* 0x80048044 */
int Utils_GetGroundHeight(const CVector &p, int up, int down); /* 0x800481e8 */
void Db_Render();                                              /* 0x80049094 */

#endif
