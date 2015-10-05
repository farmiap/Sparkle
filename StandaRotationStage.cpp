#include "StandaRotationStage.h"
#include <math.h>
#include <unistd.h>

#define ANGLEMARGIN 0.5

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
//		cout << "closing in destructor" << endl;
		if ((result = close_device( &device )) != result_ok)
			cout << "error closing device " << error_string( result ) << endl;
	}
}

void StandaRotationStage::printDeviceName()
{
	cout << "Dev:" << deviceName << endl;
}

int StandaRotationStage::setSpeed(double _speed)
{
	if (device == device_undefined)
	{
		cout << "no device: " << deviceName << endl;
		return 0;
	}

	
	cout << "setting speed!" << _speed << endl;
	if ((result = get_engine_settings( device, &engine_settings )) != result_ok)
	{
		cout << "error getting engine settings: " << error_string( result ) << endl;
		return 0;
	}
	engine_settings.NomSpeed = (int)(_speed/convSlope);
	if ((result = set_engine_settings( device, &engine_settings )) != result_ok)
	{
		cout << "error setting engine settings: " << error_string( result ) << endl;
		return 0;
	}
	return 1;
	
}

int StandaRotationStage::initializeStage(string _deviceName, double _convSlope, double _convIntercept, int _dirInv, double _speed)
{
	if ( (deviceName.compare(_deviceName)==0) && (_convSlope==convSlope) && (_convIntercept==convIntercept) && (_dirInv==directionInverted) && (_speed==speed) && (device!=device_undefined))
	{
//		cout << "Stage is already here, no need of initialization" << endl;
		return 1;
	}

	deviceName = _deviceName;
	convSlope = _convSlope;
	convIntercept = _convIntercept;
	directionInverted = _dirInv;
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
	// If direction is inverted, most of the time stage will rotate in negative direction, therefore negative antiplay will be needed rarely, and vice versa.
	if ( directionInverted ) 
		engine_settings.Antiplay = -20;
	else
		engine_settings.Antiplay = 20;
	
	// microstep fraction 8
//	engine_settings.MicrostepMode = MICROSTEP_MODE_FRAC_8;

	int newSpeed = (int)(speed/convSlope);
	engine_settings.NomSpeed = newSpeed;
	cout << "engine speed (steps/sec) " << newSpeed << ", eee " << convSlope << endl;
	 
	if ((result = set_engine_settings( device, &engine_settings )) != result_ok)
	{
		cout << "error setting engine settings: " << error_string( result ) << endl;
		return 0;
	}
	
	// do not stop at the borders (infinite motion)
/*	
	if ((result = get_edges_settings( device, &edges_settings )) != result_ok)
	{
		cout << "error getting edges settings: " << error_string( result ) << endl;
		return 0;
	}
	edges_settings.BorderFlags = 0;
	if ((result = set_edges_settings( device, &edges_settings )) != result_ok)
	{
		cout << "error setting edges settings: " << error_string( result ) << endl;
		return 0;
	}
	*/
/*
	home_settings_t home_settings;
	if ((result = get_home_settings( device, &home_settings )) != result_ok)
	{
		cout << "error getting home settings: " << error_string( result ) << endl;
		return 0;
	}
	home_settings.HomeFlags |= HOME_MV_SEC_EN;	
	home_settings.HomeFlags |= HOME_STOP_FIRST_LIM;
	if ((result = set_home_settings( device, &home_settings )) != result_ok)
	{
		cout << "error setting home settings: " << error_string( result ) << endl;
		return 0;
	}
*/	
	cout << "finding home" << endl;
	if ((result = command_home( device )) != result_ok)
	{
		cout << "error finding home: " <<  error_string( result ) << endl;
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
		msec_sleep(150);
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

int StandaRotationStage::startContiniousMotion()
{
	if (device == device_undefined)
	{
		cout << "no device: " << deviceName << endl;
		return 0;
	}

	
	if ( directionInverted )
	{
		if ((result = command_left( device )) != result_ok)
			cout << "error command move " << error_string( result ) << endl;
	}
	else
	{
		if ((result = command_right( device )) != result_ok)
			cout << "error command move " << error_string( result ) << endl;
	}
	return 1;
}

int StandaRotationStage::stopContiniousMotion()
{
	if (device == device_undefined)
	{
		cout << "no device: " << deviceName << endl;
		return 0;
	}

	if ((result = command_sstp( device )) != result_ok)
		cout << "error command move " << error_string( result ) << endl;
	return 1;
}

int StandaRotationStage::startMoveToAngleWait(double targetAngle)
{
	if (device == device_undefined)
	{
		cout << "no device: " << deviceName << endl;
		return 0;
	}

	
	startMoveToAngle(targetAngle);
		
	int isMovingFlag=1;
	double currentAngle=-999999999.9;
	usleep(500000);
	while ( isMovingFlag || ( !anglesProximityR(currentAngle,targetAngle,ANGLEMARGIN) ) )
	{
		getAngle(&isMovingFlag,&currentAngle);
		usleep(100000);
	}
}
	
int StandaRotationStage::startMoveToAngle(double targetAngle)
{
	if (device == device_undefined)
	{
		cout << "no device: " << deviceName << endl;
		return 0;
	}

	
	if ( directionInverted )
		targetAngle = 360.0 - targetAngle;
	
	if ((result = get_status( device, &state )) != result_ok)
	{
		cout << "error getting status: " << error_string( result ) << endl;
		return 0;
	}
	double cycle = 360.0/convSlope;

	double dpos = convIntercept + targetAngle/convSlope;
	int pos = (int)dpos;
	int uPos = (int)(microstepFrac*(dpos-(double)pos));

	pos = pos + (int)(cycle*round((state.CurPosition - pos)/cycle));

//	cout << "move to pos " << pos << " upos " << uPos << endl;
	
	if ((result = command_move( device, pos, uPos )) != result_ok)
		cout << "error command move " << error_string( result ) << endl;
	return 1;
}

int StandaRotationStage::startMoveByAngle(double deltaAngle)
{
	if (device == device_undefined)
	{
		cout << "no device: " << deviceName << endl;
		return 0;
	}

	
	if ( directionInverted )
		deltaAngle = -deltaAngle;
	double dpos = deltaAngle/convSlope;
	int pos = (int)dpos;
	int uPos = (int)(microstepFrac*(dpos-(double)pos));
	if ((result = command_movr( device, pos, uPos )) != result_ok)
		cout << "error command move " << error_string( result ) << endl;
//	cout << "motion started" << endl;
	return 1;
}

int StandaRotationStage::getAngle(int *isMoving,double *angle)
{
	if (device == device_undefined)
	{
		cout << "no device: " << deviceName << endl;
		return 0;
	}

	
	if ((result = get_status( device, &state )) != result_ok)
	{
		cout << "error getting status: " << error_string( result ) << endl;
		return 0;
	}
	double position = (double)state.CurPosition + ((double)state.uCurPosition)/(double)microstepFrac;
	*angle = (position - convIntercept)*convSlope;
	*angle = *angle-360.0*floor(*angle/360.0);
	if (directionInverted)
		*angle = 360.0 - *angle;
	*isMoving = state.MoveSts;
	return 1;
}

double getNextStepValue(int currentStepNumber, double refStepValue, int pairNum, int groupNum)
{
	if (currentStepNumber%(pairNum*2) == 0)
		return refStepValue/(double)groupNum;
	else
		return refStepValue;
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

int anglesProximityR(double a, double b, double margin)
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


