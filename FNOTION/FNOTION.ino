
#include <MorseTx.h>
//#include <avr/power.h>	// TODO See https://playground.arduino.cc/Learning/ArduinoSleepCode
#include <LowPower.h>
#include <SdFat.h>		// Supposedly has bug fixes that draw less power than SD.h
#include <SPI.h>
#include "Diagnostics.h"
#include "Sensors.h"
#include "SolarCharger.h"

// Pin numbers
const byte SD_SS_PIN = 10;
const byte BEACON_EN_PIN = 2;
const byte IADJ_PWM_PIN = 3;
const byte STATUS_LED_PIN = 4;

#define SEARYSPEAK_URL "searyspeak.ca"

// Objects
#ifdef _DEBUG
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

#ifdef _DEBUG
	Serial.begin(9600);
	while (!Serial);
	Serial.println("SearySpeak v1.0 - " __TIMESTAMP__);
#endif
	
	initializeBeacon();

	Diagnostics::initialize();
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
/*#ifdef DEBUG
	morse.write(SEARYSPEAK_URL);
#endif*/

	// Test program
	// Before the SearySpeak is installed at its permanent location, it will be
	// operated as a data logger to test its various systems
	dataLoggerLoop();

	Diagnostics::updateRuntime();
	Serial.print("Runtime:\t");
	Serial.println(Diagnostics::getRuntime());
	Serial.print("millis:\t");
	Serial.println(millis());
	Serial.println();

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
	char fileName[BASE_NAME_SIZE + 7] = FILE_BASE_NAME "00.csv";	// Make 8.3 name

	if (BASE_NAME_SIZE > 6) {
		//error("FILE_BASE_NAME too long");
	}

	// Initialize at the highest speed supported by the board that is
	// not over 50 MHz. Try a lower speed if SPI errors occur.
	if (!sd.begin(SD_SS_PIN, SD_SCK_MHZ(50))) {
		sd.initErrorHalt();
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
		//error("file.open");
	}

	// Write data header
	file.println(F("runtime,light,temperature,voltage,charging,done"));
}

void dataLoggerLoop() {
	// Read all sensors first to avoid SD write latency between readings
	uint16_t ambientLight = Sensors::getAmbientLight();	// Sensor reads take > 1 ms
	float temperature = Sensors::getTemperature(),
		loadVoltage = Sensors::getLoadVoltage();
	bool charging = SolarCharger::isCharging(),
		chargingDone = SolarCharger::isChargingDone();

	// Data file delimiter
	const char CSV_DELIMITER = ',';
	const int VOLTAGE_DIGITS = 3;

	// TODO Write these readings to uSD card (copy from dataLogger example)
/*#ifdef _DEBUG
	Serial.print("Ambient light: ");
	Serial.println(ambientLight);
	Serial.print("Temperature: ");
	Serial.println(temperature);
	Serial.print("Battery voltage: ");
	Serial.println(loadVoltage, VOLTAGE_DIGITS);

	Serial.print("Charging: ");
	Serial.println(charging);
	Serial.print("Charging done: ");
	Serial.println(chargingDone);
#endif*/

	// Write time stamp
	file.print(Diagnostics::getRuntime());

	// Write sensor readings
	file.write(CSV_DELIMITER);
	file.print(ambientLight);
	file.write(CSV_DELIMITER);
	file.print(temperature);
	file.write(CSV_DELIMITER);
	file.print(loadVoltage, VOLTAGE_DIGITS);

	// Write charging information
	file.write(CSV_DELIMITER);
	file.print(charging);
	file.write(CSV_DELIMITER);
	file.print(chargingDone);

	// Write newline
	file.println();

	// Force data to SD and update the directory entry to avoid data loss
	// file.sync() is like close and open, but more efficient
	// TODO How to make robust error recovery?
	if (!file.sync() || file.getWriteError()) {
		//error("write error");
		setStatusLight(true);
		delay(1000);
		setStatusLight(false);
	}

	// Blip (heartbeat)
	setStatusLight(true);
	delay(1);
	setStatusLight(false);

	// Enter power down state for 8 s with ADC and BOD module disabled
	Serial.flush();
	LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);	// This also stops the millis() timer
	Diagnostics::updateRuntime(8);
	//LowPower.idle(SLEEP_8S, ADC_OFF,
	//	TIMER2_ON, TIMER1_ON, TIMER0_ON,
	//	SPI_OFF, USART0_ON, TWI_OFF);
}

/******************************************************************************
* Utility Methods
******************************************************************************/

// TODO Define error flags in EEPROM
// TODO Make diagnostic serial command interface

void setStatusLight(bool enable) {
	digitalWrite(STATUS_LED_PIN, enable);
}
