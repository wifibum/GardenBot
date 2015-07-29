#ifndef __TYPES__
#define __TYPES__

enum StorageType
{
	PGMStr,
	EEPROMStr
};

struct SensorReading
{
	const __FlashStringHelper* SensorName;
	int SensorReading;
};

struct FullReading
{
	time_t TimeStamp;
	int NumberOfSensors;
	SensorReading* SensorReadings;
};

#endif