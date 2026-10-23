/* Songs and movies streamed from their files (see pc_stream.h).
 *
 * An STR file is raw 2352-byte CD sectors: XA-ADPCM audio sectors (submode bit 2) go through the
 * XA decoder to the mixer's music input; the others are video chunks (2048 bytes of user data: a
 * 32-byte header, 2016 bytes of a frame's bitstream), assembled into whole frames. The file is
 * read at 150 sectors a second, the rate it was laid out for, so audio and video keep the time
 * they had on the PS1. With no frame free for a new one (the game is behind) the frame is dropped
 * and reading carries on, so the music never waits on the game.
 *
 * A Dreamcast film plays its ADX audio, kept a little ahead of the mixer instead of paced by
 * sectors. Its end tells pc_music.cpp, which loops the song (a film's frames aren't counted by the
 * game, so its end frame never comes).
 *
 * The game's music levels route only the CD's left channel, to both sides (the PS1's songs and
 * logo movie are mono XA, which the CD plays on both channels). A film's audio is stereo: its
 * channels each get their side's level instead, or its right channel would be lost. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pc/playlist.h"
#include "pc/sfd.h"
#include "pc/stream.h"
#include "pc_assets.h"
#include "pc_runtime.h"
#include "pc_sound.h"
#include "pc_stream.h"
#include "pc/assets.h"
#include "pc/platform.h"
#include "pc/sound.h"
#include "pc/xa.h"

namespace pc {

Streamer g_streamer;

namespace {

constexpr int kSectorsPerSecond = 150;
constexpr int kStrMagic = 0x0160;
constexpr int kStrHeader = 32;
constexpr int kStrPayload = 2016;
constexpr unsigned kFilmAhead = 12288; /* film audio frames kept queued for the mixer (its prebuffer is 8192) */

bool HasExt(const char *path, const char *ext) {
    size_t n = strlen(path), e = strlen(ext);
    return n > e && !_stricmp(path + n - e, ext);
}

} // namespace

/* ---- files ------------------------------------------------------------------------------------ */

int Streamer::File(const char *path) {
    char full[MAX_PATH];
    int i;
    if (!os::FullPath(path, full, sizeof full) || !os::IsFile(full))
        return 0;
    for (i = 0; i < m_nfiles; i++)
        if (!_stricmp(m_files[i].path, full))
            return i + 1;
    if (m_nfiles == kMaxFiles)
        return 0;
    snprintf(m_files[m_nfiles].path, sizeof m_files[m_nfiles].path, "%s", full);
    m_files[m_nfiles].film = HasExt(full, ".sfd");
    PC_Trace("stream %d: %s", m_nfiles + 1, full);
    return ++m_nfiles;
}

int Streamer::Find(const char *discName) {
    char path[MAX_PATH];
    int id = g_assets.FindFilm(discName, path, sizeof path) ? File(path) : 0;
    if (!id)
        PC_Log("%s: not in the game's data", discName);
    return id;
}

const Streamer::FileEntry *Streamer::FileOf(int id) const {
    return id > 0 && id <= m_nfiles ? &m_files[id - 1] : nullptr;
}

const char *Streamer::Path(int id) const { return FileOf(id) ? FileOf(id)->path : nullptr; }

bool Streamer::IsFilm(int id) const { return FileOf(id) && FileOf(id)->film; }

int Streamer::LastFrame(int id) const {
    uint8_t raw[kSector];
    const uint8_t *u = raw + 24;
    long long n, i;
    FILE *f;
    int last = 0;
    if (!FileOf(id) || FileOf(id)->film || !(f = fopen(FileOf(id)->path, "rb")))
        return 0;
    PC_FSEEK(f, 0, SEEK_END);
    n = PC_FTELL(f) / kSector;
    for (i = n - 1; i >= 0 && n - i <= 1024 && !last; i--) {
        PC_FSEEK(f, i * kSector, SEEK_SET);
        if (fread(raw, 1, kSector, f) != kSector || (raw[18] & 0x04))
            continue;
        if ((u[0] | u[1] << 8) == kStrMagic)
            last = u[8] | u[9] << 8 | u[10] << 16 | u[11] << 24;
    }
    fclose(f);
    return last;
}

