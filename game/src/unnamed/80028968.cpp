/* _._9CChunkBit (0x80028968) */
/* C++ (BIT.CPP). Unlinks the bit from ChunkBitList; CBit's destructor does the rest. */
#include "game/bit.h"

char ChunkBitList[8];

CChunkBit::~CChunkBit() {
	DeleteFrom(ChunkBitList);
}
