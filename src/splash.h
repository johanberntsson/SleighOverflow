// The BASIC session on the ROM's text screen that opens the game.
#ifndef SPLASH_H
#define SPLASH_H

// `READY.`, `DLOAD "SANTA"` typed out, `SEARCHING FOR SANTA`, `LOADING`.
void splash_before_load(void);

// A dot after `LOADING`, to show that it is: load_resources' report.
void splash_loading(void);

// `READY.`, `RUN` typed out, and `ERROR 65 IN 2026: SLEIGH OVERFLOW`.
void splash_after_load(void);

#endif
