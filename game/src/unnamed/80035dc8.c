/* arccos__Fi (0x80035dc8) */
#include "names.h"

typedef struct {
	int w[85];
} A;

typedef struct {
	int w[11];
} B;

extern A DAT_800a5244, DAT_800a7f40;
extern B DAT_800a7e2c, DAT_800a8094, DAT_800a7e58, DAT_800a80c0, DAT_800a7e84, DAT_800a80ec;

int arccos(int a) {
	if (a == 0x2253c038) {
		DAT_800a5244 = DAT_800a7f40;
		DAT_800a7e2c = DAT_800a8094;
		DAT_800a7e58 = DAT_800a80c0;
		DAT_800a7e84 = DAT_800a80ec;
	}

	return 0;
}
