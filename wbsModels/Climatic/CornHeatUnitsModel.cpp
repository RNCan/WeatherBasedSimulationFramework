//**********************************************************************
// 02/10/2019	1.0.0	Rémi Saint-Amant	Creation
//**********************************************************************

#include <iostream>
#include "Basic/UtilTime.h"
#include "Basic/UtilMath.h"
#include "ModelBase/EntryPoint.h"
#include "CornHeatUnitsModel.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;

namespace WBSF
{
	
	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred = CModelFactory::RegisterModel(CCornHeatUnitsModel::CreateObject);


	enum TDailyOutput { O_CHU, NB_OUTPUTS };
	//enum TOtherOutput { O_LAI_MIN, O_LAI_MEAN, O_LAI_MAX, NB_OTHER_OUTPUTS };

	//Constructor
	CCornHeatUnitsModel::CCornHeatUnitsModel()
	{
		//specify the number of input parameter
		NB_INPUT_PARAMETER = 2;
		VERSION = "1.0.0 (2020)";

		m_planting_date.Set("05-15");
		m_frost_threshold = -1;
	}

	CCornHeatUnitsModel::~CCornHeatUnitsModel()
	{}


	//This method is call to load your parameter in your variable
	ERMsg CCornHeatUnitsModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;


		size_t c = 0;
		
		m_planting_date.Set(parameters[c++].GetString());
		if (!m_planting_date.IsValid())
			msg.ajoute("Invalid planting date. Date must have format \"mm-dd\" ");

		m_frost_threshold = parameters[c++].GetReal();
		
		return msg;
	}


	ERMsg CCornHeatUnitsModel::OnExecuteAnnual()
	{
		ERMsg msg;

		CModelStatVector output;
		ExecuteDaily(output);

		//kep annual value
		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::ANNUAL));
		m_output.Init(p, NB_OUTPUTS, -9999);


		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			CTRef TRef = m_weather[y].GetEntireTPeriod().End();
			m_output[y][O_CHU] = output[TRef][LOWEST];
		}

		return msg;
	}



	ERMsg CCornHeatUnitsModel::OnExecuteDaily()
	{
		ERMsg msg;

		ExecuteDaily(m_output);

		return msg;
	}
	
	void CCornHeatUnitsModel::ExecuteDaily(CModelStatVector& output)
	{
		ERMsg msg;

		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));
		output.Init(p, NB_OUTPUTS, 0);


		for (size_t y = 0; y < m_weather.size(); y++)
		{
			bool bFrosted=false;
			double sum_CHU = 0;
			for (size_t m = 0; m < m_weather[y].size(); m++)
			{
				for (size_t d = 0; d < m_weather[y][m].size(); d++)
				{
					CTRef ref = m_weather[y][m][d].GetTRef();
					if (ref >= m_planting_date.GetTRef(ref))
					{
						if(ref.GetJDay()>182 && m_weather[y][m][d][H_TMIN][LOWEST] <= m_frost_threshold)
							bFrosted = true;

						if (!bFrosted)
							sum_CHU += GetCHU(m_weather[y][m][d]);

						m_output[ref][O_CHU] = sum_CHU;
						
					}
				}
			}
		}
	}

	



	double CCornHeatUnitsModel::GetCHU(const CWeatherDay& weather)
	{
		double Tmin = weather[H_TMIN][HIGHEST];
		double Tmax = weather[H_TMAX][LOWEST];
		double CHU = max(0.0, (1.8 *(Tmin - 4.4) + 3.3 *(Tmax - 10) - 0.084*Square (Tmax - 10) ) / 2.0);

		return CHU;
	}

	
	
	
}