/* Db_OTSize__Fv (0x80048c74) */
#include "names.h"

extern int GNumberOfPlayers;

int Db_OTSize(void) {
	return (GNumberOfPlayers << 12) | 2;
}
