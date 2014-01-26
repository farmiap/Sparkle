#ifndef IMAGEAVERAGER_H
#define IMAGEAVERAGER_H

#include <vector>
#include <map>
#include "atmcdLXd.h"

using namespace std;

class ImageAverager
{
private:
	map<int, double*> sums;
	long counter;
	long datasize;
public:
	map<int, double> maximums;
	map<int, double> means;

	ImageAverager(vector<int> inPeriods);
	~ImageAverager();

	void initWithDatasize(long datasize);
	void uploadImage(at_32 *inImage);
};

void imageAveragerTest();

#endif
