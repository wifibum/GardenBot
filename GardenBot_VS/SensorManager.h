// SensorManager.h

// Manage taking a reading across a group of sensors.  The 
// readings are asyc because some sensors may need time to 
// "warm up" before you can take an accurate reading.
//

#ifndef _SensorManager_h
#define _SensorManager_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "MoistureSensor.h"

#ifndef MAX_SENSORS
#define MAX_SENSORS 5
#endif

typedef void(*AsyncComplete)();  // callback function typedef 


class SensorManagerClass
{
protected:
	MoistureSensor* m_arrSensors[MAX_SENSORS];
	byte m_bLastSensor = -1;
	AsyncComplete m_pCallback;
	int m_iSecondsToWait = 15;

	static void AlarmHandeler(); // Callback for alarm

public:
	SensorManagerClass(){};

	bool AddSensor(MoistureSensor* pSensor); // Add a sensor to be managed

	void AsyncStartReadings(AsyncComplete); // Starts the reading process, invokes the callback when the readings are ready

	void StartReadings(); // Manualy activate the sensors
	void EndReadings(); // Manualy take a reading and deactiveate the sensors

	void SetWaitSeconds(int iSeconds) // Sets the wait time for a sensor reading
	{
		m_iSecondsToWait = iSeconds;
	}
};

extern SensorManagerClass SensorManager;

#endif

