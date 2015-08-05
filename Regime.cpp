#include <iostream>
#include <math.h>
#include <ncurses.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "atmcdLXd.h"
#include <fitsio.h>

#include "Regime.h"
#include "ImageAverager.h"
#include "HWPRotation.h"

#define TEMP_MARGIN 3.0             // maximum stabilized temperature deviation from required

using namespace std;

Regime::Regime()
{
}

Regime::Regime(int _withDetector,int _withHWPMotor,int _withHWPAct,int _withMirrorAct,StandaRotationStage *_HWPMotor,StandaActuator *_HWPActuator,StandaActuator *_mirrorActuator)
{
	withDetector = _withDetector;
	withHWPMotor = _withHWPMotor;
	withHWPAct = _withHWPAct;
	withMirrorAct = _withMirrorAct;
	
	HWPMotor = _HWPMotor;
	HWPActuator = _HWPActuator;
	mirrorActuator = _mirrorActuator;
	
//	doubleParams["platePA"] = 0.0; // P.A. of hor+ direction relatively to camera enclosure (for ADC)
//	intParams["plateMirror"] 0; // is image mirrored? (for ADC)

// general	
	doubleParams["latitude"] = 0.0; // latitude of telescope, deg, positive is north
	doubleParams["longitude"] = 0.0; // latitude of telescope, deg, positive is east
	doubleParams["altitude"] = 0.0; // altitude a.s.l. of telescope, m
	doubleParams["objectRA"] = 0.0; // deg
	doubleParams["objectDec"] = 0.0; // deg
	stringParams["object"] = "none"; // object name
	stringParams["program"] = "none"; // observational program ID
	stringParams["author"] = "nobody"; // author of observational program
	stringParams["telescope"] = "none"; 
	stringParams["instrument"] = "instrument";
	doubleParams["aperture"] = 0.0; // aperture diameter of the telescope, m
	doubleParams["secondary"] = 0.0; // secondary mirror shadow diameter, m

// detector
	intParams["skip"] = 1;
	intParams["numKin"]  = 10;      // kinetic cycles
	intParams["shutter"] = 0;       // 0 - close, 1 - open
	intParamsValues["shutter"]["close"] = 0;
	intParamsValues["shutter"]["open"]  = 1;
	intParams["ft"] = 1;            // frame transfer: 0 - disabled, 1 - enabled
	intParamsValues["ft"]["disable"] = 0;
	intParamsValues["ft"]["enable"] = 1;
	intParams["adc"] = 1;          // A/D channel: 0 - 14-bit, 1 - 16-bit
	intParamsValues["adc"]["14-bit"] = 0;
	intParamsValues["adc"]["16-bit"] = 1;
	intParams["ampl"] = 1;          // amplifier: 0 - EM, 1 - conventional
	intParamsValues["ampl"]["EM"] = 0;
	intParamsValues["ampl"]["em"] = 0;
	intParamsValues["ampl"]["conv"] = 1;
	intParams["horSpeed"] = 0;      // horisontal speed: conv: 0 - 3 MHz; EM 0 - 10 MHz, 1 - 5 MHz, 2 - 3 MHz
	intParamsValues["horSpeed"]["10MHz"] = 0;
	intParamsValues["horSpeed"]["5MHz"] = 1;
	intParamsValues["horSpeed"]["3MHz"] = 2;
	intParams["preamp"] = 0;        // preamplifier
	intParams["vertSpeed"] = 1;     // vertical clocking speed (0 - 0.3, 1 - 0.5, 2 - 0.9, 3 - 1.7, 4 - 3.3) \mu s
	intParams["vertAmpl"] = 3;      // vertical clocking voltage amplitude 0 - 4
	intParams["temp"] = -1.0;	    // temperature
	intParams["EMGain"] = 1;      // EM gain


	intParams["imLeft"] = 1;        // image left side
	intParams["imRight"] = 512;     // image right side
	intParams["imBottom"] = 1;      // image bottom side
	intParams["imTop"] = 512;       // image top side
	intParams["bin"] = 1;		// binning: >=1
	
	doubleParams["exp"] = 0.1;      // exposure

// HWP rotation unit section
	intParams["HWPMode"]=0;          // 0 - not to use HWP, 1 - use HWP in step mode, 2 - use in continious mode
	intParamsValues["HWPMode"]["disable"] = 0;
	intParamsValues["HWPMode"]["step"] = 1;
	intParamsValues["HWPMode"]["cont"] = 2;
	intParams["HWPDir"]=0; 	// HWP direction of rotation with positive speed. Seeing from detector to telescope: 1 - CCW, 0 - CW
	intParamsValues["HWPDir"]["cw"] = 0;
	intParamsValues["HWPDir"]["ccw"] = 1;
	stringParams["HWPDevice"] = "";
	intParams["HWPPairNum"] = 1; // number of pairs in group
	intParams["HWPGroupNum"] = 1; // number of groups
	doubleParams["HWPStep"] = 10; // step of HWP P.A. (degrees)
	doubleParams["HWPStart"] = 0; // HWP P.A. (degrees)
	doubleParams["HWPIntercept"] = 0.0; // engine position when P.A. is zero (steps)
	doubleParams["HWPSlope"] = 0.015; // degrees per engine step
	doubleParams["HWPSpeed"] = 1200.0; // speed (degrees per second)
	doubleParams["HWPPeriod"] = 10.0; // period between swithes (seconds)

	// HWP switching
	stringParams["HWPActuatorDevice"] = "";
	doubleParams["HWPActuatorSpeed"] = 100.0;
	intParams["HWPActuatorPushedPosition"] = 0.0; // position where actuator is pushed (steps)
	doubleParams["HWPSwitchingAngle"] = 0.0; // start position of HWP rotator before performing switch, degrees
	doubleParams["HWPSwitchingMotion"] = 0.0; // motion of HWP rotator before performing switch, degrees 
	doubleParams["HWPSwitchingSpeed"] = 20.0; // speed of motion of HWP rotator performing switch, degrees/sec
	intParams["HWPBand"] = 0; // current HWP band
	
	// moving mirror control (prefocal group)
	stringParams["mirrorDevice"] = "";
	intParams["mirrorSpeed"] = 1000;  // speed of mirror motor, steps/sec
	intParams["mirror"] = 0;  // 0 - retracted, 1 - linear polarizer inserted, 2 - calibration/finder, 3 - auto
	intParamsValues["mirror"]["off"] = MIRROROFF;
	intParamsValues["mirror"]["linpol"] = MIRRORLINPOL;
	intParamsValues["mirror"]["calibr"] = MIRRORFINDER;
	intParamsValues["mirror"]["find"] = MIRRORFINDER;
	intParamsValues["mirror"]["auto"] = MIRRORAUTO;
	intParams["mirrorPosOff"] = 0;  // position of moving mirror when it is out of the beam
	intParams["mirrorPosLinpol"] = 0;  // position of moving mirror when the linear polarizer is in the beam
	intParams["mirrorPosFinder"] = 0;  // position of moving mirror when the finder and calibration source are in the beam
	doubleParams["mirrorBeamTime"] = 20.0; // period of time for which the linear polarizer is inserted into the beam in beginning and end of series (activated by "mirror auto" command)

	stringParams["fitsname"] = ""; 
	stringParams["fitsdir"] = "";
	stringParams["prtaname"] = "";

	pathesCommands["fitsname"] = FITSNAME;
	pathesCommands["fitsdir"] = FITSDIR;
	pathesCommands["prtaname"] = PRTANAME;

	actionCommands["acq"] = ACQUIRE;
	actionCommands["prta"] = RUNTILLABORT;
	actionCommands["prtaf"] = MAXFLUX;
	actionCommands["prtas"] = RTASPOOL;
	actionCommands["tim"] = GETTIMINGS;
	actionCommands["testnc"] = TESTNCURSES;

	commandHintsFill();

	struct timezone tz;
	gettimeofday(&prevRTATime,&tz);
	
	active = FALSE;
}

