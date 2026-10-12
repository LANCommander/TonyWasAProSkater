/* Player input: keyboard and XInput gamepads bound to game actions (see pc_input.h).
 *
 * The game reads the actions themselves: on PC every button slot of its SControl is one action
 * (Ollie, Grind, MenuAccept...), and Pad_Update (decomp/src/unnamed/80070ff4.c) fills each slot
 * from PC_InputActionDown once per game frame. Skating and the menus read different slots, so
 * their bindings are independent. The controls screen (decomp/src/front/controlleroptions.cpp)
 * edits the bindings through the PC_InputBind family. */
#include <windows.h>
#include <xinput.h>
#include <SDL2/SDL.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pc_hle.h"
#include "pc_input.h"
#include "pc_runtime.h"

/* ---- actions ---------------------------------------------------------------------------------- */

static const struct {
    const char *name;  /* Settings.ini */
    const char *label; /* the controls screen */
    PcActionSet set;
    const char *def[4]; /* two keys, two gamepad inputs */
} s_actions[PC_ACT_COUNT] = {
    [PC_ACT_UP] = {"Up", "UP", PC_SET_SKATE, {"W", "Up", "DPadUp", "LSUp"}},
    [PC_ACT_DOWN] = {"Down", "DOWN", PC_SET_SKATE, {"S", "Down", "DPadDown", "LSDown"}},
    [PC_ACT_LEFT] = {"Left", "LEFT", PC_SET_SKATE, {"A", "Left", "DPadLeft", "LSLeft"}},
    [PC_ACT_RIGHT] = {"Right", "RIGHT", PC_SET_SKATE, {"D", "Right", "DPadRight", "LSRight"}},
    [PC_ACT_OLLIE] = {"Ollie", "OLLIE", PC_SET_SKATE, {"Space", "Keypad 2", "A", ""}},
    [PC_ACT_FLIP] = {"Flip", "FLIP", PC_SET_SKATE, {"J", "Keypad 4", "X", ""}},
    [PC_ACT_GRAB] = {"Grab", "GRAB", PC_SET_SKATE, {"L", "Keypad 6", "B", ""}},
    [PC_ACT_GRIND] = {"Grind", "GRIND", PC_SET_SKATE, {"I", "Keypad 8", "Y", ""}},
    [PC_ACT_SPIN_LEFT] = {"SpinLeft", "SPIN LEFT", PC_SET_SKATE, {"Q", "Keypad 7", "LB", ""}},
    [PC_ACT_SPIN_RIGHT] = {"SpinRight", "SPIN RIGHT", PC_SET_SKATE, {"E", "Keypad 9", "RB", ""}},
    [PC_ACT_TAP_SPIN_LEFT] = {"TapSpinLeft", "TAP SPIN LEFT", PC_SET_SKATE, {"Z", "Keypad 1", "LT", ""}},
    [PC_ACT_TAP_SPIN_RIGHT] = {"TapSpinRight", "TAP SPIN RIGHT", PC_SET_SKATE, {"C", "Keypad 3", "RT", ""}},
    [PC_ACT_CAMERA] = {"Camera", "CAMERA", PC_SET_SKATE, {"Tab", "Keypad 5", "Back", ""}},
    [PC_ACT_PAUSE] = {"Pause", "PAUSE", PC_SET_SKATE, {"Escape", "P", "Start", ""}},
    [PC_ACT_MENU_UP] = {"Up", "UP", PC_SET_MENU, {"Up", "W", "DPadUp", "LSUp"}},
    [PC_ACT_MENU_DOWN] = {"Down", "DOWN", PC_SET_MENU, {"Down", "S", "DPadDown", "LSDown"}},
    [PC_ACT_MENU_LEFT] = {"Left", "LEFT", PC_SET_MENU, {"Left", "A", "DPadLeft", "LSLeft"}},
    [PC_ACT_MENU_RIGHT] = {"Right", "RIGHT", PC_SET_MENU, {"Right", "D", "DPadRight", "LSRight"}},
    [PC_ACT_MENU_ACCEPT] = {"Accept", "ACCEPT", PC_SET_MENU, {"Return", "Space", "A", ""}},
    [PC_ACT_MENU_BACK] = {"Back", "BACK", PC_SET_MENU, {"Escape", "Backspace", "B", ""}},
};

