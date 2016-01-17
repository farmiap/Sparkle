#include <cstdio>
//#include <fitsio.h>
#include "/home/safonov/cfitsio/include/fitsio.h"
#include <sys/stat.h>

#include "StepRotation.h"

using namespace std;

RotationTrigger::RotationTrigger(double _period)
{
	firstTime = 1;
	period = _period;
}

RotationTrigger::RotationTrigger()
{
	firstTime = 1;
}

RotationTrigger::~RotationTrigger()
{
}

void RotationTrigger::setPeriod(double _period)
{
	period = _period;
}

void RotationTrigger::start()
{
	struct timezone tz;
	gettimeofday(&startTime,&tz);
}

bool RotationTrigger::check(int *currentStep)
{
	struct timeval currTime;
	struct timezone tz;
	gettimeofday(&currTime,&tz);
	double deltaPrev = (double)(prevTime.tv_sec - startTime.tv_sec) + 1e-6*(double)(prevTime.tv_usec - startTime.tv_usec);
	double deltaCurr = (double)(currTime.tv_sec - startTime.tv_sec) + 1e-6*(double)(currTime.tv_usec - startTime.tv_usec);
	prevTime.tv_sec = currTime.tv_sec;
	prevTime.tv_usec = currTime.tv_usec;
	*currentStep = (int)(deltaCurr/period);
	if (firstTime) {
		firstTime = 0;
		return 0;
	} else {
		return ((int)(deltaPrev/period) != (int)(deltaCurr/period));
	}
}

AngleContainer::AngleContainer()
{
}

AngleContainer::~AngleContainer()
{
}

void AngleContainer::addStatusAndAngle(int _number,int _status,double _angle)
{
	numbers.push_back(_number);
	moved.push_back(_status);
	angles.push_back(_angle);
}

void AngleContainer::print()
{
	FILE *f=fopen("temp.dat","w");
	vector<int>::iterator itm = moved.begin();
	vector<int>::iterator itn = numbers.begin();
	for(vector<double>::iterator it=angles.begin();it!=angles.end();++it)
	{
		fprintf(f,"%d %d %f\n",*itn,*itm,*it);
		itm++;
		itn++;
	}
	fclose(f);

	FILE *f2=fopen("temp2.dat","w");
	vector<int>::iterator itb = intrvBegins.begin();
	vector<int>::iterator ite = intrvEnds.begin();
	for(vector<double>::iterator it=intrvAngles.begin();it!=intrvAngles.end();++it)
	{
		fprintf(f2,"%d %d %f\n",*itb,*ite,*it);
		itb++;
		ite++;
	}
	fclose(f2);
}

void AngleContainer::writePositionsToFits(char* filename, const char* extname)
{
	writePositionsToASCIITableFITS(moved.size(),filename,angles,extname);
}


void AngleContainer::writeIntervalsToFits(char* filename, const char* extname)
{
	writeIntervalsToASCIITableFITS(intrvBegins.size(),filename,intrvBegins,intrvEnds,intrvAngles,extname);
}

void AngleContainer::cleanStatus()
{
	// This function handles specifics of Standa stage.
	// Before start of motion (~300-400 ms before) it returns
	// status "accelerating" for a very short time instead of actually not moving.
	// This function removes such events from status vector.

	vector<int>::iterator itm = moved.begin();
	vector<double>::iterator itnext;
	vector<int>::iterator itmnext;
	for(vector<double>::iterator it=angles.begin();it!=angles.end();++it)
	{
		itnext = it;
		itnext++;
		itmnext = itm;
		itmnext++;
		if ( (*itm == 1) && (*itmnext == 0) && (*it==*itnext) )
			*itm = 0;
		itm++;
	}
}

void AngleContainer::convertToIntervals()
{
	intrvBegins.erase(intrvBegins.begin(),intrvBegins.end());
	intrvEnds.erase(intrvEnds.begin(),intrvEnds.end());
	intrvAngles.erase(intrvAngles.begin(),intrvAngles.end());

	vector<int>::iterator itm = moved.begin();
	vector<int>::iterator itn = numbers.begin();
	bool inInterval = 0;
	int intervalBegin;
	int intervalEnd;
	double intervalAngle;
	int counter = 1;
	for(vector<double>::iterator it=angles.begin();it!=angles.end();++it)
	{
		if (!inInterval)
		{
			if (*itm == 0)
			{
				intervalBegin = *itn;
				intervalAngle = *it;
				inInterval = 1;
			}
		}
		else
		{
			if (*itm == 1)
			{
				intervalEnd = *itn-1;
				intrvBegins.push_back(intervalBegin);
				intrvEnds.push_back(intervalEnd);
				intrvAngles.push_back(intervalAngle);
				inInterval = 0;
			}
		}
		itm++;
		itn++;
		counter++;
	}
	itn--;
	if (inInterval)
	{
		intervalEnd = *itn;
		intrvBegins.push_back(intervalBegin);
		intrvEnds.push_back(intervalEnd);
		intrvAngles.push_back(intervalAngle);
	}

}

