//**********************************************************************
// 26/09/2023	1.0.1	Rémi Saint-Amant	Add overheating and snow protection. 
//											No longer used mean average over 11 days period
// 02/10/2019	1.0.0	Rémi Saint-Amant	Creation
//**********************************************************************

//Calibrated with 32 station 2013-2022 from U.S. Climate Reference Network
//Correction apply for snow depth, overheating and litter
//Model base on Kang 2000: Predicting spatial and temporal patterns of soil temperature
//based on topography, surface cover and air temperature
//but with many modifications to take effect of snow.

#include <valarray>
#include <iostream>
#include "Basic/UtilTime.h"
#include "Basic/UtilMath.h"
#include "ModelBase/EntryPoint.h"
#include "SoilTemperature.h"
//#include "LAI.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;

namespace WBSF
{
	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred = CModelFactory::RegisterModel(CSoilTemperatureModel::CreateObject);


	enum TDailyOutput { O_SOIL_TEMPERATURE_MIN, O_SOIL_TEMPERATURE_MEAN, O_SOIL_TEMPERATURE_MAX, NB_OUTPUTS };

	//Constructor
	CSoilTemperatureModel::CSoilTemperatureModel()
	{
		//specify the number of input parameter
		NB_INPUT_PARAMETER = -1;
		VERSION = "1.0.1 (2013)";

		m_F = 4.0;
		m_Fo = 0.096;
	}

	CSoilTemperatureModel::~CSoilTemperatureModel()
	{}


	//This method is call to load your parameter in your variable
	ERMsg CSoilTemperatureModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;


		size_t c = 0;
		m_z = parameters[c++].GetReal();// / 100; //cm -> m
		m_LAI = parameters[c++].GetReal();
		m_Litter = parameters[c++].GetReal();

		if (parameters.size() == 9)
		{
			m_F = parameters[c++].GetReal();
			m_Fo = parameters[c++].GetReal();
			m_Cs = parameters[c++].GetReal();
			m_Kt = parameters[c++].GetReal();
			m_Cice = parameters[c++].GetReal();
			m_Fs = parameters[c++].GetReal();
		}
		else
		{
			//correction factor
			m_F = max(0.0, min(8.0, 11.8 * pow(m_z, -0.740)));

			//overheating factor in function of depth
			m_Fo = max(0.0, min(0.3, 0.212*pow(m_z,-0.224)));

			//litter correction
			//double F_litter = max(0.0936, 0.389 * log(m_z) + 1.80);
			//m_Litter *= F_litter;
			//m_Litter = 0;
		}