static const char *const s_padNames[PC_PAD_COUNT] = {
    "A", "B", "X", "Y", "LB", "RB", "LT", "RT", "Back", "Start", "LS", "RS",
    "DPadUp", "DPadDown", "DPadLeft", "DPadRight",
    "LSUp", "LSDown", "LSLeft", "LSRight", "RSUp", "RSDown", "RSLeft", "RSRight",
};

static const char *const s_padLabels[PC_PAD_COUNT] = {
    "A", "B", "X", "Y", "LB", "RB", "LT", "RT", "BACK", "START", "LS PRESS", "RS PRESS",
    "DPAD UP", "DPAD DOWN", "DPAD LEFT", "DPAD RIGHT",
    "LS UP", "LS DOWN", "LS LEFT", "LS RIGHT", "RS UP", "RS DOWN", "RS LEFT", "RS RIGHT",
};

/* bindings: SDL scancodes (player 1 only) and PcPadInputs per player; PC_BIND_NONE = empty */
static int s_key[PC_ACT_COUNT][PC_BIND_SLOTS];
static int s_pad[2][PC_ACT_COUNT][PC_BIND_SLOTS];

PcActionSet PC_InputActionSet(PcAction a) { return s_actions[a].set; }
const char *PC_InputActionName(PcAction a) { return s_actions[a].name; }
const char *PC_InputActionLabel(PcAction a) { return s_actions[a].label; }
const char *PC_InputPadName(int pad) { return pad >= 0 && pad < PC_PAD_COUNT ? s_padNames[pad] : ""; }

const char *PC_InputKeyName(int sc) {
    if (sc == SDL_SCANCODE_COMMA)
        return "Comma"; /* ',' separates the slots in Settings.ini */
    return sc > 0 && sc < SDL_NUM_SCANCODES ? SDL_GetScancodeName((SDL_Scancode)sc) : "";
}

/* A key's name in the front-end font: upper case letters, digits and spaces only (the menu font
 * has no punctuation, and '[', ']', '<', '>', '{', '}' and '^' are text colour and scale codes). */
static void KeyLabel(int sc, char *out, size_t size) {
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
    int i;
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
        const char *w = NULL;
        for (i = 0; i < (int)(sizeof words / sizeof words[0]); i++)
            if (*name == words[i].c)
                w = words[i].word;
        if (w)
            n += snprintf(out + n, size - n, "%s", w);
        else if (isalnum((unsigned char)*name) || *name == ' ')
            out[n++] = (char)toupper((unsigned char)*name);
        out[n < size ? n : size - 1] = 0;
    }
}

const char *PC_InputInputLabel(int gamepad, int input) {
    static char buf[4][32];
    static int next;
    char *b = buf[next++ & 3];
    if (input == PC_BIND_NONE)
        return "";
    if (gamepad)
        return input >= 0 && input < PC_PAD_COUNT ? s_padLabels[input] : "";
    KeyLabel(input, b, sizeof buf[0]);
    return b;
}

