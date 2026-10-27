#include <calypsi/intrinsics6502.h>
#include <mega65.h>

#include "input.h"

uint8_t input_held;
uint8_t input_pressed;

// Rows of the C64 matrix, written to $DC00, with the columns read back from
// $DC01; a key that is down reads 0. From ../SearchAndRescue's input.c:
//
//   row 0 ($FE): DEL  RETURN  CRSR-R  F7  F1  F3  F5  CRSR-D
//   row 1 ($FD): 3  W  A  4  Z  S  E  LSHIFT   (bit 0 first)
//   row 6 ($BF): £  *  ;  HOME  RSHIFT  =  ^  /
//   row 7 ($7F): 1  <-  CTRL  2  SPACE  C=  Q  STOP
static uint8_t row(uint8_t select)
{
  CIA1.pra = select;
  return (uint8_t)~CIA1.prb;
}

// ESC is one of the C65's extra keys, on a ninth row that the MEGA65 reads
// through $D614/$D613 rather than the CIA: bit 7 of row 8.
#define KEYROW_SEL  (*(volatile uint8_t *)0xD614)
#define KEYROW_READ (*(volatile uint8_t *)0xD613)

#if AUTOPLAY
// A scripted walkthrough, for xemu runs that cannot press a key: `make
// AUTOPLAY=n` plays its first n steps and then lets go of everything, so the
// game sits wherever step n left it and a screenshot can be taken there
// however late. The game counts everything in frames, so the same steps
// reach the same state every time.
//
// Each step waits `wait` frames with nothing held, then holds `keys` for
// `hold` frames. A line of dialogue goes on at a SPACE and is gone for 30
// frames first, so presses are 60 apart -- and so a walk that follows the
// last line of a conversation waits 50, or its first frames go by inside
// that delay and it stops short. Santa walks 2 pixels a frame from
// 65, and is near a thing within 25 of it: the sack is at 240, the bear's
// house at 540, the fox's at 850, the sea lion at 970 and the blue gift at
// 1032.
struct step {
  uint16_t wait;
  uint16_t hold;
  uint8_t keys;
};

#define SPACE(n) {60, 1, IN_SPACE}

static const struct step script[] = {
    {150, 1, IN_SPACE},             // 1: the title
    {100, 1, IN_SPACE},             // 2-4: "maybe three and a half years..."
    SPACE(), SPACE(),
    SPACE(),                        // 5: stand up
    {30, 88, IN_RIGHT},             // 6: to the sack, x 241
    {10, 1, IN_SPACE},              // 7: look at it
    SPACE(), SPACE(), SPACE(),      // 8-10: three lines
    SPACE(),                        // 11: back out
    {20, 150, IN_RIGHT},            // 12: to the first hut, x 541
    {10, 1, IN_SPACE},              // 13: go in
    SPACE(), SPACE(), SPACE(), SPACE(), SPACE(), SPACE(),
    SPACE(),                        // 14-20: the bear, seven lines
    {50, 155, IN_RIGHT},            // 21: to the second hut, x 851
    {10, 1, IN_SPACE},              // 22: go in
    SPACE(), SPACE(), SPACE(), SPACE(), SPACE(),
    SPACE(),                        // 23-28: the fox, six lines
    {50, 60, IN_RIGHT},             // 29: to the sea lion, x 971
    {10, 1, IN_SPACE},              // 30: talk to it
    SPACE(), SPACE(), SPACE(), SPACE(), SPACE(), SPACE(),
    SPACE(), SPACE(), SPACE(), SPACE(), SPACE(),
    SPACE(),                        // 31-42: twelve lines, then it waddles off
    {200, 1, IN_SPACE},             // 43: "Yeah, right."
    {50, 31, IN_RIGHT},             // 44: to the blue gift, x 1033
    {10, 1, IN_SPACE},              // 45: take it
    SPACE(),                        // 46: "I'm having this!"
    {50, 397, IN_LEFT},             // 47: back to the sack, x 239
    {10, 1, IN_SPACE},              // 48: the sack, complete
    SPACE(), SPACE(),               // 49-50: MAYDAY
    SPACE(),                        // 51: back out, and the boat comes
};
#define STEPS (sizeof script / sizeof script[0])

static uint8_t autoplay(void)
{
  static uint8_t step;
  static uint16_t frame;
  const struct step *s;

  if (step >= AUTOPLAY || step >= STEPS)
    return 0;
  s = &script[step];
  if (++frame <= s->wait)
    return 0;
  if (frame < s->wait + s->hold)
    return s->keys;
  frame = 0;
  step++;
  return s->keys;
}
#endif

void input_scan(void)
{
  uint8_t r0, r1, r6, r7, joy, held = 0;
  __interrupt_state_t state = __get_interrupt_state();

  // The Kernal scans the keyboard on its own interrupt and would rewrite the
  // row select between these accesses. The game runs with interrupts off,
  // but this is also called on the way in, before they are.
  __disable_interrupts();
  r0 = row(0xFE);
  r1 = row(0xFD);
  r6 = row(0xBF);
  r7 = row(0x7F);
  // Port 2 pulls $DC00's own lines low, and reads back through it with
  // every row deselected: bit 2 left, 3 right, 4 fire.
  CIA1.pra = 0xFF;
  joy = (uint8_t)~CIA1.pra;
  KEYROW_SEL = 8;
  if (!(KEYROW_READ & 0x80))
    held |= IN_ESC;
  __restore_interrupt_state(state);

  // **There is no cursor-left key in the C64 matrix.** The MEGA65's own
  // left arrow arrives as a shift with CRSR-RIGHT, the way a C64 has always
  // spelled it, so either shift turns CRSR-RIGHT into left.
  if (r0 & 0x04) {
    if ((r1 & 0x80) || (r6 & 0x10))
      held |= IN_LEFT;
    else
      held |= IN_RIGHT;
  }
  if (r0 & 0x10)
    held |= IN_F1;
  if (r7 & 0x10)
    held |= IN_SPACE;
  if (r7 & 0x80)
    held |= IN_ESC;
  if (joy & 0x04)
    held |= IN_LEFT;
  if (joy & 0x08)
    held |= IN_RIGHT;
  if (joy & 0x10)
    held |= IN_SPACE;

#if AUTOPLAY
  held = autoplay();
#endif
  input_pressed |= (uint8_t)(held & ~input_held);
  input_held = held;
}

void key_wait(void)
{
  __disable_interrupts();
  CIA1.pra = 0;  // every row
  while (CIA1.prb != 0xFF)  // let go of whatever is held
    ;
  while (CIA1.prb == 0xFF)
    ;
  __enable_interrupts();
}
