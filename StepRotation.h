#ifndef STEPROTATION_H
#define STEPROTATION_H

#include <sys/time.h>
#include <iostream>
#include <vector>

using namespace std;

class RotationTrigger
{
private:
	int firstTime;
	double period;
	struct timeval startTime;
	struct timeval prevTime;
public:
	RotationTrigger(double _period);
	RotationTrigger();
	~RotationTrigger();

	void start();
	void setPeriod(double _period);
	bool check(int *currentStep);
};

class AngleContainer
{
private:
	vector<double> angles; // vector contains angles corresponding to frames
	vector<int> moved; // vector contains whether HWP was moving during frame acquisition
	vector<int> numbers; // vector contains number of frame
	
	vector<int> intrvBegins;
	vector<int> intrvEnds;
	vector<double> intrvAngles;
public:
	AngleContainer();
	~AngleContainer();

	void addStatusAndAngle(int _number,int _status,double _angle);
	void print();
	void cleanStatus();
	void convertToIntervals();
	void writeIntervalsToFits(char* filename, const char* extname);
	void writePositionsToFits(char* filename, const char* extname);
};

void writePositionsToASCIITableFITS(int nrows, char* filename, vector<double> colData, const char *extname);
void writeIntervalsToASCIITableFITS(int nrows, char* filename, vector<int> col1data, vector<int> col2data, vector<double> col3data, const char *extname);
void printerror2( int status);

#endif
