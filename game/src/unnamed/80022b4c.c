/* FUN_80022b4c (0x80022b4c) */
struct E {
	char *f0;
	int p[5];
	int f24;
	int q[1];
};

extern struct E movieDetails[];
extern unsigned char DAT_800a6728[];

int FUN_8009085c(char *a, char *b);
int FUN_800913c0(char *a);

#include "pc_stream.h"

void FUN_80022b4c(void) {
	char buf[24];
	int i;

	for (i = 0; i < 26; i++) {
		if (movieDetails[DAT_800a6728[i]].f24) {
			/* PC: the file's stream (pc_stream.h) where the PS1 kept its position on the disc */
			movieDetails[DAT_800a6728[i]].f24 = PC_StreamFind(movieDetails[DAT_800a6728[i]].f0);
			continue;
			if (FUN_8009085c(buf, movieDetails[DAT_800a6728[i]].f0))
				movieDetails[DAT_800a6728[i]].f24 = FUN_800913c0(buf);
			else
				movieDetails[DAT_800a6728[i]].f24 = 0;
		}
	}
}
