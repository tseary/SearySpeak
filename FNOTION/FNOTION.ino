
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
SdFat sdFat;	// File system object
const char* MORSE_FILE_NAME = "Transmit.txt";
File morseFile;	// Morse code file	TODO Figure out how to use SdFile for both
SdFile logFile;	// Log file

/******************************************************************************
 * Setup / Initializers
 ******************************************************************************/

void setup() {

#ifdef _DEBUG
	Serial.begin(9600);
	while (!Serial.available());
	Serial.println("SearySpeak v1.0 - " __TIMESTAMP__);
#endif

	initializeBeacon();

	Diagnostics::initialize();
	Sensors::initialize();
	SolarCharger::initialize();

	initializeSD();

	// Open morse code file
	// TODO If morse file doesn't exist, create it with readme comments
	morseFile = sdFat.open(MORSE_FILE_NAME, O_READ | O_EXCL);
	if (!morseFile.open(MORSE_FILE_NAME, O_READ/* | O_EXCL*/)) {
		//error("file.open");
		Serial.print("Error opening morse file: ");
		Serial.println(MORSE_FILE_NAME);
	}

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

void initializeSD() {
	// Initialize at the highest speed supported by the board that is
	// not over 50 MHz. Try a lower speed if SPI errors occur.
	if (!sdFat.begin(SD_SS_PIN, SD_SCK_MHZ(50))) {
		sdFat.initErrorHalt();
	}
}

/******************************************************************************
 * Main Loop
 ******************************************************************************/

void loop() {
/*#ifdef _DEBUG
	morse.write(SEARYSPEAK_URL);
#endif*/

	// Test program
	// Before the SearySpeak is installed at its permanent location, it will be
	// operated as a data logger to test its various systems
	dataLoggerLoop();

	Diagnostics::updateRuntime();

	/*Serial.print("Runtime:\t");
	Serial.println(Diagnostics::getRuntime());
	Serial.print("millis:\t");
	Serial.println(millis());
	Serial.println();*/

	// Nightfall
	// TODO Load one line from SD card into RAM
	const uint16_t LINE_LENGTH = 100;
	static char transmitLine[LINE_LENGTH];
	int readReturn = csvReadText(&morseFile, transmitLine, LINE_LENGTH, '\n');
	Serial.print(readReturn);
	Serial.print('\t');
	if (readReturn >= 0) {
		Serial.println(transmitLine);
	}

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
	// Create file name
	const uint8_t BASE_NAME_SIZE = sizeof(FILE_BASE_NAME) - 1;
	char fileName[BASE_NAME_SIZE + 7] = FILE_BASE_NAME "00.csv";	// Make 8.3 name

	if (BASE_NAME_SIZE > 6) {
		//error("FILE_BASE_NAME too long");
	}

	// Find an unused file name.
	while (sdFat.exists(fileName)) {
		if (fileName[BASE_NAME_SIZE + 1] != '9') {
			fileName[BASE_NAME_SIZE + 1]++;	// Increment 1's place
		} else if (fileName[BASE_NAME_SIZE] != '9') {
			fileName[BASE_NAME_SIZE + 1] = '0';	// Carry 1's place
			fileName[BASE_NAME_SIZE]++;		// Increment 10's place
		} else {
			//error("Can't create file name");
		}
	}

	if (!logFile.open(fileName, O_CREAT | O_WRITE | O_EXCL)) {
		//error("file.open");
	}

	// Write data header
	logFile.println(F("runtime,light,temperature,voltage,charging,done"));
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
	logFile.print(Diagnostics::getRuntime());

	// Write sensor readings
	logFile.write(CSV_DELIMITER);
	logFile.print(ambientLight);
	logFile.write(CSV_DELIMITER);
	logFile.print(temperature);
	logFile.write(CSV_DELIMITER);
	logFile.print(loadVoltage, VOLTAGE_DIGITS);

	// Write charging information
	logFile.write(CSV_DELIMITER);
	logFile.print(charging);
	logFile.write(CSV_DELIMITER);
	logFile.print(chargingDone);

	// Write newline
	logFile.println();

	// Force data to SD and update the directory entry to avoid data loss
	// logFile.sync() is like close and open, but more efficient
	// TODO How to make robust error recovery?
	if (!logFile.sync() || logFile.getWriteError()) {
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

// Read one entry from a csv file.
// str is the output string buffer.
// maxSize is the size of str including terminating null.
// delim is typically ',' (default) but can be any char.
// Returns:
// 0 if end of file
// -1 if read error
// -2 if overflow (entry too long)
// delimiter if successful (delim or '\n')
int csvReadText(File* file, char* str, uint16_t maxSize, char delim /*= ','*/) {
	char c;
	int rtn;
	uint16_t n = 0;
	while (true) {
		// Check for end of file
		if (!file->available()) {
			rtn = 0;
			break;
		}

		// Check string size (leave space for null)
		if (n >= maxSize) {
			rtn = -2;
			break;
		}

		// Read next char
		if (file->read(&c, 1) != 1) {
			rtn = -1;	// Read error
			break;
		}

		// Delete CR
		if (c == '\r') {
			continue;
		}

		// Check for delimiter
		if (c == delim || c == '\n') {
			rtn = c;
			break;
		}

		// Append char to string
		str[n++] = c;
	}
	str[n] = '\0';	// Terminate string
	return rtn;
}