Regime::~Regime()
{
}

void Regime::setActive(bool flag)
{
	active = flag;
}

int Regime::procCommand(string command)
{
	// cut comments
	size_t percPos = command.find('%');
	if ( percPos != string::npos ) {
		command = command.erase(percPos);
	}

	vector<string> tokens;
	getTokens(command, ' ', &tokens);

	if ( tokens.size() > 2 )
	{
		cout << "wrong parameter command" << endl;
		return 0;
	}

	if ( actionCommands.count(tokens[0]) > 0 )
	{
		if ( tokens.size() == 1 )
		{
			switch ( actionCommands[tokens[0]] )
			{
			case ACQUIRE:
				acquire();
				break;
			case RUNTILLABORT:
				runTillAbort(false,false);
				break;
			case MAXFLUX:
				runTillAbort(true,false);
				break;
			case RTASPOOL:
				runTillAbort(false,true);
				break;
			case GETTIMINGS:
				printTimings();
				break;
			case TESTNCURSES:
				testNCurses();
				break;
			default:
				break;
			}
		}
		else if ( commandHints.count( tokens[0] ) > 0 )
			cout << commandHints[tokens[0]] << endl;
		else
			cout << "error: unknown action command" << endl;
	}
	else if ( intParams.count(tokens[0]) > 0 )
	{
		if ( tokens.size() == 2)  
		{
			if ( is_integer(tokens[1]) )
			{
				int value;
				istringstream ( tokens[1] ) >> value;
				intParams[tokens[0]] = value;
				active = FALSE;
			}
			else if ( intParamsValues[tokens[0]].count(tokens[1]) > 0 )
			{
				intParams[tokens[0]] = intParamsValues[tokens[0]][tokens[1]];
				active = FALSE;
			}
			else
			{
				cout << "alias " << tokens[1] << " for parameter " << tokens[0] << " not found" << endl;
			}
	        }
		else if ( commandHints.count( tokens[0] ) > 0 )
			cout << commandHints[tokens[0]] << endl;
		else
			cout << "error: incorrect setting parameter " << tokens[0] << endl;
	}
	else if ( doubleParams.count(tokens[0]) > 0 )
	{
		if ( tokens.size() == 2) 
		{
			if ( is_double(tokens[1]) )
			{
				double value;
				istringstream ( tokens[1] ) >> value;
				doubleParams[tokens[0]] = value;
				active = FALSE;
			}
			else if ( doubleParamsValues[tokens[0]].count(tokens[1]) > 0 )
			{
				doubleParams[tokens[0]] = doubleParamsValues[tokens[0]][tokens[1]];
				active = FALSE;
			} 
			else 
			{
				cout << "alias " << tokens[1] << " for parameter " << tokens[0] << " not found" << endl;
			}
		}	
		else if ( commandHints.count( tokens[0] ) > 0 )
			cout << commandHints[tokens[0]] << endl;
		else
			cout << "error: incorrect setting parameter " << tokens[0] << endl;
	}
	else if ( stringParams.count(tokens[0]) > 0 )
	{
		if ( tokens.size() == 2)
		{
			stringParams[tokens[0]] = tokens[1];
			switch ( pathesCommands[tokens[0]] )
			{
				case FITSNAME:
					pathes.setFits(tokens[1]);
					return 1;
				case FITSDIR:
					pathes.setDir(tokens[1]);
					return 1;
				case PRTANAME:
					pathes.setRTA(tokens[1]);
					return 1;
				default:
					break;
			}
			active = FALSE;
		}
		else if ( commandHints.count( tokens[0] ) > 0 )
			cout << commandHints[tokens[0]] << endl;
		else
			cout << "error: incorrect setting parameter " << tokens[0] << endl;
	}
	else
		cout << "error: unknown parameter "  << tokens[0] << endl;


	return 1;
}

void Regime::print()
{
	cout << "integer parameters:" << endl;
	for(map<string, int>::iterator it = intParams.begin();it != intParams.end();++it)
	{
		cout << "  " << it->first << ":" << it->second << endl;
	}
	cout << "double parameters:" << endl;
	for(map<string, double>::iterator it = doubleParams.begin();it != doubleParams.end();++it)
	{
		cout << "  " << it->first << ":" << it->second << endl;
	}
	cout << "string parameters:" << endl;
	for(map<string, string>::iterator it = stringParams.begin();it != stringParams.end();++it)
	{
		cout << "  " << it->first << ":" << it->second << endl;
	}
	cout << "pathes:" << endl;
	pathes.print();
}

void Regime::print2()
{
/*
	initscr();
	raw();
	noecho();
	nodelay(stdscr, TRUE);

	move(0,0);

	if ( active )
		printw("----------------------------ACTIVE------------------------------");
	else
		printw("-X-X-X-X-X-X-X-X-X-X-X-X-X-INACTIVE-X-X-X-X-X-X-X-X-X-X-X-X-X-X-X");
	
	move(1,0);
		printw("|       Detector        |");
	move(2,0);
	if ( intParams["shutter"] == 0 )	
		printw("|shutter        close    |");
	else
		printw("|shutter        open     |");
	move(3,0);
		printw("|exposure");
	move(3,7);
		printw("%.2f",);
	
	
	nodelay(stdscr, FALSE);
	endwin();
	*/
}

