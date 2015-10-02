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
	TESTNCURSES,
	GETOBJECTFROMOCS
};

enum
{
	HWPOFF = 0,
	HWPSTEP,
	HWPCONT
};

enum
{
	MIRROROFF = 0,
	MIRRORLINPOL,
	MIRRORFINDER,
	MIRRORAUTO
};

enum
{
	ADCOFF = 0,
	ADCAUTO,
	ADCMAN,
	ADCSTEP
};

class Regime
{
private:
	map<string, double> 	doubleParams;
	map<string, int> 	intParams;
	map<string, string> 	stringParams;
	map<string, int> 	actionCommands;
	map<string, string> 	commandHints;

	map<string, map<string, double> > 	doubleParamsValues;
	map<string, map<string, int> > 	        intParamsValues;
	
	int withDetector;
	int withHWPMotor;
	int withHWPAct;
	int withMirrorAct;
	int withFilterMotor;
	int withADCMotor1;
	int withADCMotor2;
	
	int HWPBand;
	
	StandaRotationStage *HWPMotor;
	StandaActuator *HWPActuator;
	StandaActuator *mirrorActuator;
	StandaRotationStage *filterMotor;
	StandaRotationStage *ADCMotor1;
	StandaRotationStage *ADCMotor2;
	
	double currentFilterPos;
	string currentFilterName;
	string currentFilterIdent;
	double currentFilterLambda;
	double currentFilterADCcoef;
	
	int detWidth;
	int detHeight;
	
	double ADCprismAngle1,ADCprismAngle2;
	double deroDifference; // Value dero-parallactic. It is constant during given tracking session. Used for ADC calculation.
	double positionAngle; // It is constant during given tracking session.
	
	Pathes pathes;
	map<string, long> pathesCommands;
	bool active;
	
	struct timeval prevRTATime;
	struct timeval prevExpTime;
	
	int switchHWP();

	void printRegimeBlock(string name,int vshift);
	void printRTABlock();
	
	void processImage(at_32* data, at_32* data2, int width, int height, int datasize2, double* xpos, double *ypos, int* satPix, int* subsatPix, double* intensity);
	
	void calculateADC(double* _angle1,double* _angle2);
	double parallacticAngle();
	
	int getObjectFromOCS();
	
	void augmentPrimaryHDU();
	void addAuxiliaryHDU();
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
	void printNeat(string name,int vshift);
	int saveToFile(string path,string name);

	Regime();
	Regime(int _withDetector,int _withHWPMotor,int _withHWPAct,int _withMirrorAct,int _withFilterMotor,int _withADCMotor1,int _withADCMotor2,StandaRotationStage *_HWPMotor,StandaActuator *_HWPActuator,StandaActuator *_mirrorActuator,StandaRotationStage *_filterMotor,StandaRotationStage *_ADCMotor1,StandaRotationStage *_ADCMotor2);
	~Regime();
};

bool finalize(int _withDetector,int _withHWPMotor,int _withHWPAct,int _withMirrorAct,int _withFilterMotor,int _withADCMotor1,int _withADCMotor2,StandaRotationStage *_HWPMotor,StandaActuator *_HWPActuator,StandaActuator *_mirrorActuator,StandaRotationStage *_filterMotor,StandaRotationStage *_ADCMotor1,StandaRotationStage *_ADCMotor2,float startTemp);
// detector interaction functions which don't require regime framework
bool checkTempInside(double lowerLim, double upperLim);
bool setTemp(double temper, bool waitForStab = true);


// auxiliary functions
void doFits(int nx, int ny, char* filename,at_32 *data);
void printerror( int status);

void getTokens(string input, char delim, vector<string> *tokens);
bool is_integer(string str);
bool is_double(string str);
int intCompare(const void * a, const void * b);
int anglesProximity(double a, double b, double margin);

double DecstringToDouble(string inputstring);
double RAstringToDouble(string inputstring);


#endif
