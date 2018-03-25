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
	//vertical
	NbVal=   231	Bias= 0.18823	MAE=13.73028	RMSE=18.02138	CD= 0.71451	R²= 0.71454
	Threshold1          	=   2.75001
	Threshold2          	=   3.00005
	a1                  	=  87.69302
	b1                  	=   1.70334
	a2                  	= 108.82778
	b2                  	=   2.78009
	a3                  	= 328.59981
	b3                  	=   1.65588
	a4                  	= 424.13127
	b4                  	=   1.32959
	a5                  	= 479.71700
	b5                  	=   1.25780
	a6                  	= 535.04164
	b6                  	=   1.02907
	a7                  	= 604.46298
	b7                  	=   1.02028
	a8                  	= 699.84631
	b8                  	=   0.99734
	a9                  	= 799.59153
	b9                  	=   0.83768
NbVal=  1737	Bias= 0.00374	MAE= 2.25896	RMSE= 6.97021	CD= 0.97881	R²= 0.97882
Threshold1          	=   3.00000
Threshold2          	=   2.80156
a1                  	=  82.99716
b1                  	=   1.50046
a2                  	= 101.60114
b2                  	=   2.09849
a3                  	= 344.46452
b3                  	=   1.23550
a4                  	= 441.16816
b4                  	=   1.04713
a5                  	= 496.19798
b5                  	=   1.01307
a6                  	= 549.80672
b6                  	=   0.92700
a7                  	= 620.20480
b7                  	=   0.89188
a8                  	= 716.50653
b8                  	=   0.89952
a9                  	= 818.73572
b9                  	=   0.75508
		*/




	enum Toutput{ O_PUPA1, O_ADULT, O_EGG, O_L1, O_L2, O_L3, O_L4, O_L5, O_SOIL, O_PUPA2, NB_OUTPUT };
	const char SpringCankerworms_header[] = "PUPA1|ADULT|EGG|L1|L2|L3|L4|L5|SOIL|PUPA2";
	enum TEvaluation { VERTICAL, HORIZONTAL, DIAGONAL };
	static const TEvaluation EVALUATION = VERTICAL;

	const double CSpringCankerwormsModel::THRESHOLD1 = 3.0;// 2.75;
	const double CSpringCankerwormsModel::THRESHOLD2 = 2.8;// 3.00;
 



	const double CSpringCankerwormsModel::A1[NB_SPRING_PARAMS1] =
	{
//		87.7,108.8
		82.99716,101.60114
	};

	const double CSpringCankerwormsModel::B1[NB_SPRING_PARAMS1] =
	{
//		1.70334,2.78009
		1.50046,2.09849
	};





	const double CSpringCankerwormsModel::A2[NB_SPRING_PARAMS2] =
	{
		//328.6,424.1,479.7,535.0,604.5,699.8,799.6
		344.46452,441.16816,496.19798,549.80672,620.20480,716.50653,818.73572
	};

	const double CSpringCankerwormsModel::B2[NB_SPRING_PARAMS2] =
	{
		//1.65588,1.32959,1.25780,1.02907,1.02028,0.99734,0.83768
		1.23550,1.04713,1.01307,0.92700,0.89188,0.89952,0.75508
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
		m_springCR1.m_startJday = 0;
		m_springCR1.m_lowerThreshold = THRESHOLD1;
		m_springCR1.m_bCumul = bCumul;
		m_springCR1.m_bMultipleVariance = true;
		m_springCR1.m_bPercent = true;
		m_springCR1.m_bAdjustFinalProportion = true;
		m_springCR1.m_method = CSpringCankerwormsCR1::DAILY_AVERAGE;
		for (size_t i = 0; i < NB_SPRING_PARAMS1; i++)
		{
			m_springCR1.m_a[i] = A1[i];
			m_springCR1.m_b[i] = B1[i];
		}

		m_springCR2.m_startJday = 0;
		m_springCR2.m_lowerThreshold = THRESHOLD2;
		m_springCR2.m_bCumul = bCumul;
		m_springCR2.m_bMultipleVariance = true;
		m_springCR2.m_bPercent = true;
		m_springCR2.m_bAdjustFinalProportion = true;
		m_springCR2.m_method = CSpringCankerwormsCR2::DAILY_AVERAGE;
		
		for (size_t i = 0; i < NB_SPRING_PARAMS2; i++)
		{
			m_springCR2.m_a[i] = A2[i];
			m_springCR2.m_b[i] = B2[i];
		}

		if (parameters.size() > 1)
		{
			m_springCR1.m_lowerThreshold = parameters[c++].GetFloat();
			m_springCR2.m_lowerThreshold = parameters[c++].GetFloat();

			for (size_t i = 0; i < NB_SPRING_PARAMS1; i++)
			{
				m_springCR1.m_a[i] = parameters[c++].GetFloat();
				m_springCR1.m_b[i] = parameters[c++].GetFloat();
			}

			for (size_t i = 0; i < NB_SPRING_PARAMS2; i++)
			{
				m_springCR2.m_a[i] = parameters[c++].GetFloat();
				m_springCR2.m_b[i] = parameters[c++].GetFloat();
			}
		}

		return msg;
	}


	ERMsg CSpringCankerwormsModel::OnExecuteDaily()
	{
		ERMsg msg;

		CModelStatVector out1;
		CModelStatVector out2;
		m_springCR1.Execute(m_weather, out1);
		m_springCR2.Execute(m_weather, out2);

		m_output.Init(m_weather.GetEntireTPeriod(CTM::DAILY), NB_OUTPUT);
		for (size_t i = 0; i < out1.size(); i++)
		{
			m_output[i][O_PUPA1] = out1[i][CSpringCankerwormsCR1::O_FIRST_STAGE + 0];
			m_output[i][O_ADULT] = out1[i][CSpringCankerwormsCR1::O_FIRST_STAGE + 1];
			m_output[i][O_EGG] = out1[i][CSpringCankerwormsCR1::O_FIRST_STAGE + 2];
			m_output[i][O_L1] = out2[i][CSpringCankerwormsCR2::O_FIRST_STAGE + 1];
			m_output[i][O_L2] = out2[i][CSpringCankerwormsCR2::O_FIRST_STAGE + 2];
			m_output[i][O_L3] = out2[i][CSpringCankerwormsCR2::O_FIRST_STAGE + 3];
			m_output[i][O_L4] = out2[i][CSpringCankerwormsCR2::O_FIRST_STAGE + 4];
			m_output[i][O_L5] = out2[i][CSpringCankerwormsCR2::O_FIRST_STAGE + 5];
			m_output[i][O_SOIL] = out2[i][CSpringCankerwormsCR2::O_FIRST_STAGE + 6];
			m_output[i][O_PUPA2] = out2[i][CSpringCankerwormsCR2::O_FIRST_STAGE + 7];
		}

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

			for (int i = 1; i < NB_SPRING_PARAMS1&&bValid; i++)
			{
				if (m_springCR1.m_a[i] < m_springCR1.m_a[i - 1])
					bValid = false;
			}
			for (int i = 1; i < NB_SPRING_PARAMS2&&bValid; i++)
			{
				if (m_springCR2.m_a[i] < m_springCR2.m_a[i - 1])
					bValid = false;
			}

			if (bValid)
			{
				//******************************************************************************************************************************************************
				//Vertical lookup
				if (EVALUATION == VERTICAL)
				{
					CModelStatVector statSim1;
					m_springCR1.m_bCumul = true;
					m_springCR1.Execute(m_weather, statSim1);
					for (size_t i = 0; i < m_SAResult.size(); i++)
					{
						ASSERT(m_SAResult[i].m_obs.size() == NB_SPRING_INPUT);
						for (size_t p = S_PUPA_ADULT; p <= S_ADULT_EGG; p++)
						{
							double obs = m_SAResult[i].m_obs[I_S_ADULT + p];
							if (/*obs > 0 &&
								obs < 100 &&*/
								statSim1.IsInside(m_SAResult[i].m_ref))
							{
								double sim = statSim1[m_SAResult[i].m_ref][CSpringCankerwormsCR1::O_FIRST_STAGE + p + 1];

								stat.Add(sim, obs);
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
							double obs = m_SAResult[i].m_obs[I_S_L1 + p];
							if (//obs > 0 &&
								//obs < 100 &&
								statSim2.IsInside(m_SAResult[i].m_ref))
							{
								
								double sim = statSim2[m_SAResult[i].m_ref][CSpringCankerwormsCR2::O_FIRST_STAGE + p + 1];

								stat.Add(sim, obs);
							}
						}
					}
				}
				//******************************************************************************************************************************************************
				//Horizontal lookup
				else if (EVALUATION == HORIZONTAL)
				{
					CModelStatVector statSim1;
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
					}
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