/* AddToFreeList__FP12SBlockHeaderi (0x8004ce54) */
#include "names.h"

typedef struct SBlockHeader {
	struct SBlockHeader *next;
	unsigned flags : 4;
	unsigned len : 28;
} SBlockHeader;

SBlockHeader *FirstFreeBlock[2];

void AddToFreeList(SBlockHeader *b, int idx) {
	SBlockHeader *cur = FirstFreeBlock[idx];
	SBlockHeader *prev = 0;

	while (cur < b && cur) {
		prev = cur;
		cur = cur->next;
	}

	if (prev) {
		if ((SBlockHeader *)((char *)prev + (prev->len + 8)) == b) {
			if ((SBlockHeader *)((char *)b + (b->len + 8)) == cur) {
				prev->next = cur->next;
				prev->len += 16 + (b->len + cur->len);
			} else {
				prev->len += 8 + b->len;
			}

			return;
		}

		prev->next = b;
	} else {
		FirstFreeBlock[idx] = b;
	}

	if ((SBlockHeader *)((char *)b + (b->len + 8)) == cur) {
		b->next = cur->next;
		b->len += 8 + cur->len;
	} else {
		b->next = cur;
	}
}
