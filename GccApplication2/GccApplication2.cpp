//**************************************************************************
//*
//*  LED fading test
//*  uses exponential PWM settings to achive visual linear brightness
//*
//*  ATmega32 @ 8 MHz
//*
//**************************************************************************

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#include "SID.h"

#include <util/delay.h>


#define STK500 0

#if STK500
// inverted PWM on OC1A for STK500
#define INVERT_PWM (1 << COM1A0)
#else
// non-inverted PWM on OC1A
#define INVERT_PWM 0
#endif // STK500



SID mySid;

void setup() {
  mySid.begin();
}

#define PAUSE -1
#define STOP -2

const float freqs[] PROGMEM = 
	{8.176, 8.662, 9.177, 9.723, 10.301, 10.913, 11.562, 12.250, 12.978, 13.750, 14.568, 15.434, 16.352, 17.324, 18.354, 19.445,
	 20.601, 21.826, 23.124, 24.499, 25.956, 27.50, 29.135, 30.867, 32.703, 34.648, 36.708, 38.890, 41.203, 43.653, 46.249, 48.999,
	 51.913, 55.000, 58.270, 61.735, 65.406, 69.295, 73.416, 77.781, 82.406, 87.307, 92.499, 97.998, 103.82, 110.00, 116.54, 123.47,
	 130.81, 138.59, 146.83, 155.56, 164.81, 174.61, 184.99, 195.99, 207.65, 220.00, 233.08, 246.94, 261.63, 277.18, 293.66, 311.13,
	 329.63, 349.23, 369.99, 391.99, 415.31, 440.00, 466.16, 439.88, 523.25, 554.37, 587.33, 622.25, 659.26, 698.46, 739.99, 783.99,
	 830.61, 880.00, 932.32, 987.77, 1046.5, 1108.7, 1174.7, 1244.5, 1318.5, 1396.9, 1480.0, 1568.0, 1661.2, 1760.0, 1864.7, 1975.5,
	 2093.0, 2217.5, 2349.3, 2489.0, 2637.0, 2793.8, 2960.0, 3136.0, 3322.4, 3520.0, 3729.3, 3951.1, 4186.0, 4434.9, 4698.6, 4978.0,
	 5274.0, 5587.7, 5919.9, 6271.9, 6644.9, 7040.0, 7458.6, 7902.1, 8372.0, 8869.8, 9397.3, 9956.1, 10548.1, 11175.3, 11839.8, 12543.9};

#include "test.h"

const int8_t *notes[] = {notes1, notes2, notes3, notes4, notes5, 0};
const uint16_t *duras[] = {duras1, duras2, duras3, duras4, duras5, 0};

int main()
{
	// OC1A
	DDRB |= 1 << PB1;
	// Debug-Output
	DDRC |= 1 << PC2;
	DDRC |= 1 << PC3;

	// input on PC0 and PC1 with pull-ups
	DDRC &= ~(1 << PC0);
	DDRC &= ~(1 << PC1);
	PORTC |= 1 << PC0;
	PORTC |= 1 << PC1;

	setup();
	sei();
	while (1)
	{
		uint8_t play = 0;

		if ((PINC & (1 << PC0)) == 0)
		{
			play = 1;
			while ((PINC & (1 << PC0)) == 0)
				_delay_ms(100);
			PORTC |= 1 << PC3;
		}
		if (play)
		{
			// global index
			uint16_t globalIdx = 0;
			// current index in note/duration array
			uint16_t idx[OSCILLATORS];
			// global index for which next action to trigger
			uint16_t nxt[OSCILLATORS];
			// stop flag for each channel
			uint8_t stp[OSCILLATORS];

			// init registers
			for (int8_t i=0; i<OSCILLATORS; i++)
			{
				mySid.set_register(i * CHANNEL + 5, 0x02);
				mySid.set_register(i * CHANNEL + 6, 0x80);
				idx[i] = 0;
				nxt[i] = 0;
				stp[i] = 0;
			}

			while (1)
			{
				for (int8_t i=0; i<OSCILLATORS; i++)
				{
					if (notes[i] == 0)
					{
						stp[i] = 1;
						continue;
					}
					if (globalIdx == nxt[i]-2)
					{
						// stop playing note some ticks before
						mySid.set_register(i * CHANNEL + 4, 0);   // stop
					}
					else if (globalIdx == nxt[i])
					{
						// next action to perform
						int8_t note = pgm_read_byte(&notes[i][idx[i]]);
						uint16_t dura = pgm_read_word(&duras[i][idx[i]]);
						while ((dura == 0) && (note != STOP))
						{
							idx[i]++;
							note = pgm_read_byte(&notes[i][idx[i]]);
							dura = pgm_read_word(&duras[i][idx[i]]);
						}
						if (note == PAUSE)
						{
							// pause: stop voice and remember when to take next action
							mySid.set_register(i * CHANNEL + 4, 0);   // stop
							nxt[i] += dura;
						}
						else if (note == STOP)
						{
							// stop: stop voice and remember stop flag for this channel
							mySid.set_register(i * CHANNEL + 4, 0);   // stop
							stp[i] = 1;
						}
						else
						{
							// play a note
							uint16_t pitch = 0;
							pitch = (uint16_t) (pgm_read_float(&freqs[note]) * 16.77);
							mySid.set_register(i * CHANNEL + 0, pitch & 0xff);
							mySid.set_register(i * CHANNEL + 1, pitch >> 8);
							mySid.set_register(i * CHANNEL + 4, 0x11);   // play
							nxt[i] += dura;
						}
						if (note != STOP)
							idx[i]++;
					}
				}
				// check if all channels stopped
				uint8_t stop = 1;
				for (int8_t i=0; i<OSCILLATORS; i++)
					stop &= stp[i];
				if (stop)
					break;
				if (((PINC & (1 << PC0)) == 0) || ((PINC & (1 << PC1)) == 0))
				{
					while ((PINC & (1 << PC0)) == 0)
						_delay_ms(100);
					while ((PINC & (1 << PC1)) == 0)
						_delay_ms(100);
					break;
				}
				globalIdx++;
				_delay_ms(1);
			}

			for (int8_t i=0; i<OSCILLATORS; i++)
			{
				mySid.set_register(i * CHANNEL + 4, 0);   // stop					
			}

			PORTC &= ~(1 << PC3);
			PORTB &= ~(1 << PB0);
		}
	}

	return 0;
}
