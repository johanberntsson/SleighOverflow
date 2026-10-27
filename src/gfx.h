// Stranded's bitmaps, resident in attic RAM, and the masked blitter that
// stands in for dos-like's maskblit.
#ifndef GFX_H
#define GFX_H

#include <stdint.h>

#include "gfx_ids.h"  // generated: enum bitmap_t, GFX_FILES, GFX_END

// Rows of the original's 320x240 screen above this one: see vic4.h.
#define YCROP 20

// The colour every bitmap is transparent in, as in the original.
#define TRANSPARENT 255

// One bitmap as tools/convgfx.py cropped it: `w` x `h` pixels, stored
// column-major, that sat at (x, y) in the original's file. An animation's
// cels follow each other, w * h bytes apart, all cropped to the same box.
//
// A label (TEXT_n) is a bitmap too, rendered offline, and its (x, y) is
// relative to the label sprite's position, outline and `yp += 30` included.
struct bitmap {
  uint32_t addr;  // attic RAM
  uint16_t w;
  uint8_t h;
  int16_t x;
  int16_t y;
  uint8_t cels;
  uint8_t opaque;  // no TRANSPARENT pixel in it: see drawsprites
};

// The label bitmap for one string drawn in one colour at one wrap width is
// TEXT_0 + its index here. Keyed on all three because the same line could be
// spoken by two characters; it never is, but the key costs nothing.
struct gfx_text {
  const char *text;
  uint8_t color;
  uint8_t wrap;
};

struct gfx_file {
  const char *name;
  uint32_t addr;  // where its stream decrunches to
};

extern const struct bitmap bitmaps[BITMAP_COUNT];
extern const struct gfx_file gfx_files[GFX_FILES];
extern const struct gfx_text gfx_texts[GFX_TEXTS];

// r, g, b interleaved at 6 bits a channel, as dos-like's loadgif returns it.
extern const uint8_t gfx_palette6[768];

// One sprite column: an F018B copy out of attic RAM, a source byte at a time,
// to a destination stepping by 8 -- straight down a character strip -- that
// leaves every destination byte alone where the source is TRANSPARENT. The
// DMA does the masking, so maskblit is no work at all for the CPU.
//
// **One job per column, and that is the cost to watch.** A 34x87 walk cel is
// 34 jobs; the 320-wide backdrop is 320. Each is this struct patched and a
// trigger, and the job options are written every time because the enhanced
// DMA resets them at the end of each job. src/gfx_asm.s does the patching and
// knows the field offsets.
struct gfx_dma_job {
  uint8_t opt_format;  // $0B: F018B job follows the options
  uint8_t opt_src_mb;  // $80
  uint8_t src_mb;      // source address bits 20-27
  uint8_t opt_dst_mb;  // $81
  uint8_t dst_mb;
  uint8_t opt_skip;    // $85
  uint8_t dst_skip;    // destination bytes advanced per byte copied
  uint8_t opt_tval;    // $86
  uint8_t tval;        // the byte that is not written
  uint8_t opt_trans;   // $07: transparency on, $06 off
  uint8_t opt_end;     // $00
  uint8_t command;     // 0 = copy
  uint16_t count;
  uint16_t src;
  uint8_t src_bank;    // source address bits 16-19
  uint16_t dst;
  uint8_t dst_bank;
  uint8_t command_msb;
  uint16_t modulo;
};

extern struct gfx_dma_job gfx_job;

// The first column of a blit, for gfx_columns to step along. gfx_blit sets
// them; src/bench.c drives them directly.
extern uint32_t gfx_col_src;  // 28-bit source address
extern uint8_t gfx_col_h;     // bytes from one source column to the next
extern uint16_t gfx_col_dst;  // offset into the framebuffer's bank
extern uint8_t gfx_col_sub;   // its x & 7
extern uint16_t gfx_col_n;    // columns, at least one

// gfx_col_n jobs, in src/gfx_asm.s.
void gfx_columns(void);

// Draw into this framebuffer from now on.
void gfx_target(uint32_t fb);

// A backdrop -- WORLD, TITLE or an interior: opaque and 200 rows tall -- at
// x, as gfx_blit would draw it, but out of a cache in chip RAM that only
// reads the columns it does not already hold. See src/gfx.c for why. One
// backdrop is cached at a time; switching costs a whole screenful from attic
// RAM, about 1.3 frames, once.
void gfx_backdrop(uint8_t id, int16_t x);

// maskblit of a whole bitmap at the original's screen coordinates: the
// file's top left corner at (x, y) of the 320x240 screen. Clipped to the
// 320x200 window.
void gfx_blit(uint8_t id, uint8_t cel, int16_t x, int16_t y);

#endif
