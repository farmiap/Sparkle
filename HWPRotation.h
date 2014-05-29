#ifndef HWPROTATION_H
#define HWPROTATION_H

#include <sys/time.h>
#include <iostream>

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

#endif