int Regime::saveToFile(string path,string name)
{
	// Regime doesn't know its name, it is passed from outside
	
	size_t pos = path.rfind("/");
	
	string directory = path.substr(0,pos+1);

	if ( access(directory.c_str(), R_OK | W_OK) != 0 )
	{
		cout << "------------->ERROR: cannot write regime to dir: " << directory << endl;
		return 0;
	}
	
	FILE *f=fopen(path.c_str(),"w");
	
	
	fprintf(f,"%s\n",name.c_str());
	
	for(map<string, int>::iterator it = intParams.begin();it != intParams.end();++it)
	{
		cout << "write parameter %s " << it->first << "all is ok" << endl;
		fprintf(f,"%s %d\n",it->first.c_str(),it->second);
	}
	for(map<string, double>::iterator it = doubleParams.begin();it != doubleParams.end();++it)
	{
                cout << "write parameter %s " << it->first << "all is ok" << endl;
		fprintf(f,"%s %f\n",it->first.c_str(),it->second);
	}
	for(map<string, string>::iterator it = stringParams.begin();it != stringParams.end();++it)
	{
		cout << "write parameter %s " << it->first << "all is ok" << endl;
		fprintf(f,"%s %s\n",it->first.c_str(),it->second.c_str());
	}
	fclose(f);
	return 1;
}


int Regime::validate()
{
	if (!pathes.validate())
	{
		cout << "pathes validation failed" << endl;
		return 0;
	}

	if (( doubleParams["latitude"] < -90.0 ) || ( doubleParams["latitude"] > 90.0 ))
	{
		cout << "latitude validation failed" << endl;
		return 0;
	}

	if (( doubleParams["longitude"] < -180.0 ) || ( doubleParams["longitude"] > 180.0 ))
	{
		cout << "longitude validation failed" << endl;
		return 0;
	}

	if (( doubleParams["altitude"] < 0.0 ) || ( doubleParams["altitude"] > 10000.0 ))
	{
		cout << "altitude validation failed" << endl;
		return 0;
	}

	if (( doubleParams["objectDec"] < -90.0 ) || ( doubleParams["objectDec"] > 90.0 ))
	{
		cout << "object declination validation failed" << endl;
		return 0;
	}

	if (( doubleParams["objectRA"] < 0.0 ) || ( doubleParams["objectRA"] > 360.0 ))
	{
		cout << "object right ascension validation failed" << endl;
	}

	if ((doubleParams["aperture"] < 0) || (doubleParams["aperture"] > 100))
	{
		cout << "aperture diameter validation failed" << endl;
		return 0;
	}
	if ((doubleParams["secondary"] < 0) || (doubleParams["secondary"] > 100))
	{
		cout << "secondary mirror diameter validation failed" << endl;
		return 0;
	}
	

	if ((intParams["skip"] < 1) || (intParams["skip"] > 1000))
	{
		cout << "rta skip validation failed" << endl;
		return 0;
	}


/*
	if ((intParams["skip"]*doubleParams["exp"] < 0.8))
	{
		cout << "Validation failed. To prevent segm. fault during rta period of writing current image shouldn't be too short" << endl;
		return 0;
	}
*/
	if (intParams["numKin"] < 1)
	{
		cout << "number of kinetic cycles validation failed" << endl;
		return 0;
	}

	if (( intParams["shutter"] != 0 ) && ( intParams["shutter"] != 1 ))
	{
		cout << "shutter validation failed" << endl;
		return 0;
	}

	if (( intParams["ft"] != 0 ) && ( intParams["ft"] != 1 ))
	{
		cout << "frame transfer validation failed" << endl;
		return 0;
	}

	if (( intParams["ampl"] != 0 ) && ( intParams["ampl"] != 1 ))
	{
		cout << "amplifier validation failed" << endl;
		return 0;
	}

	if (( intParams["adc"] != 0 ) && ( intParams["adc"] != 1 ))
	{
		cout << "AD channel validation failed" << endl;
		return 0;
	}

	if (( intParams["temp"] > 0 ) || ( intParams["temp"] < -80 ))
	{
		cout << "temperature validation failed" << endl;
		return 0;
	}
	else
	{
		if ( !checkTempInside(intParams["temp"]-TEMP_MARGIN,intParams["temp"]+TEMP_MARGIN) )
		{
			cout << "warning: temperature of sensor is not consistent with setting or not stabilized"  << endl;
			cout << "it will be set during regime application" << endl;
		}
	}

	if ( intParams["adc"] == 1 )
	{
		if ( intParams["horSpeed"] != 0 )
		{
			cout << "horizontal speed validation failed (ADC)" << endl;
			return 0;
		}
	}

	if ( intParams["ampl"] == 1 )
	{
		if ( intParams["horSpeed"] != 0 )
		{
			cout << "horizontal speed validation failed (ampl)" << endl;
			return 0;
		}
	}

	if ( intParams["ampl"] == 0 )
	{
		if ( intParams["temp"]>-50 )
		{
			cout << "EM amplifier cannot be used with indicated temperature, validation failed" << endl;
			return 0;
		}

		if (( intParams["horSpeed"] < 0 ) || ( intParams["horSpeed"] > 2 ))
		{
			cout << "horizontal speed validation failed" << endl;
			return 0;
		}

		if (( intParams["EMGain"] < 1 ) || ( intParams["EMGain"] > 1000 ))
		{
			cout << "EMGain validation failed" << endl;
			return 0;
		}
	}

	if (( intParams["preamp"] < 0 ) || ( intParams["preamp"] > 2 ))
	{
		cout << "preamp validation failed" << endl;
		return 0;
	}

	if (( intParams["vertSpeed"] < 0 ) || ( intParams["vertSpeed"] > 4 ))
	{
		cout << "vertical clocking speed validation failed" << endl;
		return 0;
	}

	if (( intParams["vertAmpl"] < 0 ) || ( intParams["vertAmpl"] > 4 ))
	{
		cout << "vertical clocking amplitude validation failed" << endl;
		return 0;
	}

	if (( intParams["imLeft"] < 1 ) || ( intParams["imLeft"] > 512 ))
	{
		cout << "image left side validation failed" << endl;
		return 0;
	}

	if (( intParams["imRight"] < 1 ) || ( intParams["imRight"] > 512 ))
	{
		cout << "image right side validation failed" << endl;
		return 0;
	}

	if (( intParams["imBottom"] < 1 ) || ( intParams["imBottom"] > 512 ))
	{
		cout << "image bottom side validation failed" << endl;
		return 0;
	}

	if (( intParams["imTop"] < 1 ) || ( intParams["imTop"] > 512 ))
	{
		cout << "image top side validation failed" << endl;
		return 0;
	}

	if ( intParams["imLeft"] >= intParams["imRight"] )
	{
		cout << "image has non-positive width, validation failed" << endl;
		return 0;
	}

	if ( intParams["imBottom"] >= intParams["imTop"] )
	{
		cout << "image has non-positive height, validation failed" << endl;
		return 0;
	}

	if (( intParams["bin"] < 1 ) || ( intParams["bin"] > 64 ))
	{
		cout << "binning validation failed" << endl;
		return 0;
	}


	if (( doubleParams["exp"] < 0.001 ) || ( doubleParams["exp"] > 1000.0 ))
	{
		cout << "exposure time validation failed" << endl;
		return 0;
	}

	if ( doubleParams["HWPSpeed"]/doubleParams["HWPSlope"] > 2000.1 )
	{
		cout << "HWP speed is too high, validation failed" << endl;
		return 0;
	}

	if ( doubleParams["HWPActuatorSpeed"] > 2000.1 )
	{
		cout << "HWP actuator speed is too high, validation failed" << endl;
		return 0;
	}

	if ( intParams["HWPActuatorPushedPosition"] > 0 )
	{
		cout << "HWP actuator pushed position should be negative, validation failed" << endl;
		return 0;
	}

	if ( ( doubleParams["HWPSwitchingSpeed"] < 0 ) || ( doubleParams["HWPSwitchingSpeed"] > 120 ) )
	{
		cout << "HWP switching speed position should be positive, and less than 120 deg/sec, validation failed" << endl;
		return 0;
	}

	if ( ( intParams["HWPBand"] != 0 ) && ( intParams["HWPBand"] != 1 ) && ( intParams["HWPBand"] != 2 ) )
	{
		cout << "HWP band should be 0, 1, 2" << endl;
		return 0;
	}

	if ( ( intParams["mirror"] != 0 ) && ( intParams["mirror"] != 1 ) && ( intParams["mirror"] != 2 ) && ( intParams["mirror"] != 3 ) )
	{
		cout << "Mirror can be: 0 - retracted, 1 - linear polarizer inserted, 2 - calibration/finder inserted, 3 - auto. Validation failed." << endl;
		return 0;
	}

	if ( ( intParams["mirrorPosOff"] > -100 ) || ( intParams["mirrorPosOff"] < -20000 ) )
	{
		cout << "Mirror position off should be between 100 and 20000, validation failed" << endl;
		return 0;
	}
	
	if ( ( intParams["mirrorPosLinpol"] > -100 ) || ( intParams["mirrorPosLinpol"] < -20000 ) )
	{
		cout << "Mirror position linpol should be between 100 and 20000, validation failed" << endl;
		return 0;
	}
	
	if ( ( intParams["mirrorPosFinder"] > -100 ) || ( intParams["mirrorPosFinder"] < -20000 ) )
	{
		cout << "Mirror position finder should be between 100 and 20000, validation failed" << endl;
		return 0;
	}
	

	if ( intParams["mirrorSpeed"] > 2000 )
	{
		cout << "Mirror actuator speed is too high, validation failed" << endl;
		return 0;
	}
	
	cout << "validation successful" << endl;

	return 1;
}

