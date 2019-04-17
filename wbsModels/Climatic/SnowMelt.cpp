//**********************************************************************
// 25/04/2017	1.0.0	Rémi Saint-Amant    Creation
//**********************************************************************

#include <iostream>
#include "Basic/UtilTime.h"
#include "Basic/UtilMath.h"
#include "ModelBase/EntryPoint.h"
#include "SnowMelt.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;


namespace WBSF
{
	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred = CModelFactory::RegisterModel(CSnowMelt::CreateObject);
	
	enum TMonthlyStat{ O_SWE_START, O_SWE_END, O_SNOWDIFF, O_PRCP, O_RUNOFF, NB_MONTHLY_OUTPUTS};

	//Contructor
	CSnowMelt::CSnowMelt()
	{
		//specify the number of input parameter
		NB_INPUT_PARAMETER = 0;

		VERSION = "1.0.0 (2019)";
	}

	CSnowMelt::~CSnowMelt()
	{}

	//This method is call to load your parameter in your variable
	ERMsg CSnowMelt::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		return msg;
	}

	

	ERMsg CSnowMelt::OnExecuteMonthly()
	{
		ERMsg msg;

		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::MONTHLY));
		m_output.Init(p, NB_MONTHLY_OUTPUTS);

		for (size_t y = 0; y<m_weather.GetNbYears(); y++)
		{
			for (size_t m = 0; m<12; m++)
			{
				double SWE_start = m_weather[y][m][0][H_SWE][MEAN];
				double SWE_end = m_weather[y][m].HaveNext()?m_weather[y][m].GetNext()[0][H_SWE][MEAN]: m_weather[y][m][m_weather[y][m].GetNbDays()-1][H_SWE][MEAN];

				double prcp = m_weather[y][m][H_PRCP][SUM];
				double snow_diff = SWE_end - SWE_start;
				
				m_output[y * 12 + m][O_SWE_START] = SWE_start;
				m_output[y * 12 + m][O_SWE_END] = SWE_end;
				m_output[y * 12 + m][O_SNOWDIFF] = snow_diff;
				//m_output[y * 12 + m][O_SNOWMELT] = max(0.0, -snow_diff);
				m_output[y * 12 + m][O_PRCP] = prcp;
				m_output[y * 12 + m][O_RUNOFF] = prcp -snow_diff;

			}
		}


		return msg;
	}


	
}