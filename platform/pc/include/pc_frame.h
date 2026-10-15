/* The frame clock (pc_frame.cpp; the class is pc::FrameClock, pc/frame_clock.h): 60 frames a
 * second, as the game was written for (NTSC). Each frame polls input, advances streaming, runs
 * the game's frame handler and presents. Frames only happen when the game waits for one or
 * polls (the port has no interrupts), so code that spins on state a frame updates must call
 * PC_FramePoll. */
#ifndef PC_FRAME_H
#define PC_FRAME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Block until n more frames have run (each presents). */
void PC_FrameWait(int n);
/* Run the frames that are due by now (at most a few). Returns how many ran. */
int PC_FramePoll(void);
/* Frames since startup. */
uint32_t PC_FrameCount(void);
/* The game's per-frame handler (its double-buffer flip, Db_Vblank), or NULL for none. */
void PC_SetFrameHandler(void (*fn)(void));

#ifdef __cplusplus
}
#endif

#endif