		return msg;
	}


	//ERMsg CSoilTemperatureModel::OnExecuteAnnual()
	//{
	//	ERMsg msg;

	//	CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::ANNUAL));
	//	m_output.Init(p, NB_OUTPUTS, -9999);

	//	double Tsoil = 0;//

	//	//init soil temperature with December
	//	//double Litter = 0.5 * m_LAImax;
	//	for (size_t d = 0; d < m_weather[size_t(0)][DECEMBER].size(); d++)
	//	{
	//		GetSoilTemperature(m_weather[size_t(0)], m_z, Tsoil, A, m_LAI, m_Litter, m_F, m_Fo);
	//	}


	//	for (size_t y = 0; y < m_weather.GetNbYears(); y++)
	//	{
	//		//double Litter = 0.5 * m_LAImax;
	//		//double LAI = m_LAI;//LAI is constant in this model, need to have a equation during the year


	//		CStatistic stat = GetSoilTemperature(m_weather[y], m_z, Tsoil, A, m_LAI, m_Litter, m_F, m_Fo);

	//		m_output[y][O_SOIL_TEMPERATURE_MIN] = stat[LOWEST];
	//		m_output[y][O_SOIL_TEMPERATURE_MEAN] = stat[MEAN];
	//		m_output[y][O_SOIL_TEMPERATURE_MAX] = stat[HIGHEST];


	//	}

	//	return msg;
	//}

	//ERMsg CSoilTemperatureModel::OnExecuteMonthly()
	//{
	//	ERMsg msg;


	//	CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::MONTHLY));
	//	m_output.Init(p, NB_OUTPUTS, -9999);

	//	double Tsoil = 0;

	//	//init soil temperature with December
	//	//double Litter = 0.5 * m_LAImax;
	//	for (size_t d = 0; d < m_weather[size_t(0)][DECEMBER].size(); d++)
	//	{
	//		GetSoilTemperature(m_weather[size_t(0)], m_z, Tsoil, m_LAI, m_Litter, m_F, m_Fo);
	//	}


	//	for (size_t y = 0; y < m_weather.GetNbYears(); y++)
	//	{
	//		//double Litter = 0.5 * m_LAImax;
	//		//double LAI = m_LAI;//LAI is constant in this model, need to have a equation during the year

	//		for (size_t m = 0; m < 12; m++)
	//		{
	//			CStatistic stat = GetSoilTemperature(m_weather[y][m], m_z, Tsoil, m_LAI, m_Litter, m_F, m_Fo);

	//			m_output[y * 12 + m][O_SOIL_TEMPERATURE_MIN] = stat[LOWEST];
	//			m_output[y * 12 + m][O_SOIL_TEMPERATURE_MEAN] = stat[MEAN];
	//			m_output[y * 12 + m][O_SOIL_TEMPERATURE_MAX] = stat[HIGHEST];

	//		}
	//	}


	//	return msg;
	//}


	ERMsg CSoilTemperatureModel::OnExecuteDaily()
	{
		ERMsg msg;

	//	if (!m_weather.IsHourly())
		//	m_weather.ComputeHourlyVariables();

		//m_weather.ComputeHourlyVariables();
		//size_t m_forestCoverType = BLACK_SPRUCE_FENS;
		//double m_quantile = 0.5;

		//m_LAImax = 0;
		//for(size_t doy=0; doy<365; doy++)
		//	m_LAImax = max(m_LAImax, CLeafAreaIndex::ComputeLAI(doy, m_forestCoverType, m_quantile));


		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));
		m_output.Init(p, NB_OUTPUTS, -9999);

		//std::valarray<double> A(0.0,11);

		double Tsoil = 0;
		//init soil temperature with December
		//double Litter = 0.5 * m_LAImax;
		//for (size_t d = 0; d < 11; d++)
