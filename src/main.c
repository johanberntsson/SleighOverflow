// Load everything, then play. The game itself is src/game.c.
#include <calypsi/intrinsics6502.h>
#include <mega65.h>

#include "basic.h"
#include "bench.h"
#include "loader.h"
#include "splash.h"

int game_main(void);

// The loading screen is the ROM's own, and the splash: see src/splash.c.
int main(void)
{
  basic_save();
  splash_before_load();
  if (load_resources(splash_loading)) {
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
  // game_main returns when the game is won (ESC, which would also end it,
  // is never pressed: see keystate), and the end goes back to BASIC.
  while (!game_main())
    ;
  basic_ending();
  return 0;  // never: basic_ending returns through BASIC's SYS
}
