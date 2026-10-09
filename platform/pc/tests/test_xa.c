/* Standalone test of the CD-XA decoder on real disc data: decodes the XA audio sectors in a
 * sector range of the disc image to a WAV, and checks that at the drive's 2x speed (150
 * sectors/s) the interleave delivers audio at exactly the stream's sample rate, which is what
 * the port's stream pump relies on.
 *
 * Build (from the repo root, MSYS2 mingw32 first on PATH):
 *   gcc -m32 -std=gnu11 -O2 -Iplatform/pc/include platform/pc/tests/test_xa.c
 *       platform/pc/src/pc_xa.c -o build/pc/test_xa.exe
 * Run:
 *   build/pc/test_xa.exe extern/thps1_disc/thps1.bin 119398 3000 build/pc/goldfing.wav */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pc_xa.h"

static void Put32(FILE *f, unsigned v) { fwrite(&v, 4, 1, f); }
static void Put16(FILE *f, unsigned v) { unsigned short s = (unsigned short)v; fwrite(&s, 2, 1, f); }

int main(int argc, char **argv) {
    static uint8_t raw[2352];
    static int16_t pcm[PC_XA_MAX_FRAMES * 2];
    long frames = 0, audioSectors = 0, sectors;
    int rate = 0, firstRate = 0, peak = 0, fails = 0;
    long lba, i;
    FILE *in, *out;

    if (argc < 5) {
        fprintf(stderr, "usage: test_xa disc.bin first_lba sectors out.wav\n");
        return 2;
    }
    lba = atol(argv[2]);
    sectors = atol(argv[3]);
    if (!(in = fopen(argv[1], "rb")) || !(out = fopen(argv[4], "wb"))) {
        fprintf(stderr, "cannot open files\n");
        return 2;
    }
    fwrite("RIFF\0\0\0\0WAVEfmt ", 1, 16, out);
    Put32(out, 16), Put16(out, 1), Put16(out, 2), Put32(out, 0), Put32(out, 0);
    Put16(out, 4), Put16(out, 16);
    fwrite("data\0\0\0\0", 1, 8, out);

    PC_XaReset();
    for (i = 0; i < sectors; i++) {
        int n;
        if (fseek(in, (lba + i) * 2352L, SEEK_SET) || fread(raw, 1, 2352, in) != 2352)
            break;
        if (!(n = PC_XaDecodeSector(raw, pcm, &rate)))
            continue;
        if (!firstRate)
            firstRate = rate;
        if (rate != firstRate)
            fails++;
        for (int k = 0; k < n * 2; k++)
            if (abs(pcm[k]) > peak)
                peak = abs(pcm[k]);
        fwrite(pcm, 4, (size_t)n, out);
        frames += n;
        audioSectors++;
    }
    /* patch the header now that the length and rate are known */
    fseek(out, 4, SEEK_SET), Put32(out, (unsigned)(36 + frames * 4));
    fseek(out, 24, SEEK_SET), Put32(out, (unsigned)firstRate), Put32(out, (unsigned)firstRate * 4);
    fseek(out, 40, SEEK_SET), Put32(out, (unsigned)(frames * 4));
    fclose(out);
    fclose(in);

    double seconds = (double)i / 150.0;
    double delivered = seconds > 0 ? frames / seconds : 0;
    printf("%ld sectors read, %ld audio (1 in %.1f), %ld frames at %d Hz, peak %d\n", i,
           audioSectors, audioSectors ? (double)i / audioSectors : 0.0, frames, firstRate, peak);
    printf("at 150 sectors/s the stream delivers %.0f frames/s (%.2f%% of the sample rate)\n",
           delivered, firstRate ? 100.0 * delivered / firstRate : 0.0);
    if (!audioSectors || fails || peak == 0 || delivered < firstRate * 0.98 ||
        delivered > firstRate * 1.02) {
        printf("FAIL\n");
        return 1;
    }
    printf("ok\n");
    return 0;
}
