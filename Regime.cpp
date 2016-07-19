#include <iostream>
#include <math.h>
#include <ncurses.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <exception>
#include "atmcdLXd.h"
//#include <fitsio.h>
#include "/home/safonov/cfitsio/include/fitsio.h"
//#include "/home/safonov/cfitsio/include/fitsio.h"

#include <vector>
#include <iterator>
#include <fstream>

#include <libnova/transform.h>
#include <libnova/julian_day.h>
#include <libnova/utility.h>
#include <libnova/sidereal_time.h>

#include "Regime.h"
#include "ImageAverager.h"
#include "StepRotation.h"
#include "MirrorMotion.h"

#define TEMP_MARGIN 3.0             // maximum stabilized temperature deviation from required

#define SAT14BIT 15500
#define SUBSAT14BIT 14000
#define SAT16BIT 65000
#define SUBSAT16BIT 60000
#define IMPROCRED 10
#define RAD 57.2957795131
#define ADCMARGIN 0.1
#define MOTIONTIMEOUT 30

using namespace std;

Regime::Regime()
{
}

Regime::Regime(int _withDetector,int _withHWPMotor,int _withHWPAct,int _withMirrorAct,int _withFilterMotor,int _withADCMotor1,int _withADCMotor2,StandaRotationStage *_HWPMotor,StandaActuator *_HWPActuator,StandaActuator *_mirrorActuator,StandaRotationStage *_filterMotor,StandaRotationStage *_ADCMotor1,StandaRotationStage *_ADCMotor2)
{
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
	
	//	intParams["plateMirror"] 0; // is image mirrored? (for ADC)20160409192346.fits

// general	
	doubleParams["latitude"] = 0.0; // latitude of telescope, deg, positive is north
	doubleParams["longitude"] = 0.0; // latitude of telescope, deg, positive is east
	doubleParams["observAltitude"] = 0.0; // altitude a.s.l. of telescope, m
	doubleParams["RA"] = 0.0; // deg
	doubleParams["dec"] = 0.0; // deg
	doubleParams["az"] = 0.0; // deg
	doubleParams["alt"] = 0.0; // deg
	intParams["tracking"] = 1; // 1 if telescope tracks, 0 otherwise
	stringParams["object"] = "none"; // object name
	stringParams["program"] = "none"; // observational program ID
	stringParams["author"] = "nobody"; // author of observational program
	stringParams["telescope"] = "none"; 
	stringParams["instrument"] = "instrument";
	doubleParams["aperture"] = 0.0; // aperture diameter of the telescope, m
	doubleParams["secondary"] = 0.0; // secondary mirror shadow diameter, m
	stringParams["focusStation"] = "none"; // N1, N2, C1 etc.
	doubleParams["referencePA"] = 0.0; // for definition see ADCreport.pdf
	intParams["plateMirror"] = 0; // is image mirrored? (for ADC)
	doubleParams["pixelSize"] = 1.5e-5; // pixel size
	
// detector
	intParams["skip"] = 1;
	intParams["numKin"]  = 10;      // kinetic cycles
	intParams["limitRTA"] = 100000; // limits length of RTA series
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
	intParams["tempStab"] = 1;	    // temperature
	
	intParams["numPol"]  = 10;      // number of series in step polarimetry mode
	
	intParams["imLeft"] = 1;        // image left side
	intParams["imWidth"] = 512;     // image width
	intParams["imBottom"] = 1;      // image bottom side
	intParams["imHeight"] = 512;    // image height
	intParams["bin"] = 1;		// binning: >=1
	
	doubleParams["exp"] = 0.1;      // exposure

// HWP rotation unit section
	intParams["HWPMode"]=0;          // 0 - not to use HWP, 1 - use HWP in step mode, 2 - use in continious mode
	intParamsValues["HWPMode"]["disable"] = HWPOFF;
	intParamsValues["HWPMode"]["off"] = HWPOFF;
	intParamsValues["HWPMode"]["step"] = HWPSTEP;
	intParamsValues["HWPMode"]["cont"] = HWPCONT;
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
	intParams["HWPActuatorPushedPosition1"] = 0.0; // position where actuator is pushed, motion #1 (steps)
	intParams["HWPActuatorPushedPosition2"] = 0.0; // position where actuator is pushed, motion #2 (steps)
	doubleParams["HWPSwitchingAngle"] = 0.0; // start position of HWP rotator before performing switch, degrees
	doubleParams["HWPSwitchingPosition1"] = 184.0; // motion of HWP rotator before performing switch, motion #1, degrees 
	doubleParams["HWPSwitchingPosition2"] = 210.0; // motion of HWP rotator before performing switch, motion #1, degrees 
	doubleParams["HWPSwitchingPosition3"] = 130.0; // motion of HWP rotator before performing switch, motion #1, degrees 
	
	doubleParams["HWPSwitchingMotion1"] = 0.0; // motion of HWP rotator before performing switch, motion #1, degrees 
	doubleParams["HWPSwitchingMotion2"] = 0.0; // motion of HWP rotator before performing switch, motion #2, degrees 
	doubleParams["HWPSwitchingSpeed"] = 20.0; // speed of motion of HWP rotator performing switch, degrees/sec
	intParams["HWPBand"] = 0; // current HWP band
	
	// moving mirror control (prefocal group)
	stringParams["mirrorDevice"] = "";
	intParams["mirrorSpeed"] = 1000;  // speed of mirror motor, steps/sec
	intParams["mirrorMode"] = 0;  // 0 - retracted, 1 - linear polarizer inserted, 2 - calibration/finder, 3 - auto
	intParamsValues["mirrorMode"]["off"] = MIRROROFF;
	intParamsValues["mirrorMode"]["linpol"] = MIRRORLINPOL;
	intParamsValues["mirrorMode"]["calibr"] = MIRRORFINDER;
	intParamsValues["mirrorMode"]["find"] = MIRRORFINDER;
	intParamsValues["mirrorMode"]["auto"] = MIRRORAUTO;
	intParams["mirrorPosOff"] = 0;  // position of moving mirror when it is out of the beam
	intParams["mirrorPosLinpol"] = 0;  // position of moving mirror when the linear polarizer is in the beam
	intParams["mirrorPosFinder"] = 0;  // position of moving mirror when the finder and calibration source are in the beam
	doubleParams["mirrorBeamTime"] = 20.0; // period of time for which the linear polarizer is inserted into the beam in beginning and end of series (activated by "mirror auto" command)

	intParams["light"] = 0; // calibration light. 0 - off, 1 - on
	intParamsValues["light"]["off"] = 0;
	intParamsValues["light"]["on"] = 1;
	
	// filter wheel
	stringParams["filter"] = ""; // Filter short name, e.g. V, B, etc.. These are configurable, e.g. filter0Name
	stringParams["filterDevice"] = ""; 
	doubleParams["filterSlope"] = 0.0; // degrees per engine step
	doubleParams["filterIntercept"] = 0.0; 
	intParams["filterDir"] = 1; // Switch rotation direction, positive should be CCW seeing from detector to telescope. Check by eye.
	doubleParams["filterSpeed"] = 30.0; // speed (degrees per second)
	doubleParams["filter0Pos"] = 0.0; // positions of filter wheel corresponding to different filters
	doubleParams["filter1Pos"] = 0.0;
	doubleParams["filter2Pos"] = 0.0;
	doubleParams["filter3Pos"] = 0.0;
	doubleParams["filter4Pos"] = 0.0;
	doubleParams["filter5Pos"] = 0.0;
	doubleParams["filter6Pos"] = 0.0;
	doubleParams["filter7Pos"] = 0.0;
	stringParams["filter0Name"] = ""; // short names of filters
	stringParams["filter1Name"] = "";
	stringParams["filter2Name"] = "";
	stringParams["filter3Name"] = "";
	stringParams["filter4Name"] = "";
	stringParams["filter5Name"] = "";
	stringParams["filter6Name"] = "";
	stringParams["filter7Name"] = "";
	stringParams["filter0Ident"] = ""; // unique identifiers of filters (http://lnfm1.sai.msu.ru/kgo/local/tech/2.5m/equip/filters/KGO_FILTER_DATA.html)
	stringParams["filter1Ident"] = "";
	stringParams["filter2Ident"] = "";
	stringParams["filter3Ident"] = "";
	stringParams["filter4Ident"] = "";
	stringParams["filter5Ident"] = "";
	stringParams["filter6Ident"] = "";
	stringParams["filter7Ident"] = "";
	doubleParams["filter0Lambda"] = 0.0; // central wavelenghts of filters
	doubleParams["filter1Lambda"] = 0.0;
	doubleParams["filter2Lambda"] = 0.0;
	doubleParams["filter3Lambda"] = 0.0;
	doubleParams["filter4Lambda"] = 0.0;
	doubleParams["filter5Lambda"] = 0.0;
	doubleParams["filter6Lambda"] = 0.0;
	doubleParams["filter7Lambda"] = 0.0;
	
	// synchro schemes are used for polarimetry with rotating HWP
	stringParams["synchro"] = ""; // Name of synchro scheme. Configurable, e.g. synchro0Name.
	stringParams["synchro0Name"] = "";
	stringParams["synchro1Name"] = "";
	stringParams["synchro2Name"] = "";
	stringParams["synchro3Name"] = "";
	doubleParams["synchro0Speed"] = 0.0; // speed of HWP
	doubleParams["synchro1Speed"] = 0.0;
	doubleParams["synchro2Speed"] = 0.0;
	doubleParams["synchro3Speed"] = 0.0;
	doubleParams["synchro0Exp"] = 0.0; // exposure
	doubleParams["synchro1Exp"] = 0.0;
	doubleParams["synchro2Exp"] = 0.0;
	doubleParams["synchro3Exp"] = 0.0;
	intParams["synchro0Height"] = 0.0; // subframe height
	intParams["synchro1Height"] = 0.0;
	intParams["synchro2Height"] = 0.0;
	intParams["synchro3Height"] = 0.0;

	// setting this to 1 leads to unconditional homing of corresponding drive
	intParams["ADCMotor1ForcedHome"] = 0;
	intParams["ADCMotor2ForcedHome"] = 0;
	intParams["HWPForcedHome"] = 0;
	intParams["HWPActuatorForcedHome"] = 0;
	intParams["filterForcedHome"] = 0;
	intParams["mirrorForcedHome"] = 0;
	
	intParams["winLeft"] = 1;
	intParams["winRight"] = 512;
	intParams["winBottom"] = 1;
	intParams["winTop"] = 512;
	
	intParams["ADCMode"] = 0; 
	intParamsValues["ADCMode"]["off"] = ADCOFF;
	intParamsValues["ADCMode"]["man"]  = ADCMAN;
	intParamsValues["ADCMode"]["step"]  = ADCSTEP;
	intParamsValues["ADCMode"]["auto"]  = ADCAUTO;
	doubleParams["ADCMotor1Start"] = 0.0;
	stringParams["ADCMotor1Device"] = ""; 
	doubleParams["ADCMotor1Slope"] = 0.0; // degrees per engine step
	doubleParams["ADCMotor1Intercept"] = 0.0; 
	intParams["ADCMotor1Dir"] = 1; // HWP direction of rotation with positive speed. Check by eye.
	doubleParams["ADCMotor1Speed"] = 30.0; // speed (degrees per second)
	doubleParams["ADCMotor2Start"] = 0.0;
	stringParams["ADCMotor2Device"] = ""; 
	doubleParams["ADCMotor2Slope"] = 0.0; // degrees per engine step
	doubleParams["ADCMotor2Intercept"] = 0.0; 
	intParams["ADCMotor2Dir"] = 1; // HWP direction of rotation with positive speed. Check by eye.
	doubleParams["ADCMotor2Speed"] = 30.0; // speed (degrees per second)
	doubleParams["ADCStep"] = 10.0;
	doubleParams["ADCPeriod"] = 10.0;

	doubleParams["dero"] = 0.0;
	
	doubleParams["filter0ADCcoef"] = 0.0;
	doubleParams["filter1ADCcoef"] = 0.0;
	doubleParams["filter2ADCcoef"] = 0.0;
	doubleParams["filter3ADCcoef"] = 0.0;
	doubleParams["filter4ADCcoef"] = 0.0;
	doubleParams["filter5ADCcoef"] = 0.0;
	doubleParams["filter6ADCcoef"] = 0.0;
	doubleParams["filter7ADCcoef"] = 0.0;
	
	stringParams["OCSIP"] = "192.168.15.12";
	intParams["OCSport"] = 5005;
	
	stringParams["fitsname"] = ""; 
	stringParams["fitsdir"] = "";
	stringParams["prtaname"] = "";
	stringParams["cfgfile"] = "";

	pathesCommands["fitsname"] = FITSNAME;
	pathesCommands["fitsdir"] = FITSDIR;
	pathesCommands["prtaname"] = PRTANAME;

	actionCommands["acq"] = ACQUIRE;
	actionCommands["acqp"] = ACQUIREPOL;
	actionCommands["prta"] = RUNTILLABORT;
	actionCommands["prtaf"] = MAXFLUX;
	actionCommands["prtas"] = RTASPOOL;
	actionCommands["tim"] = GETTIMINGS;
	actionCommands["testnc"] = TESTNCURSES;
	actionCommands["getobj"] = GETOBJECTFROMOCS;
	actionCommands["loadcfg"] = LOADCFG;

	commandHintsFill();

	ADCprismAngle1 = -1.0;
	ADCprismAngle2 = -1.0;
	deroDifference = 0.0;
	positionAngle = 0.0;

	cAccum = -1.0;
	cKinetic = -1.0;
	
	HWPBand = 0;
	
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
			case ACQUIREPOL:
				acquirePol();
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
			case GETOBJECTFROMOCS:
				getObjectFromOCS();
				break;
			case LOADCFG:
				loadCFGfile();
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
			if ( tokens[0].compare("dero") == 0 )
			{
				// Here we calculate the dero-parallactic value for a given moment. Telescope keep this value fixed during tracking.
				// This allows to use it for dero angle calculation for any moment.
				double deroAngle;
				istringstream ( tokens[1] ) >> deroAngle;
				deroDifference = deroAngle - parallacticAngle();
				positionAngle = deroAngle - parallacticAngle() + doubleParams["referencePA"];
			}
			else
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
			else if ( tokens[0].compare("RA") == 0 )
			{
				doubleParams["RA"] = RAstringToDouble(tokens[1]);
				active = FALSE;
			}
			else if ( tokens[0].compare("dec") == 0 )
			{
				doubleParams["dec"] = DecstringToDouble(tokens[1]);
				active = FALSE;
			}
			else
			{
				cout << "alias " << tokens[1] << " for parameter " << tokens[0] << " not found" << endl;
			}
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
			active = FALSE;
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

void Regime::printNeat(string name, int vshift)
{
	initscr();
	raw();
	noecho();
	nodelay(stdscr, TRUE);
	clear();
	
	printRegimeBlock(name,vshift);
	
	char ch;
	
	while ( 1 ) {
		ch = getch();
		if ( (ch=='q') || (ch=='x') ) 
			break;
		msec_sleep(200);
	}	
	
	nodelay(stdscr, FALSE);
	endwin();
	
}

void Regime::printRegimeBlock(string name, int vshift)
{
	int col1name =  1;
	int col1val  = 12;
	int col2name = 27;
	int col2val  = 38;
	int col3name = 53;
	int col3val  = 64;
	int lowname  =  1;
	int lowval   = 12;
	
	int line = 0 + vshift;
	
	move(line,0);
	if ( active )
		printw("*-----------REGIME:------------------------------APPLIED----------------------*");
	else
		printw("*-X-X-X-X-X-REGIME--X-X-X-X-X-X-X-X-X-X-X-X-X--NOT APPLIED--X-X-X-X-X-X-X-X-X-*");
	move(line,18);printw(" %s ",name.c_str());
	line++;
	move(line,0);printw("+---------General---------+---------Detector--------+---------Optomech--------+");line++;
	move(line,0);printw("|                         |                         |                         |");line++;
	move(line,0);printw("|                         |                         |                         |");line++;
	move(line,0);printw("|                         |                         |                         |");line++;
	move(line,0);printw("|                         |                         |                         |");line++;
	move(line,0);printw("|                         |                         |                         |");line++;
	move(line,0);printw("+---------Detector--------|                         |                         |");line++;
	move(line,0);printw("|                         |                         |                         |");line++;
	move(line,0);printw("|                         |                         |                         |");line++;
	move(line,0);printw("|                         |                         |                         |");line++;
	move(line,0);printw("|                         |                         |                         |");line++;
	move(line,0);printw("+-------------------------+-------------------------+-------------------------+");line++;
	move(line,0);printw("|                                                                             |");line++;
	move(line,0);printw("|                                                                             |");line++;
	move(line,0);printw("+-----------------------------------------------------------------------------+");line++;
	move(line,0);printw("+-----------------------------to exit press q or x----------------------------+");line++;
	
	line = 2 + vshift;
	move(line,col1name);printw("object");
	move(line,col1val); printw("%s",stringParams["object"].substr(0,16).c_str());line++;
	move(line,col1name);printw("program");
	move(line,col1val); printw("%s",stringParams["program"].substr(0,16).c_str());line++;
	move(line,col1name);printw("author");
	move(line,col1val); printw("%s",stringParams["author"].substr(0,16).c_str());line++;
	if ( intParams["tracking"] == 1 ) {
		move(line,col1name);printw("RA");
		move(line,col1val); printw("%.4f d",doubleParams["RA"]);line++;
		move(line,col1name);printw("dec");
		move(line,col1val); printw("%+.4f d",doubleParams["dec"]);line++;
	} else {
		move(line,col1name);printw("az");
		move(line,col1val); printw("%.4f d",doubleParams["az"]);line++;
		move(line,col1name);printw("alt");
		move(line,col1val); printw("%+.4f d",doubleParams["alt"]);line++;
	}
	line++;
	string shutterString;
	for(map<string, int>::iterator it = intParamsValues["shutter"].begin();it != intParamsValues["shutter"].end();++it)
		if ( it->second == intParams["shutter"] )
			shutterString = it->first;
		move(line,col1name);printw("shutter");
	move(line,col1val); printw("%s",shutterString.substr(0,16).c_str());line++;
	move(line,col1name);printw("exp.set");
	move(line,col1val); printw("%.5f s",doubleParams["exp"]);line++;
	
	if ( withDetector )
	{
		int status = DRV_SUCCESS;
		
		float exposure;
		
		if ( status == DRV_SUCCESS ) status = GetAcquisitionTimings(&exposure, &cAccum, &cKinetic);
		move(line,col1name);printw("exp.act");
		move(line,col1val); printw("%.5f s",exposure);line++;
		move(line,col1name);printw("cycle");
		move(line,col1val); printw("%.5f s",cKinetic);line++;
	}
	
	
	line = 2 + vshift;
	move(line,col2name);printw("left");
	move(line,col2val); printw("%d pix",intParams["imLeft"]);line++;
	move(line,col2name);printw("bottom");
	move(line,col2val); printw("%d pix",intParams["imBottom"]);line++;
	move(line,col2name);printw("width");
	move(line,col2val); printw("%d pix",intParams["imWidth"]);line++;
	move(line,col2name);printw("height");
	move(line,col2val); printw("%d pix",intParams["imHeight"]);line++;
	move(line,col2name);printw("binning");
	move(line,col2val); printw("%d pix",intParams["bin"]);line++;
	string ftModeString;
	for(map<string, int>::iterator it = intParamsValues["ft"].begin();it != intParamsValues["ft"].end();++it)
		if ( it->second == intParams["ft"] )
			ftModeString = it->first;
		move(line,col2name);printw("ft");
	move(line,col2val); printw("%s",ftModeString.substr(0,16).c_str());line++;
	move(line,col2name);printw("EMGain");
	move(line,col2val); printw("%d",intParams["EMGain"]);line++;
	string amplModeString;
	for(map<string, int>::iterator it = intParamsValues["ampl"].begin();it != intParamsValues["ampl"].end();++it)
		if ( it->second == intParams["ampl"] )
			amplModeString = it->first;
		move(line,col2name);printw("amplifier");
	move(line,col2val); printw("%s",amplModeString.substr(0,16).c_str());line++;
	string adconvModeString;
	for(map<string, int>::iterator it = intParamsValues["adc"].begin();it != intParamsValues["adc"].end();++it)
		if ( it->second == intParams["adc"] )
			adconvModeString = it->first;
		move(line,col2name);printw("A/D conv.");
	move(line,col2val); printw("%s",adconvModeString.substr(0,16).c_str());line++;
	string horSpeedModeString;
	for(map<string, int>::iterator it = intParamsValues["horSpeed"].begin();it != intParamsValues["horSpeed"].end();++it)
		if ( it->second == intParams["horSpeed"] )
			horSpeedModeString = it->first;
		move(line,col2name);printw("readout");
	move(line,col2val); printw("%s",horSpeedModeString.substr(0,16).c_str());line++;
	
	line = 2 + vshift;
	
	move(line,col3name);printw("filter");
	move(line,col3val);printw("%s",currentFilterName.c_str());line++;
	
	string ADCModeString;
	for(map<string, int>::iterator it = intParamsValues["ADCMode"].begin();it != intParamsValues["ADCMode"].end();++it)
		if ( it->second == intParams["ADCMode"] )
			ADCModeString = it->first;
		move(line,col3name);printw("ADCMode");
	move(line,col3val); printw("%s",ADCModeString.substr(0,16).c_str());line++;
	
	string mirrorModeString;
	for(map<string, int>::iterator it = intParamsValues["mirrorMode"].begin();it != intParamsValues["mirrorMode"].end();++it)
		if ( it->second == intParams["mirrorMode"] )
			mirrorModeString = it->first;
		move(line,col3name);printw("mirrorMode");
	move(line,col3val); printw("%s",mirrorModeString.substr(0,16).c_str());line++;
	string lightModeString;
	for(map<string, int>::iterator it = intParamsValues["light"].begin();it != intParamsValues["light"].end();++it)
		if ( it->second == intParams["light"] )
			lightModeString = it->first;
		move(line,col3name);printw("light");
	move(line,col3val); printw("%s",lightModeString.substr(0,16).c_str());line++;
	string HWPModeString;
	for(map<string, int>::iterator it = intParamsValues["HWPMode"].begin();it != intParamsValues["HWPMode"].end();++it)
		if ( it->second == intParams["HWPMode"] )
			HWPModeString = it->first;
		move(line,col3name);printw("HWPMode");
	move(line,col3val); printw("%s",HWPModeString.substr(0,16).c_str());line++;
	move(line,col3name);printw("HWPBand");
	move(line,col3val); printw("%d",intParams["HWPBand"]);line++;
	move(line,col3name);printw("HWPSpeed");
	move(line,col3val); printw("%.2f d/s",doubleParams["HWPSpeed"]);line++;
	if ( intParams["HWPMode"] == HWPSTEP )
	{
		move(line,col3name);printw("HWPStart");
		move(line,col3val); printw("%.2f d",doubleParams["HWPStart"]);line++;
		move(line,col3name);printw("HWPStep");
		move(line,col3val); printw("%.2f d",doubleParams["HWPStep"]);line++;
		move(line,col3name);printw("HWPPeriod");
		move(line,col3val); printw("%.2f s",doubleParams["HWPPeriod"]);line++;
	}
	
	line = 13 + vshift;
	move(line,lowname);printw("filename");
	move(line,lowval); printw("%s",stringParams["fitsname"].substr(0,65).c_str());line++;
	move(line,lowname);printw("directory");
	move(line,lowval); printw("%s",stringParams["fitsdir"].substr(0,65).c_str());line++;
	
	move(16,0);
}

void Regime::printRTABlock()
{
	int line = 0;
	
	move(line,0);
	
	move(line,0);printw("*-------------------------------RTA in progress-------------------------------*");line++;
	move(line,0);printw("|---------General---------+----------Image----------+---------Optomech--------+");line++;
	move(line,0);printw("|                         |                         |                         |");line++;
	move(line,0);printw("|                         |                         |                         |");line++;
	move(line,0);printw("|                         |                         |                         |");line++;
	move(line,0);printw("|                         |                         |                         |");line++;
	move(line,0);printw("|                         |                         |                         |");line++;
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
//		cout << "write parameter " << it->first << " all is ok" << endl;
		fprintf(f,"%s %d\n",it->first.c_str(),it->second);
	}
	for(map<string, double>::iterator it = doubleParams.begin();it != doubleParams.end();++it)
	{
  //              cout << "write parameter " << it->first << " all is ok" << endl;
		fprintf(f,"%s %f\n",it->first.c_str(),it->second);
	}
	for(map<string, string>::iterator it = stringParams.begin();it != stringParams.end();++it)
	{
//		cout << "write parameter " << it->first << " all is ok" << endl;
		fprintf(f,"%s %s\n",it->first.c_str(),it->second.c_str());
	}
	fclose(f);
	return 1;
}