void writePositionsToASCIITableFITS(int nrows, char* filename, vector<double> colData, const char *extname)
{
	fitsfile *fptr;       /* pointer to the FITS file, defined in fitsio.h */
	int status;
	
	char *ttype[1];
	ttype[0] = (char *)malloc(10*sizeof(char));
	sprintf(ttype[0],"VALUE");
	
	char *tform[1];
	tform[0] = (char *)malloc(10*sizeof(char));
	sprintf(tform[0],"E20.6");
	
	char *tunit[1];
	tunit[0] = (char *)malloc(10*sizeof(char));
	sprintf(tunit[0],"DEG");
			
	double *colArray;
	colArray = (double *)malloc( nrows * sizeof(double) );
	int counter = 0;
	for(vector<double>::iterator it=colData.begin();it!=colData.end();++it)
	{
		colArray[counter] = *it;
		counter++;
	}
	/* initialize pointers to the start of each row of the image */
	//	for( ii=1; ii<naxes[1]; ii++ )
	//		array[ii] = array[ii-1] + naxes[0];
	
	status = 0;         /* initialize status before calling fitsio routines */
	
	struct stat  buffer;	
	if ( stat(filename, &buffer) == 0 )
	{	
		if ( fits_open_file(&fptr, filename, READWRITE, &status) )
			printerror2( status );
	} 
	else
	{
		if ( fits_create_file(&fptr, filename, &status)) /* create new FITS file */
			printerror2( status );           /* call printerror if error occurs */
	}
	
	if ( fits_create_tbl(fptr,  ASCII_TBL, nrows, 1, ttype, tform, tunit, extname, &status) )
		printerror2( status );

	if ( fits_write_col(fptr, TDOUBLE, 1, 1, 0, nrows, colArray, &status) )
		printerror2( status );
		
	if ( fits_close_file(fptr, &status) )                /* close the file */
		printerror2( status );
		
	//	printf("FITS writed: %s\n",filename);

	free(ttype[0]);
	free(tform[0]);
	free(tunit[0]);
	
	return;
}


void writeIntervalsToASCIITableFITS(int nrows, char* filename, vector<int> col1data, vector<int> col2data, vector<double> col3data, const char *extname)
{
	fitsfile *fptr;       /* pointer to the FITS file, defined in fitsio.h */
	int status;

	char *ttype[3];
	ttype[0] = (char *)malloc(10*sizeof(char));
	ttype[1] = (char *)malloc(10*sizeof(char));
	ttype[2] = (char *)malloc(10*sizeof(char));
	sprintf(ttype[0],"BEGIN");
	sprintf(ttype[1],"END");
	sprintf(ttype[2],"VALUE");

	char *tform[3];
	tform[0] = (char *)malloc(10*sizeof(char));
	tform[1] = (char *)malloc(10*sizeof(char));
	tform[2] = (char *)malloc(10*sizeof(char));
	sprintf(tform[0],"I8");
	sprintf(tform[1],"I8");
	sprintf(tform[2],"E20.6");

	char *tunit[3];
	tunit[0] = (char *)malloc(10*sizeof(char));
	tunit[1] = (char *)malloc(10*sizeof(char));
	tunit[2] = (char *)malloc(10*sizeof(char));
	sprintf(tunit[0],"");
	sprintf(tunit[1],"");
	sprintf(tunit[2],"DEG");

//	int bitpix   =  FLOAT_IMG; /* 32-bit double pixel values       */
//	long naxis    =   2;  /* 2-dimensional image                            */
//	long naxes[2] = { nx, ny};   /* image is nx pixels wide by ny rows */

    /* allocate memory for the whole image */
//	array[0] = (long *)malloc( naxes[0] * naxes[1] * sizeof(long) );

	int *col1array;
	col1array = (int *)malloc( nrows * sizeof(int) );
	int counter = 0;
	for(vector<int>::iterator it=col1data.begin();it!=col1data.end();++it)
	{
		col1array[counter] = *it;
		counter++;
	}

	int *col2array;
	col2array = (int *)malloc( nrows * sizeof(int) );
	counter = 0;
	for(vector<int>::iterator it=col2data.begin();it!=col2data.end();++it)
	{
		col2array[counter] = *it;
		counter++;
	}

	double *col3array;
	col3array = (double *)malloc( nrows * sizeof(double) );
	counter = 0;
	for(vector<double>::iterator it=col3data.begin();it!=col3data.end();++it)
	{
		col3array[counter] = *it;
		counter++;
	}
    /* initialize pointers to the start of each row of the image */
//	for( ii=1; ii<naxes[1]; ii++ )
//		array[ii] = array[ii-1] + naxes[0];

	status = 0;         /* initialize status before calling fitsio routines */

	struct stat  buffer;	
	if ( stat(filename, &buffer) == 0 )
	{	
		if ( fits_open_file(&fptr, filename, READWRITE, &status) )
			printerror2( status );
	} 
	else
	{
		if ( fits_create_file(&fptr, filename, &status)) /* create new FITS file */
			printerror2( status );           /* call printerror if error occurs */
	}

	if ( fits_create_tbl(fptr,  ASCII_TBL, nrows, 3, ttype, tform, tunit, extname, &status) )
		printerror2( status );

	if ( fits_write_col(fptr, TINT, 1, 1, 0, nrows, col1array, &status) )
		printerror2( status );

	if ( fits_write_col(fptr, TINT, 2, 1, 0, nrows, col2array, &status) )
		printerror2( status );

	if ( fits_write_col(fptr, TDOUBLE, 3, 1, 0, nrows, col3array, &status) )
		printerror2( status );

	if ( fits_close_file(fptr, &status) )                /* close the file */
		printerror2( status );

//	printf("FITS writed: %s\n",filename);
	free(ttype[0]);
	free(ttype[1]);
	free(ttype[2]);

	free(tform[0]);
	free(tform[1]);
	free(tform[2]);

	free(tunit[0]);
	free(tunit[1]);
	free(tunit[2]);

	return;
}

void printerror2( int status)
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

