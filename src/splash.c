// The splash is the ROM's own screen. The MEGA65 boots to its banner and
// autoboots the game under it, so the rest of a BASIC session is typed and
// printed there: DLOAD while the resources really load, then RUN, and the
// error the game is named after. Through the Kernal, so before vic4_init.
#include <calypsi/stubs.h>
#include <mega65.h>
#include <string.h>

#include "splash.h"
#include "vic4.h"

#define CURSOR_ON  0xA0  // screen codes: a reversed space
#define CURSOR_OFF 0x20

uint8_t kernal_row(void);
uint8_t kernal_column(void);

// Straight to the Kernal's screen editor: printf would not fit.
static void print(const char *s)
{
  _Stub_write(1, s, strlen(s));
}

static void put(char c)
{
  _Stub_write(1, &c, 1);
}

static void frames(uint8_t n)
{
  while (n--)
    vic4_wait_vbl();
}

// The cursor is a reversed space where the next character will print,
// written straight into screen RAM: the ROM blinks its own only while it
// waits for input, and printing one through the editor shows as its control
// codes inside quotes, as it does when typing BASIC.
static void cursor(uint8_t code)
{
  uint16_t at = kernal_row() * VICIV.linestep + kernal_column();

  *(uint8_t __far *)((VICIV.scrnptr & 0x0FFFFFFFUL) + at) = code;
}

// One key every few frames, a little uneven, as somebody types.
static void type(const char *s)
{
  uint8_t i = 0;

  cursor(CURSOR_ON);
  frames(40);
  for (; *s; s++, i++) {
    put(*s);
    cursor(CURSOR_ON);
    frames((i & 3) == 2 ? 9 : 5);
  }
  frames(20);
  cursor(CURSOR_OFF);
  put('\n');
}

void splash_before_load(void)
{
  print("\nREADY.\n");
  type("DLOAD \"SANTA\"");
  print("\nSEARCHING FOR SANTA\n");
  frames(25);
  print("LOADING");
}

// Not what BASIC prints, but without it the screen sits still for longer
// than looks alive.
void splash_loading(void)
{
  put('.');
}

void splash_after_load(void)
{
  print("\nREADY.\n");
  type("RUN");
  frames(25);
  print("\nERROR 65 IN 2026: SLEIGH OVERFLOW\n");
  frames(150);
}
