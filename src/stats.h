// How long every frame took to draw, kept for reading back.
#ifndef STATS_H
#define STATS_H

#include <stdint.h>

// One frame's draw, in CIA2 ticks against a frame of `frame` ticks, with the
// camera where it was. Cheap enough to leave in: a handful of adds.
void stats_frame(uint32_t took, uint32_t frame, int16_t xpos);

// `make STATS=1`: hand the display back to BASIC, print what stats_frame
// has seen, wait for a key, put the game's display back and start counting
// again. The caller puts its palette back; the next frame redraws
// everything anyway.
void stats_report(uint32_t frame);

#endif
