/*
  RadioLib SX126x Transmit with Interrupts Example

  This example transmits LoRa packets with one second delays
  between them. Each packet contains up to 256 bytes
  of data, in the form of:
  - Arduino String
  - null-terminated char array (C-string)
  - arbitrary binary data (byte array)

  Other modules from SX126x family can also be used.

  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration#sx126x---lora-modem

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/
*/

// include the library
#include <Arduino.h>
#include <RadioLib.h>
#define uS_TO_S_FACTOR 1000000
#define TIME_TO_SLEEP 2

// SX1262 has the following connections:
// NSS pin:   (default 10) 5
// DIO1 pin:  (default 2) 2
// NRST pin:  (default 3) 14
// BUSY pin:  (default 9) 0
SX1262 radio = new Module(5, 2, 14, 4);

// save transmission state between loops
int transmissionState = RADIOLIB_ERR_NONE;

// flag to indicate that a packet was sent
volatile bool transmittedFlag = false;

// this function is called when a complete packet
// is transmitted by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR
#endif
void setFlag(void)
{
	// we sent a packet, set the flag
	transmittedFlag = true;
}

void lora_setup();
void lora_send(String str);

// counter to keep track of transmitted packets
int count = 0;

/* Use the Espressif EEPROM library. Skip otherwise */
#if defined(ARDUINO_ARCH_ESP32) || (ARDUINO_ARCH_ESP8266)
#include <EEPROM.h>
#define USE_EEPROM
#endif

#include <bsec2.h>

#define CLASSIFICATION 1
#define REGRESSION 2

/* Configuration for two class classification used here
 * For four class classification please use configuration under config/FieldAir_HandSanitizer_Onion_Cinnamon
 */
/* Note :
		  For the classification output from BSEC algorithm set OUTPUT_MODE macro to CLASSIFICATION.
		  For the regression output from BSEC algorithm set OUTPUT_MODE macro to REGRESSION.
*/
#define OUTPUT_MODE CLASSIFICATION

const uint8_t bsec_config[] = {
#include "config/UserCode/bsec_selectivity.txt"
};

/* Macros used */
#define STATE_SAVE_PERIOD UINT32_C(360 * 60 * 1000) /* 360 minutes - 4 times a day */
// #define PANIC_LED LED_BUILTIN
#define ERROR_DUR 1000

/* Helper functions declarations */

/**
 * @brief : This function updates/saves BSEC state
 * @param[in] bsec  : Bsec2 class object
 */
void updateBsecState(Bsec2 bsec);

/**
 * @brief : This function is called by the BSEC library when a new output is available
 * @param[in] input     : BME68X sensor data before processing
 * @param[in] outputs   : Processed BSEC BSEC output data
 * @param[in] bsec      : Instance of BSEC2 calling the callback
 */
void newDataCallback(const bme68xData data, const bsecOutputs outputs, Bsec2 bsec);

/**
 * @brief : This function retrieves the existing state
 * @param : Bsec2 class object
 */
bool loadState(Bsec2 bsec);

/**
 * @brief : This function writes the state into EEPROM
 * @param : Bsec2 class object
 */
bool saveState(Bsec2 bsec);

/* Create an object of the class Bsec2 */
Bsec2 envSensor;
#ifdef USE_EEPROM
static uint8_t bsecState[BSEC_MAX_STATE_BLOB_SIZE];
#endif
/* Gas estimate names will be according to the configuration classes used */
const String gasName[] = {"Field Air", "Hand sanitizer", "Undefined 3", "Undefined 4"};

int counter = 0;

#define GPIO32_3V3 32
#define GPIO33_AIR 33
#define GPIO25_FIRE 25

void setup()
{
	bsecSensor sensorList[] = {
		BSEC_OUTPUT_RAW_TEMPERATURE,
		BSEC_OUTPUT_RAW_PRESSURE,
		BSEC_OUTPUT_RAW_HUMIDITY,
		BSEC_OUTPUT_RAW_GAS,
		BSEC_OUTPUT_RAW_GAS_INDEX,
		BSEC_OUTPUT_GAS_ESTIMATE_1,
		BSEC_OUTPUT_GAS_ESTIMATE_2,
		BSEC_OUTPUT_GAS_ESTIMATE_3,
		BSEC_OUTPUT_GAS_ESTIMATE_4};
	Serial.begin(115200);
	pinMode(GPIO32_3V3, OUTPUT);
	pinMode(GPIO33_AIR, OUTPUT);
	pinMode(GPIO25_FIRE, OUTPUT);
	digitalWrite(GPIO32_3V3, HIGH);
  #ifdef USE_EEPROM
    EEPROM.begin(BSEC_MAX_STATE_BLOB_SIZE + 1);
  #endif
    Wire.begin();
	esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
	delay(100);
	/* Initialize the library and interfaces */
	envSensor.begin(BME68X_I2C_ADDR_HIGH, Wire);

	/* Load the configuration string that stores information on how to classify the detected gas */
	envSensor.setConfig(bsec_config);

	/* Copy state from the EEPROM to the algorithm */
	loadState(envSensor);

	/* Subscribe for the desired BSEC2 outputs */
	envSensor.updateSubscription(sensorList, ARRAY_LEN(sensorList), BSEC_SAMPLE_RATE_SCAN);

	/* Whenever new data is available call the newDataCallback function */
	envSensor.attachCallback(newDataCallback);
	lora_setup();
}

