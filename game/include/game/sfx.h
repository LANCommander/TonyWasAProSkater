/* Sound effects (SFX.CPP; the PAL module in functions.tsv): playing, positioning and
   stopping VAB voices. */
#ifndef GAME_SFX_H
#define GAME_SFX_H

#include "game/types.h"
#include "game/vector.h"

/* A sound effect: an index into the voice table DAT_800b8a9c (game/data/unnamed.cpp), which gives
   each one its VAB program/tone, loop bit and volume. The names are the THPS2 source's SND_
   #defines (from its sk2.vtg tag database, as game/script.h's names): THPS2's table grew out of
   THPS1's, and up to 174 the two agree. The ids that play from the general bank are skate2.vab's
   in a level and shell.vab's (144-151, 157) in the front end. The rest play from the level's own
   bank (Levels[].pVab), so they only mean something in the levels whose VAB fills that slot.
   The rolling loops follow the terrain (CBruce::HandleContinuousSounds), the bouncy objects'
   sounds their EBouncyType (CPlatform::Bounce). */
enum ESfx : uint {
	/* the general bank: skate2.vab */
	SND_CONCRETE = 0,         /* rolling loop: concrete, pool, and the default; a power-up pick-up */
	SND_WOOD = 1,             /* rolling loop */
	SND_FOOTSLIDE = 2,        /* braking, a female skater's bail; menu back */
	SND_BUSH3 = 3,
	SND_BUSH5 = 4,
	SND_LAND = 5,             /* also menu accept */
	SND_OLLIE = 6,            /* also the menu cursor */
	SND_WHEELSPIN = 7,        /* the loop in the air */
	SND_WIPEOUT1 = 8,         /* a female skater's bail */
	SND_WIPEOUT2 = 9,
	SND_WIPEOUT3 = 10,
	SND_BOARDSLIDE = 11,      /* loop */
	SND_METALGRIND = 12,      /* loop: grinds and slides on a RAIL_TYPE_METAL rail */
	SND_GRIND = 13,           /* loop */
	SND_14 = 14,              /* (no THPS2 name) */
	SND_WALLRIDE2 = 15,       /* loop */
	SND_METALOFF2 = 16,       /* leaving a RAIL_TYPE_METAL rail from a slide */
	SND_COUNTDING = 17,       /* cycling a letter (high score initials, the HORSE word) */
	SND_COLLECT56 = 18,       /* a credit pick-up (pitched by its value); high score entered */
	SND_COLLECT70 = 19,       /* a tape got */
	SND_COLLECT79 = 20,       /* an S-K-A-T-E letter; a trick object hit in a career game */
	SND_GOTATAPE = 21,
	SND_TIMEOUT = 22,         /* the last seconds of the clock */
	SND_PAUSEON = 23,
	SND_MARKUP2 = 24,         /* a graffiti tag */
	SND_COLLECT27 = 25,       /* a graffiti tag stolen */
	SND_HORSELETTER = 26,
	SND_UPPEDANTE = 27,
	SND_GRUNT = 28,           /* the first of the male grunts (SND_GRUNT + Rnd(3)) */
	SND_GRUNT2 = 28,
	SND_GRUNT3 = 29,
	SND_GRUNT4 = 30,
	SND_GRUNTGIRL = 31,
	SND_OW = 32,
	SND_DIRT = 33,            /* rolling loop */
	SND_FOOT1 = 34,           /* pushing off */
	SND_LANDMETAL4 = 35,      /* landing on a tin roof or metal */
	SND_LANDPOLE1 = 36,       /* onto a RAIL_TYPE_METAL rail (or SND_LANDRAIL3) */
	SND_LANDRAIL3 = 37,
	SND_LANDWOOD2 = 38,       /* landing on wood or a RAIL_TYPE_WOOD rail (or SND_LANDWOOD6), a
	                             handplant, hitting the ground in a bail */
	SND_LANDWOOD6 = 39,
	SND_SMACKMETAL = 40,      /* hitting a metal pole, a car plant, a BOUNCY_TRASHCAN */
	SND_HEADCRACK2 = 41,
	SND_GLASS1 = 42,          /* glass shattering (SND_GLASS2 in School and Downtown); a HORSE
	                             letter */
	SND_SPLASH1 = 43,         /* into water */

