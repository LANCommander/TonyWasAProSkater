/* FUN_8006613c (0x8006613c) */
typedef struct {
	unsigned char b;
	char pad[15];
} E;

typedef struct {
	char *vt;
	int a[16];
	E e[16];
} X;

extern char DAT_800a2120[];

char *strcpy(char *, const char *);

X *FUN_8006613c(X *p) {
	int i;

	p->vt = DAT_800a2120;

	for (i = 0; i < 16; i++) {
		p->a[i] = 0;
		strcpy((char *)&p->e[i].b, "");
	}

	return p;
}