static int ParseKey(const char *s) {
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

static int ParsePad(const char *s) {
    int i;
    if (!*s || !SDL_strcasecmp(s, "None"))
        return PC_BIND_NONE;
    for (i = 0; i < PC_PAD_COUNT; i++)
        if (!SDL_strcasecmp(s, s_padNames[i]))
            return i;
    PC_Log("Settings.ini: unknown gamepad button \"%s\"", s);
    return PC_BIND_NONE;
}

static int *Slot(int player, int gamepad, PcAction a, int slot) {
    if (player < 0 || player > 1 || a < 0 || a >= PC_ACT_COUNT || slot < 0 || slot >= PC_BIND_SLOTS ||
        (!gamepad && player != 0))
        return NULL;
    return gamepad ? &s_pad[player][a][slot] : &s_key[a][slot];
}

int PC_InputGetBind(int player, int gamepad, PcAction a, int slot) {
    int *p = Slot(player, gamepad, a, slot);
    return p ? *p : PC_BIND_NONE;
}

/* An input does one thing per set: if another action of the same set (or this action's other
 * slot) has it, that slot gets this slot's old input instead. */
void PC_InputBind(int player, int gamepad, PcAction a, int slot, int input) {
    int *cur = Slot(player, gamepad, a, slot);
    int old, b, s;
    if (!cur || *cur == input)
        return;
    old = *cur;
    if (input != PC_BIND_NONE)
        for (b = 0; b < PC_ACT_COUNT; b++) {
            if (s_actions[b].set != s_actions[a].set)
                continue;
            for (s = 0; s < PC_BIND_SLOTS; s++) {
                int *o = Slot(player, gamepad, (PcAction)b, s);
                if (o != cur && *o == input)
                    *o = old;
            }
        }
    *cur = input;
}

void PC_InputResetDevice(int player, int gamepad) {
    int a, i;
    for (a = 0; a < PC_ACT_COUNT; a++)
        for (i = 0; i < PC_BIND_SLOTS; i++) {
            int *p = Slot(player, gamepad, (PcAction)a, i);
            if (p)
                *p = gamepad ? ParsePad(s_actions[a].def[2 + i]) : ParseKey(s_actions[a].def[i]);
        }
}

/* ---- Settings.ini ----------------------------------------------------------------------------- */

static const struct {
    const char *name;
    PcActionSet set;
    int gamepad, player;
} s_sections[] = {
    {"Skate Keyboard", PC_SET_SKATE, 0, 0},
    {"Skate Gamepad", PC_SET_SKATE, 1, 0},
    {"Skate Gamepad 2", PC_SET_SKATE, 1, 1},
    {"Menu Keyboard", PC_SET_MENU, 0, 0},
    {"Menu Gamepad", PC_SET_MENU, 1, 0},
    {"Menu Gamepad 2", PC_SET_MENU, 1, 1},
};
#define NUM_SECTIONS ((int)(sizeof s_sections / sizeof s_sections[0]))

static const char *SettingsPath(void) {
    static char path[MAX_PATH];
    if (!path[0])
        snprintf(path, sizeof path, "%s\\Settings.ini", PC_UserDir());
    return path;
}

static char *Trim(char *s) {
    char *e;
    while (isspace((unsigned char)*s))
        s++;
    e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1]))
        *--e = 0;
    return s;
}

int PC_InputSaveSettings(void) {
    char tmp[MAX_PATH];
    FILE *f;
    int s, a, i;
    snprintf(tmp, sizeof tmp, "%s.tmp", SettingsPath());
    f = fopen(tmp, "w");
    if (!f) {
        PC_Log("cannot write %s", tmp);
        return 0;
    }
    fputs("; Tony Hawk's Pro Skater (PC) settings. Options > Player 1/2 Controls edits the controls.\n"
          ";\n"
          "; Controls: each action takes up to two keys and two gamepad buttons, separated by a\n"
          "; comma. Leave a slot empty (or write None) to unbind it.\n"
          "; Keys are SDL key names: letters and digits, Space, Return, Escape, Backspace, Tab,\n"
          "; Left Shift, Right Ctrl, Up, Down, Left, Right, Keypad 2, Comma, ...\n"
          "; Gamepad (XInput) buttons: A B X Y LB RB LT RT Back Start LS RS, DPadUp DPadDown\n"
          "; DPadLeft DPadRight, and the sticks: LSUp LSDown LSLeft LSRight RSUp RSDown RSLeft RSRight.\n"
          "; The keyboard plays player 1. [... Gamepad] is the first gamepad (player 1),\n"
          "; [... Gamepad 2] the second (player 2).\n"
          ";\n"
          "; The Skate controls are read while skating, the Menu controls by the menus, so the same\n"
          "; key can do something different in each. Pause also resumes from the pause menu, and\n"
          "; the pause-menu cheat codes are entered with the Skate controls (hold SpinLeft).\n",
          f);
    for (s = 0; s < NUM_SECTIONS; s++) {
        fprintf(f, "\n[%s]\n", s_sections[s].name);
        for (a = 0; a < PC_ACT_COUNT; a++) {
            int v[PC_BIND_SLOTS], last = PC_BIND_SLOTS - 1;
            if (s_actions[a].set != s_sections[s].set)
                continue;
            for (i = 0; i < PC_BIND_SLOTS; i++)
                v[i] = PC_InputGetBind(s_sections[s].player, s_sections[s].gamepad, (PcAction)a, i);
            while (last >= 0 && v[last] == PC_BIND_NONE)
                last--; /* no trailing empty slots */
            fprintf(f, "%s =", s_actions[a].name);
            for (i = 0; i <= last; i++) {
                const char *n = s_sections[s].gamepad ? PC_InputPadName(v[i]) : PC_InputKeyName(v[i]);
                fprintf(f, "%s%s", i ? ", " : " ", v[i] == PC_BIND_NONE ? "" : n);
            }
            fputs("\n", f);
        }
    }
    if (fclose(f) != 0 || !MoveFileExA(tmp, SettingsPath(), MOVEFILE_REPLACE_EXISTING)) {
        PC_Log("cannot write %s", SettingsPath());
        DeleteFileA(tmp);
        return 0;
    }
    return 1;
}

