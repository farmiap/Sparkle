#ifndef STANDAROTATIONSTAGE_H
#define STANDAROTATIONSTAGE_H

#include <iostream>
#include <string>

using namespace std;

class StandaRotationStage
{
private:
	string deviceName;
	double convSlope; 	// degrees per engine step
	double convIntercept;	// engine position when P.A. is zero (steps)
public:
	StandaRotationStage();
	~StandaRotationStage();

	int initializeStage(string _deviceName, double _convSlope, double _convIntercept);
};

#endif
