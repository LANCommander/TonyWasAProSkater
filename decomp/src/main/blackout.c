/* BlackOut__Fiiii (0x80024f64) */
#include "names.h"

void Draw_Rect(int, int, int, int, int, int, int, int, int);

void BlackOut(int x, int y, int w, int h) {
	Draw_Rect(x, y, x + w, y, x + w, y + h, x, y + h, 0x40000000);
}
