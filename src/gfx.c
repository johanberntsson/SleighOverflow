#include <mega65.h>

#include "dma.h"
#include "gfx.h"
#include "vic4.h"

// The job and its field order are described in gfx.h; src/gfx_asm.s knows
// the offsets.
struct gfx_dma_job gfx_job = {
    0x0B, 0x80, 0, 0x81, 0, 0x85, 8, 0x86, TRANSPARENT, 0x07, 0x00,
    0, 0, 0, 0, 0, 0, 0, 0,
};

void gfx_target(uint32_t fb)
{
  // A framebuffer is one bank of chip RAM, so the destination's upper bytes
  // are set once here and only the 16-bit offset moves.
  gfx_job.dst_mb = 0;
  gfx_job.dst_bank = (uint8_t)(fb >> 16) & 0x0F;
}

uint32_t gfx_col_src;
uint8_t gfx_col_h;
uint16_t gfx_col_dst;
uint8_t gfx_col_sub;
uint16_t gfx_col_n;

// 16 x 16 -> 32 bits on the 45GS02's multiplier.
//
// **Not the C operator.** Calypsi 5.18 compiled `(uint32_t)cel * ((uint32_t)w
// * h)` here to a product with its two low bytes swapped -- cel 1 of a 40x92
// walk, 0x0E60, came out 0x600E -- and the guy was drawn from the middle of
// another cel. ../SearchAndRescue/CLAUDE.md has a page of this family of
// miscompiles. The hardware is also about 25 times faster than the library
// call there was.
static uint32_t mul16(uint16_t a, uint16_t b)
{
  MATH.multina32 = a;
  MATH.multinb32 = b;
  return MATH.multout32;
}

// The run of columns from (cx0, cy0) on screen, already clipped: n columns of
// `count` pixels, the first at src and each h bytes after the last.
static void draw_columns(uint32_t src, uint8_t h, int16_t cx0, int16_t cy0,
                         uint8_t count, uint16_t n)
{
  uint16_t strip_at = (uint16_t)mul16((uint16_t)(cx0 >> 3), FB_STRIDE);

  gfx_col_src = src;
  gfx_col_h = h;
  gfx_col_dst = strip_at + (cx0 & 7) + (uint16_t)cy0 * 8;
  gfx_col_sub = (uint8_t)(cx0 & 7);
  gfx_col_n = n;
  gfx_job.count = count;
  gfx_columns();
}

void gfx_blit(uint8_t id, uint8_t cel, int16_t x, int16_t y)
{
  const struct bitmap *b = &bitmaps[id];
  int16_t x0 = x + b->x, y0 = y + b->y - YCROP;
  int16_t x1 = x0 + (int16_t)b->w, y1 = y0 + b->h;
  int16_t cx0 = x0 < 0 ? 0 : x0, cy0 = y0 < 0 ? 0 : y0;
  int16_t cx1 = x1 > FB_WIDTH ? FB_WIDTH : x1;
  int16_t cy1 = y1 > FB_HEIGHT ? FB_HEIGHT : y1;
  uint32_t cel_at, col_at;
  uint16_t cel_size;

  if (cx0 >= cx1 || cy0 >= cy1)
    return;

  // A cel's size only matters when there is more than one, and every
  // animation's fits sixteen bits; WORLD's 296000 does not, and is cel 0.
  //
  // Each product in a variable of its own: a call inside a 32-bit expression
  // makes Calypsi emit _FillZPQ, which is in none of its libraries.
  cel_size = (uint16_t)mul16(b->w, b->h);
  cel_at = mul16(cel % b->cels, cel_size);
  col_at = mul16((uint16_t)(cx0 - x0), b->h);

  draw_columns(b->addr + cel_at + col_at + (uint16_t)(cy0 - y0), b->h, cx0,
               cy0, (uint8_t)(cy1 - cy0), (uint16_t)(cx1 - cx0));
}

// The backdrop cache: the columns of the backdrop on screen, kept in chip
// RAM so that a frame's 64000 bytes of it are not read out of attic RAM.
//
// **Attic RAM is the whole cost of a frame.** Measured on a real MEGA65 with
// `make BENCH=1` (TODO.md): 59200 bytes as column jobs take 17.6 cycles a
// byte out of attic RAM and 3.0 out of chip RAM. The destination step, the
// transparency and the game's own display cost nothing measurable, and a job
// costs about 200 cycles of its own. So the backdrop stays column jobs and
// moves to chip RAM, and only the columns that scroll on are read from attic
// RAM -- two a frame when walking, 200 bytes each.
//
// A ring of one slot per screen column, the bitmap's column c in slot
// c % 320: when the camera moves, the columns coming on take exactly the
// slots of those going off. Bank 1 has room for 317 slots below the colour
// RAM alias at $1F800, so the last three are in RAM under the BASIC ROM.
#define CACHE_SLOTS FB_WIDTH
#define CACHE_MAIN  317
#define CACHE_BANK1 0x10000UL
#define CACHE_SPILL 0x0B000UL  // to $0B258; beside the screen tables

static uint8_t cache_id;            // INVALID while nothing is cached
static int16_t cache_lo, cache_hi;  // the bitmap's columns held, [lo, hi)

static uint32_t slot_addr(uint16_t slot)
{
  uint32_t at;

  if (slot < CACHE_MAIN) {
    at = mul16(slot, FB_HEIGHT);
    return CACHE_BANK1 + at;
  }
  at = mul16(slot - CACHE_MAIN, FB_HEIGHT);
  return CACHE_SPILL + at;
}

// How many of `want` columns from slot s lie in consecutive slots.
static uint16_t run_length(uint16_t slot, uint16_t want)
{
  uint16_t end = slot < CACHE_MAIN ? CACHE_MAIN : CACHE_SLOTS;

  return want < end - slot ? want : end - slot;
}

// The bitmap's columns [lo, hi) from attic RAM into their slots.
static void fetch(const struct bitmap *b, int16_t lo, int16_t hi)
{
  while (lo < hi) {
    uint16_t slot = (uint16_t)lo % CACHE_SLOTS;
    uint16_t n = run_length(slot, (uint16_t)(hi - lo));
    uint32_t from = mul16((uint16_t)lo, FB_HEIGHT);
    uint32_t to = slot_addr(slot);

    dma_copy(b->addr + from, to, (uint16_t)mul16(n, FB_HEIGHT));
    lo += (int16_t)n;
  }
}

void gfx_backdrop(uint8_t id, int16_t x)
{
  const struct bitmap *b = &bitmaps[id];
  int16_t left = x + b->x;  // where the bitmap's column 0 is on screen
  int16_t lo = left < 0 ? -left : 0;
  int16_t hi = FB_WIDTH - left;

  if (hi > (int16_t)b->w)
    hi = (int16_t)b->w;
  if (lo >= hi)
    return;

  if (id != cache_id || hi <= cache_lo || lo >= cache_hi) {
    fetch(b, lo, hi);
  } else {
    if (lo < cache_lo)
      fetch(b, lo, cache_lo);
    if (hi > cache_hi)
      fetch(b, cache_hi, hi);
  }
  // Columns of the old range outside this one have lost their slots, or
  // are about to: the range is exactly what is on screen.
  cache_id = id;
  cache_lo = lo;
  cache_hi = hi;

  while (lo < hi) {
    uint16_t slot = (uint16_t)lo % CACHE_SLOTS;
    uint16_t n = run_length(slot, (uint16_t)(hi - lo));

    draw_columns(slot_addr(slot), FB_HEIGHT, left + lo, 0, FB_HEIGHT, n);
    lo += (int16_t)n;
  }
}
