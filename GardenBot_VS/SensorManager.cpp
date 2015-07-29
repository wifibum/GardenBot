// 
// 
// 

#include <TimeAlarms.h>
#include "SensorManager.h"


bool SensorManagerClass::AddSensor(MoistureSensor* pSensor)
{
	if (m_bLastSensor + 1 == MAX_SENSORS)
		return false;

	m_arrSensors[++m_bLastSensor] = pSensor;
}

void SensorManagerClass::AsyncStartReadings(AsyncComplete pCallback)
{
	long lReadingDelay = 0; // How long do we wait to take the reading

	m_pCallback = pCallback;
	for (int x = 0; x <= m_bLastSensor; x++)
	{
		long lCurReadingDelay = (m_arrSensors[x])->StartReading();
		if (lCurReadingDelay > lReadingDelay) lReadingDelay = lCurReadingDelay;
	}
	Alarm.timerOnce(0, 0, 15, SensorManagerClass::AlarmHandeler);
}

void SensorManagerClass::AlarmHandeler()
{
	SensorManager.EndReadings(); 
}

void SensorManagerClass::EndReadings()
{
	for (int x = 0; x <= m_bLastSensor; x++)
	{
		(m_arrSensors[x])->EndReading();
	}

	// Invoke the callback if there is one
	if (m_pCallback) m_pCallback();
}

SensorManagerClass SensorManager;

