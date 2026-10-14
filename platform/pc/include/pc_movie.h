/* Full-screen movies, shown as a picture of their own (pc_screen.h). The PS1's STR movies play
 * through the game's own code (GameFMV_PlayMovie): it decodes each frame's bitstream and has the
 * picture decoded here (PC_MovieDecode, pc_vdec.h) and shown (PC_MovieShow).
 *
 * The Dreamcast version's films (src/pc_movie.c): GameFMV_PlayMovie
 * (decomp/src/gamefmv/playmovie.cpp, PC branch) sets up the screen and starts the film's
 * stream as for an STR, whose audio pc_stream.c plays; the pictures come from here instead of
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

/* An STR movie (the game's own player): wait a frame for the stream to deliver the next video
 * frame. 0 once the stream has stopped with no frame left to come. */
int PC_MovieWait(void);

/* An STR movie's screen: black, 320x240 pixels of 24 bits (rgb24) or 15. PC_MovieDecode decodes a
 * frame (runlevel: the game's VLC output) into it at x (halfwords, as in the PS1's screen buffer),
 * y, its columns to xend, h high; PC_MovieShow shows the screen from the next frame on;
 * PC_MovieEnd goes back to the game's screen. */
void PC_MovieBegin(int rgb24);
void PC_MovieDecode(const void *runlevel, int x, int y, int xend, int h);
void PC_MovieShow(void);
void PC_MovieEnd(void);

/* Open the film the stream plays (GameFMV_StartSector, pc_stream.h): 0 if it isn't one. */
int PC_MovieOpen(void);
/* Show the picture that is due and wait for the next frame (which shows it). 0 once the film has
 * ended. */
int PC_MovieFrame(void);
void PC_MovieClose(void);   /* and back to the game's screen */

#ifdef __cplusplus
}
#endif

#endif