	/* the level's bank */
	SND_CROWDAMBIENCE = 44,   /* the competitions' crowd loop */
	SND_SCREAM3 = 45,         /* a medal won */
	SND_CROWDAPPLAUSE = 46,
	SND_CROWDOW = 47,         /* no medal */
	SND_CROWDCHEER = 48,
	SND_CROWDAMAZED = 49,     /* also a bail */
	SND_CROWDBOO = 50,
	SND_SCORE4 = 51,          /* the judges' total shown */
	SND_LEADER5 = 52,         /* a judge's score shown; the ladder */
	SND_FENCEHIT2 = 53,       /* hitting a chain-link fence */
	SND_ROLLPOOL = 54,
	SND_ROLLMETAL2 = 55,      /* rolling loop */
	SND_ROLLROUGH1 = 56,      /* rolling loop: rock, and asphalt (but in Streets) */
	SND_BRICKROLL1 = 57,      /* rolling loop */
	SND_GLASS2 = 58,
	SND_ACHUM = 59,
	SND_TRASHHIT1 = 60,
	SND_ROLLGRASS = 61,       /* rolling loop */
	SND_SLOWFAN2 = 62,
	SND_POLEHIT = 63,
	SND_HITDUMPSTER = 64,
	SND_GYMDOOR1 = 65,
	SND_SCHOOLBELL1 = 66,
	SND_SCHOOLBELL2 = 67,
	SND_LIGHTSON2 = 68,
	SND_TRASHBOUNC1 = 69,
	SND_FLOOR = 70,
	SND_ROLLROOF = 71,        /* rolling loop: tin roof */
	SND_WATERROLL2 = 72,      /* rolling loop */
	SND_TILEROLL1 = 73,       /* rolling loop (but in Roswell) */
	SND_TILEROLL2 = 74,
	SND_ELEV = 75,
	SND_SMALLFALLS = 76,
	SND_BIGFALLS1 = 77,
	SND_NEON1 = 78,
	SND_FENCEHIT2A = 79,
	SND_CHAIRBOUNCE1 = 80,    /* a BOUNCY_PLASTIC bouncing (or SND_CHAIRBOUNCE2) */
	SND_CHAIRBOUNCE2 = 81,
	SND_CHAIRHIT1 = 82,       /* a skater hitting a BOUNCY_PLASTIC */
	SND_BOXHIT1 = 83,         /* a BOUNCY_WOODENBOX, or a BOUNCY_CONE in Warehouse, hit or bouncing
	                             (or SND_BOXHIT2) */
	SND_BOXHIT2 = 84,
	SND_DING = 85,
	SND_SPARKS2 = 86,
	SND_SPARKS3 = 87,
	SND_SPARKS7 = 88,
	SND_ELEVATORMUSIC = 89,
	SND_ESCALATOR = 90,
	SND_BARRELHIT1 = 91,
	SND_BARRELBOUNC1 = 92,
	SND_HYDRANT = 93,
	SND_VALVE1 = 94,
	SND_RIVER1 = 95,
	SND_RIVER2 = 96,
	SND_GEN1 = 97,
	SND_SPLASH2 = 98,
	SND_SPLASH3 = 99,
	SND_CRATE1 = 100,
	SND_CRATE2 = 101,
	SND_WOODFALL1 = 102,      /* a BOUNCY_BARRIER bouncing (or SND_WOODFALL2) */
	SND_WOODFALL2 = 103,
	SND_WOODHORSE3 = 104,     /* a skater hitting a BOUNCY_BARRIER */
	SND_GRAVEL2 = 105,        /* rolling loop */
	SND_CONEBOUNCE1 = 106,    /* a BOUNCY_CONE (but in Warehouse) hit or bouncing (or
	                             SND_CONEBOUNCE2) */
	SND_CONEBOUNCE2 = 107,
	SND_SIGNHIT1 = 108,
	SND_STEAM1 = 109,
	SND_TAXIHORN = 110,       /* (or SND_TAXIHORN2, SND_TAXIHORN3) */
	SND_TAXIHORN2 = 111,
	SND_TAXIHORN3 = 112,
	SND_TAXI2 = 113,          /* the taxi's loop */
	SND_CARHIT1 = 114,        /* a car hitting a skater, then the car's own sound */
	SND_CABDRIVER1 = 115,     /* the taxi's (or SND_CABDRIVER2..4) */
	SND_CABDRIVER2 = 116,
	SND_CABDRIVER3 = 117,
	SND_CABDRIVER4 = 118,
	SND_ROLLSIDEWALK = 119,   /* rolling loop */
	SND_DINGDING = 120,       /* the cable car's horn and hit */
	SND_POLICECAR3 = 121,
	SND_SIREN1 = 122,         /* the police car's horn */
	SND_BIGFALLS2 = 123,
	SND_SIRENSMASH2 = 124,
	SND_COP1 = 125,           /* the police car's hit (or SND_COP2..4) */
	SND_COP2 = 126,
	SND_COP3 = 127,
	SND_COP4 = 128,
	SND_VWBUS = 129,          /* the van's loop */
	SND_VWHORN2 = 130,        /* its horn */
	SND_VWHORN3 = 131,        /* its hit */
	SND_CANVASLAND = 132,
	SND_CANVASROLL = 133,
	SND_CABLECAR1B = 134,     /* the cable car's loop */
	SND_135 = 135,            /* (no THPS2 name) */
	SND_FENCEHIT = 136,
	SND_SPARK8 = 137,         /* Roswell's SND_SMACKMETAL */
	SND_BIGDOOROPEN2 = 138,
	SND_TRUCKIDLE2 = 139,
	SND_ALIEN1 = 140,
	SND_GRID = 141,
	SND_HELI1 = 142,
	SND_ROLLALIEN2 = 143,     /* rolling loop: Roswell's metal */

