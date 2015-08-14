#ifndef STANDAACTUATOR_H
#define STANDAACTUATOR_H

#include <iostream>
#include <string>
#include <ximc.h>

using namespace std;

class StandaActuator
{
private:
	string deviceName;
	double speed;		// engine speed, degrees per second
	
	device_t device;
	result_t result;
	status_t state;
	engine_settings_t engine_settings;

public:
	StandaActuator();
	~StandaActuator();

	void printDeviceName();
	int initializeActuator(string _deviceName, double speed);
	int startMoveToPosition(int targetPosition);
	int getPosition(int *isMoving,int *position);
	int setLight(int _light);
};

void print_state (status_t* state);
string error_string (result_t result);

#endif
