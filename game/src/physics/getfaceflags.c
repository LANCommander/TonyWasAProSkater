/* GetFaceFlags__FP9SLineInfo (0x8002ee6c) */
typedef struct {
	unsigned flags0;
	int _4, _8;
	unsigned flags;
} SFace;

typedef struct {
	char _0[0x68];
	int hit;
	char _6c[0x80 - 0x6c];
	SFace *face;
} SLineInfo;

int Face_Air, Face_Skateable, Face_WallRide, Face_Visible, Face_Terrain;

void GetFaceFlags(SLineInfo *li) {
	if (li->hit) {
		SFace *f = li->face;
		unsigned fl = f->flags;

		Face_Air = (fl >> 16) & 0x40;
		Face_Skateable = !(fl & 0x1000000);
		Face_WallRide = !(fl & 0x800000);
		Face_Visible = f->flags0 & 0x80;
		Face_Terrain = ((fl >> 16) & 0x1e00) >> 9;
	}
}
