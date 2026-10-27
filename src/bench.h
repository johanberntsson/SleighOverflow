// The DMA measured on the machine, before the game starts: `make BENCH=1`.
#ifndef BENCH_H
#define BENCH_H

// Runs every measurement twice -- on the ROM's text screen, and with the
// game's full-colour display up -- prints a table on the text screen and
// waits for a key. Before vic4_init, because it prints through the Kernal.
void bench_run(void);

#endif
