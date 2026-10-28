// The end of the game: back to BASIC, through the SYS that started it.
#ifndef BASIC_H
#define BASIC_H

// CIA2 as BASIC had it, before dos_init takes its timers for the clock.
void basic_save(void);

// Fade to BASIC's blue screen and return to READY., where LIST lists the
// last joke. Never returns.
void basic_ending(void);

#endif
