/*
	TODO:
	- Modify to make sensor manager dynamcially create array to store readings
	- Modify transmit sensor readings to make use of sensor manager, spin sensors added
	- Generally make system handle dynamic number of sensors
	- Add failure LED to let me know if it can't send data or some other error condition has occured
	- Allow multiple sensors to share power source.  Require making the power activate/deactivate an increment.decrement
	- Allow receive command via serial to dump error conditions
	- Pull config from web site (sensor levels to react to, intervul settings)
	- Put button on device to force re-pull config
	- Send notice in each transaction if new config avaliable
	- Store last config in EEPROM ** Not sure this has value
	- Modify to store the last read in EEPROM if failed
*/

#include <EEPROM.h>
#include <Time.h>
#include <TimeAlarms.h>
#include <SoftwareSerial.h>
#include <WiFlyHQ.h>
#include "Sensor.h"
#include "MoistureSensor.h"
#include "SensorManager.h"
#include "WaterValve.h"
#include "types.h"

// Software serial to wifly
#define rxPin 10
#define txPin 11

// Wifly power pin
#define wfPower		8

// Sensor pins
#define s1Power		3
#define s1			A1
#define rainPower	4
#define rain		A2

#define failLED		13
const char* firstErrorMessage = 0;

const long int SENSOR_FREQ = 1000L * 60L;
const long int SENSOR_PERIOD = 1000L * 15L;

const char c_WebAddress[] = "mygarden.azurewebsites.net";
time_t t_LastWiflyReboot;

#define FSTR(string_literal) (reinterpret_cast<const __FlashStringHelper *>(string_literal))
#define F2P(flash_string)  (reinterpret_cast<const char *>(flash_string))

#define NUM_SENSORS 2
const PROGMEM char Sensor_Name_1[] = "s1";
const PROGMEM char Sensor_Name_rain[] = "rain";

MoistureSensor Sensor1(s1Power, s1, FSTR(Sensor_Name_1));
MoistureSensor RainSensor(rainPower, rain, FSTR(Sensor_Name_rain));

// EEPROM LIFO queue for old readings
// Because the readings will grow dynamically we will recorde them
// starting at the end (highest address) of the EEPROM memory.
// QUEUE_START will store the beginning of the last entry (or lowest address)
// 
// EEPROM Memory Example
//
// ADDR			Data		Description
//
// 0x0000		NA			Not used (read somewhere first byte might be unrelyable)
// 0x0001-00	E2END - 40	QUEUE_START
// 0x0002-??	??			Other EEPROM Data
// ...
// E2END - 40	0x0F		Reading3 - (15 bytes of data)
// E2END - 25	0x0F		Reading2 - (15 bytes of data)
// E2END - 10	0x0F		Reading1 - (10 bytes of data) ** Readings might be of different sizes

#define QUEUE_START 1 
// Set reserved above the used part of EEPROM  (above all other EEPROM stored data)
#define QUEUE_RESERVED 11

SoftwareSerial wifiSerial(rxPin, txPin);
WiFly wifly = WiFly();

time_t getNtpTime()
{
	return wifly.getRTC();
}

void FormatTime(char*buf, time_t curTime)
{
	sprintf_P(buf, PSTR("%s %02d,%04d %02d:%02d:%02d"),
		monthStr(month(curTime)),
		day(curTime),
		year(curTime),
		hourFormat12(curTime),
		minute(curTime),
		second(curTime));
}

void DumpEEPROM()
{
	Serial.println();
	Serial.print(F("EEPROM:"));
	for (int x = 0; x < E2END; x++)
	{
		if (x% 10 == 0)
		{
			if (x > 0)
			{
				Serial.print(F(" - "));
				for (int i = x - 10; i < x; i++)
				{
					char c = (char)EEPROM.read(i);
					if (isprint(c))
					{
						Serial.print(c);
					}
					else
					{
						Serial.print(F("."));
					}
				}
			}
			Serial.println();
			if (x <= 0xF) Serial.print(F("000"));
			else if (x <= 0xFF) Serial.print(F("00"));
			else if (x <= 0xFFF) Serial.print(F("0"));
			Serial.print(x, HEX);
			Serial.print(F(" - "));
		}


		uint8_t byte = EEPROM.read(x);
		if (byte <= 0xF) Serial.print(F("0"));
		Serial.print(byte, HEX);
		Serial.print(F(" "));
	}
}

void Error(const __FlashStringHelper* errorMessage)
{
	if (errorMessage == 0)
	{
		digitalWrite(failLED, HIGH);
		firstErrorMessage = F2P(errorMessage);
	}
	Serial.println(errorMessage);
}

