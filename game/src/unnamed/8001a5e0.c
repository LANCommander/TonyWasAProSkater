/* Front_PrintHorseLetters__Fv (0x8001a5e0) */
#include "names.h"

extern int Status;

void CommonSwitchOnMenuCode(void);

void Front_PrintHorseLetters(void) {
	Status = 34;
	CommonSwitchOnMenuCode();
}
