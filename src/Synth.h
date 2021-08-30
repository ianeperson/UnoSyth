/*
 * Synth.h
 *
 *  Created on: Jul 17, 2021
 *      Author: Ian Eperson - ian.eperson@dedf.co.uk
 */

#ifndef _Synth_H_
#define _Synth_H_

void playnote (unsigned char note, int etime);
void stopnote (unsigned char note, int etime);
void plot (int a, int b, int c, int d);

#endif /* _Synth_H_ */
