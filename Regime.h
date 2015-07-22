#ifndef REGIME_H
#define REGIME_H

#include <sys/time.h>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include "atmcdLXd.h"

#include "Pathes.h"
#include "StandaRotationStage.h"
#include "StandaActuator.h"

using namespace std;

enum
{
	FITSNAME = 1,
	FITSDIR,
	PRTANAME
};

enum
{
	ACQUIRE = 1,
	RUNTILLABORT,
	GETTIMINGS,
	MAXFLUX,
	RTASPOOL,
	TESTNCURSES
};

class Regime
{
private:
	map<string, double> 	doubleParams;
	map<string, int> 	intParams;
	map<string, string> 	stringParams;
	map<string, int> 	actionCommands;
	map<string, string> 	commandHints;

	int withDetector;
	int withHWPMotor;

	int HWPBand;
	
	StandaRotationStage *HWPMotor;
	StandaActuator *HWPActuator;
	
	Pathes pathes;
	map<string, long> pathesCommands;
	bool active;
	
	struct timeval prevRTATime;
	struct timeval prevExpTime;
	
	int switchHWP();
public:
	int procCommand(string command);
	int validate();
	int apply();
	bool runTillAbort(bool avImg, bool doSpool);
	bool acquire();
	void testNCurses();
	bool printTimings();
	void setActive(bool flag);

	void commandHintsFill();

	void print();

	Regime();
	Regime(int _withDetector,int _withHWPMotor,StandaRotationStage *_HWPMotor, StandaActuator *_HWPActuator);
	~Regime();
};

// detector interaction functions which don't require regime framework
bool finalize(int _withDetector,int _withHWPMotor,float startTemp);
bool checkTempInside(double lowerLim, double upperLim);
bool setTemp(double temper, bool waitForStab = true);


// auxiliary functions
void doFits(int nx, int ny, char* filename,at_32 *data);
void printerror( int status);

void getTokens(string input, char delim, vector<string> *tokens);
bool is_integer(string str);
bool is_double(string str);

#endif
