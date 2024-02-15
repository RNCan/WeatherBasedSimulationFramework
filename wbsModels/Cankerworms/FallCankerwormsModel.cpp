//**********************************************************************
// 21/03/2018	1.0.0	Rémi Saint-Amant	Create from Porter 1924 data
//**********************************************************************
#include "FallCankerwormsModel.h"
#include "Basic/Statistic.h"
#include <math.h>
#include <crtdbg.h>
#include "ModelBase/EntryPoint.h"
#include "ModelBase/SimulatedAnnealingVector.h"

using namespace std;


namespace WBSF
{

	//this line links this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CFallCankerwormsModel::CreateObject);
	/*
	//vertical
	NbVal=   102	Bias=-0.17855	MAE= 7.31537	RMSE=10.32995	CD= 0.91895	R²= 0.91908
	a1                  	= 256.67226
	b1                  	=   0.51682
	a2                  	= 369.50478
	b2                  	=   0.59489
	a3                  	= 433.16780
	b3                  	=   0.67761
	a4                  	= 508.29215
	b4                  	=   0.68069
	a5                  	= 659.39926
	b5                  	=   0.81050
	a6                  	=1170.71128
	b6                  	=   0.65972
		*/



	const char FallCankerworms_header[] = "EGG|L1|L2|L3|L4|SOIL|PUPA|ADULT";

	enum TEvaluation { VERTICAL, HORIZONTAL, DIAGONAL };
	static const TEvaluation EVALUATION = VERTICAL;

	const double CFallCankerwormsModel::THRESHOLD = 3.25;

	const double CFallCankerwormsModel::A[NB_FALL_PARAMS] =
	{
		256.7,369.5,433.2,508.3,659.4,1170.7
	};

	const double CFallCankerwormsModel::B[NB_FALL_PARAMS] =
	{
		0.51682,0.59489,0.67761,0.68069,0.81050,0.65972
	};

	CFallCankerwormsModel::CFallCankerwormsModel()
	{
		//NB_INPUT_PARAMETER is used to determine if the dll
		//uses the same number of parameters as the model interface
		NB_INPUT_PARAMETER = -1;
		VERSION = "1.0.0 (2018)";
	}

	CFallCankerwormsModel::~CFallCankerwormsModel()
	{}


	//this method is call to load your parameter in your variable
	ERMsg CFallCankerwormsModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		int c = 0;

		m_fallCR.m_startJday = 0;
		m_fallCR.m_lowerThreshold = THRESHOLD;
		m_fallCR.m_bCumul = parameters[c++].GetBool();
		m_fallCR.m_bMultipleVariance = true;
		m_fallCR.m_bPercent = true;
		m_fallCR.m_bAdjustFinalProportion = true;
		m_fallCR.m_method = CFallCankerwormsCR::DAILY_AVERAGE;
		for (size_t i = 0; i < NB_FALL_PARAMS; i++)
		{
			m_fallCR.m_a[i] = A[i];
			m_fallCR.m_b[i] = B[i];
		}

		if (parameters.size() > 1)
		{
			m_fallCR.m_lowerThreshold = parameters[c++].GetFloat();
			for (size_t i = 0; i < NB_FALL_PARAMS; i++)
			{
				m_fallCR.m_a[i] = parameters[c++].GetFloat();
				m_fallCR.m_b[i] = parameters[c++].GetFloat();
			}
		}

