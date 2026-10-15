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
#define GAME_SKATERS 11
extern GameStats Stats[GAME_SKATERS];

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
