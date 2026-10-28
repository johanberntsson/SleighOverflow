#include <mega65.h>

#include "dma.h"
#include "vic4.h"

#define COLOUR_RAM 0xFF80000UL

// The first VIC-II line below the character display: it starts at 52 (see
// ../SearchAndRescue/CLAUDE.md, "Full-colour display") and is 200 lines tall.
#define VBL_LINE 252

static void unlock(void)
{
  VICIV.key = 0x47;
  VICIV.key = 0x53;
}

// The framebuffer's own character numbers, column-major: see vic4.h.
static void build_screen_table(uint32_t table, uint32_t fb)
{
  uint16_t __far *screen = (uint16_t __far *)table;
  uint16_t first = (uint16_t)(fb / 64);
  uint8_t row, col;

  for (row = 0; row < FB_ROWS; row++)
    for (col = 0; col < FB_COLS; col++)
      screen[(int16_t)(row * FB_COLS + col)] =
          first + (uint16_t)col * FB_ROWS + row;
}

// The display as BASIC left it, so that the benchmark can hand it back and
// print. From ../SearchAndRescue's vic4.c: the VIC-II and VIC-III registers
// first, because with the hot registers on a write to one of those
// recomputes the VIC-IV side; then the VIC-IV ones exactly as they were, and
// $D05D -- the hot register bit itself -- last. File scope rather than a
// static const in a function, which Calypsi has miscompiled.
static const uint8_t rom_regs[] = {
    0x11, 0x16, 0x18, 0x20, 0x21, 0x31,                    // VIC-II, VIC-III
    0x15, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B,  // VIC-IV
    0x5E, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x68, 0x69,
    0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x70, 0x5D,
};
#define ROM_REGS sizeof rom_regs
#define ROM_REGS_LEGACY 6

static uint8_t rom_values[ROM_REGS];
static uint8_t rom_palette[3][16];

// Called by vic4_init the first time, and only then: a second call would
// keep the game's display as BASIC's. **It used to be the caller's job**, and
// the frame statistics left it out -- vic4_leave then wrote zeros into every
// register here, screen pointer included, and the ROM's screen editor built
// its colour RAM pointer out of them and wrote through it into nowhere.
static void save_rom(void)
{
  uint8_t i;

  unlock();
  for (i = 0; i < ROM_REGS; i++)
    rom_values[i] = *(volatile uint8_t *)(0xD000u + rom_regs[i]);
  // **$D011's top bit reads as the raster's bit 8 and writes as the raster
  // compare's.** Read with the raster past line 255 and written back, it moved BASIC's
  // raster interrupt to a line that never comes: no IRQ after the return,
  // so no cursor. The ROM keeps its compare below 256.
  rom_values[0] &= 0x7F;
  for (i = 0; i < 16; i++) {
    rom_palette[0][i] = PALETTE.red[i];
    rom_palette[1][i] = PALETTE.green[i];
    rom_palette[2][i] = PALETTE.blue[i];
  }
}

// The registers hold each colour nybble-swapped.
static uint8_t scale(uint8_t v, uint8_t level)
{
  v = (uint8_t)(v << 4 | v >> 4);
  v = (uint8_t)(((uint16_t)v * level) >> 4);
  return (uint8_t)(v << 4 | v >> 4);
}

void vic4_rom_palette(uint8_t level)
{
  uint8_t i;

  for (i = 0; i < 16; i++) {
    PALETTE.red[i] = scale(rom_palette[0][i], level);
    PALETTE.green[i] = scale(rom_palette[1][i], level);
    PALETTE.blue[i] = scale(rom_palette[2][i], level);
  }
}

void vic4_leave(void)
{
  uint8_t i;

  unlock();
  vic4_rom_palette(16);
  VICIV.sdbdrwd_msb |= VIC4_HOTREG_MASK;
  for (i = 0; i < ROM_REGS; i++) {
    if (i == ROM_REGS_LEGACY)
      unlock();  // a VIC-II write can relock the VIC-IV
    *(volatile uint8_t *)(0xD000u + rom_regs[i]) = rom_values[i];
  }
}

