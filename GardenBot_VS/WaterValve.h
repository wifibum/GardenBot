// WaterValve.h

#ifndef _WATERVALVE_h
#define _WATERVALVE_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

class WaterValveClass
{
 protected:


 public:
	void init();
};



extern WaterValveClass WaterValve;

#endif

