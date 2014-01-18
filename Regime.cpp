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

#define TEMP_MARGIN 3.0             // maximum stabilized temperature deviation from required

using namespace std;

Regime::Regime()
{
	intParams["rtaSkip"] = 1;
	intParams["numKin"]  = 10;      // kinetic cycles
	intParams["shutter"] = 0;       // 0 - close, 1 - open
	intParams["ft"] = 1;            // frame transfer: 0 - disabled, 1 - enabled
	intParams["ampl"] = 1;          // amplifier: 0 - EM, 1 - conventional
	intParams["horSpeed"] = 0;      // horisontal speed: conv: 0 - 3 MHz; EM 0 - 10 MHz, 1 - 5 MHz, 2 - 3 MHz
	intParams["preamp"] = 0;        // preamplifier
	intParams["vertSpeed"] = 1;     // vertical clocking speed (0 - 0.3, 1 - 0.5, 2 - 0.9, 3 - 1.7, 4 - 3.3) \mu s
	intParams["vertAmpl"] = 3;      // vertical clocking voltage amplitude 0 - 4
	intParams["temp"] = -1.0;	    // temperature
	intParams["EMgain"] = 1;      // EM gain

	intParams["imLeft"] = 1;        // image left side
	intParams["imRight"] = 512;     // image right side
	intParams["imBottom"] = 1;      // image bottom side
	intParams["imTop"] = 512;       // image top side

	doubleParams["exp"] = 0.1;      // exposure

	pathesCommands["fitsname"] = FITSNAME;
	pathesCommands["fitsdir"] = FITSDIR;
	pathesCommands["rtaname"] = RTANAME;

	actionCommands["acq"] = ACQUIRE;
	actionCommands["prta"] = RUNTILLABORT;
	actionCommands["prtaf"] = MAXFLUX;
	actionCommands["tim"] = GETTIMINGS;

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
				runTillAbort(false);
				break;
			case MAXFLUX:
				runTillAbort(true);
				break;
			case GETTIMINGS:
				printTimings();
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
			cout << "error: unknown parameter" << endl;
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
			cout << "error: unknown parameter" << endl;
	}
	else
		cout << "error: unknown parameter" << endl;


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

	if ((intParams["rtaSkip"] < 1) || (intParams["rtaSkip"] > 1000))
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

	if (( intParams["temp"] > 0 ) || ( intParams["temp"] < -80 ))
	{
		cout << "temperature validation failed" << endl;
		return 0;
	}
	else
	{
		if ( !checkTempInside(intParams["temp"]-TEMP_MARGIN,intParams["temp"]-TEMP_MARGIN) )
		{
			cout << "warning: temperature of sensor is not consistent with setting or not stabilized"  << endl;
			cout << "it will be set during regime application" << endl;
		}
	}

	if ( intParams["ampl"] == 1 )
	{
		if ( intParams["horSpeed"] != 0 )
		{
			cout << "horizontal speed validation failed" << endl;
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

		if (( intParams["EMGain"] < 1 ) || ( intParams["temp"] > 1000 ))
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
	commandHints["rtaSkip"] = "RTA mode: period of writing frame to disk: 1 - every frame is written, 2 - every other frame is written, etc";
	commandHints["numKin"]  = "number of kinetic cycles in series";
	commandHints["shutter"] = "shutter: 0 - close, 1 - open";
	commandHints["ft"]      = "frame transfer: 0 - disabled, 1 - enabled";
	commandHints["ampl"]    = "amplifier: 0 - EM, 1 - conventional";
	commandHints["horSpeed"]= "horisontal speed: conv ampl: 0 - 3 MHz; EM ampl: 0 - 10 MHz, 1 - 5 MHz, 2 - 3 MHz";
	commandHints["preamp"]  = "preamplifier: 0, 1, 2. for values see performance sheet";
	commandHints["vertSpeed"] = "vertical clocking speed (0 - 0.3, 1 - 0.5, 2 - 0.9, 3 - 1.7, 4 - 3.3) mu s";
	commandHints["vertAmpl"]= "vertical clocking voltage amplitude: 0 - 4";

	commandHints["imLeft"]  = "image left side: 1-512";
	commandHints["imRight"] = "image right side: 1-512";
	commandHints["imBottom"]= "image bottom side: 1-512";
	commandHints["imTop"]   = "image top side: 1-512";

	commandHints["EMgain"]  = "EM gain: 1-1000";
	commandHints["temp"]     = "target sensor temperature: -80 - 0C";
	commandHints["exp"]     = "exposure time in seconds";

	commandHints["acq"]     = "start acquisition";
	commandHints["prta"]    = "start run till abort";
}


int Regime::apply()
{
	unsigned int status = DRV_SUCCESS;
	if ( !checkTempInside(intParams["temp"]-TEMP_MARGIN,intParams["temp"]-TEMP_MARGIN) )
	{
		status = (setTemp(intParams["temp"]))?DRV_SUCCESS:DRV_P1INVALID;
	}

	cout << "temperature is ok, setting..." << endl;

	int width, height;
	if ( status == DRV_SUCCESS ) status = GetDetector(&width,&height);
	if ( status == DRV_SUCCESS ) status = SetShutter(1,(intParams["shutter"]==1)?1:2,50,50);
	if ( status == DRV_SUCCESS ) status = SetFrameTransferMode(intParams["ft"]);
	if ( status == DRV_SUCCESS ) status = SetTriggerMode(0); // internal trigger
	if ( status == DRV_SUCCESS ) status = SetReadMode(4); // image
	if ( status == DRV_SUCCESS ) status = SetExposureTime((float)doubleParams["exp"]);
	if ( status == DRV_SUCCESS ) status = SetADChannel(intParams["ampl"]);
	if ( status == DRV_SUCCESS ) status = SetHSSpeed(intParams["ampl"],intParams["horSpeed"]);
	if ( status == DRV_SUCCESS ) status = SetPreAmpGain(intParams["preamp"]);
	if ( status == DRV_SUCCESS ) status = SetVSSpeed(intParams["vertSpeed"]);
	if ( status == DRV_SUCCESS ) status = SetVSAmplitude(intParams["vertAmpl"]);
	if ( intParams["ampl"] == 0 )
	{
		if ( intParams["EMgain"] < 2 )
		{
			SetEMGainMode(0);
		SetEMCCDGain(0);
		}
		else
		{
			SetEMAdvanced(intParams["EMgain"] > 300);
			SetEMGainMode(3);
			SetEMCCDGain(intParams["EMgain"]);
		}
	}
	if ( status == DRV_SUCCESS ) status = SetImage(1,1,intParams["imLeft"],intParams["imRight"],intParams["imBottom"],intParams["imTop"]);
	if ( status == DRV_SUCCESS )
	{
		active = TRUE;
		cout << "regime has been set successfully " << status << endl;
		return 1;
	}
	else
	{
		cout << "error: regime application failed " << status << endl;
		return 0;
	}
}

bool Regime::runTillAbort(bool avImg)
{
	if ( !active )
	{
		cout << "This regime is not applied, run rapp" << endl;
		return false;
	}

	int width, height;

	unsigned int status = GetDetector(&width, &height);

	long datasize=width*height;

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
	cbreak();

	int ch;
	nodelay(stdscr, TRUE);

	if ( status == DRV_SUCCESS ) status = SetAcquisitionMode(5); // run till abort
	if ( status == DRV_SUCCESS ) status = SetSpool(0,5,(char*)pathes.getSpoolPath(),10); // disable spooling
	if ( status == DRV_SUCCESS ) status = StartAcquisition();

	if ( status == DRV_SUCCESS ) {
		move(0,0);
		printw("Run till abort started (press any key to interrupt)");
	} else {
		move(0,0);
		printw("Run till abort failed");
		nodelay(stdscr, FALSE);
		endwin();
		return false;
	}

	int counter=0;
	while ( 1 ) {
		if ((ch = getch()) != ERR)
		{
			if ( status == DRV_SUCCESS ) status = AbortAcquisition();
				break;
		}
		else
		{
			if ( status == DRV_SUCCESS ) status = WaitForAcquisitionTimeOut(1000);
			if ( avImg )
			{
				if (status==DRV_SUCCESS) status=GetMostRecentImage(data,datasize);
				imageAverager.uploadImage(data);
				move(2,0);
				printw("maximum of running average images: ");
				for(map<int, double>::iterator it=imageAverager.maximums.begin(); it!=imageAverager.maximums.end(); ++it)
				{
					printw("frames: %d max: %.0f; ",it->first,it->second);
				}
			}
			if ( counter%intParams["rtaSkip"] == 0)
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
			counter++;
		}
	}
	erase();
	nodelay(stdscr, FALSE);
	endwin();

	return true;
}

bool Regime::acquire()
{
	if ( !active )
	{
		cout << "This regime is not applied, run rapp" << endl;
		return false;
	}

	initscr();
	cbreak();

	int ch;
	nodelay(stdscr, TRUE);

	unsigned int status = DRV_SUCCESS;

	if ( status == DRV_SUCCESS ) status = SetAcquisitionMode(3); // run till abort
	if ( status == DRV_SUCCESS ) status = SetNumberKinetics(intParams["numKin"]);
	if ( status == DRV_SUCCESS ) status = SetSpool(1,5,(char*)pathes.getSpoolPath(),10); // disable spooling
	if ( status == DRV_SUCCESS ) status = StartAcquisition();

	if ( status == DRV_SUCCESS ) {
		move(0,0);
		printw("Kinetic series started (press any key to interrupt)");
	} else {
		move(0,0);
		printw("Kinetic series failed");
		nodelay(stdscr, FALSE);
		endwin();
		return false;
	}

	while ( 1 ) {
		if ((ch = getch()) != ERR)
		{
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
	erase();
	nodelay(stdscr, FALSE);
	endwin();

	return true;
}

bool Regime::printTimings()
{
	if ( !active )
	{
		cout << "This regime is not applied, run rapp" << endl;
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

bool setTemp(double temper)
{
	initscr();
	cbreak();

	bool answer = 0;
	bool stabilized = false;
	int ch;
	float temp;
//	printw("Temperature is not consistent with setting or not stabilized.");
	printw("Starting setting temperature, press any key to interrupt.");

	nodelay(stdscr, TRUE);
	move(0,0);
	CoolerON();
	SetTemperature((int)temper);
	while ( !stabilized )
	{
		usleep(500000);
		if ((ch = getch()) != ERR) {

			unsigned int state=GetTemperatureF(&temp);
			SetTemperature((int)temp); // hold at temperature in the moment of stop
			answer = false;
			break;
		} else {
			unsigned int state=GetTemperatureF(&temp);
			move(1,0);
			printw("Current temperature: %g",temp);
			move(2,0);
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
		}
	}
	nodelay(stdscr, FALSE);
	erase();
	endwin();

	return answer;
}

bool checkTempInside(double lowerLim, double upperLim)
{
	float temp;
	unsigned int state=GetTemperatureF(&temp);
	return (( lowerLim < temp ) && (temp < upperLim) && ( state==DRV_TEMPERATURE_STABILIZED ));
}



bool finalize(float startTemp)
{
	if (!checkTempInside(startTemp-15.0,startTemp+15.0))
		if (!setTemp(startTemp-10.0))
			return false; // in case temperature cannot be set, don't quit

	ShutDown();
	return true;
}

void doFits(int nx, int ny, char* filename,at_32 *data)
{
	fitsfile *fptr;       /* pointer to the FITS file, defined in fitsio.h */
	int status, ii, jj;
	long  fpixel, nelements, exposure;
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
