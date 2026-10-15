/* Decompiled data: the retail executable's initialised globals that src/backgrnd/ uses, each at
 * its retail address (psxdata.h). Text is string literals where it's used and the tables have
 * the game's types; platform/pc/tools/data_image.py checks it all against the retail bytes. */
#include "psxdata.h"

struct CBody;

/* THPS2 demo: char* */
PSX_DATA(0x800d19f0) char *pCurrentObjFile;

/* THPS2 demo: CBody* */
PSX_DATA(0x800d1fe4) CBody *BackgroundList;

/* THPS2 demo: ushort */
PSX_DATA(0x800d1fe8) unsigned short CurrentIndex[2];
