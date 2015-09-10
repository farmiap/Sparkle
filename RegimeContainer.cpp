#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>
#include "RegimeContainer.h"
#include "Regime.h"

using namespace std;

RegimeContainer::RegimeContainer()
{

}

RegimeContainer::RegimeContainer(int _withDetector,int _withHWPMotor,int _withHWPAct,int _withMirrorAct,int _withFilterMotor,int _withADCMotor1,int _withADCMotor2,StandaRotationStage *_HWPMotor,StandaActuator *_HWPActuator,StandaActuator *_mirrorActuator,StandaRotationStage *_filterMotor,StandaRotationStage *_ADCMotor1,StandaRotationStage *_ADCMotor2)
{
	regimeCommands["rlist"]  = RLIST;
	regimeCommands["rnew"]   = RNEW;
	regimeCommands["rmod"]   = RMOD;
	regimeCommands["rdel"]   = RDEL;
	regimeCommands["rprint"] = RPRINT;
	regimeCommands["rprint2"]= RPRINT2;
	regimeCommands["rval"]   = RVAL;
	regimeCommands["rapp"]   = RAPP;
	regimeCommands["rload"]  = RLOAD;
	regimeCommands["rsave"]  = RSAVE;
	regimeCommands["rcopy"]  = RCOPY;

	withDetector = _withDetector;
	withHWPMotor = _withHWPMotor;
	withHWPAct = _withHWPAct;
	withMirrorAct = _withMirrorAct;
	withFilterMotor = _withFilterMotor;
	withADCMotor1 = _withADCMotor1;
	withADCMotor2 = _withADCMotor2;
	
	HWPMotor = _HWPMotor;
	HWPActuator = _HWPActuator;
	mirrorActuator = _mirrorActuator;
	filterMotor = _filterMotor;
	ADCMotor1 = _ADCMotor1;
	ADCMotor2 = _ADCMotor2;
	
	regimes[DEFAULT_REGIME] = Regime(withDetector,withHWPMotor,withHWPAct,withMirrorAct,withFilterMotor,withADCMotor1,withADCMotor2,HWPMotor,HWPActuator,mirrorActuator,filterMotor,ADCMotor1,ADCMotor2);
	currentName = DEFAULT_REGIME;
}

RegimeContainer::~RegimeContainer()
{
}

string RegimeContainer::currentRegimeName()
{
	return currentName;
}