void Regime::commandHintsFill()
{
	commandHints["latitude"] = "latitude of telescope, deg, positive is north";
	commandHints["longitude"] = "longitude of telescope, deg, positive is east";
	commandHints["altitude"] = "altitude a.s.l. of telescope, m";
	commandHints["objectRA"] = "object right ascension, deg";
	commandHints["objectDec"] = "object declinatio, deg";
	commandHints["object"] = "object name";
	commandHints["program"] = "observational program ID";
	commandHints["author"] = "author of observational program";
	commandHints["telescope"] = "telescope"; 
	commandHints["instrument"] = "instrument";
	commandHints["aperture"] = "aperture diameter of the telescope, m";
	commandHints["secondary"] = "secondary mirror shadow diameter, m";
	
	commandHints["skip"] = "RTA mode: period of writing frame to disk: 1 - every frame is written, 2 - every other frame is written, etc";
	commandHints["numKin"]  = "number of kinetic cycles in series";
	commandHints["shutter"] = "shutter: 0 - close, 1 - open";
	commandHints["ft"]      = "frame transfer: 0 - disabled, 1 - enabled";
	commandHints["adc"]    = "AD channel: 0 - 14-bit (faster), 1 - 16-bit (slower)";
	commandHints["ampl"]    = "amplifier: 0 - EM, 1 - conventional";
	commandHints["horSpeed"]= "horisontal speed: conv ampl: 0 - 3 MHz; EM ampl: 0 - 10 MHz, 1 - 5 MHz, 2 - 3 MHz";
	commandHints["preamp"]  = "preamplifier: 0, 1, 2. for values see performance sheet";
	commandHints["vertSpeed"] = "vertical clocking speed (0 - 0.3, 1 - 0.5, 2 - 0.9, 3 - 1.7, 4 - 3.3) mu s";
	commandHints["vertAmpl"]= "vertical clocking voltage amplitude: 0 - 4";

	commandHints["imLeft"]  = "image left side: 1-512";
	commandHints["imRight"] = "image right side: 1-512";
	commandHints["imBottom"]= "image bottom side: 1-512";
	commandHints["imTop"]   = "image top side: 1-512";
	commandHints["bin"]   = "binning: 1-64";
	
	commandHints["EMGain"]  = "EM gain: 1-1000";
	commandHints["temp"]     = "target sensor temperature: -80 - 0C";
	commandHints["exp"]     = "exposure time in seconds";

	commandHints["HWPEnable"]      = "0 - don't use HWP, 1 - step mode, 2 - continous rotation mode.";
	commandHints["HWPDirInv"]   = "HWP positive direction. Seeing from detector to telescope: 1 - CCW, 0 - CW";
	commandHints["HWPDevice"]   = "HWP motor device id (e.g. /dev/ximc/00000367)";
	commandHints["HWPPairNum"]  = "number of pairs in group: >= 1";
	commandHints["HWPGroupNum"] = "number of groups: >= 1";
	commandHints["HWPStep"]     = "step of HWP P.A. (degrees): > 0.0";
	commandHints["HWPStart"]    = "HWP P.A. start (degrees): > 0.0";
	commandHints["HWPIntercept"]= "engine position when P.A. is zero (steps): > 0.0";
	commandHints["HWPSlope"]    = "degrees per engine step: > 0.0";
	commandHints["HWPSpeed"]    = "engine speed, (degrees per second)";
	commandHints["HWPPeriod"]   = "period between swithes (seconds): > 0.0";

	commandHints["HWPActuatorDevice"]   = "HWP actuator device id (e.g. /dev/ximc/00000367)";
	commandHints["HWPActuatorSpeed"]   = "HWP actuator speed (steps/sec) > 0.0";
	commandHints["HWPActuatorPushedPosition"]   = "HWP actuator pushed position (steps) < 0";
	commandHints["HWPSwitchingAngle"] = "start position of HWP rotator before performing switch, degrees";
	commandHints["HWPSwitchingMotion"] = "motion of HWP rotator before performing switch, degrees";
	commandHints["HWPSwitchingSpeed"] = "speed of motion of HWP rotator performing switch, degrees/sec";
	commandHints["HWPBand"] = "Current HWP code 0, 1, 2";
	
	commandHints["acq"]         = "start acquisition";
	commandHints["prta"]        = "start run till abort";
}


