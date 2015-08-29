#ifndef MIRRORMOTION_H
#define MIRRORMOTION_H

#include <sys/time.h>
#include <iostream>
#include <vector>

#include "StandaActuator.h"

using namespace std;

enum
{
	MIRRORMOVINGON = 1,
	MIRRORISON,
	MIRRORMOVINGOFF,
	MIRRORISOFF,
};

class MirrorMotionRTA
{
private:
	StandaActuator *mirrorActuator;
	int mirrorPosOff,mirrorPosLinpol;
	
	double beamTime;
	int isOnStart,isOnFinish;
	int isMovingOn,isMovingOff;
	int currIntrv;
	
	struct timeval startTime;
	struct timeval finishTime;
	
	vector< vector<int> > intervals;
public:
	MirrorMotionRTA(StandaActuator *_mirrorActuator,int _mirrorPosOff,int _mirrorPosLinpol, double _beamTime);
	~MirrorMotionRTA();
	
	int process(int count, int exitRequested, int* mirrorStatus);
	void print();
	void writeIntervalsToFits(char* filename);
};

void writeMirrorIntervalsToASCIITableFITS(char* filename, vector<vector<int> > data);
void printerror4(int status);

#endif