	/* the general bank in the front end: shell.vab */
	SND_MENUCHANGE2 = 144,    /* cursor */
	SND_SELECT1 = 145,
	SND_NOSELECT1 = 146,      /* a locked choice */
	SND_GOBACK2 = 147,
	SND_SELECTBOARD1 = 148,   /* left/right through the skaters, boards... */
	SND_SELECTBOARD3 = 149,   /* the board, trucks or wheels picked */
	SND_SELECTTRUCK2 = 150,
	SND_SELECTWHEEL = 151,

	/* the level's bank */
	SND_DRIP1 = 152,
	SND_DRIP2 = 153,
	SND_DRIP3 = 154,
	SND_CHAINCREAK = 155,
	SND_CHAINCREAK2 = 156,

	/* the general bank */
	SND_MUSICLOOP1 = 157,     /* the front end's music loop (Front_SetMusicLoop) */
	SND_MUSICLOOP2 = 158,
	SND_SECRETTAPE = 159,
	SND_BUZZER = 160,         /* a competition's run over */
	SND_GAP = 161,
	SND_SPECIAL = 162,        /* a special trick */

	/* the level's bank */
	SND_CRICKET2 = 163,
	SND_HELIPASS = 164,
	SND_RADAR = 165,
	SND_ALIENGROAN1 = 166,
	SND_ALIENGROAN2 = 167,
	SND_HOWL1 = 168,
	SND_ASPHALT = 169,        /* rolling loop: Streets' asphalt */
	SND_ALIENGRIND = 170,     /* Roswell's SND_METALGRIND */
	SND_PLEXIGLASS = 171,     /* rolling loop: Roswell's tile */

	/* the general bank */
	SND_MUSICLOOP3 = 172,
	SND_MUSICLOOP4 = 173,
	SND_MUSICLOOP5 = 174,

	SND_NONE = 0xffff, /* (no THPS2 name) plays nothing, as does any negative id */
};

/* functions */
class CBruce;
void SFX_InitAtStart();
void SFX_Init(char *a);
void FUN_8004b0e4(char *s);
void SFX_SpoolInMIDI(const char *name);
void FUN_8004b180();
int SFX_PlayX(ESfx id, short vol, int pan);
int SFX_GetVoicePitch(unsigned v);
void SFX_SetVoicePitch(unsigned m, unsigned short p);
void SFX_ModifyPos(unsigned a, const CVector &pos, int b);
void SFX_ModifyVol(unsigned mask, int a, int b);
void FUN_8004b8e0();
void FUN_8004b8e8();
void SFX_Pause();
void SFX_Unpause();
void SFX_Stop(unsigned m);
void FUN_8004ba34();
void SFX_SetReverbType(unsigned char t);
void SFX_ReverbOff();
void SFX_SetReverbDepth(unsigned char l, unsigned char r);
void SFX_MIDIFadeOut(int speed);
void FUN_8004bb5c();
void SFX_MIDIFadeIn(int speed);
void SFX_MIDIChannelMute(unsigned short mask);
void SFX_ProcessingPerFrame();

int SFX_PlayLR(ESfx id, short l, short r, int pan);
int SFX_PlayPos(ESfx id, const CVector &pos, int pan);
int playSFX(uint sfx, uchar pitch, short vl, short vr, int pitch_offset);

/* globals */
extern uint SFX_ReverbDepthSetTimer;
#endif
