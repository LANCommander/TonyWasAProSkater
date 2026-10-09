/* CD-XA ADPCM audio sectors (the music and movie soundtracks in the STR files) to PCM. */
#ifndef PC_XA_H
#define PC_XA_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Most stereo frames one sector can decode to (4-bit mono: 18 groups x 8 units x 28 samples). */
#define PC_XA_MAX_FRAMES 4032

/* Clear the decoder history (start of a stream, or a jump within it). */
void PC_XaReset(void);

/* Decode one raw 2352-byte sector. Returns the number of stereo frames written to `stereo`
 * (mono is duplicated to both channels) and the sample rate in *rate, or 0 if the sector is
 * not XA audio. */
int PC_XaDecodeSector(const uint8_t *raw, int16_t *stereo, int *rate);

#ifdef __cplusplus
}
#endif

#endif
