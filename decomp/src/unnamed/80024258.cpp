/* FUN_80024258 (0x80024258) */
/* CalcPolyBufferEnd: the demo's MAIN.CPP twin used a fixed POLYBUFFERSIZE; THPS1 sizes the
   primitive buffer per level and per player count. */
struct SDoubleBuffer {
	char pad0[0x80a8];
	unsigned long Prims; /* 0x80a8 */
};

struct SLevel {
	char pad0[0x50];
	int PolyBufferExtra[2]; /* 0x50, indexed by number of players - 1 */
	char pad58[120 - 0x58];
};

extern SDoubleBuffer *pDoubleBuffer; /* pDoubleBuffer */
extern int GLevel;            /* GLevel */

int GNumberOfPlayers; /* number of players */

extern SLevel Levels[]; /* Levels */

unsigned long PolyBufferEnd; /* PolyBufferEnd */

#define POLYBUFFERSIZE (Levels[GLevel].PolyBufferExtra[GNumberOfPlayers - 1] + 0x17c00)

void FUN_80024258(void) {
	unsigned long Prims = pDoubleBuffer->Prims;
	int Size = POLYBUFFERSIZE;

	PolyBufferEnd = (Prims + Size - 256) & 0x7fffffff;
}
