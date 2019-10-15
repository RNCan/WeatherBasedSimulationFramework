//**********************************************************************
// 02/10/2019	1.0.0	Rémi Saint-Amant	Creation
//**********************************************************************

#include <iostream>
#include "Basic/UtilTime.h"
#include "Basic/UtilMath.h"
#include "ModelBase/EntryPoint.h"
#include "SoilTemperature.h"
//#include "LAI.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;
//using namespace WBSF::LeafAreaIndex;

namespace WBSF
{


	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred = CModelFactory::RegisterModel(CSoilTemperatureModel::CreateObject);


	enum TDailyOutput{ O_SOIL_TEMPERATURE_MIN, O_SOIL_TEMPERATURE_MEAN, O_SOIL_TEMPERATURE_MAX, NB_OUTPUTS };

	//Contructor
	CSoilTemperatureModel::CSoilTemperatureModel()
	{
		//specify the number of input parameter
		NB_INPUT_PARAMETER = 3;
		VERSION = "1.0.0 (2019)";
	}

	CSoilTemperatureModel::~CSoilTemperatureModel()
	{}


	//This method is call to load your parameter in your variable
	ERMsg CSoilTemperatureModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;


		size_t c = 0;
		m_z = parameters[c++].GetReal();
		m_LAI = parameters[c++].GetReal();
		m_LAImax = parameters[c++].GetReal();
		

