/* FUN_80068f40 (0x80068f40) */
extern int GVInfo1[];

int TrickOb1;
int TrickOb2;

void FUN_80068f40(int a, int b) {
	if (GVInfo1[0] == 1) {
		switch (a) {
			case 1:
				TrickOb1 = b;
				break;
			case 2:
				TrickOb2 = b;
				break;
		}
	}
}