void loop()
{
	envSensor.run();
	if (counter == 2)
	{
		delay(2000);
		printf("GOING TO SLEEP for 0.5min\n");
		esp_sleep_enable_timer_wakeup(1000000 * 1);
		esp_deep_sleep_start();

		counter = 0;
	}
}

void lora_setup()
{

	// initialize SX1262 with default settings
	Serial.print(F("[SX1262] Initializing ... "));
	int state = radio.begin();
	if (state == RADIOLIB_ERR_NONE)
	{
		Serial.println(F("success!"));
	}
	else
	{
		Serial.print(F("failed, code "));
		Serial.println(state);
		while (true)
			;
	}

	// you can also change the settings at runtime
	// and check if the configuration was changed successfully

	// set carrier frequency to 433.5 MHz
	if (radio.setFrequency(868.0) == RADIOLIB_ERR_INVALID_FREQUENCY)
	{
		Serial.println(F("Selected frequency is invalid for this module!"));
		while (true)
			;
	}

	// set bandwidth to 250 kHz
	if (radio.setBandwidth(125.0) == RADIOLIB_ERR_INVALID_BANDWIDTH)
	{
		Serial.println(F("Selected bandwidth is invalid for this module!"));
		while (true)
			;
	}

	// set spreading factor to 10
	if (radio.setSpreadingFactor(10) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR)
	{
		Serial.println(F("Selected spreading factor is invalid for this module!"));
		while (true)
			;
	}

	// set coding rate to 6
	if (radio.setCodingRate(6) == RADIOLIB_ERR_INVALID_CODING_RATE)
	{
		Serial.println(F("Selected coding rate is invalid for this module!"));
		while (true)
			;
	}

	// set LoRa sync word to 0xAB default 34
	// if (radio.setSyncWord(0xAB) != RADIOLIB_ERR_NONE) {
	//  Serial.println(F("Unable to set sync word!"));
	//  while (true);
	//}

	// set output power to 10 dBm (accepted range is -17 - 22 dBm)
	if (radio.setOutputPower(14) == RADIOLIB_ERR_INVALID_OUTPUT_POWER)
	{
		Serial.println(F("Selected output power is invalid for this module!"));
		while (true)
			;
	}

	// set over current protection limit to 80 mA (accepted range is 45 - 240 mA)
	// NOTE: set value to 0 to disable overcurrent protection
	// if (radio.setCurrentLimit(80) == RADIOLIB_ERR_INVALID_CURRENT_LIMIT) {
	//  Serial.println(F("Selected current limit is invalid for this module!"));
	//  while (true);
	//}

	// set LoRa preamble length to 15 symbols (accepted range is 0 - 65535)
	if (radio.setPreambleLength(4) == RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH)
	{
		Serial.println(F("Selected preamble length is invalid for this module!"));
		while (true)
			;
	}
}

void lora_send(String str)
{
	// set the function that will be called
	// when packet transmission is finished
	radio.setPacketSentAction(setFlag);

	// start transmitting the first packet
	Serial.print(F("[SX1262] Sending first packet ... "));

	// you can transmit C-string or Arduino string up to
	// 256 characters long
	transmissionState = radio.startTransmit(str);

	// you can also transmit byte array up to 256 bytes long
	/*
	  byte byteArr[] = {0x01, 0x23, 0x45, 0x67,
						0x89, 0xAB, 0xCD, 0xEF};
	  state = radio.startTransmit(byteArr, 8);
	*/
	while (transmittedFlag == false)
	{
	}
	// reset flag
	transmittedFlag = false;

	if (transmissionState == RADIOLIB_ERR_NONE)
	{
		// packet was successfully sent
		Serial.println(F("transmission finished!"));

		// NOTE: when using interrupt-driven transmit method,
		//       it is not possible to automatically measure
		//       transmission data rate using getDataRate()
	}
	else
	{
		Serial.print(F("failed, code "));
		Serial.println(transmissionState);
	}

	// clean up after transmission is finished
	// this will ensure transmitter is disabled,
	// RF switch is powered down etc.
	radio.finishTransmit();
}

void updateBsecState(Bsec2 bsec)
{
	static uint16_t stateUpdateCounter = 0;
	bool update = false;

	if (!stateUpdateCounter || (stateUpdateCounter * STATE_SAVE_PERIOD) < millis())
	{
		/* Update every STATE_SAVE_PERIOD minutes */
		update = true;
		stateUpdateCounter++;
	}

	if (update && !saveState(bsec))
	{
	}
}

