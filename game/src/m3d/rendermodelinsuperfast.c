/* RenderModelInSuperFast__FP6SModel (0x80084f78) */
typedef struct {
	short _0;
	unsigned short n;
	unsigned short a, b;
	char _8[0x14];
	char verts[1];
} SModel;

int Dewibble, SubObjectMasked;

int FUN_80086404(char *, int);
int FUN_800862d4(char *, int);
void FUN_800898ac(char *, char *, int);

void RenderModelInSuperFast(SModel *m) {
	int n = m->n;
	char *p = (char *)m + 28;
	int r;

	if (Dewibble)
		r = FUN_80086404(p, n);
	else
		r = FUN_800862d4(p, n);

	p += n * 8;

	if (SubObjectMasked == 0 && (r & 0x60006000) == 0)
		FUN_800898ac(p + *(unsigned short *)((char *)m + 4) * 8, p,
		             *(unsigned short *)((char *)m + 6));
}
