/* The flashing on-screen messages (FLASH.CPP). */
#ifndef GAME_FLASH_H
#define GAME_FLASH_H

#include "game/types.h"

/* functions */
void Flash_Reset();
int Flash_FadeFinished();
void Flash_Screen(unsigned char r, unsigned char g, unsigned char b, int n, unsigned char c, int d);
void Flash_Fade(int a, int b);
void Flash_Update();
void Flash_Display();
#endif
