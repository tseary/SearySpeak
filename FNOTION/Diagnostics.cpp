
#include "Diagnostics.h"

uint32_t Diagnostics::_lastRunTimeSeconds;
uint32_t Diagnostics::_lastSleepTimeSeconds;
uint32_t Diagnostics::_lastUpdateMillis;

uint32_t Diagnostics::_linesTransmitted;

void Diagnostics::initialize() {
	// Runtime
	EEPROM.get(EEPROM_RUN_TIME, _lastRunTimeSeconds);
	EEPROM.get(EEPROM_SLEEP_TIME, _lastSleepTimeSeconds);
	_lastUpdateMillis = 0;

	// Lines transmitted
	EEPROM.get(EEPROM_LINES_TRANSMITTED, _linesTransmitted);
}

void Diagnostics::updateTimes(uint32_t sleepSeconds) {
	// Calculate elapsed time
	uint32_t millisElapsed = millis() - _lastUpdateMillis;
	uint32_t secondsElapsed = millisElapsed / 1000;

	// Add elapsed to recorded time
	_lastRunTimeSeconds += sleepSeconds + secondsElapsed;
	_lastSleepTimeSeconds += sleepSeconds;
	EEPROM.put(EEPROM_RUN_TIME, _lastRunTimeSeconds);
	EEPROM.put(EEPROM_SLEEP_TIME, _lastSleepTimeSeconds);

	// _lastUpdateMillis is always incremented by a multiple of 1000, so that
	// truncation errors from division do not accumulate.
	// Note that _lastUpdateMillis will be valid but will not be a multiple
	// of 1000 after rollover.
	_lastUpdateMillis += secondsElapsed * 1000;
}

uint32_t Diagnostics::getRunTime() {
	return _lastRunTimeSeconds;
}

uint32_t Diagnostics::getSleepTime() {
	return _lastSleepTimeSeconds;
}

void Diagnostics::incrementLinesTransmitted() {
	_linesTransmitted++;
	EEPROM.put(EEPROM_LINES_TRANSMITTED, _linesTransmitted);
}

uint32_t Diagnostics::getLinesTransmitted() {
	return _linesTransmitted;
}
