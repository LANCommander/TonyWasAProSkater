/* FUN_800601fc (0x800601fc) */
#include "names.h"

/* GameText (LocalText): pLoadComplete, pLoadFailed */
typedef struct {
	char *p[0xcc / 4];
	char *pLoadComplete;
	char *pSaveComplete;
	char *pLoadFailed;
} Q;

extern Q GameText;

int Front_MemCardScreen(void);
void FUN_8005e4ec(char **p, int a, int b);

int FUN_800601fc(void) {
	int r = Front_MemCardScreen();
	char *v;

	if (r != -1) {
		if (r == 19) {
			v = GameText.pLoadComplete;
			FUN_8005e4ec(&v, 1, 60);

			r = 0;
		} else if (r == 0) {
			v = GameText.pLoadFailed;
			FUN_8005e4ec(&v, 1, -1);

			r = -1;
		}
	}

	return r;
}
