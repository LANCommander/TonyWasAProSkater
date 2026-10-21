/* The frame clock (pc_frame.cpp): 60 frames a second, as the game was written for (NTSC). Each
 * frame polls input, advances streaming, runs the game's frame handler and presents. Frames only
 * happen when the game waits for one or polls (the port has no interrupts), so code that spins
 * on state a frame updates must call Poll. The game's C interface is pc_frame.h. */
#pragma once

#include <stdint.h>

namespace pc {

class FrameClock {
public:
    static constexpr int kHz = 60;

    /* Block until n more frames have run (each presents). */
    void Wait(int n);
    /* Run the frames that are due by now (at most a few). Returns how many ran. */
    int Poll();
    /* Frames since startup. */
    uint32_t Count() const { return m_frames; }
    /* The game's per-frame handler (its double-buffer flip, Db_Vblank), or NULL for none. */
    void SetHandler(void (*fn)(void));

private:
    int64_t Now() const;
    void Init();
    void RunFrame(bool present = true);
    void WaitFrame();
    void DebugHooks();

    void (*m_handler)(void) = nullptr;
    uint32_t m_frames = 0;
    int64_t m_freq = 0, m_next = 0;
    bool m_inHandler = false;
    bool m_unthrottled = false;
    /* THPS_DETERMINISTIC=1: a virtual clock instead of the wall clock, so automated runs (scripted
     * input, frame dumps) behave the same every time. Each poll advances it 1 ms. */
    bool m_deterministic = false;
    int64_t m_virtualNow = 0;

    /* DebugHooks: frames it has seen, THPS_FRAMES (-1: not read yet), THPS_SHOT, THPS_POKE */
    int m_hookFrames = 0, m_maxFrames = -1;
    const char *m_shots = nullptr, *m_pokes = nullptr;
};

extern FrameClock g_frameClock;

} // namespace pc
