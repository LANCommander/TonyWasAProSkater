/* libpad + host input. PadInitMtap registers two 34-byte receive buffers that the game reads
 * every frame (ReadControllers); each vblank they are filled from the keyboard/game controller in
 * the hardware format: status (0 = ok, 0xff = no pad), id (0x41 digital, 0x73 analog), the
 * active-low button word, then four analog bytes. */
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pc_hle.h"
#include "pc_runtime.h"

/* PS1 button bits (word = buf[2] | buf[3] << 8, 0 = pressed) */
enum {
    B_SELECT = 0, B_L3 = 1, B_R3 = 2, B_START = 3, B_UP = 4, B_RIGHT = 5, B_DOWN = 6, B_LEFT = 7,
    B_L2 = 8, B_R2 = 9, B_L1 = 10, B_R1 = 11, B_TRIANGLE = 12, B_CIRCLE = 13, B_CROSS = 14,
    B_SQUARE = 15
};

static uint32_t s_buf[2];
static int s_padEnabled;
static int s_quit;
static SDL_GameController *s_ctrl[2];
static int s_inputInit;

static const struct {
    SDL_Scancode key;
    int bit;
} s_keys[] = {
    {SDL_SCANCODE_UP, B_UP},          {SDL_SCANCODE_DOWN, B_DOWN},
    {SDL_SCANCODE_LEFT, B_LEFT},      {SDL_SCANCODE_RIGHT, B_RIGHT},
    {SDL_SCANCODE_Z, B_CROSS},        {SDL_SCANCODE_X, B_CIRCLE},
    {SDL_SCANCODE_A, B_SQUARE},       {SDL_SCANCODE_S, B_TRIANGLE},
    {SDL_SCANCODE_RETURN, B_START},   {SDL_SCANCODE_RSHIFT, B_SELECT},
    {SDL_SCANCODE_BACKSPACE, B_SELECT}, {SDL_SCANCODE_Q, B_L1},
    {SDL_SCANCODE_W, B_R1},           {SDL_SCANCODE_E, B_L2},
    {SDL_SCANCODE_R, B_R2},           {SDL_SCANCODE_SPACE, B_CROSS},
};

static const struct {
    SDL_GameControllerButton b;
    int bit;
} s_btns[] = {
    {SDL_CONTROLLER_BUTTON_A, B_CROSS},          {SDL_CONTROLLER_BUTTON_B, B_CIRCLE},
    {SDL_CONTROLLER_BUTTON_X, B_SQUARE},         {SDL_CONTROLLER_BUTTON_Y, B_TRIANGLE},
    {SDL_CONTROLLER_BUTTON_START, B_START},      {SDL_CONTROLLER_BUTTON_BACK, B_SELECT},
    {SDL_CONTROLLER_BUTTON_DPAD_UP, B_UP},       {SDL_CONTROLLER_BUTTON_DPAD_DOWN, B_DOWN},
    {SDL_CONTROLLER_BUTTON_DPAD_LEFT, B_LEFT},   {SDL_CONTROLLER_BUTTON_DPAD_RIGHT, B_RIGHT},
    {SDL_CONTROLLER_BUTTON_LEFTSHOULDER, B_L1},  {SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, B_R1},
    {SDL_CONTROLLER_BUTTON_LEFTSTICK, B_L3},     {SDL_CONTROLLER_BUTTON_RIGHTSTICK, B_R3},
};

/* Scripted input for automated runs: THPS_INPUT="frame:buttons,frame:buttons,..." where buttons
 * is a hex PS1 button mask (pressed = 1) held from that vblank on. */
static uint16_t ScriptedButtons(void) {
    static const char *spec;
    static int parsed;
    static uint32_t frames[256];
    static uint16_t masks[256];
    static int n;
    uint32_t now = PC_VBlankCount();
    uint16_t m = 0;
    int i;
    if (!parsed) {
        const char *p;
        parsed = 1;
        spec = getenv("THPS_INPUT");
        for (p = spec; p && *p && n < 256;) {
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

void PC_InputInit(void) {
    if (s_inputInit)
        return;
    s_inputInit = 1;
    SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS);
}

static void OpenControllers(void) {
    int i, port = 0;
    for (i = 0; i < SDL_NumJoysticks() && port < 2; i++) {
        if (!SDL_IsGameController(i))
            continue;
        if (!s_ctrl[port])
            s_ctrl[port] = SDL_GameControllerOpen(i);
        port++;
    }
}

void PC_PumpEvents(void) {
    SDL_Event e;
    PC_InputInit();
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT)
            s_quit = 1;
        else if (e.type == SDL_CONTROLLERDEVICEADDED || e.type == SDL_CONTROLLERDEVICEREMOVED)
            OpenControllers();
    }
}

