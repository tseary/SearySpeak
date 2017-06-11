
#include "SolarCharger.h"

void SolarCharger::initialize() {
	// These pins are pulled up by LED and resistor on solar charger
	pinMode(XCHARGE_PIN, INPUT_PULLUP);
	pinMode(XCHARGE_DONE_PIN, INPUT_PULLUP);
}

bool SolarCharger::isCharging() {
	return !digitalRead(XCHARGE_PIN);
}

bool SolarCharger::isChargingDone() {
	return !digitalRead(XCHARGE_DONE_PIN);
}
