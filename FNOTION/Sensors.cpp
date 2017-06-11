
#include "Sensors.h"

const float Sensors::ANALOG_READ_FULL_SCALE = 1024.0;
const float Sensors::REFERENCE_VOLTAGE = 2.500;

void Sensors::initialize() {
	// Ambient light
	pinMode(SENSOR_LIGHT_EN_PIN, OUTPUT);
	digitalWrite(SENSOR_LIGHT_EN_PIN, LOW);
	pinMode(SENSOR_LIGHT_ADC_PIN, INPUT);

	// Temperature
	pinMode(SENSOR_TEMPERATURE_EN_PIN, OUTPUT);
	digitalWrite(SENSOR_TEMPERATURE_EN_PIN, LOW);
	pinMode(SENSOR_TEMPERATURE_ADC_PIN, INPUT);

	// Voltage reference
	pinMode(SENSOR_VOLTAGE_EN_PIN, OUTPUT);
	digitalWrite(SENSOR_VOLTAGE_EN_PIN, LOW);
	pinMode(SENSOR_VOLTAGE_ADC_PIN, INPUT);
}

// Reads the light sensor (unitless)
// Light readings should only be taken when the beacon and status light are off
uint16_t Sensors::getAmbientLight() {
	return readSensor(SENSOR_LIGHT_EN_PIN, SENSOR_LIGHT_ADC_PIN);
}

// Reads the temperature sensor
// TODO Convert to Celsius
float Sensors::getTemperature() {
	return readSensor(SENSOR_TEMPERATURE_EN_PIN, SENSOR_TEMPERATURE_ADC_PIN);
}

// Reads the voltage sensor and calculates the battery (AREF) voltage
float Sensors::getLoadVoltage() {
	uint16_t voltageAnalog = readSensor(SENSOR_VOLTAGE_EN_PIN, SENSOR_VOLTAGE_ADC_PIN);
	return REFERENCE_VOLTAGE * ANALOG_READ_FULL_SCALE / voltageAnalog;
}

uint16_t Sensors::readSensor(byte enablePin, byte analogPin) {
	digitalWrite(enablePin, HIGH);
	delay(SENSOR_SETTLING_MILLIS);
	uint16_t rawValue = analogRead(analogPin);
	digitalWrite(enablePin, LOW);
	return rawValue;
}
