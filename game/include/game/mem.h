/* Memory manager (MEM.CPP): every block is preceded by an 8-byte SBlockHeader. */
#ifndef GAME_MEM_H
#define GAME_MEM_H

#ifndef GAME_DATA_TU /* (the data files: full debug types) */
#pragma interface /* see class.h */
#endif

#include "game/types.h"

struct SBlockHeader {
	SBlockHeader *pNext; /* 0x0: next free block; the allocation serial while in use (odd) */
	int ParentHeap : 4;  /* 0x4 bits 0..3: heap index (0/1; -2 for the two scratch blocks) */
	uint Size : 28;      /* 0x4 bits 4..31: payload size in bytes */
};

/* The header's size: 8 bytes on the PS1, 16 in a 64-bit build. */
#define MEM_HEADER ((int)sizeof(SBlockHeader))
/* A request rounded to the heap's granularity (4 bytes on the PS1, a pointer's size), which keeps
   every payload aligned for the pointers in it. */
#define MEM_ROUND(size) (((size) + (sizeof(void *) - 1)) & ~(sizeof(void *) - 1))
/* The header's ParentHeap and Size as one word (`(Size << 4) | (ParentHeap & 15)`). */
#define MEM_INFO(pBlock) (*(uint *)((char *)(pBlock) + sizeof(SBlockHeader *)))

/* where the heaps are (DAT_800be09c[2]: 0 the small heap, 1 the big one; Init_ResetMemory) */
struct SHeapRange {
	char *Start; /* 0x0 */
	char *End;   /* 0x4 */
};
extern SHeapRange DAT_800be09c[2];

/* the allocator's state (MEM.CPP) */
extern int LowMemory;
extern uint CriticalBigHeapUsage;
extern uint UniqueIndentifier;
extern int Used[2]; /* bytes in use per heap (headers included) */
extern SBlockHeader *FirstFreeBlock[2];
extern char *DAT_800d1b40; /* the scratch heap (-2): its bottom */
extern char *DAT_800d1b44; /* and top */
extern SBlockHeader *DAT_800d1b48; /* its block from the bottom */
extern SBlockHeader *DAT_800d1b4c; /* and from the top */

/* A checked reference to a heap block: the pointer and the block's serial at the time it was
   taken (Mem_GetHandle builds one; the block is still the same allocation while serials match). */
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

/* functions */
void Mem_Shrink(void *, uint);       /* 0x8004d6a0 */
SMemHandle Mem_GetHandle(void *);    /* 0x8004d7ac */
void *Mem_CheckHandle(SMemHandle *); /* 0x8004d83c */
void FUN_8004cf7c();                 /* 0x8004cf7c: the free lists over the whole heaps */
void Init_ResetMemory(int);          /* 0x80071e40 */
int Mem_GetFree(char i);             /* 0x8004d878 */

#endif
