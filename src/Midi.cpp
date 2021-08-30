/*
 * Midi.c
 *
 *  Created on: Feb 7, 2021
 *      Author: Ian Eperson - ian.eperson@dedf.co.uk
 *
 *  Just enough interpretation to play our tunes
 *  - considering what play facilities we have
 */

#include "Arduino.h"
#include <unistd.h>

#include "Midi.h"
#include "Synth.h"

inline uint32_t swap32 (uint32_t x) { unsigned char *p = reinterpret_cast<unsigned char *>(&x);
							p[0] ^= p[3]; p[3] ^= p[0]; p[0] ^= p[3];
							p[1] ^= p[2]; p[2] ^= p[1]; p[1] ^= p[2];
							return ((*(unsigned int *)p)); }
inline uint32_t swap24 (uint32_t x) { unsigned char *p = reinterpret_cast<unsigned char *>(&x);
							p[0] ^= p[2]; p[2] ^= p[0]; p[0] ^= p[2];
							return ((*(unsigned int *)p)); }
inline uint16_t swap16 (uint16_t x) { unsigned char *p = reinterpret_cast<unsigned char *>(&x);
							p[0] ^= p[1]; p[1] ^= p[0]; p[0] ^= p[1];
							return ((*(unsigned int *)p)); }

unsigned int pos;
unsigned char * buffer;
int numtracks;
int tickperquarter;
uint32_t timed;
uint32_t tickcountperusec = 500; /* scaled default */

void bread (unsigned char *p, int n)
{
	while (n-- > 0) *p++ = pgm_read_byte_near (buffer + pos++);
}

uint32_t process_tstamp()
	{
	uint32_t output = 0;
	unsigned char ch;
		do	{
			bread((unsigned char *)&ch, 1);
			output = (output << 7) | (0x7f & ch);
			} while (ch >= 0x80);
//		Serial.print(" timestamp = "); Serial.println(output);
		return (output);
	}

void readfileheader ()
{
struct __attribute__((packed)) Header {
	uint32_t	chunkID;
	uint32_t	chunksize;
	uint16_t 	formattype;
	uint16_t	numtracks;
	uint16_t	timediv;
} h;

	bread ((unsigned char *)&h, sizeof(h));
	numtracks = swap16(h.numtracks);
	timed = swap16(h.timediv);
	tickperquarter = timed; /* assume we don't get frames per second (> 0x8000) */
//	Serial.print(" numtracks = "); Serial.println(numtracks);
//	Serial.print(" timed = "); Serial.print(timed);
}

void readtrackheader ()
{
struct __attribute__((packed)) Track {
	uint32_t	chunkID;
	uint32_t	chunksize;
} t;
	bread((unsigned char *)&t, 8 /* sizeof(Track) */);
		/* we don't use track chunk id or chunk size */
}

int process_event()
{
unsigned char type[2];
unsigned char parms[50];  // how big does this need to be (for the types we recognize)
uint32_t timestamp = (process_tstamp() * (uint32_t)tickcountperusec) / ((uint32_t)tickperquarter);
	bread(&type[0], 1); 	// get event type
	if (type[0] == 0xff)
		{
		bread(&type[0], 2);
		bread(&parms[0], (int)type[1]); // collect the event data in stuff
		switch (type[0])
			{
				case 0x2f : /* done track */ return (0);
				case 0x51 : tickcountperusec = parms[0];
							tickcountperusec = (tickcountperusec << 8) + parms[1];
							tickcountperusec = (tickcountperusec << 8) + parms[2];
							tickcountperusec = tickcountperusec / 1000;
							break;
				case 0x59 : /* key/scale */
				case 0x58 : /* key signature */ break;
				case 0x03 : /* track name */
				case 0x01 : /* text */
				case 0x05 : /* lyrics */
				default : /* unhandled meta event */;
				}
			}
		else
			{
			switch (type[0] & 0xf0)
				{
				case 0x90:  bread(&parms[0], 2);
							playnote (parms[0], timestamp);
							break;
				case 0x80: 	bread(&parms[0], 2);
							stopnote (parms[0], timestamp);
                			break;
				case 0xb0:	bread(&parms[0], 2); /* change controller */
							break;
				case 0xc0:	bread(&parms[0], 1); /* change program */
							break;
//				default: /* unprocessed midi event */ return (0);
				}
			}
		return (1);
}

int process_track()
	{
	readtrackheader ();
	for (;;) if (process_event () == 0) return (1);
	return (0);
	}

void play_track (const unsigned char *p)
{
	pos = 0;
	buffer = (unsigned char *)p;
	readfileheader ();
	for (int i = numtracks; i > 0; i--) process_track ();
	Serial.println("All done");
}


