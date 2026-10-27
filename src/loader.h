// Reading the resources off the boot D81 into attic RAM.
#ifndef LOADER_H
#define LOADER_H

#include <stdint.h>

// Attic RAM: 8 MB at $8000000. The VIC-IV cannot see it, and DMA out of it
// costs 16 cycles a byte on the real machine against 2.2 from chip RAM --
// xemu does not model that and reports chip RAM speed for both.
#define ATTIC_BASE  0x8000000UL
#define ATTIC_GFX   ATTIC_BASE   // tools/convgfx.py's ATTIC_GFX
#define ATTIC_MUSIC 0x8500000UL  // MUSIC.BIN: the tune's streams, src/music.c
#define ATTIC_STAGE 0x8600000UL  // the crunched stream being unpacked

// Load every GFXn.BIN to where gfx_files says. Returns 0 on success, or -1
// with loader_error() and loader_error_file() saying why. `report`, if not
// null, is called after each file with how many are done.
//
// **Before vic4_init**, which leaves the Kernal unable to open a file.
int load_resources(void (*report)(uint8_t done));

const char *loader_error(void);
const char *loader_error_file(void);

#endif
