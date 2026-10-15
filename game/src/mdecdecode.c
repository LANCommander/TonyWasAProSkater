/* mdecdecode.s as C: the VLC stage of STR video decoding (DecDCTvlcBuild3/DecDCTvlc3).
   Not byte-matching by design; the port's interpretation of the retail assembly. */
#include "game/scratchpad.h"

extern unsigned char DecDCTvlcCodes3[216];
/* Output budget of one call, in halfwords: DecDCTvlc3 suspends at the first macroblock (block, for
 * version 2) boundary past it, and DecDCTvlc3(0, ...) carries on. Nothing in THPS changes it. */
extern unsigned int DecDCTvlcMaxSize3;
/* A suspended decode: the retail code saves its registers here, in this order. */
struct VlcState {
	unsigned int bits; /* the next 32 bits of the stream, MSB first */
	unsigned int next; /* the 32 after them, of which 32 - used are valid */
	unsigned int peek; /* v3: the 10 bits after the last EOB; v2: the next block's DC */
	unsigned int *in;  /* the stream word after `word` */
	unsigned short *out;
	unsigned int word;      /* the stream word after `next`, as stored */
	int used;               /* bits of `next` already shifted into `bits` */
	unsigned int quant;     /* quantiser scale << 10, ORed into every DC */
	unsigned short *out_end; /* where the MDEC header says the output ends */
	int dc_cr, dc_cb, dc_y; /* v3 DC predictors */
	unsigned int v2;        /* 0 for a v3 frame (retail: the return address of its v2 loop) */
};
struct VlcState DecDCTvlcState3;
#define VLC_TABLE ((unsigned short *)SCRATCHPAD) /* scratchpad */
#define EOB       0xfe00

/* DecDCTvlcBuild3 (0x80022498) -- C interpretation of hand-written assembly (mdecdecode.s) */
/* Also DecDCTvlc3 (0x800224e4). Not byte-matching by design: listed in reimpl.tsv, verified against
 * the retail MIPS by platform/pc/tests/test_decdctvlc.c.
 *
 * The VLC stage of STR video decoding: turns a BS frame (version 2 or 3; MPEG-1 style codes) into
 * the run-level halfwords the MDEC takes. The labels inside the retail DecDCTvlc3 that other code
 * never calls (0x800225f0 .. 0x80022994) are entry points of its internal subroutines. */
/* DecDCTvlcBuild3 expands this into the scratchpad table DecDCTvlc3 reads: one (run, level) pair
 * per AC code, grouped by code shape (see AcCode). Comments give the codes, s being the sign. */
void DecDCTvlcBuild3(void) {
	unsigned short *t = VLC_TABLE;
	unsigned char *c;
	int i;

	/* each code's halfword for a positive and a negative level: run << 10 | level (10 bits). The
	 * retail loop runs once more than there are pairs, taking the low two bytes of the next word
	 * (DecDCTvlcMaxSize3) as a 109th: only a corrupt stream ever indexes that entry. */
	for (i = 0; i <= 108; i++, t += 2) {
		c = i < 108 ? DecDCTvlcCodes3 + 2 * i : (unsigned char *)&DecDCTvlcMaxSize3;
		t[0] = c[0] << 10 | c[1];
		t[1] = c[0] << 10 | (-c[1] & 1023);
	}
}

/* The GTE's LZCS/LZCR: leading bits equal to the sign bit (32 for 0 and -1). */
static int Lzc(unsigned int x) {
	int n = 0;

	if ((int)x < 0)
		x = ~x;

	while (n < 32 && !(x & 0x80000000u)) {
		x <<= 1;
		n++;
	}

	return n;
}

static unsigned int Swap(unsigned int w) {
	return w << 16 | w >> 16; /* the stream is 16-bit little-endian units */
}

/* Shift n bits out of the window, refilling `next` from the stream. Shift amounts are taken mod 32
 * as by the R3000's sllv/srlv: only a corrupt stream gets n = 0 or n >= 32 here, and the refill
 * happens at most once per call. */
