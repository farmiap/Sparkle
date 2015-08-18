#include <sstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>     
#include <unistd.h>     

#include "CommandLogger.h"

using namespace std;

CommandLogger::CommandLogger(const char* dir)
{
	struct stat sb;
	int status;
	
	status = stat(dir, &sb);

	if ( ( status == 0 ) && S_ISDIR(sb.st_mode) )
	{
		if ( access(dir, R_OK | W_OK) != 0 )
		{
			cout << "ERROR: cannot write LOG to dir: " << dir << endl;
		}
	} 
	else
	{
		status = mkdir(dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	}
	
	filename = (char *)malloc(100*sizeof(char));
	
	struct timeval tv,tv2;
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
//	strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S", nowtm);
	strftime(tmbuf, sizeof tmbuf, "%y%m%d-spol.log", nowtm);
	sprintf(filename,"%s%s",dir,tmbuf);
	
	logFile = fopen(filename,"a+");
	log("","Sparkle started");
}

int CommandLogger::log(const char* regimeName,const char* command)
{
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
	strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S", nowtm);
	fprintf(logFile,"L %s %s %s\n",tmbuf,regimeName,command);
	fflush(logFile);
	
	return 0;
}

CommandLogger::~CommandLogger()
{
	if (filename)
		free(filename);
	if (logFile) 
		fclose(logFile);
}
