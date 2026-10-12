/* Full-screen movies. The PS1's STR movies play through the game's own code (GameFMV_PlayMovie,
 * with the MDEC in pc_mdec.c), which only needs PC_MovieWait in its busy-waits.
 *
 * The Dreamcast version's films (src/pc_movie.c): GameFMV_PlayMovie
 * (decomp/src/gamefmv/playmovie.cpp, PC branch) sets up the screen and starts the film's CD
 * stream as for an STR, whose audio hle_libcd.c plays; the pictures come from here instead of
 * the MDEC, timed to that audio. */
#ifndef PC_MOVIE_H
#define PC_MOVIE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 1 if movie-table entry `track` (GameFMV_PlayMovie's argument: ~entry) has its file here (an STR
 * or a Dreamcast film). */
int PC_MovieAvailable(int track);

/* An STR movie (the game's own player, with pc_mdec.c): wait a vblank for the CD stream to
 * deliver the next frame. 0 once the stream has stopped with no frame left to come. */
int PC_MovieWait(void);

/* Open the film the CD stream plays (mounted at GameFMV_StartSector): 0 if it isn't one. */
int PC_MovieOpen(void);
/* Put the picture that is due in VRAM at (0, 256) as 24-bit pixels, 320 wide, and wait for the
 * next vblank (which shows it). 0 once the film has ended. */
int PC_MovieFrame(void);
void PC_MovieClose(void);

#ifdef __cplusplus
}
#endif

#endif
