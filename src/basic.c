// The end: back to BASIC, with the game's last joke in its memory. The
// screen fades up blue and empty, BASIC prints READY., and LIST is in the
// keyboard queue, so it lists
//
//   10 PRINT "MERRY CHRISTMAS"
//   20 GOTO 10
//
// -- a real program, which RUN runs. This puts the hardware back;
// src/basic_asm.s puts back the memory, with the program in it, and returns.
#include <mega65.h>

#include "basic.h"
#include "dma.h"
#include "vic4.h"

#define SNAP_LOW 0x8710000UL  // $0002-$1FFF as BASIC had it; basic_asm.s

static uint8_t cia2_cra, cia2_crb;

void basic_save(void)
{
  cia2_cra = CIA2.cra;
  cia2_crb = CIA2.crb;
}

void basic_return(void);

void basic_ending(void)
{
  uint32_t screen;
  uint8_t i;

  // All four SIDs silent, by the CPU: a DMA to $D400 writes the RAM under
  // the I/O, and left the tune's last note sounding. Every gate down first,
  // then every register, volumes included, to 0.
  for (i = 0; i < 12; i++)
    *(volatile uint8_t *)(0xD404u + (i / 3) * 0x20u + (i % 3) * 7u) = 0;
  for (i = 0; i < 0x80; i++)
    *(volatile uint8_t *)(0xD400u + i) = 0;
  CIA2.cra = (uint8_t)(cia2_cra & ~0x10);  // without the force-load bit
  CIA2.crb = (uint8_t)(cia2_crb & ~0x10);

  // BASIC's display, black, its text screen empty: inside the blank, so the
  // splash still on that screen never shows. The ROM's banner, as it booted,
  // is emptied in the copy that goes back.
  vic4_wait_vbl();
  vic4_leave();
  vic4_rom_palette(0);
  screen = VICIV.scrnptr & 0x0FFFFFFFUL;
  dma_fill(screen, ' ', 2000);
  dma_fill(SNAP_LOW + screen, ' ', 2000);

  for (i = 1; i <= 16; i++) {
    vic4_wait_vbl();
    vic4_wait_vbl();
    vic4_rom_palette(i);
  }
  basic_return();
}
