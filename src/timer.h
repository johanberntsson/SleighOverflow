// A clock for measuring: CIA2's two timers as one 32-bit down counter.
#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

// Timer A free-runs off phi2 and timer B counts its underflows, as
// ../SearchAndRescue's profile.c sets them up. Nothing else uses CIA2's
// timers: the Kernal drives them only for RS232. They run at about 1 MHz,
// 19968 ticks to a PAL frame in xemu; timer_frame() measures it.
void timer_init(void);

// Counts DOWN: elapsed is earlier - later.
uint32_t ticks(void);

// Ticks in one frame, measured against the raster: about a tenth of a
// second, since it takes the longest of four.
uint32_t timer_frame(void);

#endif
