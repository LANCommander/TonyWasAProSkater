/* SControl's buttons as byte offsets, for the sources that view the pads as bytes
   (`extern char Pad[]`): Pad[PAD_SIZE * player + PAD_OLLIE + PAD_TRIGGERED]. Plain C, so the .c
   sources can use it; game/pad.h has the same layout as a struct. Each button is an SButton:
   Pressed at +0, Triggered at +1. */
#ifndef GAME_PADBUTTONS_H
#define GAME_PADBUTTONS_H

#define PAD_SIZE 0x16c
#define PAD_PRESSED 0
#define PAD_TRIGGERED 1

#define PAD_GRIND 0x000          /* Triangle */
#define PAD_FLIP 0x010           /* Square */
#define PAD_GRAB 0x020           /* Circle */
#define PAD_OLLIE 0x030          /* X */
#define PAD_SPIN_LEFT 0x040      /* L1 */
#define PAD_TAP_SPIN_LEFT 0x050  /* L2 */
#define PAD_SPIN_RIGHT 0x060     /* R1 */
#define PAD_TAP_SPIN_RIGHT 0x070 /* R2 */
#define PAD_LEFT 0x080
#define PAD_RIGHT 0x090
#define PAD_UP 0x0a0
#define PAD_DOWN 0x0b0
#define PAD_PAUSE 0x0e0          /* Start */
#define PAD_CAMERA 0x0f0         /* Select */

/* menus: their own buttons on PC, the pad's own on the PS1 */
#ifdef PC_PORT
#define PAD_MENU_UP 0x0c0
#define PAD_MENU_DOWN 0x0d0
#define PAD_MENU_LEFT 0x100
#define PAD_MENU_RIGHT 0x110
#define PAD_MENU_ACCEPT 0x120
#define PAD_MENU_BACK 0x130
#else
#define PAD_MENU_UP PAD_UP
#define PAD_MENU_DOWN PAD_DOWN
#define PAD_MENU_LEFT PAD_LEFT
#define PAD_MENU_RIGHT PAD_RIGHT
#define PAD_MENU_ACCEPT PAD_OLLIE
#define PAD_MENU_BACK PAD_GRIND
#endif

/* the front end's Start (see MenuStart in game/pad.h) */
#ifdef PC_PORT
#define PAD_MENU_START PAD_MENU_ACCEPT
#else
#define PAD_MENU_START PAD_PAUSE
#endif

#endif
