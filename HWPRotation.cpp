#include "HWPRotation.h"

using namespace std;

HWPRotationTrigger::HWPRotationTrigger(double _period)
{
	firstTime = 1;
	period = _period;
}

HWPRotationTrigger::~HWPRotationTrigger()
{
}

void HWPRotationTrigger::start()
{
	struct timezone tz;
	gettimeofday(&startTime,&tz);
}

bool HWPRotationTrigger::check(int *currentStep)
{
	struct timeval currTime;
	struct timezone tz;
	gettimeofday(&currTime,&tz);
	double deltaPrev = (double)(prevTime.tv_sec - startTime.tv_sec) + 1e-6*(double)(prevTime.tv_usec - startTime.tv_usec);
	double deltaCurr = (double)(currTime.tv_sec - startTime.tv_sec) + 1e-6*(double)(currTime.tv_usec - startTime.tv_usec);
	prevTime.tv_sec = currTime.tv_sec;
	prevTime.tv_usec = currTime.tv_usec;
	*currentStep = (int)(deltaCurr/period);
	if (firstTime) {
		firstTime = 0;
		return 0;
	} else {
		return ((int)(deltaPrev/period) != (int)(deltaCurr/period));
	}
}
