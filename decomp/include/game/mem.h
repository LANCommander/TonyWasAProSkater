/* Memory manager (MEM.CPP): every block is preceded by an 8-byte SBlockHeader. */
#ifndef GAME_MEM_H
#define GAME_MEM_H

#pragma interface /* see class.h */

#include "game/types.h"

struct SBlockHeader {
	SBlockHeader *pNext; /* 0x0: next free block; the allocation serial while in use (>= 0) */
	int ParentHeap : 4;  /* 0x4 bits 0..3: heap index (0/1; -2 for the two scratch blocks) */
	uint Size : 28;      /* 0x4 bits 4..31: payload size in bytes */
};

/* A checked reference to a heap block: the pointer and the block's serial at the time it was
   taken (FUN_8004d7ac builds one; the block is still the same allocation while serials match). */
struct SMemHandle {
	void *p;    /* 0x0 */
	int Serial; /* 0x4 */
};

void Bang(uchar, uchar, uchar);                           /* 0x8004ce28 */
void AddToFreeList(SBlockHeader *, int);                  /* 0x8004ce54 */
void *Mem_NewX(uint size, int heap, int assertOnFailure); /* 0x8004d068 */
void *Mem_New(uint size, int heap, int assertOnFailure);  /* the demo's name for 0x8004d068 */
void *Mem_NewTop(uint size);                              /* 0x8004d3a4 */
void Mem_Delete(void *);                                  /* 0x8004d5d0 */
void Mem_Copy(void *, void *, int);                       /* 0x8004d8b4 */

#endif
