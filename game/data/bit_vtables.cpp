/* The vtables of the classes src/bit/ defines (native data, psxdata.h). */
#include "psxdata.h"

extern char FUN_8002ee54[];
extern char FUN_8002ee5c[];
extern char FUN_8002ee64[];
extern char Move__10CRibbonBit[];
extern char Move__11CGrindSpark[];
extern char Move__6CSpark[];
extern char dtor_10CRibbonBit[] PSX_NAME("_._10CRibbonBit");
extern char dtor_11CGrindSpark[] PSX_NAME("_._11CGrindSpark");
extern char dtor_15CSpecialDisplay[] PSX_NAME("_._15CSpecialDisplay");
extern char dtor_5CGlow[] PSX_NAME("_._5CGlow");
extern char dtor_6CGLine[] PSX_NAME("_._6CGLine");
extern char dtor_6CSpark[] PSX_NAME("_._6CSpark");
extern char dtor_7CRibbon[] PSX_NAME("_._7CRibbon");
extern char dtor_9CChunkBit[] PSX_NAME("_._9CChunkBit");

PsxVtEntry __vt_11CGrindSpark[3] = {
	{0, 0, 0},
	{0, 0, dtor_11CGrindSpark},
	{0, 0, Move__11CGrindSpark},
};

PsxVtEntry __vt_6CGLine[3] = {
	{0, 0, 0},
	{0, 0, dtor_6CGLine},
	{0, 0, FUN_8002ee64},
};

PsxVtEntry __vt_7CRibbon[3] = {
	{0, 0, 0},
	{0, 0, dtor_7CRibbon},
	{0, 0, FUN_8002ee64},
};

PsxVtEntry __vt_10CRibbonBit[3] = {
	{0, 0, 0},
	{0, 0, dtor_10CRibbonBit},
	{0, 0, Move__10CRibbonBit},
};

PsxVtEntry __vt_6CSpark[3] = {
	{0, 0, 0},
	{0, 0, dtor_6CSpark},
	{0, 0, Move__6CSpark},
};

PsxVtEntry __vt_5CGlow[3] = {
	{0, 0, 0},
	{0, 0, dtor_5CGlow},
	{0, 0, FUN_8002ee54},
};

PsxVtEntry __vt_9CChunkBit[3] = {
	{0, 0, 0},
	{0, 0, dtor_9CChunkBit},
	{0, 0, FUN_8002ee64},
};

PsxVtEntry __vt_15CSpecialDisplay[4] = {
	{0, 0, 0},
	{0, 0, dtor_15CSpecialDisplay},
	{0, 0, FUN_8002ee64},
	{0, 0, FUN_8002ee5c},
};
