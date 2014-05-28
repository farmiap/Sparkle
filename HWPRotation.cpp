#include "HWPRotation.h"

using namespace std;

HWPRotationTrigger::HWPRotationTrigger(double _period)
{
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

bool HWPRotationTrigger::check()
{
	struct timeval currTime;
	struct timezone tz;
	gettimeofday(&currTime,&tz);
	double deltaPrev = (double)(prevTime.tv_sec - startTime.tv_sec) + 1e-6*(double)(prevTime.tv_usec - startTime.tv_usec);
	double deltaCurr = (double)(currTime.tv_sec - startTime.tv_sec) + 1e-6*(double)(currTime.tv_usec - startTime.tv_usec);
	prevTime.tv_sec = currTime.tv_sec;
	prevTime.tv_usec = currTime.tv_usec;
	return ((int)(deltaPrev/period) != (int)(deltaCurr/period));
}
