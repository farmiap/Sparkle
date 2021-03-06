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
	double speed;		// engine speed, degrees per second
	int microstepFrac;
	int directionInverted;  // Looking from detector to telescope: 0 CCW, 1 CW
	
	device_t device;
	result_t result;
	status_t state;
	engine_settings_t engine_settings;
	edges_settings_t edges_settings;
	
public:
	StandaRotationStage();
	~StandaRotationStage();

	void printDeviceName();
	int initializeStage(string _deviceName, double _convSlope, double _convIntercept, int _dirInv, double speed, int forced);
	int setSpeed(double _speed);
	int startMoveToAngle(double deltaAngle);
	int startMoveToAngleWait(double deltaAngle);
	int startMoveByAngle(double deltaAngle);
	int startContiniousMotion();
	int stopContiniousMotion();
	int getAngle(int *isMoving,double *angle);
};

void print_state (status_t* state);
string error_string (result_t result);
double getNextStepValue(int currentStep, double refStep, int pairNum, int groupNum);
int anglesProximityR(double a, double b, double margin);

#endif
