#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "atmcdLXd.h"

#include "RegimeContainer.h"
#include "ImageAverager.h"

using namespace std;

int main(int argc, char* argv[])
{
	cout << "Sparkle: command-line tool to control iXon+897" << endl;

	string command;

	RegimeContainer regimeContainer;

	unsigned int error;
	float startTemp;

	int c;
	int withoutDevice;

	while ((c = getopt (argc, argv, "awh")) != -1)
		switch (c)
		{
		case 'w':
			withoutDevice = 1;
			break;
		case 'a':
			imageAveragerTest();
			return 1;
			break;
		case 'h':
			cout << "-w start without detector initialization" << endl;
			cout << "-a image averager test" << endl;
			return 1;
			break;
		default:
			break;
		}

	if ( withoutDevice == 0 )
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
		getline(cin, command);

		if ( command.compare("exit") == 0 )
			if ( withoutDevice == 0 )
			{
				if ( finalize(startTemp) )
					break;
			}
			else
			{
				break;
			}

		if ( command.compare("") == 0 )
			continue;

		regimeContainer.procCommand(command);
	}

	return 0;
}
