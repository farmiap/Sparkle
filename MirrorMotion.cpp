#include <cstdio>
#include <fitsio.h>
#include <sys/stat.h>
#include <ncurses.h>

#include "MirrorMotion.h"

using namespace std;

MirrorMotionRTA::MirrorMotionRTA(StandaActuator *_mirrorActuator,int _mirrorPosOff,int _mirrorPosLinpol, double _beamTime)
{
	mirrorActuator  = _mirrorActuator;
	mirrorPosOff    = _mirrorPosOff;
	mirrorPosLinpol = _mirrorPosLinpol;
	beamTime = _beamTime;
	
	isOnStart = 1;
	isMovingOn = 0;
	isOnFinish = 0;
	isMovingOff = 0;

	currIntrv = 0;
	intervals.resize(currIntrv+1);
	intervals[currIntrv].resize(3);
	intervals[currIntrv][0] = 0;
	intervals[currIntrv][2] = 1;
	
	struct timezone tz;
	gettimeofday(&startTime,&tz);	
}

MirrorMotionRTA::~MirrorMotionRTA()
{
}

int MirrorMotionRTA::process(int count,int exitRequested)
{
	int isMovingFlag;
	int currentPosition;
	
	struct timeval currTime;
	struct timezone tz;
	double deltaTime,deltaTime2;
	gettimeofday(&currTime,&tz);	
	deltaTime = (double)(currTime.tv_sec - startTime.tv_sec) + 1e-6*(double)(currTime.tv_usec - startTime.tv_usec);
	deltaTime2 = (double)(currTime.tv_sec - finishTime.tv_sec) + 1e-6*(double)(currTime.tv_usec - finishTime.tv_usec);
	
	if ( exitRequested )
	{
		if ( deltaTime < beamTime )
			return 1; // 1
		else
		{
			if ( !isMovingOn && !isOnFinish )
			{
				mirrorActuator->startMoveToPosition(mirrorPosLinpol);
				isMovingOn = 1;
				intervals[currIntrv][1] = count;
				currIntrv++;
				intervals.resize(currIntrv+1);
				intervals[currIntrv].resize(3);
			}
			else
			{
				mirrorActuator->getPosition(&isMovingFlag,&currentPosition);
				if ( (isOnFinish==0) && (isMovingFlag==0) && ( abs(currentPosition-mirrorPosLinpol)<100 ) )
				{
					gettimeofday(&finishTime,&tz);	
					isMovingOn = 0;
					isOnFinish = 1;
					intervals[currIntrv][0] = count;
					intervals[currIntrv][2] = 1;
					return 0;
				}
			}
			if ( ( isOnFinish ) && ( deltaTime2 > beamTime ) )
			{
				intervals[currIntrv][1] = count;
				return 1; // 1
			}
			else
			{
				return 0;
			}
		}
	}
	else
	{
		if ( ( deltaTime > beamTime ) && isOnStart )
		{
			mirrorActuator->startMoveToPosition(mirrorPosOff);
			isMovingOff = 1;
			isOnStart = 0;
			intervals[currIntrv][1] = count;
			currIntrv++;
			intervals.resize(currIntrv+1);
			intervals[currIntrv].resize(3);			
		}
		else
		{
			mirrorActuator->getPosition(&isMovingFlag,&currentPosition);
			if ( isMovingOff && !isMovingFlag && ( abs(currentPosition-mirrorPosOff)<100 ) )
			{
				intervals[currIntrv][0] = count;
				intervals[currIntrv][2] = 0;
				isMovingOff = 0;
			}
		}
		return 0;
	}
}

void MirrorMotionRTA::print()
{
	for(vector<vector<int> >::iterator it=intervals.begin();it!=intervals.end();++it)
	{
		cout << "begin: " << (*it)[0] << " end: " << (*it)[1] << " status: " << (*it)[2] << endl; 
	}
}

void MirrorMotionRTA::writeIntervalsToFits(char* filename)
{
	writeMirrorIntervalsToASCIITableFITS(filename, intervals);
}

void writeMirrorIntervalsToASCIITableFITS(char* filename, vector<vector<int> > data)
{
	fitsfile *fptr;       /* pointer to the FITS file, defined in fitsio.h */
	int status;
	
	int nrows = data.size();
	
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
	sprintf(tform[2],"I8");
	
	char *tunit[3];
	tunit[0] = (char *)malloc(10*sizeof(char));
	tunit[1] = (char *)malloc(10*sizeof(char));
	tunit[2] = (char *)malloc(10*sizeof(char));
	sprintf(tunit[0],"");
	sprintf(tunit[1],"");
	sprintf(tunit[2],"MIRROR");
	
	char extname[100];
	sprintf(extname,"MIRRORINTERVALS");
	
	//	int bitpix   =  FLOAT_IMG; /* 32-bit double pixel values       */
	//	long naxis    =   2;  /* 2-dimensional image                            */
	//	long naxes[2] = { nx, ny};   /* image is nx pixels wide by ny rows */
	
	/* allocate memory for the whole image */
	//	array[0] = (long *)malloc( naxes[0] * naxes[1] * sizeof(long) );
	
	int *col1array,*col2array,*col3array;
	col1array = (int *)malloc( nrows * sizeof(int) );
	col2array = (int *)malloc( nrows * sizeof(int) );
	col3array = (int *)malloc( nrows * sizeof(int) );
	int counter = 0;
	for(vector<vector<int> >::iterator it=data.begin();it!=data.end();++it)
	{
		col1array[counter] = (*it)[0];
		col2array[counter] = (*it)[1];
		col3array[counter] = (*it)[2];
		counter++;
	}
	
	
	status = 0;         /* initialize status before calling fitsio routines */
			
	struct stat  buffer;	
	if ( stat(filename, &buffer) == 0 )
	{	
		if ( fits_open_file(&fptr, filename, READWRITE, &status) )
			printerror4( status );
	} 
	else
	{
		if ( fits_create_file(&fptr, filename, &status)) /* create new FITS file */
			printerror4( status );           /* call printerror if error occurs */
	}	
		
	if ( fits_create_tbl(fptr,  ASCII_TBL, nrows, 3, ttype, tform, tunit, extname, &status) )
		printerror4( status );
	
	if ( fits_write_col(fptr, TINT, 1, 1, 0, nrows, col1array, &status) )
		printerror4( status );
	
	if ( fits_write_col(fptr, TINT, 2, 1, 0, nrows, col2array, &status) )
		printerror4( status );
	
	if ( fits_write_col(fptr, TINT, 3, 1, 0, nrows, col3array, &status) )
		printerror4( status );
	
	if ( fits_close_file(fptr, &status) )                /* close the file */
		printerror4( status );
		
		//	printf("FITS writed: %s\n",filename);
		
	free(col1array);
	free(col2array);
	free(col3array);
	return;
}

void printerror4( int status)
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
