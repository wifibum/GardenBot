// 
// 
// 

#include "MoistureSensor.h"

MoistureSensor::MoistureSensor(byte bDigitalPowerPin, byte bAnalogSensorPin, const __FlashStringHelper* fsSensorName) : Sensor(fsSensorName)
{
	m_bDigitalPowerPin = bDigitalPowerPin;
	m_bAnalogSensorPin = bAnalogSensorPin;
	m_iLastReading = -1;

	pinMode(m_bDigitalPowerPin, OUTPUT);
	digitalWrite(m_bDigitalPowerPin, LOW);
}

long MoistureSensor::StartReading()
{
	digitalWrite(m_bDigitalPowerPin, HIGH);
	return m_SensorPeriod;
}

int MoistureSensor::EndReading()
{
	m_iLastReading = analogRead(m_bAnalogSensorPin);
	digitalWrite(m_bDigitalPowerPin, LOW);
	return LastReading();
}

int MoistureSensor::LastReading()
{
	return m_iLastReading;
}


// Static members

long int MoistureSensor::m_SensorPeriod = 15L;
