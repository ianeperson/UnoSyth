/*
 * Interrupt.c
 *
 *  Created on: Jul 17, 2021
 *      Author: Ian Eperson - ian.eperson@dedf.co.uk
 */

#include "Arduino.h"
#include <unistd.h>

#include "Interrupt.h"
#include "Sine.h"		/* for the default note type */

/*
 * for simplicity have everything volatile so the optimization
 * won't hide variables from the interrupt service routine
 */
volatile unsigned char tick = 0;
volatile unsigned int tock = 0;

volatile int total = 0; /* must  hold negative values */
volatile unsigned int ydelta = 1;
volatile unsigned int xdelta = 1;

volatile unsigned char evcount;
volatile unsigned int elim = 0;

/* setup the note waveshape and envelope */
const unsigned char *notebase = snote;
const unsigned char *envelopebase = senvelope;

void setnotetype (const unsigned char *note, const unsigned char *envelope)
{
	notebase = note;
	envelopebase = envelope;
}

/* these values set the note ratios
 * for example (in Midi terms)
 * Note 48 => C3  => 130.81 Hz
 * Note 49 => C#3 => 138.59 Hz
 * Required ratio = 1.05948 (this is the same for any pair of adjacent notes)
 * Our integer division estimate = 18/17 = 1.05882
 * etc
 */
unsigned char divid[] = { 1, 18, 55, 44, 63, 4, 41, 3, 100, 37, 98, 17};
unsigned char divis[] = { 1, 17, 49, 37, 50, 3, 29, 2, 63, 22, 55, 9};

void setupInterrupts ()
{
	cli(); /* stop interrupts whilst we set everything up */
/*  Timer 2: */
	pinMode(11, OUTPUT);	// Configure output pins
	pinMode(3, OUTPUT);		// 0C2A = Pin11, 0C2B = Pin3
	// WGM22, WGM21, WGM20 : 0, 1, 1 : Mode 3 => Fast PWM, TOP = 0xFF, Update OCR2A on BOTTOM, TOV FlagSet on MAX
	// COM2A0, COM2A1 : 1, 0 => Clear OC2A on compare match, set OC2A at BOTTOM,(non-inverting mode)
	// COM2B0, COM2B1 : 1, 1 => Set OC2B on compare match, clear OC2B at BOTTOM,(inverting mode)
	// CS22, CS21, CS20 : 0, 0, 1 => clk/1  (no prescaling)
	TCCR2A = (1 << COM2A1) | (1 << COM2B0) | (1 << COM2B1) | (1 << WGM21) | (1 << WGM20);
	TCCR2B = (1 << CS20);
	TIMSK2 = (1 << TOIE2);  // enable timer compare interrupt
	OCR2A = 200; // some initial values
	OCR2B = 200;
	sei(); // enable interrupts
}

ISR(TIMER2_OVF_vect) /* timer 2 overflow ISR */
{	/* interpolate to the desired note */
	total += ydelta;
	while (total > 0)
		{
		total -= xdelta;
		tick++;
		}
/*
 * interrupt at 62500 Hz, envelope ticker every 256 interrupts, 256 envelope entries
 * 4.096 mSec per tock interrupt step
 * elim sets the number of 4.096 mSec tock interrupts per envelope step
 */
	if (tock++ == elim)
		{
		/* don't wander past the end of the array, don't loop round to restart the envelope */
		if (evcount < 255) evcount++;
		tock = 0; /* restart the timer */
		}
	int val = (((int)pgm_read_byte_near (notebase + tick))-128) * pgm_read_byte_near (envelopebase + evcount);
	OCR2A = OCR2B = (val >> 8) + 127;
}

void playnote (unsigned char note, int period)
{
/*
 * the PWM interrupts occur at 16 MHz / 256
 * our wave shape has 256 elements so one cycle (at one element per interrupt)
 * gives a base note of 244.14 Hz
 * This is closest to Midi note 59
 * Note 59 =>   B3    => 246.94 Hz
 * That is around 1% error or 1/6 of the interval down to
 * Note 58 => A#3/Bb3 => 233.08 Hz
 */
/*
 * To create other notes we need to stretch/compress the base note
 * The basic algorithm is good for the Midi range:
 * Note 0 (8.18Hz) - Note 127 (12543.85Hz)
 * but above Note 100 (2637.02 Hz) the small number of samples per cycle start to degrade waveform
 * above Note 140 beating between the (desired) output frequency and the underlying sample rate starts to dominate_
 */
int pitch = ++note % 12;		/* 0 for Note 35, 47, 59 .... */
int	octave = (note / 12) - 5;	/* 0 for Note 59, 60, 61 .... */
	if (octave < 0)
		{
		ydelta = divid[pitch];
		xdelta = (1 << -octave) * divis[pitch];
		}
	else
		{
		ydelta = (1 << octave) * divid[pitch];
		xdelta = divis[pitch];
		}
}

void stopnote (unsigned char note, int period)
{	/* now we known how long the note needs to be we can calculate what the envelope duration needs to be */
	evcount = tock = 0; /* start new note */
	elim = period / 4; /* elim counts 4.096 millisecond units, delay is milliseconds so close enough */
	while (evcount < 255) delay (10); /* will add, on average, 1% to a 0.5 second half note */
}
