/* Player input: keyboard and gamepads (XInput and SDL) bound to game actions (see pc_input.h).
 *
 * The game reads the actions themselves: on PC every button slot of its SControl is one action
 * (Ollie, Grind, MenuAccept...), and Pad_Update (game/src/pad.cpp) fills each slot
 * from PC_InputActionDown once per game frame. Skating and the menus read different slots, so
 * their bindings are independent. The controls screen (Front_ControllerOptions, game/src/frontend.cpp)
 * edits the bindings through the PC_InputBind family. */
#ifdef _WIN32
#include <windows.h>
#include <xinput.h>
#else
#include <stdint.h>
/* No XInput: only SDL's game controllers, read into XInput's state (SdlGetState below) */
typedef uint32_t DWORD;
typedef uint16_t WORD;
typedef struct _XINPUT_GAMEPAD {
    WORD wButtons;
    uint8_t bLeftTrigger, bRightTrigger;
    int16_t sThumbLX, sThumbLY, sThumbRX, sThumbRY;
} XINPUT_GAMEPAD;
typedef struct _XINPUT_STATE {
    DWORD dwPacketNumber;
    XINPUT_GAMEPAD Gamepad;
} XINPUT_STATE;
#define WINAPI
#define ERROR_SUCCESS 0
#define XINPUT_GAMEPAD_DPAD_UP 0x0001
#define XINPUT_GAMEPAD_DPAD_DOWN 0x0002
#define XINPUT_GAMEPAD_DPAD_LEFT 0x0004
#define XINPUT_GAMEPAD_DPAD_RIGHT 0x0008
#define XINPUT_GAMEPAD_START 0x0010
#define XINPUT_GAMEPAD_BACK 0x0020
#define XINPUT_GAMEPAD_LEFT_THUMB 0x0040
#define XINPUT_GAMEPAD_RIGHT_THUMB 0x0080
#define XINPUT_GAMEPAD_LEFT_SHOULDER 0x0100
#define XINPUT_GAMEPAD_RIGHT_SHOULDER 0x0200
#define XINPUT_GAMEPAD_A 0x1000
#define XINPUT_GAMEPAD_B 0x2000
#define XINPUT_GAMEPAD_X 0x4000
#define XINPUT_GAMEPAD_Y 0x8000
#define XINPUT_GAMEPAD_TRIGGER_THRESHOLD 30
#endif
#include <SDL2/SDL.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <type_traits>

#include "pc_frame.h"
#include "pc_input.h"
#include "pc_runtime.h"
#include "pc_video.h"
#include "pc/input.h"
#include "pc/platform.h"
#include "pc/runtime.h"
#include "pc/settings.h"
#include "pc/video.h"