int Regime::apply()
{
	if ( validate() == 0 )
	{
		cout << "error: current regime isn't valid" << endl;
		return 0;
	}
	unsigned int status = DRV_SUCCESS;

	if ( withDetector )
	{
		if ( !checkTempInside(intParams["temp"]-TEMP_MARGIN,intParams["temp"]+TEMP_MARGIN) )
		{
			status = (setTemp(intParams["temp"]))?DRV_SUCCESS:DRV_P1INVALID;
		}

		if ( status == DRV_SUCCESS ) cout << "temperature is ok, setting..." << endl;

		int width, height;
		if ( status == DRV_SUCCESS ) status = GetDetector(&width,&height);
		if ( status == DRV_SUCCESS ) status = SetShutter(1,(intParams["shutter"]==1)?1:2,50,50);
		if ( status == DRV_SUCCESS ) status = SetFrameTransferMode(intParams["ft"]);
		if ( status == DRV_SUCCESS ) status = SetTriggerMode(0); // internal trigger
		if ( status == DRV_SUCCESS ) status = SetReadMode(4); // image
		if ( status == DRV_SUCCESS ) status = SetExposureTime((float)doubleParams["exp"]);
		if ( status == DRV_SUCCESS ) status = SetADChannel(intParams["adc"]);
		if ( status == DRV_SUCCESS ) status = SetHSSpeed(intParams["ampl"],intParams["horSpeed"]);
		if ( status == DRV_SUCCESS ) status = SetPreAmpGain(intParams["preamp"]);
		if ( status == DRV_SUCCESS ) status = SetVSSpeed(intParams["vertSpeed"]);
		if ( status == DRV_SUCCESS ) status = SetVSAmplitude(intParams["vertAmpl"]);
		if ( intParams["ampl"] == 0 )
		{
			if ( intParams["EMGain"] < 2 )
			{
				if ( status == DRV_SUCCESS ) status = SetEMGainMode(0);
				if ( status == DRV_SUCCESS ) status = SetEMCCDGain(0);
			}
			else
			{
				if ( status == DRV_SUCCESS ) status = SetEMAdvanced(intParams["EMGain"] > 300);
				if ( status == DRV_SUCCESS ) status = SetEMGainMode(3);
				if ( status == DRV_SUCCESS ) status = SetEMCCDGain(intParams["EMGain"]);
			}
		}
		if ( status == DRV_SUCCESS ) status = SetImage(intParams["bin"],intParams["bin"],intParams["imLeft"],intParams["imRight"],intParams["imBottom"],intParams["imTop"]);
	}

	if ( status == DRV_SUCCESS )
	{
		active = TRUE;
		cout << "detector regime has been set successfully " << status << endl;
	}
	else
	{
		cout << "error: detector regime application failed " << status << endl;
		return 0;
	}

	int HWPRotationStatus = 0;

	if ( withHWPMotor )
	{
		HWPRotationStatus = HWPMotor->initializeStage(stringParams["HWPDevice"],doubleParams["HWPSlope"],doubleParams["HWPIntercept"],intParams["HWPDirInv"],doubleParams["HWPSpeed"]);
	}

	int HWPActuatorStatus = 0;

	if ( withHWPAct && withHWPMotor ) 
	{
		HWPActuatorStatus = HWPActuator->initializeActuator(stringParams["HWPActuatorDevice"],doubleParams["HWPActuatorSpeed"]);
	
		cout << "current band " << HWPBand << " desired band " << intParams["HWPBand"] << endl;
		if ( HWPBand!=intParams["HWPBand"] ) 
		{
			if ( HWPBand == 0 )
			{
				if ( intParams["HWPBand"] == 1 ) switchHWP();
				if ( intParams["HWPBand"] == 2 ) {switchHWP();switchHWP();}
			}
			if ( HWPBand == 1 )
			{
				if ( intParams["HWPBand"] == 2 ) switchHWP();
				if ( intParams["HWPBand"] == 0 ) {switchHWP();switchHWP();}
			}
			if ( HWPBand == 2 )
			{
				if ( intParams["HWPBand"] == 0 ) switchHWP();
				if ( intParams["HWPBand"] == 1 ) {switchHWP();switchHWP();}
			}
			HWPBand = intParams["HWPBand"];
		}
	}
	else
	{
		cout << "HWP band switching is not possible, HWP actuator or/and motor is off" << endl;
	}	

	int mirrorActuatorStatus = 0;
	if ( withMirrorAct ) 
	{
		mirrorActuatorStatus = mirrorActuator->initializeActuator(stringParams["mirrorDevice"],doubleParams["mirrorSpeed"]);

		switch ( intParams["mirror"] )
		{
		case MIRROROFF:
			mirrorActuator->startMoveToPosition(intParams["mirrorPosOff"]);
			break;
		case MIRRORLINPOL:
			mirrorActuator->startMoveToPosition(intParams["mirrorPosLinpol"]);
			break;
		case MIRRORFINDER:
			mirrorActuator->startMoveToPosition(intParams["mirrorPosFinder"]);
			break;
		case MIRRORAUTO:
			mirrorActuator->startMoveToPosition(intParams["mirrorPosOff"]);
			break;
		default :
			cout << "This is not normal. Validation didn't do its work." << endl;
			break;
		}
		
		usleep(500000);
		int isMovingFlag=1;
		int currentPosition;
		while (isMovingFlag)
		{
			mirrorActuator->getPosition(&isMovingFlag,&currentPosition);
			usleep(100000);
		}
	}
	
		
	return HWPRotationStatus;
}

