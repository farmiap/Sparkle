#include <cstdio>

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

HWPAngleContainer::HWPAngleContainer()
{
}

HWPAngleContainer::~HWPAngleContainer()
{
}

void HWPAngleContainer::addStatusAndAngle(int _status,double _angle)
{
	moved.push_back(_status);
	angles.push_back(_angle);
}

void HWPAngleContainer::print()
{
	FILE *f=fopen("temp.dat","w");
	vector<int>::iterator itm = moved.begin();
	for(vector<double>::iterator it=angles.begin();it!=angles.end();++it)
	{
		fprintf(f,"%d %f\n",*itm,*it);
		itm++;
	}
	fclose(f);

	FILE *f2=fopen("temp2.dat","w");
	vector<int>::iterator itb = intrvBegins.begin();
	vector<int>::iterator ite = intrvEnds.begin();
	for(vector<double>::iterator it=intrvAngles.begin();it!=intrvAngles.end();++it)
	{
		fprintf(f2,"%d %d %f\n",*itb,*ite,*it);
		itb++;
		ite++;
	}
	fclose(f2);
}

void HWPAngleContainer::cleanStatus()
{
	// This function handles specifics of Standa stage.
	// Before start of motion (~300-400 ms before) it returns
	// status "accelerating" for a very short time instead of actually not moving.
	// This function removes such events from status vector.

	vector<int>::iterator itm = moved.begin();
	vector<double>::iterator itnext;
	vector<int>::iterator itmnext;
	for(vector<double>::iterator it=angles.begin();it!=angles.end();++it)
	{
		itnext = it;
		itnext++;
		itmnext = itm;
		itmnext++;
		if ( (*itm == 1) && (*itmnext == 0) && (*it==*itnext) )
			*itm = 0;
		itm++;
	}
}

void HWPAngleContainer::convertToIntervals()
{
	intrvBegins.erase(intrvBegins.begin(),intrvBegins.end());
	intrvEnds.erase(intrvEnds.begin(),intrvEnds.end());
	intrvAngles.erase(intrvAngles.begin(),intrvAngles.end());

	vector<int>::iterator itm = moved.begin();
	bool inInterval = 0;
	int intervalBegin;
	int intervalEnd;
	double intervalAngle;
	int counter = 1;
	for(vector<double>::iterator it=angles.begin();it!=angles.end();++it)
	{
		if (!inInterval)
		{
			if (*itm == 0)
			{
				intervalBegin = counter;
				intervalAngle = *it;
				inInterval = 1;
			}
		}
		else
		{
			if (*itm == 1)
			{
				intervalEnd = counter-1;
				intrvBegins.push_back(intervalBegin);
				intrvEnds.push_back(intervalEnd);
				intrvAngles.push_back(intervalAngle);
				inInterval = 0;
			}
		}
		itm++;
		counter++;
	}

}
