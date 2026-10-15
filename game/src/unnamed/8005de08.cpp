/* FUN_8005de08 (0x8005de08) */
/* C++ (the menu-input module's static constructor, _GLOBAL_.I): copies two words into
   DAT_800d1cc8/DAT_800d1ccc, then builds DAT_800bf104, the PadBuffer for any pad (-1) the
   front end reads (FUN_8005ddec is its teardown). */
#include "game/shellutils.h"

extern int DAT_800bf0f4[];
extern int DAT_800bf100[];

int DAT_800d1cc8 = DAT_800bf0f4[0];
int DAT_800d1ccc = DAT_800bf100[0];
PadBuffer DAT_800bf104(-1);
