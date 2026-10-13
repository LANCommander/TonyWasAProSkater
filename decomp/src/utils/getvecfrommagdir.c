/* Utils_GetVecFromMagDir__FR7CVectoriRC8CSVector (0x80047b24) */
typedef struct {
	int x, y, z;
} V;

typedef struct {
	unsigned short x, y, z;
} SV;

typedef struct {
	short s, c;
} SC;

extern SC rcossin_tbl[1]; /* declared <= 8 bytes: one la, as retail */

#define rsin(a)                                                                                    \
	({                                                                                             \
		SC *p_ = &rcossin_tbl[(a) & 4095];                                                         \
		p_->s;                                                                                     \
	})
#define rcos(a)                                                                                    \
	({                                                                                             \
		SC *p_ = &rcossin_tbl[(a) & 4095];                                                         \
		p_->c;                                                                                     \
	})

void Utils_GetVecFromMagDir(V *out, int mag, SV *dir) {
	out->x = -(((mag * rcos(dir->x)) >> 12) * rsin(dir->y));
	out->y = mag * rsin(dir->x);
	out->z = -(((mag * rcos(dir->x)) >> 12) * rcos(dir->y));
}
