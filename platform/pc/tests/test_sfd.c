/* Standalone test of the Sofdec film reader (pc_sfd.c) on the Dreamcast files: decodes a film's
 * ADX audio to a WAV and counts its video bytes, then checks that a seek into the middle and a
 * rewind give the same samples as reading straight through.
 *
 * Build (from the repo root, MSYS2 mingw32 first on PATH):
 *   gcc -m32 -std=gnu11 -O2 -Iplatform/pc/include platform/pc/tests/test_sfd.c
 *       platform/pc/src/pc_sfd.c -o build/pc/test_sfd.exe
 * Run:
 *   build/pc/test_sfd.exe Data/Music/goldfing.sfd build/pc/goldfing_dc.wav */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pc_sfd.h"

static void Put32(FILE *f, unsigned v) { fwrite(&v, 4, 1, f); }
static void Put16(FILE *f, unsigned v) { unsigned short s = (unsigned short)v; fwrite(&s, 2, 1, f); }

static long long s_video;
static void OnVideo(void *ctx, const uint8_t *p, int n) {
    (void)ctx; (void)p;
    s_video += n;
}

#define CHUNK 4096

int main(int argc, char **argv) {
    static int16_t pcm[CHUNK * 2], again[CHUNK * 2];
    long long frames = 0, mid = 0, sum2 = 0;
    long long midPos = 0;
    int rate = 0, n, peak = 0, fails = 0, i;
    PcSfd *s;
    FILE *out;
    if (argc < 3) {
        fprintf(stderr, "usage: test_sfd film.sfd out.wav\n");
        return 2;
    }
    s = PC_SfdOpen(argv[1]);
    if (!s) {
        fprintf(stderr, "%s: not a program stream\n", argv[1]);
        return 1;
    }
    printf("%s: %lld bytes, %d s of audio\n", argv[1], PC_SfdSize(s), PC_SfdAudioSeconds(argv[1]));
    out = fopen(argv[2], "wb");
    if (!out)
        return 1;
    fwrite("RIFF\0\0\0\0WAVEfmt ", 1, 16, out);
    Put32(out, 16);
    Put16(out, 1);
    Put16(out, 2);
    Put32(out, 44100);
    Put32(out, 44100 * 4);
    Put16(out, 4);
    Put16(out, 16);
    fwrite("data\0\0\0\0", 1, 8, out);
    PC_SfdSetVideo(s, OnVideo, NULL);
    while ((n = PC_SfdAudio(s, pcm, CHUNK, &rate)) > 0) {
        for (i = 0; i < n * 2; i++) {
            int a = pcm[i] < 0 ? -pcm[i] : pcm[i];
            if (a > peak)
                peak = a;
            sum2 += (long long)pcm[i] * pcm[i] / 256;
        }
        fwrite(pcm, 4, (size_t)n, out);
        frames += n;
        if (!midPos && PC_SfdTell(s) > PC_SfdSize(s) / 2) {
            midPos = PC_SfdTell(s);
            mid = frames;
        }
    }
    while (PC_SfdDemux(s))
        ;
    {
        long data = (long)(frames * 4);
        fseek(out, 4, SEEK_SET);
        Put32(out, (unsigned)(36 + data));
        fseek(out, 40, SEEK_SET);
        Put32(out, (unsigned)data);
    }
    fclose(out);
    printf("audio: %lld frames at %d Hz (%.2f s), peak %d, rms %.0f; video: %lld bytes\n", frames,
           rate, (double)frames / (rate ? rate : 1), peak,
           frames ? __builtin_sqrt((double)sum2 * 256 / (frames * 2)) : 0.0, s_video);

    /* a seek to the middle resumes on a frame boundary: what follows is what a straight read
     * decoded from that point on (after the filter history settles, a few frames) */
    PC_SfdSetVideo(s, NULL, NULL);
    PC_SfdSeek(s, midPos);
    {
        PcSfd *ref = PC_SfdOpen(argv[1]);
        long long at = 0, skip;
        int r2;
        PC_SfdSetAudio(ref, 1);
        n = PC_SfdAudio(s, again, CHUNK, &r2);
        /* find where the resumed audio sits in the straight read: within a few frames of mid */
        while (at + CHUNK <= mid - CHUNK && PC_SfdAudio(ref, pcm, CHUNK, &r2) == CHUNK)
            at += CHUNK;
        skip = 0;
        {
            static int16_t win[CHUNK * 8 * 2];
            int got = 0, k, best = -1;
            while (got < CHUNK * 7 - CHUNK && (k = PC_SfdAudio(ref, win + got * 2, CHUNK, &r2)) > 0)
                got += k;
            /* the filter history starts at zero after a seek: compare once it has settled */
            for (k = 0; k + 2048 + 256 <= got && best < 0; k += 32) {
                int j, worst = 0;
                for (j = 2048 * 2; j < (2048 + 256) * 2 && worst < 64; j++) {
                    int d = win[k * 2 + j] - again[j];
                    if ((d < 0 ? -d : d) > worst)
                        worst = d < 0 ? -d : d;
                }
                if (worst < 64)
                    best = k;
            }
            if (best < 0) {
                printf("FAIL: audio after the seek matches nothing near the middle\n");
                fails++;
            } else {
                skip = at + best;
                printf("seek to byte %lld resumed at sample %lld (straight read: %lld at that byte)\n",
                       midPos, skip, mid);
            }
        }
        PC_SfdClose(ref);
    }
    PC_SfdSeek(s, 0);
    n = PC_SfdAudio(s, again, CHUNK, &rate);
    {
        PcSfd *ref = PC_SfdOpen(argv[1]);
        int r2, m = PC_SfdAudio(ref, pcm, CHUNK, &r2);
        if (m != n || memcmp(pcm, again, (size_t)n * 4)) {
            printf("FAIL: rewind does not repeat the start\n");
            fails++;
        }
        PC_SfdClose(ref);
    }
    PC_SfdClose(s);
    printf("%s\n", fails ? "FAILED" : "ok");
    return fails != 0;
}
