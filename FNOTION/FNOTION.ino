
#include <MorseTx.h>
//#include <avr/power.h>	// TODO See https://playground.arduino.cc/Learning/ArduinoSleepCode
#include <LowPower.h>
#include <SdFat.h>		// Supposedly has bug fixes that draw less power than SD.h
#include <SPI.h>
#include "Diagnostics.h"
#include "Sensors.h"
#include "SolarCharger.h"

// Comment/uncomment to change program behaviour
//#define REDUCE_BRIGHTNESS;
//#define TRANSMIT_ON_STATUS_LED;

// Pin numbers
const byte SD_SS_PIN = 10;
const byte BEACON_EN_PIN = 2;
const byte IADJ_PWM_PIN = 3;
const byte STATUS_LED_PIN = 4;

const char* SEARYSPEAK_URL = "searyspeak.ca";
const char* FIRMWARE_VERSION = "SearySpeak v1.0 - " __TIMESTAMP__;

// Objects
#ifdef TRANSMIT_ON_STATUS_LED
MorseTx morse(STATUS_LED_PIN);  // Use status LED to save my eyes
#else
MorseTx morse(BEACON_EN_PIN, false);
#endif

// SD card
SdFat sdFat;	// File system object
const char* MORSE_FILE_NAME = "Transmit.txt";
File morseFile;	// Morse code file	TODO Figure out how to use SdFile for both
SdFile logFile;	// Log file

// Buffer for transmit string
const uint16_t LINE_BUFFER_LENGTH = 100;
char transmitLine[LINE_BUFFER_LENGTH];

const uint32_t BAUD_RATE = 250000;

/******************************************************************************
 * Setup / Initializers
 ******************************************************************************/

void setup() {
	Serial.begin(BAUD_RATE);
	Serial.println(FIRMWARE_VERSION);

	pinMode(STATUS_LED_PIN, OUTPUT);
	setStatusLight(true);
	delay(1000);
	setStatusLight(false);

	initializeBeacon();
	initializeSD();

	Diagnostics::initialize();
	Sensors::initialize();
	SolarCharger::initialize();

	// Open morse code file
	// TODO If morse file doesn't exist, create it with readme comments
	morseFile = sdFat.open(MORSE_FILE_NAME, O_READ | O_EXCL);
	if (!morseFile.open(MORSE_FILE_NAME, O_READ/* | O_EXCL*/)) {
		//error("file.open");
		Serial.print("Error opening morse file: ");
		Serial.println(MORSE_FILE_NAME);
		errorFlash(8);
	}

	dataLoggerSetup();
}

void initializeBeacon() {
  // Analog LED current reduction
#ifdef REDUCE_BRIGHTNESS
	analogWrite(IADJ_PWM_PIN, 168);		// Partial brightness
#else
	pinMode(IADJ_PWM_PIN, OUTPUT);
	digitalWrite(IADJ_PWM_PIN, LOW);	// Full brightness
#endif

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

	Diagnostics::updateTimes();

	/*Serial.print("Runtime:\t");
	Serial.println(Diagnostics::getRuntime());
	Serial.print("millis:\t");
	Serial.println(millis());
	Serial.println();*/

	// Nightfall
	// Load one line from SD card into RAM
	// TODO Ignore blank and comment lines
	int readReturn = csvReadText(&morseFile, transmitLine, LINE_BUFFER_LENGTH, '\n');
#ifdef _DEBUG
	Serial.print(readReturn);
	Serial.print('\t');
	if (readReturn >= 0) {
		Serial.println(transmitLine);
	}
#endif

	// Transmit the line
	static uint8_t lineCounter = 0;	// The number of lines sent since the URL was transmitted
	if (readReturn >= 0) {
		// The line was read successfully

		// Transmit the line
		// e.g. "Murphy,St. John's,1600\0"
		for (byte i = 0; i < LINE_BUFFER_LENGTH; i++) {
			if (transmitLine[i] == '\0') {
				break;
			}
			if (transmitLine[i] == ',') {
				transmitLine[i] = '\t';
			}
		}
		morse.write(transmitLine);
		morse.write('\n');	// Delay between lines
		Diagnostics::incrementLinesTransmitted();
		lineCounter++;

		// Rewind if the end of the file was reached
		if (readReturn == '\0') {
			morseFile.rewind();
		}
	} else if (readReturn == -1) {
		// Read error
	} else {
		// Entry too long
		// TODO Allocate a longer char*, attempt to append the rest of the line
	}

	// Transmit URL every few lines
	const byte LINES_PER_URL = 3;
	if (lineCounter >= LINES_PER_URL) {
		morse.write(SEARYSPEAK_URL);
		morse.write('\n');	// Delay between lines
		lineCounter = 0;
	}

	// TODO Shut down after const number of names or amount of time (~1 hour)

	// TODO Wake up every ~hour to take sensor readings
	// TODO Maybe save all sensor data in RAM and write to memory card before first block read
	// TODO Detect next nightfall by millis() and light sensor
}

// TODO Make diagnostic serial command interface
void serialEvent() {
	while (Serial.available()) {
		char commandChar = Serial.read();
		Serial.println(commandChar);
		switch (commandChar) {
		case 'D':	// Diagnostics
			printDiagnostics();
			break;
		case 'M':	// Mount SD card
			// TODO Start the SD card
			break;
		case 'U':	// Unmount SD card
			// TODO Stop the SD card
			break;
		case 'S':	// Deep sleep
			powerDownUntilCharge();
			break;
		}
	}
}

