#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "atmcdLXd.h"

#include "RegimeContainer.h"
#include "ImageAverager.h"
#include "StandaRotationStage.h"

using namespace std;

int main(int argc, char* argv[])
{
	cout << "Sparkle: command-line tool to control iXon+897" << endl;

	string command;

	unsigned int error;
	float startTemp;

	int c;
##	int withDetector = 1;
	int withHWPMotor = 1;

	while ((c = getopt (argc, argv, "dmah")) != -1)
		switch (c)
		{
		case 'd':
			withDetector = 0;
			break;
		case 'm':
			withHWPMotor = 0;
			break;
		case 'a':
			imageAveragerTest();
			return 1;
			break;
		case 'h':
			cout << "-d work without detector" << endl;
			cout << "-m work without HWP motor" << endl;
			cout << "-a image averager test" << endl;
			return 1;
			break;
		default:
			break;
		}

	StandaRotationStage HWPMotor;

	RegimeContainer regimeContainer(withDetector,withHWPMotor,&HWPMotor);

	if ( withDetector )
	{
		error = Initialize("/usr/local/etc/andor");

		if ( error != DRV_SUCCESS )
		{
			cout << "!!Error initialising system!!:: " << error << endl;
			return 1;
		}

		error = GetTemperatureF(&startTemp);
		cout << "starting temperature:" << startTemp << endl;
	}
	else
	{
		startTemp = 10.0;
	}

	while ( 1 )
	{
		cout << regimeContainer.currentRegimeName() << "<";

		if (!getline(cin,command)) {
		        cin.clear(); // Sometimes getline behaves strangely after ncurses session, especially in screen. This is workaround.
		        getline(cin,command);
		}

		if ( command.compare("exit") == 0 )
		{
			if ( finalize(withDetector,withHWPMotor,startTemp) )
					break;
		}

		if ( command.compare("") == 0 )
			continue;

		regimeContainer.procCommand(command);
	}

	return 0;
}
