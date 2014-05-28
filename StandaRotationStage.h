#ifndef STANDAROTATIONSTAGE_H
#define STANDAROTATIONSTAGE_H

#include <iostream>
#include <string>
#include <ximc.h>

using namespace std;

class StandaRotationStage
{
private:
	string deviceName;
	double convSlope; 	// degrees per engine step
	double convIntercept;	// engine position when P.A. is zero (steps)
	int microstepFrac;

	device_t device;
	result_t result;
	status_t state;
	engine_settings_t engine_settings;
	edges_settings_t edges_settings;

public:
	StandaRotationStage();
	~StandaRotationStage();

	void printDeviceName();
	int initializeStage(string _deviceName, double _convSlope, double _convIntercept);
};

void print_state (status_t* state);
string error_string (result_t result);

#endif
