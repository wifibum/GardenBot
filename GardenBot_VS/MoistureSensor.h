// MoistureSensor.h

// A Moisture sensor needs to be unpowered most of the time to avoid corrosion of the sensor
// To read the sensor, we need to turn it on, wait a few seconds (15 by default) then take the reading.
// Then power it back off

#ifndef _MOISTURESENSOR_h
#define _MOISTURESENSOR_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "Sensor.h"

class MoistureSensor : public Sensor
{
protected:
	byte m_bDigitalPowerPin;
	byte m_bAnalogSensorPin;
	int m_iLastReading;
	static long m_SensorPeriod; // How long to wait to take a reading.  
	// i.e. Turn sensor on, wait for Period time, take reading...


public:

	MoistureSensor(byte bDigitalPowerPin, byte bAnalogSensorPin, const __FlashStringHelper* fsSensorName);

	virtual long StartReading(); // Turns on the sensor, returns delay required before EndReading
	virtual int EndReading(); // Takes a reading from the sensor
	virtual int LastReading(); // Returns the last reading from the sensor (-1 if no reading has been taken)
	static void SetPeriod(long newPeriod) // Overrides the sensor period
	{
		m_SensorPeriod = newPeriod;
	}
};

#endif

