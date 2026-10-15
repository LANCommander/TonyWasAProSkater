/* FUN_80065520 (0x80065520) */
extern char DAT_800a2130[];

/* FileLoader (game/shellutils.h) */
typedef struct {
	char *vt;
	int mMode;
	int mNum;
} FL;

FL *FUN_80065520(FL *p) {
	p->vt = DAT_800a2130;
	p->mNum = 0;
	p->mMode = 0;
	return p;
}
