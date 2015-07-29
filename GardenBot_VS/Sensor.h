// Sensor.h

#ifndef _SENSOR_h
#define _SENSOR_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

class Sensor
{
 protected:
	 const __FlashStringHelper* m_sSensorName;

 public:
	 Sensor(const __FlashStringHelper* fsSensorName) { m_sSensorName = fsSensorName; }
	 virtual long StartReading() = 0; // Turns on the sensor, returns delay required before EndReading
	 virtual int EndReading() = 0; // Takes a reading from the sensor
	 virtual int LastReading() = 0; // Returns the last reading from the sensor (-1 if no reading has been taken)
	 virtual const __FlashStringHelper* GetSensorName() { return m_sSensorName; }
};

#endif

