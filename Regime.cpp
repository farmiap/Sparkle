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

Regime::Regime(int _withDetector,int _withHWPMotor,StandaRotationStage *_HWPMotor)
{
	withDetector = _withDetector;
	withHWPMotor = _withHWPMotor;

	HWPMotor = _HWPMotor;

	intParams["skip"] = 1;
	intParams["numKin"]  = 10;      // kinetic cycles
	intParams["shutter"] = 0;       // 0 - close, 1 - open
	intParams["ft"] = 1;            // frame transfer: 0 - disabled, 1 - enabled
	intParams["adc"] = 1;          // A/D channel: 0 - 14-bit, 1 - 16-bit
	intParams["ampl"] = 1;          // amplifier: 0 - EM, 1 - conventional
	intParams["horSpeed"] = 0;      // horisontal speed: conv: 0 - 3 MHz; EM 0 - 10 MHz, 1 - 5 MHz, 2 - 3 MHz
	intParams["preamp"] = 0;        // preamplifier
	intParams["vertSpeed"] = 1;     // vertical clocking speed (0 - 0.3, 1 - 0.5, 2 - 0.9, 3 - 1.7, 4 - 3.3) \mu s
	intParams["vertAmpl"] = 3;      // vertical clocking voltage amplitude 0 - 4
	intParams["temp"] = -1.0;	    // temperature
	intParams["EMGain"] = 1;      // EM gain

	intParams["imLeft"] = 1;        // image left side
	intParams["imRight"] = 512;     // image right side
	intParams["imBottom"] = 1;      // image bottom side
	intParams["imTop"] = 512;       // image top side

	doubleParams["exp"] = 0.1;      // exposure

	// HWP rotation unit section
	stringParams["HWPDevice"] = "";
	intParams["HWPPairNum"] = 1; // number of pairs in group
	intParams["HWPGroupNum"] = 1; // number of groups
	doubleParams["HWPStep"] = 10; // step of HWP P.A. (degrees)
	doubleParams["HWPStart"] = 0; // HWP P.A. (degrees)
	doubleParams["HWPIntercept"] = 0.0; // engine position when P.A. is zero (steps)
	doubleParams["HWPSlope"] = 0.015; // degrees per engine step
	doubleParams["HWPPeriod"] = 10.0; // period between swithes (seconds)

	pathesCommands["fitsname"] = FITSNAME;
	pathesCommands["fitsdir"] = FITSDIR;
	pathesCommands["rtaname"] = RTANAME;

	actionCommands["acq"] = ACQUIRE;
	actionCommands["prta"] = RUNTILLABORT;
	actionCommands["prtaf"] = MAXFLUX;
	actionCommands["prtas"] = RTASPOOL;
	actionCommands["tim"] = GETTIMINGS;
	actionCommands["testnc"] = TESTNCURSES;

	commandHintsFill();

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

	if ( tokens.size() == 2 )
	{
		switch ( pathesCommands[tokens[0]] )
		{
		case FITSNAME:
			active = FALSE;
			pathes.setFits(tokens[1]);
			return 1;
		case FITSDIR:
			active = FALSE;
			pathes.setDir(tokens[1]);
			return 1;
		case RTANAME:
			active = FALSE;
			pathes.setRTA(tokens[1]);
			return 1;
		default:
			break;
		}
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
		if (( tokens.size() == 2) && ( is_integer(tokens[1]) ))
		{
			int value;
			istringstream ( tokens[1] ) >> value;
			intParams[tokens[0]] = value;
			active = FALSE;
		}
		else if ( commandHints.count( tokens[0] ) > 0 )
			cout << commandHints[tokens[0]] << endl;
		else
			cout << "error: unknown parameter" << tokens[0] << endl;
	}
	else if ( doubleParams.count(tokens[0]) > 0 )
	{
		if (( tokens.size() == 2) && ( is_double(tokens[1]) ))
		{
			double value;
			istringstream ( tokens[1] ) >> value;
			doubleParams[tokens[0]] = value;
			active = FALSE;
		}
		else if ( commandHints.count( tokens[0] ) > 0 )
			cout << commandHints[tokens[0]] << endl;
		else
			cout << "error: unknown parameter" << tokens[0] << endl;
	}
	else if ( stringParams.count(tokens[0]) > 0 )
	{
		if ( tokens.size() == 2)
		{
			stringParams[tokens[0]] = tokens[1];
			active = FALSE;
		}
		else if ( commandHints.count( tokens[0] ) > 0 )
			cout << commandHints[tokens[0]] << endl;
		else
			cout << "error: unknown parameter" << tokens[0] << endl;
	}
	else
		cout << "error: unknown parameter"  << tokens[0] << endl;


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

int Regime::validate()
{
	if (!pathes.validate())
	{
		cout << "pathes validation failed" << endl;
		return 0;
	}

	if ((intParams["skip"] < 1) || (intParams["skip"] > 1000))
	{
		cout << "rta skip validation failed" << endl;
		return 0;
	}

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

	if (( doubleParams["exp"] < 0.001 ) || ( doubleParams["exp"] > 1000.0 ))
	{
		cout << "exposure time validation failed" << endl;
		return 0;
	}

	cout << "validation successful" << endl;

	return 1;
}

void Regime::commandHintsFill()
{
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

	commandHints["EMGain"]  = "EM gain: 1-1000";
	commandHints["temp"]     = "target sensor temperature: -80 - 0C";
	commandHints["exp"]     = "exposure time in seconds";

	commandHints["HWPDevice"]  = "HWP motor device id (e.g. /dev/ximc/00000367)";
	commandHints["HWPPairNum"] = "number of pairs in group: >= 1";
	commandHints["HWPGroupNum"] = "number of groups: >= 1";
	commandHints["HWPStep"] = "step of HWP P.A. (degrees): > 0.0";
	commandHints["HWPStart"] = "HWP P.A. start (degrees): > 0.0";
	commandHints["HWPIntercept"] = "engine position when P.A. is zero (steps): > 0.0";
	commandHints["HWPSlope"] = "degrees per engine step: > 0.0";
	commandHints["HWPPeriod"] = "period between swithes (seconds): > 0.0";

	commandHints["acq"]     = "start acquisition";
	commandHints["prta"]    = "start run till abort";
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
		if ( status == DRV_SUCCESS ) status = SetImage(1,1,intParams["imLeft"],intParams["imRight"],intParams["imBottom"],intParams["imTop"]);
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

	int HWPStatus = 0;

	if ( withHWPMotor )
	{
		HWPStatus = HWPMotor->initializeStage(stringParams["HWPDevice"],doubleParams["HWPSlope"],doubleParams["HWPIntercept"]);
	}

	return HWPStatus;
}

bool Regime::runTillAbort(bool avImg, bool doSpool)
{
	cout << "entering RTA " << endl;
	if ( !active )
	{
		cout << "This regime is not applied, run rapp" << endl;
		return false;
	}

	if ( doubleParams["exp"] > 0.5 )
	{
		cout << "run till abort is possible only if exposure < 0.5 s" << endl;
		return false;
	}

	int status = DRV_SUCCESS;

	int width, height;
	width = intParams["imRight"]-intParams["imLeft"]+1;
	height = intParams["imTop"]-intParams["imBottom"]+1;
	long datasize = width*height;

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

	initscr();
	raw();
	noecho();

	int ch;
	nodelay(stdscr, TRUE);

	if (withDetector)
	{
		if ( status == DRV_SUCCESS ) status = SetAcquisitionMode(5); // run till abort
		if ( status == DRV_SUCCESS ) status = SetSpool(doSpool,5,(char*)pathes.getSpoolPath(),10); // disable spooling
		if ( status == DRV_SUCCESS ) status = StartAcquisition();
	}

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
		return false;
	}

	int counter=0;
	int HWPisMoving;
	int HWPisMovingPrev=1;
	int motionStarted=0;
	double HWPAngle;

	HWPMotor->startMoveToAngle(doubleParams["HWPStart"]);
	msec_sleep(200.0);
	do
	{
		HWPMotor->getAngle(&HWPisMoving,&HWPAngle);
//		cout << "is moving" << HWPisMoving << "angle" << HWPAngle << endl;
		msec_sleep(50.0);
	} while ( HWPisMoving );
	msec_sleep(1000.0);
	HWPRotationTrigger HWPTrigger(doubleParams["HWPPeriod"]);
	if (withHWPMotor)
		HWPTrigger.start();
	HWPAngleContainer angleContainer;

	while ( 1 ) {
		ch = getch();
	        if ( (ch=='q') || (ch=='x') ) {
	        	if (( withDetector ) && ( status == DRV_SUCCESS )) status = AbortAcquisition();
			break;
		}
		else
		{
			if ( withHWPMotor ) {
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
						printw("ATTENTION: HWP stage is skipping steps",counter);
					}
				}
				else
				{
					motionStarted = 0;
				}
			}
			if ( withDetector )
			{
				if ( status == DRV_SUCCESS ) status = WaitForAcquisitionTimeOut(1000);
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
					printw(" frame no.: %d",counter);
				}
			}
			else
			{
				move(1,0);
				printw(" frame no.: %d, angle %f",counter,HWPAngle);
				msec_sleep(3.0);
			}
			// Logic: if HWP was moving in the end of previous step, it moved also during current step.
			if ( withHWPMotor )
			{
				angleContainer.addStatusAndAngle((int)(HWPisMovingPrev!=0),HWPAngle);
				if (motionStarted)
					HWPisMovingPrev = 1;
				else
					HWPisMovingPrev = HWPisMoving;
			}
			counter++;
		}
	}
	werase(stdscr);
	nodelay(stdscr, FALSE);
	refresh();
	endwin();

	if ( withHWPMotor )
	{
		cout << "writing HWP angle data" << endl;
		angleContainer.cleanStatus();
		angleContainer.convertToIntervals();
//		angleContainer.print();
		angleContainer.writeToFits((char*)pathes.getIntrvPath());
	}

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
			usleep(300000);
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



bool finalize(int _withDetector,int _withHWPMotor,float startTemp)
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
