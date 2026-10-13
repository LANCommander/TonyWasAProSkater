/* PreProcessAnimPacket__FPUiT0 (0x8006ff48) */
#include "names.h"

typedef struct {
	char pad[4];
	unsigned char f4;
	char pad5;
	unsigned short flags;
	char pad8;
	unsigned char f9, fa, fb;
} STex;

typedef struct {
	int a;
	STex *t;
} SAnimFrame;

typedef struct SAnimNode {
	unsigned *data;
	struct SAnimNode *next;
} SAnimNode;

SAnimNode *AnimPackets;

unsigned *Spool_SkipPackets(unsigned *);
void *Mem_NewX(int, int, int);

void PreProcessAnimPacket(unsigned *packet, unsigned *data) {
	STex **tbl = (STex **)(Spool_SkipPackets(packet) + 1);
	SAnimNode *node = Mem_NewX(8, 0, 1);
	unsigned n, i;

	node->next = AnimPackets;
	node->data = data;
	AnimPackets = node;
	n = *data++;

	for (i = 0; i < n; i++) {
		unsigned m, j;
		SAnimFrame *f;

		data += 2;
		m = *data++;
		f = (SAnimFrame *)data;

		for (j = 0; j < m; j++) {
			f[j].t = tbl[(int)f[j].t];
			f[j].t->flags |= 0x20;
			f[j].t->f4++;
			f[j].t->f9++;
			f[j].t->fa++;
			f[j].t->fb++;
			data += 2;
		}
	}
}