namespace pc {

Input g_input;

static_assert(Input::kScancodes == SDL_NUM_SCANCODES, "Input's key arrays hold SDL's scancodes");
#ifdef _WIN32
static_assert(std::is_same<decltype(&XInputGetState), DWORD(WINAPI *)(DWORD, XINPUT_STATE *)>::value &&
                  std::is_same<DWORD(WINAPI *)(DWORD, XINPUT_STATE *), unsigned long(__stdcall *)(unsigned long, _XINPUT_STATE *)>::value &&
                  std::is_same<DWORD(WINAPI *)(DWORD, XINPUT_VIBRATION *), unsigned long(__stdcall *)(unsigned long, _XINPUT_VIBRATION *)>::value,
              "pc/input.h's XInput function types are XInput's");
#endif

namespace {

/* ---- actions ---------------------------------------------------------------------------------- */

const struct {
    const char *name;  /* Settings.ini */
    const char *label; /* the controls screen */
    ActionSet set;
    const char *def[4]; /* two keys, two gamepad inputs */
} s_actions[PC_ACT_COUNT] = {
    [PC_ACT_UP] = {"Up", "UP", kSetSkate, {"W", "Up", "DPadUp", "LSUp"}},
    [PC_ACT_DOWN] = {"Down", "DOWN", kSetSkate, {"S", "Down", "DPadDown", "LSDown"}},
    [PC_ACT_LEFT] = {"Left", "LEFT", kSetSkate, {"A", "Left", "DPadLeft", "LSLeft"}},
    [PC_ACT_RIGHT] = {"Right", "RIGHT", kSetSkate, {"D", "Right", "DPadRight", "LSRight"}},
    [PC_ACT_OLLIE] = {"Ollie", "OLLIE", kSetSkate, {"Space", "Keypad 2", "A", ""}},
    [PC_ACT_FLIP] = {"Flip", "FLIP", kSetSkate, {"J", "Keypad 4", "X", ""}},
    [PC_ACT_GRAB] = {"Grab", "GRAB", kSetSkate, {"L", "Keypad 6", "B", ""}},
    [PC_ACT_GRIND] = {"Grind", "GRIND", kSetSkate, {"I", "Keypad 8", "Y", ""}},
    [PC_ACT_SPIN_LEFT] = {"SpinLeft", "SPIN LEFT", kSetSkate, {"Q", "Keypad 7", "LB", ""}},
    [PC_ACT_SPIN_RIGHT] = {"SpinRight", "SPIN RIGHT", kSetSkate, {"E", "Keypad 9", "RB", ""}},
    [PC_ACT_TAP_SPIN_LEFT] = {"TapSpinLeft", "TAP SPIN LEFT", kSetSkate, {"Z", "Keypad 1", "LT", ""}},
    [PC_ACT_TAP_SPIN_RIGHT] = {"TapSpinRight", "TAP SPIN RIGHT", kSetSkate, {"C", "Keypad 3", "RT", ""}},
    [PC_ACT_CAMERA] = {"Camera", "CAMERA", kSetSkate, {"Tab", "Keypad 5", "Back", ""}},
    [PC_ACT_PAUSE] = {"Pause", "PAUSE", kSetSkate, {"Escape", "P", "Start", ""}},
    [PC_ACT_MENU_UP] = {"Up", "UP", kSetMenu, {"Up", "W", "DPadUp", "LSUp"}},
    [PC_ACT_MENU_DOWN] = {"Down", "DOWN", kSetMenu, {"Down", "S", "DPadDown", "LSDown"}},
    [PC_ACT_MENU_LEFT] = {"Left", "LEFT", kSetMenu, {"Left", "A", "DPadLeft", "LSLeft"}},
    [PC_ACT_MENU_RIGHT] = {"Right", "RIGHT", kSetMenu, {"Right", "D", "DPadRight", "LSRight"}},
    [PC_ACT_MENU_ACCEPT] = {"Accept", "ACCEPT", kSetMenu, {"Return", "Space", "A", ""}},
    [PC_ACT_MENU_BACK] = {"Back", "BACK", kSetMenu, {"Escape", "Backspace", "B", ""}},
};

const char *const s_padNames[kPadCount] = {
    "A", "B", "X", "Y", "LB", "RB", "LT", "RT", "Back", "Start", "LS", "RS",
    "DPadUp", "DPadDown", "DPadLeft", "DPadRight",
    "LSUp", "LSDown", "LSLeft", "LSRight", "RSUp", "RSDown", "RSLeft", "RSRight",
};

const char *const s_padLabels[kPadCount] = {
    "A", "B", "X", "Y", "LB", "RB", "LT", "RT", "BACK", "START", "LS PRESS", "RS PRESS",
    "DPAD UP", "DPAD DOWN", "DPAD LEFT", "DPAD RIGHT",
    "LS UP", "LS DOWN", "LS LEFT", "LS RIGHT", "RS UP", "RS DOWN", "RS LEFT", "RS RIGHT",
};

/* A key's name in the front-end font: upper case letters, digits and spaces only (the menu font
 * has no punctuation, and '[', ']', '<', '>', '{', '}' and '^' are text colour and scale codes). */
void KeyLabel(int sc, char *out, size_t size) {
    static const struct {
        char c;
        const char *word;
    } words[] = {
        {'/', "SLASH"}, {'\\', "BACKSLASH"}, {'*', "STAR"}, {'=', "EQUALS"}, {';', "SEMICOLON"},
        {'[', "L BRACKET"}, {']', "R BRACKET"}, {'`', "BACKQUOTE"}, {'<', "LESS"}, {'>', "GREATER"},
        {'^', "CARET"}, {'{', "L BRACE"}, {'}', "R BRACE"}, {'|', "BAR"}, {'$', "DOLLAR"},
        {'(', "L PAREN"}, {')', "R PAREN"}, {'~', "TILDE"}, {'.', "PERIOD"}, {',', "COMMA"},
        {'-', "MINUS"}, {'\'', "QUOTE"}, {'+', "PLUS"}, {'#', "HASH"}, {'&', "AND"}, {'@', "AT"},
        {'!', "EXCLAIM"}, {'"', "QUOTES"}, {'%', "PERCENT"}, {'?', "QUESTION"}, {':', "COLON"},
        {'_', "UNDERSCORE"},
    };
    const char *name = sc > 0 && sc < SDL_NUM_SCANCODES ? SDL_GetScancodeName((SDL_Scancode)sc) : "";
    size_t n = 0;
    out[0] = 0;
    if (!*name) {
        snprintf(out, size, "KEY %d", sc);
        return;
    }
    if (!SDL_strncasecmp(name, "Keypad ", 7)) {
        n = snprintf(out, size, "NUM ");
        name += 7;
    } else if (!SDL_strncasecmp(name, "Left ", 5) && name[5]) {
        n = snprintf(out, size, "L ");
        name += 5;
    } else if (!SDL_strncasecmp(name, "Right ", 6) && name[6]) {
        n = snprintf(out, size, "R ");
        name += 6;
    }
    for (; *name && n + 1 < size; name++) {
        const char *w = nullptr;
        for (const auto &word : words)
            if (*name == word.c)
                w = word.word;
        if (w)
            n += snprintf(out + n, size - n, "%s", w);
        else if (isalnum((unsigned char)*name) || *name == ' ')
            out[n++] = (char)toupper((unsigned char)*name);
        out[n < size ? n : size - 1] = 0;
    }
}

int ParseKey(const char *s) {
    SDL_Scancode sc;
    if (!*s || !SDL_strcasecmp(s, "None"))
        return PC_BIND_NONE;
    if (!SDL_strcasecmp(s, "Comma"))
        return SDL_SCANCODE_COMMA;
    sc = SDL_GetScancodeFromName(s);
    if (sc == SDL_SCANCODE_UNKNOWN) {
        PC_Log("Settings.ini: unknown key \"%s\"", s);
        return PC_BIND_NONE;
    }
    return sc;
}

int ParsePad(const char *s) {
    int i;
    if (!*s || !SDL_strcasecmp(s, "None"))
        return PC_BIND_NONE;
    for (i = 0; i < kPadCount; i++)
        if (!SDL_strcasecmp(s, s_padNames[i]))
            return i;
    PC_Log("Settings.ini: unknown gamepad button \"%s\"", s);
    return PC_BIND_NONE;
}

/* ---- Settings.ini ----------------------------------------------------------------------------- */

const struct {
    const char *name;
    ActionSet set;
    int gamepad, player;
} s_sections[] = {
    {"Skate Keyboard", kSetSkate, 0, 0},
    {"Skate Gamepad", kSetSkate, 1, 0},
    {"Skate Gamepad 2", kSetSkate, 1, 1},
    {"Menu Keyboard", kSetMenu, 0, 0},
    {"Menu Gamepad", kSetMenu, 1, 0},
    {"Menu Gamepad 2", kSetMenu, 1, 1},
};
constexpr int kNumSections = (int)(sizeof s_sections / sizeof s_sections[0]);

char *Trim(char *s) {
    char *e;
    while (isspace((unsigned char)*s))
        s++;
    e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1]))
        *--e = 0;
    return s;
}

