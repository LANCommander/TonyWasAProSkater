/* FUN_80065f1c (0x80065f1c) */
struct E {
	int a;
	int pad;
	int x;
	int y;
	int p2;
	int q;
	int r;
	int s;
	int t;
};

int strcmp(char *p, int a);

int FUN_80065f1c(char *p, int a) {
	int i;
	int r = 0;

	for (i = 0; i < *(int *)(p + 8); i++) {
		if (strcmp(p + 20 + i * 36, a) == 0) {
			r = *(int *)(p + 16 + i * 36);
			*(int *)(p + 16 + i * 36) = 0;
			*(int *)(p + 48 + i * 36) = 0;
			break;
		}
	}

	return r;
}