// the setup function runs once when you press reset or power the board
void setup() {
	pinMode(failLED, OUTPUT);
	digitalWrite(failLED, HIGH);

	Serial.begin(115200);
	pinMode(rxPin, INPUT);
	pinMode(txPin, OUTPUT);
	pinMode(wfPower, OUTPUT);

	// Powerup the wifly
	digitalWrite(wfPower, HIGH);
	delay(5000);

	Serial.println(F("starting wifly"));
	wifiSerial.begin(9600);
	wifly.begin(&wifiSerial,&Serial);
	Serial.println(F("wifly started."));

	// useful for debugging the wifly via the serial interface
	//wifly.terminal();

	// Start syncing time for wifly
	char address[30];
	strcpy_P(address, PSTR("131.107.13.100"));

	wifly.setTimeAddress(address);
	wifly.setTimezone(5);
	wifly.time();

	// Sync time between wifly and arduino
	setSyncProvider(getNtpTime);

	//Serial.print("Arduino time synced:");
	//char messageBuf[60];
	//FormatTime(messageBuf, now());
	//Serial.println(messageBuf);

  SensorManager.AddSensor(&Sensor1);
  SensorManager.AddSensor(&RainSensor);
  

//  EEPROM.put(QUEUE_START, E2END);
//  LogSensorReadings();
//  SendOldReadings();
  //Alarm.timerRepeat(0, 60, 0, CheckSensors);
  Alarm.timerRepeat(0, 0, 30, CheckSensors);

  
//  DumpEEPROM();

  //EEPROM.put(QUEUE_START, 0x03EB);

  t_LastWiflyReboot = now();

  digitalWrite(failLED, LOW);
}

void LogSensorReadings()
{

	FullReading reading;
	SensorReading sensorData[NUM_SENSORS];
	reading.SensorReadings = sensorData;

	reading.TimeStamp = now();
	reading.NumberOfSensors = NUM_SENSORS;

	reading.SensorReadings[0].SensorName = Sensor1.GetSensorName();
	reading.SensorReadings[0].SensorReading = Sensor1.LastReading();
	reading.SensorReadings[1].SensorName = RainSensor.GetSensorName();
	reading.SensorReadings[1].SensorReading = RainSensor.LastReading();
	if (!TransmitSensorReading(reading,PGMStr))
	{
		Error(F("Failed to send reading."));
		StoreReading(reading);
	}
}

void ResetWifly()
{
	if ((now() - 300) > t_LastWiflyReboot)  // only reboot every 5 min at most
	{
		Serial.println("Resetting Wifly");
		digitalWrite(wfPower, LOW);
		delay(5000);
		digitalWrite(wfPower, HIGH);
		delay(5000);

		wifly.  ,.reboot();
	}
}

void SendOldReadings()
{
	// Read old readings from EEPROM and try to send them.
	// The queue of old readins stored in EEPROM will be a LIFO queue to make
	// memory management the simplest
	int pOldReading;
	EEPROM.get(QUEUE_START, /* ref */ pOldReading);
	do{
		if (pOldReading == 0xFFFF)
		{
			pOldReading = E2END;
		}

		if (pOldReading != E2END)
		{
			if (!SendOldReading(pOldReading))
			{
				break; // Failed to send reading, so try again later...
			}
			int iReadingSize;
			EEPROM.get<int>(pOldReading, iReadingSize);
			pOldReading += iReadingSize;
		}
	} while (pOldReading != E2END);
	
	// Update to last reading we couldn't send.  We may have successfuly sent one or two stored readings but not all of them.
	EEPROM.put(QUEUE_START, pOldReading);

	// If we successfuly sent all old readings, clear the error condition, we seem to be working again.
	if (pOldReading == E2END)
	{
		firstErrorMessage = 0;
		digitalWrite(failLED, LOW);
	}
}

bool SendOldReading(int eepromAddress)
{
	FullReading reading;
	SensorReading sensorData[NUM_SENSORS];
	reading.SensorReadings = sensorData;

	int pOldReading = eepromAddress;

	int iOffset = 0;
	int iReadingSize;
	EEPROM.get<int>(pOldReading, iReadingSize);
	iOffset += sizeof(int); // Reading size

	EEPROM.get<time_t>(pOldReading + iOffset, reading.TimeStamp);
	iOffset += sizeof(reading.TimeStamp);

	EEPROM.get<int>(pOldReading + iOffset, reading.NumberOfSensors);
	iOffset += sizeof(reading.NumberOfSensors);

	for (int x = 0; x < reading.NumberOfSensors; x++)
	{

		// Just store the string pointer to EEPROM, the transmit function will read it.
		Serial.print("EEPromAddr : ");
		Serial.print(pOldReading + iOffset);
		Serial.print(" : ");

		reading.SensorReadings[x].SensorName = FSTR(pOldReading + iOffset);

		Serial.print((int)reading.SensorReadings[x].SensorName);

		for (; EEPROM.read(pOldReading + iOffset) != '\0'; iOffset++)
		{
			if (pOldReading + iOffset > E2END)
			{
				Error(F("Failed trying to find the end of a sensor name stored in EEPROM."));
				return false;
			}
		}
		iOffset++; // once more for the null.
		EEPROM.get(pOldReading + iOffset, reading.SensorReadings[x].SensorReading);
		iOffset += sizeof(reading.SensorReadings[x].SensorReading);
	}

	if (pOldReading + iOffset > E2END)
	{
		Error(F("Failed loading stored sensor reading from EEPROM."));
		return false;
	}

	return TransmitSensorReading(reading, EEPROMStr);
}