/******************************************************************************
 * Test Methods
 ******************************************************************************/

#define FILE_NAME_BASE "Data"
#define FILE_NAME_DEFAULT FILE_NAME_BASE "0000.csv"

 // TODO clean up all of this
void dataLoggerSetup() {
	// Create file name
	char fileName[] = FILE_NAME_DEFAULT;

	// Find an unused file name
	const uint8_t BASE_NAME_SIZE = sizeof(FILE_NAME_BASE) - 1;
	const uint8_t TENS_PLACE = BASE_NAME_SIZE + 2;
	const uint8_t ONES_PLACE = BASE_NAME_SIZE + 3;
	while (sdFat.exists(fileName)) {
		if (fileName[ONES_PLACE] != '9') {
			fileName[ONES_PLACE]++;	// Increment 1's place
		} else if (fileName[BASE_NAME_SIZE] != '9') {
			fileName[ONES_PLACE] = '0';	// Carry 1's place
			fileName[TENS_PLACE]++;		// Increment 10's place
		} else {
			Serial.println("Can't create numbered file name");
			fileName[TENS_PLACE] = 'X';	// Clear 10's place
			fileName[ONES_PLACE] = 'X';	// Clear 1's place
			break;
		}
	}

	Serial.print("Logging to file: ");
	Serial.println(fileName);

	if (!logFile.open(fileName, O_CREAT | O_WRITE | O_EXCL)) {
		Serial.println("file.open failed");
		return;
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
#ifdef _DEBUG
	/*Serial.print("Ambient light: ");
	Serial.println(ambientLight);
	Serial.print("Temperature: ");
	Serial.println(temperature);*/
	Serial.print("Battery voltage: ");
	Serial.println(loadVoltage, VOLTAGE_DIGITS);

	/*Serial.print("Charging: ");
	Serial.println(charging);
	Serial.print("Charging done: ");
	Serial.println(chargingDone);*/
#endif

	// Write time stamp
	logFile.print(Diagnostics::getRunTime());

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

	// Enter power down state for 8 seconds with ADC and Brown-Out Detect module disabled
	Serial.flush();
	powerDownFor8Seconds();
}

void printDiagnostics() {
	// Print formatted run time
	Serial.print("Run time (h:mm:ss): ");
	printTime(Diagnostics::getRunTime());
	Serial.println();

	// Print formatted sleep time
	Serial.print("Sleep time (h:mm:ss): ");
	printTime(Diagnostics::getSleepTime());
	Serial.println();

	// Print lines transmitted
	Serial.print("Lines transmitted: ");
	Serial.println(Diagnostics::getLinesTransmitted());
}

void printTime(uint32_t timeInSeconds) {
	// Split runtime into components
	uint8_t seconds = timeInSeconds % 60;
	uint8_t minutes = (timeInSeconds / 60) % 60;
	uint8_t hours = timeInSeconds / 3600;

	// Print formatted runtime
	Serial.print(hours);
	Serial.print(':');
	if (minutes <= 9) {
		Serial.print('0');
	}
	Serial.print(minutes);
	Serial.print(':');
	if (seconds <= 9) {
		Serial.print('0');
	}
	Serial.print(seconds);
}


/******************************************************************************
* Utility Methods
******************************************************************************/

// TODO Define error flags in EEPROM

void errorFlash(byte errorLevel) {
	const uint16_t ERROR_FLASH_HALF_PERIOD = 166;
	for (byte i = 0; i < errorLevel; i++) {
		setStatusLight(true);
		delay(ERROR_FLASH_HALF_PERIOD);
		setStatusLight(false);
		delay(ERROR_FLASH_HALF_PERIOD);
	}
}

void setStatusLight(bool enable) {
	digitalWrite(STATUS_LED_PIN, enable);
}

// Read one entry from a csv file.
// str is the output string buffer.
// maxSize is the size of str including terminating null.
// delim is typically ',' (default) but can be any char.
// Returns:
// '\0' if end of file
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
			rtn = '\0';
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

		// Skip CR
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

void powerDownUntilCharge() {
	// Turn everything off
	Serial.println("Powering down until battery charging resumes.");
	Serial.flush();
	Serial.end();
	setStatusLight(false);

	// Sleep until we are not charging/done
	while (SolarCharger::isCharging() || SolarCharger::isChargingDone()) {
		powerDownFor8Seconds();
	}

	// Sleep while we are not charging/done
	while (!SolarCharger::isCharging() && !SolarCharger::isChargingDone()) {
		powerDownFor8Seconds();
	}

	// Start up
	Serial.begin(BAUD_RATE);
	Serial.println(FIRMWARE_VERSION);

	setStatusLight(true);
	delay(1000);
	setStatusLight(false);
}

// Enters power down state for 8 seconds with ADC and Brown-Out Detect module disabled
void powerDownFor8Seconds() {
	LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);	// This also pauses the millis() timer
	Diagnostics::updateTimes(8);
	//LowPower.idle(SLEEP_8S, ADC_OFF,
	//	TIMER2_ON, TIMER1_ON, TIMER0_ON,
	//	SPI_OFF, USART0_ON, TWI_OFF);
}
