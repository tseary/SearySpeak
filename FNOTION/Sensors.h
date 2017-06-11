
#include <Arduino.h>
#include <stdint.h>

class Sensors {
public:
	// Sets up pins
	static void initialize();

	// Reads the light sensor (unitless)
	// Light readings should only be taken when the beacon and status light are off
	static uint16_t getAmbientLight();

	// Reads the temperature sensor
	static float getTemperature();

	// Reads the voltage sensor and calculates the battery (AREF) voltage
	static float getLoadVoltage();

private:
	Sensors() {}

	// The voltage sensor is connected to a 2.5 V reference, therefore the
	// analog reading is inversely proportionate to the battery voltage at AREF
	static const float ANALOG_READ_FULL_SCALE;
	static const float REFERENCE_VOLTAGE;

	// Delay between enabling a sensor and taking a reading
	static const uint16_t SENSOR_SETTLING_MILLIS = 1;

	// Pin numbers
	static const byte
		SENSOR_LIGHT_EN_PIN = A4,
		SENSOR_LIGHT_ADC_PIN = A2,
		SENSOR_TEMPERATURE_EN_PIN = A3,
		SENSOR_TEMPERATURE_ADC_PIN = A1,
		SENSOR_VOLTAGE_EN_PIN = A5,
		SENSOR_VOLTAGE_ADC_PIN = A0;

	// Enables a sensor, takes a reading, then disables the sensor
	static uint16_t readSensor(byte enablePin, byte analogPin);
};
