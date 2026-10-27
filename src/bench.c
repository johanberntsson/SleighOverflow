// What the blitter's DMA actually costs on the machine.
//
// The first build on a real MEGA65 drew a frame in about two frames, where
// SearchAndRescue's 16 cycles a byte out of attic RAM predicted 1.3. A walk
// time cannot say which of these is the difference, and each points at a
// different renderer, so this measures them apart:
//
//   - attic against chip RAM as the source, the same 296 columns either way
//     (bank 1 holds 296 columns of 200 and no more);
//   - the destination stepping by 8, as every column job does, against 1;
//   - transparency on against off;
//   - the fixed cost of a job, with jobs of one byte;
//   - one long linear copy, which is what a strip-at-a-time renderer would
//     do instead;
//   - and all of it again with the game's display up, whose 64 KB a frame of
//     full-colour character fetches may be competing with the DMA.
//
// Times are CIA2 ticks, about 1 MHz; the frame's own count is printed too.
#include <calypsi/intrinsics6502.h>
#include <mega65.h>
#include <stdio.h>

#include "bench.h"
#include "dma.h"
#include "gfx.h"
#include "input.h"
#include "loader.h"
#include "timer.h"
#include "vic4.h"

#define COLS      296
#define COL_BYTES 200
#define CHIP_SRC  0x10000UL               // bank 1, below the screen tables
#define LINEAR    ((uint16_t)COLS * COL_BYTES)  // 59200

enum { ATTIC, CHIP };

struct test {
  const char *name;
  uint8_t from;   // ATTIC or CHIP
  uint8_t bytes;  // per column; 0 for one linear copy
  uint8_t skip;   // destination step
  uint8_t trans;  // $07 on, $06 off
};

// File scope: see the note on rom_regs in vic4.c.
static const struct test tests[] = {
    {"COLS ATTIC STEP8 TRANS", ATTIC, COL_BYTES, 8, 0x07},
    {"COLS ATTIC STEP8 OPAQUE", ATTIC, COL_BYTES, 8, 0x06},
    {"COLS ATTIC STEP1", ATTIC, COL_BYTES, 1, 0x07},
    {"COLS CHIP STEP8 TRANS", CHIP, COL_BYTES, 8, 0x07},
    {"COLS CHIP STEP1", CHIP, COL_BYTES, 1, 0x07},
    {"JOBS OF 1 BYTE ATTIC", ATTIC, 1, 8, 0x07},
    {"JOBS OF 1 BYTE CHIP", CHIP, 1, 8, 0x07},
    {"LINEAR ATTIC 59200", ATTIC, 0, 1, 0x06},
    {"LINEAR CHIP 59200", CHIP, 0, 1, 0x06},
};
#define TESTS (sizeof tests / sizeof tests[0])

static uint32_t result[2][TESTS];
static uint32_t frame[2];

static uint32_t measure(const struct test *t)
{
  uint32_t src = t->from == CHIP ? CHIP_SRC : ATTIC_GFX;  // WORLD's first columns
  uint32_t t0;

  t0 = ticks();
  if (!t->bytes) {
    dma_copy(src, FB_A, LINEAR);
  } else {
    gfx_job.count = t->bytes;
    gfx_job.dst_skip = t->skip;
    gfx_job.opt_trans = t->trans;
    gfx_col_src = src;
    gfx_col_h = COL_BYTES;
    gfx_col_dst = 0;
    gfx_col_sub = 0;
    gfx_col_n = COLS;
    gfx_columns();
  }
  return t0 - ticks();
}

// Each one twice, keeping the faster: in xemu the first measurement of a
// run came out four times the second, and nothing here should depend on
// what ran before it.
static void run_all(uint32_t *into)
{
  uint8_t i;

  gfx_target(FB_A);
  for (i = 0; i < TESTS; i++) {
    uint32_t a = measure(&tests[i]), b = measure(&tests[i]);

    into[i] = a < b ? a : b;
  }
  // Back as the blitter expects them.
  gfx_job.dst_skip = 8;
  gfx_job.opt_trans = 0x07;
}

void bench_run(void)
{
  uint8_t i;

  timer_init();
  dma_copy(ATTIC_GFX, CHIP_SRC, LINEAR);

  // Interrupts off for every measurement: the Kernal's would land inside
  // them and be counted.
  __disable_interrupts();
  frame[0] = timer_frame();
  run_all(result[0]);

  vic4_init();
  vic4_set_palette6(gfx_palette6);
  frame[1] = timer_frame();
  run_all(result[1]);
  vic4_leave();
  __enable_interrupts();

  printf("\n\nDMA BENCHMARK, CIA2 TICKS\n\n");
  printf("%-24s %7s %7s\n", "", "TEXT", "GAME");
  printf("%-24s %7lu %7lu\n", "ONE FRAME", frame[0], frame[1]);
  for (i = 0; i < TESTS; i++)
    printf("%-24s %7lu %7lu\n", tests[i].name, result[0][i], result[1][i]);
  printf("\nPRESS A KEY\n");

  key_wait();
}