/* ---- devices ---------------------------------------------------------------------------------- */

constexpr int kStickThreshold = 16000;

uint32_t XInputMask(const XINPUT_GAMEPAD *g) {
    static const struct {
        WORD bit;
        int input;
    } btn[] = {
        {XINPUT_GAMEPAD_A, kPadA},
        {XINPUT_GAMEPAD_B, kPadB},
        {XINPUT_GAMEPAD_X, kPadX},
        {XINPUT_GAMEPAD_Y, kPadY},
        {XINPUT_GAMEPAD_LEFT_SHOULDER, kPadLb},
        {XINPUT_GAMEPAD_RIGHT_SHOULDER, kPadRb},
        {XINPUT_GAMEPAD_BACK, kPadBack},
        {XINPUT_GAMEPAD_START, kPadStart},
        {XINPUT_GAMEPAD_LEFT_THUMB, kPadLs},
        {XINPUT_GAMEPAD_RIGHT_THUMB, kPadRs},
        {XINPUT_GAMEPAD_DPAD_UP, kPadDpadUp},
        {XINPUT_GAMEPAD_DPAD_DOWN, kPadDpadDown},
        {XINPUT_GAMEPAD_DPAD_LEFT, kPadDpadLeft},
        {XINPUT_GAMEPAD_DPAD_RIGHT, kPadDpadRight},
    };
    uint32_t m = 0;
    for (const auto &b : btn)
        if (g->wButtons & b.bit)
            m |= 1u << b.input;
    if (g->bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
        m |= 1u << kPadLt;
    if (g->bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
        m |= 1u << kPadRt;
    if (g->sThumbLY > kStickThreshold)
        m |= 1u << kPadLsUp;
    if (g->sThumbLY < -kStickThreshold)
        m |= 1u << kPadLsDown;
    if (g->sThumbLX < -kStickThreshold)
        m |= 1u << kPadLsLeft;
    if (g->sThumbLX > kStickThreshold)
        m |= 1u << kPadLsRight;
    if (g->sThumbRY > kStickThreshold)
        m |= 1u << kPadRsUp;
    if (g->sThumbRY < -kStickThreshold)
        m |= 1u << kPadRsDown;
    if (g->sThumbRX < -kStickThreshold)
        m |= 1u << kPadRsLeft;
    if (g->sThumbRX > kStickThreshold)
        m |= 1u << kPadRsRight;
    return m;
}

}   // namespace

/* ---- actions ---------------------------------------------------------------------------------- */

ActionSet Input::SetOf(PcAction a) { return s_actions[a].set; }
const char *Input::ActionName(PcAction a) { return s_actions[a].name; }
const char *Input::ActionLabel(PcAction a) { return s_actions[a].label; }
const char *Input::PadName(int pad) { return pad >= 0 && pad < kPadCount ? s_padNames[pad] : ""; }

const char *Input::KeyName(int sc) {
    if (sc == SDL_SCANCODE_COMMA)
        return "Comma"; /* ',' separates the slots in Settings.ini */
    return sc > 0 && sc < SDL_NUM_SCANCODES ? SDL_GetScancodeName((SDL_Scancode)sc) : "";
}

const char *Input::InputLabel(int gamepad, int input) {
    char *b = m_labels[m_nextLabel++ & 3];
    if (input == PC_BIND_NONE)
        return "";
    if (gamepad)
        return input >= 0 && input < kPadCount ? s_padLabels[input] : "";
    KeyLabel(input, b, sizeof m_labels[0]);
    return b;
}

int *Input::Slot(int player, int gamepad, PcAction a, int slot) {
    if (player < 0 || player > 1 || a < 0 || a >= PC_ACT_COUNT || slot < 0 || slot >= kBindSlots ||
        (!gamepad && player != 0))
        return nullptr;
    return gamepad ? &m_players[player].pad[a][slot] : &m_key[a][slot];
}

int Input::GetBind(int player, int gamepad, PcAction a, int slot) {
    int *p = Slot(player, gamepad, a, slot);
    return p ? *p : PC_BIND_NONE;
}

/* An input does one thing per set: if another action of the same set (or this action's other
 * slot) has it, that slot gets this slot's old input instead. */
void Input::Bind(int player, int gamepad, PcAction a, int slot, int input) {
    int *cur = Slot(player, gamepad, a, slot);
    int old, b, s;
    if (!cur || *cur == input)
        return;
    old = *cur;
    if (input != PC_BIND_NONE)
        for (b = 0; b < PC_ACT_COUNT; b++) {
            if (s_actions[b].set != s_actions[a].set)
                continue;
            for (s = 0; s < kBindSlots; s++) {
                int *o = Slot(player, gamepad, (PcAction)b, s);
                if (o != cur && *o == input)
                    *o = old;
            }
        }
    *cur = input;
}

/* back to the defaults (not saved) */
void Input::ResetDevice(int player, int gamepad) {
    int a, i;
    for (a = 0; a < PC_ACT_COUNT; a++)
        for (i = 0; i < kBindSlots; i++) {
            int *p = Slot(player, gamepad, (PcAction)a, i);
            if (p)
                *p = gamepad ? ParsePad(s_actions[a].def[2 + i]) : ParseKey(s_actions[a].def[i]);
        }
}

/* ---- Settings.ini ----------------------------------------------------------------------------- */

int Input::SaveSettings() { return g_settings.Save(); }

void Input::SettingsDefaults() {
    ResetDevice(0, 0);
    ResetDevice(0, 1);
    ResetDevice(1, 1);
}

void Input::WriteSettings(FILE *f) {
    int s, a, i;
    fputs("\n; Controls: each action takes up to two keys and two gamepad buttons, separated by a\n"
          "; comma. Leave a slot empty (or write None) to unbind it.\n"
          "; Keys are SDL key names: letters and digits, Space, Return, Escape, Backspace, Tab,\n"
          "; Left Shift, Right Ctrl, Up, Down, Left, Right, Keypad 2, Comma, ...\n"
          "; Gamepad buttons: A B X Y LB RB LT RT Back Start LS RS, DPadUp DPadDown DPadLeft\n"
          "; DPadRight, and the sticks: LSUp LSDown LSLeft LSRight RSUp RSDown RSLeft RSRight. These are\n"
          "; the Xbox names; on other gamepads they are the buttons in the same place (A = Cross).\n"
          "; The keyboard plays player 1. [... Gamepad] is the first gamepad (player 1),\n"
          "; [... Gamepad 2] the second (player 2).\n"
          ";\n"
          "; The Skate controls are read while skating, the Menu controls by the menus, so the same\n"
          "; key can do something different in each. Pause also resumes from the pause menu, and\n"
          "; the pause-menu cheat codes are entered with the Skate controls (hold SpinLeft).\n",
          f);
    for (s = 0; s < kNumSections; s++) {
        fprintf(f, "\n[%s]\n", s_sections[s].name);
        for (a = 0; a < PC_ACT_COUNT; a++) {
            int v[kBindSlots], last = kBindSlots - 1;
            if (s_actions[a].set != s_sections[s].set)
                continue;
            for (i = 0; i < kBindSlots; i++)
                v[i] = GetBind(s_sections[s].player, s_sections[s].gamepad, (PcAction)a, i);
            while (last >= 0 && v[last] == PC_BIND_NONE)
                last--; /* no trailing empty slots */
            fprintf(f, "%s =", s_actions[a].name);
            for (i = 0; i <= last; i++) {
                const char *n = s_sections[s].gamepad ? PadName(v[i]) : KeyName(v[i]);
                fprintf(f, "%s%s", i ? ", " : " ", v[i] == PC_BIND_NONE ? "" : n);
            }
            fputs("\n", f);
        }
    }
}

int Input::SettingsLine(const char *section, const char *key, char *value) {
    char *field;
    int sec, a, n;
    for (sec = 0; sec < kNumSections; sec++)
        if (!SDL_strcasecmp(section, s_sections[sec].name))
            break;
    if (sec == kNumSections)
        return 0;
    for (a = 0; a < PC_ACT_COUNT; a++)
        if (s_actions[a].set == s_sections[sec].set && !SDL_strcasecmp(key, s_actions[a].name))
            break;
    if (a == PC_ACT_COUNT) {
        PC_Log("Settings.ini: unknown action \"%s\" in [%s]", key, s_sections[sec].name);
        return 1;
    }
    for (n = 0, field = value; n < kBindSlots; n++) {
        char *comma = field ? strchr(field, ',') : nullptr;
        const char *v;
        int *p = Slot(s_sections[sec].player, s_sections[sec].gamepad, (PcAction)a, n);
        if (comma)
            *comma = 0;
        v = field ? Trim(field) : "";
        *p = s_sections[sec].gamepad ? ParsePad(v) : ParseKey(v);
        field = comma ? comma + 1 : nullptr;
    }
    return 1;
}

/* ---- devices ---------------------------------------------------------------------------------- */

/* Whether SDL's device i is one XInput reads already: SDL marks its XInput devices' GUIDs with an
 * 'x', and its other drivers (HIDAPI) may have an Xbox pad too. */
bool Input::ReadByXInput(int i) {
#ifdef _WIN32
    SDL_GameControllerType t = SDL_GameControllerTypeForIndex(i);
    return m_xinputGetState && (SDL_JoystickGetDeviceGUID(i).data[14] == 'x' ||
                                t == SDL_CONTROLLER_TYPE_XBOX360 || t == SDL_CONTROLLER_TYPE_XBOXONE);
#else
    (void)i;
    return false;
#endif
}

uint32_t Input::SdlGetState(int slot, XINPUT_STATE *st) {
    static const struct {
        SDL_GameControllerButton b;
        WORD bit;
    } btn[] = {
        {SDL_CONTROLLER_BUTTON_A, XINPUT_GAMEPAD_A},
        {SDL_CONTROLLER_BUTTON_B, XINPUT_GAMEPAD_B},
        {SDL_CONTROLLER_BUTTON_X, XINPUT_GAMEPAD_X},
        {SDL_CONTROLLER_BUTTON_Y, XINPUT_GAMEPAD_Y},
        {SDL_CONTROLLER_BUTTON_LEFTSHOULDER, XINPUT_GAMEPAD_LEFT_SHOULDER},
        {SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, XINPUT_GAMEPAD_RIGHT_SHOULDER},
        {SDL_CONTROLLER_BUTTON_BACK, XINPUT_GAMEPAD_BACK},
        {SDL_CONTROLLER_BUTTON_START, XINPUT_GAMEPAD_START},
        {SDL_CONTROLLER_BUTTON_LEFTSTICK, XINPUT_GAMEPAD_LEFT_THUMB},
        {SDL_CONTROLLER_BUTTON_RIGHTSTICK, XINPUT_GAMEPAD_RIGHT_THUMB},
        {SDL_CONTROLLER_BUTTON_DPAD_UP, XINPUT_GAMEPAD_DPAD_UP},
        {SDL_CONTROLLER_BUTTON_DPAD_DOWN, XINPUT_GAMEPAD_DPAD_DOWN},
        {SDL_CONTROLLER_BUTTON_DPAD_LEFT, XINPUT_GAMEPAD_DPAD_LEFT},
        {SDL_CONTROLLER_BUTTON_DPAD_RIGHT, XINPUT_GAMEPAD_DPAD_RIGHT},
    };
    SDL_GameController *gc = m_gc[slot];
    XINPUT_GAMEPAD *g = &st->Gamepad;
    int i;
    if (gc && !SDL_GameControllerGetAttached(gc)) {
        SDL_GameControllerClose(gc);
        gc = m_gc[slot] = nullptr;
    }
    for (i = 0; !gc && i < SDL_NumJoysticks(); i++) {
        SDL_JoystickID id = SDL_JoystickGetDeviceInstanceID(i);
        int k, taken = 0;
        if (!SDL_IsGameController(i) || ReadByXInput(i))
            continue;
        for (k = 0; k < kSdlSlots; k++)
            taken |= m_gc[k] && SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(m_gc[k])) == id;
        if (!taken && (gc = m_gc[slot] = SDL_GameControllerOpen(i)) != nullptr)
            PC_Log("gamepad %d: %s", kXiSlots + slot, SDL_GameControllerName(gc));
    }
    if (!gc)
        return 1;
    memset(st, 0, sizeof *st);
    for (const auto &b : btn)
        if (SDL_GameControllerGetButton(gc, b.b))
            g->wButtons |= b.bit;
    g->bLeftTrigger = (uint8_t)(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_TRIGGERLEFT) >> 7);
    g->bRightTrigger = (uint8_t)(SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) >> 7);
    /* XInput's Y axes point up */
    g->sThumbLX = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTX);
    g->sThumbLY = (int16_t)~SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTY);
    g->sThumbRX = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_RIGHTX);
    g->sThumbRY = (int16_t)~SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_RIGHTY);
    return ERROR_SUCCESS;
}

