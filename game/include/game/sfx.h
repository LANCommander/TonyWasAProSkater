/* Sound effects (SFX.CPP; the PAL module in functions.tsv): playing, positioning and
   stopping VAB voices. */
#ifndef GAME_SFX_H
#define GAME_SFX_H

#include "game/types.h"
#include "game/vector.h"

/* A sound effect: an index into the voice table DAT_800b8a9c (game/data/unnamed.cpp), which gives
   each one its VAB program/tone, loop bit and volume. The original names are lost. These come from
   where the game plays them. The ids that play from the general bank are skate2.vab's in a level
   and shell.vab's (SFX_SHELL_*, SFX_MENU_*) in the front end. The rest play from the level's own
   bank (Levels[].pVab), so they only mean something in the levels whose VAB fills that slot. An id
   that's only played from level scripts (TRG_PLAYSOUND, C_PLAY_SFX...) is named after its level.
   A bare number means the id is never played, by code or by script. Terrain numbers are
   CBruce::mTerrain / Face_Terrain values. */
enum ESfx : uint {
	/* the general bank: skate2.vab */
	SFX_ROLL = 0,             /* rolling loop: terrain 11 and default; a power-up pick-up */
	SFX_ROLL_TERRAIN2 = 1,    /* rolling loop */
	SFX_SKID = 2,             /* braking, a female skater's bail; menu back */
	SFX_3 = 3,                /* Downtown, Mall, Streets and Warehouse scripts */
	SFX_4 = 4,                /* Downtown, Mall, Streets and Warehouse scripts */
	SFX_LAND = 5,             /* landing on most terrains; menu accept */
	SFX_MENU_MOVE = 6,        /* menu cursor */
	SFX_AIR_LOOP = 7,         /* the wind loop in the air */
	SFX_BAIL_FEMALE = 8,
	SFX_BAIL_MALE_1 = 9,
	SFX_BAIL_MALE_2 = 10,
	SFX_GRIND_LOOP = 11,      /* a grind on a kind 0/2 rail */
	SFX_GRIND_LOOP_RAIL1 = 12, /* grind or slide on a kind 1 rail */
	SFX_SLIDE_LOOP = 13,      /* a slide on a kind 0/2 rail */
	SFX_14 = 14,
	SFX_WALLRIDE_LOOP = 15,   /* also a Downhill Jam script */
	SFX_RAIL_OFF = 16,        /* leaving a kind 1 rail from a slide */
	SFX_MENU_LETTER = 17,     /* cycling a letter (high score initials, the HORSE word) */
	SFX_PONG = 18,            /* a credit pick-up (pitched by its value); high score entered */
	SFX_TAPE = 19,            /* a tape got */
	SFX_LETTER = 20,          /* an S-K-A-T-E letter; a trick object hit in a career game */
	SFX_21 = 21,
	SFX_TIMER_TICK = 22,      /* the last seconds of the clock */
	SFX_23 = 23,
	SFX_GRAFFITI_TAG = 24,
	SFX_GRAFFITI_STEAL = 25,
	SFX_26 = 26,
	SFX_27 = 27,
	SFX_GRUNT_MALE_1 = 28,
	SFX_GRUNT_MALE_2 = 29,
	SFX_GRUNT_MALE_3 = 30,
	SFX_GRUNT_FEMALE = 31,
	SFX_32 = 32,
	SFX_ROLL_TERRAIN3 = 33,   /* rolling loop */
	SFX_KICK = 34,            /* pushing off */
	SFX_LAND_TERRAIN10 = 35,  /* landing on terrains 10 and 12 */
	SFX_RAIL_LAND_1 = 36,     /* onto a kind 1 rail (or SFX_RAIL_LAND_2) */
	SFX_RAIL_LAND_2 = 37,
	SFX_IMPACT_1 = 38,        /* landing on terrain 2 or a kind 2 rail (or SFX_IMPACT_2), a
	                             handplant, hitting the ground in a bail */
	SFX_IMPACT_2 = 39,
	SFX_CLANG = 40,           /* hitting a terrain 9 wall, a car plant, a bouncy object; also a
	                             Downhill Jam script */
	SFX_41 = 41,
	SFX_GLASS = 42,           /* glass shattering (SFX_GLASS_LEVEL in School and Downtown); a
	                             HORSE letter */
	SFX_SPLASH = 43,          /* into water */