void newDataCallback(const bme68xData data, const bsecOutputs outputs, Bsec2 bsec)
{
	if (!outputs.nOutputs)
	{
		return;
	}

	// Serial.println("BSEC outputs:\n\ttimestamp = " + String((int) (outputs.output[0].time_stamp / INT64_C(1000000))));
	uint8_t index = 0;
	String message="";

	for (uint8_t i = 0; i < outputs.nOutputs; i++)
	{
		const bsecData output = outputs.output[i];
		switch (output.sensor_id)
		{
		case BSEC_OUTPUT_RAW_TEMPERATURE:
			// Serial.println("\ttemperature = " + String(output.signal));
			//Add to message the temperature
			message+=String(output.signal)+" ";
			break;
		case BSEC_OUTPUT_RAW_PRESSURE:
			// Serial.println("\tpressure = " + String(output.signal));
			//Add to message the pressure
			message+=String(output.signal)+" ";
			break;
		case BSEC_OUTPUT_RAW_HUMIDITY:
			// Serial.println("\thumidity = " + String(output.signal));
			//Add to message the humidity
			message+=String(output.signal)+" ";
			break;
		case BSEC_OUTPUT_RAW_GAS:
			// Serial.println("\tgas resistance = " + String(output.signal));
			break;
		case BSEC_OUTPUT_RAW_GAS_INDEX:
			// Serial.println("\tgas index = " + String(output.signal));
			break;
		case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE:
			// Serial.println("\tcompensated temperature = " + String(output.signal));
			break;
		case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY:
			// Serial.println("\tcompensated humidity = " + String(output.signal));
			break;
		case BSEC_OUTPUT_GAS_ESTIMATE_1:
		case BSEC_OUTPUT_GAS_ESTIMATE_2:
			index = (output.sensor_id - BSEC_OUTPUT_GAS_ESTIMATE_1);
			if (index == 0) // The four classes are updated from BSEC with same accuracy, thus printing is done just once.
			{
				// Serial.println("\taccuracy = " + String(output.accuracy));
				counter++;
			}
			if (index == 0)
			{
				Serial.println("\tFIRE" + String(index + 1) + " probability : " + String(output.signal * 100) + "%");
				//Add to message the fire probability
				//message+=String(output.signal * 100)+" ";
				if(counter==2 && output.signal*100>70){
					//Set gpio FIRE to HIGH
					digitalWrite(GPIO25_FIRE, HIGH);
				}
			}
			if (index == 1){
				Serial.println("\tAMBIENT AIR" + String(index + 1) + " probability : " + String(output.signal * 100) + "%");
				//Add to message the ambient air probability
				//message+=String(output.signal * 100)+" ";
				if(counter==2 && output.signal*100>70){
					//Set gpio AIR to HIGH
					digitalWrite(GPIO33_AIR, HIGH);
				}
			}
			break;
		case BSEC_OUTPUT_GAS_ESTIMATE_3:
		case BSEC_OUTPUT_GAS_ESTIMATE_4:
			break;
		default:
			break;
		}
	}

	updateBsecState(envSensor);
}

bool loadState(Bsec2 bsec)
{
#ifdef USE_EEPROM

	if (EEPROM.read(0) == BSEC_MAX_STATE_BLOB_SIZE)
	{
		/* Existing state in EEPROM */
		// Serial.println("Reading state from EEPROM");
		// Serial.print("State file: ");
		for (uint8_t i = 0; i < BSEC_MAX_STATE_BLOB_SIZE; i++)
		{
			bsecState[i] = EEPROM.read(i + 1);
			// Serial.print(String(bsecState[i], HEX) + ", ");
		}
		// Serial.println();

		if (!bsec.setState(bsecState))
			return false;
	}
	else
	{
		/* Erase the EEPROM with zeroes */
		// Serial.println("Erasing EEPROM");

		for (uint8_t i = 0; i <= BSEC_MAX_STATE_BLOB_SIZE; i++)
			EEPROM.write(i, 0);

		EEPROM.commit();
	}
#endif
	return true;
}

bool saveState(Bsec2 bsec)
{
#ifdef USE_EEPROM
	if (!bsec.getState(bsecState))
		return false;

	// Serial.println("Writing state to EEPROM");
	// Serial.print("State file: ");

	for (uint8_t i = 0; i < BSEC_MAX_STATE_BLOB_SIZE; i++)
	{
		EEPROM.write(i + 1, bsecState[i]);
		// Serial.print(String(bsecState[i], HEX) + ", ");
	}
	// Serial.println();

	EEPROM.write(0, BSEC_MAX_STATE_BLOB_SIZE);
	EEPROM.commit();
#endif
	return true;
}
