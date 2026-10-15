/* Sound (pc_sound.cpp; the class is pc/sound.h): sound banks decoded to PCM, 24 channels with
 * pitch, volume and an envelope, a reverb, the music input, mixed to 44.1 kHz stereo on an SDL
 * audio device.
 *
 * The game's sound code (game/src/sfx and friends) works in the terms it was
 * written in: a bank's samples are found by their offset in the bank's sample data (the VAB's
 * sizes table), sounds play on 24 numbered channels that it allocates itself, and a channel is
 * free again once its envelope level reads 0. Pitches are 0x1000 for a sample's own rate, volumes
 * 0x4000 for full (negative inverts the phase), envelopes the PS1's ADSR words, as the VAB tones
 * give them. */
#ifndef PC_SOUND_H
#define PC_SOUND_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- banks ---- */

/* Drop every bank (the game is about to load its banks again); channels stop. */
void PC_SoundBanksReset(void);
/* A VAB's sample data (the PS1's ADPCM, decoded here). Returns the bank's base: a sample is
 * base + its byte offset in the data. 0 if it can't be loaded. */
uint32_t PC_SoundBankLoad(const void *data, uint32_t size);

/* ---- channels ---- */

/* What channel ch plays at its next key on: the sample at `sample` (a bank base + offset). */
void PC_SoundChannelSet(int ch, uint32_t sample, int pitch, int volL, int volR, uint16_t adsr1,
                        uint16_t adsr2);
void PC_SoundChannelVolume(int ch, int volL, int volR);
void PC_SoundChannelPitch(int ch, int pitch);
int PC_SoundChannelGetPitch(int ch);
/* The envelope's level, 0..0x7fff: 0 once the sound has ended or been released to silence. */
int PC_SoundChannelLevel(int ch);
/* Start the channels in the mask from the beginning, or release them (their envelopes fade). */
void PC_SoundKeyOn(uint32_t channels);
void PC_SoundKeyOff(uint32_t channels);
/* The pitch for a note (60 = middle C, fine in 1/128) of a sample centred on centre/centreFine. */
int PC_SoundNotePitch(int note, int fine, int centre, int centreFine);

/* ---- mix ---- */

/* The sound effects' and the music's levels (0x4000 = full). */
void PC_SoundLevels(int sfxL, int sfxR, int musicL, int musicR);
/* Reverb: the PS1's modes (0 off, 1 room, 2-4 studio small/medium/large, 5 hall, 6 space echo,
 * 7 echo, 8 delay, 9 pipe), its depth (0x7fff full; a new mode starts at 0), and which channels
 * are sent to it. 0 if the mode is unknown. */
int PC_SoundReverbMode(int mode);
void PC_SoundReverbDepth(int left, int right);
void PC_SoundReverbChannels(int on, uint32_t channels);

#ifdef __cplusplus
}
#endif

#endif
