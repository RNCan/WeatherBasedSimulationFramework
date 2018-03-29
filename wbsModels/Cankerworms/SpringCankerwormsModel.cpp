//**********************************************************************
// 21/03/2018	1.0.0	Rémi Saint-Amant	Create from Porter 1924 data
//**********************************************************************
#include "SpringCankerwormsModel.h"
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
		CModelFactory::RegisterModel(CSpringCankerwormsModel::CreateObject);
	/*
NbVal=  1737	Bias= 0.00097	MAE= 2.25700	RMSE= 6.96968	CD= 0.97881	R²= 0.97882
Threshold1          	=   2.99997
a1                  	=  83.00751
b1                  	=   1.49920
a2                  	= 101.60651
b2                  	=   2.09868
a3                  	= 332.59499
b3                  	=   1.24226
a4                  	= 427.56069
b4                  	=   1.05008
a5                  	= 481.50824
b5                  	=   1.00756
a6                  	= 534.25200
b6                  	=   0.92139
a7                  	= 603.49191
b7                  	=   0.88681
a8                  	= 698.50334
b8                  	=   0.89527
a9                  	= 799.37988
b9                  	=   0.74483

		*/







	enum Toutput{ O_DD, O_PUPA1, O_ADULT, O_EGG, O_L1, O_L2, O_L3, O_L4, O_L5, O_SOIL, O_PUPA2, O_AI, NB_OUTPUT };
	const char SpringCankerworms_header[] = "DD|PUPA1|ADULT|EGG|L1|L2|L3|L4|L5|SOIL|PUPA2|AI";
	enum TEvaluation { VERTICAL, HORIZONTAL, DIAGONAL };
	static const TEvaluation EVALUATION = VERTICAL;

	const double CSpringCankerwormsModel::THRESHOLD = 3.0;
	const double CSpringCankerwormsModel::A[NB_SPRING_PARAMS] =
	{
		83.0,101.6,332.6,427.6,481.5,534.3,603.5,698.5,799.4
	};

	const double CSpringCankerwormsModel::B[NB_SPRING_PARAMS] =
	{
		1.49920,2.09868,1.24226,1.05008,1.00756,0.92139,0.88681,0.89527,0.74483
	};






	CSpringCankerwormsModel::CSpringCankerwormsModel()
	{
		//NB_INPUT_PARAMETER is used to determine if the dll
		//uses the same number of parameters as the model interface
		NB_INPUT_PARAMETER = -1;
		VERSION = "1.0.0 (2018)";
	}

	CSpringCankerwormsModel::~CSpringCankerwormsModel()
	{}


	//this method is call to load your parameter in your variable
	ERMsg CSpringCankerwormsModel::ProcessParameters(const CParameterVector& parameters)
	{
		ASSERT(m_info.m_modelName.find("Fall") || m_info.m_modelName.find("String"));

		ERMsg msg;

		int c = 0;

		bool bCumul = parameters[c++].GetBool();
		m_springCR.m_startJday = 0;
		m_springCR.m_lowerThreshold = THRESHOLD;
		m_springCR.m_bCumul = bCumul;
		m_springCR.m_bMultipleVariance = true;
		m_springCR.m_bPercent = true;
		m_springCR.m_bAdjustFinalProportion = true;
		m_springCR.m_method = CSpringCankerwormsCR::DAILY_AVERAGE;
		for (size_t i = 0; i < NB_SPRING_PARAMS; i++)
		{
			m_springCR.m_a[i] = A[i];
			m_springCR.m_b[i] = B[i];
		}

		
		if (parameters.size() > 1)
		{
			m_springCR.m_lowerThreshold = parameters[c++].GetFloat();
			parameters[c++].GetFloat();

			for (size_t i = 0; i < NB_SPRING_PARAMS; i++)
			{
				m_springCR.m_a[i] = parameters[c++].GetFloat();
				m_springCR.m_b[i] = parameters[c++].GetFloat();
			}
		}
		return msg;
	}


	ERMsg CSpringCankerwormsModel::OnExecuteDaily()
	{
		ERMsg msg;

		
		m_springCR.Execute(m_weather, m_output);
		

		//m_output.Init(m_weather.GetEntireTPeriod(CTM::DAILY), NB_OUTPUT);
		//for (size_t i = 0; i < out.size(); i++)
		//{
		//	m_output[i][O_PUPA1] = out[i][CSpringCankerwormsCR::O_FIRST_STAGE + 0];
		//	m_output[i][O_ADULT] = out[i][CSpringCankerwormsCR::O_FIRST_STAGE + 1];
		//	m_output[i][O_EGG] = out[i][CSpringCankerwormsCR::O_FIRST_STAGE + 2];
		//	m_output[i][O_L1] = out[i][CSpringCankerwormsCR::O_FIRST_STAGE + 3];
		//	m_output[i][O_L2] = out[i][CSpringCankerwormsCR::O_FIRST_STAGE + 4];
		//	m_output[i][O_L3] = out[i][CSpringCankerwormsCR::O_FIRST_STAGE + 5];
		//	m_output[i][O_L4] = out[i][CSpringCankerwormsCR::O_FIRST_STAGE + 6];
		//	m_output[i][O_L5] = out[i][CSpringCankerwormsCR::O_FIRST_STAGE + 7];
		//	m_output[i][O_SOIL] = out[i][CSpringCankerwormsCR::O_FIRST_STAGE + 8];
		//	m_output[i][O_PUPA2] = out[i][CSpringCankerwormsCR::O_FIRST_STAGE + 9];
		//}

		return msg;
	}


	void CSpringCankerwormsModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		ASSERT(header[0] == "KeyID");
		ASSERT(header[1] == "Year");
		ASSERT(header[2] == "Month");
		ASSERT(header[3] == "Day");

		CTRef ref(ToShort(data[1]), ToShort(data[2]) - 1, ToShort(data[3]) - 1);

		ASSERT(header.size() == 13);
		ASSERT(header[4] == "Adult");
		ASSERT(header[5] == "Egg");
		ASSERT(header[6] == "L1");
		ASSERT(header[7] == "L2");
		ASSERT(header[8] == "L3");

		std::vector<double> obs(NB_SPRING_INPUT);
		for (size_t i = 0; i < NB_SPRING_INPUT; i++)
		{
			obs[i] = ToDouble(data[i + 4]);//Cumulative
		}

		m_SAResult.push_back(CSAResult(ref, obs));

	}

	void CSpringCankerwormsModel::GetFValueDaily(CStatisticXY& stat)
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

			for (int i = 1; i < NB_SPRING_PARAMS&&bValid; i++)
			{
				if (m_springCR.m_a[i] < m_springCR.m_a[i - 1])
					bValid = false;
			}

			/*for (int i = 1; i < NB_SPRING_PARAMS1&&bValid; i++)
			{
				if (m_springCR1.m_a[i] < m_springCR1.m_a[i - 1])
					bValid = false;
			}
			for (int i = 1; i < NB_SPRING_PARAMS2&&bValid; i++)
			{
				if (m_springCR2.m_a[i] < m_springCR2.m_a[i - 1])
					bValid = false;
			}
*/
			if (bValid)
			{
				//******************************************************************************************************************************************************
				//Vertical lookup
				if (EVALUATION == VERTICAL)
				{
					CModelStatVector statSim;
					m_springCR.m_bCumul = true;
					m_springCR.Execute(m_weather, statSim);
					for (size_t i = 0; i < m_SAResult.size(); i++)
					{
						ASSERT(m_SAResult[i].m_obs.size() == NB_SPRING_INPUT);
						for (size_t p = S_PUPA_ADULT; p <= S_SOIL_PUPA; p++)
						{
							double obs = m_SAResult[i].m_obs[I_S_ADULT + p];
							if (/*obs > 0 &&
								obs < 100 &&*/
								statSim.IsInside(m_SAResult[i].m_ref))
							{
								double sim = statSim[m_SAResult[i].m_ref][CSpringCankerwormsCR::O_FIRST_STAGE + p + 1];

								stat.Add(sim, obs);
							}
						}
					}

				}
				//******************************************************************************************************************************************************
				//Horizontal lookup
				else if (EVALUATION == HORIZONTAL)
				{
				/*	CModelStatVector statSim1;
					m_springCR1.m_bCumul = true;
					m_springCR1.Execute(m_weather, statSim1);

					for (size_t i = 0; i < m_SAResult.size(); i++)
					{
						ASSERT(m_SAResult[i].m_obs.size() == NB_SPRING_INPUT);
						for (size_t p = S_PUPA_ADULT; p <= S_ADULT_EGG; p++)
						{
							double obsS = m_SAResult[i].m_obs[I_S_ADULT + p];
							if (obsS > 0 &&
								obsS < 100 &&
								statSim1.IsInside(m_SAResult[i].m_ref))
							{
								size_t pp = CSpringCankerwormsCR1::O_FIRST_STAGE + p + 1;

								int year = m_SAResult[i].m_ref.GetYear();
								size_t index = statSim1.GetFirstIndex(pp, obsS, 1, CTPeriod(year, FIRST_MONTH, FIRST_DAY, year, LAST_MONTH, LAST_DAY));
								if (index >= 1)
								{
									CTRef TrefSim = statSim1.GetFirstTRef() + index;
									stat.Add(TrefSim.GetJDay(), m_SAResult[i].m_ref.GetJDay());
								}
							}
						}
					}

					CModelStatVector statSim2;
					m_springCR2.m_bCumul = true;
					m_springCR2.Execute(m_weather, statSim2);

					for (size_t i = 0; i < m_SAResult.size(); i++)
					{
						ASSERT(m_SAResult[i].m_obs.size() == NB_SPRING_INPUT);
						for (size_t p = S_EGG_L1; p <= S_SOIL_PUPA; p++)
						{
							double obsS = m_SAResult[i].m_obs[I_S_L1 + p];
							if (obsS > 0 &&
								obsS < 100 &&
								statSim2.IsInside(m_SAResult[i].m_ref))
							{
								size_t pp = CSpringCankerwormsCR2::O_FIRST_STAGE + p + 1;

								int year = m_SAResult[i].m_ref.GetYear();
								size_t index = statSim2.GetFirstIndex(pp, obsS, 1, CTPeriod(year, FIRST_MONTH, FIRST_DAY, year, LAST_MONTH, LAST_DAY));
								if (index >= 1)
								{
									CTRef TrefSim = statSim2.GetFirstTRef() + index;
									stat.Add(TrefSim.GetJDay(), m_SAResult[i].m_ref.GetJDay());
								}
							}
						}
					}*/
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
	}
}