		return msg;
	}

	
	ERMsg CSoilTemperatureModel::OnExecuteAnnual()
	{
		ERMsg msg;

		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::ANNUAL));
		m_output.Init(p, NB_OUTPUTS, -9999);

		double Tsoil = 0;//
		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			double Litter = 0.5*m_LAImax;
			double LAI = m_LAI;//LAI is constant in this model, need to have a equation during the year


			CStatistic stat = GetSoilTemperature(m_weather[y], m_z, Tsoil, LAI, Litter);
			
			m_output[y][O_SOIL_TEMPERATURE_MIN] = stat[LOWEST];
			m_output[y][O_SOIL_TEMPERATURE_MEAN] = stat[MEAN];
			m_output[y][O_SOIL_TEMPERATURE_MAX] = stat[HIGHEST];

			
		}

		return msg;
	}

	ERMsg CSoilTemperatureModel::OnExecuteMonthly()
	{
		ERMsg msg;


		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::MONTHLY));
		m_output.Init(p, NB_OUTPUTS, -9999);

		double Tsoil = 0;
		for (size_t y = 0; y<m_weather.GetNbYears(); y++)
		{
			double Litter = 0.5*m_LAImax;
			double LAI = m_LAI;//LAI is constant in this model, need to have a equation during the year

			for (size_t m = 0; m<12; m++)
			{
				CStatistic stat = GetSoilTemperature(m_weather[y][m], m_z, Tsoil, LAI, Litter);

				m_output[y * 12 + m][O_SOIL_TEMPERATURE_MIN] = stat[LOWEST];
				m_output[y * 12 + m][O_SOIL_TEMPERATURE_MEAN] = stat[MEAN];
				m_output[y * 12 + m][O_SOIL_TEMPERATURE_MAX] = stat[HIGHEST];
				
			}
		}


		return msg;
	}


	ERMsg CSoilTemperatureModel::OnExecuteDaily()
	{
		ERMsg msg;

		//size_t m_forestCoverType = BLACK_SPRUCE_FENS;
		//double m_quantile = 0.5;

		//m_LAImax = 0;
		//for(size_t doy=0; doy<365; doy++)
		//	m_LAImax = max(m_LAImax, CLeafAreaIndex::ComputeLAI(doy, m_forestCoverType, m_quantile));


		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));
		m_output.Init(p, NB_OUTPUTS, -9999);

		double Tsoil = 0;

		for (size_t y = 0; y<m_weather.size(); y++)
		{
			//Ground litter as LAI equivalent
			double Litter = 0.5*m_LAImax;
			double LAI = m_LAI;//LAI is constant in this model, need to have a equation during the year
			
			for (size_t m = 0; m<m_weather[y].size(); m++)
			{
				for (size_t d = 0; d<m_weather[y][m].size(); d++)
				{
					CTRef ref = m_weather[y][m][d].GetTRef();
					//double LAI = CLeafAreaIndex::ComputeLAI(ref.GetJDay(), m_forestCoverType, m_quantile);

					CStatistic stat = GetSoilTemperature(m_weather[y][m][d], m_z, Tsoil, LAI, Litter);

					m_output[ref][O_SOIL_TEMPERATURE_MIN] = stat[LOWEST];
					m_output[ref][O_SOIL_TEMPERATURE_MEAN] = stat[MEAN];
					m_output[ref][O_SOIL_TEMPERATURE_MAX] = stat[HIGHEST];
					
				}
			}
		}


		return msg;
	}

	ERMsg CSoilTemperatureModel::OnExecuteHourly()
	{
		ERMsg msg;

		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();

		CTPeriod p = m_weather.GetEntireTPeriod(CTM::HOURLY);
		m_output.Init(p, NB_OUTPUTS, -9999);

		
		CSun sun(m_weather.m_lat, m_weather.m_lon);
		
		
		for (size_t y = 0; y<m_weather.size(); y++)
		{
			for (size_t m = 0; m<m_weather[y].size(); m++)
			{
				for (size_t d = 0; d<m_weather[y][m].size(); d++)
				{
					for (size_t h = 0; h < m_weather[y][m][d].size(); h++)
					{
					}
				}
			}
		}


		return msg;
	}


	
	CStatistic CSoilTemperatureModel::GetSoilTemperature(const CWeatherYear& weather, double z, double& Tsoil, double LAI, double& Litter)
	{
		CStatistic stat;
		for (size_t m = 0; m < weather.size(); m++)
			stat += GetSoilTemperature(weather[m], z, Tsoil, LAI, Litter);

		return stat[MEAN];
	}
	
	CStatistic CSoilTemperatureModel::GetSoilTemperature(const CWeatherMonth& weather, double z, double& Tsoil, double LAI, double& Litter)
	{
		CStatistic stat;
		for (size_t d = 0; d < weather.size(); d++)
			stat += GetSoilTemperature(weather[d], z, Tsoil, LAI, Litter);

		return stat;
	}

	CStatistic CSoilTemperatureModel::GetSoilTemperature(const CWeatherDay& weather, double z, double& Tsoil, double LAI, double& Litter)
	{
		CStatistic stat;
		
		
		double Tair = weather[H_TAIR][MEAN];
		if (Tair > 0)
			Litter -= 0.01*Litter;


		if (weather.IsHourly())
		{
			ASSERT(false);//todo
		}
		else
		{
			Tsoil = GetSoilTemperature(z, Tair, Tsoil, LAI, Litter);
			stat += Tsoil;
		}

		return stat;
	}

	//Tair: mean air temperature of actual day [°C]
	//Tsoil: soil temperature of last day [°C]
	//LAI: Leaf area index [m²/m²]
	//Litter: Ground litter as LAI equivalent [m²/m²]
	//z:	soil depth [cm]
	//return soil temperature [°C]
	//Note that the initial value of soil temperature is calculated by multiplying DRz by air
	//temperature of the first Julian day and LAI equivalent of above ground litter is assumed as a half of the maximum LAI.
	//the first time, Tsoil must be 0.
	double CSoilTemperatureModel::GetSoilTemperature(double z, double Tair, double Tsoil, double LAI, double Litter)
	{
		CStatistic stat;

		const double Ks=0.005;//soil thermal diffusivity [cm²/s]. possible range  [0.001, 0.01]
		//const double p = 365.0*24.0 * 60.0 *60.0; //[s]// does not seem to work with this value!!!!
		const double p = 24.0 * 60.0 *60.0; //[s]
		const double k = 0.45;//extinction coefficient


		double DRz = exp(-z * sqrt(PI / (Ks*p)));//range from [0.93, 0.97]
		Tsoil = Tsoil + (Tair - Tsoil) * DRz*exp(-k *(LAI+ Litter) );

		//if Tair > Tsoil 
		//if (Tair > Tsoil)
			//Tsoil = Tsoil + (Tair - Tsoil) * DRz*exp(-k * (LAI + Litter));
		//else//Tair <= Tsoil
			//Tsoil = Tsoil + (Tair - Tsoil) * DRz*exp(-k * Litter);
		
		return Tsoil;
	}
	
}