uint32_t Input::GetState(int slot, XINPUT_STATE *st) {
#ifdef _WIN32
    if (slot < kXiSlots)
        return m_xinputGetState ? m_xinputGetState((DWORD)slot, st) : 1;
#endif
    return m_sdl ? SdlGetState(slot - kXiSlots, st) : 1;
}

/* low-frequency (large) and high-frequency (small) motor speeds, 0..65535 */
void Input::RumbleSlot(int slot, uint16_t low, uint16_t high) {
#ifdef _WIN32
    if (slot < kXiSlots) {
        XINPUT_VIBRATION v;
        v.wLeftMotorSpeed = low;
        v.wRightMotorSpeed = high;
        if (m_xinputSetState)
            m_xinputSetState((DWORD)slot, &v);
        return;
    }
#endif
    if (m_gc[slot - kXiSlots])
        SDL_GameControllerRumble(m_gc[slot - kXiSlots], low, high, 1000); /* renewed while it lasts */
}

void Input::LoadGamepads() {
#ifdef _WIN32
    static const char *const dlls[] = {"xinput1_4.dll", "xinput1_3.dll", "xinput9_1_0.dll"};
    int i;
    for (i = 0; i < 3 && !m_xinputGetState; i++) {
        HMODULE m = LoadLibraryA(dlls[i]);
        if (m) {
            m_xinputGetState = (XInputGetStateFn)(void (*)(void))GetProcAddress(m, "XInputGetState");
            m_xinputSetState = (XInputSetStateFn)(void (*)(void))GetProcAddress(m, "XInputSetState");
            if (m_xinputGetState)
                PC_Log("gamepads: %s", dlls[i]);
        }
    }
    if (!m_xinputGetState)
        PC_Log("gamepads: XInput not available");
#endif
    m_sdl = SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) == 0;
    if (m_sdl)
        PC_Log("gamepads: SDL game controllers");
    else
        PC_Log("gamepads: SDL game controllers not available (%s)", SDL_GetError());
    if (m_sdl)
        AddMappings();
}

