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

#endif
