/* Front_SwitchOnStartLevelMenu__Fv (0x8001a5b8) */
#include "names.h"

int Status;

void CommonSwitchOnMenuCode(void);

void Front_SwitchOnStartLevelMenu(void) {
	Status = 33;
	CommonSwitchOnMenuCode();
}