int Regime::validate()
{
	int synchroNum = -1;
	for(map<string, string>::iterator it = stringParams.begin();it != stringParams.end();++it)
		if ( it->first.size() == 12 )
			if ( ( it->first.substr(0,7) == "synchro" ) && ( it->first.substr(8,4) == "Name" ) )
				if ( it->second == stringParams["synchro"] )
					istringstream ( it->first.substr(7,1) ) >> synchroNum;
	
		
	if ( synchroNum >= 0 )
	{
		std::ostringstream ossS;

		ossS << "synchro" << synchroNum << "Speed";
		doubleParams["HWPSpeed"] = doubleParams[ossS.str()];
		ossS.str("");

		ossS << "synchro" << synchroNum << "Exp";
		doubleParams["exp"] = doubleParams[ossS.str()];
		ossS.str("");

		ossS << "synchro" << synchroNum << "Height";
		intParams["height"] = intParams[ossS.str()];
		ossS.str("");
	}

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

	if (( doubleParams["observAltitude"] < 0.0 ) || ( doubleParams["observAltitude"] > 10000.0 ))
	{
		cout << "altitude validation failed" << endl;
		return 0;
	}

	if (( doubleParams["dec"] < -90.0 ) || ( doubleParams["dec"] > 90.0 ))
	{
		cout << "object declination validation failed" << endl;
		return 0;
	}

	if (( doubleParams["RA"] < 0.0 ) || ( doubleParams["RA"] > 360.0 ))
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
	
	if ((intParams["plateMirror"] != 0) && (intParams["plateMirror"] != 1))
	{
		cout << "plate mirror validation failed" << endl;
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

	if (( intParams["imWidth"] < 1 ) || ( intParams["imLeft"] + intParams["imWidth"] - 1 > 512 ))
	{
		cout << "image width validation failed" << endl;
		return 0;
	}

	if (( intParams["imBottom"] < 1 ) || ( intParams["imBottom"] > 512 ))
	{
		cout << "image bottom side validation failed" << endl;
		return 0;
	}

	if (( intParams["imHeight"] < 1 ) || ( intParams["imBottom"] + intParams["imHeight"] - 1 > 512 ))
	{
		cout << "image height validation failed" << endl;
		return 0;
	}

	if (( intParams["bin"] < 1 ) || ( intParams["bin"] > 64 ))
	{
		cout << "binning validation failed" << endl;
		return 0;
	}

	if (( intParams["winLeft"] < 1 ) || ( intParams["winLeft"] > 512 ))
	{
		cout << "window left side validation failed" << endl;
		return 0;
	}

	if (( intParams["winRight"] < 1 ) || ( intParams["winRight"] > 512 ))
	{
		cout << "window right side validation failed" << endl;
		return 0;
	}

	if (( intParams["winBottom"] < 1 ) || ( intParams["winBottom"] > 512 ))
	{
		cout << "window bottom side validation failed" << endl;
		return 0;
	}

	if (( intParams["winTop"] < 1 ) || ( intParams["winTop"] > 512 ))
	{
		cout << "window top side validation failed" << endl;
		return 0;
	}

	if ( intParams["winLeft"] >= intParams["winRight"] )
	{
		cout << "window has non-positive width, validation failed" << endl;
		return 0;
	}

	if ( intParams["winBottom"] >= intParams["winTop"] )
	{
		cout << "window has non-positive height, validation failed" << endl;
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

	if ( intParams["HWPActuatorPushedPosition1"] > 0 )
	{
		cout << "HWP actuator pushed position should be negative, validation failed" << endl;
		return 0;
	}

	if ( intParams["HWPActuatorPushedPosition2"] > 0 )
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

	if ( ( intParams["mirrorMode"] != 0 ) && ( intParams["mirrorMode"] != 1 ) && ( intParams["mirrorMode"] != 2 ) && ( intParams["mirrorMode"] != 3 ) )
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
	
	if ( ( intParams["light"] != 0 ) && ( intParams["light"] != 1 ) )
	{
		cout << "Calibration light should be off - 0 or on - 1" << endl;
		return 0;
	}

	if ( doubleParams["filterSpeed"] > 30.0 )
	{
		cout << "Filter motor speed is too high, validation failed" << endl;
		return 0;
	}

	if ( doubleParams["ADCMotor1Speed"] > 30.0 )
	{
		cout << "ADC motor 1 speed is too high, validation failed" << endl;
		return 0;
	}

	if ( doubleParams["ADCMotor2Speed"] > 30.0 )
	{
		cout << "ADC motor 2 speed is too high, validation failed" << endl;
		return 0;
	}
	
	if (!( !stringParams["focusStation"].compare("N2") || !stringParams["focusStation"].compare("C1") ))
	{
		cout << "Only N2 and C1 stations is available at the moment" << endl;
		return 0;
	}
	
	int filtNum = -1;
	for(map<string, string>::iterator it = stringParams.begin();it != stringParams.end();++it)
		if ( it->second == stringParams["filter"] )
			istringstream ( it->first.substr(6,1) ) >> filtNum;

	if ( filtNum < 0 )
	{
		cout << "There is no such filter in wheel" << endl;
		return 0;
	}
	
	std::ostringstream oss;
	oss << "filter" << filtNum << "Pos";
	currentFilterPos = doubleParams[oss.str()];
	if ( ( currentFilterPos < 0.0 ) || ( currentFilterPos > 360.0 ) )
	{
		cout << "Filter position should be between 0 and 360" << endl;
		return 0;
	}
	oss.str("");
	
	oss << "filter" << filtNum << "Name";
	currentFilterName = stringParams[oss.str()];
	oss.str("");
	
	oss << "filter" << filtNum << "Ident";
	currentFilterIdent = stringParams[oss.str()];
	oss.str("");
	
	oss << "filter" << filtNum << "Lambda";
	currentFilterLambda = doubleParams[oss.str()];
	oss.str("");

	oss << "filter" << filtNum << "ADCcoef";
	currentFilterADCcoef = doubleParams[oss.str()];
	oss.str("");
	
	cout << "validation successful" << endl;

	return 1;
}

void Regime::commandHintsFill()
{
	commandHints["latitude"] = "latitude of telescope, deg, positive is north";
	commandHints["longitude"] = "longitude of telescope, deg, positive is east";
	commandHints["observAltitude"] = "altitude a.s.l. of telescope, m";
	commandHints["RA"] = "object right ascension, deg. Also HH:MM:SS format allowable.";
	commandHints["dec"] = "object declination, deg. Also +dd:mm:ss format allowable.";
	commandHints["az"] = "Object azimuth. Used while telescope doesnt track";
	commandHints["alt"] = "Object altitude. Used while telescope doesnt track";
	commandHints["object"] = "object name";
	commandHints["program"] = "observational program ID";
	commandHints["author"] = "author of observational program";
	commandHints["telescope"] = "telescope"; 
	commandHints["instrument"] = "instrument";
	commandHints["aperture"] = "aperture diameter of the telescope, m";
	commandHints["secondary"] = "secondary mirror shadow diameter, m";
	commandHints["focusStation"] = "N1, N2, C1 etc.";
	commandHints["referencePA"] = "for definition see ADCreport.pdf";
	commandHints["plateMirror"] = "is image mirrored? (for ADC)";
	
	commandHints["skip"] = "RTA mode: period of writing frame to disk: 1 - every frame is written, 2 - every other frame is written, etc";
	commandHints["numKin"]  = "number of kinetic cycles in series";
	commandHints["limitRTA"] = "limits length of RTA series";
	commandHints["shutter"] = "shutter: 0 - close, 1 - open";
	commandHints["ft"]      = "frame transfer: 0 - disabled, 1 - enabled";
	commandHints["adc"]    = "AD channel: 0 - 14-bit (faster), 1 - 16-bit (slower)";
	commandHints["ampl"]    = "amplifier: 0 - EM, 1 - conventional";
	commandHints["horSpeed"]= "horisontal speed: conv ampl: 0 - 3 MHz; EM ampl: 0 - 10 MHz, 1 - 5 MHz, 2 - 3 MHz";
	commandHints["preamp"]  = "preamplifier: 0, 1, 2. for values see performance sheet";
	commandHints["vertSpeed"] = "vertical clocking speed (0 - 0.3, 1 - 0.5, 2 - 0.9, 3 - 1.7, 4 - 3.3) mu s";
	commandHints["vertAmpl"]= "vertical clocking voltage amplitude: 0 - 4";

	commandHints["numPol"]  = "number of series in step polarimetry mode";
		
	commandHints["imLeft"]  = "image left side: 1-512";
	commandHints["imWidth"] = "image width: 1-512";
	commandHints["imBottom"]= "image bottom side: 1-512";
	commandHints["imHeight"]   = "image height: 1-512";
	commandHints["bin"]   = "binning: 1-64";
	
	commandHints["EMGain"]  = "EM gain: 1-1000";
	commandHints["temp"]     = "target sensor temperature: -80 - 0C";
	commandHints["exp"]     = "exposure time in seconds";

	commandHints["HWPMode"]      = "0 - don't use HWP, 1 - step mode, 2 - continous rotation mode.";
	commandHints["HWPDir"]   = "Switch rotation direction, positive should be CCW seeing from detector to telescope. Check by eye.";
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
	commandHints["HWPActuatorPushedPosition1"]   = "HWP actuator pushed position (steps) < 0, motion #1";
	commandHints["HWPActuatorPushedPosition2"]   = "HWP actuator pushed position (steps) < 0, motion #2";
	commandHints["HWPSwitchingAngle"] = "start position of HWP rotator before performing switch, degrees";
	commandHints["HWPSwitchingMotion1"] = "motion of HWP rotator before performing switch, degrees, motion #1";
	commandHints["HWPSwitchingMotion2"] = "motion of HWP rotator before performing switch, degrees, motion #2";
	commandHints["HWPSwitchingSpeed"] = "speed of motion of HWP rotator performing switch, degrees/sec";
	commandHints["HWPBand"] = "Current HWP code 0, 1, 2";
	
	commandHints["light"] = "Calibration light should be off - 0 or on - 1";
	
	commandHints["filter"]   = "Desired filter short name. E.g. B, or V, or R, or I. Configurable.";
	commandHints["filterDevice"]   = "Filter motor device id (e.g. /dev/ximc/00000367)";
	commandHints["filterIntercept"]= "engine position when P.A. is zero (steps): > 0.0";
	commandHints["filterSlope"]    = "degrees per engine step: > 0.0";
	commandHints["filterSpeed"]    = "engine speed, (degrees per second)";

	commandHints["ADCMotor1Device"]   = "Filter motor device id (e.g. /dev/ximc/00000367)";
	commandHints["ADCMotor1Intercept"]= "engine position when P.A. is zero (steps): > 0.0";
	commandHints["ADCMotor1Dir"]= "Switch rotation direction, positive should be CCW seeing from detector to telescope. Check by eye.";
	commandHints["ADCMotor1Slope"]    = "degrees per engine step: > 0.0";
	commandHints["ADCMotor1Speed"]    = "engine speed, (degrees per second)";
	commandHints["ADCMotor2Device"]   = "Filter motor device id (e.g. /dev/ximc/00000367)";
	commandHints["ADCMotor2Intercept"]= "engine position when P.A. is zero (steps): > 0.0";
	commandHints["ADCMotor2Dir"]= "Switch rotation direction, positive should be CCW seeing from detector to telescope. Check by eye.";
	commandHints["ADCMotor2Slope"]    = "degrees per engine step: > 0.0";
	commandHints["ADCMotor2Speed"]    = "engine speed, (degrees per second)";
	
	commandHints["filter0ADCcoef"] = "Control coefficient for ADC, see ADCreport.pdf";
	commandHints["filter1ADCcoef"] = "Control coefficient for ADC, see ADCreport.pdf";
	commandHints["filter2ADCcoef"] = "Control coefficient for ADC, see ADCreport.pdf";
	commandHints["filter3ADCcoef"] = "Control coefficient for ADC, see ADCreport.pdf";
	commandHints["filter4ADCcoef"] = "Control coefficient for ADC, see ADCreport.pdf";
	commandHints["filter5ADCcoef"] = "Control coefficient for ADC, see ADCreport.pdf";
	commandHints["filter6ADCcoef"] = "Control coefficient for ADC, see ADCreport.pdf";
	commandHints["filter7ADCcoef"] = "Control coefficient for ADC, see ADCreport.pdf";
	
	commandHints["winLeft"]  = "window (star parameter determination) left side: 1-512";
	commandHints["winRight"] = "window (star parameter determination) right side: 1-512";
	commandHints["winBottom"]= "window (star parameter determination) bottom side: 1-512";
	commandHints["winTop"]   = "window (star parameter determination) top side: 1-512";
	
	commandHints["acq"]         = "start acquisition";
	commandHints["acqp"]         = "start acquisition in step pol regime";
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


	if ( withADCMotor1 && withADCMotor2 && ( intParams["ADCMode"] > 0 ) )
	{
		int ADCMotor1RotationStatus = 0;
		int ADCMotor2RotationStatus = 0;
		
		ADCMotor1RotationStatus = ADCMotor1->initializeStage(stringParams["ADCMotor1Device"],doubleParams["ADCMotor1Slope"],doubleParams["ADCMotor1Intercept"],intParams["ADCMotor1Dir"],doubleParams["ADCMotor1Speed"],intParams["ADCMotor1ForcedHome"]);
		
		intParams["ADCMotor1ForcedHome"] = 0;
		
		ADCMotor2RotationStatus = ADCMotor2->initializeStage(stringParams["ADCMotor2Device"],doubleParams["ADCMotor2Slope"],doubleParams["ADCMotor2Intercept"],intParams["ADCMotor2Dir"],doubleParams["ADCMotor2Speed"],intParams["ADCMotor2ForcedHome"]);
		
		intParams["ADCMotor2ForcedHome"] = 0;

		if ( intParams["ADCMode"] == ADCAUTO )
		{
			calculateADC(&ADCprismAngle1,&ADCprismAngle2);
		
			ADCMotor1->startMoveToAngleWait(ADCprismAngle1);
			ADCMotor2->startMoveToAngleWait(ADCprismAngle2);
		} else
		{
			ADCMotor1->startMoveToAngleWait(doubleParams["ADCMotor1Start"]);
			ADCMotor2->startMoveToAngleWait(doubleParams["ADCMotor2Start"]);
		}
		
//		cout << "ADC1 angle: " << ADCprismAngle1 << " ADC2 angle: " << ADCprismAngle2 << endl;
	}

	int HWPRotationStatus = 0;

	if ( withHWPMotor )
	{
		HWPRotationStatus = HWPMotor->initializeStage(stringParams["HWPDevice"],doubleParams["HWPSlope"],doubleParams["HWPIntercept"],intParams["HWPDir"],doubleParams["HWPSpeed"],intParams["HWPForcedHome"]);
		
		intParams["HWPForcedHome"] = 0;
	}

	int HWPActuatorStatus = 0;
	
	if ( withHWPAct && withHWPMotor ) 
	{
		HWPActuatorStatus = HWPActuator->initializeActuator(stringParams["HWPActuatorDevice"],doubleParams["HWPActuatorSpeed"],intParams["HWPActuatorForcedHome"]);
	
		intParams["HWPActuatorForcedHome"] = 0;
		
		cout << "current band " << HWPBand << " desired band " << intParams["HWPBand"] << endl;
		if ( HWPBand!=intParams["HWPBand"] ) 
		{
			if ( HWPBand == 0 )
			{
				if ( intParams["HWPBand"] == 1 ) switchHWP(0.0);
				if ( intParams["HWPBand"] == 2 ) {switchHWP(0.0);switchHWP(1.0);}
			}
			if ( HWPBand == 1 )
			{
				if ( intParams["HWPBand"] == 2 ) switchHWP(1.0);
				if ( intParams["HWPBand"] == 0 ) {switchHWP(0.0);switchHWP(0.0);}
			}
			if ( HWPBand == 2 )
			{
				if ( intParams["HWPBand"] == 0 ) switchHWP(0.0);
				if ( intParams["HWPBand"] == 1 ) {switchHWP(0.0);switchHWP(0.0);}
			}
			HWPBand = intParams["HWPBand"];
		}
	}
	else
	{
		cout << "HWP band switching is not possible, HWP actuator or/and motor is off" << endl;
	}	

	if ( withHWPMotor && (intParams["HWPMode"] == HWPOFF) )
	{
		HWPMotor->startMoveToAngleWait(doubleParams["HWPStart"]);
	}

	
	int mirrorActuatorStatus = 0;
	if ( withMirrorAct ) 
	{
		mirrorActuatorStatus = mirrorActuator->initializeActuator(stringParams["mirrorDevice"],intParams["mirrorSpeed"],intParams["mirrorForcedHome"]);

		intParams["mirrorForcedHome"] = 0;
		
		switch ( intParams["mirrorMode"] )
		{
		case MIRROROFF:
			mirrorActuator->startMoveToPositionWait(intParams["mirrorPosOff"]);
			break;
		case MIRRORLINPOL:
			mirrorActuator->startMoveToPositionWait(intParams["mirrorPosLinpol"]);
			break;
		case MIRRORFINDER:
			mirrorActuator->startMoveToPositionWait(intParams["mirrorPosFinder"]);
			break;
		case MIRRORAUTO:
			mirrorActuator->startMoveToPositionWait(intParams["mirrorPosLinpol"]);
			break;
		default :
			cout << "This is not normal. Validation didn't do its work." << endl;
			break;
		}

		mirrorActuatorStatus = mirrorActuator->setLight(intParams["light"]);
	}
	
	int filterRotationStatus = 0;
	
	if ( withFilterMotor )
	{
		filterRotationStatus = filterMotor->initializeStage(stringParams["filterDevice"],doubleParams["filterSlope"],doubleParams["filterIntercept"],intParams["filterDir"],doubleParams["filterSpeed"],intParams["filterForcedHome"]);
		
		intParams["filterForcedHome"] = 0;
		
		filterMotor->startMoveToAngleWait(currentFilterPos);
		
		cout << "filter: " << currentFilterName << " pos: " << currentFilterPos << endl;
	}

	if ( withDetector )
	{
		if ( !checkTempInside(intParams["temp"]-TEMP_MARGIN,intParams["temp"]+TEMP_MARGIN) )
		{
			status = (setTemp(intParams["temp"],intParams["tempStab"]))?DRV_SUCCESS:DRV_P1INVALID;
		}
		
		if ( status == DRV_SUCCESS ) cout << "temperature is ok, setting..." << endl;
		
		if ( status == DRV_SUCCESS ) status = GetDetector(&detWidth,&detHeight);
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
				if ( intParams["ampl"] == 0 )
				{
					if ( status == DRV_SUCCESS ) status = SetImage(intParams["bin"],intParams["bin"],intParams["imLeft"],intParams["imLeft"]+intParams["imWidth"]-1,intParams["imBottom"],intParams["imBottom"]+intParams["imHeight"]-1);
				}
				else
				{
					// for conv amplifier image X axis is switched
					if ( status == DRV_SUCCESS ) status = SetImage(intParams["bin"],intParams["bin"],detWidth-intParams["imLeft"]-intParams["imWidth"]+2,detWidth-intParams["imLeft"]+1,intParams["imBottom"],intParams["imTop"]);
				}
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
	
	
	return HWPRotationStatus;
}

bool Regime::runTillAbort(bool avImg, bool doSpool)
{
	cout << "entering RTA " << endl;

	getObjectFromOCS();
	
	if ( !active )
	{
		cout << "This regime is not applied. Applying" << endl;
		apply();
	}

	if ( doubleParams["exp"] > 2.0 )
	{
		cout << "run till abort is possible only if exposure < 2.0 s" << endl;
		return false;
	}

	int col1name =  1;
	int col1val  = 12;
	int col2name = 27;
	int col2val  = 38;
	int col3name = 53;
	int col3val  = 64;
	
	int status = DRV_SUCCESS;

	float exposure;
	
	if ( withDetector)
	{
		if ( status == DRV_SUCCESS ) status = GetAcquisitionTimings(&exposure, &cAccum, &cKinetic);
	}
	int width, height;
	width = floor(((double)intParams["imWidth"])/(double)intParams["bin"]);
	height = floor(((double)intParams["imHeight"])/(double)intParams["bin"]);
	long datasize = width*height; // do not divide by binning, segm. fault instead!
	long datasize2 = long(datasize/IMPROCRED); // create array smaller than main one, for purposes of processImage
	
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
	at_32 *data2 = new at_32[datasize2]; // this is undensified copy
	

	int ch = 'a';
	int frameCounter=0;
	int HWPisMoving;
	int HWPisMovingPrev=0;
	int motionStarted=0;
	int isMovingFlag=1;
	int currentPosition;
	int acc=0;

	int ADC1isMoving;
	int ADC1isMovingPrev=0;
	int ADC2isMoving;
	int ADC2isMovingPrev=0;
	int ADCmotionStarted=0;
	
	double HWPAngle;
	double HWPAngleBeforeCurrentStep = doubleParams["HWPStart"];

	double ADC1Angle,ADC2Angle;
	double ADC1AngleBeforeCurrentStep = doubleParams["ADCMotor1Start"];
	double ADC2AngleBeforeCurrentStep = doubleParams["ADCMotor2Start"];
	
	double nextStepValue = 0;

	RotationTrigger HWPTrigger;
	AngleContainer HWPAngleContainer;
	
	if ( withHWPMotor && intParams["HWPMode"] )
	{
		cout << "HWP reaching starting position ... " << endl;
		HWPMotor->startMoveToAngleWait(doubleParams["HWPStart"]);
		
		if ( intParams["HWPMode"] == HWPSTEP )
		{
			HWPTrigger.setPeriod(doubleParams["HWPPeriod"]);
			HWPTrigger.start();
		}
		else 
		{
			HWPMotor->startContiniousMotion();
		}
		cout << "done" << endl;
	}


	int mirrorIsOn = 0;
	if ( withMirrorAct && ( intParams["mirrorMode"]==MIRRORAUTO ) )
	{
		cout << "Mirror reaching starting position ... " << endl;
		mirrorActuator->startMoveToPositionWait(intParams["mirrorPosLinpol"]);
		cout << "done" << endl;
	}

	RotationTrigger ADCTrigger;
	AngleContainer ADCAngleContainer;
	
	if ( withADCMotor1 && withADCMotor2 && ( intParams["ADCMode"] > 0 ) )
	{
		if ( intParams["ADCMode"] == ADCAUTO )
		{
			calculateADC(&ADCprismAngle1,&ADCprismAngle2);
			ADCMotor1->startMoveToAngleWait(ADCprismAngle1);
			ADCMotor2->startMoveToAngleWait(ADCprismAngle2);
		}
		else
		{
			ADCMotor1->startMoveToAngleWait(doubleParams["ADCMotor1Start"]);
			ADCMotor2->startMoveToAngleWait(doubleParams["ADCMotor2Start"]);
		}
		if ( intParams["ADCMode"] == ADCSTEP) 
		{
			ADCTrigger.setPeriod(doubleParams["ADCPeriod"]);
			ADCTrigger.start();
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

	printRTABlock();
	printRegimeBlock("",7);
	
	if ( status == DRV_SUCCESS ) {
		move(2,col1name);printw("detector");
		if ( withDetector )
		{
			move(2,col1val);printw("on");
		}
		else
		{
			move(2,col1val);printw("off");
		}
	} else {
		nodelay(stdscr, FALSE);
		endwin();
		cout << "Run till abort failed, status=" << status << endl;
		delete data;
		delete data2;
		return false;
	}
	
	if ( withMirrorAct && ( intParams["mirrorMode"]==MIRRORAUTO )  )
	{
		move(3,col3name);printw("mirror");
		move(3,col3val);printw("is on");
	}
	
	
	struct timeval startTime;
	struct timezone startTz;
	gettimeofday(&startTime,&startTz);
	
	int quitRequest = 0;
	int quitRTA = 0;
	int mirrorStatus = 0;
	int imageGot = 0;
	
	int frameCounterDiv = 0;
	int frameCounterDivPrev = -1;
	
	MirrorMotionRTA mirrorMotion(mirrorActuator,intParams["mirrorPosOff"],intParams["mirrorPosLinpol"],doubleParams["mirrorBeamTime"]);
	
	while ( frameCounter < intParams["limitRTA"] ) {
		if (frameCounter>0) ch = getch();
		if ( (ch=='q') || (ch=='x') ) 
			if ( withMirrorAct && ( intParams["mirrorMode"]==MIRRORAUTO )  )
			{
				quitRequest = 1;
				move(6,col1name);printw("exit requested");
			}
			else
				quitRTA = 1;
		
		if ( withMirrorAct && ( intParams["mirrorMode"]==MIRRORAUTO )  )
		{
			quitRTA = mirrorMotion.process(frameCounter,quitRequest,&mirrorStatus);
			move(3,col3name);printw("mirror");
			move(3,col3val);
			switch (mirrorStatus) {
				case MIRRORMOVINGON: 	printw("moving on  "); break;
				case MIRRORMOVINGOFF:	printw("moving off "); break;
				case MIRRORISON: 	printw("is on      "); break;
				case MIRRORISOFF: 	printw("is off     "); break;
				default: break;
			}
		}
		if ( quitRTA )
			break;

		if ( withHWPMotor && intParams["HWPMode"] ) {
			if ( intParams["HWPMode"] == HWPSTEP)
			{
				HWPMotor->getAngle(&HWPisMoving,&HWPAngle);
				if (!anglesProximity(HWPAngleBeforeCurrentStep+nextStepValue,HWPAngle,1.0))
					HWPisMoving = 1;
				int currentStepNumber;
				if ( HWPTrigger.check(&currentStepNumber) )
				{
					motionStarted = 1;
					HWPAngleBeforeCurrentStep = HWPAngle;
					nextStepValue = getNextStepValue(currentStepNumber,doubleParams["HWPStep"],intParams["HWPPairNum"],intParams["HWPGroupNum"]);
					move(2,col3name);printw("HWP step");
					move(2,col3val);printw("%d",currentStepNumber);
//					printw("trigger fired: frame: %d HWP step: %d pair number: %d group number %d",frameCounter,currentStepNumber,(int)ceil(((currentStepNumber%(intParams["HWPPairNum"]*2))+1)/2.0),(int)ceil(currentStepNumber/(intParams["HWPPairNum"]*2.0)));
					if ( !HWPisMoving )
					{
						HWPMotor->startMoveByAngle(nextStepValue);
						move(6,col3name);printw("                         ");
					}
					else
					{
						move(6,col3name);printw("ATT: HWP skips steps");
					}
				}
				else
				{
					motionStarted = 0;
				}
			}
		}

		if ( withADCMotor1 && withADCMotor2 && ( intParams["ADCMode"]==ADCSTEP ) ) {
			ADCMotor1->getAngle(&ADC1isMoving,&ADC1Angle);
			if (!anglesProximity(ADC1AngleBeforeCurrentStep+nextStepValue,ADC1Angle,1.0))
				ADC1isMoving = 1;
			ADCMotor2->getAngle(&ADC2isMoving,&ADC2Angle);
			if (!anglesProximity(ADC2AngleBeforeCurrentStep+nextStepValue,ADC2Angle,1.0))
				ADC2isMoving = 1;
			int currentStepNumber;
			if ( ADCTrigger.check(&currentStepNumber) )
			{
				ADCmotionStarted = 1;
				ADC1AngleBeforeCurrentStep = ADC1Angle;
				ADC2AngleBeforeCurrentStep = ADC2Angle;
				nextStepValue = getNextStepValue(currentStepNumber,doubleParams["ADCStep"],1,1);
				move(3,col3name);printw("ADC step");
				move(3,col3val);printw("%d",currentStepNumber);
//					printw("trigger fired: frame: %d HWP step: %d pair number: %d group number %d",frameCounter,currentStepNumber,(int)ceil(((currentStepNumber%(intParams["HWPPairNum"]*2))+1)/2.0),(int)ceil(currentStepNumber/(intParams["HWPPairNum"]*2.0)));
				if ( !ADC1isMoving && !ADC2isMoving )
				{
					ADCMotor1->startMoveByAngle(nextStepValue);
					ADCMotor2->startMoveByAngle(nextStepValue);
					move(6,col3name);printw("                         ");
				}
				else
				{
					move(6,col3name);printw("ATT: ADC skips steps");
				}
			}
			else
			{
				ADCmotionStarted = 0;
			}
		}

		
		if ( withDetector )
		{
			if ( status == DRV_SUCCESS ) status = WaitForAcquisitionTimeOut(4500);
			if ( status == DRV_SUCCESS ) status = GetAcquisitionProgress(&acc,&frameCounter);
			move(3,col1name);printw("frame no.");
			move(3,col1val);printw("%d",frameCounter);
			
//			struct timeval currExpTime1,currExpTime2;
//			struct timezone tz;
//			gettimeofday(&currExpTime1,&tz);
/*			double deltaTime = (double)(currExpTime.tv_sec - prevExpTime.tv_sec) + 1e-6*(double)(currExpTime.tv_usec - prevExpTime.tv_usec);
			if ( deltaTime > kinetic*1.8 )  {
				move(5,0);
				printw("frames skipping is possible: delta %.2f ms, exp %.2f ms",deltaTime*1e+3,kinetic*1e+3);
			}
*/
//			prevExpTime = currExpTime;


			if ( intParams["HWPMode"] == HWPCONT )
			{
				if ( (status==DRV_SUCCESS) && !imageGot ) status=GetMostRecentImage(data,datasize);
				imageGot = 1;
				double xpos,ypos,intensity;
				int satPix,subsatPix;
				processImage(data,data2,width,height,datasize2,&xpos,&ypos,&satPix,&subsatPix,&intensity);
				move(2,col2name);printw("pos. X");
				move(2,col2val);printw("%.1f   ",xpos);
				move(3,col2name);printw("pos. Y");
				move(3,col2val);printw("%.1f   ",ypos);
				move(4,col2name);printw("intensity");
				move(4,col2val);printw("               ");
				move(4,col2val);printw("%e",intensity);
				move(5,col2name);printw("sat pix");
				move(5,col2val);printw("%d     ",satPix);
				move(6,col2name);printw("subsat pix");
				move(6,col2val);printw("%d     ",subsatPix);
			}

//			gettimeofday(&currExpTime2,&tz);
//			double deltaTime = (double)(currExpTime2.tv_sec - currExpTime1.tv_sec) + 1e-6*(double)(currExpTime2.tv_usec - currExpTime1.tv_usec);
//			move(25,0);printw("delta %.2f ms",deltaTime*1e+3);
//			if ( deltaTime > kinetic*1.8 )  {
//				move(5,0);
//				printw("frames skipping is possible: delta %.2f ms, exp %.2f ms",deltaTime*1e+3,kinetic*1e+3);
//			}

			if ( avImg )
			{
				if ( (status==DRV_SUCCESS) && !imageGot ) status=GetMostRecentImage(data,datasize);
				imageGot = 1;				
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
			frameCounterDiv = floor((double)frameCounter/(double)intParams["skip"]);
			if ( frameCounterDiv > frameCounterDivPrev)
			{
				struct timeval currRTATime;
				struct timezone tz;
				gettimeofday(&currRTATime,&tz);
				double deltaTime = (double)(currRTATime.tv_sec - prevRTATime.tv_sec) + 1e-6*(double)(currRTATime.tv_usec - prevRTATime.tv_usec);
				if ( deltaTime < 0.7 )  {
//					move(1,0);
//					printw(" frame no.: %d skipped",frameCounter);
					continue;
				}
				prevRTATime = currRTATime;
				
				if ( (status==DRV_SUCCESS) && !imageGot) status=GetMostRecentImage(data,datasize);
				imageGot = 1;				
				if (status==DRV_SUCCESS) doFits(width,height,(char*)pathes.getRTAPath(),data);

				/*
				move(1,0);
				printw("Current status: ");
				switch (status) {
				case DRV_SUCCESS: printw("OK"); break;
				case DRV_P1INVALID: printw("invalid pointer"); break;
				case DRV_P2INVALID: printw("array size is incorrect"); break;
				case DRV_NO_NEW_DATA: printw("no new data"); break;
				default: printw("unknown error");
				}
				*/
//				printw(" frame no.: %d %f",frameCounter,deltaTime);
			}
			frameCounterDivPrev = frameCounterDiv;
			if ( intParams["HWPMode"] == HWPCONT )
			{
				// do not poll detector too frequently. During continious motion of HWP it is not needed
				msec_sleep(250.0);
			}
			imageGot = 0;
		}
		else
		{
			frameCounter++;
//			if ( ( withHWPMotor && ( intParams["HWPMode"]==HWPSTEP ) ) ||
//				( withMirrorAct && ( intParams["mirrorMode"]==MIRRORAUTO ) ) )
//			{
				move(3,col1name);printw("frame no.");
				move(3,col1val);printw("%d",frameCounter);
				msec_sleep(400.0);
//			}
		}
		// Logic: if HWP was moving in the end of previous step, it moved also during current step.
		if ( withHWPMotor && (intParams["HWPMode"]==HWPSTEP) )
		{
			if (motionStarted)
			{
				HWPAngleContainer.addStatusAndAngle(frameCounter,1,HWPAngle);
//					HWPisMovingPrev = 1;
			}
			else
			{
				HWPAngleContainer.addStatusAndAngle(frameCounter,(int)(HWPisMoving!=0),HWPAngle);
//					HWPisMovingPrev = HWPisMoving;
			}
		}

		if ( withADCMotor1 && withADCMotor2 && (intParams["ADCMode"]==ADCSTEP) )
		{
			if (ADCmotionStarted)
			{
				ADCAngleContainer.addStatusAndAngle(frameCounter,1,ADC2Angle);
//					HWPisMovingPrev = 1;
			}
			else
			{
				ADCAngleContainer.addStatusAndAngle(frameCounter,(int)(ADC1isMoving!=0),ADC2Angle);
//					HWPisMovingPrev = HWPisMoving;
			}
		}

		
		if ( withADCMotor1 && withADCMotor2 && ( intParams["ADCMode"] == ADCAUTO ) )
		{
			double tmpAngle1,tmpAngle2;
			calculateADC(&tmpAngle1,&tmpAngle2);
			
			if ( fabs( tmpAngle1 - ADCprismAngle1 ) > ADCMARGIN )
			{
				ADCprismAngle1 = tmpAngle1;
				ADCMotor1->startMoveToAngle(ADCprismAngle1);
			}
			
			if ( fabs( tmpAngle2 - ADCprismAngle2 ) > ADCMARGIN )
			{
				ADCprismAngle2 = tmpAngle2;
				ADCMotor2->startMoveToAngle(ADCprismAngle2);
			}
		}
	}
	
	if (( withDetector ) && ( status == DRV_SUCCESS )) status = AbortAcquisition();
	if ( withHWPMotor && intParams["HWPMode"] ) HWPMotor->stopContiniousMotion();
	
	werase(stdscr);
	nodelay(stdscr, FALSE);
	refresh();
	endwin();

	if (withMirrorAct && ( intParams["mirrorMode"]==MIRRORAUTO ))
	{
		mirrorMotion.print();
		cout << "Mirror reaching final position ... " << endl;
		mirrorActuator->startMoveToPositionWait(intParams["mirrorPosLinpol"]);
		cout << "Done." << endl;
	}
	
	// First we write essential keywords into primary HDU header, substituting some unneccesary technical keywords. It is not possible to ADD keywords, this would very expensive in terms of time.
	if ( doSpool ) augmentPrimaryHDU(); // keywords: TELESCOP, INSTRUME, OBJECT, PROGRAM, AUTHOR, RA, DEC

	// Then we add data on rotation of HWP (if any) and motion of Mirror (if any)
	if ( doSpool && withHWPMotor && (intParams["HWPMode"]==HWPSTEP) )
	{
		cout << "writing HWP angle data" << endl;
		HWPAngleContainer.convertToIntervals();
		HWPAngleContainer.print();
		HWPAngleContainer.writeIntervalsToFits((char*)pathes.getSpoolPathSuff(),"HWPINTERVALS");
	}

	if ( doSpool && withADCMotor1 && withADCMotor2 && (intParams["ADCMode"]==ADCSTEP) )
	{
		cout << "writing ADC angle data" << endl;
		ADCAngleContainer.convertToIntervals();
		ADCAngleContainer.print();
		ADCAngleContainer.writeIntervalsToFits((char*)pathes.getSpoolPathSuff(),"ADCINTERVALS");
	}

	
	if ( doSpool && withMirrorAct && ( intParams["mirrorMode"]==MIRRORAUTO )  )
	{
		mirrorMotion.writeIntervalsToFits((char*)pathes.getSpoolPathSuff());
	}

	// Finally we write additional keywords into specially created empty HDU.
	if ( doSpool ) addAuxiliaryHDU(); // keywords: LONGITUD, LATITUDE, ALTITUDE, APERTURE, SECONDAR, FOCUSSTA, REFERPA, PLATEMIR, MIRRMODE, ADCMODE, HWPMODE, HWPBAND, RONSIGMA
	
	if ( ( stringParams["fitsname"] == "auto" ) && ( doSpool == 1 ) )
	{
		rename((char*)pathes.getSpoolPathSuff(),(char*)pathes.getAutopathSuff());
	}
	
	delete data;
	delete data2;
	return true;
}

bool Regime::acquirePol()
{
	
	getObjectFromOCS();
	
	if ( !active )
	{
		cout << "This regime is not applied. Applying" << endl;
		apply();
	}
	
	doubleParams["HWPStart"] = 0.0;
	intParams["HWPMode"] = 0;
	
	double nextStepValue;
	cout << "acq pol" << endl;

	bool acqResult = false;
	
	for(int currentStepNumber = 1; currentStepNumber <= intParams["numPol"]; currentStepNumber++ )
	{
		cout << "HWP moves to angle: " << doubleParams["HWPStart"] << endl;
		if ( withHWPMotor )
			HWPMotor->startMoveToAngleWait(doubleParams["HWPStart"]);
		
		cout << "exposure" << endl;
		if ( withDetector )
			acqResult = acquire();
		else
			usleep(10000000);
		cout << "done" << endl;
		
		if ( !acqResult ) break;
		
		nextStepValue = getNextStepValue(currentStepNumber,doubleParams["HWPStep"],intParams["HWPPairNum"],intParams["HWPGroupNum"]);
		cout << "currentStep: " << currentStepNumber << " next step value: " << nextStepValue << endl;
		doubleParams["HWPStart"] += nextStepValue;
	}
}

bool Regime::acquire()
{
	getObjectFromOCS();
		
	if ( !active )
	{
		cout << "This regime is not applied. Applying" << endl;
		apply();
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

	bool aborted = false;

	while ( 1 ) {
		ch = getch();
	        if ( (ch=='q') || (ch=='x') ) {
			if ( status == DRV_SUCCESS ) status = AbortAcquisition();
			{
				aborted = true;
				break;
			}
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
	
	augmentPrimaryHDU();
	addAuxiliaryHDU();
	
	if ( stringParams["fitsname"] == "auto" )
	{
		rename((char*)pathes.getSpoolPathSuff(),(char*)pathes.getAutopathSuff());
	}
	
	return !aborted;
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

	cout << "acquisition timings:" << endl;
	if ( status == DRV_SUCCESS ) status = GetAcquisitionTimings(&exposure, &cAccum, &cKinetic);
	cout << "exposure: " << exposure << " s" << endl;
	cout << "accumulation cycle time: " << cAccum << " s" << endl;
	cout << "kinetic cycle time: " << cKinetic << " s" << endl;

	return ( status == DRV_SUCCESS );
}

int Regime::switchHWP(double posShift)
{
	int isMovingFlag = 1;
	double currentAngle;

	cout << "Setting initial position ... " << endl;

	HWPMotor->initializeStage(stringParams["HWPDevice"],doubleParams["HWPSlope"],doubleParams["HWPIntercept"],intParams["HWPDir"],doubleParams["HWPSwitchingSpeed"],1);
	cout << "done." << endl;

	cout << "Switching ... " << endl;
	HWPMotor->startMoveToAngleWait(doubleParams["HWPSwitchingPosition1"]);
	HWPActuator->startMoveToPositionWait(intParams["HWPActuatorPushedPosition1"]);
	HWPMotor->startMoveToAngleWait(doubleParams["HWPSwitchingPosition2"]);
	HWPActuator->startMoveToPositionWait(0);
	HWPMotor->startMoveToAngleWait(doubleParams["HWPSwitchingPosition3"]);
	HWPActuator->startMoveToPositionWait(intParams["HWPActuatorPushedPosition2"]);
	HWPMotor->startMoveToAngleWait(doubleParams["HWPSwitchingPosition1"]+posShift);
//	HWPActuator->startMoveToPositionWait(intParams["HWPActuatorPushedPosition1"]);

	

//	HWPMotor->setSpeed(doubleParams["HWPSwitchingSpeed"]);
	
	
/*	HWPMotor->startMoveByAngle(doubleParams["HWPSwitchingMotion1"]);
	isMovingFlag = 1;
	
	struct timeval startTime;
	struct timeval currTime;	
	struct timezone tz;
	double deltaTime;
	gettimeofday(&startTime,&tz);
	
	while (isMovingFlag)
	{
		HWPMotor->getAngle(&isMovingFlag,&currentAngle);
		usleep(100000);
		
		gettimeofday(&currTime,&tz);	
		deltaTime = (double)(currTime.tv_sec - startTime.tv_sec) + 1e-6*(double)(currTime.tv_usec - startTime.tv_usec);
		if ( deltaTime > MOTIONTIMEOUT )
		{
			cout  << "HWP switch, time for motion is over!" << endl;
			break;
		}
	}

	HWPActuator->startMoveToPositionWait(intParams["HWPActuatorPushedPosition2"]);
	HWPMotor->startMoveByAngle(doubleParams["HWPSwitchingMotion2"]);
	isMovingFlag = 1;
	
	gettimeofday(&startTime,&tz);
	
	while (isMovingFlag)
	{
		HWPMotor->getAngle(&isMovingFlag,&currentAngle);
		usleep(100000);
		
		gettimeofday(&currTime,&tz);	
		deltaTime = (double)(currTime.tv_sec - startTime.tv_sec) + 1e-6*(double)(currTime.tv_usec - startTime.tv_usec);
		if ( deltaTime > MOTIONTIMEOUT )
		{
			cout  << "HWP switch, time for motion is over!" << endl;
			break;
		}
	}
*/
	cout << "done." << endl;

//	HWPMotor->setSpeed(doubleParams["HWPSpeed"]);
	
	cout << "Actuator push back ... " << endl;
	HWPActuator->startMoveToPositionWait(0);
	cout << "done." << endl;
	
	HWPMotor->initializeStage(stringParams["HWPDevice"],doubleParams["HWPSlope"],doubleParams["HWPIntercept"],intParams["HWPDir"],doubleParams["HWPSpeed"],1);
	
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


bool Regime::finalize(float startTemp)
{
	int status = DRV_SUCCESS;
	if ( withDetector )
	{
		if ( status == DRV_SUCCESS ) status = SetShutter(1,2,50,50);
		if ( status != DRV_SUCCESS )
			return false;

		if (!checkTempInside(startTemp-15.0,startTemp+15.0))
			if (!setTemp(startTemp-10.0,false))
				return false; // in case temperature cannot be set, don't quit

		ShutDown();
	}
	
	intParams["HWPBand"] = 0;
	apply();
	
	cout << "Parking motors ... " << endl;
	
	cout << "HWP..." << flush;
	if (withHWPMotor) {
		HWPMotor->startMoveToAngleWait(0.0);
	}
	cout << "done" << endl;
	
	cout << "HWP act..." << flush;
	if (withHWPAct) {
		HWPActuator->startMoveToPositionWait(0);
	}
	cout << "done" << endl;
	
	cout << "Mirror act..." << flush;
	if (withMirrorAct) {
		mirrorActuator->startMoveToPositionWait(0);
	}
	cout << "done" << endl;
	
	cout << "Filter..." << flush;
	if (withFilterMotor) {
		filterMotor->startMoveToAngleWait(0.0);
	}
	cout << "done" << endl;
	
	cout << "ADC motor1..." << flush;
	if (withADCMotor1) {
		ADCMotor1->startMoveToAngleWait(0.0);
	}
	cout << "done" << endl;
	
	cout << "ADC motor2..." << flush;
	if (withADCMotor2) {
		ADCMotor2->startMoveToAngleWait(0.0);
	}
	cout << "done" << endl;
	
	return true;
}

void Regime::augmentPrimaryHDU()
{
	float version; 
	fits_get_version(&version);

	
	cout << "FITSIO ver: " << version << "Start augmenting primary HDU of fits:" << (char*)pathes.getSpoolPathSuff() << endl;
	fitsfile *fptr;
	int status = 0, keytype = 0;
	char card[FLEN_CARD],newcard[FLEN_CARD],value[FLEN_CARD];
	// If file doesn't exist, what happens when we work without detector, create it with empty primary array
	struct stat  buffer;
	if ( stat((char*)pathes.getSpoolPathSuff(), &buffer) == 0 )
	{	
		if ( fits_open_file(&fptr, (char*)pathes.getSpoolPathSuff(), READWRITE, &status) )
			printerror( status );
	}
	else
	{
		if (fits_create_file(&fptr, (char*)pathes.getSpoolPathSuff(), &status))
			printerror( status );        			
		
		int bitpix   =  FLOAT_IMG; /* 32-bit double pixel values       */
		long naxis    =   0;  /* 2-dimensional image                            */
		long naxes[0];   /* image is nx pixels wide by ny rows */
		
		if ( fits_create_img(fptr,  bitpix, naxis, naxes, &status) )
			printerror( status );
	}
	
	//forced update of ACT and KCT keywords, because detector doesn't write them
	
	fits_update_key(fptr, TFLOAT, "ACT     ", &cAccum, "",&status);
        fits_update_key(fptr, TFLOAT, "KCT     ", &cKinetic, "",&status);   
        	        
	// replace unused technical keywords with seven basic keywords defining observation
	fits_modify_name(fptr, "AVERAGINGFILTERMODE", "TELESCOP", &status);
	strcpy(value, stringParams["telescope"].c_str());   
	fits_update_key(fptr, TSTRING, "TELESCOP", value, "",&status);

        fits_modify_name(fptr, "AVERAGINGFACTOR", "INSTRUME", &status);
        strcpy(value, stringParams["instrument"].c_str());  
	fits_update_key(fptr, TSTRING, "INSTRUME", value, "",&status);

        fits_modify_name(fptr, "FRAMECOUNT", "OBJECT", &status);    
        strcpy(value, stringParams["object"].c_str());    
	fits_update_key(fptr, TSTRING, "OBJECT", value, "",&status);

        fits_modify_name(fptr, "USERTXT1", "PROGID", &status);    
        strcpy(value, stringParams["program"].c_str());    
	fits_update_key(fptr, TSTRING, "PROGID", value, "",&status);

        fits_modify_name(fptr, "USERTXT2", "AUTHOR", &status);    
        strcpy(value, stringParams["author"].c_str());    
	fits_update_key(fptr, TSTRING, "AUTHOR", value, "",&status);

	if ( intParams["tracking"] == 1 )
	{
		sprintf(newcard,"RA = %.4f",doubleParams["RA"]);
		fits_parse_template(newcard, card, &keytype, &status);
		fits_update_card(fptr, "USERTXT3", card, & status);

		sprintf(newcard,"DEC = %.4f",doubleParams["dec"]);
		fits_parse_template(newcard, card, &keytype, &status);
		fits_update_card(fptr, "USERTXT4", card, & status);
	}
	else
	{
		sprintf(newcard,"AZ = %.4f",doubleParams["az"]);
		fits_parse_template(newcard, card, &keytype, &status);
		fits_update_card(fptr, "USERTXT3", card, & status);

		sprintf(newcard,"ALT = %.4f",doubleParams["alt"]);
		fits_parse_template(newcard, card, &keytype, &status);
		fits_update_card(fptr, "USERTXT4", card, & status);
	}
	
	fits_close_file(fptr, &status);

	if ( status )
		printerror( status );
//	cout << "Finish augmenting primary HDU" << endl;
}

void Regime::addAuxiliaryHDU()
{
	fitsfile *fptr;
	int status = 0, keytype = 0;
	char card[FLEN_CARD],newcard[FLEN_CARD];
	if ( fits_open_file(&fptr, (char*)pathes.getSpoolPathSuff(), READWRITE, &status) )
		printerror( status );
	
	char extname[] = "METADATA";             /* extension name */
	int nrows = 0,tfields = 0;
	char *ttype[] = {};
	char *tform[] = {};
	char *tunit[] = {};
	
	if ( fits_create_tbl( fptr, ASCII_TBL, nrows, tfields, ttype, tform, tunit, extname, &status) )
		printerror( status );
	
	sprintf(newcard,"LONGITUD = %.4f",doubleParams["longitude"]);
	fits_parse_template(newcard, card, &keytype, &status);
	fits_update_card(fptr, "LONGITUD", card, & status);
	
	sprintf(newcard,"LATITUDE = %.4f",doubleParams["latitude"]);
	fits_parse_template(newcard, card, &keytype, &status);
	fits_update_card(fptr, "LATITUDE", card, & status);

	sprintf(newcard,"ALTITUDE = %.4f",doubleParams["observAltitude"]);
	fits_parse_template(newcard, card, &keytype, &status);
	fits_update_card(fptr, "ALTITUDE", card, & status);
	
	sprintf(newcard,"APERTURE = %.4f",doubleParams["aperture"]);
	fits_parse_template(newcard, card, &keytype, &status);
	fits_update_card(fptr, "APERTURE", card, & status);
	
	sprintf(newcard,"SECONDAR = %.4f",doubleParams["secondary"]);
	fits_parse_template(newcard, card, &keytype, &status);
	fits_update_card(fptr, "SECONDAR", card, & status);

	sprintf(newcard,"FOCUSSTA = %s",stringParams["focusStation"].c_str());
	fits_parse_template(newcard, card, &keytype, &status);
	fits_update_card(fptr, "FOCUSSTA", card, & status);

	sprintf(newcard,"REFERPA = %.2f",doubleParams["referencePA"]);
	fits_parse_template(newcard, card, &keytype, &status);
	fits_update_card(fptr, "REFERPA", card, & status);
	
	sprintf(newcard,"PLATEPA = %.2f",positionAngle); // attention: correctness of this value relies on referencePA
	fits_parse_template(newcard, card, &keytype, &status);
	fits_update_card(fptr, "PLATEPA", card, & status);
	
	sprintf(newcard,"PLATEMIR = %d",intParams["plateMirror"]);
	fits_parse_template(newcard, card, &keytype, &status);
	fits_update_card(fptr, "PLATEMIR", card, & status);

	string mirrorModeString;
	for(map<string, int>::iterator it = intParamsValues["mirrorMode"].begin();it != intParamsValues["mirrorMode"].end();++it)
		if ( it->second == intParams["mirrorMode"] )
			mirrorModeString = it->first;
	sprintf(newcard,"MIRRMODE = %s",mirrorModeString.c_str());
	fits_parse_template(newcard, card, &keytype, &status);
	fits_update_card(fptr, "MIRRMODE", card, & status);

	
	string ADCModeString;
	for(map<string, int>::iterator it = intParamsValues["ADCMode"].begin();it != intParamsValues["ADCMode"].end();++it)
		if ( it->second == intParams["ADCMode"] )
			ADCModeString = it->first;
	sprintf(newcard,"ADCMODE = %s",ADCModeString.c_str());
	fits_parse_template(newcard, card, &keytype, &status);
	fits_update_card(fptr, "ADCMODE", card, & status);
	
	if ( ( intParams["ADCMode"] == ADCOFF ) || ( intParams["ADCMode"] == ADCMAN ) )
	{
		sprintf(newcard,"ADC1ANGL = %.2f",doubleParams["ADCMotor1Start"]);
		fits_parse_template(newcard, card, &keytype, &status);
		fits_update_card(fptr, "ADC1ANGL", card, & status);

		sprintf(newcard,"ADC2ANGL = %.2f",doubleParams["ADCMotor2Start"]);
		fits_parse_template(newcard, card, &keytype, &status);
		fits_update_card(fptr, "ADC2ANGL", card, & status);
	}
	
	string HWPModeString;
	for(map<string, int>::iterator it = intParamsValues["HWPMode"].begin();it != intParamsValues["HWPMode"].end();++it)
		if ( it->second == intParams["HWPMode"] )
			HWPModeString = it->first;
	sprintf(newcard,"HWPMODE = %s",HWPModeString.c_str());
	fits_parse_template(newcard, card, &keytype, &status);
	fits_update_card(fptr, "HWPMODE", card, & status);
		
	sprintf(newcard,"HWPBAND = %d",intParams["HWPBand"]);
	fits_parse_template(newcard, card, &keytype, &status);
	fits_update_card(fptr, "HWPBAND", card, & status);
	
	if ( intParams["HWPMode"] == HWPOFF )
	{
		sprintf(newcard,"HWPANGLE = %.2f",doubleParams["HWPStart"]);
		fits_parse_template(newcard, card, &keytype, &status);
		fits_update_card(fptr, "HWPANGLE", card, & status);
	}

        sprintf(newcard,"SHUTTER = %d",intParams["shutter"]);
        fits_parse_template(newcard, card, &keytype, &status);
        fits_update_card(fptr, "SHUTTER", card, & status);

	double RONSigma[3];
	double sensitiv[3];
	if (intParams["adc"] == 0) // 14-bit
		if (intParams["ampl"] == 0) // EM amplifier
			if      (intParams["horSpeed"] == 0)
			{RONSigma[0] = 93.33; RONSigma[1] = 58.58; RONSigma[2] = 49.64;
			 sensitiv[0] = 61.00; sensitiv[1] = 25.14; sensitiv[2] = 11.57;}
			else if (intParams["horSpeed"] == 1)
			{RONSigma[0] = 82.54; RONSigma[1] = 50.78; RONSigma[2] = 39.48;
	 		 sensitiv[0] = 53.60; sensitiv[1] = 22.08; sensitiv[2] =  9.97;}
			else  
			{RONSigma[0] = 61.02; RONSigma[1] = 36.87; RONSigma[2] = 30.02;
   			 sensitiv[0] = 52.60; sensitiv[1] = 21.69; sensitiv[2] =  9.94;}
		else // conv. amplifier
		{RONSigma[0] = 14.37; RONSigma[1] = 10.56; RONSigma[2] = 9.64;
		 sensitiv[0] = 10.05; sensitiv[1] =  3.97; sensitiv[2] = 1.75;}
	else // 16-bit
		if (intParams["ampl"] == 0)
		{RONSigma[0] = 36.24; RONSigma[1] = 22.03; RONSigma[2] = 18.56;
		 sensitiv[0] = 20.83; sensitiv[1] =  8.54; sensitiv[2] =  3.89;} // EM amplifier
		else
		{RONSigma[0] = 8.45; RONSigma[1] = 6.7; RONSigma[2] = 6.08;
		 sensitiv[0] = 3.66; sensitiv[1] = 1.47; sensitiv[2] = 0.64;} 	  // conv. amplifier
	
	sprintf(newcard,"RONSIGMA = %.2f",RONSigma[intParams["preamp"]]);
	fits_parse_template(newcard, card, &keytype, &status);
	fits_update_card(fptr, "RONSIGMA", card, & status);

	sprintf(newcard,"SENSITIV = %.2f",sensitiv[intParams["preamp"]]);
	fits_parse_template(newcard, card, &keytype, &status);
	fits_update_card(fptr, "SENSITIV", card, & status);
	
	sprintf(newcard,"PIXELSIZ = %e",doubleParams["pixelSize"]);
	fits_parse_template(newcard, card, &keytype, &status);
	fits_update_card(fptr, "PIXELSIZ", card, & status);
	
	sprintf(newcard,"LIGHT = %d",intParams["light"]);
	fits_parse_template(newcard, card, &keytype, &status);
	fits_update_card(fptr, "LIGHT", card, & status);
	
	sprintf(newcard,"FILTER = %s",currentFilterName.c_str());
	fits_parse_template(newcard, card, &keytype, &status);
	fits_update_card(fptr, "FILTER", card, & status);
	
	sprintf(newcard,"FILTIDEN = %s",currentFilterIdent.c_str());
	fits_parse_template(newcard, card, &keytype, &status);
	fits_update_card(fptr, "FILTIDEN", card, & status);
	
	sprintf(newcard,"FILTLAM = %.2f",currentFilterLambda);
	fits_parse_template(newcard, card, &keytype, &status);
	fits_update_card(fptr, "FILTLAM", card, & status);
	
	if ( fits_close_file(fptr, &status) )       /* close the FITS file */
		printerror( status );
	
}

void Regime::calculateADC(double *_angle1, double *_angle2)
{
	double zen;

	if ( intParams["tracking"] == 1 )
	{
		struct ln_lnlat_posn observer;
		struct ln_equ_posn object;
		struct ln_hrz_posn hrz;
		
		observer.lng = doubleParams["longitude"];
		observer.lat = doubleParams["latitude"];
		
		object.ra = doubleParams["RA"];
		object.dec = doubleParams["dec"];
		
		double JD = ln_get_julian_from_sys();
		
		ln_get_hrz_from_equ(&object, &observer, JD, &hrz);
		
		zen = 90-hrz.alt;
	}
	else
	{
		zen = 90-doubleParams["alt"];
	}
	
	
	double gamma = currentFilterADCcoef*tan(zen/RAD);
	 
	double deltaChi=90.00;
	
	if (gamma<1)
	{
		deltaChi = asin(gamma)*RAD;
	}
	
	double angle1 = -1.0;
	double angle2 = -1.0;
	
	if ( stringParams["focusStation"].compare("N2") == 0 )
	{
		angle1 = doubleParams["referencePA"] + zen - 90.0 + deltaChi;
		angle2 = doubleParams["referencePA"] + zen + 90.0 - deltaChi;
	} 
	else if ( stringParams["focusStation"].compare("C1") == 0 )
	{
		double currentDero = 0;
		if ( intParams["tracking"] == 1 )
		{
			currentDero = deroDifference + parallacticAngle();
		} else {
			currentDero = doubleParams["dero"];
		}
		angle1 = doubleParams["referencePA"] + currentDero + 90.0 + deltaChi;
		angle2 = doubleParams["referencePA"] + currentDero - 90.0 - deltaChi;
	}
		
	while ( angle1 > 360.0 )
		angle1 -= 360.0;
	while ( angle1 < 0.0 )
		angle1 += 360.0;

	while ( angle2 > 360.0 )
		angle2 -= 360.0;
	while ( angle2 < 0.0 )
		angle2 += 360.0;
	
	*_angle1 = angle1;
	*_angle2 = angle2;
//	move(25,0);printw("an1:%f, an2%f",angle1,angle2);
//	move(26,0);printw("JD:%7.8f, %f",JD,zen);
}

double Regime::parallacticAngle()
{
	struct ln_lnlat_posn observer;
	struct ln_equ_posn object;
	struct ln_hrz_posn hrz;
	
	observer.lng = doubleParams["longitude"];
	observer.lat = doubleParams["latitude"];
	
	object.ra = doubleParams["RA"];
	object.dec = doubleParams["dec"];
	
	double JD = ln_get_julian_from_sys();
	double sidTime = ln_get_apparent_sidereal_time(JD)*15.0+observer.lng;
	
	double hourAngle = sidTime - object.ra;

//	cout << " lng " << observer.lng << " lat " << observer.lat << endl;
//	cout << " ra " << object.ra << " dec " << object.dec << endl;
	
	ln_get_hrz_from_equ(&object, &observer, JD, &hrz);
	
	double sinp = cos(observer.lat/RAD)*sin(hrz.az/RAD)/cos(object.dec/RAD);
	double cosp = cos(hrz.az/RAD)*cos(hourAngle/RAD) + sin(hrz.az/RAD)*sin(hourAngle/RAD)*cos(observer.lat/RAD);
	
	double parAngle = atan2(sinp,cosp)*RAD;
	
//	cout << "az " << hrz.az << " alt " << hrz.alt << endl;
//	cout << "ha " << hourAngle << " parAngle " << parAngle << endl;
	
	return parAngle;
}


void doFits(int nx, int ny, char* filename,at_32 *data)
{

		fitsfile *fptr;       /* pointer to the FITS file, defined in fitsio.h */
		int status, ii, jj;
		long  fpixel, nelements;
		long *array[ny];

		int bitpix   =  LONG_IMG; 
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

		fpixel = 1;                               /* first pixel to write      */
		nelements = naxes[0] * naxes[1];          /* number of pixels to write */
		
		for (jj = 0; jj < naxes[1]; jj++){
			for (ii = 0; ii < naxes[0]; ii++) {
				array[jj][ii] = data[ii+naxes[0]*jj];
	        	}
		}


/* write the array of unsigned integers to the FITS file */
		if ( fits_write_img(fptr, TLONG, fpixel, nelements, array[0], &status) )
			printerror( status );

		free( array[0] );  /* free previously allocated memory */

		if ( fits_close_file(fptr, &status) )                /* close the file */
			printerror( status );
//	printf("FITS writed: %s\n",filename);
	
	return;
}

void Regime::processImage(at_32* data, at_32* data2, int width, int height, int datasize2, double* xpos, double *ypos, int* satPix, int* subsatPix, double* inten)
{
	long datasize = width*height;
	
	int satTres,subsatTres;
	if ( intParams["adc"] == 0 )
	{
		satTres = SAT14BIT;
		subsatTres = SUBSAT14BIT;
	}
	else
	{
		satTres = SAT16BIT;
		subsatTres = SUBSAT16BIT;
	}
//	move(25,0);printw("w:%d, h:%d, pix1:%d, pix2:%d, pix3:%d, tres:%d",width,height,data[100],data[200],data[300],satTres);

	// crude estimation of background
	// prepare rarefied copy of array data
	for(long i = 0; i < datasize2; i++)
		data2[i] = data[i*IMPROCRED];
	// sort it
	qsort(data2, datasize2, sizeof(at_32), intCompare);
	// find median, use it as background level
	int mbias = data2[int(datasize2/2)];
	
	// window corresponding to field diaphragm
	long Nleft = (long)((double)(intParams["winLeft"]-intParams["imLeft"])/(double)intParams["bin"]);
	long Nright = (long)((double)(intParams["winRight"]-intParams["imLeft"])/(double)intParams["bin"]);
	long Nbottom = (long)((double)(intParams["winBottom"]-intParams["imBottom"])/(double)intParams["bin"]);
	long Ntop = (long)((double)(intParams["winTop"]-intParams["imBottom"])/(double)intParams["bin"]);
	
	// find preliminary COG
	long _satPix = 0;
	long _subsatPix = 0;
	long _inten = 0;
	long _posX = 0;
	long _posY = 0;
	long x,y;
	for(long i = 0; i < datasize; i++)
	{
		y = int(i/width);
		x = i - y*width;
		
		if ( (x>=Nleft) && (x<=Nright) && (y>=Nbottom) && (y<=Ntop) )
		{
			if ( data[i] > satTres ) 
				_satPix++;
			if ( data[i] > subsatTres ) 
				_subsatPix++;
			_inten += data[i]-mbias;
			_posX += x*(data[i]-mbias);
			_posY += y*(data[i]-mbias);
		}
	}
	
	if ( _inten > 1 )
	{
		_posX = _posX/_inten;
		_posY = _posY/_inten;
	}
	else
	{
		*xpos = -1.0;
		*ypos = -1.0;	
		*satPix = -1;
		*subsatPix = -1;
		*inten = -1.0;
		return;
	}
	
	// very crudely we estimate expected image size as 1/8 of detector size
	long windowHalfSize = (32/intParams["bin"]);
	long _nposX = 0;
	long _nposY = 0;
	long _ninten = 0;
	
	// final estimation of COG, windowed
	for(long i = 0; i < datasize; i++)
	{
		y = int(i/width);
		x = i - y*width;
		
		if ( (x>=(_posX-windowHalfSize)) && (x<=(_posX+windowHalfSize)) && (y>=(_posY-windowHalfSize)) && (y<=(_posY+windowHalfSize)) )
		{
			_ninten += data[i]-mbias;
			_nposX += x*(data[i]-mbias);
			_nposY += y*(data[i]-mbias);
		}
	}
	
	
	if ( _ninten > 1 )
	{
		*ypos = (intParams["bin"]*(double)_nposY/(double)_ninten) + intParams["imBottom"];
		*xpos = (intParams["bin"]*(double)_nposX/(double)_ninten) + intParams["imLeft"];
//		if ( intParams["ampl"] == 0 )
//		{
//			*xpos = (intParams["bin"]*(double)_nposX/(double)_ninten) + intParams["imLeft"];
//		}
//		else
//		{
			// for conv amplifier image X axis is switched
//			*xpos = (intParams["bin"]*(width-((double)_nposX/(double)_ninten))) + (detWidth - intParams["imRight"]);
//		}
	}
	else
	{
		*xpos = -1.0;
		*ypos = -1.0;
	}
	
	*satPix = _satPix;
	*subsatPix = _subsatPix;
	*inten = (double)_ninten;
}

int Regime::loadCFGfile()
{
	struct stat  buffer;
	int st = stat(stringParams["cfgfile"].c_str(), &buffer);
	if ( st != 0 )
	{
		cout << "error: stat check: file doesn't exist" << endl;
		return 0;
	}
	if ( S_ISDIR(buffer.st_mode) )
	{
		cout << "error: stat check: this is directory!" << endl;
		return 0;
	}

	ifstream file(stringParams["cfgfile"].c_str(),ios::in);

	if ( !file )
	{
		cout << "error: ifstream check: file doesn't exist" << endl;
		return 0;
	}

	string str;
	while ( getline(file,str) )
		procCommand(str);

	file.close();

	return 1;

}

int Regime::getObjectFromOCS()
{
	int sockfd, portno, n;

	struct sockaddr_in serv_addr;
	struct hostent *server;

	char buffer[256];
	cout << "Getting data from OCS on " << stringParams["OCSIP"] << ", port " << intParams["OCSport"] << endl;

	portno = intParams["OCSport"];
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
		cout << "ERROR opening socket" << endl;
	server = gethostbyname(stringParams["OCSIP"].c_str());
	if (server == NULL) {
		cout << "ERROR, no such host" << endl;
		return 0;
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,server->h_length);
	serv_addr.sin_port = htons(portno);
	if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
	{
		cout << "ERROR connecting" << endl;
		return 0;
	}
	bzero(buffer,256);
//    sprintf(buffer,"getTarget");
	
	struct timeval tv;
	time_t nowtime;
	struct tm *nowtm;
	char tmbuf[64];
	
	gettimeofday(&tv, NULL);
	nowtime = tv.tv_sec;
	nowtm = gmtime(&nowtime);
	if ( nowtm->tm_hour < 12 )
		tv.tv_sec = tv.tv_sec-86400;
	nowtime = tv.tv_sec;
	nowtm = gmtime(&nowtime);
	strftime(tmbuf, sizeof tmbuf, "%Y%m%d.%H%M%S", nowtm);
	sprintf(buffer,"<CmdCUID=6.0.1.2.1.%s>",tmbuf);

	n = write(sockfd,buffer,strlen(buffer));
	if (n < 0)
	{
		cout << "ERROR writing to socket" << endl;
		return 0;
	}
	
	bzero(buffer,256);
	n = read(sockfd,buffer,255);
	if (n < 0) 
	{
		cout << "ERROR reading from socket" << endl;
		return 0;
	}
	
	string message(buffer,256);
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
	{
		cout << "ERROR connecting" << endl;
		return 0;
	}
	bzero(buffer,256);
	
	gettimeofday(&tv, NULL);
	nowtime = tv.tv_sec;
	nowtm = gmtime(&nowtime);
	if ( nowtm->tm_hour < 12 )
		tv.tv_sec = tv.tv_sec-86400;
	nowtime = tv.tv_sec;
	nowtm = gmtime(&nowtime);
	strftime(tmbuf, sizeof tmbuf, "%Y%m%d.%H%M%S", nowtm);
	sprintf(buffer,"<CmdCUID=6.0.1.1.1.%s>",tmbuf);

	n = write(sockfd,buffer,strlen(buffer));
	if (n < 0)
	{
		cout << "ERROR writing to socket" << endl;
		return 0;
	}
	
	bzero(buffer,256);
	n = read(sockfd,buffer,255);
	if (n < 0) 
	{
		cout << "ERROR reading from socket" << endl;
		return 0;
	}
	
	string messageName(buffer,256);

	size_t posName  = messageName.find("Name=");
	if (posName>0)
	{
		size_t posNameF = messageName.substr(posName+5,20).find(">");
		stringParams["object"] = messageName.substr(posName+5,posNameF);
		cout << "object:" << stringParams["object"] << endl;
	}
	
	size_t posAz    = message.find("curAz=");
	size_t posAlt   = message.find("curAlt=");
	size_t posTrack = message.find("trackTime=");
	size_t posRA    = message.find("curRA=");
	size_t posDec   = message.find("curDec=");
	size_t posDero  = message.find("curDero=");
	if (posTrack>0)
	{
		double trackTime;
		istringstream ( message.substr(posTrack+10,1) ) >> trackTime;
		if (trackTime>0)
		{	
			intParams["tracking"] = 1;
			cout << "The telescope is tracking" << endl;
			if (posRA>0)
			{
				doubleParams["RA"] = RAstringToDouble(message.substr(posRA+6,9));
				cout << "RA:" << doubleParams["RA"] << endl;
			}
			if (posDec>0)
			{
				doubleParams["dec"] = DecstringToDouble(message.substr(posDec+7,10));
				cout << "dec:" << doubleParams["dec"] << endl;
			}
		}
		else
		{
			intParams["tracking"] = 0;
			cout << "The telescope is not tracking" << endl;
			if (posAz>0)
			{
				istringstream ( message.substr(posAz+6,9) ) >> doubleParams["az"];
				cout << "Azimuth:" << doubleParams["az"] << endl;
			}
			if (posAlt>0)
			{
				istringstream ( message.substr(posAlt+7,9) ) >> doubleParams["alt"];
				cout << "Altitude:" << doubleParams["alt"] << endl;
			}
		}
	}

	if (posDero>0)
	{
		double deroAngle;
		istringstream ( message.substr(posDero+8,7) ) >> deroAngle;
		deroDifference = deroAngle - parallacticAngle();
		positionAngle = deroAngle - parallacticAngle() + doubleParams["referencePA"];
		while (positionAngle > 360) positionAngle -= 360.0;
		while (positionAngle < 0) positionAngle += 360.0;
		cout << "Dero:" << deroAngle << endl;
		cout << "positionAngle:" << positionAngle << endl;
	}
	return 1;
}

void printerror( int status)
{
	if (status)
	{
//		move(30,0);printw("cfitsio error %d", status);	
		cout << "cfitsio error:" << status << endl;
//		fits_report_error(stderr, status); /* print error report */
//		exit( status );    /* terminate the program, returning error status */
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
	if ((!isdigit(*it))&&((*it=='-')||(*it=='+'))) it++; // skip minus if any
	while (it != str.end() && std::isdigit(*it)) ++it;
	return !str.empty() && it == str.end();
}

bool is_double(string str)
{
	int dotCount = 0;
	string::iterator it = str.begin();
	if ((!isdigit(*it))&&((*it=='-')||(*it=='+'))) it++; // skip minus if any
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

int intCompare(const void * a, const void * b)
{
	return ( *(int*)a - *(int*)b );
}

int anglesProximity(double a, double b, double margin)
{
	
	if (a<margin)
	{
		if (b>270) b -= 360;
	}
	if (a>360-margin)
	{
		if (b<90) b += 360;
	}
	return (int)(fabs(a-b)<margin);
}

double RAstringToDouble(string inputstring)
{
	double RA;
	vector<string> RAtokens;
	getTokens(inputstring, ':', &RAtokens);
	if ( RAtokens.size() == 3 )
	{
		int hour,min;
		double sec;
		if ( is_integer( RAtokens[0] ) )
			istringstream ( RAtokens[0] ) >> hour;
		else
			cout << "wrong hour format" << endl;
		if ( is_integer( RAtokens[1] ) )
			istringstream ( RAtokens[1] ) >> min;
		else
			cout << "wrong min format" << endl;
		if ( is_double( RAtokens[2] ) )
			istringstream ( RAtokens[2] ) >> sec;
		else
			cout << "wrong sec format" << endl;
		RA = 15.0*(double)hour + 0.25*(double)min + 0.25*sec/60.0;
	}
	else
	{
		cout << "wrong RA format" << endl;
		RA = -1;
	}
	return RA;
}

double DecstringToDouble(string inputstring)
{
	double dec;
	vector<string> Dectokens;
	getTokens(inputstring, ':', &Dectokens);
	if ( Dectokens.size() == 3 )
	{
		int deg,min;
		double sec;
		
		// remove double minuses (OCS issue)
		size_t dMinusPos = Dectokens[0].find("--");

		if ( dMinusPos != string::npos ) Dectokens[0].replace(dMinusPos, 2, "-");
	
		if ( is_integer( Dectokens[0] ) )
			istringstream ( Dectokens[0] ) >> deg;
		else
			cout << "wrong deg format" << endl;
		if ( is_integer( Dectokens[1] ) )
			istringstream ( Dectokens[1] ) >> min;
		else
			cout << "wrong min format" << endl;
		if ( is_double( Dectokens[2] ) )
			istringstream ( Dectokens[2] ) >> sec;
		else
			cout << "wrong sec format" << endl;

		// remove sign from min and sec (OCS issue)
		if (min<0) min = min*-1;
		if (sec<0) sec = sec*-1;
		
		if ( Dectokens[0].compare(0,1,"-") == 0 )
		{
			dec = (double)deg - (double)min/60.0 - sec/3600.0;
		}
		else
		{
			dec = (double)deg + (double)min/60.0 + sec/3600.0;
		} 
	}
	else
		cout << "wrong dec format " << Dectokens[0] << endl;
	return dec;
}