static void Skip(struct VlcState *v, int n) {
	unsigned int w;

	v->bits = v->bits << (n & 31) | v->next >> (-n & 31);
	v->next <<= n & 31;
	v->used += n;

	if (v->used < 32)
		return;

	v->used -= 32;
	w = Swap(v->word);

	if (v->used)
		v->bits |= w >> (-v->used & 31);

	v->next = w << (v->used & 31);
	v->word = *v->in++;
}

/* Decode the AC code at the top of `b` (not an EOB): returns its halfword, sets *len. */
static unsigned short AcCode(unsigned int b, int *len) {
	const unsigned short *t = VLC_TABLE;
	int lz;

	if ((int)b < 0) { /* 11s: run 0, level 1 */
		*len = 3;
		return (int)(b << 2) < 0 ? 1023 : 1;
	}

	if ((int)(b << 1) < 0) {
		if ((int)(b << 2) < 0) { /* 011s: run 1, level 1 */
			*len = 4;
			return (int)(b << 3) < 0 ? 2047 : 1025;
		}

		*len = 5;

		if ((int)(b << 3) < 0) /* 0101s: run 2, level 1 */
			return (int)(b << 4) < 0 ? 3071 : 2049;

		return (int)(b << 4) < 0 ? 1022 : 2; /* 0100s: run 0, level 2 */
	}

	if ((int)(b << 2) < 0) {
		if ((b << 3) >> 30) { /* 001xxs, xx != 00 */
			*len = 6;
			return t[206 + ((b >> 26) & 7)];
		}

		*len = 9; /* 00100xxxs (the last pair is never indexed) */
		return t[192 + ((b >> 23) & 15)];
	}

	lz = Lzc(b);

	if (b >> 26 == 1) { /* 000001: escape, then 6 bits of run and 10 of level */
		*len = 22;
		return (b >> 10) & 0xffff;
	}

	if (b >> 26) { /* 0001xxs, 00001xxs */
		*len = lz + 4;
		return t[(lz - 3) * 8 + ((b << (lz & 31)) >> 28 & 7)];
	}

	if (lz <= 6) { /* 0000001xxxs */
		*len = 11;
		return t[16 + ((b << (lz & 31)) >> 27 & 15)];
	}

	/* 7 to 11 zeros, then 1 and 5 bits; more zeros can only come from a corrupt stream, which
	 * indexes past the table (on the PS1, past the scratchpad from 22 zeros) */
	*len = lz + 6;
	return t[(lz - 6) * 32 + ((b << (lz & 31)) >> 26 & 31)];
}

/* Chroma DC size code + difference at the top of `b`; adds it to *dc, returns the code length. */
static int DcChroma(unsigned int b, int *dc) {
	unsigned int x, s;
	int k;

	if ((int)b >= 0) {
		if ((int)(b << 1) >= 0) /* 00: size 0 */
			return 2;

		*dc += (int)(b << 2) < 0 ? 1 : -1; /* 01s: size 1 */
		return 3;
	}

	k = Lzc(b); /* k ones, a zero, then k + 1 bits */
	x = (b << 1) << (k & 31);
	s = (unsigned int)(-1 - k) & 31;
	*dc += (int)(x >> s);

	if ((int)x >= 0) /* a leading 0 bit: negative */
		*dc -= (int)(0xffffffffu >> s);

	return 2 * (k + 1);
}

/* Luma DC size code + difference at the top of `b`; adds it to *dc, returns the code length. */
static int DcLuma(unsigned int b, int *dc) {
	unsigned int x, s;
	int k;

	if ((int)b >= 0) {
		if ((int)(b << 1) >= 0) { /* 00s: size 1 */
			*dc += (int)(b << 2) < 0 ? 1 : -1;
			return 3;
		}

		x = b << 2; /* 01xx: size 2 */
		*dc += (int)(x >> 30) - ((int)x < 0 ? 0 : 3);
		return 4;
	}

	if ((int)(b << 1) >= 0) {
		if ((int)(b << 2) >= 0) /* 100: size 0 */
			return 3;

		x = b << 3; /* 101xxx: size 3 */
		*dc += (int)(x >> 29) - ((int)x < 0 ? 0 : 7);
		return 6;
	}

	k = Lzc(b); /* k ones, a zero, then k + 2 bits */
	x = (b << 1) << (k & 31);
	s = (unsigned int)(-2 - k) & 31;
	*dc += (int)(x >> s);

	if ((int)x >= 0)
		*dc -= (int)(0xfffffffeu >> s);

	return 2 * k + 3;
}

