/* The Dreamcast version's films (.sfd, CRI Sofdec): an MPEG-1 program stream with MPEG-1 video
 * (stream 0xE0) and CRI ADX audio (stream 0xC0) instead of MPEG audio. The songs are 96x64 video
 * with 44.1 kHz stereo ADX, the movies 320x240 with 44.1 kHz stereo or 22.05 kHz mono ADX.
 * src/pc_sfd.c demultiplexes them and decodes the ADX; pl_mpeg decodes the video (pc_movie.c). */
#ifndef PC_SFD_H
#define PC_SFD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PcSfd PcSfd;

/* Video elementary-stream bytes, in stream order, as the demultiplexer meets them. */
typedef void (*PC_SfdVideoFn)(void *ctx, const uint8_t *data, int len);

/* Open a film: NULL if it can't be read or isn't a program stream. */
PcSfd *PC_SfdOpen(const char *path);
void PC_SfdClose(PcSfd *s);

/* Where the video goes (NULL: dropped, the default). With audio off the ADX is dropped too
 * (a reader that only wants the pictures). */
void PC_SfdSetVideo(PcSfd *s, PC_SfdVideoFn fn, void *ctx);
void PC_SfdSetAudio(PcSfd *s, int on);

/* Demultiplex the next packet: 0 at the end of the file. */
int PC_SfdDemux(PcSfd *s);

/* Up to max decoded stereo frames (mono is duplicated) and their rate: 0 once the audio has
 * ended (or the film has none). Demultiplexes as far as it needs to. */
int PC_SfdAudio(PcSfd *s, int16_t *stereo, int max, int *rate);

/* Start over from the beginning, or from the first pack at or after byte pos. */
void PC_SfdSeek(PcSfd *s, long long pos);
/* Bytes of the file demultiplexed so far, and the file's size. */
long long PC_SfdTell(const PcSfd *s);
long long PC_SfdSize(const PcSfd *s);

/* Length of a film's audio in seconds (0 if none or unreadable). */
int PC_SfdAudioSeconds(const char *path);

#ifdef __cplusplus
}
#endif

#endif
