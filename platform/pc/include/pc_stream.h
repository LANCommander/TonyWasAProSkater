/* Songs and movies, streamed from their files (pc_stream.cpp).
 *
 * The PS1 streamed them from the disc: the STR files interleave XA-ADPCM audio sectors with
 * sectors of MDEC video, laid out to be read at 150 sectors a second. Here a stream reads the
 * extracted file (Data/Music, Data/Movies) at that rate, on the frame clock: the audio goes to the
 * mixer's music input, the video comes out as whole frames for the game's decoder (strNext, the
 * MDEC). A Dreamcast film (.sfd, pc_sfd.h) streams its ADX audio the same way, kept a little ahead
 * of the mixer; its pictures are pc_movie.cpp's.
 *
 * The game (game/src/gamefmv and friends) keeps a stream's id where the PS1 kept
 * the file's position on the disc: the movie table's start sector and GameFMV_StartSector. Ids
 * start at 1; 0 means the file isn't there. */
#ifndef PC_STREAM_H
#define PC_STREAM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The stream for a movie-table name ("\\SUICIDE.STR;1"): its file under Data/, or the Dreamcast
 * film of it (Assets::FindFilm). 0 if there is none. */
int PC_StreamFind(const char *discName);

/* Play a stream from the start (the game's strKickCD: the start of a song, a movie, or a song
 * looping). Any frames not taken yet are dropped. */
void PC_StreamPlay(int id);
/* Stop reading (the stream is over until the next PC_StreamPlay). */
void PC_StreamStop(void);
/* Pause (the game paused, or the sound menu): no reading, no frames; Resume carries on. */
void PC_StreamPause(void);
void PC_StreamResume(void);

/* The oldest whole video frame: 1 and its MDEC bitstream and its 32-byte STR sector header (the
 * frame number is header word 2), until PC_StreamFreeFrame(frame). 0 if none is ready. */
int PC_StreamNextFrame(uint32_t **frame, uint32_t **header);
void PC_StreamFreeFrame(const void *frame);

/* The music levels (the PS1's CdMix: left to left, left to right, right to right, right to left;
 * 0x80 = 100%), and muting (the game's pause). */
void PC_StreamMix(const uint8_t levels[4]);
void PC_StreamMute(int mute);

/* GameFMV_SetStartTrack's "next song" (pc_music.cpp): sets the GameFMV_* track globals up for the next
 * playable entry of Data/Music/Playlist.m3u. 0 when there is none: the game takes its own next. */
int PC_MusicNextSong(void);

#ifdef __cplusplus
}
#endif

#endif
