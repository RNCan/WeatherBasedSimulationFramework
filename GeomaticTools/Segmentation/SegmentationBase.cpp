//***********************************************************************
// program to merge Landsat image image over a period
//									 
//***********************************************************************
// version
// 25/01/2019	Rémi Saint-Amant	Creation



//-FirstYear 1984 -RMSEThreshold 75 -MaxBreaks 5 -of VRT "U:\GIS\#documents\TestCodes\Segmentation\Input\1984-2016.vrt" "U:\GIS\#documents\TestCodes\Segmentation\output\1984-2016.vrt"

#include "stdafx.h"
#include <math.h>
#include "SegmentationBase.h"
#include "Basic/UtilMath.h"
#include "Basic/Statistic.h"

using namespace std;

namespace WBSF
{
	
	pair<double, size_t> ComputeRMSE(const NBRVector& data, size_t i)
	{
		ASSERT(i< data.size());

		if ((i - 1) >= data.size() || (i + 1) >= data.size())
			return make_pair(32767, data[i].second);

		CStatisticXY stat;
		for (int ii = -1; ii <= 1; ii++)
			stat.Add((double)data[i + ii].second, (double)data[i + ii].first);

		double a0 = stat[INTERCEPT];
		double a1 = stat[SLOPE];
		
		CStatistic rsq;
		for (int ii = -1; ii <= 1; ii++)
			rsq += Square(a0 + a1*data[i + ii].second - data[i + ii].first);
	
		return make_pair(sqrt(rsq[MEAN]), data[i].second);
	}

	
	vector<size_t> Segmentation(const NBRVector& dataIn, size_t maxLayers, double max_error)
	{
		ASSERT(!dataIn.empty());

		//compute mean and variance 
		//CStatistic stat;
		//for (size_t z = 0; z < dataIn.size(); z++)
			//stat += dataIn[z].first;

		//copy vector
		NBRVector data(dataIn.size());

		//normalize data
		
		for (size_t i = 0; i < dataIn.size(); i++)
			data[i] = dataIn[i];
			//data[i] = make_pair((dataIn[i].first - stat[MEAN]) / stat[STD_DEV], dataIn[i].second);

		//now compute RMSE and eliminate lo RMSE
		NBRVector dataRMSE(data.size());

		//1-first scan make pair and calculate the RMSE 
		for (size_t i = 0; i < data.size(); i++)
			dataRMSE[i] = ComputeRMSE(data, i);

		//RMSE array have the same size
		ASSERT(dataRMSE.size() == data.size());

		NBRVector::iterator minIt = std::min_element(dataRMSE.begin(), dataRMSE.end());
		while (dataRMSE.size() > 2 &&
			((minIt->first <= max_error) || (dataRMSE.size() > maxLayers)))
		{
			//erase data item
			NBRVector::iterator it = std::find_if(data.begin(), data.end(), [minIt](const NBRPair& p) { return p.second == minIt->second; });
			ASSERT(it != data.end());
			data.erase(it);

			//A-Detect the point - Remove it - and recompute all the stats
			//1- GET the min value in the RMSE array and REMOVE it 
			//remove RMSE item and update RMSE for points that have change
			size_t pos = std::distance(dataRMSE.begin(), minIt);
			dataRMSE.erase(minIt);

			if (pos - 1 < dataRMSE.size())
				dataRMSE[pos - 1] = ComputeRMSE(data, pos - 1);
			if (pos + 1 < dataRMSE.size())
				dataRMSE[pos] = ComputeRMSE(data, pos);


			ASSERT(dataRMSE.size() == data.size());

			minIt = std::min_element(dataRMSE.begin(), dataRMSE.end());
		}

		//add breaking point images index
		vector<size_t> breaks;

		for (size_t i = 0; i < dataRMSE.size(); i++)
			breaks.push_back(dataRMSE[i].second);

		return breaks;
	}

}