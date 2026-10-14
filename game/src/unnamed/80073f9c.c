/* FUN_80073f9c (0x80073f9c) */
struct S {
	char pad[12];
	int f;
	char q[104];
};

extern struct S Levels[];
extern unsigned char GCareer_c[];
extern char *GSkater;

int FUN_80073f9c(void) {
	int c = 0;
	int i = 0;
	char *g = GSkater;

	for (; i < 10; i++) {
		if (Levels[i].f) {
			if (GCareer_c[i + *(int *)(g + 744) * 14] & 224)
				c++;
		}
	}

	return c == 3;
}
