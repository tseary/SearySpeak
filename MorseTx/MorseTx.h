
#ifndef MorseTx_h
#define MorseTx_h

#include "Arduino.h"
#include <avr/pgmspace.h>

class MorseTx {
public:
	MorseTx(byte txPin, bool activeState = true) {
		// Set transmit pin number
		_txPin = txPin;
		_activeState = activeState;

		// Initialize pin
		pinMode(_txPin, OUTPUT);
		setTx(false);
	}

	// Transmits a single character
	void write(char c);

	// Transmits a null-terminated string
	void write(const char* str);

	// Sets the transmission speed (8 is slow, 20 is fast)
	void setWordsPerMinute(byte wordsPerMinute);

	// Shortens each pulse by part of a unit to reduce transmission power
	void setShortFist(float offUnit) {
		// Ignore invalid arguments
		if (offUnit < 0.0 || offUnit > 1.0) {
			return;
		}

		// Set the unit and calculate the delay
		_shortFistUnit = offUnit;
		_shortFistMillis = round(offUnit * getUnitMillis());
	}

private:
	// Output pin
	byte _txPin;
	bool _activeState;

	// Sends a Morse character in the form 0bLLLDDDDD
	void sendMorse(byte morse);

	// Gets a byte representing a Morse character in the form 0bLLLDDDDD
	// LLL = code length
	// If LLL = 11X, length is 6 and X is a dot or dash
	// DDDDD = dots (0) and dashes (1)
	// LSB is transmitted first
	byte getMorse(char c);

	void setTx(bool active) {
		digitalWrite(_txPin, active == _activeState);
	}

	// Timing units
	const byte UNIT_DOT = 1;
	const byte UNIT_DASH = 3;
	const byte UNIT_SPACE_SYMBOL = 1;
	const byte UNIT_SPACE_LETTER = 3 - UNIT_SPACE_SYMBOL;
	const byte UNIT_SPACE_WORD = 7 - UNIT_SPACE_SYMBOL;	// Word-space is assumed to follow a non-word-space

	byte _wordsPerMinute = 8;	// 8 is slow, 20 is fast

	void delayUnits(byte units, bool shortFist = false) {
		delay(units * getUnitMillis() - (shortFist ? _shortFistMillis : 0));
	}

	uint16_t getUnitMillis() {
		return 1200 / _wordsPerMinute;	// Based on 50-dot standard word PARIS
	}

	// Power-saving compromise
	float _shortFistUnit = 0.0;
	uint16_t _shortFistMillis = 0;

	// The position of the length information 
	const byte LENGTH_SHIFT = 5;

	// Zero-length codes
	const byte MORSE_SPACE = 0b00000;	// Space character
	const byte MORSE_UNKNOWN = 0b00001;	// Code for unsupported characters

										// Letters A - Z
	const PROGMEM byte MORSE_LETTERS[26] = {
		(2 << LENGTH_SHIFT) | 0b00010, // A
		(4 << LENGTH_SHIFT) | 0b00001, // B
		(4 << LENGTH_SHIFT) | 0b00101, // C
		(3 << LENGTH_SHIFT) | 0b00001, // D
		(1 << LENGTH_SHIFT) | 0b00000, // E
		(4 << LENGTH_SHIFT) | 0b00100, // F
		(3 << LENGTH_SHIFT) | 0b00011, // G
		(4 << LENGTH_SHIFT) | 0b00000, // H
		(2 << LENGTH_SHIFT) | 0b00000, // I
		(4 << LENGTH_SHIFT) | 0b01110, // J
		(3 << LENGTH_SHIFT) | 0b00101, // K
		(4 << LENGTH_SHIFT) | 0b00010, // L
		(2 << LENGTH_SHIFT) | 0b00011, // M
		(2 << LENGTH_SHIFT) | 0b00001, // N
		(3 << LENGTH_SHIFT) | 0b00111, // O
		(4 << LENGTH_SHIFT) | 0b00110, // P
		(4 << LENGTH_SHIFT) | 0b01011, // Q
		(3 << LENGTH_SHIFT) | 0b00010, // R
		(3 << LENGTH_SHIFT) | 0b00000, // S
		(1 << LENGTH_SHIFT) | 0b00001, // T
		(3 << LENGTH_SHIFT) | 0b00100, // U
		(4 << LENGTH_SHIFT) | 0b01000, // V
		(3 << LENGTH_SHIFT) | 0b00110, // W
		(4 << LENGTH_SHIFT) | 0b01001, // X
		(4 << LENGTH_SHIFT) | 0b01101, // Y
		(4 << LENGTH_SHIFT) | 0b00011  // Z
	};

	// Digits 0 - 9
	const PROGMEM byte MORSE_DIGITS[10] = {
		(5 << LENGTH_SHIFT) | 0b11111, // 0
		(5 << LENGTH_SHIFT) | 0b11110, // 1
		(5 << LENGTH_SHIFT) | 0b11100, // 2
		(5 << LENGTH_SHIFT) | 0b11000, // 3
		(5 << LENGTH_SHIFT) | 0b10000, // 4
		(5 << LENGTH_SHIFT) | 0b00000, // 5
		(5 << LENGTH_SHIFT) | 0b00001, // 6
		(5 << LENGTH_SHIFT) | 0b00011, // 7
		(5 << LENGTH_SHIFT) | 0b00111, // 8
		(5 << LENGTH_SHIFT) | 0b01111  // 9
	};

	const PROGMEM byte MORSE_PUNCTUATION[16] = {
		(6 << LENGTH_SHIFT) | 0b110101, // !
		(6 << LENGTH_SHIFT) | 0b010010, // "
		(6 << LENGTH_SHIFT) | 0b011110, // '
		(6 << LENGTH_SHIFT) | 0b101101, // (
		(6 << LENGTH_SHIFT) | 0b101101, // )
		(5 << LENGTH_SHIFT) | 0b01010,  // +
		(6 << LENGTH_SHIFT) | 0b110011, // ,
		(6 << LENGTH_SHIFT) | 0b100001, // -
		(6 << LENGTH_SHIFT) | 0b101010, // .
		(5 << LENGTH_SHIFT) | 0b01001,  // /
		(6 << LENGTH_SHIFT) | 0b000111, // :
		(6 << LENGTH_SHIFT) | 0b010101, // ;
		(5 << LENGTH_SHIFT) | 0b10001,  // =
		(6 << LENGTH_SHIFT) | 0b001100, // ?
		(6 << LENGTH_SHIFT) | 0b010110, // @
		(6 << LENGTH_SHIFT) | 0b101100  // _
	};
};

#endif
