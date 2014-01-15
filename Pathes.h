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
	string fits_suffix;
public:
	int validate();

	void setFits(string input) {fits_file = input;};
	void setDir(string input) {fits_dir = input;};
	void setRTA(string input) {rta_file = input;};

	const char *getSpoolPath() {return spool_path.c_str();};
	const char *getRTAPath() {return rta_path.c_str();};

	void print();

	Pathes();
	~Pathes();
};

#endif
