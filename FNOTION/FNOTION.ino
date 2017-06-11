
#include <MorseTx.h>
#include "Sensors.h"
#include "SolarCharger.h"

#define DEBUG

// Pin numbers
const byte SD_SS_PIN       = 10;
const byte BEACON_EN_PIN   =  2;
const byte IADJ_PWM_PIN    =  3;
const byte STATUS_LED_PIN  =  4;

#define SEARYSPEAK_URL "searyspeak.ca"

// Objects
#ifdef DEBUG
  MorseTx morse(STATUS_LED_PIN);  // Use status LED to save my eyes
#else
  MorseTx morse(BEACON_EN_PIN);
#endif

/******************************************************************************
 * Setup / Initializers
 ******************************************************************************/

void setup() {
  #ifdef DEBUG
    Serial.begin(9600);
    while (!Serial);
	Serial.println("SearySpeak v1.0 - Compiled " __TIMESTAMP__);
  #endif
  
  initializeBeacon();
  Sensors::initialize();
  SolarCharger::initialize();
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
  
  #ifdef DEBUG
    morse.write(SEARYSPEAK_URL);
  #endif
  
  // Nightfall
  // TODO Load block of names from SD card into RAM
  
  // TODO Transmit names interspersed with URL
  
  // TODO Shut down after const number of names or amount of time (~1 hour)
  
  // TODO Wake up every ~hour to take sensor readings
  // TODO Maybe save all sensor data in RAM and write to memory card before first block read
  // TODO Detect next nightfall by millis() and light sensor
}

/******************************************************************************
 * Utility Methods
 ******************************************************************************/
