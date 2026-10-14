/* TrickOb_DarkenNode__Fi (0x8004f24c) */
#include "names.h"

typedef struct Node {
	int _0;
	unsigned key;
	unsigned short id;
	unsigned char used;
	unsigned char b;
	unsigned short w;
	short _e;
	struct Node *next;
	struct Item *it;
} Node;

typedef struct Item {
	unsigned short flags;
	char _2[0x1e];
	unsigned c;
} Item;

extern int GGame;

Node *TrickObList;
unsigned char TrickPlayer;
unsigned short TrickScore;

Item *Spool_FindEnviroItem(unsigned);

void TrickOb_DarkenNode(int id) {
	Node *n;

	if (GGame == 8) {
		for (n = TrickObList; n; n = n->next) {
			if (n->id == id) {
				Item *it;

				if (n->used && GGame != 8)
					return;

				it = Spool_FindEnviroItem(n->key);
				n->w = TrickScore;
				n->it = it;
				n->b = TrickPlayer;
				it->c = 0x202020;
				it->flags |= 0x400;
				n->used = 1;
				return;
			}
		}
	}
}
