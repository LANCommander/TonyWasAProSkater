/* _._14CGouraudRibbon (0x8002da78) */
/* C++ (BIT.CPP). Really CGouraudRibbon::~CGouraudRibbon (functions.tsv's TextScreenElement name is a
   mis-port; see build/hdr_notes/bit.md): frees the ribbon's points; CSpecialDisplay's destructor does
   the rest. */
#include "game/bit.h"

void Mem_Delete(void *);

CGouraudRibbon::~CGouraudRibbon() {
	Mem_Delete(mpPoints);
}
