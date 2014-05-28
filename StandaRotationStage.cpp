#include "StandaRotationStage.h"

using namespace std;

StandaRotationStage::StandaRotationStage()
{
	deviceName = "";
	device = device_undefined;
	microstepFrac = 8;
}

StandaRotationStage::~StandaRotationStage()
{
	if (device != device_undefined)
	{
		cout << "closing in destructor" << endl;
		if ((result = close_device( &device )) != result_ok)
			cout << "error closing device " << error_string( result ) << endl;
	}
}

void StandaRotationStage::printDeviceName()
{
	cout << "Dev:" << deviceName << endl;
}

int StandaRotationStage::initializeStage(string _deviceName, double _convSlope, double _convIntercept)
{
	if ( (deviceName.compare(_deviceName)==0) && (_convSlope==convSlope) && (_convIntercept==convIntercept) && (device!=device_undefined))
	{
		cout << "Stage is already here, no need of initialization" << endl;
		return 1;
	}

	deviceName = _deviceName;
	convSlope = _convSlope;
	convIntercept = _convIntercept;

	if (device != device_undefined)
	{
		cout << "Closing previosly open device." << endl;
		if ((result = close_device( &device )) != result_ok)
			cout << "error closing device " << error_string( result ) << endl;
	}

	cout << "Trying to open device" << endl;

	device = open_device( deviceName.c_str() );
	if (device == device_undefined)
	{
		cout << "error opening device\n" << endl;
		return 0;
	}

	cout << "Device opened" << endl;

	if ((result = get_status( device, &state )) != result_ok)
		cout << "error getting status: " << error_string( result ) << endl;
	print_state( &state );

	if ((result = get_engine_settings( device, &engine_settings )) != result_ok)
		cout << "error getting engine settings: " << error_string( result ) << endl;

	// activate backlash compensation
	engine_settings.EngineFlags |= ENGINE_ANTIPLAY;
	// microstep fraction 8
	engine_settings.MicrostepMode = MICROSTEP_MODE_FRAC_8;

	if ((result = set_engine_settings( device, &engine_settings )) != result_ok)
		cout << "error setting engine settings: " << error_string( result ) << endl;

	// do not stop at the borders (infinite motion)
	if ((result = get_edges_settings( device, &edges_settings )) != result_ok)
		cout << "error getting edges settings: " << error_string( result ) << endl;
	edges_settings.BorderFlags = 0;
	if ((result = set_edges_settings( device, &edges_settings )) != result_ok)
		cout << "error setting edges settings: " << error_string( result ) << endl;

	cout << "finding home" << endl;
	if ((result = command_home( device )) != result_ok)
		cout << "error finding home: " <<  error_string( result ) << endl;
	msec_sleep(300);

	do
	{
		if ((result = get_status( device, &state )) != result_ok)
			cout << "error getting status: " << error_string( result ) << endl;
		msec_sleep(50);
	} while ( state.MoveSts != 0 );

	cout << "done. Zeroing" << endl;
	if ((result = command_zero( device )) != result_ok)
		cout << "error zeroing: " << error_string( result ) << endl;
	cout << "done" << endl;

	return 1;
}

void print_state (status_t* state)
{
	cout << "speed: " << state->CurSpeed;
	cout << " pos: " << state->CurPosition;
	cout << " upos: " << state->uCurPosition;
	cout << " flags: " << state->Flags;
	if (state->Flags & STATE_ALARM)
		cout << " ALARM";
	if (state->Flags & STATE_ERRC)
		cout << " ERRC";
	if (state->Flags & STATE_ERRD)
		cout << " ERRD";
	cout << endl;
}

string error_string (result_t result)
{
	switch (result)
	{
		case result_error:				return "error";
		case result_not_implemented:	return "not implemented";
		case result_nodevice:			return "no device";
		default:						return "success";
	}
}
