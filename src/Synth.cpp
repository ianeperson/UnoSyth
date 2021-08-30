
#include "Arduino.h"
#include <unistd.h>

#include "Synth.h"
#include "Interrupt.h"
#include "Midi.h"

#include "Sine.h"
#include "Piano.h"
#include "Tune.h"

void setup()
{
	Serial.begin(115200);
	setupInterrupts ();
}

void loop()
{
	setnotetype (snote, senvelope); /* set up for piano */
	play_track (tune_mid);
}
