
#include <Arduino.h>
#include <stdint.h>

class SolarCharger {
public:
	// Sets up pins
	static void initialize();

	static bool isCharging();
	static bool isChargingDone();

private:
	SolarCharger() {}

	// Pin numbers
	static const byte
		XCHARGE_PIN = 9,
		XCHARGE_DONE_PIN = 8;
};
