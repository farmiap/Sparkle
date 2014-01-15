#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "atmcdLXd.h"

#include "RegimeContainer.h"

using namespace std;

int main(int argc, char* argv[])
{
	cout << "Sparkle: command-line tool to control iXon+897" << endl;

	string command;

	RegimeContainer regimeContainer;

	unsigned int error = Initialize("/usr/local/etc/andor");

	if ( error != DRV_SUCCESS )
	{
		cout << "!!Error initialising system!!:: " << error << endl;
		return 1;
	}

	float startTemp;
	error = GetTemperatureF(&startTemp);
	cout << "starting temperature:" << startTemp << endl;

	while ( 1 )
	{
		cout << regimeContainer.currentRegimeName() << "<";
		getline(cin, command);

		if ( command.compare("exit") == 0 )
			if ( finalize(startTemp) )
				break;

		if ( command.compare("") == 0 )
			continue;

		regimeContainer.procCommand(command);
	}

	return 0;
}
