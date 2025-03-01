/*
 * pitches.h
 *
 *  Created on: Mar 1, 2025
 *      Author: mickey
 */

#ifndef INC_PITCHES_H_
#define INC_PITCHES_H_

#define TCLOCK_HZ 72000000

#define PITCH_PERIOD(pitch_hz) (uint32_t)((TCLOCK_HZ) / (float)(pitch_hz))

// hz values of note pitches
#define B2 PITCH_PERIOD(123.47)

#define C3 PITCH_PERIOD(130.81)
#define Db3 PITCH_PERIOD(138.59)
#define D3 PITCH_PERIOD(146.83)
#define Eb3 PITCH_PERIOD(155.56)
#define E3 PITCH_PERIOD(164.81)
#define F3 PITCH_PERIOD(174.61)
#define Gb3 PITCH_PERIOD(185)
#define G3 PITCH_PERIOD(196)
#define Ab3 PITCH_PERIOD(207.65)
#define A3 PITCH_PERIOD(220)
#define Bb3 PITCH_PERIOD(233.08)
#define B3 PITCH_PERIOD(246.94)

#define C4 PITCH_PERIOD(261.63)
#define Db4 PITCH_PERIOD(277.18)
#define D4 PITCH_PERIOD(293.66)
#define Eb4 PITCH_PERIOD(311.13)
#define E4 PITCH_PERIOD(329.63)
#define F4 PITCH_PERIOD(349.23)
#define Gb4 PITCH_PERIOD(369.99)
#define G4 PITCH_PERIOD(392)
#define Ab4 PITCH_PERIOD(415.3)
#define A4 PITCH_PERIOD(440)
#define Bb4 PITCH_PERIOD(466.16)
#define B4 PITCH_PERIOD(493.88)

#define C5 PITCH_PERIOD(523.25)
#define Db5 PITCH_PERIOD(554.37)
#define D5 PITCH_PERIOD(587.33)
#define Eb5 PITCH_PERIOD(622.25)
#define E5 PITCH_PERIOD(659.25)
#define F5 PITCH_PERIOD(698.46)
#define Gb5 PITCH_PERIOD(739.99)
#define G5 PITCH_PERIOD(783.99)
#define Ab5 PITCH_PERIOD(830.61)
#define A5 PITCH_PERIOD(880)
#define Bb5 PITCH_PERIOD(932.33)
#define B5 PITCH_PERIOD(987.77)

#define C6 PITCH_PERIOD(1046.5)

#endif /* INC_PITCHES_H_ */
