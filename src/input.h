// The keyboard and joystick port 2, read straight off the hardware.
#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

// The keys Stranded asks about, as bits. Joystick port 2 stands in for the
// arrows and its fire button for SPACE, which is the whole of the game's
// controls.
#define IN_LEFT  0x01
#define IN_RIGHT 0x02
#define IN_SPACE 0x04
#define IN_ESC   0x08  // ESC, or RUN/STOP
#define IN_F1    0x10  // the frame statistics, in a STATS build

// Read everything once. Called once a frame, from waitvbl: what is held now
// is input_held, and what has gone down since the last scan is ORed into
// input_pressed for readkeys to collect.
void input_scan(void);

extern uint8_t input_held;
extern uint8_t input_pressed;

// Wait for any key to be let go of and then pressed, with interrupts off so
// that the Kernal's own scan does not write the port under it.
void key_wait(void);

#endif
