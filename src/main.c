// Load everything, then play. The game itself is src/game.c.
#include <calypsi/intrinsics6502.h>
#include <mega65.h>

#include "bench.h"
#include "loader.h"

void game_main(void);

// The loading screen is the ROM's own, so progress is the border colour.
static void loading(uint8_t done)
{
  VICIV.bordercol = done;
}

int main(void)
{
  if (load_resources(loading)) {
    VICIV.bordercol = 2;  // red: see loader_error() in a memory dump
    for (;;)
      ;
  }

#if BENCH
  bench_run();
#endif

  // The Kernal's interrupt would scan the keyboard under input_scan and blink
  // a cursor into colour RAM; nothing after this needs it.
  __disable_interrupts();
  // The original ends after the boat comes; this starts it again.
  for (;;)
    game_main();
}
