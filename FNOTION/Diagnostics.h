
#pragma once

#include <Arduino.h>
#include <EEPROM.h>
#include <stdint.h>

class Diagnostics {
public:
	// TODO make function to count total uptime and save in EEPROM

	static void initialize();

	static void updateRuntime(uint32_t extraSeconds = 0);
	static uint32_t getRuntime();

private:
	Diagnostics() {}

	static uint32_t _lastRuntimeSeconds;
	static uint32_t _lastUpdateMillis;

	static const uint16_t EEPROM_RUNTIME_DEBUG = 6;	// uint32_t
	static const uint16_t EEPROM_RUNTIME_RELEASE = 10;	// uint32_t
#ifdef _DEBUG
	static const uint16_t EEPROM_RUNTIME = EEPROM_RUNTIME_DEBUG;	// uint32_t
#else
	static const uint16_t EEPROM_RUNTIME = EEPROM_RUNTIME_RELEASE;	// uint32_t
#endif
};