/* Pads SDL's own database leaves out. The PlayStation Classic's pad (054c:0cda) is a plain HID
 * joystick: 10 buttons, and its D-pad as the two axes. SDL ignores the CRC in a mapping's GUID
 * (bytes 2-3), so one line covers every name the pad reports. */
void Input::AddMappings() {
    static const char *const maps[] = {
#if defined(_WIN32)
        "030000004c050000da0c000000000000,PlayStation Classic Controller,"
#elif defined(__APPLE__)
        "030000004c050000da0c000000010000,PlayStation Classic Controller,"
#else
        "030000004c050000da0c000011010000,PlayStation Classic Controller,"
#endif
        "a:b2,b:b1,x:b3,y:b0,back:b8,start:b9,leftshoulder:b6,rightshoulder:b7,lefttrigger:b4,"
        "righttrigger:b5,dpup:-a1,dpdown:+a1,dpleft:-a0,dpright:+a0",
    };
    char *base = SDL_GetBasePath();
    for (const char *m : maps)
        if (SDL_GameControllerAddMapping(m) < 0)
            PC_Log("gamepads: bad mapping (%s)", SDL_GetError());
    /* more pads: a gamecontrollerdb.txt (github.com/mdqinc/SDL_GameControllerDB) by the exe, then
     * one by Settings.ini, whose lines replace the exe one's for the same pad */
    if (base) {
        AddMappingsFrom(base);
        SDL_free(base);
    }
    AddMappingsFrom(g_runtime.UserDir());
}

