
#include "MorseTx.h"

void MorseTx::setWordsPerMinute(byte wordsPerMinute) {
	// Ignore invalid arguments
	if (wordsPerMinute == 0) {
		return;
	}

	// Set the value and calculate delay
	_wordsPerMinute = wordsPerMinute;
	setShortFist(_shortFistUnit);
}

void MorseTx::write(char *str) {
	// Send each character until null is encountered
	char c;
	for (uint16_t i = 0; (c = str[i]) != '\0'; i++) {
		write(c);
	}
}

void MorseTx::write(char c) {
	sendMorse(getMorse(c));
}

void MorseTx::sendMorse(byte morse) {
	// Get the length of the character
	byte length = min(morse >> LENGTH_SHIFT, 6);	// Maximum character length is 6

	if (length > 0) {
		// Sendable character
		for (byte i = 0; i < length; i++) {
			// Send each symbol (dot or dash)
			bool dashNotDot = morse & (1 << i);
			//Serial.print(dashNotDot ? '-' : '.');	// DEBUG

			setTx(true);
			delayUnits(dashNotDot ? UNIT_DASH : UNIT_DOT, true);
			
			setTx(false);
			delay(_shortFistMillis);
			delayUnits(UNIT_SPACE_SYMBOL);
		}
		delayUnits(UNIT_SPACE_LETTER);	// Letter space
	} else if (morse == MORSE_SPACE) {
		// Space character
		delayUnits(UNIT_SPACE_WORD);	// Word space
	} // else unknown character
}

byte MorseTx::getMorse(char c) {
	// White space
	// TODO Maybe make codes for longer pauses for '\t' and '\n'
	if (c == ' ' || c == '\t' || c == '\n') {
		return MORSE_SPACE;
	}

	// Letters
	if (c >= 'a' && c <= 'z') {
		return MORSE_LETTERS[c - 'a'];
	}
	if (c >= 'A' && c <= 'Z') {
		return MORSE_LETTERS[c - 'A'];
	}

	// Digits
	if (c >= '0' && c <= '9') {
		return MORSE_DIGITS[c - '0'];
	}

	// Punctuation / special characters
	if (c == '!') return MORSE_PUNCTUATION[0];
	if (c == '\"') return MORSE_PUNCTUATION[1];
	if (c == '\'') return MORSE_PUNCTUATION[2];
	if (c == '(') return MORSE_PUNCTUATION[3];
	if (c == ')') return MORSE_PUNCTUATION[4];
	if (c == '+') return MORSE_PUNCTUATION[5];
	if (c == ',') return MORSE_PUNCTUATION[6];
	if (c == '-') return MORSE_PUNCTUATION[7];
	if (c == '.') return MORSE_PUNCTUATION[8];
	if (c == '/') return MORSE_PUNCTUATION[9];
	if (c == ':') return MORSE_PUNCTUATION[10];
	if (c == ';') return MORSE_PUNCTUATION[11];
	if (c == '=') return MORSE_PUNCTUATION[12];
	if (c == '?') return MORSE_PUNCTUATION[13];
	if (c == '@') return MORSE_PUNCTUATION[14];
	if (c == '_') return MORSE_PUNCTUATION[15];

	// Other (unsupported character)
	return MORSE_UNKNOWN;
}
