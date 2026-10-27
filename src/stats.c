// The frame-time record behind the border's timing bar.
//
// The bar shows one frame at a time and the eye cannot count red flashes, so
// this keeps every frame: a histogram of draw times, the longest, and the
// first few that ran past a whole frame, with where the camera was. In xemu
// read them out of a -dumpmem image (the addresses are in
// build/sleigh.lst); on the machine `make STATS=1` prints them when F1 is
// pressed.
#include <calypsi/intrinsics6502.h>
#include <stdio.h>

#include "gfx.h"
#include "input.h"
#include "stats.h"
#include "vic4.h"

#define BUCKET_SHIFT 10  // 1024 ticks, about 5% of a frame
#define BUCKETS      24  // the last takes everything longer
#define LATE_KEPT    8

uint16_t stat_frames, stat_late;
uint32_t stat_max;
uint16_t stat_hist[BUCKETS];
uint16_t stat_late_frame[LATE_KEPT];
int16_t stat_late_xpos[LATE_KEPT];
uint32_t stat_late_ticks[LATE_KEPT];

void stats_frame(uint32_t took, uint32_t frame, int16_t xpos)
{
  uint32_t bucket = took >> BUCKET_SHIFT;

  stat_frames++;
  if (took > stat_max)
    stat_max = took;
  stat_hist[bucket >= BUCKETS ? BUCKETS - 1 : (uint8_t)bucket]++;
  if (took > frame) {
    if (stat_late < LATE_KEPT) {
      stat_late_frame[stat_late] = stat_frames;
      stat_late_xpos[stat_late] = xpos;
      stat_late_ticks[stat_late] = took;
    }
    stat_late++;
  }
}

void stats_report(uint32_t frame)
{
  uint8_t i;

  vic4_leave();
  __enable_interrupts();
  printf("\n\nFRAME TIMES, CIA2 TICKS, %lu A FRAME\n\n", frame);
  printf("FRAMES %u  LATE %u  LONGEST %lu\n\n", stat_frames, stat_late,
         stat_max);
  printf("DRAWS BY LENGTH, 1024 TICKS A ROW:\n");
  for (i = 0; i < BUCKETS; i++)
    if (stat_hist[i])
      printf("  %5lu- %5u\n", (uint32_t)i << BUCKET_SHIFT, stat_hist[i]);
  if (stat_late) {
    printf("\nLATE: FRAME, CAMERA X, TICKS\n");
    for (i = 0; i < stat_late && i < LATE_KEPT; i++)
      printf("  %5u %5d %7lu\n", stat_late_frame[i], stat_late_xpos[i],
             stat_late_ticks[i]);
  }
  printf("\nPRESS A KEY\n");
  key_wait();

  vic4_init();
  stat_frames = stat_late = 0;
  stat_max = 0;
  for (i = 0; i < BUCKETS; i++)
    stat_hist[i] = 0;
}