bool Regime::runTillAbort(bool avImg, bool doSpool)
{
	cout << "entering RTA " << endl;
	if ( !active )
	{
		cout << "This regime is not applied, run rapp" << endl;
		return false;
	}

	if ( doubleParams["exp"] > 2.0 )
	{
		cout << "run till abort is possible only if exposure < 2.0 s" << endl;
		return false;
	}

	int status = DRV_SUCCESS;

	float exposure;
	float accum;
	float kinetic;
	
	if ( withDetector)
	{
		if ( status == DRV_SUCCESS ) status = GetAcquisitionTimings(&exposure, &accum, &kinetic);
	}
	int width, height;
	width = floor(((double)intParams["imRight"]-(double)intParams["imLeft"]+1)/(double)intParams["bin"]);
	height = floor(((double)intParams["imTop"]-(double)intParams["imBottom"]+1)/(double)intParams["bin"]);
	long datasize = width*height; // do not divide by binning, segm. fault instead!
	
	vector<int> periods;
	periods.push_back(3);
	periods.push_back(10);
	periods.push_back(20);
	ImageAverager imageAverager = ImageAverager(periods);
	if (avImg)
	{
		imageAverager.initWithDatasize(datasize);
	}
	
	at_32 *data = new at_32[datasize];


	int ch = 'a';
	int counter=0;
	int HWPisMoving;
	int HWPisMovingPrev=0;
	int motionStarted=0;
	
	int acc=0;
	int kin=0;
	
	double HWPAngle;
	
	HWPRotationTrigger HWPTrigger;
	HWPAngleContainer angleContainer;
	if ( withHWPMotor && intParams["HWPEnable"] )
	{
		HWPMotor->startMoveToAngle(doubleParams["HWPStart"]);
		msec_sleep(200.0);
		do
		{
			HWPMotor->getAngle(&HWPisMoving,&HWPAngle);
//		cout << "is moving" << HWPisMoving << "angle" << HWPAngle << endl;
			msec_sleep(50.0);
		} while ( HWPisMoving );
		msec_sleep(1000.0);
		
		if ( intParams["HWPEnable"] == 1 )
		{
			HWPTrigger.setPeriod(doubleParams["HWPPeriod"]);
			HWPTrigger.start();
		}
		else 
		{
			HWPMotor->startContiniousMotion();
		}
	}

	if (withDetector)
	{
		if ( status == DRV_SUCCESS ) status = SetAcquisitionMode(5); // run till abort
		if ( status == DRV_SUCCESS ) status = SetSpool(doSpool,5,(char*)pathes.getSpoolPath(),10); // disable spooling
		if ( status == DRV_SUCCESS ) status = StartAcquisition();
	}
	
	initscr();
	raw();
	noecho();
	nodelay(stdscr, TRUE);
	
	if ( status == DRV_SUCCESS ) {
		move(0,0);
		if ( withDetector )
		{
			printw("Run till abort started (press q or x to interrupt), width = %d, height = %d", width, height);
		}
		else
		{
			printw("Run till abort started (press q or x to interrupt), without detector");
		}
	} else {
		nodelay(stdscr, FALSE);
		endwin();
		cout << "Run till abort failed, status=" << status << endl;
		delete data;
		return false;
	}
	
	while ( 1 ) {
		if (counter>0) ch = getch();
		if ( (ch=='q') || (ch=='x') ) {
			if (( withDetector ) && ( status == DRV_SUCCESS )) status = AbortAcquisition();
			if ( withHWPMotor && intParams["HWPEnable"] ) HWPMotor->stopContiniousMotion();
			break;
		}
		else
		{
			if ( withHWPMotor && intParams["HWPEnable"] ) {
				if ( intParams["HWPEnable"] == 1)
				{
					HWPMotor->getAngle(&HWPisMoving,&HWPAngle);
					int currentStepNumber;
					if ( HWPTrigger.check(&currentStepNumber) )
					{
						move(2,0);
						motionStarted = 1;
						double nextStepValue = getNextStepValue(currentStepNumber,doubleParams["HWPStep"],intParams["HWPPairNum"],intParams["HWPGroupNum"]);
						printw("trigger fired: frame: %d HWP step: %d pair number: %d group number %d",counter,currentStepNumber,(int)ceil(((currentStepNumber%(intParams["HWPPairNum"]*2))+1)/2.0),(int)ceil(currentStepNumber/(intParams["HWPPairNum"]*2.0)));
						if ( !HWPisMoving )
							HWPMotor->startMoveByAngle(nextStepValue);
						else
						{
							move(3,0);
							printw("ATTENTION: HWP stage is skipping steps %d",counter);
						}
					}
					else
					{
						motionStarted = 0;
					}
				}
			}
			if ( withDetector )
			{
				if ( status == DRV_SUCCESS ) status = WaitForAcquisitionTimeOut(4500);
				if ( status == DRV_SUCCESS ) status = GetAcquisitionProgress(&acc,&kin);
				move(6,0);
				printw("acc num %d, kin num %d",acc,kin);
				
				struct timeval currExpTime;
				struct timezone tz;
				gettimeofday(&currExpTime,&tz);
				double deltaTime = (double)(currExpTime.tv_sec - prevExpTime.tv_sec) + 1e-6*(double)(currExpTime.tv_usec - prevExpTime.tv_usec);
				if ( deltaTime > kinetic*1.8 )  {
					move(5,0);
					printw("frames skipping is possible: delta %.2f ms, exp %.2f ms",deltaTime*1e+3,kinetic*1e+3);
				}
				prevExpTime = currExpTime;
				
				if ( avImg )
				{
					if (status==DRV_SUCCESS) status=GetMostRecentImage(data,datasize);
					imageAverager.uploadImage(data);
					move(2,0);
					printw("maximum of running average images: ");
					int linenum = 0;
					for(map<int, double>::iterator it=imageAverager.maximums.begin(); it!=imageAverager.maximums.end(); ++it)
					{
						move(3+linenum,0);
						printw("frames: %d max: %.0f mean: %.0f ",it->first,it->second,imageAverager.means[it->first]);
						linenum++;
					}
				}
				if ( counter%intParams["skip"] == 0)
				{
					struct timeval currRTATime;
					struct timezone tz;
					gettimeofday(&currRTATime,&tz);
					double deltaTime = (double)(currRTATime.tv_sec - prevRTATime.tv_sec) + 1e-6*(double)(currRTATime.tv_usec - prevRTATime.tv_usec);
					if ( deltaTime < 0.7 )  {
						move(1,0);
						printw(" frame no.: %d skipped",counter);
						counter++;
						continue;
					}
					prevRTATime = currRTATime;
					
					if ( ( !avImg ) && (status==DRV_SUCCESS) ) status=GetMostRecentImage(data,datasize);
					if (status==DRV_SUCCESS) doFits(width,height,(char*)pathes.getRTAPath(),data);
					move(1,0);
					printw("Current status: ");
					switch (status) {
					case DRV_SUCCESS: printw("OK"); break;
					case DRV_P1INVALID: printw("invalid pointer"); break;
					case DRV_P2INVALID: printw("array size is incorrect"); break;
					case DRV_NO_NEW_DATA: printw("no new data"); break;
					default: printw("unknown error");
					}
					printw(" frame no.: %d %f",counter,deltaTime);
				}
			}
			else
			{
				if ( withHWPMotor && ( intParams["HWPEnable"]==1 ) )
				{
					move(1,0);
					printw(" frame no.: %d, angle %f",counter,HWPAngle);
					msec_sleep(100.0);
				}
			}
			// Logic: if HWP was moving in the end of previous step, it moved also during current step.
			if ( withHWPMotor && (intParams["HWPEnable"]==1) )
			{
				if (motionStarted)
				{
					angleContainer.addStatusAndAngle(kin,1,HWPAngle);
//					HWPisMovingPrev = 1;
				}
				else
				{
					angleContainer.addStatusAndAngle(kin,(int)(HWPisMoving!=0),HWPAngle);
//					HWPisMovingPrev = HWPisMoving;
				}
			}
			counter++;
		}
	}
	werase(stdscr);
	nodelay(stdscr, FALSE);
	refresh();
	endwin();

	if ( withHWPMotor && (intParams["HWPEnable"]==1) )
	{
		cout << "writing HWP angle data" << endl;
//		angleContainer.cleanStatus();
//		angleContainer.print();
//		if ( intParams["HWPEnable"] == 2 )
//		{
//			angleContainer.writePositionsToFits((char*)pathes.getHWPPosPath());
//		}
//		else
//		{
			angleContainer.convertToIntervals();
			angleContainer.print();
			angleContainer.writeIntervalsToFits((char*)pathes.getIntrvPath());
//		}
	}

	delete data;
	return true;
}