void StoreReading(struct FullReading reading)
{
	int pOldReading;
	EEPROM.get(QUEUE_START, /* ref */ pOldReading);
	if (pOldReading == 0xFFFF) pOldReading = E2END;

	int iReadingSize = 0;
	for (int x = 0; x < reading.NumberOfSensors; x++)
	{
		iReadingSize += strlen_P(F2P(reading.SensorReadings[x].SensorName)) + 1;
		iReadingSize += sizeof(reading.SensorReadings[x].SensorReading);
	}

	iReadingSize += sizeof(iReadingSize) + sizeof(reading.TimeStamp) + sizeof(reading.NumberOfSensors);

	int pCurrentReading = pOldReading - iReadingSize;
	if (pCurrentReading < QUEUE_RESERVED)
	{
		Error(F("Failed to store old reading because out of EEPROM space."));
		return;
	}

	int iOffset = 0;
	EEPROM.put(pCurrentReading, iReadingSize); // Total size
	iOffset += sizeof(iReadingSize);
	EEPROM.put<time_t>(pCurrentReading + iOffset, reading.TimeStamp); // 
	iOffset += sizeof(reading.TimeStamp);
	EEPROM.put(pCurrentReading + iOffset, reading.NumberOfSensors); // 
	iOffset += sizeof(reading.NumberOfSensors);
	for (int x = 0; x < reading.NumberOfSensors; x++)
	{
		const char* pgmSensorName = F2P(reading.SensorReadings[x].SensorName);
		//int iSensorNameLen = strlen_P(pgmSensorName);
		//iOffset += iSensorNameLen + 1;
		do
		{
			EEPROM.put(pCurrentReading + iOffset, pgm_read_byte(pgmSensorName));
			iOffset++;
			if (iOffset > iReadingSize)
			{
				Error(F("Some Error occured, failed to detect end of sensor name string correctly."));
				break;
			}
		} while (pgm_read_byte(pgmSensorName++) != '\0');

		EEPROM.put(pCurrentReading + iOffset, reading.SensorReadings[x].SensorReading);
		iOffset += sizeof(reading.SensorReadings[x].SensorReading);
	}
	if (iOffset != iReadingSize)
	{
		Error(F("Failed to store current reading into EEPROM."));
	}
	else
	{
		EEPROM.put(QUEUE_START, pCurrentReading);
	}
}

bool TransmitSensorReading(struct FullReading reading, enum StorageType storageType)
{
	bool bSent = false;
	char messageBuf[100];

	wifly.setIpProtocol(WIFLY_PROTOCOL_TCP);
	if (wifly.open(c_WebAddress, 80))
	{

		// Send initial post and token
		sprintf_P(messageBuf, PSTR("POST http://mygarden.azurewebsites.net/data.cshtml?Token=himom"));
		Serial.print(messageBuf);
		wifly.print(messageBuf);

		// Send timestamp
		sprintf_P(messageBuf, PSTR("&time=%ld"), reading.TimeStamp);
		Serial.print(messageBuf);
		wifly.print(messageBuf);

		// Send sensor data
		for (int x = 0; x < reading.NumberOfSensors; x++)
		{
			// Use of special capitial S to pull from PROGMEM
			if (storageType == PGMStr)
			{
				sprintf_P(messageBuf, PSTR("&%S=%d")
					, reading.SensorReadings[x].SensorName, reading.SensorReadings[x].SensorReading);
			}
			else
			{
				char sensorName[21];
				int pSensorName = (int) reading.SensorReadings[x].SensorName;
				for (int i = 0; i < 20; i++)
				{
					char c = EEPROM.read(pSensorName + i);
					sensorName[i] = c;
					if (c == '\0') break;
				}
				sensorName[21] = 0;
				sprintf_P(messageBuf, PSTR("&%s=%d")
					, sensorName, reading.SensorReadings[x].SensorReading);
			}

			Serial.print(messageBuf);
			wifly.print(messageBuf);
		}

		// Send HTTP headers
		sprintf_P(messageBuf, PSTR(" HTTP/1.1\r\n"
									"Host: mygarden.azurewebsites.net\r\n"
									"Content-Length : 0\r\n"));

		Serial.print(messageBuf);
		wifly.print(messageBuf);
		wifly.println();
		//wifly.println();

		// Check response
		if (wifly.match("<p>Thanks!</p>", 10000))
		{
			Serial.println(F("Sent data OK"));
			bSent = true;
		}
		else
		{
			Error(F("Data failed to send"));
		}
		wifly.close();
	}
	else
	{
		Error(F("Failed to open web site"));
		ResetWifly();
	}
	Serial.println(messageBuf);
	if (!bSent && storageType == PGMStr)
	{
		StoreReading(reading);
	}
	return bSent;
}


void CheckSensors()
{
	SensorManager.AsyncStartReadings(LogSensorReadings);
	SendOldReadings();
}

// the loop function runs over and over again forever
void loop() {
	Alarm.delay(0);
}
