#include "StandaActuator.h"
#include "StandaRotationStage.h"
#include <math.h>

using namespace std;

StandaActuator::StandaActuator()
{
	deviceName = "";
	device = device_undefined;
//	microstepFrac = 8;
}

StandaActuator::~StandaActuator()
{
	if (device != device_undefined)
	{
		cout << "closing in destructor" << endl;
		if ((result = close_device( &device )) != result_ok)
			cout << "error closing device " << error_string( result ) << endl;
	}
}

void StandaActuator::printDeviceName()
{
	cout << "Dev:" << deviceName << endl;
}

int StandaActuator::initializeActuator(string _deviceName, double _speed)
{
	if ( (deviceName.compare(_deviceName)==0) && (_speed==speed) && (device!=device_undefined))
	{
		cout << "Stage is already here, no need of initialization" << endl;
		return 1;
	}
	deviceName = _deviceName;
	speed = _speed;
	
	if (device != device_undefined)
	{
		cout << "Closing previosly open device." << endl;
		if ((result = close_device( &device )) != result_ok)
			cout << "error closing device " << error_string( result ) << endl;
	}

	cout << "Trying to open device" << _deviceName << endl;

	device = open_device( deviceName.c_str() );
	if (device == device_undefined)
	{
		cout << "error opening device\n" << _deviceName << endl;
		return 0;
	}

	cout << "Device opened" << endl;

	if ((result = get_status( device, &state )) != result_ok)
	{
		cout << "error getting status: " << error_string( result ) << endl;
		return 0;
	}
	print_state( &state );

	if ((result = get_engine_settings( device, &engine_settings )) != result_ok)
	{
		cout << "error getting engine settings: " << error_string( result ) << endl;
		return 0;
	}

	// activate backlash compensation
	engine_settings.EngineFlags |= ENGINE_ANTIPLAY;
	// microstep fraction 8
	engine_settings.MicrostepMode = MICROSTEP_MODE_FRAC_8;
	
	if ((result = set_engine_settings( device, &engine_settings )) != result_ok)
	{
		cout << "error setting engine settings: " << error_string( result ) << endl;
		return 0;
	}

	cout << "finding positive limit" << endl;
	if ((result = command_right( device )) != result_ok)
	{
		cout << "error finding limit: " <<  error_string( result ) << endl;
		return 0;
	}
	msec_sleep(300);

	do
	{
		if ((result = get_status( device, &state )) != result_ok)
		{
			cout << "error getting status: " << error_string( result ) << endl;
			return 0;
		}
		msec_sleep(50);
	} while ( state.MoveSts != 0 );

	cout << "done. Zeroing" << endl;
	if ((result = command_zero( device )) != result_ok)
	{
		cout << "error zeroing: " << error_string( result ) << endl;
		return 0;
	}
	cout << "done" << endl;

	return ( result==result_ok );
}



int StandaActuator::startMoveToPosition(int targetPosition)
{
	
	if ((result = get_status( device, &state )) != result_ok)
	{
		cout << "error getting status: " << error_string( result ) << endl;
		return 0;
	}

	if ((result = command_move( device, targetPosition, 0)) != result_ok)
		cout << "error command move " << error_string( result ) << endl;
	return 1;
}

int StandaActuator::getPosition(int *isMoving,int *position)
{
	if ((result = get_status( device, &state )) != result_ok)
	{
		cout << "error getting status: " << error_string( result ) << endl;
		return 0;
	}
	*position = (int)state.CurPosition;
	*isMoving = state.MoveSts;
	return 1;
}

