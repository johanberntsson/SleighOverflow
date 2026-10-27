// The part of dos-like that Stranded uses, on the MEGA65.
//
// dos-like is Mattias Gustavsson's framework, and src/stranded.c calls
// sixteen of its functions. This is those, with dos-like's names and
// meanings, so that src/game.c can stay as close to the original as it does.
// What is missing: the bitmaps are not loaded through it (they are in attic
// RAM before the game starts, see src/gfx.h), text is drawn from labels
// rendered offline (tools/labels.py), and there is no music yet.
#ifndef DOS_H
#define DOS_H

#include <stdint.h>

enum keycode_t {
  KEY_INVALID,  // ends the list readkeys returns
  KEY_ESCAPE,
  KEY_SPACE,
  KEY_LEFT,
  KEY_RIGHT,
  KEY_F1,
};

// Held now, as of the last waitvbl.
int keystate(enum keycode_t key);

// The keys pressed since the last call, ended by KEY_INVALID -- in practice
// one at most, since the game looks at the first. Each call takes them: a
// second call in the same frame returns none, as in dos-like.
enum keycode_t *readkeys(void);

// Wait for the next frame, and read the keyboard. After a swapbuffers, the
// buffer that was showing is free to draw into once this returns.
void waitvbl(void);

// Show what has been drawn, from the next frame on, and draw into the other
// buffer from now on.
void swapbuffers(void);

// dos-like's palette: 6 bits a channel.
void setpal(int index, int r, int g, int b);
void getpal(int index, int *r, int *g, int *b);

// The game never quits on the MEGA65; see TODO.md.
int shuttingdown(void);

// Set the display up, load the palette, and point drawing at the back
// buffer. Instead of setvideomode, setdoublebuffer and the palette loop.
void dos_init(void);

#endif