//			A[d] = GetTairAtSurface(m_weather[size_t(0)][DECEMBER][d], NOT_INIT, m_Fo);
			

		for (size_t d = 0; d < m_weather[size_t(0)][DECEMBER].size(); d++)
		{
			//A = A.shift(1);
			//A[10] = GetTairAtSurface(m_weather[size_t(0)][DECEMBER][d], NOT_INIT, m_Fo);
			GetSoilTemperature(m_weather[size_t(0)][DECEMBER][d], m_z, Tsoil, m_LAI, m_Litter);
		}


		for (size_t y = 0; y < m_weather.size(); y++)
		{
			//Ground litter as LAI equivalent
			//double Litter = 0.5 * m_LAImax;
			//double LAI = m_LAI;//LAI is constant in this model, need to have a equation during the year

			for (size_t m = 0; m < m_weather[y].size(); m++)
			{
				for (size_t d = 0; d < m_weather[y][m].size(); d++)
				{
					CTRef ref = m_weather[y][m][d].GetTRef();
					//A = A.shift(1);
					//A[10] = GetTairAtSurface(m_weather[y][m][d], NOT_INIT, m_Fo);


					CStatistic stat = GetSoilTemperature(m_weather[y][m][d], m_z, Tsoil, m_LAI, m_Litter);

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


		//CSun sun(m_weather.m_lat, m_weather.m_lon);

		double Tsoil = 0;
		//init soil temperature with December
		//double Litter = 0.5 * m_LAImax;

		//std::valarray<double> A(0.0, 11);
		//for (size_t d = 0; d < 11; d++)
			//A[d] = GetTairAtSurface(m_weather[size_t(0)][DECEMBER][d], NOT_INIT, m_Fo);

		for (size_t d = 0; d < m_weather[size_t(0)][DECEMBER].size(); d++)
		{
			//A = A.shift(1);
			//A[10] = GetTairAtSurface(m_weather[size_t(0)][DECEMBER][d], NOT_INIT, m_F, m_Fo);
			GetSoilTemperature(m_weather[size_t(0)][DECEMBER][d], m_z, Tsoil, m_LAI, m_Litter);
		}

		for (size_t y = 0; y < m_weather.size(); y++)
		{
			for (size_t m = 0; m < m_weather[y].size(); m++)
			{
				for (size_t d = 0; d < m_weather[y][m].size(); d++)
				{
					//A = A.shift(1);
					//A[10] = GetTairAtSurface(m_weather[y][m][d], NOT_INIT, m_Fo);

					for (size_t h = 0; h < m_weather[y][m][d].size(); h++)
					{
						CTRef ref = m_weather[y][m][d].GetTRef();
						//double LAI = CLeafAreaIndex::ComputeLAI(ref.GetJDay(), m_forestCoverType, m_quantile);

						CStatistic stat = GetSoilTemperature(m_weather[y][m][d], m_z, Tsoil, m_LAI, m_Litter);

						m_output[ref][O_SOIL_TEMPERATURE_MIN] = stat[LOWEST];
						m_output[ref][O_SOIL_TEMPERATURE_MEAN] = stat[MEAN];
						m_output[ref][O_SOIL_TEMPERATURE_MAX] = stat[HIGHEST];
					}
				}
			}
		}


		return msg;
	}



	/*CStatistic CSoilTemperatureModel::GetSoilTemperature(const CWeatherYear& weather, double z, double& Tsoil, const std::valarray<double>& A, double LAI, double& Litter, double F, double Fo)
	{
		CStatistic stat;
		for (size_t m = 0; m < weather.size(); m++)
			stat += GetSoilTemperature(weather[m], z, Tsoil, A, LAI, Litter, F, Fo);

		return stat[MEAN];
	}

	CStatistic CSoilTemperatureModel::GetSoilTemperature(const CWeatherMonth& weather, double z, double& Tsoil, const std::valarray<double>& A, double LAI, double& Litter, double F, double Fo)
	{
		CStatistic stat;
		for (size_t d = 0; d < weather.size(); d++)
			stat += GetSoilTemperature(weather[d], z, Tsoil, A, LAI, Litter, F, Fo);

		return stat;
	}*/

	double CSoilTemperatureModel::GetTairAtSurface(const CWeatherDay& weather, size_t h, double Fo)
	{
		
		static const double Fs = 0.71;//[cm¯¹], 
		double Tair = -999;
		if (h == NOT_INIT)
		{
			double overheat = 0;
			if (weather[H_TAIR][MEAN] > 10)
				overheat = weather[H_TRNG2][MEAN] * Fo;

			Tair = weather[H_TAIR][MEAN] + overheat;
			Tair *= exp(-Fs * weather[H_SNDH][MEAN]);//take effect of snow
		}
		else
		{
			double overheat = 0;
			if (weather[h][H_TAIR] > 10)
				overheat = weather[H_TRNG2][MEAN] * Fo;

			Tair = weather[h][H_TAIR] + overheat;
			Tair *= exp(-Fs * weather[H_SNDH][MEAN]);//take effect of snow
		}

		return Tair;
	}


	CStatistic CSoilTemperatureModel::GetSoilTemperature(const CWeatherDay& weather, double z, double& Tsoil, double LAI, double& Litter)
	{
		_ASSERTE(!isnan(Tsoil));


		CStatistic stat;

		//double mean11day = A.sum() / A.size();


		//	if (Tair > 0)
			//	Litter -= 0.01 * Litter;
		if (weather.IsHourly())
		{
			//compute mean of the last 11 days
			for (size_t h = 0; h < weather.size(); h++)
			{
				CTRef ref = weather[h].GetTRef();

				double Tair = GetTairAtSurface(weather, h, m_Fo);
				

				double delta = GetDeltaSoilTemperature(z, (Tair > Tsoil) ? LAI : 0, Litter, m_F) / 24.0;
				Tsoil += (Tair - Tsoil) * delta;
				stat += Tsoil;
			}
		}
		else
		{
		//	//double Cs = 1.3E6; //[J/(m³°C)]. Between 1.0E6 and 1.3E6
		//	//double Kt = 0.61;//[W/(m°C)]. Between 0.4 and 0.8
		//	//double Cice = 8.95E6; //[J/(m³°C)], between 4E6 and 15E6
		//	//double Fs = 7.1; //1/m
		//	double dt = 24*60*60;
		//	double Ds = weather[H_SNDH][MEAN] / 100;//cm --> m
		//	double Zs = z/100;// / 100;//cm --> m

		//	double Ca = (m_Cs + m_Cice)*1E6;
		//	double Tair = weather[H_TAIR][MEAN];
		//	double DRz = F*dt * m_Kt / (Ca * Square(2.0 * Zs));
		//	_ASSERTE(!isnan(DRz));
		//	


		//	double Tstar = Tsoil + DRz * (Tair - Tsoil);
		//	Tsoil = Tstar * exp(-m_Fs*Ds);
		//	_ASSERTE(!isnan(Tsoil));

		//	Tsoil = max(-998.0, min(999.0, Tsoil));


			double Tair = GetTairAtSurface(weather, NOT_INIT, m_Fo);
			double delta = GetDeltaSoilTemperature(z, (Tair > Tsoil) ? LAI : 0, Litter, m_F);

			Tsoil += (Tair - Tsoil) * delta;
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
	double CSoilTemperatureModel::GetDeltaSoilTemperature(double z, double LAI, double Litter, double F)
	{
		CStatistic stat;

		const double Ks = 0.005;//soil thermal diffusivity [cm²/s]. possible range  [0.001, 0.01]
		const double p = 24.0 * 60.0 * 60.0; //[s]
		const double k = 0.45;//extinction coefficient

		double DRz = exp(-F * z * sqrt(PI / (Ks * p)));//range from [0.93, 0.97],

		return DRz * exp(-k * (LAI + Litter));
	}

	enum TInputEmergence { I_TAIR, I_TSOIL5, I_TSOIL10, I_TSOIL20, I_TSOIL50, I_TSOIL100, NB_INPUTS };
	void CSoilTemperatureModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		ASSERT(data.size() == NB_INPUTS + 2);

		CSAResult obs;

		obs.m_ref.FromFormatedString(data[1]);
		obs.m_obs.resize(NB_INPUTS);
		for (size_t s = 0; s < NB_INPUTS; s++)
		{
			obs.m_obs[s] = (data[s + 2] != "NA") ? stod(data[s + 2]) : -999.0;
			//if (s == 1 && obs.m_obs[1] > -999)
				//obs.m_obs[s] /= 100;

			//if (obs.m_obs[s] >= 0)
				//m_DOY[s] += obs.m_ref.GetJDay();
		}


		//if(!m_SAResult.empty())
		//obs.m_obs[I_CUMUL_EGG_HATCH] = stod(data[2]);//EggHatch
		//obs.m_obs[I_CUMUL_ADULT] = stod(data[7]);//Adult emergence

		//m_years.insert(obs.m_ref.GetYear());


		m_SAResult.push_back(obs);

	}

	bool CSoilTemperatureModel::GetFValueDaily(CStatisticXY& stat)
	{
		//return GetFValueDailyAdultCatch(stat);

		//OnExecuteDaily();

		size_t v = NOT_INIT;
		if (fabs(m_z - 5) < 0.1)
			v = I_TSOIL5- I_TSOIL5;
		if (fabs(m_z - 10) < 0.1)
			v = I_TSOIL10 - I_TSOIL5;
		if (fabs(m_z - 20) < 0.1)
			v = I_TSOIL20 - I_TSOIL5;
		if (fabs(m_z - 50) < 0.1)
			v = I_TSOIL50 - I_TSOIL5;
		if (fabs(m_z - 100) < 0.1)
			v = I_TSOIL100 - I_TSOIL5;
			ASSERT(v != NOT_INIT);
		

		//static const double SOIL_DEPTH[5] = { 5,10,20,50,100 };
		//for (size_t v = 0; v < 5; v++)
		{
			//m_z = SOIL_DEPTH[v];
			OnExecuteDaily();
			for (size_t i = 0; i < m_SAResult.size(); i++)
			{

				if (m_SAResult[i].m_obs[I_TSOIL5+v] > -999)
				{

					double obs = m_SAResult[i].m_obs[I_TSOIL5+v];
					double sim = m_output[m_SAResult[i].m_ref][O_SOIL_TEMPERATURE_MEAN];
					stat.Add(obs, sim);

				}
			}//for all results
		}
		return true;
	}






}