int PC_QuitRequested(void) { return s_quit; }

static uint16_t HostButtons(int port) {
    uint16_t m = 0;
    int i;
    if (port == 0) {
        const Uint8 *k = SDL_GetKeyboardState(NULL);
        if (k)
            for (i = 0; i < (int)(sizeof s_keys / sizeof s_keys[0]); i++)
                if (k[s_keys[i].key])
                    m |= 1u << s_keys[i].bit;
        m |= ScriptedButtons();
    }
    if (s_ctrl[port]) {
        for (i = 0; i < (int)(sizeof s_btns / sizeof s_btns[0]); i++)
            if (SDL_GameControllerGetButton(s_ctrl[port], s_btns[i].b))
                m |= 1u << s_btns[i].bit;
        if (SDL_GameControllerGetAxis(s_ctrl[port], SDL_CONTROLLER_AXIS_TRIGGERLEFT) > 16000)
            m |= 1u << B_L2;
        if (SDL_GameControllerGetAxis(s_ctrl[port], SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > 16000)
            m |= 1u << B_R2;
        {
            int x = SDL_GameControllerGetAxis(s_ctrl[port], SDL_CONTROLLER_AXIS_LEFTX);
            int y = SDL_GameControllerGetAxis(s_ctrl[port], SDL_CONTROLLER_AXIS_LEFTY);
            if (x < -16000) m |= 1u << B_LEFT;
            if (x > 16000) m |= 1u << B_RIGHT;
            if (y < -16000) m |= 1u << B_UP;
            if (y > 16000) m |= 1u << B_DOWN;
        }
    }
    return m;
}

void PC_InputFillPadBuffer(int port, uint8_t *buf) {
    uint16_t m;
    if (port == 1 && !s_ctrl[1]) {
        buf[0] = 0xff; /* nothing connected */
        buf[1] = 0;
        return;
    }
    m = (uint16_t)~HostButtons(port);
    buf[0] = 0x00;
    buf[1] = 0x41; /* digital pad */
    buf[2] = (uint8_t)m;
    buf[3] = (uint8_t)(m >> 8);
    buf[4] = buf[5] = buf[6] = buf[7] = 0x80;
}

void PC_InputPoll(void) {
    int p;
    if (!s_padEnabled)
        return;
    SDL_PumpEvents();
    for (p = 0; p < 2; p++)
        if (s_buf[p])
            PC_InputFillPadBuffer(p, PSX_PTR(s_buf[p]));
}

/* ---- libpad entry points ---------------------------------------------------------------------- */

void PadInitMtap(uint32_t buf1, uint32_t buf2) {
    s_buf[0] = buf1;
    s_buf[1] = buf2;
    memset(PSX_PTR(buf1), 0xff, 34);
    memset(PSX_PTR(buf2), 0xff, 34);
    PC_InputInit();
    OpenControllers();
}

int hle_80096050(void) { /* PadStartCom */
    s_padEnabled = 1;
    PC_InputPoll();
    return 1;
}
int hle_80096070(void) { /* PadStopCom */
    return 1;
}
int PadStartCom(void) { return hle_80096050(); }
int PadStopCom(void) { return 1; }
int PadEnableCom(int mode) {
    (void)mode;
    return 1;
}
int PadGetState(int port) {
    (void)port;
    return 6; /* PadStateStable */
}
int PadInfoAct(int port, int actno, int term) {
    (void)port;
    (void)actno;
    (void)term;
    return 0;
}
void PadSetAct(int port, uint32_t data, int len) {
    (void)port;
    (void)data;
    (void)len;
}
int PadSetActAlign(int port, uint32_t data) {
    (void)port;
    (void)data;
    return 1;
}
int PadSetMainMode(int port, int offs, int lock) {
    (void)port;
    (void)offs;
    (void)lock;
    return 1;
}