bool Regime::acquire()
{
	if ( !active )
	{
		cout << "This regime is not applied, run rapp" << endl;
		return false;
	}

	if ( !withDetector )
	{
		cout << "This command doesn't work without detector" << endl;
		return false;
	}

	initscr();
	raw();
	noecho();
	werase(stdscr);
	int ch;
	nodelay(stdscr, TRUE);

	unsigned int status = DRV_SUCCESS;

	if ( status == DRV_SUCCESS ) status = SetAcquisitionMode(3); // run till abort
	if ( status == DRV_SUCCESS ) status = SetNumberKinetics(intParams["numKin"]);
	if ( status == DRV_SUCCESS ) status = SetSpool(1,5,(char*)pathes.getSpoolPath(),10); // disable spooling
	if ( status == DRV_SUCCESS ) status = StartAcquisition();

	if ( status == DRV_SUCCESS ) {
		move(0,0);
		printw("Kinetic series started (press q or x to interrupt)");
	} else {
		move(0,0);
		printw("Kinetic series failed");
		nodelay(stdscr, FALSE);
		endwin();
		return false;
	}

	while ( 1 ) {
		ch = getch();
	        if ( (ch=='q') || (ch=='x') ) {
			if ( status == DRV_SUCCESS ) status = AbortAcquisition();
				break;
		}
		else
		{
			usleep(100000);
			int acc,kin;
			if ( status == DRV_SUCCESS ) status = GetAcquisitionProgress(&acc,&kin);
			move(1,0);
			printw("progress: %3.1f%%",100.0*(float)kin/(float)intParams["numKin"]);
			int state;
			if ( status == DRV_SUCCESS ) status = GetStatus(&state);
			if ( state == DRV_IDLE) break;
		}
	}
	werase(stdscr);
	nodelay(stdscr, FALSE);
	refresh();
	endwin();
	
	return true;
}

void Regime::testNCurses()
{
	initscr();
	raw();
	noecho();

	int ch;
	nodelay(stdscr, TRUE);

	move(0,0);
	printw("Testing ncurses functionality, press q or x to stop");

	long val=0;

	while ( 1 ) {
		ch = getch();
		if ( (ch=='q') || (ch=='x') ) break;
		else
		{
			usleep(100000);
			move(1,0);
			printw("progress: %d",val);
			move(2,0);
			printw("progress: %d",val);
			val++;
		}
	}
	werase(stdscr);
	nodelay(stdscr, FALSE);
	refresh();
	endwin();
	usleep(10000);
}

bool Regime::printTimings()
{
	if ( !active )
	{
		cout << "This regime is not applied, run rapp" << endl;
		return false;
	}

	if ( !withDetector )
	{
		cout << "This command doesn't work without detector" << endl;
		return false;
	}

	unsigned int status = DRV_SUCCESS;
	float exposure;
	float accum;
	float kinetic;

	cout << "acquisition timings:" << endl;
	if ( status == DRV_SUCCESS ) status = GetAcquisitionTimings(&exposure, &accum, &kinetic);
	cout << "exposure: " << exposure << " s" << endl;
	cout << "accumulation cycle time: " << accum << " s" << endl;
	cout << "kinetic cycle time: " << kinetic << " s" << endl;

	return ( status == DRV_SUCCESS );
}

int Regime::switchHWP()
{
	HWPMotor->startMoveToAngle(doubleParams["HWPSwitchingAngle"]);

	cout << "Setting initial position ... " << endl;
	
	int isMovingFlag = 0;
	double currentAngle = 0.0;
	int currentPosition = 0;
	while (isMovingFlag)
	{
		HWPMotor->getAngle(&isMovingFlag,&currentAngle);
		usleep(100000);
	}
	cout << "done." << endl;
	usleep(500000);

	HWPActuator->startMoveToPosition(intParams["HWPActuatorPushedPosition"]);
	usleep(500000);
	isMovingFlag = 1;
	while (isMovingFlag)
	{
		HWPActuator->getPosition(&isMovingFlag,&currentPosition);
		usleep(100000);
	}
	cout << "done." << endl;
	
	cout << "Switching ... " << endl;

	HWPMotor->setSpeed(doubleParams["HWPSwitchingSpeed"]);
	HWPMotor->startMoveByAngle(doubleParams["HWPSwitchingMotion"]);
	isMovingFlag = 1;
	while (isMovingFlag)
	{
		HWPMotor->getAngle(&isMovingFlag,&currentAngle);
		usleep(100000);
	}
	cout << "done." << endl;
	HWPMotor->setSpeed(doubleParams["HWPSpeed"]);
	
	cout << "Actuator push back ... " << endl;
	HWPActuator->startMoveToPosition(0);
	usleep(500000);
	isMovingFlag = 1;
	while (isMovingFlag)
	{
		HWPActuator->getPosition(&isMovingFlag,&currentPosition);
		usleep(100000);
	}
	cout << "done." << endl;
	
	
	return 1;
}