/* ---- video frames ------------------------------------------------------------------------------ */

void Streamer::ClearFrames() {
    for (VideoFrame &f : m_fr)
        f.state = FrameState::Free;
    m_filling = -1;
}

/* A free frame with room for `need` bytes, or null if every one is in use. */
Streamer::VideoFrame *Streamer::AllocFrame(uint32_t need) {
    for (VideoFrame &f : m_fr) {
        if (f.state != FrameState::Free)
            continue;
        if (f.cap < need) {
            uint8_t *b = (uint8_t *)realloc(f.buf, need);
            if (!b)
                return nullptr;
            f.buf = b;
            f.cap = need;
        }
        return &f;
    }
    return nullptr;
}

/* One video chunk (a sector's 2048 bytes of user data). */
void Streamer::VideoChunk(const uint8_t *u) {
    VideoFrame *f;
    int chunk, chunks;
    uint32_t frame;
    if ((u[0] | u[1] << 8) != kStrMagic)
        return;
    chunk = u[4] | u[5] << 8;
    chunks = u[6] | u[7] << 8;
    frame = u[8] | u[9] << 8 | u[10] << 16 | (uint32_t)u[11] << 24;
    if (chunks <= 0 || chunks > 64 || chunk >= chunks)
        return;
    f = m_filling >= 0 ? &m_fr[m_filling] : nullptr;
    if (f && (f->frame != frame || f->chunks != chunks)) { /* the last one never completed */
        f->state = FrameState::Free;
        f = nullptr;
        m_filling = -1;
    }
    if (!f) {
        f = AllocFrame(kStrHeader + (uint32_t)chunks * kStrPayload);
        if (!f)
            return; /* all in use: drop this frame, keep reading */
        f->state = FrameState::Filling;
        f->frame = frame;
        f->chunks = chunks;
        f->got = 0;
        memcpy(f->buf, u, kStrHeader);
        m_filling = (int)(f - m_fr);
    }
    memcpy(f->buf + kStrHeader + (uint32_t)chunk * kStrPayload, u + kStrHeader, kStrPayload);
    if (++f->got >= f->chunks) {
        f->state = FrameState::Ready;
        f->seq = m_seq++;
        m_filling = -1;
    }
}

bool Streamer::NextFrame(uint32_t **frame, uint32_t **header) {
    VideoFrame *best = nullptr;
    for (VideoFrame &f : m_fr)
        if (f.state == FrameState::Ready && (!best || f.seq < best->seq))
            best = &f;
    if (!best)
        return false;
    best->state = FrameState::Taken;
    *frame = (uint32_t *)(best->buf + kStrHeader);
    *header = (uint32_t *)best->buf;
    return true;
}

void Streamer::FreeFrame(const void *frame) {
    for (VideoFrame &f : m_fr)
        if (f.state != FrameState::Free && f.buf + kStrHeader == (const uint8_t *)frame) {
            f.state = FrameState::Free;
            return;
        }
}

/* ---- playing ------------------------------------------------------------------------------------ */

void Streamer::Close() {
    if (m_f)
        fclose(m_f);
    m_f = nullptr;
    delete m_sfd;
    m_sfd = nullptr;
    m_reading = false;
}

void Streamer::Play(int id) {
    const FileEntry *file = FileOf(id);
    if (id != m_id || !(m_f || m_sfd)) {
        Close();
        m_id = 0;
        if (!file)
            return;
        if (file->film)
            m_sfd = SfdFilm::Open(file->path).release();
        else
            m_f = fopen(file->path, "rb");
        if (!m_f && !m_sfd) {
            PC_Log("cannot read %s", file->path);
            return;
        }
        m_id = id;
        ApplyMix();
    }
    if (m_f)
        PC_FSEEK(m_f, 0, SEEK_SET);
    else
        m_sfd->Seek(0);
    g_xaDecoder.Reset();
    ClearFrames();
    m_reading = true;
    m_paused = false;
    m_owed = 0;
    m_heardFrom = g_sound.MusicConsumed() + g_sound.MusicQueued();
    m_filmRate = 0;
    PC_Trace("stream %d plays from the start", id);
}