	/* the level's bank */
	SFX_CROWD_LOOP = 44,      /* the competitions' crowd */
	SFX_CROWD_APPLAUSE = 45,  /* a medal won */
	SFX_CROWD_MURMUR = 46,
	SFX_CROWD_AWW = 47,       /* no medal */
	SFX_CROWD_CHEER = 48,
	SFX_CROWD_OOH = 49,       /* a bail */
	SFX_CROWD_BORED = 50,
	SFX_JUDGE_TOTAL = 51,     /* the judges' total shown */
	SFX_JUDGE_SCORE = 52,     /* a judge's score shown; the ladder */
	SFX_HIT_TERRAIN7 = 53,    /* hitting a terrain 7 wall */
	SFX_54 = 54,
	SFX_ROLL_TERRAIN12 = 55,  /* rolling loop */
	SFX_ROLL_TERRAIN13 = 56,  /* rolling loop: also terrain 4 (but in Streets) */
	SFX_ROLL_TERRAIN8 = 57,   /* rolling loop */
	SFX_GLASS_LEVEL = 58,     /* glass shattering in School and Downtown */
	SFX_59 = 59,
	SFX_60 = 60,
	SFX_ROLL_TERRAIN5 = 61,   /* rolling loop */
	SFX_SCHOOL_62 = 62,
	SFX_SCHOOL_63 = 63,
	SFX_64 = 64,
	SFX_SCHOOL_65 = 65,
	SFX_SCHOOL_66 = 66,
	SFX_67 = 67,
	SFX_SCHOOL_68 = 68,
	SFX_69 = 69,
	SFX_70 = 70,
	SFX_ROLL_TERRAIN10 = 71,  /* rolling loop */
	SFX_ROLL_TERRAIN6 = 72,   /* rolling loop */
	SFX_ROLL_TERRAIN1 = 73,   /* rolling loop (but in Roswell) */
	SFX_74 = 74,
	SFX_75 = 75,
	SFX_76 = 76,              /* Mall and Downtown scripts */
	SFX_MALL_77 = 77,
	SFX_78 = 78,
	SFX_79 = 79,
	SFX_BOUNCY4_BOUNCE_1 = 80, /* a type 4 bouncy object (CPlatform) bouncing (or _2) */
	SFX_BOUNCY4_BOUNCE_2 = 81,
	SFX_BOUNCY4_HIT = 82,     /* a skater hitting a type 4 bouncy object */
	SFX_BOUNCY_1 = 83,        /* a type 5 bouncy object, or type 2 in Warehouse, hit or bouncing
	                             (or _2); also Warehouse and Mall scripts */
	SFX_BOUNCY_2 = 84,        /* also Warehouse scripts */
	SFX_MALL_85 = 85,
	SFX_86 = 86,              /* Streets, Downtown and Warehouse scripts */
	SFX_87 = 87,              /* Streets, Downtown and Warehouse scripts */
	SFX_88 = 88,              /* Streets, Downtown and Warehouse scripts */
	SFX_MALL_89 = 89,
	SFX_90 = 90,
	SFX_91 = 91,
	SFX_92 = 92,
	SFX_JAM_93 = 93,
	SFX_JAM_94 = 94,
	SFX_JAM_95 = 95,
	SFX_JAM_96 = 96,
	SFX_JAM_97 = 97,
	SFX_JAM_98 = 98,
	SFX_JAM_99 = 99,
	SFX_JAM_100 = 100,
	SFX_JAM_101 = 101,
	SFX_BOUNCY1_BOUNCE_1 = 102, /* a type 1 bouncy object bouncing (or _2) */
	SFX_BOUNCY1_BOUNCE_2 = 103,
	SFX_BOUNCY1_HIT = 104,    /* a skater hitting a type 1 bouncy object */
	SFX_ROLL_TERRAIN14 = 105, /* rolling loop */
	SFX_BOUNCY2_1 = 106,      /* a type 2 bouncy object (but in Warehouse) hit or bouncing (or _2) */
	SFX_BOUNCY2_2 = 107,
	SFX_DOWNTOWN_108 = 108,
	SFX_109 = 109,
	SFX_TAXI_HORN_1 = 110,    /* (or _2, _3) */
	SFX_TAXI_HORN_2 = 111,
	SFX_TAXI_HORN_3 = 112,
	SFX_TAXI_ENGINE = 113,    /* loop */
	SFX_CAR_HIT = 114,        /* a car hitting a skater, then the car's own sound */
	SFX_TAXI_HIT_1 = 115,     /* (or _2.._4) */
	SFX_TAXI_HIT_2 = 116,
	SFX_TAXI_HIT_3 = 117,
	SFX_TAXI_HIT_4 = 118,
	SFX_ROLL_TERRAIN15 = 119, /* rolling loop */
	SFX_CABLE_CAR_BELL = 120, /* its horn and its hit */
	SFX_121 = 121,
	SFX_POLICE_HORN = 122,
	SFX_STREETS_123 = 123,
	SFX_STREETS_124 = 124,
	SFX_POLICE_HIT_1 = 125,   /* (or _2.._4); also Streets scripts */
	SFX_POLICE_HIT_2 = 126,
	SFX_POLICE_HIT_3 = 127,
	SFX_POLICE_HIT_4 = 128,
	SFX_VAN_ENGINE = 129,     /* loop */
	SFX_VAN_HORN = 130,
	SFX_VAN_HIT = 131,
	SFX_132 = 132,
	SFX_133 = 133,
	SFX_CABLE_CAR_LOOP = 134,
	SFX_135 = 135,
	SFX_136 = 136,
	SFX_ROSWELL_CLANG = 137,  /* Roswell's SFX_CLANG for a terrain 9 wall */
	SFX_ROSWELL_138 = 138,
	SFX_ROSWELL_139 = 139,
	SFX_ROSWELL_140 = 140,
	SFX_ROSWELL_141 = 141,
	SFX_ROSWELL_142 = 142,
	SFX_ROSWELL_ROLL_TERRAIN12 = 143, /* rolling loop */

