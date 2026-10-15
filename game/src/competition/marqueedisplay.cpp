/* Competition_MarqueeDisplay__Fv (0x800764d8) */
#include "game/competition.h"
#include "game/mess.h"

void Competition_MarqueeDisplay() {
	if (theCompetition.mMarqueeText) {
		Mess_SetRGB(122, 122, 122, 0);
		Mess_SetScale(256);
		Mess_SetTextJustify(1);

		if (theCompetition.mMarqueeTimer > 768)
			theCompetition.mMarqueeTimer = 0;

		Mess_DrawText(512 - theCompetition.mMarqueeTimer, 200, theCompetition.mMarqueeText, 0, 0, 256, 0);

		theCompetition.mMarqueeTimer += 5;
	}
}
