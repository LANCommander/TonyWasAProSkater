/* Player input as game actions (pc_input.cpp).
 *
 * The keyboard and gamepads (XInput, and SDL's game controllers) are bound to actions (ollie, grind, menu accept...), each with
 * two keyboard and two gamepad bindings kept in Settings.ini in the player's folder (Runtime::UserDir).
 * The game reads the actions directly: on PC each button slot of its SControl is one action,
 * filled by Pad_Update (game/src/pad.cpp) from PC_InputActionDown. The skating and
 * menu actions are separate slots, so the same key can mean different things in each. */
#ifndef PC_INPUT_H
#define PC_INPUT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    /* skating */
    PC_ACT_UP,
    PC_ACT_DOWN,
    PC_ACT_LEFT,
    PC_ACT_RIGHT,
    PC_ACT_OLLIE,
    PC_ACT_FLIP,
    PC_ACT_GRAB,
    PC_ACT_GRIND,
    PC_ACT_SPIN_LEFT,      /* held: rotate left */
    PC_ACT_SPIN_RIGHT,
    PC_ACT_TAP_SPIN_LEFT,  /* each tap queues a 180 to the left */
    PC_ACT_TAP_SPIN_RIGHT,
    PC_ACT_CAMERA,         /* close camera; 2P viewport layout */
    PC_ACT_PAUSE,          /* also resumes from the pause menu */
    /* menus, including the pause menu and the loading screens */
    PC_ACT_MENU_UP,
    PC_ACT_MENU_DOWN,
    PC_ACT_MENU_LEFT,
    PC_ACT_MENU_RIGHT,
    PC_ACT_MENU_ACCEPT,    /* also the front end's "press Start" */
    PC_ACT_MENU_BACK,
    PC_ACT_COUNT
} PcAction;



#define PC_BIND_SLOTS 2
#define PC_BIND_NONE (-1)

/* Bindings: per action, PC_BIND_SLOTS keyboard keys (SDL scancodes; player 0 only) and, per
 * player, PC_BIND_SLOTS gamepad inputs (pc::PadInput). PC_BIND_NONE is an empty slot. */
const char *PC_InputActionLabel(PcAction a); /* the controls screen's ("TAP SPIN LEFT") */
/* An input as the front-end font can draw it ("NUM 2", "L SHIFT", "LS UP"); "" for none. */
const char *PC_InputInputLabel(int gamepad, int input);

int PC_InputGetBind(int player, int gamepad, PcAction a, int slot);
/* Set a slot. An input does one thing per set (skate / menu): whatever else in the set had it
 * gets this slot's old input instead. */
void PC_InputBind(int player, int gamepad, PcAction a, int slot, int input);
void PC_InputResetDevice(int player, int gamepad); /* back to the defaults (not saved) */
int PC_InputSaveSettings(void);                    /* write Settings.ini; 0 on failure */

/* For the controls screen, as of the last PC_InputUpdate: a key (player 0) or a button on the
 * player's gamepad that went down, or PC_BIND_NONE; and whether the "clear binding" input went
 * down (Delete, or X on the player's gamepad). */
int PC_InputCapture(int player, int gamepad);
int PC_InputClearPressed(int player);

/* Once per game frame, before the actions are read: sample the keyboard and gamepads. */
void PC_InputUpdate(void);
/* The player's gamepad motors, as the game sets a DualShock's: small on (nonzero) or off, large
 * 0..255. Nothing without a gamepad. */
void PC_InputRumble(int player, int small, int large);

/* Player 0 is the keyboard plus the first gamepad (always connected), player 1 the second
 * gamepad. */
int PC_InputPlayerConnected(int player);
int PC_InputGamepadConnected(int player);
int PC_InputActionDown(int player, PcAction a);

/* Typing a name (the name entry screens): the next key typed on the keyboard, as 'A'..'Z', ' ',
 * or '\b' (Backspace); 0 for none. Calling it every frame puts the keyboard in text mode for that
 * frame and the next: the letters, Space and Backspace type instead of doing what they're bound to
 * (the arrows, Return and Escape still do). Keys typed outside text mode are dropped. */
int PC_InputTextChar(void);

#ifdef __cplusplus
}
#endif

#endif
