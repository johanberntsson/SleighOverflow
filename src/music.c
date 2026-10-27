// The player: twelve byte streams, one per SID voice, a step a frame.
//
// The format is tools/convmidi.py's, and all the arranging was done there;
// this does exactly what each stream says. It runs from waitvbl and not from
// an interrupt. The game keeps interrupts off -- the Kernal's handler would
// scan the keyboard under input_scan -- and every frame passes through
// waitvbl, which knows how many frames have gone by since it last ran.
#include <mega65.h>

#include "loader.h"
#include "music.h"

#define VOICES 12

#define WAIT_MAX 0x7F
#define NOTE_ON  0x80
#define NOTE_OFF 0xC0
#define SET_FREQ 0xC1
#define LOOP     0xFF

static const uint8_t __far *const data = (const uint8_t __far *)ATTIC_MUSIC;

// Not static, so that a -dumpmem image can be read for them by name: see
// build/sleigh.lst.
int16_t music_start_at[VOICES];  // where each stream begins
int16_t music_pos[VOICES];       // and where it has got to
uint8_t music_wait[VOICES];      // frames left before reading on
static uint8_t wave[VOICES];   // the sounding instrument's waveform bits
static int16_t slide[VOICES];
static uint16_t freq[VOICES];
static uint8_t playing;

// A voice's seven registers. SIDs are $20 apart from $D400: 1 and 2 lean
// left, 3 and 4 right, in the MEGA65's default mix.
static volatile uint8_t *voice_regs(uint8_t v)
{
  uint8_t sid = v / 3;

  return (volatile uint8_t *)(0xD400u + sid * 0x20u + (v - sid * 3) * 7u);
}

// Two reads into two variables: Calypsi has dropped one of two byte reads
// combined into a word in a single expression (../SearchAndRescue/CLAUDE.md).
static uint16_t word_at(int16_t at)
{
  uint8_t lo = data[at];
  uint8_t hi = data[at + 1];

  return (uint16_t)hi << 8 | lo;
}

void music_start(void)
{
  uint8_t v, sid;

  if (playing)
    return;
  for (sid = 0; sid < 4; sid++)
    *(volatile uint8_t *)(0xD418u + sid * 0x20u) = 0x0F;  // full volume
  for (v = 0; v < VOICES; v++) {
    volatile uint8_t *r = voice_regs(v);

    r[4] = 0;  // gate off, no waveform
    music_start_at[v] = music_pos[v] = (int16_t)word_at((int16_t)(v * 2));
    music_wait[v] = 0;
    slide[v] = 0;
  }
  playing = 1;
}

static void set_freq(volatile uint8_t *r, uint16_t f)
{
  r[0] = (uint8_t)f;
  r[1] = (uint8_t)(f >> 8);
}

static void step(uint8_t v)
{
  volatile uint8_t *r = voice_regs(v);

  if (music_wait[v]) {
    music_wait[v]--;
    if (slide[v]) {
      freq[v] += (uint16_t)slide[v];
      set_freq(r, freq[v]);
    }
    return;
  }
  for (;;) {
    uint8_t b = data[music_pos[v]++];

    if (b <= WAIT_MAX) {
      music_wait[v] = (uint8_t)(b - 1);
      return;
    }
    if (b == LOOP) {
      music_pos[v] = music_start_at[v];
    } else if (b == NOTE_OFF) {
      r[4] = wave[v];
      slide[v] = 0;
    } else if (b == SET_FREQ) {
      freq[v] = word_at(music_pos[v]);
      music_pos[v] += 2;
      set_freq(r, freq[v]);
    } else {
      const struct music_instrument *i = &music_instruments[b & 0x3F];

      freq[v] = word_at(music_pos[v]);
      music_pos[v] += 2;
      // The envelope before the gate: it starts from these the moment the
      // gate goes up.
      r[5] = i->ad;
      r[6] = i->sr;
      r[2] = (uint8_t)i->pw;
      r[3] = (uint8_t)(i->pw >> 8);
      set_freq(r, freq[v]);
      wave[v] = i->ctrl;
      slide[v] = i->slide;
      r[4] = (uint8_t)(i->ctrl | 1);
    }
  }
}

void music_frame(void)
{
  uint8_t v;

  if (!playing)
    return;
  for (v = 0; v < VOICES; v++)
    step(v);
}