void Input::AddMappingsFrom(const char *dir) {
    char path[1024];
    int n;
    size_t len = strlen(dir);
    snprintf(path, sizeof path, "%s%sgamecontrollerdb.txt", dir,
             len && (dir[len - 1] == '/' || dir[len - 1] == '\\') ? "" : PC_SEP);
    n = SDL_GameControllerAddMappingsFromFile(path);
    if (n >= 0)
        PC_Log("gamepads: %d mappings from %s", n, path);
}

/* A pad SDL has no mapping for is never opened: say so, so it isn't silently ignored. */
void Input::DeviceAdded(int i) {
    char guid[33];
    if (SDL_IsGameController(i))
        return;
    SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(i), guid, sizeof guid);
    PC_Log("gamepads: \"%s\" (%04x:%04x, GUID %s) is not a known controller; add a mapping for it "
           "to gamecontrollerdb.txt", SDL_JoystickNameForIndex(i), SDL_JoystickGetDeviceVendor(i),
           SDL_JoystickGetDeviceProduct(i), guid);
}

/* Polling an empty XInput slot is slow, so empty XInput slots are only tried every 120 polls. */
void Input::PollGamepads() {
    int i, p = 0;
    if (m_ignorePads < 0)
        m_ignorePads = getenv("THPS_NOGAMEPAD") && atoi(getenv("THPS_NOGAMEPAD")) != 0;
    for (i = 0; i < kPadSlots; i++) {
        Pad &pad = m_pads[i];
        XINPUT_STATE st;
        uint32_t was = pad.down;
        pad.pressed = 0;
        if (m_ignorePads || (!pad.connected && pad.retry-- > 0))
            continue;
        if (GetState(i, &st) == ERROR_SUCCESS) {
            if (!pad.connected)
                PC_Log("gamepad %d connected", i);
            pad.connected = true;
            pad.down = XInputMask(&st.Gamepad);
            pad.pressed = pad.down & ~was;
        } else {
            if (pad.connected)
                PC_Log("gamepad %d disconnected", i);
            pad.connected = false;
            pad.retry = i < kXiSlots ? 120 : 0;
            pad.down = 0;
        }
    }
    m_players[0].slot = m_players[1].slot = -1;
    for (i = 0; i < kPadSlots && p < 2; i++)
        if (m_pads[i].connected)
            m_players[p++].slot = i;
}

/* ---- scripted input (automated runs) ---------------------------------------------------------- */

