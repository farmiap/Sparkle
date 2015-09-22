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
	int withADCMotor1  = 1;
	int withADCMotor2  = 1;
	
	CommandLogger commandLogger("/home/safonov/SparkleLog/");
	
	while ((c = getopt (argc, argv, "dmnifabvh")) != -1)
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
			withFilterMotor = 0;
			break;
		case 'a':
			withADCMotor1 = 0;
			break;
		case 'b':
			withADCMotor2 = 0;
			break;
		case 'v':
			imageAveragerTest();
			return 1;
			break;
		case 'h':
			cout << "-d work without detector" << endl;
			cout << "-m work without HWP motor" << endl;
			cout << "-n work without HWP actuator" << endl;
			cout << "-i work without Mirror actuator" << endl;
			cout << "-f work without Filter motor" << endl;
			cout << "-a work without ADC motor 1" << endl;
			cout << "-b work without ADC motor 2" << endl;
			cout << "-v image averager test" << endl;
			return 1;
			break;
		default:
			break;
		}

	StandaRotationStage HWPMotor;
	StandaActuator HWPActuator;
	StandaActuator mirrorActuator;
	StandaRotationStage filterMotor;
	StandaRotationStage ADCMotor1;
	StandaRotationStage ADCMotor2;
	
	
	RegimeContainer regimeContainer(withDetector,withHWPMotor,withHWPAct,withMirrorAct,withFilterMotor,withADCMotor1,withADCMotor2,&HWPMotor,&HWPActuator,&mirrorActuator,&filterMotor,&ADCMotor1,&ADCMotor2);

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
