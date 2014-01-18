#ifndef REGIME_H
#define REGIME_H

#include <sstream>
#include <string>
#include <vector>
#include <map>
#include "atmcdLXd.h"

#include "Pathes.h"

using namespace std;

enum
{
	FITSNAME = 1,
	FITSDIR,
	RTANAME
};

enum
{
	ACQUIRE = 1,
	RUNTILLABORT,
	GETTIMINGS
};

class Regime
{
private:
	map<string, double> doubleParams;
	map<string, int> intParams;
	map<string, int> actionCommands;
	map<string, string> commandHints;

	Pathes pathes;
	map<string, long> pathesCommands;
	bool active;
public:
	int procCommand(string command);
	int validate();
	int apply();
	bool runTillAbort();
	bool acquire();
	bool printTimings();
	void setActive(bool flag);

	void commandHintsFill();

	void print();

	Regime();
	~Regime();
};

// detector interaction functions which don't require regime framework
bool finalize(float startTemp);
bool checkTempInside(double lowerLim, double upperLim);
bool setTemp(double temper);


// auxiliary functions
void doFits(int nx, int ny, char* filename,at_32 *data);
void printerror( int status);

void getTokens(string input, char delim, vector<string> *tokens);
bool is_integer(string str);
bool is_double(string str);

#endif
