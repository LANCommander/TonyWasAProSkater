/* Front_PrintGoBackSelect__Fv (0x80017a08) */
#include "names.h"

void Front_PrintGoBack(void);
void Front_PrintSelect(void);

void Front_PrintGoBackSelect(void) {
	Front_PrintGoBack();
	Front_PrintSelect();
}
