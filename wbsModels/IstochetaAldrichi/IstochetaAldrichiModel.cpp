//***********************************************************
// 21/05/2025	1.0.0	Rémi Saint-Amant   Creation 
//***********************************************************
#include "IstochetaAldrichiModel.h"
#include "IstochetaAldrichiEquations.h"
#include "ModelBase/EntryPoint.h"
#include "Basic/DegreeDays.h"
#include "ModelBase/SimulatedAnnealingVector.h"
#include <boost/math/distributions/logistic.hpp>


using namespace WBSF::HOURLY_DATA;
using namespace WBSF::IAM;
using namespace std;


namespace WBSF
{
	double GetDeltaSoilTemperature(double z, double F, double litter)
	{
		//LAI ant litter is equal to 0

		static const double Ks = 0.005;//soil thermal diffusivity [cm²/s]. possible range  [0.001, 0.01]
		static const double p = 24.0 * 60.0 * 60.0; //[s]
		static const double k = 0.45;//extinction coefficient

		return exp(-F * z * sqrt(PI / (Ks * p))) * exp(-k * litter);//range from [0.93, 0.97]
	}

	double GetTairAtSurface(const CWeatherDay& weather, size_t h, double Fo)
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

	CModelStatVector GetSoilTemperature(const CWeatherYears& weather, double z, double litter)
	{
		CTPeriod p = weather.GetEntireTPeriod();
		CModelStatVector output(p, 1, 0);


		//correction factor
		double F = max(0.0, min(8.0, 11.8 * pow(z, -0.740)));

		//overheating factor in function of depth
		double Fo = max(0.0, min(0.3, 0.212 * pow(z, -0.224)));
		double delta = GetDeltaSoilTemperature(z, F, litter);//daily change


		double Tsoil = 0;
		//pre init simulation with December of the first year
		for (size_t d = 0; d < weather[size_t(0)][DECEMBER].size(); d++)
		{
			double Tair = GetTairAtSurface(weather[size_t(0)][DECEMBER][d], NOT_INIT, Fo);
			Tsoil += (Tair - Tsoil) * delta;
		}

		for (size_t y = 0; y < weather.size(); y++)
		{
			for (size_t m = 0; m < weather[y].size(); m++)
			{
				for (size_t d = 0; d < weather[y][m].size(); d++)
				{
					for (size_t h = 0; h < weather[y][m][d].size(); h++)
					{
						//weather.GetHour();

						CTRef ref = weather[y][m][d][h].GetTRef();// .as(CTM(CTM::DAILY));

						double Tair = GetTairAtSurface(weather[y][m][d], h, Fo);
						Tsoil += (Tair - Tsoil) * delta / 24.0;


						output[ref][0] = Tsoil;
					}
				}
			}
		}

		return output;
	}





	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CIstochetaAldrichiModel::CreateObject);

	CIstochetaAldrichiModel::CIstochetaAldrichiModel()
	{
		//NB_INPUT_PARAMETER is used to determine if the dll
		//uses the same number of parameters than the model interface
		NB_INPUT_PARAMETER = -1;
		VERSION = "1.0.0 (2026)";


		m_bApplyAttrition = false;
		m_bCumul = false;

		//set with default values
		m_adult_emerg = CIstochetaAldrichiEquations::ADULT_EMERG;
		m_pupa_param = CIstochetaAldrichiEquations::PUPA_PARAM;
		m_C_param = CIstochetaAldrichiEquations::C_PARAM;

	}

	CIstochetaAldrichiModel::~CIstochetaAldrichiModel()
	{
	}


	//this method is call to load your parameter in your variable
	ERMsg CIstochetaAldrichiModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		size_t c = 0;

		m_bApplyAttrition = parameters[c++].GetBool();
		m_bCumul = parameters[c++].GetBool();

		if (parameters.size() == 2 + NB_EMERGENCE_PARAMS + NB_PUPA_PARAMS + NB_C_PARAMS)
		{
			for (size_t p = 0; p < NB_EMERGENCE_PARAMS; p++)
				m_adult_emerg[p] = parameters[c++].GetFloat();

			for (size_t p = 0; p < NB_PUPA_PARAMS; p++)
				m_pupa_param[p] = parameters[c++].GetFloat();

			for (size_t p = 0; p < NB_C_PARAMS; p++)
				m_C_param[p] = parameters[c++].GetFloat();

			
		}


		return msg;
	}





	//This method is called to compute the solution
	ERMsg CIstochetaAldrichiModel::OnExecuteDaily()
	{
		ERMsg msg;


		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();

		//compute soil temperature for all years
		CModelStatVector Tsoil = GetSoilTemperature(m_weather, 10, 0.0);


		//This is where the model is actually executed
		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));
		m_output.Init(p, NB_STATS, 0);

		//For all years
		for (size_t y = 0; y < m_weather.size(); y++)
		{
			ExecuteDaily(m_weather[y].GetTRef().GetYear(), m_weather, Tsoil, m_output);
		}

		return msg;
	}

	void CIstochetaAldrichiModel::ExecuteDaily(int year, const CWeatherYears& weather, const CModelStatVector& Tsoil, CModelStatVector& output, bool in_calibration)
	{

		


		//Create stand and init it
		CIAMStand stand(this, Tsoil);
		stand.m_bApplyAttrition = m_bApplyAttrition;
		stand.m_equations.m_adult_emerg = m_adult_emerg;
		stand.m_equations.m_pupa_param = m_pupa_param;
		stand.m_equations.m_C_param = m_C_param;
		stand.m_in_calibration = in_calibration;
		stand.init(year, weather);


		//Create host
		CIAMHostPtr pHost(new CIAMHost(&stand));

		//pHost->m_nbMinObjects = 10;
		pHost->m_nbMinObjects = 10;
		pHost->m_nbMaxObjects = 1000;

		pHost->Initialize<CIstochetaAldrichi>(CInitialPopulation(CTRef(year, JANUARY, DAY_01), 0, 400, 100, PUPAE));//+ m_C_param[0]

		//add host to stand			
		stand.m_host.push_front(pHost);

		CTPeriod p = weather[year].GetEntireTPeriod(CTM(CTM::DAILY));

		if (output.empty())
			output.Init(p, NB_STATS, 0);



		for (CTRef d = p.Begin(); d <= p.End(); d++)
		{
			stand.Live(weather.GetDay(d));
			if (output.IsInside(d))
				stand.GetStat(d, output[d]);

			stand.AdjustPopulation();
			HxGridTestConnection();
		}



		if (m_bCumul)
		{


			//cumulative result
			for (size_t ss = 0; ss < NB_CUMUL_STATS; ss++)
			{
				size_t s = CUM_STAT[ss];


				CStatistic stat = output.GetStat(s, p);
				if (stat.IsInit() && stat[SUM] > 0)
				{
					output[0][s] = output[0][s] * 100 / stat[SUM];//when first day is not 0
					for (CTRef d = p.Begin() + 1; d <= p.End(); d++)
					{
						output[d][s] = output[d - 1][s] + output[d][s] * 100 / stat[SUM];
						_ASSERTE(!_isnan(output[d][s]));
					}
				}
			}

		}
	}

	

	
	enum TInput { I_SITE, I_YEAR, I_DOY, I_N, NB_INPUTS };
	void CIstochetaAldrichiModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		ASSERT(data.size() == NB_INPUTS);

		CSAResult obs;

	
		int year = stoi(data[I_YEAR]);
		int DOY = stoi(data[I_DOY]);
		int N = stoi(data[I_N]);

		ASSERT(year >= 2005 && year <= 2025);
		ASSERT(DOY >= 1 && DOY <= 366);
		ASSERT(N >= 1);


		obs.m_ref = CJDayRef(year,DOY-1);
		obs.m_obs.resize(1);
		obs.m_obs[0] = N;
		

		
		m_SAResult.push_back(obs);
		m_pupae_DOY += DOY-1;
		m_years.insert(year);
		
	}

	bool CIstochetaAldrichiModel::IsParamValid()const
	{
		bool bValid = true;


		if (m_adult_emerg[Τᴴ¹] >= m_adult_emerg[Τᴴ²])
			bValid = false;

		return bValid;
	}

	bool CIstochetaAldrichiModel::CalibratePupa(CStatisticXY& stat)
	{

		if (!m_SAResult.empty())
		{
			if (!m_weather.IsHourly())
				m_weather.ComputeHourlyVariables();

			//compute soil temperature for all years
			CModelStatVector Tsoil = GetSoilTemperature(m_weather, 10, 0.0);

			m_bCumul = true;//SA always cumulative
			//Always used the same seed for calibration
			m_randomGenerator.Randomize(CRandomGenerator::FIXE_SEED);



			for (size_t y = 0; y < m_weather.GetNbYears(); y++)
			{
				int year = m_weather[y].GetTRef().GetYear();
				if (m_years.find(year)!= m_years.end())//does this year is present in the data for this locations
				{
					CModelStatVector output;
					CTPeriod p = m_weather[y].GetEntireTPeriod(CTM(CTM::DAILY));

					output.Init(p, NB_STATS, 0);
					ExecuteDaily(m_weather[y].GetTRef().GetYear(), m_weather, Tsoil, output, true);


					for (size_t i = 0; i < m_SAResult.size(); i++)
					{
						if (output.IsInside(m_SAResult[i].m_ref))
						{
							//CTRef median_emergence = output.GetFirstTRef(S_EMERGENCE, ">", m_adult_emerg[Τᴴ²], 0);
							CTRef median_emergence = output.GetFirstTRef(S_ADULT, ">", m_adult_emerg[Τᴴ²], 0);
							assert(median_emergence.IsInit());
							double obs_y = m_SAResult[i].m_ref.GetJDay();
							double sim_y = median_emergence.GetJDay();

							//if (obs_y > -999)
							//{
							//for (size_t ii = 0; ii < m_SAResult[i].m_obs[0]; ii++)
								stat.Add(obs_y, sim_y);
							//}
						}
					}//for all results
				}
			}
		}

		return true;
	}


	bool CIstochetaAldrichiModel::GetFValueDaily(CStatisticXY& stat)
	{
		if (!IsParamValid())
			return false;

		return CalibratePupa(stat);
	}


}
