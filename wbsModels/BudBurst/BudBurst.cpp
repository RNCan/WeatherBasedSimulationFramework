//*********************************************************************
//17/04/2019	1.0.0	Rémi Saint-Amant	Creation
//*********************************************************************
#include "BudBurst.h"
#include "Basic/WeatherDefine.h"
#include "ModelBase/EntryPoint.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;

namespace WBSF
{

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CBudBurst::CreateObject);


	CBudBurst::CBudBurst()
	{
		// initialize your variable here (optional)
		NB_INPUT_PARAMETER = 1;
		VERSION = "1.0.0 (2019)";
	}

	CBudBurst::~CBudBurst()
	{
	}

	//This method is call to compute solution
	ERMsg CBudBurst::OnExecuteAnnual()
	{
		ERMsg msg;

		CTPeriod pp(m_weather.GetEntireTPeriod(CTM::ANNUAL));
		pp.Begin().m_year++;
		m_output.Init(pp, 3);


		static const double Sw150 = 16.891;
		static const double α2 = -0.0675;

		for (size_t y = 0; y < m_weather.GetNbYears()-1; y++)
		{ 
			CTRef budBurst;
			double sum = 0;
			int dc = 0;

			int year = m_weather[y].GetTRef().GetYear();
			CTPeriod p(CTRef(year, DECEMBER, DAY_01), CTRef(year+1, DECEMBER, DAY_01));
			for (CTRef TRef = p.Begin(); TRef < p.End()&& !budBurst.IsInit(); TRef++)
			{
				const CWeatherDay&  wDay = m_weather.GetDay(TRef);
				
				if (wDay[H_TAIR][MEAN] < 10)
					dc++;
				else
					sum+= wDay[H_TAIR][MEAN] - 10;

				double Sw = Sw150 * exp(α2*(dc - 150.0));
				if(sum >= Sw)
				{ 
					budBurst = TRef;
				}
			}

			if (budBurst.IsInit())
			{
				m_output[y][0] = dc;
				m_output[y][1] = Sw150 * exp(α2*(dc - 150.0));
				m_output[y][2] = budBurst.GetRef();
			}
		}
		

		return msg;
	}

	//this method is call to load your parameter in your variable
	ERMsg CBudBurst::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		//transfer your parameter here
		short c=0;
		m_species = parameters[c++].GetInt();

		return msg;
	}
	
}