/* CD-XA ADPCM decoder (pc_xa.cpp; the C interface is pc_xa.h). */
#pragma once

#include <stdint.h>


namespace pc {

/* One stream's decoder: the ADPCM filter history of its two channels, carried across sectors. */
class XaDecoder {
public:
    /* Most stereo frames one sector can decode to (4-bit mono: 18 groups x 8 units x 28 samples). */
    static constexpr int kMaxFrames = 4032;

    /* Clear the decoder history (start of a stream, or a jump within it). */
    void Reset();

    /* Decode one raw 2352-byte sector. Returns the number of stereo frames written to `stereo`
     * (mono is duplicated to both channels) and the sample rate in rate, or 0 if the sector is
     * not XA audio. */
    int DecodeSector(const uint8_t *raw, int16_t *stereo, int &rate);

private:
    int m_hist[2][2] = {}; /* [channel][0 = previous sample, 1 = the one before] */
};

/* The music and movie soundtracks' decoder (pc_stream.cpp). */
extern XaDecoder g_xaDecoder;

} // namespace pc
