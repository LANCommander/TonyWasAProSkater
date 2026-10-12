/* libpad. PadInitMtap registers two 34-byte receive buffers that the game's ReadControllers
 * copies every frame. They only say whether a pad is there: status (0 = ok, 0xff = no pad), id
 * 0x41 (digital), no buttons. The buttons don't go through them on PC: Pad_Update reads the bound
 * actions (pc_input.c). */
#include <string.h>

#include "pc_hle.h"
#include "pc_input.h"
#include "pc_runtime.h"

static uint32_t s_buf[2];
static int s_padEnabled;

static void FillPadBuffer(int port, uint8_t *buf) {
    if (!PC_InputPlayerConnected(port)) {
        buf[0] = 0xff; /* nothing connected */
        buf[1] = 0;
        return;
    }
    buf[0] = 0x00;
    buf[1] = 0x41; /* digital pad */
    buf[2] = buf[3] = 0xff; /* no buttons (active low) */
    buf[4] = buf[5] = buf[6] = buf[7] = 0x80;
}

void PC_InputPoll(void) {
    int p;
    if (!s_padEnabled)
        return;
    for (p = 0; p < 2; p++)
        if (s_buf[p])
            FillPadBuffer(p, PSX_PTR(s_buf[p]));
}

/* ---- libpad entry points ---------------------------------------------------------------------- */

void PadInitMtap(uint32_t buf1, uint32_t buf2) {
    s_buf[0] = buf1;
    s_buf[1] = buf2;
    memset(PSX_PTR(buf1), 0xff, 34);
    memset(PSX_PTR(buf2), 0xff, 34);
    PC_InputInit();
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
