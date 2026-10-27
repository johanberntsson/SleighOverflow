#include <mega65.h>

#include "timer.h"
#include "vic4.h"

void timer_init(void)
{
  CIA2.ta_lo = 0xFF;
  CIA2.ta_hi = 0xFF;
  CIA2.tb_lo = 0xFF;
  CIA2.tb_hi = 0xFF;
  CIA2.cra = 0x11;  // force load, start, continuous, phi2 input
  CIA2.crb = 0x51;  // force load, start, continuous, counts A's underflows
}

uint32_t ticks(void)
{
  uint16_t hi, lo, again;

  // Each half runs on between its two byte reads; read again if it moved.
  do {
    hi = (uint16_t)CIA2.tb_hi << 8 | CIA2.tb_lo;
    do {
      lo = CIA2.ta_hi;
      lo = lo << 8 | CIA2.ta_lo;
    } while (CIA2.ta_hi != (uint8_t)(lo >> 8));
    again = (uint16_t)CIA2.tb_hi << 8 | CIA2.tb_lo;
  } while (again != hi);
  return (uint32_t)hi << 16 | lo;
}

// The longest of several. In xemu one frame measured straight after
// vic4_init has come out as 3584 ticks rather than 19968 -- the raster wait
// let go early -- and a short frame makes every draw look late and the whole
// border red. A frame can be measured short and never long.
uint32_t timer_frame(void)
{
  uint32_t t0, t, longest = 0;
  uint8_t i;

  vic4_wait_vbl();
  for (i = 0; i < 4; i++) {
    t0 = ticks();
    vic4_wait_vbl();
    t = t0 - ticks();
    if (t > longest)
      longest = t;
  }
  return longest;
}
