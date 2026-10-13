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
 * sectors. Its end tells pc_music.c, which loops the song (a film's frames aren't counted by the
 * game, so its end frame never comes). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pc_assets.h"
#include "pc_platform.h"
#include "pc_runtime.h"
#include "pc_sfd.h"
#include "pc_spu.h"
#include "pc_stream.h"
#include "pc_xa.h"

#define SECTOR 2352
#define SECTORS_PER_SECOND 150
#define STR_MAGIC 0x0160
#define STR_HEADER 32
#define STR_PAYLOAD 2016
#define MAX_FRAMES 8
#define MAX_FILES 256
#define FILM_AHEAD 12288 /* film audio frames kept queued for the mixer (its prebuffer is 8192) */
#define FILM_CHUNK 1024

typedef struct {
    char path[MAX_PATH];
    int film;
} File;

static File s_files[MAX_FILES];
static int s_nfiles;

/* the stream */
static int s_id;          /* playing (or paused), 0 for none */
static FILE *s_f;         /* an STR */
static PcSfd *s_sfd;      /* a film */
static int s_reading, s_paused;
static uint32_t s_owed;   /* sectors owed, in sixtieths */

/* its video frames */
enum { FR_FREE, FR_FILLING, FR_READY, FR_TAKEN };
typedef struct {
    int state;
    uint8_t *buf; /* the sector header, then the frame's bitstream */
    uint32_t cap;
    uint32_t frame, seq;
    int chunks, got;
} Frame;

static Frame s_fr[MAX_FRAMES];
static uint32_t s_seq;
static int s_filling = -1;

/* a film's audio */
static uint64_t s_heardFrom; /* the mixer's frame count at which its first frame plays */
static int s_filmRate;

/* ---- files ------------------------------------------------------------------------------------ */

static int HasExt(const char *path, const char *ext) {
    size_t n = strlen(path), e = strlen(ext);
    return n > e && !_stricmp(path + n - e, ext);
}

int PC_StreamFile(const char *path) {
    char full[MAX_PATH];
    int i;
    if (!PC_FullPath(path, full, sizeof full) || !PC_FileExists(full))
        return 0;
    for (i = 0; i < s_nfiles; i++)
        if (!_stricmp(s_files[i].path, full))
            return i + 1;
    if (s_nfiles == MAX_FILES)
        return 0;
    snprintf(s_files[s_nfiles].path, sizeof s_files[s_nfiles].path, "%s", full);
    s_files[s_nfiles].film = HasExt(full, ".sfd");
    PC_Trace("stream %d: %s", s_nfiles + 1, full);
    return ++s_nfiles;
}

int PC_StreamFind(const char *discName) {
    char path[MAX_PATH];
    int id = PC_AssetsFindFilm(discName, path, sizeof path) ? PC_StreamFile(path) : 0;
    if (!id)
        PC_Log("%s: not in the game's data", discName);
    return id;
}

static const File *FileOf(int id) { return id > 0 && id <= s_nfiles ? &s_files[id - 1] : NULL; }

const char *PC_StreamPath(int id) { return FileOf(id) ? FileOf(id)->path : NULL; }

int PC_StreamIsFilm(int id) { return FileOf(id) && FileOf(id)->film; }

int PC_StreamLastFrame(int id) {
    uint8_t raw[SECTOR];
    const uint8_t *u = raw + 24;
    long long n, i;
    FILE *f;
    int last = 0;
    if (!FileOf(id) || FileOf(id)->film || !(f = fopen(FileOf(id)->path, "rb")))
        return 0;
    PC_FSEEK(f, 0, SEEK_END);
    n = PC_FTELL(f) / SECTOR;
    for (i = n - 1; i >= 0 && n - i <= 1024 && !last; i--) {
        PC_FSEEK(f, i * SECTOR, SEEK_SET);
        if (fread(raw, 1, SECTOR, f) != SECTOR || (raw[18] & 0x04))
            continue;
        if ((u[0] | u[1] << 8) == STR_MAGIC)
            last = u[8] | u[9] << 8 | u[10] << 16 | u[11] << 24;
    }
    fclose(f);
    return last;
}

/* ---- video frames ------------------------------------------------------------------------------ */

static void ClearFrames(void) {
    for (int i = 0; i < MAX_FRAMES; i++)
        s_fr[i].state = FR_FREE;
    s_filling = -1;
}

/* A free frame with room for `need` bytes, or NULL if every one is in use. */
static Frame *AllocFrame(uint32_t need) {
    for (int i = 0; i < MAX_FRAMES; i++) {
        Frame *f = &s_fr[i];
        if (f->state != FR_FREE)
            continue;
        if (f->cap < need) {
            uint8_t *b = realloc(f->buf, need);
            if (!b)
                return NULL;
            f->buf = b;
            f->cap = need;
        }
        return f;
    }
    return NULL;
}