/* THPS_KEYS="frame:Key+Key,frame:Key,frame:,..." holds the named keys (Settings.ini names) from
 * that vblank on, so runs go through the bindings. */
void Input::ScriptedKeys(uint8_t *down) {
    uint32_t now = PC_FrameCount();
    const char *held = nullptr;
    char *p;
    if (!m_keysParsed) {
        m_keysParsed = true;
        m_keysSpec = getenv("THPS_KEYS") ? strdup(getenv("THPS_KEYS")) : nullptr;
    }
    for (p = m_keysSpec; p && *p;) {
        char *colon = strchr(p, ':');
        size_t len;
        if (!colon || (uint32_t)strtoul(p, nullptr, 10) > now)
            break;
        held = colon + 1;
        len = strcspn(held, ",");
        p = (char *)held + len + (held[len] == ',');
    }
    while (held && *held && *held != ',') {
        char name[64];
        size_t len = strcspn(held, "+,");
        int sc;
        snprintf(name, sizeof name, "%.*s", (int)len, held);
        sc = ParseKey(name);
        if (sc != PC_BIND_NONE)
            down[sc] = 1;
        held += len + (held[len] == '+');
    }
}

/* THPS_INPUT="frame:mask,..." holds a PS1 button mask (pressed = 1, libpad bit order) from that
 * vblank on, for player 1. */
uint16_t Input::ScriptedButtons() {
    uint32_t now = PC_FrameCount();
    uint16_t m = 0;
    int i;
    if (!m_buttonsParsed) {
        const char *p;
        m_buttonsParsed = true;
        for (p = getenv("THPS_INPUT"); p && *p && m_nButtons < 256;) {
            unsigned f, b;
            int used;
            if (sscanf(p, "%u:%x%n", &f, &b, &used) != 2)
                break;
            m_buttonFrames[m_nButtons] = f;
            m_buttonMasks[m_nButtons++] = (uint16_t)b;
            p += used;
            if (*p == ',')
                p++;
        }
    }
    for (i = 0; i < m_nButtons; i++)
        if (m_buttonFrames[i] <= now)
            m = m_buttonMasks[i];
    return m;
}

/* What a scripted PS1 button means: on the PS1 the menus read the same buttons as skating. */
bool Input::ScriptedAction(PcAction a) {
    enum {
        B_SELECT = 0, B_START = 3, B_UP = 4, B_RIGHT = 5, B_DOWN = 6, B_LEFT = 7, B_L2 = 8,
        B_R2 = 9, B_L1 = 10, B_R1 = 11, B_TRIANGLE = 12, B_CIRCLE = 13, B_CROSS = 14, B_SQUARE = 15
    };
    static const signed char bit[PC_ACT_COUNT] = {
        [PC_ACT_UP] = B_UP, [PC_ACT_DOWN] = B_DOWN, [PC_ACT_LEFT] = B_LEFT,
        [PC_ACT_RIGHT] = B_RIGHT, [PC_ACT_OLLIE] = B_CROSS, [PC_ACT_FLIP] = B_SQUARE,
        [PC_ACT_GRAB] = B_CIRCLE, [PC_ACT_GRIND] = B_TRIANGLE, [PC_ACT_SPIN_LEFT] = B_L1,
        [PC_ACT_SPIN_RIGHT] = B_R1, [PC_ACT_TAP_SPIN_LEFT] = B_L2, [PC_ACT_TAP_SPIN_RIGHT] = B_R2,
        [PC_ACT_CAMERA] = B_SELECT, [PC_ACT_PAUSE] = B_START, [PC_ACT_MENU_UP] = B_UP,
        [PC_ACT_MENU_DOWN] = B_DOWN, [PC_ACT_MENU_LEFT] = B_LEFT, [PC_ACT_MENU_RIGHT] = B_RIGHT,
        [PC_ACT_MENU_ACCEPT] = B_CROSS, [PC_ACT_MENU_BACK] = B_TRIANGLE,
    };
    return m_scripted >> bit[a] & 1;
}

/* ---- per frame -------------------------------------------------------------------------------- */

/* Before anything else: the settings and the gamepad API. */
void Input::Init() {
    if (m_init)
        return;
    m_init = true;
    SDL_InitSubSystem(SDL_INIT_EVENTS);
    g_settings.Load();
    LoadGamepads();
    PollGamepads();
}

void Input::PumpEvents() {
    SDL_Event e;
    Init();
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT)
            m_quit = true;
        if (e.type == SDL_JOYDEVICEADDED)
            DeviceAdded(e.jdevice.which);
        g_video.Event(&e);
    }
}

