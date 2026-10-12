/* Full-screen movies from the Dreamcast version's films (src/pc_movie.c). GameFMV_PlayMovie
 * (decomp/src/gamefmv/playmovie.cpp, PC branch) sets up the screen and starts the film's CD
 * stream as for an STR, whose audio hle_libcd.c plays; the pictures come from here instead of
 * the MDEC, timed to that audio. */
#ifndef PC_MOVIE_H
#define PC_MOVIE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 1 if movie-table entry `track` (GameFMV_PlayMovie's argument: ~entry) has a film here. */
int PC_MovieAvailable(int track);

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