bool setTemp(double temper, bool waitForStab)
{


	initscr();
	raw();
	noecho();
	nodelay(stdscr, TRUE);

	bool answer = 0;
	bool stabilized = false;
	int ch;
	float temp;

	move(0,0);
	if (waitForStab) {
		printw("Starting setting temperature, press q or x to interrupt. Will wait for stabilization.");
	} else {
                printw("Starting setting temperature, press q or x to interrupt. Will NOT wait for stabilization");
	}
	move(1,0);
	printw("Target temperature %d", (int)temper);
	unsigned int status = DRV_SUCCESS;
	status = SetTemperature((int)temper);
	if (status == DRV_SUCCESS) status = CoolerON();
	while ( ( !stabilized ) && ( status == DRV_SUCCESS ) )
	{
		usleep(500000);
                ch = getch();
                if ( (ch=='q') || (ch=='x') ) {
			unsigned int state=GetTemperatureF(&temp);
			status = SetTemperature((int)temp); // hold at temperature in the moment of stop
			answer = false;
			break;
		} else {
			unsigned int state=GetTemperatureF(&temp);
			move(2,0);
			printw("Current temperature: %g",temp);
			move(3,0);
			switch (state) {
			case DRV_TEMPERATURE_OFF: printw("Status: Cooler OFF"); break;
			case DRV_TEMPERATURE_STABILIZED:
			{
				printw("Status: Stabilised");
				stabilized = true;
				answer = true;
			}
			break;
			case DRV_TEMPERATURE_NOT_REACHED: printw("Status: Cooling"); break;
			case DRV_TEMPERATURE_DRIFT: printw("Status: Temperature had stabilised but has since drifted"); break;
			case DRV_TEMPERATURE_NOT_STABILIZED:	printw("Status: Temperature reached but not stabilized"); break;
			default: printw("Status: Unknown"); break;
			}
			if ( !waitForStab && (fabs((double)temp-temper)<3.0) )
			{
				stabilized = true;
				answer = true;
			}


		}
	}
	nodelay(stdscr, FALSE);
	werase(stdscr);
	endwin();

	return answer;
}

bool checkTempInside(double lowerLim, double upperLim)
{
	float temp;
	unsigned int state=GetTemperatureF(&temp);
//	if ( temp < lowerLim ) cout << "temperature is too low" << endl;
//	if ( temp > upperLim ) cout << "temperature is too high" << endl;
//	if ( state != DRV_TEMPERATURE_STABILIZED ) cout << "temperature is not stabilized" << endl;
	return (( lowerLim < temp ) && (temp < upperLim) && ( state==DRV_TEMPERATURE_STABILIZED ));
}


bool finalize(int _withDetector,int _withHWPMotor,int _withHWPAct,float startTemp)
{
	int status = DRV_SUCCESS;
	if ( _withDetector )
	{
		if ( status == DRV_SUCCESS ) status = SetShutter(1,2,50,50);
		if ( status != DRV_SUCCESS )
			return false;

		if (!checkTempInside(startTemp-15.0,startTemp+15.0))
			if (!setTemp(startTemp-10.0,false))
				return false; // in case temperature cannot be set, don't quit

		ShutDown();
	}
	return true;
}

void doFits(int nx, int ny, char* filename,at_32 *data)
{
	fitsfile *fptr;       /* pointer to the FITS file, defined in fitsio.h */
	int status, ii, jj;
	long  fpixel, nelements;
	long *array[nx];

	int bitpix   =  FLOAT_IMG; /* 32-bit double pixel values       */
	long naxis    =   2;  /* 2-dimensional image                            */
	long naxes[2] = { nx, ny};   /* image is nx pixels wide by ny rows */

    /* allocate memory for the whole image */
	array[0] = (long *)malloc( naxes[0] * naxes[1] * sizeof(long) );

    /* initialize pointers to the start of each row of the image */
	for( ii=1; ii<naxes[1]; ii++ )
		array[ii] = array[ii-1] + naxes[0];

	remove(filename);               /* Delete old file if it already exists */

	status = 0;         /* initialize status before calling fitsio routines */

	if (fits_create_file(&fptr, filename, &status)) /* create new FITS file */
		printerror( status );           /* call printerror if error occurs */

	if ( fits_create_img(fptr,  bitpix, naxis, naxes, &status) )
		printerror( status );

	for (jj = 0; jj < naxes[1]; jj++){
		for (ii = 0; ii < naxes[0]; ii++) {
			array[jj][ii] = data[ii+naxes[0]*jj];
        	}
	}

	fpixel = 1;                               /* first pixel to write      */
	nelements = naxes[0] * naxes[1];          /* number of pixels to write */

/* write the array of unsigned integers to the FITS file */
	if ( fits_write_img(fptr, TLONG, fpixel, nelements, array[0], &status) )
		printerror( status );

	free( array[0] );  /* free previously allocated memory */

	if ( fits_close_file(fptr, &status) )                /* close the file */
		printerror( status );

//	printf("FITS writed: %s\n",filename);

	return;
}

void printerror( int status)
{
    /*****************************************************/
    /* Print out cfitsio error messages and exit program */
    /*****************************************************/
	if (status)
	{
		fits_report_error(stderr, status); /* print error report */
		exit( status );    /* terminate the program, returning error status */
	}
	return;
}

void getTokens(string input, char delim, vector<string> *tokens)
{
	stringstream ss(input);
	string item;
	while (getline(ss, item, delim)) {
		tokens->push_back(item);
	}
}

bool is_integer(string str)
{
	string::iterator it = str.begin();
	if ((!isdigit(*it))&&(*it=='-')) it++; // skip minus if any
	while (it != str.end() && std::isdigit(*it)) ++it;
	return !str.empty() && it == str.end();
}

bool is_double(string str)
{
	int dotCount = 0;
	string::iterator it = str.begin();
	if ((!isdigit(*it))&&(*it=='-')) it++; // skip minus if any
	while (it != str.end())
	{
		if ( *it=='.' )
			dotCount++;
		else if (!isdigit(*it))
			break;
		it++;
	}
	return !str.empty() && it == str.end() && dotCount <= 1;
}
