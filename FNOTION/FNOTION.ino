
#include <MorseTx.h>
#include <avr/power.h>	// TODO See https://playground.arduino.cc/Learning/ArduinoSleepCode
#include <LowPower.h>
#include <SdFat.h>		// Supposedly has bug fixes that draw less power than SD.h
#include <SPI.h>
#include "Sensors.h"
#include "SolarCharger.h"

#define DEBUG

// Pin numbers
const byte SD_SS_PIN = 10;
const byte BEACON_EN_PIN = 2;
const byte IADJ_PWM_PIN = 3;
const byte STATUS_LED_PIN = 4;

#define SEARYSPEAK_URL "searyspeak.ca"

// Objects
#ifdef DEBUG
MorseTx morse(STATUS_LED_PIN);  // Use status LED to save my eyes
#else
MorseTx morse(BEACON_EN_PIN);
#endif

// SD card
SdFat sd;		// File system object
SdFile file;	// Log file

/******************************************************************************
 * Setup / Initializers
 ******************************************************************************/

void setup() {
#ifdef DEBUG
	Serial.begin(9600);
	while (!Serial);
	Serial.println("SearySpeak v1.0 - " __TIMESTAMP__);
#endif

	initializeBeacon();
	Sensors::initialize();
	SolarCharger::initialize();

	dataLoggerSetup();
}

void initializeBeacon() {
  // Analog LED current reduction
	pinMode(IADJ_PWM_PIN, OUTPUT);
	digitalWrite(IADJ_PWM_PIN, LOW);  // Full brightness

	// Set transmit timing parameters
	morse.setWordsPerMinute(8);
	morse.setShortFist(0.25); // Shorter pulses to save power
}

/******************************************************************************
 * Main Loop
 ******************************************************************************/

void loop() {
#ifdef DEBUG
	morse.write(SEARYSPEAK_URL);
#endif

	// Test program
	// Before the SearySpeak is installed at its permanent location, it will be
	// operated as a data logger to test its various systems
	dataLoggerLoop();

	// Nightfall
	// TODO Load block of names from SD card into RAM

	// TODO Transmit names interspersed with URL

	// TODO Shut down after const number of names or amount of time (~1 hour)

	// TODO Wake up every ~hour to take sensor readings
	// TODO Maybe save all sensor data in RAM and write to memory card before first block read
	// TODO Detect next nightfall by millis() and light sensor
}

/******************************************************************************
 * Test Methods
 ******************************************************************************/

#define FILE_BASE_NAME "Data"

 // TODO clean up all of this
void dataLoggerSetup() {

	const uint8_t BASE_NAME_SIZE = sizeof(FILE_BASE_NAME) - 1;
	char fileName[13] = FILE_BASE_NAME "00.csv";	// Make 8.3 name

	if (BASE_NAME_SIZE > 6) {
		error("FILE_BASE_NAME too long");
	}


	// Wait for USB Serial 
	/*Serial.begin(9600);
	while (!Serial) {
		SysCall::yield();
	}
	delay(1000);

	Serial.println(F("Type any character to start"));
	while (!Serial.available()) {
		SysCall::yield();
	}*/

	// Initialize at the highest speed supported by the board that is
	// not over 50 MHz. Try a lower speed if SPI errors occur.
	if (!sd.begin(SD_SS_PIN, SD_SCK_MHZ(50))) {
		sd.initErrorHalt();	// TODO See if there are other sd.init() options
	}

	// Find an unused file name.
	while (sd.exists(fileName)) {
		if (fileName[BASE_NAME_SIZE + 1] != '9') {
			fileName[BASE_NAME_SIZE + 1]++;	// Increment 1's place
		} else if (fileName[BASE_NAME_SIZE] != '9') {
			fileName[BASE_NAME_SIZE + 1] = '0';	// Carry 1's place
			fileName[BASE_NAME_SIZE]++;		// Increment 10's place
		} else {
			//error("Can't create file name");
		}
	}
	if (!file.open(fileName, O_CREAT | O_WRITE | O_EXCL)) {
		error("file.open");
	}
	// Read any Serial data.
	do {
		delay(10);
	} while (Serial.available() && Serial.read() >= 0);

	Serial.print(F("Logging to: "));
	Serial.println(fileName);
	Serial.println(F("Type any character to stop"));

	// Write data header.
	





	file.print(F("micros"));
	for (uint8_t i = 0; i < ANALOG_COUNT; i++) {
		file.print(F(",adc"));
		file.print(i, DEC);
	}
	file.println();





	// Start on a multiple of the sample interval.
	logTime = micros() / (1000UL * SAMPLE_INTERVAL_MS) + 1;
	logTime *= 1000UL * SAMPLE_INTERVAL_MS;











}

void dataLoggerLoop() {
	// TODO Write these readings to uSD card (copy from dataLogger example)
#ifdef DEBUG
	Serial.print("Ambient light: ");
	Serial.println(Sensors::getAmbientLight());
	Serial.print("Temperature: ");
	Serial.println(Sensors::getTemperature());
	Serial.print("Battery voltage: ");
	Serial.println(Sensors::getLoadVoltage());

	Serial.print("Charging: ");
	Serial.println(SolarCharger::isCharging());
	Serial.print("Charging done: ");
	Serial.println(SolarCharger::isChargingDone());
#endif





	// TODO bring in function contents
	logData();

	// Force data to SD and update the directory entry to avoid data loss.
	// file.sync() is like close and open, but more efficient
	// TODO How to make robust error recovery?
	if (!file.sync() || file.getWriteError()) {
		//error("write error");
		digitalWrite(STATUS_LED_PIN, HIGH);
	}




	// Enter power down state for 8 s with ADC and BOD module disabled
	LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
}

/******************************************************************************
* Utility Methods
******************************************************************************/

// TODO Define error flags in EEPROM
// TODO Make diagnostic serial command interface
// TODO Make uptime counter and other metering
