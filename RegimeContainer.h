#ifndef REGIMECONTAINER_H
#define REGIMECONTAINER_H

#include <string>
#include <map>

#include "Regime.h"
#include "StandaRotationStage.h"
#include "StandaActuator.h"

#define DEFAULT_REGIME "default"

using namespace std;

enum
{
	RLIST = 1,
	RNEW,
	RMOD,
	RDEL,
	RPRINT,
	RPRINT2,
	RVAL,
	RLOAD,
	RSAVE,
	RAPP,
	RCOPY
};

class RegimeContainer
{
private:
	map<string, long> regimeCommands;
	map<string, Regime> regimes;
	string currentName;
	StandaRotationStage *HWPMotor;
	StandaActuator *HWPActuator;
	StandaActuator *mirrorActuator;
	StandaRotationStage *filterMotor;
	StandaRotationStage *ADCMotor1;
	StandaRotationStage *ADCMotor2;
	
	int withDetector;
	int withHWPMotor;
	int withHWPAct;
	int withMirrorAct;
	int withFilterMotor;
	int withADCMotor1;
	int withADCMotor2;
	
	void addRegime(string name);
public:
	RegimeContainer();
	RegimeContainer(int _withDetector,int _withHWPMotor,int _withHWPAct,int _withMirrorAct,int _withFilterMotor,int _withADCMotor1,int _withADCMotor2,StandaRotationStage *_HWPMotor,StandaActuator *_HWPActuator,StandaActuator *_mirrorActuator,StandaRotationStage *_filterMotor,StandaRotationStage *_ADCMotor1,StandaRotationStage *_ADCMotor2);
	~RegimeContainer();
	int procCommand(string command);
	string currentRegimeName();
};

#endif
