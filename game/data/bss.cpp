/* Decompiled data: the zero-initialised globals the game code shares that no module's data file
 * holds (the retail executable's .bss), in retail order, typed as the code uses them (pointers as
 * pointers, so that a 64-bit build has room for them). Value types without pointers are plain
 * arrays of their size. */
#include "psxdata.h"
#include "game/career.h"
#include "game/panel.h"
#include "game/tricktips.h"

struct CBruce;
struct SlicedImage;
struct Sprite;
struct STrig;
struct SViewport;
struct Texture;

/* rfront: the memory card screens */
int Delay;
int Retries;
void *Tri1;
void *Tri2;
int SaveNeedBlocks;
int SaveFreeBlocks;
int GReturnToState;
int soundVol;
int musicVol;

/* mess */
unsigned int LastFlagsBGR;

/* flash: the flash colour (16.16 fixed point) and its steps */
int CurrentR;
int CurrentG;
int CurrentB;
int dR;
int dG;
int dB;

/* shatter */
int VRAMClut;
int VRAMTPage;
unsigned char AverageR;
unsigned char AverageG;
unsigned char AverageB;

/* gamefmv */
unsigned int *pRingBuffer;

/* utils: Rnd's state */
int DAT_800d2230;
int RandSeed;
int RandMul;
int RandAdd;

/* fileio */
int FileSize;
int WadStart;
void *Dest;
int NumSectors;
int Start;

/* FUN_800502bc, FUN_800507e4 and the credits */
int DAT_800d226c;
int DAT_800d2270;
int DAT_800d2274;
int DAT_800d2278;
int DAT_800d227c;
void *DAT_800d2280;
int DAT_800d2284;
int DAT_800d2288;
int DAT_800d228c;
short DAT_800d2294[4];
CBruce *DAT_800d229c;
SViewport *DAT_800d22a0;
int SKATER_Y_POS;

/* front: the two-player board select */
int DAT_800d22a8;
int DAT_800d22ac;
SlicedImage *DAT_800d22b0;
int DAT_800d22b8[2];
char *DAT_800d22c0[2];
unsigned char DAT_800d22c8;
int DAT_800d22cc;
int DAT_800d22d0[2];

/* the one-player board select */
int DAT_800d22d8;
SlicedImage *DAT_800d22dc;
int DAT_800d22e0;
char *DAT_800d22e4;
unsigned char DAT_800d22e8;
int DAT_800d22ec;
int DAT_800d22f0;
int DAT_800d22f4;

/* the skater selects */
int DAT_800d22f8;
int DAT_800d22fc;
int DAT_800d2300;
int DAT_800d2304[2];
int DAT_800d230c;
int DAT_800d2310;
int DAT_800d2314;
int DAT_800d231c[2];
int DAT_800d2324;
int DAT_800d2328;
int DAT_800d232c;
int DAT_800d2330;
int DAT_800d2334;
int DAT_800d2338;
int DAT_800d233c;
int DAT_800d2340;
int DAT_800d2344;
int DAT_800d2348;
int DAT_800d234c;
unsigned char DAT_800d2350;

/* the front end's main menu */
Sprite *DAT_800d2358;
Sprite *DAT_800d235c;

/* the career level select */
int DAT_800d2360;
int DAT_800d2364;
int DAT_800d2368;
int DAT_800d236c;
int DAT_800d2370;
int DAT_800d2374;
int DAT_800d2378;
int DAT_800d237c;
int DAT_800d2380;
int DAT_800d2384;
int DAT_800d2388;
int DAT_800d238c;
int DAT_800d2390;
void *DAT_800d2398[2]; /* TSPack * */
void *DAT_800d23a0[2];
void *DAT_800d23a8;
void *DAT_800d23ac;
int DAT_800d23b0[2];
int DAT_800d23b8[2];

/* spool (the texture list) */
int Urgent;
int HashIndex;
Texture *pCurrentTex;
int DAT_800d23e4;
int DAT_800d23e8;
int DAT_800d23ec;
int DAT_800d23f0;
int DAT_800d23f4;
int DAT_800d23f8;
int DAT_800d23fc;
int DAT_800d2400;

/* camera */
int old_camYOffset;
int old_camDist;
int old_camPitch;
short LandShakeSpeed[4]; /* CSVector */
unsigned char LandShakeDecay[4]; /* CFriction */
short SmallShakeAmp[4];
short MediumShakeAmp[4];
short BigShakeAmp[4];

/* FUN_800785cc */
int DAT_800d2464;
int DAT_800d2468;
int DAT_800d246c;

/* a text buffer */
char msg[240];

/* shatter: CVectors */
int ShatterSoundPos[3];
int DAT_800d2574[3];
char DAT_800d2580[100];
int DAT_800d25e4[3];
char DAT_800d25f0[32];

/* trig: the trigger hash table */
STrig *HashTable[256];

/* pal: the free palette slots */
char Free16Slots[256];
char Free256Slots[76];

/* sfx: per voice */
unsigned short SFXFalloffArray[24];
int pitchPriorToPause[25];

/* FUN_800507e4 */
int DAT_800d2bf0[11];

/* front: the board selects' pictures and their state */
int DAT_800d2c1c[3];
char DAT_800d2c2c[2][3][15][16];
SlicedImage *DAT_800d31cc[2][3][15];
int DAT_800d333c[2][3];
int DAT_800d335c[2][3];
int DAT_800d337c[2][3];
int DAT_800d3394[3];
int DAT_800d33a4[3];
int DAT_800d33b4[3];
int DAT_800d33c4[3];
char DAT_800d33d4[3][15][16];
SlicedImage *DAT_800d36a4[3][15];

/* the skater selects and the career level select */
int DAT_800d3758[MAX_SKATERS + 1];
int DAT_800d3788[MAX_SKATERS];
int DAT_800d37b4[MAX_SKATERS + 1];
int DAT_800d37e4[MAX_SKATERS];
char *DAT_800d3810[MAX_LEVELS + 2];
int DAT_800d3840[MAX_LEVELS];
char DAT_800d3868[32];

/* mess */
char Msg[64];

/* tricktips (its static constructor builds it) */
PsxStorage<CTrickTipMonitor> TrickTipMonitor;

/* the demo and fly-through cameras' focus points ({x, y, z, ?}) */
int DemoFocus_8[4];
int DAT_800d3a04[4];
int DAT_800d3a14[4];
int DAT_800d3a24[4];
int DAT_800d3a34[4];
int DAT_800d3a44[4];
int DAT_800d3a54[4];
int DAT_800d3a64[4];
int DAT_800d3a74[4];
int DAT_800d3a84[4];
int DAT_800d3a94[4];
int FlyFocus[4];
int DAT_800d3ab4[4];
