/* Graffiti_Stats__FP6CBruceiiiiiiiii (0x80017abc) */
/* C++. The graffiti game's scoreboard of one player: labels, then the skater's tags and steals
   and the high score, wins, losses and streak the caller passes. */
#include "game/bruce.h"
#include "game/mess.h"
#include "game/psyq.h"

void Graffiti_Stats(CBruce *b, int x, int y, int r, int g, int bl, int v1, int v2, int v3, int v4) {
	char buf[16];

	Mess_SetRGB(r, g, bl, 0);
	Mess_SetTextJustify(1);
	Mess_DrawText(x + 20, y, "TAGS", 0, 0, 256, 0);
	Mess_DrawText(x + 20, y + 17, "STEALS", 0, 0, 256, 0);
	Mess_DrawText(x + 20, y + 51, "HIGH", 0, 0, 256, 0);
	Mess_DrawText(x + 20, y + 68, "WINS", 0, 0, 256, 0);
	Mess_DrawText(x + 20, y + 85, "LOSSES", 0, 0, 256, 0);
	Mess_DrawText(x + 20, y + 102, "STREAK", 0, 0, 256, 0);
	Mess_SetTextJustify(2);
	sprintf(buf, "%d", b->mGraffitiCount);
	Mess_DrawText(x + 136, y, buf, 0, 0, 256, 0);
	sprintf(buf, "%d", b->mGraffitiSteals);
	Mess_DrawText(x + 136, y + 17, buf, 0, 0, 256, 0);
	sprintf(buf, "%d", v4);
	Mess_DrawText(x + 136, y + 51, buf, 0, 0, 256, 0);
	sprintf(buf, "%d", v1);
	Mess_DrawText(x + 136, y + 68, buf, 0, 0, 256, 0);
	sprintf(buf, "%d", v2);
	Mess_DrawText(x + 136, y + 85, buf, 0, 0, 256, 0);
	sprintf(buf, "%d", v3);
	Mess_DrawText(x + 136, y + 102, buf, 0, 0, 256, 0);
}
