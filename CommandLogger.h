#ifndef COMMANDLOGGER_H
#define COMMANDLOGGER_H

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

class CommandLogger
{
private:
	char *filename;
	FILE *logFile;
public:
	CommandLogger(const char* dir);
	~CommandLogger();
	
	int log(const char* regimeName,const char* command);
};

#endif

