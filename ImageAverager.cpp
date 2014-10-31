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
		means[*it] = 0.0;
	}
	counter = 1;
}

ImageAverager::~ImageAverager()
{
	for(map<int, double*>::iterator it = sums.begin(); it != sums.end(); ++it)
	{
		if ( it->second != NULL )
			delete it->second;
//		else
//			cout << "it doesn't exist!" << endl;
	}

}

void ImageAverager::initWithDatasize(long inDatasize)
{
	datasize = inDatasize;
	for(map<int, double*>::iterator it = sums.begin(); it != sums.end(); ++it)
	{
		if ( it->second == NULL )
			it->second = new double[datasize];
//		else
//			cout << "it exists!" << endl;
		for(long i = 0; i < datasize; i++)
			it->second[i] = 0.0;
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
			double mean = 0.0;
			for(long i = 0; i < datasize; i++)
			{
				maximum = (maximum>it->second[i])?maximum:it->second[i];
				mean = mean + it->second[i];
				it->second[i] = 0.0;
			}
			maximums[it->first] = maximum/(double)it->first;
			means[it->first] = mean/(datasize*it->first);
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

	long datasize = 10101;
	at_32 *sample = new at_32[datasize];

	imageAverager.initWithDatasize(datasize);

	for(long i = 0;i < datasize;i++)
	{
		sample[i] = i;
	}

	for(int c = 0;c < 50;c++)
	{
		imageAverager.uploadImage(sample);
		for(map<int, double>::iterator it=imageAverager.maximums.begin(); it!=imageAverager.maximums.end(); ++it)
		{
			cout << "frames:" << it->first << " maximum:" << it->second << " mean: " << imageAverager.means[it->first] << endl;
		}
	}
}
