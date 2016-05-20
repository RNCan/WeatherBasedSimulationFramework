//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//******************************************************************************
#include "stdafx.h"
#include <algorithm>
#include <cassert>

#include "Geomatic/LandsatCloudsCleaner.h"
#include "Basic/OpenMP.h"


using namespace std;

namespace WBSF
{


	std::vector<AttValue> CLandsatDecisionTree::GetDataRecord(int t, const CLandsatPixel& img1, const CLandsatPixel& img2)const
	{
		//int t = omp_get_thread_num();
		assert(t >= 0 && t < (int)size());

		std::vector<AttValue> block(at(t).MaxAtt + 1);//nbAtt + 1 + 1

		for (int i = 0; i < at(t).MaxAtt; i++)
		{
			int ii = i + 1;
			if (i == 0)
			{
				if (Continuous(at(t), ii))
					CVal(block, ii) = DT_UNKNOWN;
				else
					DVal(block, ii) = 0;
			}
			else if (i > 0 && i <= Landsat::SCENES_SIZE)//First image
			{
				CVal(block, ii) = (ContValue)img1[i - 1];
			}
			else if (i > Landsat::SCENES_SIZE && i <= 2 * Landsat::SCENES_SIZE)//second image
			{
				CVal(block, ii) = (ContValue)img2[i - Landsat::SCENES_SIZE - 1];
			}
			else//virtual bands 
			{
				ASSERT(at(t).AttDef[ii]);

				//assuming virtual band never return no data
				block[ii] = at(t).EvaluateDef(at(t).AttDef[ii], block.data());
			}
		}//for

		//  Preserve original case number and virtual layer
		DVal(block.data(), 0) = at(t).MaxClass + 1;

		return block;
	}

	
	//****************************************************************************************************
//010 - t1 Cloud
//012 - t1 Haze and Shadow
//130 - t2 Cloud
//150 - t2 Haze and Shadow ans smoke 

	int CLandsatCloudCleaner::GetDTCode(const CLandsatPixel& pixel1, const CLandsatPixel& pixel2)const
	{
		int t = omp_get_thread_num();
		assert(t >= 0 && t < (int)size());

		vector <AttValue> block = GetDataRecord(t, pixel1, pixel2);

		int predict = (int) const_cast<CLandsatCloudCleaner*>(this)->at(t).Classify(block.data());
		ASSERT(predict >= 1 && predict <= at(t).MaxClass);
		int DTCode = atoi(at(t).ClassName[predict]);

		return DTCode;
	}

	bool CLandsatCloudCleaner::IsFirstCloud(int DTCode)const
	{
		return DTCode == 10 || DTCode == 11 || DTCode == 12;
	}

	bool CLandsatCloudCleaner::IsSecondCloud(int DTCode)const
	{
		return DTCode > 100;
	}

	bool CLandsatCloudCleaner::IsCloud(int DTCode)const
	{
		return (DTCode >= 10 && DTCode <= 20) || DTCode>100;
	}


	//bool CLandsatCloudCleaner::IsFirstCloud(const CLandsatPixel& pixel1, const CLandsatPixel& pixel2)const
	//{
	//	int t = omp_get_thread_num();
	//	assert(t >= 0 && t < (int)size());

	//	vector <AttValue> block = GetDataRecord(t, pixel1, pixel2);

	//	int predict = (int) const_cast<CLandsatCloudCleaner*>(this)->at(t).Classify(block.data());
	//	ASSERT(predict >= 1 && predict <= at(t).MaxClass);
	//	int DTCode = atoi(at(t).ClassName[predict]);

	//	return DTCode >= 100;
	//}

	//bool CLandsatCloudCleaner::IsSecondCloud(const CLandsatPixel& pixel1, const CLandsatPixel& pixel2)const
	//{
	//	int t = omp_get_thread_num();
	//	assert(t >= 0 && t < (int)size());

	//	vector <AttValue> block = GetDataRecord(t, pixel1, pixel2);

	//	int predict = (int) const_cast<CLandsatCloudCleaner*>(this)->at(t).Classify(block.data());
	//	ASSERT(predict >= 1 && predict <= at(t).MaxClass);
	//	int DTCode = atoi(at(t).ClassName[predict]);

	//	return (DTCode >= 10 && DTCode <= 20);
	//}

	//bool CLandsatCloudCleaner::IsCloud(const CLandsatPixel& pixel1, const CLandsatPixel& pixel2)const
	//{
	//	int t = omp_get_thread_num();
	//	assert(t >= 0 && t < (int)size());

	//	vector <AttValue> block = GetDataRecord(t, pixel1, pixel2);

	//	int predict = (int) const_cast<CLandsatCloudCleaner*>(this)->at(t).Classify(block.data());
	//	ASSERT(predict >= 1 && predict <= at(t).MaxClass);
	//	int DTCode = atoi(at(t).ClassName[predict]);

	//	bool bCloud = (DTCode >= 10 && DTCode <= 20) || DTCode > 100;
	//	return bCloud;
	//}

}