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

/* store the note waveshape and envelope */
static unsigned char notes[256];
static unsigned char envelope[256];

void setnotetype (const unsigned char *note, const unsigned char *env)
{
int i;
      for (i = 0; i < 256 ; i++) notes[i] = pgm_read_byte_near (note + i);
      for (i = 0; i < 256 ; i++) envelope[i] = pgm_read_byte_near (env + i);
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
	pinMode(9, OUTPUT);	// Configure output pins
	pinMode(10, OUTPUT);		// 0C1A = Pin9, 0C1B = Pin10
	// WGM13, WGM12, WGM11, WGM10 : 0, 1, 0, 1 : Mode 5 => Fast PWM, TOP = 0xFF, Update OCR1A on BOTTOM, TOV FlagSet on TOP
	// COM1A0, COM1A1 : 1, 0 => Clear OC1A on compare match, set OC1A at BOTTOM,(non-inverting mode)
	// COM1B0, COM1B1 : 1, 1 => Set OC1B on compare match, clear OC1B at BOTTOM,(inverting mode)
	// CS12, CS11, CS10 : 0, 0, 1 => clk/1  (no prescaling)
	TCCR1A = (1 << COM1A1) | (1 << COM1B0) | (1 << COM1B1) | (0 << WGM11) | (1 << WGM10);
	TCCR1B = (1 << CS10) | (0 << WGM13) | (1 << WGM12);
	TIMSK1 = (1 << TOIE1);  // enable timer compare interrupt
	OCR1A = OCR1B = 200; // some initial values
	sei(); // enable interrupts
}

ISR(TIMER1_OVF_vect) /* timer 1 overflow ISR */
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
	OCR1A = OCR1B = (((int)notes[tick]) * (int)envelope[evcount]) >> 8; //* pgm_read_byte_near (envelopebase + evcount);
}

void playnote (unsigned char note, int period)
{
  note = note + 6;
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