/* One video chunk (a sector's 2048 bytes of user data). */
static void VideoChunk(const uint8_t *u) {
    Frame *f;
    int chunk, chunks;
    uint32_t frame;
    if ((u[0] | u[1] << 8) != STR_MAGIC)
        return;
    chunk = u[4] | u[5] << 8;
    chunks = u[6] | u[7] << 8;
    frame = u[8] | u[9] << 8 | u[10] << 16 | (uint32_t)u[11] << 24;
    if (chunks <= 0 || chunks > 64 || chunk >= chunks)
        return;
    f = s_filling >= 0 ? &s_fr[s_filling] : NULL;
    if (f && (f->frame != frame || f->chunks != chunks)) { /* the last one never completed */
        f->state = FR_FREE;
        f = NULL;
        s_filling = -1;
    }
    if (!f) {
        f = AllocFrame(STR_HEADER + (uint32_t)chunks * STR_PAYLOAD);
        if (!f)
            return; /* all in use: drop this frame, keep reading */
        f->state = FR_FILLING;
        f->frame = frame;
        f->chunks = chunks;
        f->got = 0;
        memcpy(f->buf, u, STR_HEADER);
        s_filling = (int)(f - s_fr);
    }
    memcpy(f->buf + STR_HEADER + (uint32_t)chunk * STR_PAYLOAD, u + STR_HEADER, STR_PAYLOAD);
    if (++f->got >= f->chunks) {
        f->state = FR_READY;
        f->seq = s_seq++;
        s_filling = -1;
    }
}

int PC_StreamNextFrame(uint32_t **frame, uint32_t **header) {
    Frame *best = NULL;
    for (int i = 0; i < MAX_FRAMES; i++)
        if (s_fr[i].state == FR_READY && (!best || s_fr[i].seq < best->seq))
            best = &s_fr[i];
    if (!best)
        return 0;
    best->state = FR_TAKEN;
    *frame = (uint32_t *)(best->buf + STR_HEADER);
    *header = (uint32_t *)best->buf;
    return 1;
}

void PC_StreamFreeFrame(const void *frame) {
    for (int i = 0; i < MAX_FRAMES; i++)
        if (s_fr[i].state != FR_FREE && s_fr[i].buf + STR_HEADER == (const uint8_t *)frame) {
            s_fr[i].state = FR_FREE;
            return;
        }
}

/* ---- playing ------------------------------------------------------------------------------------ */

static void Close(void) {
    if (s_f)
        fclose(s_f);
    s_f = NULL;
    PC_SfdClose(s_sfd);
    s_sfd = NULL;
    s_reading = 0;
}

void PC_StreamPlay(int id) {
    const File *file = FileOf(id);
    if (id != s_id || !(s_f || s_sfd)) {
        Close();
        s_id = 0;
        if (!file)
            return;
        if (file->film)
            s_sfd = PC_SfdOpen(file->path);
        else
            s_f = fopen(file->path, "rb");
        if (!s_f && !s_sfd) {
            PC_Log("cannot read %s", file->path);
            return;
        }
        s_id = id;
    }
    if (s_f)
        PC_FSEEK(s_f, 0, SEEK_SET);
    else
        PC_SfdSeek(s_sfd, 0);
    PC_XaReset();
    ClearFrames();
    s_reading = 1;
    s_paused = 0;
    s_owed = 0;
    s_heardFrom = PC_SpuCdConsumed() + PC_SpuCdQueued();
    s_filmRate = 0;
    PC_Trace("stream %d plays from the start", id);
}

void PC_StreamStop(void) {
    if (s_reading)
        PC_Trace("stream %d stopped", s_id);
    Close();
    s_id = 0;
    s_paused = 0;
    ClearFrames();
}

void PC_StreamPause(void) {
    if (s_reading && !s_paused)
        PC_Trace("stream %d paused", s_id);
    s_paused = 1;
}

void PC_StreamResume(void) {
    if (s_reading && s_paused)
        PC_Trace("stream %d resumed", s_id);
    s_paused = 0;
    s_owed = 0;
}

void PC_StreamMix(const uint8_t levels[4]) { PC_SpuCdSetMix(levels); }

void PC_StreamMute(int mute) { PC_SpuCdMute(mute); }

int PC_StreamBusy(void) {
    if (s_reading && !s_paused)
        return 1;
    for (int i = 0; i < MAX_FRAMES; i++)
        if (s_fr[i].state == FR_READY || s_fr[i].state == FR_FILLING)
            return 1;
    return 0;
}

double PC_StreamFilmTime(void) {
    uint64_t now = PC_SpuCdConsumed();
    if (!s_sfd || !s_filmRate || now < s_heardFrom)
        return 0.0;
    return (double)(now - s_heardFrom) / s_filmRate;
}

static void FilmAudio(void) {
    static int16_t pcm[FILM_CHUNK * 2];
    while (PC_SpuCdQueued() < FILM_AHEAD) {
        int rate, n = PC_SfdAudio(s_sfd, pcm, FILM_CHUNK, &rate);
        if (!n) {
            PC_Trace("stream %d: end of the film's audio", s_id);
            s_reading = 0;
            PC_MusicStreamEnded();
            return;
        }
        s_filmRate = rate;
        PC_SpuCdQueue(pcm, n, rate);
    }
}

void PC_StreamFrame(void) {
    static uint8_t raw[SECTOR];
    static int16_t pcm[PC_XA_MAX_FRAMES * 2];
    if (!s_reading || s_paused)
        return;
    if (s_sfd) {
        FilmAudio();
        return;
    }
    s_owed += SECTORS_PER_SECOND;
    while (s_owed >= 60) {
        s_owed -= 60;
        if (fread(raw, 1, SECTOR, s_f) != SECTOR) {
            PC_Trace("stream %d: end of file", s_id);
            s_reading = 0;
            return;
        }
        if (raw[15] != 2) /* not a mode 2 sector */
            continue;
        if (raw[18] & 0x04) { /* submode: audio */
            int rate, n = PC_XaDecodeSector(raw, pcm, &rate);
            if (n)
                PC_SpuCdQueue(pcm, n, rate);
        } else {
            VideoChunk(raw + 24);
        }
    }
}
