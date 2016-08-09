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
		size_t nbDay = T.m_period.GetNbDay();
		resize(nbDay);
		for (CTRef TRef = T.m_period.Begin(); TRef != T.m_period.End(); TRef++)
		{
			size_t d = TRef.GetDay() - T.m_period.Begin().GetDay();
			for (size_t s = 0; s < NB_STAGES; s++)
			{
				at(d)[s] = 0;
				at(d)[s] += MPB_RATES_TABLE.GetRate(s, T[TRef]) / 24;
			}
		}


		/*for (size_t d = 0; d < nbDay; d++)
		{
			for (size_t s = 0; s < NB_STAGES; s++)
			{
				at(d)[s] = 0;
				for (size_t h = 0; h < T.size(); h++)
				{
					size_t i = d*T.NbSteps() + h;
					at(d)[s] += MPB_RATES_TABLE.GetRate(s, T[i]) / T.NbSteps();
				}
			}
		}*/
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
	size_t CMPBDevelopmentVector::GetNextStageDay(size_t firstDay, size_t s, double threshold)const
	{
		_ASSERTE(s >= 0 || s < NB_STAGES);

		size_t index = NOT_INIT;

		//init the accumulation
		double sum = 0;

		size_t maxDay = size() * 2;//simulate on 2 years max

		//accumulate all values until reach the next stage 
		for (size_t d = firstDay; d < maxDay; d++)
		{
			size_t jd = d%size();
			sum += at(jd)[s];
			if (sum >= threshold)
			{
				index = d;
				break;
			}
		}

		_ASSERTE(index == NOT_INIT || index < maxDay);

		return index;
	}


}