/* The screens and what the window shows (src/pc_screen.cpp; the game's interface is pc_screen.h):
 * the CPU's writes and reads of the game's screen areas, which go to the renderer's render
 * targets (pc/gl_gpu.h), the display the game set up (its DISPENV), and a picture shown instead
 * of the game's screen (full-screen movies). */
#pragma once

#include <stdint.h>

#include "pc_screen.h"

namespace pc {

/* What the display shows. */
struct DisplayState {
    int x, y;         /* the screen area shown */
    int w, h;         /* its size in pixels (h 480 interlaced) */
    int interlaced;
    int enabled;
};

class Screens {
  public:
    /* Fill a screen area with one colour, rgb = 0x00BBGGRR (8 bits each, as ClearImage). */
    void Fill(int x, int y, int w, int h, uint32_t rgb);
    /* Pixels (w x h, rows of w) over a screen area. */
    void Blit(int x, int y, int w, int h, const void *src);
    /* An image (sw x sh pixels) filling the screen area x, y, w, h, drawn there filtered. */
    void BlitHiRes(int x, int y, int w, int h, const void *src, int sw, int sh);
    /* A screen area's pixels as drawn, at the PS1's resolution (w x h into dst). */
    void Read(int x, int y, int w, int h, void *dst);

    /* The display (the game's Psy-Q DISPENV): the screen area shown, and its size as a TV
     * showed it. */
    void Show(const void *dispenv);
    /* Show the display, or black. */
    void Enable(int on) { m_disp.enabled = on != 0; }
    /* The display at power-on: off. */
    void Reset();
    const DisplayState &Display() const { return m_disp; }

    /* Show a picture (w x h, 0xAARRGGBB rows, copied) at 4:3 instead of the game's screen, until
     * ShowGame. */
    void ShowImage(const uint32_t *argb, int w, int h);
    void ShowGame() { m_imageOn = 0; }
    /* The picture being shown (NULL: the game's screen). */
    const uint32_t *Image(int *w, int *h) const;

  private:
    uint32_t *m_image = nullptr;
    int m_imageW = 0, m_imageH = 0, m_imageCap = 0, m_imageOn = 0;
    DisplayState m_disp = {};
    int m_warned24 = 0;   /* a 24-bit display was logged */
};

extern Screens g_screens;

}   // namespace pc