/* Entries missing from the file keep their defaults. The file is only written when it doesn't
 * exist yet (and when the controls screen changes something). */
static void LoadSettings(void) {
    char line[512];
    int sec = -1;
    FILE *f;
    PC_InputResetDevice(0, 0);
    PC_InputResetDevice(0, 1);
    PC_InputResetDevice(1, 1);
    f = fopen(SettingsPath(), "r");
    if (!f) {
        if (PC_InputSaveSettings())
            PC_Log("controls: wrote the default bindings to %s", SettingsPath());
        return;
    }
    while (fgets(line, sizeof line, f)) {
        char *s = Trim(line), *eq, *field;
        int a, i, n;
        if (!*s || *s == ';' || *s == '#')
            continue;
        if (*s == '[') {
            char *e = strchr(s, ']');
            if (e)
                *e = 0;
            s = Trim(s + 1);
            sec = -1;
            for (i = 0; i < NUM_SECTIONS; i++)
                if (!SDL_strcasecmp(s, s_sections[i].name))
                    sec = i;
            continue;
        }
        eq = strchr(s, '=');
        if (sec < 0 || !eq)
            continue;
        *eq = 0;
        s = Trim(s);
        for (a = 0; a < PC_ACT_COUNT; a++)
            if (s_actions[a].set == s_sections[sec].set && !SDL_strcasecmp(s, s_actions[a].name))
                break;
        if (a == PC_ACT_COUNT) {
            PC_Log("Settings.ini: unknown action \"%s\" in [%s]", s, s_sections[sec].name);
            continue;
        }
        for (n = 0, field = eq + 1; n < PC_BIND_SLOTS; n++) {
            char *comma = field ? strchr(field, ',') : NULL;
            const char *v;
            int *p = Slot(s_sections[sec].player, s_sections[sec].gamepad, (PcAction)a, n);
            if (comma)
                *comma = 0;
            v = field ? Trim(field) : "";
            *p = s_sections[sec].gamepad ? ParsePad(v) : ParseKey(v);
            field = comma ? comma + 1 : NULL;
        }
    }
    fclose(f);
    PC_Log("controls: %s", SettingsPath());
}

/* ---- devices ---------------------------------------------------------------------------------- */

typedef DWORD(WINAPI *XInputGetStateFn)(DWORD, XINPUT_STATE *);
static XInputGetStateFn s_xinputGetState;

#define XI_SLOTS 4
#define STICK_THRESHOLD 16000

static struct {
    int connected;
    int retry;        /* polls until an empty slot is tried again */
    uint32_t down;    /* PcPadInput bits */
    uint32_t pressed; /* went down this update */
} s_xi[XI_SLOTS];
static int s_playerSlot[2] = {-1, -1};

static uint8_t s_keyDown[SDL_NUM_SCANCODES];
static uint8_t s_keyPressed[SDL_NUM_SCANCODES]; /* went down this update */
static uint16_t s_scripted;                     /* THPS_INPUT's PS1 buttons */

static int s_init;
static int s_quit;

static void LoadXInput(void) {
    static const char *const dlls[] = {"xinput1_4.dll", "xinput1_3.dll", "xinput9_1_0.dll"};
    int i;
    for (i = 0; i < 3 && !s_xinputGetState; i++) {
        HMODULE m = LoadLibraryA(dlls[i]);
        if (m) {
            s_xinputGetState = (XInputGetStateFn)(void (*)(void))GetProcAddress(m, "XInputGetState");
            if (s_xinputGetState)
                PC_Log("gamepads: %s", dlls[i]);
        }
    }
    if (!s_xinputGetState)
        PC_Log("gamepads: XInput not available");
}

