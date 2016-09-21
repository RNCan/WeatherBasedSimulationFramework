//*****************************************************************************
//                BioSIM-ready                   
//            Phenology model for                 
//                                               
//Pikonema alaskensis Yellow-Headed Spruce Sawfly
//                                               
//           Data from D. Lavigne                 
//                   NBDNRE                      
//                                               
//     Analysis and coding by J. Regniere        
//        Canadian Forest Service, LFC           
//*****************************************************************************
// File: YHSSModel.cpp
//
// Class: CCYHSSModel
//
// Description: CYHSSModel is a BioSIM model that computes Yellowheaded Spruce Sawfly
//              seasonal biology. 
//
//*****************************************************************************
// 20/09/2016	2.3.0	Rémi Saint-Amant    Change Tair and Trng by Tmin and Tmax
// 22/01/2016	2.2.0	Rémi Saint-Amant	Using Weather-Based Simulation Framework (WBSF)
// 01/03/2011			Rémi Saint-Amant    Integration with new BioSIMModelBase
// 22/04/2009			Rémi Saint-Amant    Integration with BioSIMModelBase
// 01/03/2000			Jacques Regniere	Creation 
//*****************************************************************************
#include "Basic/DegreeDays.h"
#include "ModelBase/EntryPoint.h"
#include "YHSSModel.h"



using namespace std;
using namespace WBSF::HOURLY_DATA;

namespace WBSF
{

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CYHSSModel::CreateObject);

	CYHSSModel::CYHSSModel()
	{
		//NB_INPUT_PARAMETER is use to determine if the dll
		//use the same number of parameter than the model interface
		NB_INPUT_PARAMETER = 2;
		VERSION = "2.3.0 (2016)";

		m_bCumulativeOutput = false;
		m_adultLongevity = 14;

	}

	//this method is call to load your parameter in your variable
	ERMsg CYHSSModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		m_bCumulativeOutput = parameters[0].GetBool();
		m_adultLongevity = parameters[1].GetInt();

		ASSERT(m_adultLongevity < 89);

		return msg;
	}

	enum TOuutput{ O_DEGREE_DAY, O_ADULT, O_TRAP_CATCH, O_L1, O_L2, O_L3, O_L4, O_L5, O_L6, O_AI, NB_STATS };
	typedef CModelStatVectorTemplate<NB_STATS> CYHSSStatVector;
	//This method is call to compute solution
	ERMsg CYHSSModel::OnExecuteDaily()
	{
		ERMsg msg;

	//	if (!m_weather.IsHourly() )
			//m_weather.ComputeHourlyVariables();

		static const double THRESHOLD = 0;

		CDegreeDays DD(CDegreeDays::DAILY_AVERAGE, THRESHOLD);
		

		CYHSSStatVector stat(m_weather.GetEntireTPeriod(CTM(CTM::DAILY)));

		for (size_t y = 0; y < m_weather.size(); y++)
		{
			CTPeriod p = m_weather[y].GetEntireTPeriod();

			double tot_adult[366] = { 0 };
			double dd = 0;
			double cum_adult = 0;

			for (size_t jd = 0; jd < 90; jd++)
			{
				CTRef TRef = p.Begin() + jd;
				stat[TRef][O_L1] = 100;
				stat[TRef][O_AI] = 1;
			}

			for (size_t jd = 89; jd < m_weather[y].GetNbDays(); jd++)
			{
				CTRef TRef = p.Begin() + jd;
				const CWeatherDay& wDay = m_weather.GetDay(TRef);
				dd += DD.GetDD(wDay);

				double freq[9] = { 0 };
				double ai = ComputeStageFrequencies(dd, freq);

				//		adults alive
				tot_adult[jd] = freq[0];
				cum_adult += (tot_adult[jd] - tot_adult[jd - m_adultLongevity]) / m_adultLongevity;
				

				stat[TRef][O_DEGREE_DAY] = dd;

				stat[TRef][O_ADULT] = freq[0];
				stat[TRef][O_TRAP_CATCH] = cum_adult;

				for (size_t i = 0; i < 6; i++)
					stat[TRef][O_L1 + i] = freq[i + 3];
				
				stat[TRef][O_AI] = ai;
			}
		}

		SetOutput(stat);

		return msg;
	}

	double CYHSSModel::ComputeStageFrequencies(double dd, double freq[9])
	{
		static const int sign[9] = { 1, 1, 1, -1, 1, 1, 1, 1, 1 };
		static const double a[9] = { 563.8, 563.8f, 634.5, 786.4, 786.4, 862.1, 949.2, 1063.2, 1177.5 };
		static const double b[9] = { 1.821, 0.001f, 0.001, 0.741, 0.741, 0.466, 0.861, 2.320, 4.326 };
		static const double c[9] = { 100.0, 100.0f, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 69.8 };

		freq[3] = 100;

		if (dd <= 0)
			return 1;

		//		    adult emergence
		freq[0] = max(0.0, (c[0] / (1. + exp(sign[0] * (a[0] - dd) / sqrt(b[0] * dd)))));

		//		1st egg, 1st larva
		for (int i = 1; i < 3; i++)
		{
			if (dd < a[i])freq[i] = 0.0f;
			else freq[i] = 100.0f;
		}

		//		% in 6 larval stages. L1 start at 100%. % L2 (cumul) is 100- (% L1)
		for (int i = 3; i < 9; i++)
		{
			freq[i] = max(0.0, (c[i] / (1. + exp(sign[i] * (a[i] - dd) / sqrt(b[i] * dd)))));
			ASSERT(freq[i] >= 0);

			if (freq[i] < 0.00001)
				freq[i] = 0;
		}


		//		average instar
		double f1 = freq[3];
		double f2 = max(0.0, freq[4] - freq[5]);
		double f3 = max(0.0, freq[5] - freq[6]);
		double f4 = max(0.0, freq[6] - freq[7]);
		double f5 = max(0.0, freq[7] - freq[8]);
		double f6 = freq[8];
		double totf = f1 + f2 + f3 + f4 + f5 + f6;
		double ai = 1;

		if (totf>0)
			ai = (f1 * 1 + f2 * 2 + f3 * 3 + f4 * 4 + f5 * 5 + f6 * 6) / totf;


		//		if user reqested non-cumulative output (% in stage rather than in stage or later...
		if (!m_bCumulativeOutput)
		{
			freq[4] = f2;
			freq[5] = f3;
			freq[6] = f4;
			freq[7] = f5;
		}

		return ai;
	}


	

}