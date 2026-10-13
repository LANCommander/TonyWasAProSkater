/* Panel_Trick__FP6CBrucePcii (0x8006e19c) */
#include "names.h"

void CPanelData_Trick(char *, char *, int, int);

void Panel_Trick(char *p, char *name, int score, int a3) {
	CPanelData_Trick(p + 2340, name, score, a3);
}
