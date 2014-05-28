#include "StandaRotationStage.h"

using namespace std;

StandaRotationStage::StandaRotationStage()
{

}

StandaRotationStage::~StandaRotationStage()
{

}

int StandaRotationStage::initializeStage(string _deviceName, double _convSlope, double _convIntercept)
{
	if ( (deviceName.compare(_deviceName)==0) && (_convSlope==convSlope) && (_convIntercept==convIntercept) )
	{
		cout << "Stage is already here, no need of initialization" << endl;
	}
/*
	device = open_device( device_name );
	if (device == device_undefined)
	{
		wprintf( L"error opening device\n" );
		break;
	}
*/
	return 1;
}
