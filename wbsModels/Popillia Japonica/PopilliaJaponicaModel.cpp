//***********************************************************
// 20/09/2023   Rémi Saint-Amant    Creation
//***********************************************************
#include "PopilliaJaponicaModel.h"
#include "ModelBase/EntryPoint.h"


using namespace WBSF::HOURLY_DATA;
using namespace WBSF::PJN;
using namespace std;


namespace WBSF
{
	enum TOutput { NB_OUTPUTS = NB_STATS };

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CPopilliaJaponicaModel::CreateObject);

	CPopilliaJaponicaModel::CPopilliaJaponicaModel()
	{
		//NB_INPUT_PARAMETER is used to determine if the dll
		//uses the same number of parameters than the model interface
		NB_INPUT_PARAMETER = -1;
		VERSION = "1.0.0 (2023)";


		m_bApplyAttrition = true;
		m_bApplyFrost = true;
		m_bCumul = true;

		//Correction factor: fitted curve don't estimate fields observation accurately
		m_psy = CPopilliaJaponicaEquations::PSY;
		m_EOD = CPopilliaJaponicaEquations::EOD_NA;
		m_ACC = CPopilliaJaponicaEquations::ACC;
		m_other = CPopilliaJaponicaEquations::OTHER_NA;
	}

	CPopilliaJaponicaModel::~CPopilliaJaponicaModel()
	{
	}


	//this method is call to load your parameter in your variable
	ERMsg CPopilliaJaponicaModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		size_t c = 0;
		//m_bApplyAttrition = parameters[c++].GetBool();
		//m_bApplyFrost = parameters[c++].GetBool();
		m_bCumul = parameters[c++].GetBool();


		if (parameters.size() == NB_CDD_PARAMS + 7 + NB_OTHER_PARAMS + NB_PSY + 1 + 1)
		{
			m_EOD.resize(7);
			for (size_t p = 0; p < 7; p++)
				m_EOD[p] = parameters[c++].GetFloat();


			//parameters[c++].GetFloat();


			for (size_t p = 0; p < NB_CDD_PARAMS; p++)
				m_ACC[p] = parameters[c++].GetFloat();


			for (size_t p = 0; p < NB_OTHER_PARAMS; p++)
				m_other[p] = parameters[c++].GetFloat();


			m_calibration_type = parameters[c++].GetSizeT();

			for (size_t p = 0; p < NB_PSY; p++)
				m_psy[p] = parameters[c++].GetFloat();


			//if (size_t(m_EOD[CDD_DIST]) > CModelDistribution::NB_DISTRIBUTIONS)
				//msg.ajoute("Invalid EOD distribution type");

			if (size_t(m_ACC[CDD_DIST]) > CModelDistribution::NB_DISTRIBUTIONS)
				msg.ajoute("Invalid ACC distribution type");
		}
		else
		{
			enum TLocation{ NORTH_AMERICA, EUROPE, NB_LOCATIONS };
			size_t location = parameters[c++].GetInt();
			ASSERT(location < NB_LOCATIONS);
			
			m_EOD = location == NORTH_AMERICA?CPopilliaJaponicaEquations::EOD_NA: CPopilliaJaponicaEquations::EOD_EU;
			m_other = location == NORTH_AMERICA ? CPopilliaJaponicaEquations::OTHER_NA: CPopilliaJaponicaEquations::OTHER_EU;
		}


		return msg;
	}





	double GetAETFactor(double SMI, double SMIcrit)
	{
		double AETFactor = 0;

		//if (m_model == BILINEAR)
		AETFactor = min(1.0, SMI / SMIcrit);
		//		else if (m_model == QUADRATIC_LINEAR)
			//		AETFactor = min(1.0, 2 * SMI / m_SMIcrit - Square(SMI / m_SMIcrit));

		return AETFactor;
	}

	CModelStatVector GetPSMI(const CWeatherYears& weather, double SMIcrit, double SMImax)
	{
		CModelStatVector output;

		//Coefficient for calculating PET (dimensionless). Set to 2.77 for daily calculations.
		static const double PET_COEF = 2.77;
		double SMI = SMIcrit;						//Initial soil moisture (mm) ???
		const double elev = weather.GetLocation().m_elev;	//Elevation (m)

		output.Init(weather.GetEntireTPeriod(CTM::DAILY), 1);

		for (size_t y = 0; y < weather.GetNbYears(); y++)
		{
			for (size_t m = 0; m < 12; m++)
			{
				for (size_t d = 0; d < weather[y][m].GetNbDays(); d++)
				{
					CTRef Tref = weather[y][m][d].GetTRef();

					//Input:
					double Tmin = weather[y][m][d][H_TMIN][MEAN]; //Minimum daily temperature (°C)
					double Tmax = weather[y][m][d][H_TMAX][MEAN]; //Maximum daily temperature (°C)
					double ppt = weather[y][m][d][H_PRCP][SUM]; //Precipitation (mm) 

					//Calculate:
					//Saturation vapor pressure for Tmax
					double svp_Tmax = 0.61078 * exp(17.269 * Tmin / (237.3 + Tmin));

					//Saturation vapor pressure for Tmin
					double svp_Tmin = 0.61078 * exp(17.269 * Tmax / (237.3 + Tmax));

					//Saturation vapor pressure for Tdew
					double svp_Tdew = 0.61078 * exp(17.269 * (Tmin - 2.5) / (237.3 + (Tmin - 2.5)));

					//Daily vapor pressure deficit
					double VPD = 0.5 * (svp_Tmax + svp_Tmin) - svp_Tdew;

					//Daily mean temperature
					double Tmean = 0.5 * (Tmin + Tmax);

					//Cold temperature reduction factor
					double K_trf = max(min((Tmean + 5) / 15, 1.0), 0.0);

					//Daily potential evapotranspiration (mm d-1)
					double PET = PET_COEF * VPD * K_trf * exp(elev / 9300);

					//Daily actual evapotranspiration (mm d-1)
					double AET = PET * GetAETFactor(SMI, SMIcrit);

					//Daily water runoff (mm d-1) 
					double Runoff = max(0.0, SMI + ppt - AET - SMImax);

					//Soil moisture at end of day d (mm)
					SMI = SMI + ppt - AET - Runoff;

					//Soil Moisture Index (%)
					double PSMI = 100 * (SMI / SMImax);

					//sout outputs
					//output[Tref][O_TMAX] = Tmax;
					//output[Tref][O_TMIN] = Tmin;
					//output[Tref][O_PPT] = ppt;
					//output[Tref][O_SVP_TMAX] = svp_Tmax;
					//output[Tref][O_SVP_TMIN] = svp_Tmin;
					//output[Tref][O_SVP_TDEW] = svp_Tdew;
					//output[Tref][O_VPD] = VPD;
					//output[Tref][O_TMEAN] = Tmean;
					//output[Tref][O_K_TRF] = K_trf;
					//output[Tref][O_PET] = PET;
					//output[Tref][O_AET] = AET;
					//output[Tref][O_RUNOFF] = Runoff;
					//output[Tref][O_SMI] = SMI;
					output[Tref][0] = PSMI;
				}
			}
		}

		return output;
	}

	//Tair: mean air temperature of actual day [°C]
		//Tsoil: soil temperature of last day [°C]
		//LAI: Leaf area index [m²/m²]
		//Litter: Ground litter as LAI equivalent [m²/m²]
		//z:	soil depth [m]
		// p: time lapse [second]
		//return soil temperature [°C]
		//Note that the initial value of soil temperature is calculated by multiplying DRz by air
		//temperature of the first Julian day and LAI equivalent of above ground litter is assumed as a half of the maximum LAI.
		//the first time, Tsoil must be 0.
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
		//pre init simulation with January of the first year
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

	//This method is called to compute the solution
	ERMsg CPopilliaJaponicaModel::OnExecuteDaily()
	{
		ERMsg msg;

		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();

		CModelStatVector PSMI(m_weather.GetEntireTPeriod(CTM::DAILY), 1);// = GetPSMI(m_weather, 200, 400);
		CModelStatVector Tsoil = GetSoilTemperature(m_weather, 10, m_other[LITTER]);

//copy soil temperature to weather
		/*for (size_t y = 0; y < m_weather.size(); y++) 
		{
			for (size_t m = 0; m < m_weather[y].size(); m++)
			{
				for (size_t d = 0; d < m_weather[y][m].size(); d++)
				{
					for (size_t h = 0; h < m_weather[y][m][d].size(); h++)
					{
						m_weather[y][m][d][h][H_TMIN] = Tsoil[m_weather[y][m][d][h].GetTRef()][0];
						m_weather[y][m][d][h][H_TAIR] = Tsoil[m_weather[y][m][d][h].GetTRef()][0];
						m_weather[y][m][d][h][H_TMAX] = Tsoil[m_weather[y][m][d][h].GetTRef()][0];
					}
				}
			}
		}*/

		//This is where the model is actually executed
		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));
		m_output.Init(p, NB_OUTPUTS, 0);

		//we simulate 2 years at a time. 
		//we also manager the possibility to have only one year
		for (size_t y = 0; y < m_weather.size(); y++)
		{
			ExecuteDaily(m_weather[y], Tsoil, PSMI, m_output);
		}

		return msg;
	}


	void CPopilliaJaponicaModel::ExecuteDaily(const CWeatherYear& weather, const CModelStatVector& Tsoil, const CModelStatVector& PSMI, CModelStatVector& output)
	{

		CTPeriod p = weather.GetEntireTPeriod(CTM(CTM::DAILY));

		if (output.empty())
			output.Init(p, NB_OUTPUTS, 0);



		//Create stand
		CPJNStand stand(this, Tsoil, PSMI);
		stand.InitStand(m_EOD, m_ACC, m_other, weather);

		for (size_t p = 0; p < NB_PSY; p++)
			stand.m_equations.m_psy[p] = m_psy[p];

		//stand.m_bApplyAttrition = m_bApplyAttrition;
		//stand.m_bApplyFrost = m_bApplyFrost;
		//stand.m_psy_factor= m_param[1];

		//Create host
		CPJNHostPtr pHost(new CPJNHost(&stand));

		pHost->Initialize<CPopilliaJaponica>(CInitialPopulation(p.Begin(), 0, 400, 100, L3, RANDOM_SEX, true));

		//add host to stand			
		stand.m_host.push_front(pHost);




		for (CTRef d = p.Begin(); d <= p.End(); d++)
		{
			stand.Live(weather.GetDay(d));
			//if (output.IsInside(d))
			stand.GetStat(d, output[d]);
			output[d][S_TAIR] = weather.GetDay(d)[H_TNTX][MEAN];
			output[d][S_SNDH] = weather.GetDay(d)[H_SNDH][MEAN];
			output[d][S_PSMI] = PSMI[d][0];

			stand.AdjustPopulation();
			HxGridTestConnection();
		}


		if (m_bCumul)
		{

			for (size_t s = S_EGG; s <= S_ADULT_EMERGENCE; s++)
			{
				//size_t ss = (s == 0) ? S_EGG_HATCH : S_EGG + s;
				if (s != S_L3)
				{
					CStatistic stat = output.GetStat(s, p);
					if (stat.IsInit() && stat[SUM] > 0)
					{
						double S = stat[(s != S_DEAD_ADULT) ? SUM : HIGHEST];
						output[p.Begin()][s] = 100 * output[p.Begin()][s] / S;
						for (CTRef TRef = p.Begin() + 1; TRef <= p.End(); TRef++)
							output[TRef][s] = ((s != S_DEAD_ADULT) ? output[TRef - 1][s] : 0) + 100 * output[TRef][s] / S;

						for (CTRef TRef = p.Begin() + 1; TRef <= p.End(); TRef++)
						{
							if (output[TRef][s] < 0.1)
								output[TRef][s] = 0.0;
							if (output[TRef][s] > 99.9)
								output[TRef][s] = 100.0;
						}
					}
				}
			}

			//for (size_t s = 0; s < DEAD_ADULT; s++)
				//output = ;
		}

		//stat = output.GetStat(S_ADULT, p);
		//if (stat.IsInit() && stat[SUM] > 0)
		//{
		//	output[p.Begin()][O_CUMUL_ADULT] = 0;
		//	for (CTRef TRef = p.Begin() + 1; TRef <= p.End(); TRef++)
		//		output[TRef][O_CUMUL_ADULT] = output[TRef - 1][O_CUMUL_ADULT] + 100 * output[TRef][S_ADULT] / stat[SUM];
		//}
	}

	enum TInputCatch { I_SOURCE, I_SITE, I_DATE, I_ADULT_EMERGENCE, I_ADULT_EMERGENCE_CUMUL, I_ADULT_CATCH, I_ADULT_CATCH_CUMUL, NB_INPUTS };
	void CPopilliaJaponicaModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		ASSERT(data.size() == NB_INPUTS);

		CSAResult obs;

		obs.m_ref.FromFormatedString(data[I_DATE]);
		obs.m_obs.resize(NB_INPUTS);
		for (size_t s = I_ADULT_EMERGENCE; s < NB_INPUTS; s++)
		{
			obs.m_obs[s] = (data[s] != "NA") ? stod(data[s]) : -999.0;

			if (obs.m_obs[s] >= 0)
				m_DOY[s] += obs.m_ref.GetJDay();
		}

		m_years.insert(obs.m_ref.GetYear());


		m_SAResult.push_back(obs);

	}

	void CPopilliaJaponicaModel::GetEggHacth(const array<double, NB_CDD_PARAMS >& P, const CWeatherYear& weather, CModelStatVector& output)const
	{
		//CModelDistribution dist(m_EOD);

	}

	bool CPopilliaJaponicaModel::GetFValueDailyEggHacth(CStatisticXY& stat)
	{
		if (m_EOD[CDD_Τᴴ¹] > m_EOD[CDD_Τᴴ²])
			return false;

		/*double Nd = 0;
		for (size_t i = 0; i < m_SAResult.size(); i++)
		{
			if (m_SAResult[i].m_obs[I_EGG_HATCH] > -999)
				Nd += m_SAResult[i].m_obs[I_EGG_HATCH];
		}*/


		//for (auto it = m_years.begin(); it != m_years.end(); it++)
		//{
		//	int year = *it;

		//	CModelStatVector cumcul_output;
		//	CModelDistribution::get_CDF(m_EOD, m_weather[year], cumcul_output);

		//	CTPeriod p = cumcul_output.GetTPeriod();

		//	for (size_t i = 0; i < m_SAResult.size(); i++)
		//	{
		//		if (m_SAResult[i].m_ref.GetYear() == year)
		//		{
		//			//flies catch 

		//			if (m_SAResult[i].m_obs[I_CUMUL_EGG_HATCH] > -999)
		//			{
		//				double obs = m_SAResult[i].m_obs[I_CUMUL_EGG_HATCH];
		//				double sim = cumcul_output[m_SAResult[i].m_ref][0];

		//				//for (size_t ii = 0; ii < log(Nd); ii++)
		//				stat.Add(obs, sim);
		//			}

		//		}
		//	}//for all results

		//}

		return true;

	}

	bool CPopilliaJaponicaModel::Overwintering(CStatisticXY& stat)
	{
		return true;
	}


	bool CPopilliaJaponicaModel::GetFValueDailyAdultCatch(CStatisticXY& stat)
	{
		if (m_ACC[CDD_Τᴴ¹] > m_ACC[CDD_Τᴴ²])
			return false;

		/*double Nd = 0;
		for (size_t i = 0; i < m_SAResult.size(); i++)
		{
			if (m_SAResult[i].m_obs[I_EGG_HATCH] > -999)
				Nd += m_SAResult[i].m_obs[I_EGG_HATCH];
		}*/


		for (auto it = m_years.begin(); it != m_years.end(); it++)
		{
			int year = *it;

			CModelStatVector cumcul_output;
			CModelDistribution::get_CDF(m_ACC, m_weather[year], cumcul_output);

			CTPeriod p = cumcul_output.GetTPeriod();

			for (size_t i = 0; i < m_SAResult.size(); i++)
			{
				if (m_SAResult[i].m_ref.GetYear() == year)
				{
					//Beattle catch 
					if (m_SAResult[i].m_obs[I_ADULT_CATCH_CUMUL] > -999)
					{
						double obs = m_SAResult[i].m_obs[I_ADULT_CATCH_CUMUL];
						double sim = 100 * cumcul_output[m_SAResult[i].m_ref][0];

						//for (size_t ii = 0; ii < log(Nd); ii++)
						stat.Add(obs, sim);
					}

				}
			}//for all results

		}

		return true;

	}

	bool CPopilliaJaponicaModel::GetFValueDailyAdultEmergence(CStatisticXY& stat)
	{
		if (m_ACC[CDD_Τᴴ¹] > m_ACC[CDD_Τᴴ²])
			return false;

		/*double Nd = 0;
		for (size_t i = 0; i < m_SAResult.size(); i++)
		{
			if (m_SAResult[i].m_obs[I_EGG_HATCH] > -999)
				Nd += m_SAResult[i].m_obs[I_EGG_HATCH];
		}*/


		for (auto it = m_years.begin(); it != m_years.end(); it++)
		{
			int year = *it;

			CModelStatVector cumcul_output;
			CModelDistribution::get_CDF(m_ACC, m_weather[year], cumcul_output);

			CTPeriod p = cumcul_output.GetTPeriod();

			for (size_t i = 0; i < m_SAResult.size(); i++)
			{
				if (m_SAResult[i].m_ref.GetYear() == year)
				{
					//Beattle catch 
					if (m_SAResult[i].m_obs[I_ADULT_EMERGENCE_CUMUL] > -999)
					{
						double obs = m_SAResult[i].m_obs[I_ADULT_EMERGENCE_CUMUL];
						double sim = 100 * cumcul_output[m_SAResult[i].m_ref][0];

						//for (size_t ii = 0; ii < log(Nd); ii++)
						stat.Add(obs, sim);
					}

				}
			}//for all results

		}

		return true;

	}



	bool CPopilliaJaponicaModel::GetFValueDaily(CStatisticXY& stat)
	{
		//		return GetFValueDailyAdultCatch(stat);

		if (m_EOD[2] > m_EOD[3])
			return false;
		if (m_EOD[3] > m_EOD[4])
			return false;

		if (m_ACC[CDD_Τᴴ¹] > m_ACC[CDD_Τᴴ²])
			return false;



		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();

		if (m_PSMI.empty())
			m_PSMI = GetPSMI(m_weather, 200, 400);
		
		if (m_Tsoil.empty())
			m_Tsoil = GetSoilTemperature(m_weather, 10, m_other[LITTER]);


		//replace air temperature by soil temperature

		/*for (size_t y = 0; y < m_weather.size(); y++)
		{
			for (size_t m = 0; m < m_weather[y].size(); m++)
			{
				for (size_t d = 0; d < m_weather[y][m].size(); d++)
				{
					for (size_t h = 0; h < m_weather[y][m][d].size(); h++)
					{
						m_weather[y][m][d][h][H_TMIN] = m_Tsoil[m_weather[y][m][d][h].GetTRef()][0];
						m_weather[y][m][d][h][H_TAIR] = m_Tsoil[m_weather[y][m][d][h].GetTRef()][0];
						m_weather[y][m][d][h][H_TMAX] = m_Tsoil[m_weather[y][m][d][h].GetTRef()][0];
					}
				}
			}
		}*/

		//m_weather.ResetStat();


		m_bCumul = true;
		for (auto it = m_years.begin(); it != m_years.end(); it++)
		{
			int year = *it;

			CModelStatVector output;
			ExecuteDaily(m_weather[year], m_Tsoil, m_PSMI, output);

			for (size_t i = 0; i < m_SAResult.size(); i++)
			{
				if (m_SAResult[i].m_ref.GetYear() == year)
				{
					static const size_t V_I[2] = { I_ADULT_EMERGENCE_CUMUL,I_ADULT_CATCH_CUMUL };
					static const size_t V_O[2] = { S_ADULT_EMERGENCE, S_ADULT };

					for (size_t v = 0; v < 2; v++)
					{
						if (v == C_EMERGENCE && m_calibration_type == C_CATCH)
							continue;
						if (v == C_CATCH && m_calibration_type == C_EMERGENCE)
							continue;

						if (m_SAResult[i].m_obs[V_I[v]] > -999)
						{
							double obs = m_SAResult[i].m_obs[V_I[v]];
							double sim = output[m_SAResult[i].m_ref][V_O[v]];

							//	for (size_t ii = 0; ii < log(Nd); ii++)
							stat.Add(obs, sim);


							if (obs >= 2.5 && obs <= 97.5)
							{
								double sim_DOY = GetSimDOY(V_O[v], m_SAResult[i].m_ref, obs, output);
								if (sim_DOY > -999)
								{
									double obs_DOYp = GetDOYPercent(V_I[v], m_SAResult[i].m_ref.GetJDay());
									double sim_DOYp = GetDOYPercent(V_I[v], sim_DOY);

									//for (size_t ii = 0; ii < log(3 * Ne); ii++)
									stat.Add(obs_DOYp, sim_DOYp);
								}
							}
						}
					}
				}
			}//for all results

		}

		return true;

	}


	double CPopilliaJaponicaModel::GetSimDOY(size_t s, CTRef TRefO, double obs, const CModelStatVector& output)
	{
		ASSERT(obs > -999);

		double DOY = -999;



		//if (obs > 0.01 && obs < 99.99)
		//if (obs >= 100)
			//obs = 99.99;//to avoid some problem of truncation

		long index = output.GetFirstIndex(s, ">=", obs, 1, CTPeriod(TRefO.GetYear(), JANUARY, DAY_01, TRefO.GetYear(), DECEMBER, DAY_31));
		if (index >= 1)
		{
			double obsX1 = output.GetFirstTRef().GetJDay() + index;
			double obsX2 = output.GetFirstTRef().GetJDay() + index + 1;

			double obsY1 = output[index][s];
			double obsY2 = output[index + 1][s];
			if (obsY2 != obsY1)
			{
				double slope = (obsX2 - obsX1) / (obsY2 - obsY1);
				double obsX = obsX1 + (obs - obsY1) * slope;
				ASSERT(!_isnan(obsX) && _finite(obsX));

				DOY = obsX;
			}

		}

		return DOY;
	}

	double  CPopilliaJaponicaModel::GetDOYPercent(size_t s, double DOY)const
	{
		//return value can be negative of greater than 100%
		return 100 * (DOY - m_DOY.at(s)[LOWEST]) / m_DOY.at(s)[RANGE];
	}

}