	/* the general bank in the front end: shell.vab */
	SFX_SHELL_MOVE = 144,     /* cursor */
	SFX_SHELL_SELECT = 145,
	SFX_SHELL_DENIED = 146,   /* a locked choice */
	SFX_SHELL_BACK = 147,
	SFX_SHELL_SCROLL = 148,   /* left/right through the skaters, boards... */
	SFX_SHELL_PICK_1 = 149,   /* a choice in the first, second or third column */
	SFX_SHELL_PICK_2 = 150,
	SFX_SHELL_PICK_3 = 151,

	/* the level's bank */
	SFX_WARE_152 = 152,
	SFX_WARE_153 = 153,
	SFX_WARE_154 = 154,
	SFX_155 = 155,
	SFX_156 = 156,

	/* the general bank */
	SFX_SHELL_MUSIC = 157,    /* the front end's music loop (Front_SetMusicLoop) */
	SFX_158 = 158,
	SFX_TAPE_HIDDEN = 159,    /* the secret tape got */
	SFX_COMP_TIME_UP = 160,   /* a competition's run over */
	SFX_GAP = 161,            /* a gap trick */
	SFX_SPECIAL_TRICK = 162,

	/* the level's bank */
	SFX_ROSWELL_163 = 163,
	SFX_ROSWELL_164 = 164,
	SFX_165 = 165,
	SFX_ROSWELL_166 = 166,
	SFX_ROSWELL_167 = 167,
	SFX_ROSWELL_168 = 168,
	SFX_STREETS_ROLL_TERRAIN4 = 169, /* rolling loop */
	SFX_ROSWELL_GRIND_LOOP = 170,    /* Roswell's SFX_GRIND_LOOP_RAIL1 */
	SFX_ROSWELL_ROLL_TERRAIN1 = 171, /* rolling loop */

	/* the general bank */
	SFX_172 = 172,
	SFX_173 = 173,
	SFX_174 = 174,

	SFX_NONE = 0xffff, /* plays nothing (as does any negative id) */
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
