#ifndef HWPROTATION_H
#define HWPROTATION_H

#include <sys/time.h>
#include <iostream>
#include <vector>

using namespace std;

class HWPRotationTrigger
{
private:
	int firstTime;
	double period;
	struct timeval startTime;
	struct timeval prevTime;
public:
	HWPRotationTrigger(double _period);
	~HWPRotationTrigger();

	void start();
	bool check(int *currentStep);
};

class HWPAngleContainer
{
private:
	vector<double> angles; // vector contains angles corresponding to frames
	vector<int> moved; // vector contains whether HWP was moving during frame acquisition

	vector<int> intrvBegins;
	vector<int> intrvEnds;
	vector<double> intrvAngles;
public:
	HWPAngleContainer();
	~HWPAngleContainer();

	void addStatusAndAngle(int _status,double _angle);
	void print();
	void cleanStatus();
	void convertToIntervals();
};

#endif
