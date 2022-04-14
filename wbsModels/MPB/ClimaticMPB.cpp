//**********************************************************************
// 14/04/2022	1.0.0	Rémi Saint-Amant    Model create for Kishan
//**********************************************************************
#include <stdio.h>
#include <math.h>
#include <crtdbg.h>
#include <float.h>
#include <limits>

#include "basic/WeatherDefine.h"
#include "ModelBase/EntryPoint.h"
#include "ClimaticMPB.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;


namespace WBSF
{
	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CClimaticMPB::CreateObject);



	CClimaticMPB::CClimaticMPB()
	{
		NB_INPUT_PARAMETER = 0;
		VERSION = "1.0.0 (2022)";

		// initialize your variable here (optional)
	}

	CClimaticMPB::~CClimaticMPB()
	{}


	//	nbDay annual lowest Tmin lower than -20°C
	//	percent of days when lowest Tmin lower than -20°C
	//	nbDay annual highest Tmax greater than 32°C
	//	percent of days when highest Tmax greater than 32°C
	
	enum TOutput
	{
		O_NB_DAYS_LOWER20, O_PERCENT_LOWER20, NB_DAYS_GREATER32, NB_PERCENT_GREATER32,	NB_OUTPUTS
	};

	//this method is call to load your parameter in your variable
	ERMsg CClimaticMPB::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg message;

		//transfer your parameter here
		int c = 0;
		return message;
	}


	ERMsg CClimaticMPB::OnExecuteAnnual()
	{
		ERMsg message;

		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::ANNUAL));
		p.Begin().m_year++;

		m_output.Init(p, NB_OUTPUTS, -999);


		for (size_t y = 1; y < m_weather.size(); y++)
		{
			int year = m_weather[y].GetTRef().GetYear();
			CTPeriod winterPeriod(CTRef(year-1, DECEMBER, FIRST_DAY), CTRef(year, FEBRUARY, LAST_DAY));
			CTPeriod summerPeriod(CTRef(year, MAY, FIRST_DAY), CTRef(year, SEPTEMBER, LAST_DAY));

			

			int nbDaysLower20 = 0;
			for (CTRef TRef = winterPeriod.Begin(); TRef <= winterPeriod.End(); TRef++)
			{
				double T = m_weather[TRef][H_TMIN][LOWEST];
				if (T <= -20.0)
					nbDaysLower20 ++;
			}
			
			int nbDaysGreater32 = 0;
			for (CTRef TRef = summerPeriod.Begin(); TRef <= summerPeriod.End(); TRef++)
			{
				double T = m_weather[TRef][H_TMAX][HIGHEST];
				if (T >= 32.0)
					nbDaysGreater32++;
			}

			m_output[y-1][O_NB_DAYS_LOWER20] = nbDaysLower20;
			m_output[y-1][O_PERCENT_LOWER20] = 100.0 * nbDaysLower20/ winterPeriod.GetNbDay(); //[%]
			m_output[y-1][NB_DAYS_GREATER32] = nbDaysGreater32;
			m_output[y-1][NB_PERCENT_GREATER32] = 100.0*nbDaysGreater32 / summerPeriod.GetNbDay();//[%]
			
			HxGridTestConnection();
		}



		return message;
	}




}