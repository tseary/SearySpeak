
#include "Diagnostics.h"

uint32_t Diagnostics::_lastRuntimeSeconds;
uint32_t Diagnostics::_lastUpdateMillis;

void Diagnostics::initialize() {
	EEPROM.get(EEPROM_RUNTIME, _lastRuntimeSeconds);
	_lastUpdateMillis = 0;
}

void Diagnostics::updateRuntime(uint32_t extraSeconds) {
	// Calculate elapsed time
	uint32_t millisElapsed = millis() - _lastUpdateMillis;
	uint32_t secondsElapsed = millisElapsed / 1000;

	// Add elapsed to recorded time
	_lastRuntimeSeconds += extraSeconds + secondsElapsed;
	EEPROM.put(EEPROM_RUNTIME, _lastRuntimeSeconds);

	// _lastUpdateMillis is always incremented by a multiple of 1000, so that
	// truncation errors from division do not accumulate
	// Note that _lastUpdateMillis will be valid but will not be a multiple
	// of 1000 after rollover
	_lastUpdateMillis += secondsElapsed * 1000;
}

uint32_t Diagnostics::getRuntime() {
	return _lastRuntimeSeconds;
}
