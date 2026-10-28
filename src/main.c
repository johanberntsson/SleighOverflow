// Load everything, then play. The game itself is src/game.c.
#include <calypsi/intrinsics6502.h>
#include <mega65.h>
#include <stddef.h>

#include "bench.h"
#include "loader.h"
#include "splash.h"

void game_main(void);

// The loading screen is the ROM's own, and the splash: see src/splash.c.
int main(void)
{
  splash_before_load();
  if (load_resources(NULL)) {
    VICIV.bordercol = 2;  // red: see loader_error() in a memory dump
    for (;;)
      ;
  }
  splash_after_load();

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
