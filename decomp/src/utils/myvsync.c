/* MyVSync__Fv (0x80047870) */
#include "names.h"

int Vblanks;
int Xblanks, DoVblankProcessing;

extern int GamePaused, GameFrozen;

int Db_Vblank(void);
void SFX_ProcessingPerFrame(void);
void Utils_VblankProcessing(void);

void MyVSync(void) {
	Vblanks++;
	Db_Vblank();
	SFX_ProcessingPerFrame();

	if (!GamePaused && !GameFrozen)
		Xblanks++;

	if (DoVblankProcessing)
		Utils_VblankProcessing();
}
