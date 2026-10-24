/* Player input (src/pc_input.cpp; the game's interface is pc_input.h): the keyboard and gamepads
 * (XInput on Windows, SDL's game controllers everywhere) bound to game actions, the bindings'
 * Settings.ini sections, rumble, scripted input for automated runs, and the SDL event pump. */
#pragma once

#include <stdint.h>
#include <stdio.h>

#include "pc_input.h"

typedef struct _SDL_GameController SDL_GameController;
struct _XINPUT_STATE;   /* XINPUT_STATE (Linux: pc_input.cpp's stand-in) */
#ifdef _WIN32
struct _XINPUT_VIBRATION;
#endif

namespace pc {

/* Gamepad inputs (an Xbox pad's; other pads' by position), sticks and triggers as buttons. */
enum PadInput {
    kPadA,
    kPadB,
    kPadX,
    kPadY,
    kPadLb,
    kPadRb,
    kPadLt,
    kPadRt,
    kPadBack,
    kPadStart,
    kPadLs,
    kPadRs,
    kPadDpadUp,
    kPadDpadDown,
    kPadDpadLeft,
    kPadDpadRight,
    kPadLsUp,
    kPadLsDown,
    kPadLsLeft,
    kPadLsRight,
    kPadRsUp,
    kPadRsDown,
    kPadRsLeft,
    kPadRsRight,
    kPadCount
};

/* Which set of bindings an action belongs to: skating or the menus. */
enum ActionSet { kSetSkate, kSetMenu };

/* Gamepad inputs per action and player (keys: the same). */
constexpr int kBindSlots = PC_BIND_SLOTS;

class Input {
  public:
    /* Gamepad slots: on Windows XInput's four pads come first, then SDL's game controllers (the
     * gamepads XInput doesn't read: DualShock, Switch, generic USB pads...); elsewhere SDL's only.
     * Both are read into XInput's state. */
#ifdef _WIN32
    static constexpr int kXiSlots = 4;
#else
    static constexpr int kXiSlots = 0;
#endif
    static constexpr int kSdlSlots = 4;
    static constexpr int kPadSlots = kXiSlots + kSdlSlots;
    static constexpr int kScancodes = 512;   /* SDL_NUM_SCANCODES */

    /* ---- actions and bindings ---- */
    static ActionSet SetOf(PcAction a);
    static const char *ActionName(PcAction a);   /* the Settings.ini key ("TapSpinLeft") */
    static const char *ActionLabel(PcAction a);  /* the controls screen's ("TAP SPIN LEFT") */
    static const char *PadName(int pad);         /* Settings.ini: "A", "LSUp"... */
    static const char *KeyName(int scancode);    /* Settings.ini: SDL's name ("Space"), "Comma" for ',' */
    /* An input as the front-end font can draw it ("NUM 2", "L SHIFT", "LS UP"); "" for none. */
    const char *InputLabel(int gamepad, int input);

    int GetBind(int player, int gamepad, PcAction a, int slot);
    void Bind(int player, int gamepad, PcAction a, int slot, int input);
    void ResetDevice(int player, int gamepad);

    /* ---- Settings.ini (pc_settings.h) ---- */
    void SettingsDefaults();
    void WriteSettings(FILE *f);
    int SettingsLine(const char *section, const char *key, char *value);
    /* write Settings.ini; 0 on failure */
    int SaveSettings();

    /* ---- per frame ---- */
    void Init();
    void PumpEvents();
    bool QuitRequested() const { return m_quit; }
    /* Main menu QUIT: exits on the next vblank, like closing the window. */
    void RequestQuit() { m_quit = true; }
    void Update();
    void Rumble(int player, int small, int large);

    bool PlayerConnected(int player) const { return player == 0 || m_players[player].slot >= 0; }
    bool GamepadConnected(int player) const { return player >= 0 && player < 2 && m_players[player].slot >= 0; }
    bool ActionDown(int player, PcAction a);
    int Capture(int player, int gamepad);
    bool ClearPressed(int player);
    int TextChar();

  private:
    /* a gamepad slot */
    struct Pad {
        bool connected;
        int retry;        /* polls until an empty slot is tried again */
        uint32_t down;    /* pc::PadInput bits */
        uint32_t pressed; /* went down this update */
    };
    /* a player: the gamepad slot playing it and its bindings */
    struct Player {
        int slot = -1;                        /* the Pad, or -1 */
        int pad[PC_ACT_COUNT][kBindSlots] = {};   /* PcPadInputs; PC_BIND_NONE = empty */
        int rumble[2] = {-1, -1};             /* the motors as last set: small, large */
        uint32_t rumbleRenewed = 0;           /* PC_FrameCount when they were */
    };

    int *Slot(int player, int gamepad, PcAction a, int slot);
    bool ReadByXInput(int i);
    uint32_t SdlGetState(int slot, _XINPUT_STATE *st);
    uint32_t GetState(int slot, _XINPUT_STATE *st);
    void RumbleSlot(int slot, uint16_t low, uint16_t high);
    void LoadGamepads();
    void AddMappings();
    void AddMappingsFrom(const char *dir);
    void DeviceAdded(int i);
    void PollGamepads();
    void ScriptedKeys(uint8_t *down);
    uint16_t ScriptedButtons();
    bool ScriptedAction(PcAction a);
    static int TextKey(int scancode);   /* what the key types in text mode, 0 if nothing */
    void TypeKey(int scancode);

    /* bindings: SDL scancodes (player 1 only); the gamepads' are the Players' */
    int m_key[PC_ACT_COUNT][kBindSlots] = {};
    Player m_players[2];

    Pad m_pads[kPadSlots] = {};
    int m_lead = -1;         /* the slot that pressed a button first: player 1; -1 none yet */
    int m_ignorePads = -1;   /* THPS_NOGAMEPAD=1: none (scripted test runs); -1 not read yet */

    uint8_t m_keyDown[kScancodes] = {};
    uint8_t m_keyPressed[kScancodes] = {};   /* went down this update */
    uint16_t m_scripted = 0;                 /* THPS_INPUT's PS1 buttons */
    uint32_t m_traceLastDown = 0;            /* the actions last traced */

    /* text mode (TextChar): updates left in it, and the keys typed, oldest first */
    int m_textLease = 0;
    bool m_textMode = false;
    char m_typed[16] = {};
    int m_nTyped = 0;

    bool m_init = false;
    bool m_quit = false;

#ifdef _WIN32
    using XInputGetStateFn = unsigned long(__stdcall *)(unsigned long, _XINPUT_STATE *);
    using XInputSetStateFn = unsigned long(__stdcall *)(unsigned long, _XINPUT_VIBRATION *);
    XInputGetStateFn m_xinputGetState = nullptr;
    XInputSetStateFn m_xinputSetState = nullptr;
#endif

    /* SDL slot i is the i-th game controller opened. */
    bool m_sdl = false;
    SDL_GameController *m_gc[kSdlSlots] = {};

    /* InputLabel's results, the last four */
    char m_labels[4][32] = {};
    int m_nextLabel = 0;

    /* THPS_KEYS, THPS_INPUT */
    char *m_keysSpec = nullptr;
    bool m_keysParsed = false;
    bool m_buttonsParsed = false;
    int m_nButtons = 0;
    uint32_t m_buttonFrames[256] = {};
    uint16_t m_buttonMasks[256] = {};
};

extern Input g_input;

}   // namespace pc
