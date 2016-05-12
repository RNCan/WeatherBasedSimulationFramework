//*********************************************************************
// File: DevelopementVector.cpp
//
// Class: CDevelopementVector
//          
//
//************** MODIFICATIONS  LOG ********************
// 15/07/2012   Rémi Saint-Amant    Creation
//*********************************************************************
#include "DevelopementVector.h"

using namespace std;


namespace WBSF
{


	//**********************************************************************

	CMPBDevelopmentVector::CMPBDevelopmentVector(const CRandomGenerator& RG):
		MPB_RATES_TABLE(RG)
	{}


	//CMPBDevelopmentTable CMPBDevelopmentVector::;
	void CMPBDevelopmentVector::Init(const CDailyWaveVector& T)
	{
		//NOTE: the number of day is truck to the entire day
		int nbDay = T.m_period.GetNbDay();
		resize(nbDay);
		for (size_t d = 0; d < nbDay; d++)
		{
			for (size_t s = 0; s < NB_STAGES; s++)
			{
				at(d)[s] = 0;
				for (size_t h = 0; h < T.NbStep(); h++)
				{
					size_t i = d*T.NbStep() + h;
					at(d)[s] += MPB_RATES_TABLE.GetRate(s, T[i]) / T.NbStep();
				}
			}
		}
	}

	//*****************************************************
	// GetNextStageDay : Find the day when the bug change of stage
	//          
	// Input :
	//  devRates : developement rate table
	//  firstDay : The first day in the current stage
	//  stage : The stage to evaluate
	//
	// Output 
	//  return: day of the next stage if sussesfukl. -1 otherwise.
	//*****************************************************
	int CMPBDevelopmentVector::GetNextStageDay(int firstDay, int s, double threshold)const
	{
		_ASSERTE(s >= 0 || s < NB_STAGES);

		int index = -1;

		//init the accumulation
		double sum = 0;

		int maxDay = int(size() * 2);//simulate on 2 years max

		//accumulate all values until reach the next stage 
		for (int d = firstDay; d < maxDay; d++)
		{
			int jd = int(d%size());
			sum += at(jd)[s];
			if (sum >= threshold)
			{
				index = d;
				break;
			}
		}

		_ASSERTE(index >= -1 && index < maxDay);

		return index;
	}


}