/* The game's globals the runtime reads and writes, by their names (game/data defines them; the
 * layouts are those of the headers in game/include/game, here in C). */
#ifndef PC_GAMEDATA_H
#define PC_GAMEDATA_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* SMovieDetails (game/gamefmv.h): the movie and music table */
typedef struct {
    char *name; /* "\\SUICIDE.STR;1" */
    uint16_t width, height, endframe, pad0a;
    uint32_t vlcbuffersize, ringbuffersectors;
    int32_t is24bit, startsector, musicScale;
} GameMovieDetails;
#define GAME_MOVIE_COUNT 26 /* (the table's 27th entry is empty) */
extern GameMovieDetails movieDetails[27];

/* what StartFMV streams next (game/data/gamefmv.cpp) */
extern int32_t GameFMV_Active[1], GameFMV_IsMovie[1], GameFMV_RewindSwitch[1];
extern int32_t GameFMV_StartSector[1], GameFMV_EndFrame[1], GameFMV_MusicScale[1];
extern uint32_t GameFMV_VLCBufferSize[1], GameFMV_RingBufferSectors[1];
extern uint16_t GameFMV_Width[1], GameFMV_Height[2];
extern uint8_t GameFMV_Is24Bit[1], GameFMV_CurrentTrack[3];

/* SBoard / SStats (game/career.h): the skaters */
typedef struct {
    char *File, *Name;
    uint8_t Req;
    int32_t Stats[5];
} GameBoard;
typedef struct {
    char *PSXName, *PSXName2, *FrontName; /* "hawk", "hawk", "hawk_fe" */
    int32_t ComboID;
    uint8_t Board, Wheel1, Wheel2, Head;
    int8_t Neck;
    int32_t Stats[5];
    char *Name;
    char *pBio[6];
    uint32_t BoardChecksum, WheelChecksum, TruckChecksum;
    GameBoard Boards[8];
} GameStats;
#define GAME_SKATERS 11      /* the game's own (game/career.h STOCK_SKATERS) */
#define GAME_MAX_SKATERS 32  /* with the mods' (MAX_SKATERS) */
extern GameStats Stats[GAME_MAX_SKATERS];
extern int32_t GNumSkaters; /* in use */

/* SLevel (game/career.h): the levels; the game's nine, the mods', then an entry with an empty
 * name */
typedef struct {
    char *pLevelName, *pCityName, *pVab; /* "Warehouse", "Woodland Hills", "ware" */
    int32_t isCompetition, isLocked;
    char *pTRGName; /* "SkWare_T" */
    uint32_t GoldScore, SilverScore, BronzeScore;
    char *pImageName; /* "levsel0.bmp" */
    int32_t map_loc[2];
    char *pLoadImage, *pBackImage, *pGoalName;
    int32_t TapesToUnlock, MedalsToUnlock, HiScore, ProScore, Crowd;
    int32_t ExtraPolyBuffer[2], TrickTipMask[2], FMV;
    int32_t IntroStartPosX, IntroStartPosY, IntroStartPosZ;
    int16_t IntroStartAngX, IntroStartAngY, IntroStartAngZ;
} GameLevel;
#define GAME_LEVELS 9      /* the game's own (STOCK_LEVELS) */
#define GAME_MAX_LEVELS 32 /* entries, the end entry included (MAX_LEVELS) */
#define GAME_MOD_LEVELS (GAME_MAX_LEVELS - 1 - GAME_LEVELS)
#define GAME_MOD_SKATERS (GAME_MAX_SKATERS - GAME_SKATERS)
extern GameLevel Levels[GAME_MAX_LEVELS];
/* the game modes a level is in (LEVEL_*) */
#define GAME_LEVEL_CAREER 1
#define GAME_LEVEL_SINGLESESSION 2
#define GAME_LEVEL_FREESKATE 4
#define GAME_LEVEL_MULTIPLAYER 8
extern int32_t LevelModes[GAME_MAX_LEVELS];

/* SCareer (game/career.h): the career save */
typedef struct {
    uint8_t NumTapes, NumBronze, NumSilver, NumGold;
    uint8_t Tapes[10]; /* per level: goals in bits 0-4, gold/silver/bronze 0x20/0x40/0x80 */
} GameSkaterCareer;
typedef struct {
    char Name[4];
    uint8_t Skater;
    uint32_t Score;
} GameHighScore;
typedef struct {
    int32_t Version, MaxTapes;
    GameSkaterCareer CharacterProgress[GAME_SKATERS];
    char pada2[2];
    GameHighScore HighScore[20][8];
    char LastSkater[2], LastDeck[2], LastWheel[2], LastTrucks[2];
    uint8_t LastLevel;
    uint16_t TimeStamp;
    uint8_t Options[0x1a0];
} GameCareer;
extern GameCareer GCareer;
/* SCareerExt: the career's progress with the mods' skaters and levels */
typedef struct {
    GameSkaterCareer CharacterProgress[GAME_MOD_SKATERS];
    uint8_t Tapes[GAME_MAX_SKATERS][GAME_MOD_LEVELS];
    GameHighScore HighScore[GAME_MOD_LEVELS][8];
    int32_t MedalMessagesUsed[GAME_MOD_SKATERS];
} GameCareerExt;
extern GameCareerExt GCareerExt;

/* the sound effects (SFX_PlayX, game/data/unnamed.cpp): per id the VAB voice word (bit 31 loops,
 * bit 30 the level's bank, program in bits 16-22, a mask of its tones in the low half), then
 * note << 16 | volume percent */
#define GAME_SFX_COUNT 175
extern int32_t DAT_800b8a9c[GAME_SFX_COUNT * 2];

/* the game's static constructors (retail's __CTOR_LIST__): GAME_CTORS of them, then teardowns */
#define GAME_CTORS 19
extern void (*GameCtorList[25])(void);

#ifdef __cplusplus
}
#endif

#endif