int RegimeContainer::procCommand(string command)
{
	int answer = 1;

	if ( command.substr(0,1).compare("r") != 0 )
	{
		// if command doesn't start with 'r', then pass the command to current regime
		answer = regimes[currentName].procCommand(command);
		return answer;
	}

	vector<string> tokens;
	getTokens(command, ' ', &tokens);

	switch ( regimeCommands[tokens[0]] )
	{
		case RLIST:
		{
			cout << "list of regimes:" << endl;
			for(map<string, Regime>::iterator it = regimes.begin();it!=regimes.end();++it)
			{
				cout << it->first << endl;
			}
		}
		break;
		case RNEW:
		{
			if ( tokens.size()!=2 )
			{
				cout << "synt. error: rnew name" << endl;
				break;
			}
			cout << "new regime: " << tokens[1] << endl;
			regimes[tokens[1]] = Regime(withDetector,withHWPMotor,withHWPAct,withMirrorAct,withFilterMotor,withADCMotor1,withADCMotor2,HWPMotor,HWPActuator,mirrorActuator,filterMotor,ADCMotor1,ADCMotor2);
		}
		break;
		case RMOD:
		{
			if ( tokens.size() != 2 )
			{
				cout << "synt. error: rmod name" << endl;
				break;
			}

			if ( regimes.count(tokens[1]) > 0 )
			{
				currentName = tokens[1];
			}
			else
			{
				cout << "there is no regime: " << tokens[1] << endl;
			}
		}
		break;
		case RDEL:
		{
			if ( tokens.size() != 2 )
			{
			cout << "synt. error: rdel name" << endl;
			break;
			}

			if (tokens[1].compare(DEFAULT_REGIME) == 0)
			{
				cout << "default regime cannot be deleted" << endl;
				break;
			}
			cout << "delete regime: " << tokens[1] << endl;

			if ( currentName.compare(tokens[1]) == 0 )
				currentName = DEFAULT_REGIME;

			if ( regimes.erase(tokens[1]) > 0 )
			{
				cout << "deletion successful" << endl;
			}
			else
			{
				cout << "no such regime" << endl;
			}
		}
		break;
		case RPRINT:
		{
			if ( tokens.size() > 2 )
			{
				cout << "synt. error: rprint <name>" << endl;
				break;
			}

			string nameToPrint;
			if ( tokens.size() > 1 )
				nameToPrint = tokens[1];
			else
				nameToPrint = currentName;

			regimes[nameToPrint].printNeat(nameToPrint,0);
		}
		break;
		case RPRINT2:
		{
			if ( tokens.size() > 2 )
			{
				cout << "synt. error: rprint2 <name>" << endl;
				break;
			}
			
			string nameToPrint;
			if ( tokens.size() > 1 )
				nameToPrint = tokens[1];
			else
				nameToPrint = currentName;
			
			cout << "printing parameters of regime:" << nameToPrint << endl;
			
			regimes[nameToPrint].print();
		}
		break;
		case RVAL:
		{
			if ( tokens.size() > 2 )
			{
				cout << "synt. error: rprint <name>" << endl;
				break;
			}
			string name;
			if ( tokens.size() > 1 )
				name = tokens[1];
			else
				name = currentName;

			regimes[name].validate();
		}
		break;
		case RLOAD:
		{
			if ( tokens.size() > 2 )
			{
				cout << "synt. error: rload filename" << endl;
				break;
			}

			struct stat  buffer;
			int st = stat(tokens[1].c_str(), &buffer);
			if ( st != 0 )
			{
				cout << "error: stat check: file doesn't exist" << endl;
				break;
			}

			if ( S_ISDIR(buffer.st_mode) )
			{
				cout << "error: stat check: this is directory!" << endl;
				break;
			}

			ifstream file(tokens[1].c_str(),ios::in);

			if ( !file )
			{
				cout << "error: ifstream check: file doesn't exist" << endl;
				break;
			}

			string str;
			getline(file,str);

			vector<string> ftokens;
			getTokens(str, ' ', &ftokens);

			if ( ftokens.size() > 1 )
			{
				cout << "synt. error: first line should contain only regime name" << endl;
				break;
			}

			string newname = ftokens[0];

			Regime regime(withDetector,withHWPMotor,withHWPAct,withMirrorAct,withFilterMotor,withADCMotor1,withADCMotor2,HWPMotor,HWPActuator,mirrorActuator,filterMotor,ADCMotor1,ADCMotor2);
			while ( getline(file,str) )
				regime.procCommand(str);
			regimes[newname] = regime;

			file.close();
		}
		break;
		case RSAVE:
		{
			if ( tokens.size() > 2 )
			{
				cout << "synt. error: rload filename" << endl;
				break;
			}

			struct stat  buffer;
			int st = stat(tokens[1].c_str(), &buffer);
			if ( S_ISDIR(buffer.st_mode) )
			{
				cout << "error: stat check: this is directory!" << endl;
				break;
			}

			regimes[currentName].saveToFile(tokens[1],currentName);
		}
		break;
		case RCOPY:
		{
			if ( tokens.size()!=2 )
			{
				cout << "synt. error: rcopy name" << endl;
				break;
			}
			cout << "current regime saved as: " << tokens[1] << endl;
			regimes[tokens[1]] = regimes[currentName];
		}
		break;
		case RAPP:
		{
			if ( tokens.size() > 2 )
			{
				cout << "synt. error: rapp <name>" << endl;
				break;
			}
			string name;
			if ( tokens.size() > 1 )
				name = tokens[1];
			else
				name = currentName;

			regimes[name].apply();

			for(map<string, Regime>::iterator it = regimes.begin(); it!=regimes.end(); ++it)
				if ( name.compare(it->first) != 0 )
					it->second.setActive(false);
		}
		break;
		default:
		{
			cout << "unknown regime command" << endl;
			answer = 0;
		}
		break;
	}
	return answer;
}
