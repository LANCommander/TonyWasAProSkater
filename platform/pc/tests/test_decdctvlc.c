/* The C DecDCTvlcBuild3/DecDCTvlc3 (decomp/src/shatter/decdctvlc3.c), checked against
 *   - hand-assembled BS frames, decoded by hand;
 *   - frames from a small BS v2/v3 encoder here, against the run-level data it encoded;
 *   - the retail MIPS (run by mips_ref.c) on all of the above, on every video frame of the game's
 *     STR files, and on corrupted copies of them, with and without suspending.
 *   gcc -m32 -O2 -std=gnu11 test_decdctvlc.c mips_ref.c ../../../decomp/src/shatter/decdctvlc3.c -o test_decdctvlc
 *   ./test_decdctvlc [SLUS_008.60 [Data dir]]
 *     (defaults ../../../extern/thps1_retail/SLUS_008.60 and ../../../build/pc/Data)
 * The C reads its table from the scratchpad address, so the test maps memory at 0x1f800000. */
#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "mips_ref.h"

void DecDCTvlcBuild3(void);
int DecDCTvlc3(unsigned long *bs, unsigned long *buf);
extern unsigned char DecDCTvlcCodes3[216];
extern unsigned long DecDCTvlcMaxSize3;
extern struct {
	unsigned long bits, next, peek, in, out, word, used, quant, out_end, dc_cr, dc_cb, dc_y, v2;
} DecDCTvlcState3;

#define ADDR_BUILD 0x80022498u
#define ADDR_VLC 0x800224e4u
#define ADDR_CODES 0x800a6278u
#define ADDR_MAXSIZE 0x800a6350u
#define ADDR_STATE 0x800a6354u
#define SCRATCH ((unsigned char *)0x1f800000)

/* The reference machine's buffers, and native ones of the same sizes. Output gets slack past the
 * frame's own end: corrupt streams run on until the next suspension point. */
#define REF_BS 0x80100000u
#define BS_MAX 0x10000u
#define REF_OUT 0x80110000u
#define OUT_MAX 0x40000u /* game frames need up to 60 KB */

static unsigned long *bs_c;
static unsigned short *out_c;

static MRef m;
static int have_ref;
static int failures, checks;

