/* Card_SetHeader__FP9SCardHeadiPcT2 (0x80014300) */
#include "names.h"

extern int GSaveAction;
extern char CARD_GTITLE[];
extern char CARD_VTITLE[];

void FUN_8008e904(char *, char *);

typedef struct {
	char sig[4];
	char title[64];
	char pad[28];
	char name[32];
	char d1[128];
	char d2[128];
	char d3[128];
} SCardHead;

void Card_SetHeader(SCardHead *h, int icon, char *name, char *data) {
	int i;

	h->sig[0] = 83;
	h->sig[1] = 67;
	h->sig[2] = 17;
	h->sig[3] = icon;

	for (i = 0; i < 64; i++)
		h->title[i] = 0;

	if (GSaveAction == 1)
		FUN_8008e904(h->title, CARD_GTITLE);
	else
		FUN_8008e904(h->title, CARD_VTITLE);

	for (i = 0; i < 28; i++)
		h->pad[i] = 0;

	for (i = 0; i < 32; i++)
		h->name[i] = name[i];

	for (i = 0; i < 128; i++) {
		h->d1[i] = data[i];
		h->d2[i] = data[i];
		h->d3[i] = data[i];
	}
}
