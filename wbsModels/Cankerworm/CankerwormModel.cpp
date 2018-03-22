//**********************************************************************
// 21/03/2018	1.0.0	Rémi Saint-Amant	Create from Porter 1924 data
//**********************************************************************
#include "CankerwormModel.h"
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
		CModelFactory::RegisterModel(CCankerwormModel::CreateObject);



	const char FallCankerworm_header[] = "EGG|L1|L2|L3|L4|PUPA|ADULT";
	enum TEvaluation { VERTICAL, HORIZONTAL, DIAGONAL };
	static const TEvaluation EVALUATION = HORIZONTAL;

	const double CCankerwormModel::A[NB_SPECIES][NB_PARAMS_MAX] =
	{
		{231.8, 282.8, 323.9, 394.7, 473.2, 612.6, 0},
		{ 231.8, 282.8, 323.9, 394.7, 473.2, 612.6, 0 }
	};

	const double CCankerwormModel::B[NB_SPECIES][NB_PARAMS_MAX] =
	{
		{1.0, 1.25, 1.7, 1.6, 1.5, 1.5, 0},
		{ 1.0, 1.25, 1.7, 1.6, 1.5, 1.5, 0 },
	};

	CCankerwormModel::CCankerwormModel()
	{
		//NB_INPUT_PARAMETER is used to determine if the dll
		//uses the same number of parameters as the model interface
		NB_INPUT_PARAMETER = -1;
		VERSION = "1.0.0 (2018)";


		m_firstYear = -999;
		m_lastYear = -999;
	}

	CCankerwormModel::~CCankerwormModel()
	{}


	//this method is call to load your parameter in your variable
	ERMsg CCankerwormModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		int c = 0;

		m_species = parameters[c++].GetInt();

		m_fallCR.m_startJday = 60 - 1;
		m_fallCR.m_lowerThreshold = 4.5;
		m_fallCR.m_bMultipleVariance = true;
		m_fallCR.m_bPercent = true;
		m_fallCR.m_bAdjustFinalProportion = true;
		m_fallCR.m_method = CFallCankerwormCR::SINGLE_SINE;

		switch (m_species)
		{
		case FALL:
			for (size_t i = 0; i < NB_FALL_PARAMS; i++)
			{
				m_fallCR.m_a[i] = A[FALL][i];
				m_fallCR.m_b[i] = B[FALL][i];
			}
			break;

		case SPRING:
			for (size_t i = 0; i < NB_SPRING_PARAMS; i++)
			{
				m_springCR.m_a[i] = A[SPRING][i];
				m_springCR.m_b[i] = B[SPRING][i];
			}
			break;
		default: ASSERT(false);
		}

		if (parameters.size() > 1)
		{
			m_fallCR.m_lowerThreshold = parameters[c++].GetFloat();
			m_springCR.m_lowerThreshold = parameters[c++].GetFloat();

			switch (m_species)
			{
			case FALL:
				for (size_t i = 0; i < NB_FALL_PARAMS; i++)
				{
					m_fallCR.m_a[i] = parameters[c++].GetFloat();
					m_fallCR.m_b[i] = parameters[c++].GetFloat();
				}

				break;
			case SPRING:
				for (size_t i = 0; i < NB_SPRING_PARAMS; i++)
				{
					m_springCR.m_a[i] = parameters[c++].GetFloat();
					m_springCR.m_b[i] = parameters[c++].GetFloat();
				}
				break;
			default: ASSERT(false);
			}
		}


		return msg;
	}


	ERMsg CCankerwormModel::OnExecuteDaily()
	{
		ERMsg msg;

		switch (m_species)
		{
		case FALL:	m_fallCR.Execute(m_weather, m_output); break;
		case SPRING:m_springCR.Execute(m_weather, m_output); break;
		default: ASSERT(false);
		}

		return msg;
	}


	void CCankerwormModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		//		PLOT_ID	PLOT_NAME	PLOT_LAT	PLOT_LON	PLOT_ELEV	DATE	Year	Month	Day	jDay	SPECIES	SURVEY_ID	Spayed	LI2	LI3	LI4	LI5	LI6	PUPA	nbInd	AI
		//transform value to date/stage
		ASSERT(header[0] == "Name");
		ASSERT(header[1] == "ID");
		ASSERT(header[2] == "Group");
		ASSERT(header[3] == "Year");
		ASSERT(header[4] == "Month");
		ASSERT(header[5] == "Day");
		ASSERT(header[7] == "LI2");
		ASSERT(header[13] == "NbInds");
		ASSERT(header[14] == "AvIns");


		if (ToInt(data[2]) == m_subModel)
		{
			CTRef ref(ToShort(data[3]), ToShort(data[4]) - 1, ToShort(data[5]) - 1);

			std::vector<double> obs(NB_INPUT);
			for (int i = 0; i < NB_INPUT; i++)
			{
				obs[i] = ToDouble(data[13 + i]);//Cumulative L2
			}

			m_SAResult.push_back(CSAResult(ref, obs));
		}
	}

	void CCankerwormModel::GetFValueDaily(CStatisticXY& stat)
	{
		ERMsg msg;

		if (m_SAResult.size() > 0)
		{
			if (m_DDStat[NB_VALUE] == 0)
			{
				CModelStatVector statSim;
				m_continuingRatio.m_bCumul = true;
				m_continuingRatio.Execute(m_weather, statSim);

				const CSimulatedAnnealingVector& SAVector = GetSimulatedAnnealingVector();

				for (size_t sa = 0; sa < SAVector.size(); sa++)
				{
					const CSAResultVector& SAResult = SAVector[sa]->GetSAResult();

					for (size_t i = 0; i < SAResult.size(); i++)
					{
						ASSERT(SAResult[i].m_obs.size() == NB_INPUT);
						for (int p = P_L2_L3; p < NB_PARAMS; p++)
						{
							int s = p + 1;
							if (SAResult[i].m_obs[I_L2 + s] > -999 && statSim.IsInside(SAResult[i].m_ref))
							{
								double obsS = SAResult[i].m_obs[I_L2 + s];
								if (obsS > 1 && obsS < 99)
								{
									int pp = CSBWContinuingRatio::O_FIRST_STAGE + s;

									short year = SAResult[i].m_ref.GetYear();
									long index = statSim.GetFirstIndex(pp, obsS, 1, CTPeriod(year, FIRST_MONTH, FIRST_DAY, year, LAST_MONTH, LAST_DAY));
									if (index >= 1)
									{
										double obsDD1 = statSim[index][CSBWContinuingRatio::O_DD];
										double obsDD2 = statSim[index + 1][CSBWContinuingRatio::O_DD];

										double obsS1 = statSim[index][pp];
										double obsS2 = statSim[index + 1][pp];
										double slope = (obsDD2 - obsDD1) / (obsS2 - obsS1);
										double obsDD = obsDD1 + (obsS - obsS1)*slope;
										ASSERT(!_isnan(obsDD) && _finite(obsDD));
										ASSERT(!_isnan(obsS) && _finite(obsS));

										m_DDStat += obsDD;
										m_stageStat[p] += obsS;
									}
								}
							}
						}
					}
				}

				ASSERT(m_DDStat[NB_VALUE] > 0);
			}




			//remove unused years
			if (m_firstYear == -999 && m_lastYear == -999)
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
				//m_weather.RemoveYear(m_weather.GetNbYears() - 1);
			}


			//look to see if all ai are in growing order
			bool bValid = true;
			for (int i = 1; i < NB_PARAMS&&bValid; i++)
			{
				if (m_continuingRatio.m_a[i] < m_continuingRatio.m_a[i - 1])
					bValid = false;
			}

			if (bValid)
			{
				//******************************************************************************************************************************************************
				//CModelStatVector statSim;
				//m_continuingRatio.m_bCumul = false;
				//m_continuingRatio.Execute(m_weather, statSim);
				//
				//int L3 = statSim.GetFirstIndex(CSBWContinuingRatio::O_AVERAGE_INSTAR, 3);
				//int L4 = statSim.GetFirstIndex(CSBWContinuingRatio::O_AVERAGE_INSTAR, 4);
				//int L5 = statSim.GetFirstIndex(CSBWContinuingRatio::O_AVERAGE_INSTAR, 5);
				//int L6 = statSim.GetFirstIndex(CSBWContinuingRatio::O_AVERAGE_INSTAR, 6);
				////int Pupa = statSim.GetFirstIndex(CSBWContinuingRatio::O_AVERAGE_INSTAR, 7);
				//bValid = L2>=0 && L4>=0 && L5>=0 && L6>=0;
				//if( bValid )
				//{
				//	stat.Add(L3>=0?statSim[L3][CSBWContinuingRatio::O_DD]:-999, 310); 
				//	stat.Add(L4>=0?statSim[L4][CSBWContinuingRatio::O_DD]:-999, 380); 
				//	stat.Add(L5>=0?statSim[L5][CSBWContinuingRatio::O_DD]:-999, 475); 
				//	stat.Add(L6>=0?statSim[L6][CSBWContinuingRatio::O_DD]:-999, 580); 
				////stat.Add(Pupa>=0?statSim[Pupa][CSBWContinuingRatio::O_DD]:-999, 720); 
				//}

				//******************************************************************************************************************************************************
				//Vertical lookup
				if (EVALUATION == VERTICAL)
				{
					CModelStatVector statSim;
					m_continuingRatio.m_bCumul = true;
					m_continuingRatio.Execute(m_weather, statSim);

					for (size_t i = 0; i < m_SAResult.size(); i++)
					{
						ASSERT(m_SAResult[i].m_obs.size() == NB_INPUT);
						for (int p = P_L2_L3; p <= P_L6_PUPA; p++)
						{
							int s = p + 1;
							if (m_SAResult[i].m_obs[I_L2 + s] > -999 && statSim.IsInside(m_SAResult[i].m_ref))
							{

								double obs = m_SAResult[i].m_obs[I_L2 + s];
								double sim = statSim[m_SAResult[i].m_ref][CSBWContinuingRatio::O_FIRST_STAGE + s];

								stat.Add(sim, obs);
							}
						}
					}
				}
				//******************************************************************************************************************************************************
				//Horizontal lookup
				else if (EVALUATION == HORIZONTAL)
				{
					CModelStatVector statSim;
					m_continuingRatio.m_bCumul = true;
					m_continuingRatio.Execute(m_weather, statSim);

					for (size_t i = 0; i < m_SAResult.size(); i++)
					{
						ASSERT(m_SAResult[i].m_obs.size() == NB_INPUT);
						for (int p = P_L2_L3; p <= P_L6_PUPA; p++)
						{
							int s = p + 1;
							if (m_SAResult[i].m_obs[I_L2 + s] > -999 && statSim.IsInside(m_SAResult[i].m_ref))
							{
								double obsS = m_SAResult[i].m_obs[I_L2 + s];
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
										double obsH = obsDD1 + (obsS - obsS1)*slope;
										double simH = statSim[m_SAResult[i].m_ref][CSBWContinuingRatio::O_DD];
										ASSERT(!_isnan(obsH) && !_isnan(simH));

										stat.Add(simH, obsH);
									}
								}
							}
						}
					}
				}
				//******************************************************************************************************************************************************			
				//Diagonal lookup
				else if (EVALUATION == DIAGONAL)
				{
					CModelStatVector statSim;
					m_continuingRatio.m_bCumul = true;
					m_continuingRatio.Execute(m_weather, statSim);

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
					}
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
	}



}