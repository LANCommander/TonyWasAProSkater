/* The PS1's scratchpad: 1 KB of fast RAM at 0x1f800000 that the game uses as working memory (vertex
   caches, clipping and subdivision buffers, the renderer's shared words). In the port it is a
   native buffer, Scratchpad (platform/pc/src/pc_main.c); the game reaches it by offset from
   SCRATCHPAD. */
#ifndef GAME_SCRATCHPAD_H
#define GAME_SCRATCHPAD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
extern unsigned char Scratchpad[];
#ifdef __cplusplus
}
#endif

#define SCRATCHPAD ((uintptr_t)Scratchpad)

/* A pointer the game keeps in the scratchpad, by its retail offset: in the port it has a slot of
   its own in ScratchPointers (at its retail offset, a pointer would overlap the word after it in a
   64-bit build; and the scratchpad's buffers run past the PS1's 1 KB). */
#ifdef __cplusplus
extern "C" {
#endif
extern uintptr_t ScratchPointers[];
#ifdef __cplusplus
}
#endif
#define SCRATCH_PTR(o) (((volatile uintptr_t *)ScratchPointers)[(o) >> 2])

#endif
