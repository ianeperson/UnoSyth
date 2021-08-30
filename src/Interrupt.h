/*
 * Interrupt.h
 *
 *  Created on: Jul 17, 2021
 *      Author: Ian Eperson - ian.eperson@dedf.co.uk
 */

#ifndef INTERRUPT_H_
#define INTERRUPT_H_

void setupInterrupts ();
void setnotetype (const unsigned char *snote, const unsigned char *senvelope);
void playnote (unsigned char note, int period);
void stopnote (unsigned char note, int period);

#endif /* INTERRUPT_H_ */
