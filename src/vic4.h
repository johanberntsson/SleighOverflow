// VIC-IV full-colour display: 320x200, 8 bits a pixel, double buffered.
#ifndef VIC4_H
#define VIC4_H

#include <stdint.h>

// The original is 320x240, but its art is 320x200: the interiors are 200
// rows of picture between two 20-row black bars, and everything on the beach
// lies between rows 20 and 220. So this shows rows YCROP..YCROP+199 of the
// original's screen, and the display is the stock 40x25 characters.
#define FB_WIDTH  320
#define FB_HEIGHT 200
#define FB_COLS   (FB_WIDTH / 8)    // 40 character strips across
#define FB_ROWS   (FB_HEIGHT / 8)   // 25 characters down each
#define FB_STRIDE (FB_ROWS * 64)    // 1600 bytes per strip
#define FB_BYTES  ((uint16_t)FB_COLS * FB_STRIDE)  // 64000

// Full-colour mode has no linear bitmap: the screen is a grid of 8x8
// characters whose 64 bytes of pixel data live at (character number * 64).
// The characters are laid out in column strips rather than rows, so that a
// vertical run of pixels is one address stepping by 8 for its whole length,
// whatever character rows it crosses:
//
//     address(x, y) = base + (x >> 3) * FB_STRIDE + (x & 7) + y * 8
//
// which is what lets a sprite column be a single DMA job (src/gfx.c).
//
// One buffer per bank, so that the blitter's destination is a 16-bit offset
// and a bank byte that never changes within a frame. Banks 2 and 3 are the
// C65 ROM, and bank 1 ends at $1F800 where colour RAM is aliased into it.
#define FB_A 0x40000UL
#define FB_B 0x50000UL

// The two screen tables, 25 rows of 40 16-bit character numbers. In the RAM
// under the BASIC ROM, which the VIC-IV and the DMA see whatever the CPU has
// mapped there, and which SearchAndRescue used for its loader's buffer: bank
// 1 belongs to the backdrop cache (src/gfx.c), all 63488 bytes of it.
#define SCREEN_A 0x0A000UL
#define SCREEN_B 0x0A800UL

// Set the display up. **Everything that comes off the disk must be loaded
// before this**: the Kernal cannot open a file afterwards (found in
// ../SearchAndRescue, where it looked like a corrupt disk).
void vic4_init(void);

// Put BASIC's display back, as the first vic4_init found it, for printing on
// the ROM's text screen. vic4_init again takes the game's back.
void vic4_leave(void);

// BASIC's sixteen colours at level/16 of their brightness, 0 to 16.
void vic4_rom_palette(uint8_t level);

// One entry from dos-like's 6-bit channels, shown the way dos-like shows
// them: each channel times four. The registers take 8 bits nybble-swapped.
void vic4_set_entry6(uint8_t index, uint8_t r, uint8_t g, uint8_t b);

// And back: an entry's three 6-bit channels, read out of the registers.
void vic4_get_entry6(uint8_t index, uint8_t *rgb);

// All 256 from an interleaved 6-bit table such as gfx_palette6.
void vic4_set_palette6(const uint8_t *rgb);

// Show buffer 0 (FB_A) or 1 (FB_B), from the next frame on: SCRNPTR is
// latched once a frame.
void vic4_show(uint8_t buffer);

uint32_t vic4_base(uint8_t buffer);

// Wait for the raster to reach the bottom border. Called after vic4_show,
// it is the point from which the buffer just hidden may be drawn into.
void vic4_wait_vbl(void);

#endif
