#include <mega65.h>

#include "dos.h"
#include "gfx.h"
#include "input.h"
#include "music.h"
#include "stats.h"
#include "timer.h"
#include "vic4.h"

static uint8_t back;  // the buffer being drawn into

// The border as a timing display, `make TIMING=1`: white from waitvbl --
// the top of the frame -- until swapbuffers, so the band's height is the
// time a frame took; red after it if that ran past a whole frame. Indices
// into the game's palette: COLOR_WHITE, black, and its reddest.
#define BAR_DRAW 31
#define BAR_IDLE 0
#define BAR_LATE 85

static uint32_t frame_ticks, frame_began, last_vbl;

void dos_init(void)
{
  vic4_init();
  vic4_set_palette6(gfx_palette6);
  timer_init();
  frame_ticks = timer_frame();
  back = 1;
  gfx_target(vic4_base(back));
  // playmusic, from the first frame of the title, and not restarted when
  // the game is: the original's loops on under everything.
  last_vbl = ticks();
  music_start();
}

// The palette is the registers' (vic4_get_entry6), not a copy: the ending's
// fade reads it back a step at a time through getpal.
void setpal(int index, int r, int g, int b)
{
  if (index < 0 || index >= 256)
    return;
  vic4_set_entry6((uint8_t)index, (uint8_t)(r & 63), (uint8_t)(g & 63),
                  (uint8_t)(b & 63));
}

// **Blue comes back through a mask of 0xf, not 0xff**, as it does in
// dos-like: its getpal reads `( c >> 16 ) & 0xf` out of the 8-bit colour.
// So the first step of the ending's fade drops every colour's blue to its
// low two bits at once, and the fade starts from there. That is how the
// original looks, so it is how this looks.
void getpal(int index, int *r, int *g, int *b)
{
  uint8_t p[3];

  if (index < 0 || index >= 256)
    return;
  vic4_get_entry6((uint8_t)index, p);
  if (r)
    *r = p[0];
  if (g)
    *g = p[1];
  if (b)
    *b = ((p[2] << 2) & 0xf) >> 2;
}

int shuttingdown(void)
{
  return 0;
}

int keystate(enum keycode_t key)
{
  switch (key) {
  case KEY_ESCAPE:
    // Quitting is not ported: the original exits to the desktop. ESC does
    // nothing; only winning goes back to BASIC (src/basic.c), and an ESC
    // that did the same would call basic_ending without the program.
    return 0;
  case KEY_SPACE:
    return (input_held & IN_SPACE) != 0;
  case KEY_LEFT:
    return (input_held & IN_LEFT) != 0;
  case KEY_RIGHT:
    return (input_held & IN_RIGHT) != 0;
  case KEY_F1:
    return (input_held & IN_F1) != 0;
  default:
    return 0;
  }
}

enum keycode_t *readkeys(void)
{
  // One key at most, and SPACE before the arrows: the game only ever asks
  // whether the first key is SPACE or ESCAPE.
  static enum keycode_t keys[2];
  uint8_t p = input_pressed;

  input_pressed = 0;
  keys[0] = (p & IN_SPACE)   ? KEY_SPACE
            : (p & IN_LEFT)  ? KEY_LEFT
            : (p & IN_RIGHT) ? KEY_RIGHT
                             : KEY_INVALID;
  keys[1] = KEY_INVALID;
  return keys;
}

void waitvbl(void)
{
  uint32_t now, gone;
  uint8_t frames = 1;

  vic4_wait_vbl();
  // The tune moves on by the frames that have gone by, not by the calls: a
  // draw that ran long, such as a backdrop filling the cache, skips one. Three
  // at most, so that a long wait -- the frame statistics, say -- does not
  // come back as a burst.
  now = ticks();
  gone = last_vbl - now;
  last_vbl = now;
  while (gone > frame_ticks + frame_ticks / 2 && frames < 3) {
    gone -= frame_ticks;
    frames++;
  }
  while (frames--)
    music_frame();
  input_scan();
#if STATS
  // F1 prints every frame's draw time on the ROM's screen, then carries on.
  if (input_pressed & IN_F1) {
    input_pressed &= (uint8_t)~IN_F1;
    stats_report(frame_ticks);
    vic4_set_palette6(gfx_palette6);
    gfx_target(vic4_base(back));
    vic4_wait_vbl();
  }
#endif
#if TIMING
  VICIV.bordercol = BAR_DRAW;
#endif
  frame_began = ticks();
}

void swapbuffers(void)
{
  uint32_t took = frame_began - ticks();

  stats_frame(took, frame_ticks, 0);
#if TIMING
  VICIV.bordercol = took > frame_ticks ? BAR_LATE : BAR_IDLE;
#endif
  vic4_show(back);
  back ^= 1;
  gfx_target(vic4_base(back));
}