#define CHECK_EQ(actual, expected)                                                                 \
	do {                                                                                           \
		long long a_ = (long long)(actual), e_ = (long long)(expected);                            \
		checks++;                                                                                  \
		if (a_ != e_) {                                                                            \
			failures++;                                                                            \
			printf("%s:%d: %s = 0x%llx (%lld), expected 0x%llx (%lld)\n", __FILE__, __LINE__,      \
			       #actual, a_, a_, e_, e_);                                                       \
		}                                                                                          \
	} while (0)

static uint32_t rng = 0x9e3779b9u;

static uint32_t Rand(void) {
	rng ^= rng << 13;
	rng ^= rng >> 17;
	rng ^= rng << 5;
	return rng;
}

/* ---- BS writer ------------------------------------------------------------------------------ */

typedef struct {
	unsigned long words[BS_MAX / 4];
	int nbits;
	unsigned short expect[OUT_MAX / 2]; /* what the decoder should write after the header word */
	int nexpect;
} Frame;

static Frame fr;

static void Begin(int version, int quant) {
	memset(&fr, 0, sizeof fr);
	fr.words[1] = (unsigned long)version << 16 | quant;
	fr.nbits = 64;
}

/* bits go MSB first into 16-bit little-endian units */
static void Put(unsigned long value, int n) {
	while (n--) {
		int unit = fr.nbits >> 4, bit = 15 - (fr.nbits & 15);
		if ((value >> n) & 1)
			((unsigned short *)fr.words)[unit] |= 1 << bit;
		fr.nbits++;
	}
}

static void PutStr(const char *s) {
	for (; *s; s++)
		if (*s != ' ')
			Put(*s == '1', 1);
}

static void Expect(unsigned short h) {
	fr.expect[fr.nexpect++] = h;
}

/* The MDEC header gives the output size in words; the decoder pads it with EOBs. */
static int Finish(void) {
	int words = (fr.nexpect + 1) / 2;
	while (fr.nexpect < words * 2)
		Expect(0xfe00);
	fr.words[0] = 0x38000000u | words;
	return (fr.nbits + 31) / 32 + 8; /* the decoder reads a few words ahead */
}

/* ---- running both decoders ------------------------------------------------------------------ */

static void SetMaxSize(unsigned long n) {
	DecDCTvlcMaxSize3 = n;
	if (have_ref)
		memcpy(MRef_Ptr(&m, ADDR_MAXSIZE), &n, 4);
}

/* Decode with the C; then, if there's a reference, the retail code, comparing what each call
 * returns and writes, and the state a suspension leaves. Returns 0 if the retail code faulted
 * (only corrupt streams can make it), else 1. */
static int RunBoth(const unsigned long *bs, int nwords, int max_calls, const char *what) {
	int call, rc, rr = 0, fault = 0;
	int n_fail = failures;

	memset(bs_c, 0, BS_MAX);
	memcpy(bs_c, bs, nwords * 4);
	memset(out_c, 0, OUT_MAX);
	if (have_ref) {
		memset(MRef_Ptr(&m, REF_BS), 0, BS_MAX);
		memcpy(MRef_Ptr(&m, REF_BS), bs, nwords * 4);
		memset(MRef_Ptr(&m, REF_OUT), 0, OUT_MAX);
	}
	for (call = 0; call < max_calls; call++) {
		rc = DecDCTvlc3(call ? NULL : bs_c, (unsigned long *)out_c);
		if (have_ref) {
			rr = (int)MRef_Call(&m, ADDR_VLC, call ? 0 : REF_BS, REF_OUT, 0, 0);
			if (m.fault) {
				fault = 1;
				break;
			}
			checks++;
			if (rc != rr) {
				failures++;
				printf("%s, call %d: C returns %d, retail %d\n", what, call, rc, rr);
				break;
			}
			if (rc) { /* the suspended state, apart from the pointers */
				unsigned long s[13];
				memcpy(s, MRef_Ptr(&m, ADDR_STATE), sizeof s);
				checks++;
				if (DecDCTvlcState3.bits != s[0] || DecDCTvlcState3.next != s[1] ||
				    DecDCTvlcState3.peek != s[2] || DecDCTvlcState3.word != s[5] ||
				    DecDCTvlcState3.used != s[6] || DecDCTvlcState3.quant != s[7] ||
				    DecDCTvlcState3.dc_cr != s[9] || DecDCTvlcState3.dc_cb != s[10] ||
				    DecDCTvlcState3.dc_y != s[11] || !DecDCTvlcState3.v2 != !s[12] ||
				    DecDCTvlcState3.in - (unsigned long)bs_c != s[3] - REF_BS ||
				    DecDCTvlcState3.out - (unsigned long)out_c != s[4] - REF_OUT) {
					failures++;
					printf("%s, call %d: suspended state differs\n", what, call);
					break;
				}
			}
		}
		if (!rc)
			break;
	}
	/* the output, once at the end (after every call would be quadratic in the suspend tests) */
	if (have_ref && !fault && failures == n_fail) {
		const unsigned short *o = (const unsigned short *)MRef_Ptr(&m, REF_OUT);
		int i;
		checks++;
		if (memcmp(out_c, o, OUT_MAX)) {
			failures++;
			for (i = 0; out_c[i] == o[i]; i++)
				;
			printf("%s: output differs from halfword %d (C %04x, retail %04x)\n", what, i,
			       out_c[i], o[i]);
		}
	}
	return !fault;
}

/* Decode the frame in `fr` and check the output against what was encoded (and the retail code
 * against the C). */
static void CheckFrame(const char *what) {
	int nwords = Finish();
	int i;

	SetMaxSize(0xffffff);
	RunBoth(fr.words, nwords, 1, what);
	checks++;
	if (out_c[0] != (fr.words[0] & 0xffff) || out_c[1] != fr.words[0] >> 16) {
		failures++;
		printf("%s: header word not copied\n", what);
		return;
	}
	for (i = 0; i < fr.nexpect; i++)
		if (out_c[2 + i] != fr.expect[i])
			break;
	checks++;
	if (i < fr.nexpect || out_c[2 + i] != 0) {
		failures++;
		printf("%s: halfword %d is %04x, encoded %04x\n", what, i, out_c[2 + i],
		       i < fr.nexpect ? fr.expect[i] : 0);
	}
}

/* ---- hand-worked frames --------------------------------------------------------------------- */

static void TestHandWorked(void) {
	/* v2, quant 1: DC 0x155, AC "110" (run 0, +1), EOB, end code 0111111111 */
	Begin(2, 1);
	PutStr("0101010101 110 10 0111111111");
	Expect(0x0555);
	Expect(0x0001);
	Expect(0xfe00);
	CheckFrame("hand v2");
	CHECK_EQ(fr.words[2], 0xff805574u); /* the bits as the stream stores them */

	/* v2, quant 0: two blocks; "0100 1" is run 0 level -2, "0101 0" run 2 level +1, "011 1" run 1
	 * level -1, "000001 000011 1111111101" an escape for run 3 level -3. The second block's DC
	 * follows the first's EOB directly. */
	Begin(2, 0);
	PutStr("0000000001 01001 01010 0111 000001 000011 1111111101 10");
	PutStr("1000000000 10 0111111111");
	Expect(0x0001);
	Expect(0x03fe);
	Expect(0x0801);
	Expect(0x07ff);
	Expect(0x0ffd);
	Expect(0xfe00);
	Expect(0x0200);
	Expect(0xfe00);
	CheckFrame("hand v2 codes");

	/* v3, quant 2: one macroblock. Cr: "01 1" size 1 = +1 -> DC 4; Cb: "1110 1001" size 4 = +9
	 * -> 36; Y1 "100" size 0 -> 0, Y2 "00 0" size 1 = -1 -> -4 & 1023, Y3 "101 011" size 3 = 3-7
	 * = -4 -> -5 * 4, Y4 "110 0110" size 4 = 6-15 = -9 -> -14 * 4. Then end code 1111111111. */
	Begin(3, 2);
	PutStr("01 1 10");
	PutStr("1110 1001 10");
	PutStr("100 10");
	PutStr("00 0 10");
	PutStr("101 011 10");
	PutStr("110 0110 10");
	PutStr("1111111111");
	Expect(0x0800 | 4);
	Expect(0xfe00);
	Expect(0x0800 | 36);
	Expect(0xfe00);
	Expect(0x0800 | 0);
	Expect(0xfe00);
	Expect(0x0800 | (-4 & 1023));
	Expect(0xfe00);
	Expect(0x0800 | (-20 & 1023));
	Expect(0xfe00);
	Expect(0x0800 | (-56 & 1023));
	Expect(0xfe00);
	CheckFrame("hand v3");
}

/* ---- encoder -------------------------------------------------------------------------------- */

/* code for each (run, level > 0) of the table, from DecDCTvlcCodes3's layout */
static struct {
	unsigned long code;
	int len;
} ac_code[64][41];

static void BuildEncoder(void) {
	int h;
	memset(ac_code, 0, sizeof ac_code);
	for (h = 0; h < 214; h += 2) {
		int run = DecDCTvlcCodes3[h], level = DecDCTvlcCodes3[h + 1];
		unsigned long code;
		int len;
		if (h < 8) /* 0001 xx s */
			code = 0x4 | h >> 1, len = 6;
		else if (h < 16) /* 00001 xx s */
			code = 0x04 | (h - 8) >> 1, len = 7;
		else if (h < 32) /* 0000001 xxx s */
			code = 0x08 | (h - 16) >> 1, len = 10;
		else if (h < 192) { /* lz zeros, 1, xxxx s */
			int lz = 7 + (h - 32) / 32;
			code = 0x10 | (h & 31) >> 1, len = lz + 5;
		} else if (h < 208) /* 00100 xxx s */
			code = 0x20 | (h - 192) >> 1, len = 8;
		else /* 001 xx s */
			code = 0x4 | (h - 206) >> 1, len = 5;
		ac_code[run][level].code = code;
		ac_code[run][level].len = len;
	}
	/* the codes AcCode decodes without the table */
	ac_code[0][1].code = 0x3, ac_code[0][1].len = 2;   /* 11 s */
	ac_code[1][1].code = 0x3, ac_code[1][1].len = 3;   /* 011 s */
	ac_code[2][1].code = 0x5, ac_code[2][1].len = 4;   /* 0101 s */
	ac_code[0][2].code = 0x4, ac_code[0][2].len = 4;   /* 0100 s */
}

static void PutAc(int run, int level) {
	int a = level < 0 ? -level : level;
	if (a <= 40 && ac_code[run][a].len && (Rand() & 7)) {
		Put(ac_code[run][a].code, ac_code[run][a].len);
		Put(level < 0, 1);
	} else { /* escape */
		Put(1, 6);
		Put(run, 6);
		Put(level & 1023, 10);
	}
	Expect(run << 10 | (level & 1023));
}

/* AC codes for a block: a few random ones, biased towards the table */
static void PutAcs(void) {
	int n = Rand() % 12, i;
	for (i = 0; i < n; i++) {
		int run, level;
		if (Rand() & 3) { /* a code from the table or the special ones */
			do {
				run = Rand() % 32;
				level = 1 + Rand() % 40;
			} while (!ac_code[run][level].len);
		} else {
			run = Rand() % 64;
			level = (int)(Rand() % 1023) - 511;
			if (!level)
				level = 1;
		}
		PutAc(run, Rand() & 1 ? -level : level);
	}
	Put(2, 2); /* EOB */
	Expect(0xfe00);
}

/* A DC difference in MPEG-1 style: size code, then size bits (negative: d + 2^size - 1). */
static void PutDcDiff(int d, int luma) {
	int a = d < 0 ? -d : d, size = 0;
	while (a >> size)
		size++;
	if (luma) {
		static const char *small[] = {"100", "00", "01", "101"};
		if (size < 4)
			PutStr(small[size]);
		else
			Put((1ul << (size - 1)) - 2, size - 1); /* size-2 ones, then 0 */
	} else {
		if (size == 0)
			PutStr("00");
		else if (size == 1)
			PutStr("01");
		else
			Put((1ul << size) - 2, size); /* size-1 ones, then 0 */
	}
	if (size)
		Put(d > 0 ? (unsigned long)d : (unsigned long)(d + (1 << size) - 1), size);
}

static void EncodeV2(int quant, int blocks) {
	int b;
	Begin(2, quant);
	for (b = 0; b < blocks; b++) {
		int dc;
		do
			dc = Rand() & 1023;
		while (dc == 0x1ff); /* the end code */
		Put(dc, 10);
		Expect(dc | quant << 10);
		PutAcs();
	}
	PutStr("0111111111");
}

static void EncodeV3(int quant, int macroblocks) {
	long pred[3] = {0, 0, 0};
	int mb, b;
	Begin(3, quant);
	for (mb = 0; mb < macroblocks; mb++) {
		for (b = 0; b < 6; b++) {
			int kind = b < 2 ? b : 2, size = Rand() % 9, d;
			d = size ? (int)((1u << (size - 1)) + Rand() % (1u << (size - 1))) : 0;
			if (Rand() & 1)
				d = -d;
			PutDcDiff(d, kind == 2);
			pred[kind] += d;
			Expect(((unsigned long)pred[kind] << 2 & 1023) | quant << 10);
			PutAcs();
		}
	}
	PutStr("1111111111");
}

static void TestEncoded(void) {
	int i;
	char what[64];
	BuildEncoder();
	for (i = 0; i < 3000; i++) {
		int q = Rand() & 63, n = 1 + Rand() % 60;
		if (i & 1)
			EncodeV3(q, n);
		else
			EncodeV2(q, n * 6);
		sprintf(what, "encoded v%d frame %d", i & 1 ? 3 : 2, i);
		CheckFrame(what);
	}
}

/* ---- the game's STR files ------------------------------------------------------------------- */

typedef struct {
	unsigned long *bs;
	int nwords;
} StrFrame;

static StrFrame *frames;
static int nframes, cap;

/* Assemble the video frames of a raw 2352-byte-sector STR file. */
static void LoadStr(const char *path) {
	static unsigned char sec[2352];
	unsigned char *cur = NULL;
	unsigned long cur_no = ~0ul, cur_bytes = 0;
	FILE *f = fopen(path, "rb");
	if (!f)
		return;
	while (fread(sec, 1, sizeof sec, f) == sizeof sec) {
		const unsigned char *d = sec + 24;
		uint16_t magic, type, secno, nsec;
		uint32_t frame, bytes;
		if (sec[18] & 0x04) /* audio */
			continue;
		memcpy(&magic, d, 2);
		memcpy(&type, d + 2, 2);
		memcpy(&secno, d + 4, 2);
		memcpy(&nsec, d + 6, 2);
		memcpy(&frame, d + 8, 4);
		memcpy(&bytes, d + 12, 4);
		if (magic != 0x0160 || type != 0x8001 || bytes > BS_MAX - 64)
			continue;
		if (frame != cur_no) {
			cur_no = frame;
			cur_bytes = (bytes + 2015) / 2016 * 2016; /* whole sectors */
			cur = calloc(1, cur_bytes + 64);
			if (nframes == cap)
				frames = realloc(frames, (cap = cap ? cap * 2 : 1024) * sizeof *frames);
			frames[nframes].bs = (unsigned long *)cur;
			frames[nframes].nwords = (int)(cur_bytes + 64) / 4;
			nframes++;
		}
		if ((secno + 1) * 2016u <= cur_bytes)
			memcpy(cur + secno * 2016, d + 32, 2016);
		(void)nsec;
	}
	fclose(f);
}

static void LoadStrDir(const char *dir) {
	DIR *d = opendir(dir);
	struct dirent *e;
	char path[512];
	if (!d)
		return;
	while ((e = readdir(d))) {
		size_t n = strlen(e->d_name);
		if (n > 4 && !strcasecmp(e->d_name + n - 4, ".STR")) {
			snprintf(path, sizeof path, "%s/%s", dir, e->d_name);
			LoadStr(path);
		}
	}
	closedir(d);
}

static void TestGameFrames(void) {
	int i;
	char what[64];
	SetMaxSize(0xffffff);
	for (i = 0; i < nframes; i++) {
		sprintf(what, "STR frame %d", i);
		RunBoth(frames[i].bs, frames[i].nwords, 1, what);
	}
	printf("compared %d STR frames\n", nframes);
}

/* Resuming: a small output budget makes both decoders suspend and resume, many times a frame. */
static void TestSuspend(void) {
	int i;
	char what[64];
	for (i = 0; i < 400; i++) {
		int nwords;
		const unsigned long *bs;
		if (i & 1 || !nframes) {
			if (i & 2)
				EncodeV3(Rand() & 63, 1 + Rand() % 40);
			else
				EncodeV2(Rand() & 63, 6 + Rand() % 240);
			nwords = Finish();
			bs = fr.words;
		} else {
			StrFrame *s = &frames[Rand() % nframes];
			bs = s->bs;
			nwords = s->nwords;
		}
		SetMaxSize(1 + Rand() % 600);
		sprintf(what, "suspended decode %d", i);
		RunBoth(bs, nwords, 100000, what);
	}
	SetMaxSize(0xffffff);
}

/* Corrupt streams: flipped bits, then decode until the first suspension. Cases where the retail
 * code would bus-error on a PS1 (a code with 22+ leading zeros reads past the scratchpad) are
 * skipped. */
static void TestCorrupt(void) {
	static unsigned long buf[BS_MAX / 4];
	int i, skipped = 0, n = 0;
	char what[64];
	for (i = 0; i < 20000; i++) {
		int nwords, flips = 1 + Rand() % 8, k;
		if (i & 1 || !nframes) {
			if (i & 2)
				EncodeV3(Rand() & 63, 1 + Rand() % 40);
			else
				EncodeV2(Rand() & 63, 6 + Rand() % 240);
			nwords = Finish();
			memcpy(buf, fr.words, nwords * 4);
		} else {
			StrFrame *s = &frames[Rand() % nframes];
			nwords = s->nwords;
			memcpy(buf, s->bs, nwords * 4);
		}
		for (k = 0; k < flips; k++) /* keep the header: its size bounds the padding */
			buf[2 + Rand() % (nwords - 2)] ^= 1u << (Rand() & 31);
		if (i % 5 == 0) /* sometimes the version word too */
			buf[1] ^= 1u << (16 + (Rand() & 15));
		SetMaxSize(2 + Rand() % 0x4000);
		sprintf(what, "corrupt stream %d", i);
		if (RunBoth(buf, nwords, 1, what))
			n++;
		else
			skipped++;
	}
	SetMaxSize(0xffffff);
	printf("compared %d corrupt streams (%d skipped: the retail code faults)\n", n, skipped);
}

int main(int argc, char **argv) {
	const char *exe = argc > 1 ? argv[1] : "../../../extern/thps1_retail/SLUS_008.60";
	const char *data = argc > 2 ? argv[2] : "../../../build/pc/Data";
	char dir[512];

	setvbuf(stdout, NULL, _IONBF, 0);
	if (!VirtualAlloc(SCRATCH, 4096, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE)) {
		printf("can't map the scratchpad at %p\n", (void *)SCRATCH);
		return 2;
	}
	bs_c = calloc(1, BS_MAX + 4096);
	out_c = calloc(1, OUT_MAX + 4096);
	have_ref = MRef_Load(&m, exe);

	DecDCTvlcBuild3();
	if (have_ref) {
		CHECK_EQ(memcmp(DecDCTvlcCodes3, MRef_Ptr(&m, ADDR_CODES), 216), 0);
		MRef_Call(&m, ADDR_BUILD, 0, 0, 0, 0);
		CHECK_EQ(m.fault, 0);
		CHECK_EQ(memcmp(SCRATCH, m.scratch, MREF_SCRATCH_SIZE), 0);
		/* the C reads past the table only for corrupt streams; give both the same contents */
		memcpy(m.scratch, SCRATCH, MREF_SCRATCH_SIZE);
	}
	/* table spot checks: run 0 level 1 is not in it; (7, 1) is the first entry */
	CHECK_EQ(((unsigned short *)SCRATCH)[0], 7 << 10 | 1);
	CHECK_EQ(((unsigned short *)SCRATCH)[1], 7 << 10 | 1023);
	CHECK_EQ(((unsigned short *)SCRATCH)[216], 0xfcff); /* the 109th pair: 0xff, 0xff */

	TestHandWorked();
	TestEncoded();
	if (have_ref) {
		snprintf(dir, sizeof dir, "%s/Movies", data);
		LoadStrDir(dir);
		snprintf(dir, sizeof dir, "%s/Music", data);
		LoadStrDir(dir);
		if (!nframes)
			printf("SKIPPED the STR frames: none under %s\n", data);
		TestGameFrames();
		TestSuspend();
		TestCorrupt();
	} else {
		printf("SKIPPED the retail comparison: can't load %s\n", exe);
	}
	printf("%d/%d checks passed\n", checks - failures, checks);
	return failures != 0;
}
