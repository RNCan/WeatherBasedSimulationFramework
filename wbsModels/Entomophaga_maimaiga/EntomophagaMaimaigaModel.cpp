//**********************************************************************
// 03/06/2025	1.0		Rémi Saint-Amant	Creation
//**********************************************************************

#include "EntomophagaMaimaigaModel.h"
#include "Basic/Evapotranspiration.h"
#include "Basic/DegreeDays.h"
#include "Basic/GrowingSeason.h"
#include "ModelBase/EntryPoint.h"
#include "eco-climate.h"

//#include "inputdata.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;

//int RunModel(const EMParameters& Params, const EMWeatherYear& CCDATA, std::vector<double>& ave);

namespace WBSF
{

	//INFECTION: 

	enum TAnnualStat { O_INFECTION, NB_OUTPUTS };


	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CEntomophagaMaimaigaModel::CreateObject);

	//Contructor
	CEntomophagaMaimaigaModel::CEntomophagaMaimaigaModel()
	{
		//specify the number of input parameter
		NB_INPUT_PARAMETER = 2;
		VERSION = "1.0.0 (2025)";

		//m_nb_years = 1;//Annual;
	}

	CEntomophagaMaimaigaModel::~CEntomophagaMaimaigaModel()
	{
	}



	//this method is call to load your parameter in your variable
	ERMsg CEntomophagaMaimaigaModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		//transfer your parameter here
		int c = 0;
		m_host_density = parameters[c++].GetReal();// *250 / 10000;//egg masses/ha --> larvae/m²
		size_t fd = parameters[c++].GetInt();
		ASSERT(fd < EMParameters::NB_FUNGUS_DESITIES);
		m_fungus_density = EMParameters::INITIAL_R[fd];
		

		return msg;
	}

	ERMsg CEntomophagaMaimaigaModel::ExecuteDaily(CWeatherYears& weather, CModelStatVector &output)
	{
		if (!m_weather.IsHourly())
		{
			//m_weather.ComputeHourlyVariables();
		}

		ERMsg msg;

		CTPeriod p = m_weather.GetEntireTPeriod(CTM::DAILY);
		output.Init(p, NB_OUTPUTS, -999);


		EMParameters Params;

		Params.initS = m_host_density;
		Params.initR = m_fungus_density;



		EMWeatherYears CCDATA(m_weather.GetNbYears());
		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			size_t DOY = 0;
			//for (size_t DOY = 0; DOY < 365; y++)
			for (size_t m = 0; m < 12; m++)
			{
				for (size_t d = 0; d < GetNbDayPerMonth(m); d++, DOY++)//fixed year at 365 days, don't use 29 of february
				{
					const CWeatherDay& wDay = m_weather[y][m][d];

					CCDATA[y][DOY][EM_PRCP] = wDay[H_PRCP][SUM];
					CCDATA[y][DOY][EM_RH_MIN] = WBSF::Td2Hr(wDay[H_TMAX], wDay[H_TDEW]);//Estimate of RHlow to avoid hourly compution
					CCDATA[y][DOY][EM_TMAX] = wDay[H_TMAX][HIGHEST];
					CCDATA[y][DOY][EM_TAIR] = wDay[H_TAIR][MEAN];
				}
			}
		}


		std::vector<std::array<double, 365> > infected = execute_ecoclimate_model(Params, CCDATA);
		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			int year = m_weather[y].GetTRef().GetYear();
			for (size_t d = 0; d < 365; d++)
			{
				output[CJDayRef(year,d)][O_INFECTION] = infected[y][d];
			}

			//copy lastd day for leap year
			if(IsLeap(year))
				output[CJDayRef(year, 365)][O_INFECTION] = infected[y][364];//take the last days
		}

		return msg;
	}

	ERMsg CEntomophagaMaimaigaModel::OnExecuteDaily()
	{
		return ExecuteDaily(m_weather, m_output);
	}


	ERMsg CEntomophagaMaimaigaModel::OnExecuteAnnual()
	{
		ERMsg msg;
		
		CModelStatVector daily;
		ExecuteDaily(m_weather, daily);


		CTPeriod p = m_weather.GetEntireTPeriod(CTM::ANNUAL);
		m_output.Init(p, NB_OUTPUTS, -999);
		
		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			int year = m_weather[y].GetTRef().GetYear();
			m_output[y][O_INFECTION] = daily[CJDayRef(year,364)][O_INFECTION];//take the last days
		}

		return msg;

	}


}