/* One block: its DC halfword, then AC codes up to the EOB. `len` is the length of the DC code
 * plus whatever precedes it at the top of the window: a block's EOB is written but its 2 bits
 * are left there, for the next DC's length to count. */
static void Block(struct VlcState *v, unsigned short dc, int len) {
	int n;

	*v->out++ = dc;
	Skip(v, len);

	while ((v->bits >> 30) != 2) {
		*v->out++ = AcCode(v->bits, &n);
		Skip(v, n);
	}

	*v->out++ = EOB;
}

static unsigned short Dc(struct VlcState *v, int dc) {
	return (((unsigned int)dc << 2) & 1023) | v->quant;
}

static int Full(struct VlcState *v, unsigned short *limit) {
	return (intptr_t)((uintptr_t)v->out - (uintptr_t)limit) >= 0;
}

/* Decode a BS frame into buf: the MDEC header word, then run-level halfwords up to the size in
 * that header (padded with EOBs). bs = 0 resumes a suspended decode where it left off; buf is
 * then ignored. Returns 1 when suspended (see DecDCTvlcMaxSize3), 0 when the frame is done. */
int DecDCTvlc3(unsigned int *bs, unsigned int *buf) {
	struct VlcState v;
	unsigned short *limit;
	int i;

	if (!bs) {
		v = DecDCTvlcState3;
		limit = (unsigned short *)((char *)v.out + (DecDCTvlcMaxSize3 << 1));

		if (v.v2)
			goto v2_block;

		goto v3_next;
	}

	limit = (unsigned short *)((char *)buf + (DecDCTvlcMaxSize3 << 1));
	*buf = bs[0];
	v.out = (unsigned short *)(buf + 1);
	v.out_end = (unsigned short *)(buf + 1 + (bs[0] & 0xffff));
	v.quant = (bs[1] & 63) << 10;
	v.bits = Swap(bs[2]);
	v.next = Swap(bs[3]);
	v.word = bs[4];
	v.in = bs + 5;
	v.used = 0;
	v.dc_cr = v.dc_cb = v.dc_y = 0;

	if ((bs[1] >> 16) == 3) {
		v.v2 = 0;

		for (;;) {
			/* a macroblock: Cr, Cb, then four Y blocks; DCs are differences from the last one
			 * of the same kind */
			i = DcChroma(v.bits, &v.dc_cr);
			Block(&v, Dc(&v, v.dc_cr), i);

			i = DcChroma(v.bits << 2, &v.dc_cb);
			Block(&v, Dc(&v, v.dc_cb), i + 2);

			for (i = 0; i < 4; i++) {
				int n = DcLuma(v.bits << 2, &v.dc_y);

				Block(&v, Dc(&v, v.dc_y), n + 2);
			}

			v.peek = (v.bits << 2) >> 22;

			if (Full(&v, limit))
				goto suspend;
		v3_next:
			if (v.peek == 0x3ff) /* end of frame */
				break;

			/* the EOB. The retail code drops its 2 bits without refilling, leaving the window's
			 * last bit stale until the next refill; the Cr DC code read before that can't reach
			 * it (fewer than 10 leading ones, or it would be the end code), so refilling now is
			 * the same. */
			Skip(&v, 2);
		}
	} else {
		/* version 2: every DC is 10 bits as is */
		v.v2 = 1;
		Block(&v, (v.bits >> 22) | v.quant, 10);

		for (;;) {
			v.peek = (v.bits << 2) >> 22;

			if (v.peek == 0x1ff) /* end of frame */
				break;

			v.peek |= v.quant;

			if (Full(&v, limit))
				goto suspend;
		v2_block:
			Block(&v, v.peek, 12);
		}
	}

	while ((intptr_t)((uintptr_t)v.out - (uintptr_t)v.out_end) < 0)
		*v.out++ = EOB;

	return 0;

suspend:
	DecDCTvlcState3 = v;
	return 1;
}
