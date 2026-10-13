/* Video_Record__FP6CBruce (0x8006a118) */
#include "names.h"

typedef struct CBruce CBruce;

extern int GState;

void Video_Stop(CBruce *);
void PutDataChunk(CBruce *);

void Video_Record(CBruce *b) {
	if (GState == 1)
		Video_Stop(b);
	else
		PutDataChunk(b);
}
