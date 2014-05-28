#ifndef REGIMECONTAINER_H
#define REGIMECONTAINER_H

#include <string>
#include <map>

#include "Regime.h"
#include "StandaRotationStage.h"

#define DEFAULT_REGIME "default"

using namespace std;

enum
{
	RLIST = 1,
	RNEW,
	RMOD,
	RDEL,
	RPRINT,
	RVAL,
	RLOAD,
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

	int withDetector;
	int withHWPMotor;
	void addRegime(string name);
public:
	RegimeContainer();
	RegimeContainer(int _withDetector,int _withHWPMotor,StandaRotationStage *_HWPMotor);
	~RegimeContainer();
	int procCommand(string command);
	string currentRegimeName();
};

#endif
