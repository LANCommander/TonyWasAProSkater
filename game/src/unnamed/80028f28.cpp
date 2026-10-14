/* _._5CGlow (0x80028f28) */
/* C++ (BIT.CPP). Frees the sections (the fringes share their allocation) and unlinks the glow from
   GlowList; CBit's destructor does the rest. */
#include "game/bit.h"

char GlowList[8];

void Mem_Delete(void *);

CGlow::~CGlow() {
	Mem_Delete(mpSections);
	DeleteFrom(GlowList);
}
