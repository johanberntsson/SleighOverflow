// The tune: Christmas songs, arranged for four SIDs by tools/convsong.py.
#ifndef MUSIC_H
#define MUSIC_H

#include <stdint.h>

// One instrument: the SID's waveform bits, its envelope and a pulse width.
struct music_instrument {
  uint8_t ctrl;
  uint8_t ad;
  uint8_t sr;
  uint16_t pw;
};

extern const struct music_instrument music_instruments[];

// Each SID's master volume, 0-15: the chords have SIDs of their own, and
// this is how loud they are against the rest.
extern const uint8_t music_volume[4];

// Start the tune from the top, once MUSIC.BIN is in attic RAM. Does nothing
// if it is already playing: the game calls it each time it starts.
void music_start(void);

// One frame of the tune. Called from waitvbl, once for every frame that has
// gone by, so a frame the game took too long over does not slow the tune.
void music_frame(void);

#endif