void vic4_init(void)
{
  static uint8_t saved;
  uint8_t __far *cram = (uint8_t __far *)COLOUR_RAM;
  uint16_t i;

  if (!saved) {
    save_rom();
    saved = 1;
  }

  CPU_PORTDDR = 0x41;  // 40 MHz

  unlock();
  // The hot registers off, or a stray write to a legacy VIC-II register makes
  // the VIC-IV recompute the layout and undo everything below.
  VICIV.sdbdrwd_msb &= (uint8_t)~VIC4_HOTREG_MASK;
  // Whatever sprites the ROM left on: their pointers are somebody's pixels.
  VICIV.spr_ena = 0;

  VICIV.ctrlb &= (uint8_t)~0x80;  // H320, not H640
  VICIV.ctrlc |= VIC4_CHR16_MASK     // 16-bit character numbers
                 | VIC4_FCLRHI_MASK  // full colour for characters above $FF
                 | VIC4_VFAST_MASK;
  VICIV.linestep = FB_COLS * 2;
  VICIV.chrcount = FB_COLS;
  VICIV.chrxscl = 120;  // 1:1
  VICIV.bordercol = 0;
  VICIV.screencol = 0;

  // **Ink 0 on every cell.** The top four bits of a cell's second colour
  // byte are blink, reverse, bold and underline, and the VIC-IV applies them
  // to full-colour characters too: SearchAndRescue's logo grew a blinking
  // underline on the real machine that xemu never showed.
  for (i = 0; i < FB_COLS * FB_ROWS * 2; i++)
    cram[(int16_t)i] = 0;

  // Both buffers black before either is shown.
  dma_fill(FB_A, 0, FB_BYTES);
  dma_fill(FB_B, 0, FB_BYTES);
  build_screen_table(SCREEN_A, FB_A);
  build_screen_table(SCREEN_B, FB_B);
  vic4_show(0);
}

static uint8_t swap6(uint8_t v)
{
  v = (uint8_t)(v << 2);
  return (uint8_t)(v << 4 | v >> 4);
}

void vic4_set_entry6(uint8_t index, uint8_t r, uint8_t g, uint8_t b)
{
  PALETTE.red[index] = swap6(r);
  PALETTE.green[index] = swap6(g);
  PALETTE.blue[index] = swap6(b);
}

static uint8_t unswap6(uint8_t v)
{
  return (uint8_t)((uint8_t)(v << 4 | v >> 4) >> 2);
}

void vic4_get_entry6(uint8_t index, uint8_t *rgb)
{
  rgb[0] = unswap6(PALETTE.red[index]);
  rgb[1] = unswap6(PALETTE.green[index]);
  rgb[2] = unswap6(PALETTE.blue[index]);
}

void vic4_set_palette6(const uint8_t *rgb)
{
  uint16_t i;

  for (i = 0; i < 256; i++, rgb += 3)
    vic4_set_entry6((uint8_t)i, rgb[0], rgb[1], rgb[2]);
}

void vic4_show(uint8_t buffer)
{
  // All four bytes, which also clears the CHRCOUNT high bits and the screen
  // pointer's megabyte in $D063.
  VICIV.scrnptr = (buffer & 1) ? SCREEN_B : SCREEN_A;
}

uint32_t vic4_base(uint8_t buffer)
{
  return (buffer & 1) ? FB_B : FB_A;
}

static uint8_t in_bottom(void)
{
  return (VICIV.ctrl1 & 0x80) || VICIV.rasterline >= VBL_LINE;
}

// Down into the bottom border and then out of it into the next frame, which
// is when the VIC-IV latches a new SCRNPTR. Only after that is the buffer it
// was showing safe to draw into.
void vic4_wait_vbl(void)
{
  while (!in_bottom())
    ;
  while (in_bottom())
    ;
}
