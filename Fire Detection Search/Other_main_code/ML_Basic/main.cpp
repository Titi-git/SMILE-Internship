
// include the library
#include <Arduino.h>
#define uS_TO_S_FACTOR 1000000
#define TIME_TO_SLEEP 2


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
}

void loop()
{
	envSensor.run();
	if (counter == 2)
	{
		printf("GOING TO SLEEP for 0.5min\n");
		esp_sleep_enable_timer_wakeup(1000000 * 1);
		esp_deep_sleep_start();

		counter = 0;
	}
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

	for (uint8_t i = 0; i < outputs.nOutputs; i++)
	{
		const bsecData output = outputs.output[i];
		switch (output.sensor_id)
		{
		case BSEC_OUTPUT_RAW_TEMPERATURE:
			// Serial.println("\ttemperature = " + String(output.signal));
			break;
		case BSEC_OUTPUT_RAW_PRESSURE:
			// Serial.println("\tpressure = " + String(output.signal));
			break;
		case BSEC_OUTPUT_RAW_HUMIDITY:
			// Serial.println("\thumidity = " + String(output.signal));
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
			}
			if (index == 1)
				Serial.println("\tAMBIENT AIR" + String(index + 1) + " probability : " + String(output.signal * 100) + "%");
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
