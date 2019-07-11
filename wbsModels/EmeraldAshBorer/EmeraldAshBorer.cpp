//**************************************************************************************************************
// 13/04/2018	1.0.1	Rémi Saint-Amant	Compile with VS 2017
// 08/05/2017	1.0.0	Rémi Saint-Amant	Create from articles
//												Lyons and Jones 2006
//**************************************************************************************************************

#include "ModelBase/EntryPoint.h"
#include "ModelBase/ContinuingRatio.h"
#include "EmeraldAshBorer.h"
#include "TreeMicroClimate.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;


namespace WBSFh
{

	//links this class with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CEmeraldAshBorerModel::CreateObject);


	//Defining a simple continuing ratio model
	enum TInstar{ EGG, L1, L2, L3, L4, PRE_PUPA, PUPA, TENERAL, ADULT, DEAD_ADULT, EMERGENCE, NB_STAGES };
	extern const char HEADER[] = "EGG,L1,L2,L3,L4,Pre-Pupa,Pupa,Teneral,Adult,DeadAdult,Emergence";
	//typedef CContinuingRatio<NB_PARAMS, EGG, ADULT, HEADER> CHemlockLooperCR;
	static const double T_THRESHOLD[2][NB_STAGES] =
	{
		{ 13.9, 0, 0, 0, 0, 12.0, 13.6, 13.6, 0 },
		{ 13.9, 0, 0, 0, 0, 11.5, 14.7, 10.1, 0 },
	};

	static const double DD_THRESHOLD[2][NB_STAGES] =
	{
		{ 155.2, 0, 0, 0, 0, 118.3, 139.2, 43.1, 0 },
		{ 155.2, 0, 0, 0, 0, 121.0, 114.6, 64.4, 0 },
	};

	//const size_t CEmeraldAshBorerModel::FISRT_HL_JDAY = 61 - 1;   //first of March zero base
	//const double CEmeraldAshBorerModel::THRESHOLD = 5.4;			//°C
	//const double CEmeraldAshBorerModel::A[NB_PARAMS] = { 179.7, 259.8, 398.0, 495.1, 723.8, 1075.5 };	//accumulated degree-days
	//const double CEmeraldAshBorerModel::B[NB_PARAMS] = { 0.637, 2.591, 1.564, 1.414, 1.531, 1.2437 };	//variability
	//const double CEmeraldAshBorerModel::A[NB_PARAMS] = { 179.7, 259.8, 398.0, 495.1, 723.8, 945.9};	//accumulated degree-days
	//const double CEmeraldAshBorerModel::B[NB_PARAMS] = { 0.637, 2.591, 1.564, 1.414, 1.531, 0.741 };	//variability


	CEmeraldAshBorerModel::CEmeraldAshBorerModel()
	{
		NB_INPUT_PARAMETER = -1;
		VERSION = "1.0.1 (2018)";
	}

	CEmeraldAshBorerModel::~CEmeraldAshBorerModel()
	{}

	//this method is called to load the generic parameters vector into the specific class member
	ERMsg CEmeraldAshBorerModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		size_t c = 0;
		if (parameters.size() == 7)
		{
		}

		return msg;
	}

	ERMsg CEmeraldAshBorerModel::OnExecuteDaily()
	{
		ERMsg msg;

		//Excute model on a daily basis
		ExecuteDaily(m_output);

		return msg;
	}

	void CEmeraldAshBorerModel::ExecuteDaily(CModelStatVector& output)
	{
		//for (size_t y = 0; y < m_weather.size(); y++)
			//m_weather[y][H_TAIR];

		//m_weather.SetHourly(false);


		//execute daily using continuing ratio structure
		/*CHemlockLooperCR CR;
		CR.m_startJday = m_startJday;
		CR.m_lowerThreshold = m_threshold;
		CR.m_method = CDegreeDays::DAILY_AVERAGE;
		CR.m_bPercent = true;
		CR.m_bCumul = m_bCumulative;
		CR.m_bMultipleVariance = true;
		CR.m_bAdjustFinalProportion = true;*/

	/*	for (size_t i = 0; i < NB_PARAMS; i++)
		{
			CR.m_a[i] = m_a[i];
			CR.m_b[i] = m_b[i];
		}*/

		output.Init(m_weather.GetEntireTPeriod(), NB_STAGES, 0, HEADER);//+3 for DD, death and AI
		COverheat overheating(0.0);

		for (size_t y = 0; y < m_weather.size(); y++)
		{
			size_t stage[2] = { PRE_PUPA, PRE_PUPA };
			double DD[2] = { 0 };
			double DD_E[2] = { 0 };

			//double DD = 0.0;
			CTPeriod p = m_weather[y].GetEntireTPeriod(CTM::DAILY);

			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			{
				const CWeatherDay& wDay = m_weather.GetDay(TRef);
				CMicroClimate micro(wDay);
				double T = micro.GetTair();
				
				for (size_t s = 0; s < 2; s++)
				{
					if (stage[s] < ADULT)
					{
						//if (TRef.GetJDay() >= m_startJday)
						DD[s] += max(0.0, T - T_THRESHOLD[s][stage[s]]);
						if (DD[s] > DD_THRESHOLD[s][stage[s]])
						{
							DD[s] = 0;
							stage[s] ++;
						}
					}
					else if (stage[s] == ADULT)
					{
						for (size_t h = 0; h < 24; h++)
						{
							double R = 0.0004130*exp(0.187197*T);
							DD[s] += R/24;
						}

						if (DD[s] >= 1)
						{
							DD[s] = 0;
							stage[s] ++;
						}
					}

					static const double MAX_DDE[2] = {303.0,344.8};
					if (DD_E[s] < MAX_DDE[s] && DD_E[s] + max(0.0, T - 13.5 ) >= MAX_DDE[s])
					{
						output[TRef][EMERGENCE] += 50;
					}
					DD_E[s] += max(0.0, T - 13.5);

					output[TRef][stage[s]] += 50;
				}
			}
		}
		
		//CR.Execute(m_weather, output);
	}

}