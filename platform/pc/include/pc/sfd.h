/* The Dreamcast version's films (.sfd, CRI Sofdec): an MPEG-1 program stream with MPEG-1 video
 * (stream 0xE0) and CRI ADX audio (stream 0xC0) instead of MPEG audio. The songs are 96x64 video
 * with 44.1 kHz stereo ADX, the movies 320x240 with 44.1 kHz stereo or 22.05 kHz mono ADX.
 * src/pc_sfd.cpp demultiplexes them and decodes the ADX; pl_mpeg decodes the video (pc_movie.cpp).
 * pc_sfd.h is the old C interface to the same class. */
#pragma once

#include <stdint.h>
#include <stdio.h>

#include <memory>

namespace pc {

class SfdFilm {
public:
    /* Video elementary-stream bytes, in stream order, as the demultiplexer meets them. */
    using VideoFn = void (*)(void *ctx, const uint8_t *data, int len);

    /* Open a film: null if it can't be read or isn't a program stream. */
    static std::unique_ptr<SfdFilm> Open(const char *path);
    /* Length of a film's audio in seconds (0 if none or unreadable). */
    static int AudioSeconds(const char *path);

    ~SfdFilm();
    SfdFilm(const SfdFilm &) = delete;
    SfdFilm &operator=(const SfdFilm &) = delete;

    /* Where the video goes (null: dropped, the default). With audio off the ADX is dropped too
     * (a reader that only wants the pictures). */
    void SetVideo(VideoFn fn, void *ctx) {
        m_video = fn;
        m_videoCtx = ctx;
    }
    void SetAudio(bool on) { m_audioOn = on; }

    /* Demultiplex the next packet: false at the end of the file. */
    bool Demux();

    /* Up to max decoded stereo frames (mono is duplicated) and their rate: 0 once the audio has
     * ended (or the film has none). Demultiplexes as far as it needs to. */
    int Audio(int16_t *stereo, int max, int *rate);

    /* Start over from the beginning, or from the first pack at or after byte pos. */
    void Seek(long long pos);
    /* Bytes of the file demultiplexed so far, and the file's size. */
    long long Tell() const { return m_bufBase + m_bufPos; }
    long long Size() const { return m_size; }

private:
    static constexpr int kHdrMax = 1024;
    static constexpr int kAdxMaxCh = 2;

    SfdFilm() = default;

    bool Need(int n);
    void ParseAdxHeader();
    void AudioBytes(const uint8_t *p, int n);
    void Payload(int id, const uint8_t *b, int len);
    void Restart();

    FILE *m_f = nullptr;
    long long m_size = 0;
    std::unique_ptr<uint8_t[]> m_buf;
    int m_bufLen = 0, m_bufPos = 0;
    long long m_bufBase = 0;          /* file offset of m_buf[0] */
    bool m_ended = false;
    VideoFn m_video = nullptr;
    void *m_videoCtx = nullptr;
    bool m_audioOn = true;

    /* audio stream */
    uint8_t m_hdr[kHdrMax] = {};      /* its first bytes: the ADX header */
    int m_hdrLen = 0;
    long long m_audioBytes = 0;       /* audio-stream bytes seen */
    bool m_adxReady = false, m_adxEnded = false;
    int m_dataStart = 0, m_frameBytes = 0, m_chFrame = 0, m_channels = 0, m_rate = 0;
    uint32_t m_total = 0, m_done = 0; /* samples per channel: in the stream, decoded */
    int m_c1 = 0, m_c2 = 0, m_hist[kAdxMaxCh][2] = {};
    long long m_keepFrom = 0;         /* audio offset from which bytes are queued */
    long long m_skipTo = 0;           /* seeking: demultiplex to this file offset without queueing */
    uint8_t *m_q = nullptr;           /* the queued ADX bytes (realloc'd as it grows) */
    int m_qLen = 0, m_qCap = 0;
};

} // namespace pc
