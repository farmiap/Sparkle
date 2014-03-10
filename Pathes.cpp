#include <iostream>
#include <sys/stat.h>

#include "Pathes.h"

using namespace std;

Pathes::Pathes()
{
//    cout << "pathes constructor" << endl;
	fits_suffix = ".fits";
//    cout << fits_suffix << endl;
}

Pathes::~Pathes()
{

}

void Pathes::print()
{
	cout << "  " << "FITS:" << spool_path << endl;
	cout << "  " << "RTA:" << rta_path << endl;
}

int Pathes::validate()
{
	if ( fits_dir[fits_dir.size()-1] != '/' )
	{
		cout << "------------->ERROR: " << fits_dir << " is not a directory" << endl;
		return 0;
	}

	if ( access(fits_dir.c_str(), R_OK | W_OK) != 0 )
	{
		cout << "------------->ERROR: cannot write to dir: " << fits_dir << endl;
		return 0;
	}

	spool_path = fits_dir + fits_file;
	rta_path = fits_dir + rta_file + fits_suffix;

	string spool_path_suff = spool_path + fits_suffix;

	struct stat  buffer;
	if ( stat(spool_path_suff.c_str(), &buffer) == 0)
		cout << "------------->WARNING: file " << spool_path_suff << " exists!!!" << endl;
//	if ( stat(rta_path.c_str(), &buffer) == 0)
//		cout << "------------->WARNING: file " << rta_path << " exists!!!" << endl;

	return 1;
}
