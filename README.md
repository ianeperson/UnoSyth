# UnoSynth

Hardly a full blown synthesizer but this does provide an implementation some of the key synth components – within the constraints of the UNO:

	i)	High speed PWM producing an arbitrary analog audio output voltage
	ii)	Note generator using a pre-defined or calculated single cycle wave shape
	iii)	A stretch/compress algorithm to change the pitch of the note – scales and octaves
	iv)	Minimal Midi parser

In more detail:

The high speed PWM uses Timer 2 to give an (effective) 8 bit resolution analog value at 62,500 Hz (16 MHz/256).  The timer operates in complementary output mode to give a double output voltage swing (Pins 3 & 11) for increased volume and, with no effective DC component, a speaker can be driven directly.
The note generator creates the analog output values from a 256 byte sequence which sets the waveform for a single cycle.  This means any synthetic (sine, square, triangle,...) waveform can be generated or a waveform derived from a real instrument used to set the individual 62,500 Hz values.  The current code has a sampled piano note suitably stretched to provide 256 samples per cycle.

The note  stretcher/compressor uses two arrays of numerator/denominator to approximate to the 12 ratios (1.059,  1.122, 1.259 …) needed for an octave's equal temperament note values.  For higher octaves the  numerator is doubled,  for lower octaves the  denominator is doubled.  This allows the complete range of Midi notes to be produced from Note 0 (8.18Hz) to Note 127 (12543.85Hz), though above Note 100 (2637.02 Hz) the decreasing number of samples per cycle progressively degrades the waveform.

The Midi parser does just enough to extract the note sequence and note duration values since these are the only information our note sequence generator can cope with.  There are two built in tunes – the parser is designed so that the sending Midi files, byte by byte, from an external computer is straightforward.