void Streamer::Stop() {
    if (m_reading)
        PC_Trace("stream %d stopped", m_id);
    Close();
    m_id = 0;
    m_paused = false;
    ClearFrames();
}

void Streamer::Pause() {
    if (m_reading && !m_paused)
        PC_Trace("stream %d paused", m_id);
    m_paused = true;
}

void Streamer::Resume() {
    if (m_reading && m_paused)
        PC_Trace("stream %d resumed", m_id);
    m_paused = false;
    m_owed = 0;
}

bool Streamer::Busy() const {
    if (m_reading && !m_paused)
        return true;
    for (const VideoFrame &f : m_fr)
        if (f.state == FrameState::Ready || f.state == FrameState::Filling)
            return true;
    return false;
}

void Streamer::Mix(const uint8_t levels[4]) {
    memcpy(m_mix, levels, sizeof m_mix);
    ApplyMix();
}

void Streamer::ApplyMix() {
    uint8_t m[4];
    memcpy(m, m_mix, sizeof m);
    if (IsFilm(m_id) && !m[2] && !m[3]) {
        /* left to left and left to right: left to left and right to right */
        m[2] = m[1];
        m[1] = 0;
    }
    g_sound.MusicMix(m);
}

double Streamer::FilmTime() const {
    uint64_t now = g_sound.MusicConsumed();
    if (!m_sfd || !m_filmRate || now < m_heardFrom)
        return 0.0;
    return (double)(now - m_heardFrom) / m_filmRate;
}

void Streamer::FilmAudio() {
    while (g_sound.MusicQueued() < kFilmAhead) {
        int rate, n = m_sfd->Audio(m_filmPcm, kFilmChunk, &rate);
        if (!n) {
            PC_Trace("stream %d: end of the film's audio", m_id);
            m_reading = false;
            g_playlist.StreamEnded();
            return;
        }
        m_filmRate = rate;
        g_sound.MusicQueue(m_filmPcm, n, rate);
    }
}

void Streamer::Frame() {
    if (!m_reading || m_paused)
        return;
    if (m_sfd) {
        FilmAudio();
        return;
    }
    m_owed += kSectorsPerSecond;
    while (m_owed >= 60) {
        m_owed -= 60;
        if (fread(m_raw, 1, kSector, m_f) != kSector) {
            PC_Trace("stream %d: end of file", m_id);
            m_reading = false;
            return;
        }
        if (m_raw[15] != 2) /* not a mode 2 sector */
            continue;
        if (m_raw[18] & 0x04) { /* submode: audio */
            int rate, n = g_xaDecoder.DecodeSector(m_raw, m_xaPcm, rate);
            if (n)
                g_sound.MusicQueue(m_xaPcm, n, rate);
        } else {
            VideoChunk(m_raw + 24);
        }
    }
}

} // namespace pc

/* ---- the game's interface (pc_stream.h) ---- */

int PC_StreamFind(const char *discName) { return pc::g_streamer.Find(discName); }
void PC_StreamPlay(int id) { pc::g_streamer.Play(id); }
void PC_StreamStop(void) { pc::g_streamer.Stop(); }
void PC_StreamPause(void) { pc::g_streamer.Pause(); }
void PC_StreamResume(void) { pc::g_streamer.Resume(); }
int PC_StreamNextFrame(uint32_t **frame, uint32_t **header) {
    return pc::g_streamer.NextFrame(frame, header);
}
void PC_StreamFreeFrame(const void *frame) { pc::g_streamer.FreeFrame(frame); }
/* muting is the mixer's */
void PC_StreamMix(const uint8_t levels[4]) { pc::g_streamer.Mix(levels); }
void PC_StreamMute(int mute) { pc::g_sound.MusicMute(mute); }
