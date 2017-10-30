
#pragma once

#include <Arduino.h>
#include <EEPROM.h>
#include <stdint.h>

class Diagnostics {
public:
	static void initialize();

	// Run time
	static void updateTimes(uint32_t sleepSeconds = 0);
	static uint32_t getRunTime();
	static uint32_t getSleepTime();

	// Lines transmitted
	static void incrementLinesTransmitted();
	static uint32_t getLinesTransmitted();

private:
	Diagnostics() {}

	// Runtime
	static uint32_t _lastRunTimeSeconds;	// The total program run time (awake and asleep)
	static uint32_t _lastSleepTimeSeconds;	// The time spent sleeping
	static uint32_t _lastUpdateMillis;

	// Lines transmitted
	static uint32_t _linesTransmitted;

	// EEPROM addresses
	static const int EEPROM_RUN_TIME = 6;	// uint32_t
	static const int EEPROM_SLEEP_TIME = 10;	// uint32_t
	static const int EEPROM_LINES_TRANSMITTED = 14;	// uint32_t
};