/* Once per game frame, before the actions are read: sample the keyboard and gamepads. */
void Input::Update() {
    uint8_t down[SDL_NUM_SCANCODES];
    const Uint8 *k;
    uint32_t acts = 0;
    int sc, a;
    Init();
    SDL_PumpEvents();
    k = SDL_GetKeyboardState(nullptr);
    for (sc = 0; sc < SDL_NUM_SCANCODES; sc++)
        down[sc] = k ? k[sc] : 0;
    if (SDL_GetModState() & KMOD_ALT)
        down[SDL_SCANCODE_RETURN] = 0; /* Alt+Enter switches fullscreen (Video::Event) */
    ScriptedKeys(down);
    for (sc = 0; sc < SDL_NUM_SCANCODES; sc++) {
        m_keyPressed[sc] = down[sc] && !m_keyDown[sc];
        m_keyDown[sc] = down[sc];
    }
    m_scripted = ScriptedButtons();
    PollGamepads();
    if (pc_trace) {
        for (a = 0; a < PC_ACT_COUNT; a++)
            if (ActionDown(0, (PcAction)a))
                acts |= 1u << a;
        if (acts != m_traceLastDown) {
            char list[512] = "";
            for (a = 0; a < PC_ACT_COUNT; a++)
                if (acts >> a & 1)
                    snprintf(list + strlen(list), sizeof list - strlen(list), " %s%s",
                             s_actions[a].set == kSetMenu ? "Menu" : "", s_actions[a].name);
            PC_Trace("player 1 at vblank %u:%s", PC_FrameCount(), acts ? list : " -");
            m_traceLastDown = acts;
        }
    }
}

/* The player's gamepad motors, as the game sets a DualShock's: small on (nonzero) or off, large
 * 0..255. Nothing without a gamepad. */
void Input::Rumble(int player, int small, int large) {
    int slot;
    if (player < 0 || player > 1 || (slot = m_players[player].slot) < 0)
        return;
    Player &pl = m_players[player];
    small = small ? 0xffff : 0; /* the DualShock's small motor is on or off */
    large = large * 0x101;
    /* SDL's rumble runs out after a second: renew it while it lasts */
    if (small == pl.rumble[0] && large == pl.rumble[1] &&
        (!(small | large) || PC_FrameCount() - pl.rumbleRenewed < 30))
        return;
    pl.rumble[0] = small;
    pl.rumble[1] = large;
    pl.rumbleRenewed = PC_FrameCount();
    RumbleSlot(slot, (WORD)large, (WORD)small);
}

bool Input::ActionDown(int player, PcAction a) {
    int s, slot;
    if (player < 0 || player > 1 || a < 0 || a >= PC_ACT_COUNT)
        return false;
    slot = m_players[player].slot;
    if (player == 0 && ScriptedAction(a))
        return true;
    for (s = 0; s < kBindSlots; s++) {
        int key = m_key[a][s], pad = m_players[player].pad[a][s];
        if (player == 0 && key != PC_BIND_NONE && m_keyDown[key])
            return true;
        if (slot >= 0 && pad != PC_BIND_NONE && (m_pads[slot].down >> pad & 1))
            return true;
    }
    return false;
}

/* For the controls screen, as of the last Update: a key (player 0) or a button on the player's
 * gamepad that went down, or PC_BIND_NONE. */
int Input::Capture(int player, int gamepad) {
    int i;
    if (!gamepad) {
        if (player != 0)
            return PC_BIND_NONE;
        for (i = 1; i < SDL_NUM_SCANCODES; i++)
            if (m_keyPressed[i])
                return i;
    } else if (player >= 0 && player < 2 && m_players[player].slot >= 0) {
        uint32_t m = m_pads[m_players[player].slot].pressed;
        for (i = 0; i < kPadCount; i++)
            if (m >> i & 1)
                return i;
    }
    return PC_BIND_NONE;
}

/* Whether the "clear binding" input went down (Delete, or X on the player's gamepad). */
bool Input::ClearPressed(int player) {
    if (player == 0 && m_keyPressed[SDL_SCANCODE_DELETE])
        return true;
    return player >= 0 && player < 2 && m_players[player].slot >= 0 &&
           (m_pads[m_players[player].slot].pressed >> kPadX & 1);
}

}   // namespace pc

/* ---- the game's interface (pc_input.h, pc_settings.h, pc_runtime.h) ---- */

const char *PC_InputActionLabel(PcAction a) { return pc::Input::ActionLabel(a); }
const char *PC_InputInputLabel(int gamepad, int input) { return pc::g_input.InputLabel(gamepad, input); }
int PC_InputGetBind(int player, int gamepad, PcAction a, int slot) { return pc::g_input.GetBind(player, gamepad, a, slot); }
void PC_InputBind(int player, int gamepad, PcAction a, int slot, int input) { pc::g_input.Bind(player, gamepad, a, slot, input); }
void PC_InputResetDevice(int player, int gamepad) { pc::g_input.ResetDevice(player, gamepad); }
int PC_InputSaveSettings(void) { return pc::g_input.SaveSettings(); }
int PC_InputCapture(int player, int gamepad) { return pc::g_input.Capture(player, gamepad); }
int PC_InputClearPressed(int player) { return pc::g_input.ClearPressed(player); }
void PC_InputUpdate(void) { pc::g_input.Update(); }
void PC_InputRumble(int player, int small, int large) { pc::g_input.Rumble(player, small, large); }
int PC_InputPlayerConnected(int player) { return pc::g_input.PlayerConnected(player); }
int PC_InputGamepadConnected(int player) { return pc::g_input.GamepadConnected(player); }
int PC_InputActionDown(int player, PcAction a) { return pc::g_input.ActionDown(player, a); }


void PC_RequestQuit(void) { pc::g_input.RequestQuit(); }
