/* Trig_ParseTRGFile__Fv (0x80047214) */
#include "names.h"

typedef struct {
	int x, y, z;
} V;

typedef struct {
	unsigned short us0, us2, us4, us6;
	unsigned char b8, _9, b10, _b, b12, _d, b14, _f, b16, _11, b18;
} Q;

unsigned short **OffsetList;
int NumNodes, NumTrigMenuEntries;
char DAT_800d1a10[8];

extern int UseCacheForBits;
extern unsigned short *MenuFilenamePointers[];

int GetFlag(unsigned char, unsigned char *);
int *Trig_GetPosition(V *, int);
void Trig_CreateObject(int);
void CreateCommandPoint(int, unsigned short, char *);
void *__nw__4CBitUi(int);
int CLight_ctor(void *, V *, int, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned,
                unsigned, unsigned, unsigned, unsigned);
void *__nw__6CClassUi(unsigned);
void CTrickOb_ctor(void *, unsigned short, int);
void CamPt_New(int);
void Trig_ClearTrigMenu(void);

void Trig_ParseTRGFile(void) {
	int i, j, k;
	unsigned short **e;
	unsigned short ui;
	unsigned short *b;
	short *s;
	V v;
	int *r;
	Q *q;
	void *w;
	void *it;

	for (i = 0; i < NumNodes; i++) {
		b = OffsetList[i];

		switch (*(short *)b) {
			case 7:
				GetFlag(1, (unsigned char *)b + (((short *)b)[3] * 2 + 8));
				break;
			case 1:
				if (GetFlag(1, (unsigned char *)b + (((short *)b)[3] * 2 + 8)))
					Trig_CreateObject(i);

				break;
			case 5:
				r = Trig_GetPosition(&v, i);

				if (*(unsigned short *)r == 1)
					Trig_CreateObject(i);

				break;
			case 6:
				ui = i;
				b += 1;
				b = (unsigned short *)((char *)b + (*(short *)b * 2 + 2));

				if ((int)b & 2)
					b += 1;

				CreateCommandPoint(*(int *)b, ui, (char *)b + 4);
				break;
			case 2:
			case 9:
				b += 1;
				b = (unsigned short *)((char *)b + (*(short *)b * 2 + 2));

				if ((int)b & 2)
					b += 1;

				CreateCommandPoint(*(int *)b, i, DAT_800d1a10);
				break;
			case 500:
				q = (Q *)Trig_GetPosition(&v, i);
				UseCacheForBits = 0;
				w = __nw__4CBitUi(100);

				if (w)
					CLight_ctor(w, &v, i, q->us0, q->us2, q->us4, q->us6, q->b8, q->b10, q->b12,
					            q->b14, q->b16, q->b18);

				UseCacheForBits = 1;
				break;
			case 12:
			case 14:
				it = __nw__6CClassUi(24);

				if (it)
					CTrickOb_ctor(it, i, 0);

				break;
			case 13:
				CamPt_New(i);
				break;
		}
	}

	Trig_ClearTrigMenu();

	for (j = 0; j < NumNodes; j++) {
		short *pNode = (short *)OffsetList[j];

		if (*pNode == 8) {
			char *pNodeName = (char *)Trig_GetPosition(&v, j) + 6;

			MenuFilenamePointers[NumTrigMenuEntries++] = (unsigned short *)pNodeName;
		}
	}
}