static uint32_t XInputMask(const XINPUT_GAMEPAD *g) {
    static const struct {
        WORD bit;
        int input;
    } btn[] = {
        {XINPUT_GAMEPAD_A, PC_PAD_A},
        {XINPUT_GAMEPAD_B, PC_PAD_B},
        {XINPUT_GAMEPAD_X, PC_PAD_X},
        {XINPUT_GAMEPAD_Y, PC_PAD_Y},
        {XINPUT_GAMEPAD_LEFT_SHOULDER, PC_PAD_LB},
        {XINPUT_GAMEPAD_RIGHT_SHOULDER, PC_PAD_RB},
        {XINPUT_GAMEPAD_BACK, PC_PAD_BACK},
        {XINPUT_GAMEPAD_START, PC_PAD_START},
        {XINPUT_GAMEPAD_LEFT_THUMB, PC_PAD_LS},
        {XINPUT_GAMEPAD_RIGHT_THUMB, PC_PAD_RS},
        {XINPUT_GAMEPAD_DPAD_UP, PC_PAD_DPAD_UP},
        {XINPUT_GAMEPAD_DPAD_DOWN, PC_PAD_DPAD_DOWN},
        {XINPUT_GAMEPAD_DPAD_LEFT, PC_PAD_DPAD_LEFT},
        {XINPUT_GAMEPAD_DPAD_RIGHT, PC_PAD_DPAD_RIGHT},
    };
    uint32_t m = 0;
    int i;
    for (i = 0; i < (int)(sizeof btn / sizeof btn[0]); i++)
        if (g->wButtons & btn[i].bit)
            m |= 1u << btn[i].input;
    if (g->bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
        m |= 1u << PC_PAD_LT;
    if (g->bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
        m |= 1u << PC_PAD_RT;
    if (g->sThumbLY > STICK_THRESHOLD)
        m |= 1u << PC_PAD_LS_UP;
    if (g->sThumbLY < -STICK_THRESHOLD)
        m |= 1u << PC_PAD_LS_DOWN;
    if (g->sThumbLX < -STICK_THRESHOLD)
        m |= 1u << PC_PAD_LS_LEFT;
    if (g->sThumbLX > STICK_THRESHOLD)
        m |= 1u << PC_PAD_LS_RIGHT;
    if (g->sThumbRY > STICK_THRESHOLD)
        m |= 1u << PC_PAD_RS_UP;
    if (g->sThumbRY < -STICK_THRESHOLD)
        m |= 1u << PC_PAD_RS_DOWN;
    if (g->sThumbRX < -STICK_THRESHOLD)
        m |= 1u << PC_PAD_RS_LEFT;
    if (g->sThumbRX > STICK_THRESHOLD)
        m |= 1u << PC_PAD_RS_RIGHT;
    return m;
}

/* Polling an empty XInput slot is slow, so empty slots are only tried every 120 polls. */
static void PollGamepads(void) {
    int i, p = 0;
    for (i = 0; i < XI_SLOTS; i++) {
        XINPUT_STATE st;
        uint32_t was = s_xi[i].down;
        s_xi[i].pressed = 0;
        if (!s_xinputGetState || (!s_xi[i].connected && s_xi[i].retry-- > 0))
            continue;
        if (s_xinputGetState(i, &st) == ERROR_SUCCESS) {
            if (!s_xi[i].connected)
                PC_Log("gamepad %d connected", i);
            s_xi[i].connected = 1;
            s_xi[i].down = XInputMask(&st.Gamepad);
            s_xi[i].pressed = s_xi[i].down & ~was;
        } else {
            if (s_xi[i].connected)
                PC_Log("gamepad %d disconnected", i);
            s_xi[i].connected = 0;
            s_xi[i].retry = 120;
            s_xi[i].down = 0;
        }
    }
    s_playerSlot[0] = s_playerSlot[1] = -1;
    for (i = 0; i < XI_SLOTS && p < 2; i++)
        if (s_xi[i].connected)
            s_playerSlot[p++] = i;
}

/* ---- scripted input (automated runs) ---------------------------------------------------------- */

/* THPS_KEYS="frame:Key+Key,frame:Key,frame:,..." holds the named keys (Settings.ini names) from
 * that vblank on, so runs go through the bindings. */
static void ScriptedKeys(uint8_t *down) {
    static char *spec;
    static int parsed;
    uint32_t now = PC_VBlankCount();
    const char *held = NULL;
    char *p;
    if (!parsed) {
        parsed = 1;
        spec = getenv("THPS_KEYS") ? strdup(getenv("THPS_KEYS")) : NULL;
    }
    for (p = spec; p && *p;) {
        char *colon = strchr(p, ':');
        size_t len;
        if (!colon || (uint32_t)strtoul(p, NULL, 10) > now)
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
static uint16_t ScriptedButtons(void) {
    static int parsed, n;
    static uint32_t frames[256];
    static uint16_t masks[256];
    uint32_t now = PC_VBlankCount();
    uint16_t m = 0;
    int i;
    if (!parsed) {
        const char *p;
        parsed = 1;
        for (p = getenv("THPS_INPUT"); p && *p && n < 256;) {
            unsigned f, b;
            int used;
            if (sscanf(p, "%u:%x%n", &f, &b, &used) != 2)
                break;
            frames[n] = f;
            masks[n++] = (uint16_t)b;
            p += used;
            if (*p == ',')
                p++;
        }
    }
    for (i = 0; i < n; i++)
        if (frames[i] <= now)
            m = masks[i];
    return m;
}

/* What a scripted PS1 button means: on the PS1 the menus read the same buttons as skating. */
static int ScriptedAction(PcAction a) {
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
    return s_scripted >> bit[a] & 1;
}

/* ---- per frame -------------------------------------------------------------------------------- */

void PC_InputInit(void) {
    if (s_init)
        return;
    s_init = 1;
    SDL_InitSubSystem(SDL_INIT_EVENTS);
    LoadSettings();
    LoadXInput();
    PollGamepads();
}

void PC_PumpEvents(void) {
    SDL_Event e;
    PC_InputInit();
    while (SDL_PollEvent(&e))
        if (e.type == SDL_QUIT)
            s_quit = 1;
}

int PC_QuitRequested(void) { return s_quit; }

void PC_InputUpdate(void) {
    static uint32_t s_lastDown;
    uint8_t down[SDL_NUM_SCANCODES];
    const Uint8 *k;
    uint32_t acts = 0;
    int sc, a;
    PC_InputInit();
    SDL_PumpEvents();
    k = SDL_GetKeyboardState(NULL);
    for (sc = 0; sc < SDL_NUM_SCANCODES; sc++)
        down[sc] = k ? k[sc] : 0;
    ScriptedKeys(down);
    for (sc = 0; sc < SDL_NUM_SCANCODES; sc++) {
        s_keyPressed[sc] = down[sc] && !s_keyDown[sc];
        s_keyDown[sc] = down[sc];
    }
    s_scripted = ScriptedButtons();
    PollGamepads();
    if (pc_trace) {
        for (a = 0; a < PC_ACT_COUNT; a++)
            if (PC_InputActionDown(0, (PcAction)a))
                acts |= 1u << a;
        if (acts != s_lastDown) {
            char list[512] = "";
            for (a = 0; a < PC_ACT_COUNT; a++)
                if (acts >> a & 1)
                    snprintf(list + strlen(list), sizeof list - strlen(list), " %s%s",
                             s_actions[a].set == PC_SET_MENU ? "Menu" : "", s_actions[a].name);
            PC_Trace("player 1 at vblank %u:%s", PC_VBlankCount(), acts ? list : " -");
            s_lastDown = acts;
        }
    }
}

int PC_InputPlayerConnected(int player) { return player == 0 || s_playerSlot[player] >= 0; }
int PC_InputGamepadConnected(int player) { return player >= 0 && player < 2 && s_playerSlot[player] >= 0; }

int PC_InputActionDown(int player, PcAction a) {
    int s, slot;
    if (player < 0 || player > 1 || a < 0 || a >= PC_ACT_COUNT)
        return 0;
    slot = s_playerSlot[player];
    if (player == 0 && ScriptedAction(a))
        return 1;
    for (s = 0; s < PC_BIND_SLOTS; s++) {
        int key = s_key[a][s], pad = s_pad[player][a][s];
        if (player == 0 && key != PC_BIND_NONE && s_keyDown[key])
            return 1;
        if (slot >= 0 && pad != PC_BIND_NONE && (s_xi[slot].down >> pad & 1))
            return 1;
    }
    return 0;
}

int PC_InputCapture(int player, int gamepad) {
    int i;
    if (!gamepad) {
        if (player != 0)
            return PC_BIND_NONE;
        for (i = 1; i < SDL_NUM_SCANCODES; i++)
            if (s_keyPressed[i])
                return i;
    } else if (player >= 0 && player < 2 && s_playerSlot[player] >= 0) {
        uint32_t m = s_xi[s_playerSlot[player]].pressed;
        for (i = 0; i < PC_PAD_COUNT; i++)
            if (m >> i & 1)
                return i;
    }
    return PC_BIND_NONE;
}

int PC_InputClearPressed(int player) {
    if (player == 0 && s_keyPressed[SDL_SCANCODE_DELETE])
        return 1;
    return player >= 0 && player < 2 && s_playerSlot[player] >= 0 &&
           (s_xi[s_playerSlot[player]].pressed >> PC_PAD_X & 1);
}
