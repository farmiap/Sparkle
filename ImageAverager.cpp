#include <iostream>
#include <stdlib.h>
#include <time.h>

#include "ImageAverager.h"

using namespace std;

ImageAverager::ImageAverager(vector<int> inPeriods)
{
	for(vector<int>::iterator it = inPeriods.begin(); it != inPeriods.end(); ++it)
	{
		sums[*it] = NULL;
		maximums[*it] = 0.0;
	}
	counter = 1;
}

ImageAverager::~ImageAverager()
{

}

void ImageAverager::initWithDatasize(long inDatasize)
{
	datasize = inDatasize;
	for(map<int, double*>::iterator it = sums.begin(); it != sums.end(); ++it)
	{
		it->second = new double[datasize];
	}
}

void ImageAverager::uploadImage(at_32 *inImage)
{
	for(map<int, double*>::iterator it = sums.begin();it != sums.end(); ++it)
	{
		for(long i = 0; i < datasize; i++)
		{
			it->second[i] = it->second[i] + inImage[i];
		}
		if ( counter%it->first == 0 )
		{
			double maximum = -1.0;
			for(long i = 0; i < datasize; i++)
			{
				maximum = (maximum>it->second[i])?maximum:it->second[i];
				it->second[i] = 0.0;
			}
			maximums[it->first] = maximum/(double)it->first;
//			cout << counter << ", "<< maximums[it->first] << endl;
		}

	}
	counter++;
}

void imageAveragerTest()
{
	srand(time(0));

	vector<int> periods;
	periods.push_back(3);
	periods.push_back(5);
	periods.push_back(10);

	ImageAverager imageAverager = ImageAverager(periods);

	long datasize = 500000;
	at_32 *sample = new at_32[datasize];

	imageAverager.initWithDatasize(datasize);

	for(long i = 0;i < datasize;i++)
	{
		sample[i] = i;
	}

	for(int c = 0;c < 50;c++)
	{
		imageAverager.uploadImage(sample);
	}
}
