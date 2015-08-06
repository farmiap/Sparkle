#ifndef PATHES_H
#define PATHES_H

#include <string>

using namespace std;

class Pathes
{
private:
	string fits_file;
	string fits_dir;
	string rta_file; // for refreshing image in run till abort mode
	string rta_path;
	string spool_path;
	string spool_path_suff;
	string fits_suffix;
	string intrv_path; // file containing HWP intervals
	string intrv_suffix; // file containing HWP intervals
	string hwppos_path; // file containing HWP intervals
	string hwppos_suffix; // file containing HWP intervals
public:
	int validate();

	void setFits(string input) {fits_file = input;};
	void setDir(string input) {fits_dir = input;};
	void setRTA(string input) {rta_file = input;};

	const char *getHWPPosPath() {return hwppos_path.c_str();};
	const char *getIntrvPath() {return intrv_path.c_str();};
	const char *getSpoolPath() {return spool_path.c_str();};
	const char *getSpoolPathSuff() {return spool_path_suff.c_str();};
	const char *getRTAPath() {return rta_path.c_str();};

	void print();

	Pathes();
	~Pathes();
};

#endif
