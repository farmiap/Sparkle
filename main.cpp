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
#include "CommandLogger.h"

using namespace std;

int main(int argc, char* argv[])
{
	cout << "Sparkle: command-line tool to control iXon+897" << endl;

	string command;

	unsigned int error;
	float startTemp;

	int c;
	int withDetector  = 1;
	int withHWPMotor  = 1;
	int withHWPAct    = 1;
	int withMirrorAct = 1;
	int withFilterMotor  = 1;
	
	CommandLogger commandLogger("/home/safonov/SparkleLog/");
	
	while ((c = getopt (argc, argv, "dmnifah")) != -1)
		switch (c)
		{
		case 'd':
			withDetector = 0;
			break;
		case 'm':
			withHWPMotor = 0;
			break;
		case 'n':
			withHWPAct = 0;
			break;
		case 'i':
			withMirrorAct = 0;
			break;
		case 'f':
			withMirrorAct = 0;
			break;
		case 'a':
			imageAveragerTest();
			return 1;
			break;
		case 'h':
			cout << "-d work without detector" << endl;
			cout << "-m work without HWP motor" << endl;
			cout << "-n work without HWP actuator" << endl;
			cout << "-i work without Mirror actuator" << endl;
			cout << "-f work without Filter motor" << endl;
			cout << "-a image averager test" << endl;
			return 1;
			break;
		default:
			break;
		}

	StandaRotationStage HWPMotor;
	StandaActuator HWPActuator;
	StandaActuator mirrorActuator;
	StandaRotationStage filterMotor;
	
	RegimeContainer regimeContainer(withDetector,withHWPMotor,withHWPAct,withMirrorAct,withFilterMotor,&HWPMotor,&HWPActuator,&mirrorActuator,&filterMotor);

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
		
		if ( command.compare("") == 0 )
			continue;

		commandLogger.log(regimeContainer.currentRegimeName().c_str(),command.c_str());
		
		if ( command.compare("exit") == 0 )
		{
			if ( finalize(withDetector,withHWPMotor,withHWPAct,startTemp) )
					break;
		}

		regimeContainer.procCommand(command);
	}

	return 0;
}