		return msg;
	}


	ERMsg CFallCankerwormsModel::OnExecuteDaily()
	{
		ERMsg msg;

		m_fallCR.Execute(m_weather, m_output);

		return msg;
	}


	void CFallCankerwormsModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		ASSERT(header[0] == "KeyID");
		ASSERT(header[1] == "Year");
		ASSERT(header[2] == "Month");
		ASSERT(header[3] == "Day");

		CTRef ref(ToShort(data[1]), ToShort(data[2]) - 1, ToShort(data[3]) - 1);

		ASSERT(header.size() == 10);
		ASSERT(header[4] == "L1");
		ASSERT(header[5] == "L2");
		ASSERT(header[6] == "L3");

		std::vector<double> obs(NB_FALL_INPUT);
		for (size_t i = 0; i < NB_FALL_INPUT; i++)
		{
			obs[i] = ToDouble(data[i + 4]);//Cumulative
		}

		m_SAResult.push_back(CSAResult(ref, obs));
	}

	bool CFallCankerwormsModel::GetFValueDaily(CStatisticXY& stat)
	{
		ERMsg msg;

		if (m_SAResult.size() > 0)
		{
			//remove unused years
			/*if (m_firstYear == -999 && m_lastYear == -999)
			{
				CStatistic years;
				for (CSAResultVector::const_iterator p = m_SAResult.begin(); p < m_SAResult.end(); p++)
					years += p->m_ref.GetYear();

				m_firstYear = (int)years[LOWEST];
				m_lastYear = (int)years[HIGHEST];
				while (m_weather.GetNbYears() > 1 && m_weather.GetFirstYear() < m_firstYear)
					m_weather.erase(m_weather.begin());

				while (m_weather.GetNbYears() > 1 && m_weather.GetLastYear() > m_lastYear)
					m_weather.erase(--m_weather.end());

			}*/


			//look to see if all ai are in growing order
			bool bValid = true;

			for (int i = 1; i < NB_FALL_PARAMS && bValid; i++)
			{
				if (m_fallCR.m_a[i] < m_fallCR.m_a[i - 1])
					bValid = false;
			}


			if (bValid)
			{
				//******************************************************************************************************************************************************
				//Vertical lookup
				if (EVALUATION == VERTICAL)
				{
					CModelStatVector statSim;
					m_fallCR.m_bCumul = true;
					m_fallCR.Execute(m_weather, statSim);
					for (size_t i = 0; i < m_SAResult.size(); i++)
					{
						ASSERT(m_SAResult[i].m_obs.size() == NB_FALL_INPUT);
						for (size_t p = F_EGG_L1; p <= F_SOIL_PUPA; p++)
						{
							if (m_SAResult[i].m_obs[I_F_L1 + p] > 0.0 &&
								m_SAResult[i].m_obs[I_F_L1 + p] < 99.9 &&
								statSim.IsInside(m_SAResult[i].m_ref))
							{
								double obs = m_SAResult[i].m_obs[I_F_L1 + p];
								double sim = statSim[m_SAResult[i].m_ref][CFallCankerwormsCR::O_FIRST_STAGE + p + 1];

								stat.Add(sim, obs);
							}
						}
					}
				}
				//******************************************************************************************************************************************************
				//Horizontal lookup
				else if (EVALUATION == HORIZONTAL)
				{
					//CModelStatVector statSim;
					//m_fallCR.m_bCumul = true;
					//m_fallCR.Execute(m_weather, statSim);

					//for (size_t i = 0; i < m_SAResult.size(); i++)
					//{
					//	ASSERT(m_SAResult[i].m_obs.size() == NB_INPUT);
					//	for (size_t p = F_EGG_L1; p <= F_SOIL_PUPA; p++)
					//	{
					//		double obsS = m_SAResult[i].m_obs[I_L1 + p];
					//		if (obsS > 0 &&
					//			obsS < 100 &&
					//			statSim.IsInside(m_SAResult[i].m_ref))
					//		{
					//			size_t pp = CFallCankerwormsCR::O_FIRST_STAGE + p + 1;

					//			int year = m_SAResult[i].m_ref.GetYear();
					//			size_t index = statSim.GetFirstIndex(pp, obsS, 1, CTPeriod(year, FIRST_MONTH, FIRST_DAY, year, LAST_MONTH, LAST_DAY));
					//			if (index >= 1)
					//			{
					//				//double obsDD1 = statSim[index][pp];
					//				//double obsDD2 = statSim[index + 1][pp];

					//				//double obsS1 = statSim[index][pp];
					//				//double obsS2 = statSim[index + 1][pp];
					//				//double slope = (obsDD2 - obsDD1) / (obsS2 - obsS1);
					//				//double obsH = obsDD1 + (obsS - obsS1)*slope;
					//				//double simH = statSim[m_SAResult[i].m_ref][pp];
					//				//ASSERT(!_isnan(obsH) && !_isnan(simH));

					//				CTRef TrefSim = statSim.GetFirstTRef() + index;
					//				stat.Add(TrefSim.GetJDay(), m_SAResult[i].m_ref.GetJDay());
					//			}
					//		}
					//	}
					//}
				}
				//******************************************************************************************************************************************************			
				//Diagonal lookup
				else if (EVALUATION == DIAGONAL)
				{
					/*CModelStatVector statSim;
					m_fallCR.m_bCumul = true;
					m_fallCR.Execute(m_weather, statSim);

					for (size_t i = 0; i < m_SAResult.size(); i++)
					{
						ASSERT(m_SAResult[i].m_obs.size() == NB_INPUT);
						for (int p = P_L2_L3; p < NB_PARAMS; p++)
						{
							int s = p + 1;
							if (m_SAResult[i].m_obs[I_L2 + s] > -999 && statSim.IsInside(m_SAResult[i].m_ref))
							{
								double obsS = m_SAResult[i].m_obs[I_L2 + s];
								double simS = statSim[m_SAResult[i].m_ref][CSBWContinuingRatio::O_FIRST_STAGE + s];
								if (obsS > 1 && obsS < 99)
								{
									int pp = CSBWContinuingRatio::O_FIRST_STAGE + s;

									short year = m_SAResult[i].m_ref.GetYear();
									long index = statSim.GetFirstIndex(pp, obsS, 1, CTPeriod(year, FIRST_MONTH, FIRST_DAY, year, LAST_MONTH, LAST_DAY));
									if (index >= 1)
									{
										double obsDD1 = statSim[index][CSBWContinuingRatio::O_DD];
										double obsDD2 = statSim[index + 1][CSBWContinuingRatio::O_DD];

										double obsS1 = statSim[index][pp];
										double obsS2 = statSim[index + 1][pp];
										double slope = (obsDD2 - obsDD1) / (obsS2 - obsS1);
										double obsDD = obsDD1 + (obsS - obsS1)*slope;
										double simDD = statSim[m_SAResult[i].m_ref][CSBWContinuingRatio::O_DD];


										obsDD = (obsDD - m_DDStat[MEAN]) / m_DDStat[STD_DEV_OVER_POP];
										simDD = (simDD - m_DDStat[MEAN]) / m_DDStat[STD_DEV_OVER_POP];
										obsS = (obsS - m_stageStat[p][MEAN]) / m_stageStat[p][STD_DEV_OVER_POP];
										simS = (simS - m_stageStat[p][MEAN]) / m_stageStat[p][STD_DEV_OVER_POP];
										ASSERT(!_isnan(obsDD) && !_isnan(simDD));
										ASSERT(!_isnan(obsS) && !_isnan(simS));

										stat.Add(simDD, obsDD);
										stat.Add(simS, obsS);
									}
								}
							}
						}
					}*/
				}
				//******************************************************************************************************************************************************			
				//CModelStatVector statSim;
				//m_continuingRatio.m_bCumul = true;
				//m_continuingRatio.Execute(m_weather, statSim);

				//for(int k=0; k<(int)m_SAResult.size(); k++)
				//{
				//	if( m_SAResult[k].m_obs[I_AI]>-999 && statSim.IsInside(m_SAResult[k].m_ref) )
				//	{
				//	
				//		double obs= m_SAResult[k].m_obs[I_AI];
				//		double sim= statSim[m_SAResult[k].m_ref][CSBWContinuingRatio::O_AVERAGE_INSTAR];
				//		stat.Add(sim, obs); 
				//	}
				//}
			}
		}

		return true;
	}



}