//***********************************************************
// 07/07/2021	1.0.0	Rémi Saint-Amant   Creation
//***********************************************************
#include "SpottedLanternflyModel.h"
#include "ModelBase/EntryPoint.h"


using namespace WBSF::HOURLY_DATA;
using namespace WBSF::LDW;
using namespace std;


namespace WBSF
{
	enum TOutput { NB_OUTPUTS = NB_STATS };

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CSpottedLanternflyModel::CreateObject);

	CSpottedLanternflyModel::CSpottedLanternflyModel()
	{
		//NB_INPUT_PARAMETER is used to determine if the dll
		//uses the same number of parameters than the model interface
		NB_INPUT_PARAMETER = -1;
		VERSION = "1.0.0 (2022)";

		m_bApplyAttrition = true;

		//Correction factor: fitted curve don't estimate fields observation accurately
		m_psy = { {1,1.44,1.48,1.69,1.48,1,1} };
	}

	CSpottedLanternflyModel::~CSpottedLanternflyModel()
	{
	}


	//this method is call to load your parameter in your variable
	ERMsg CSpottedLanternflyModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		size_t c = 0;
		m_bApplyAttrition = parameters[c++].GetBool();
		m_bApplyFrost = parameters[c++].GetBool();
		
		if (parameters.size() == NB_CDD_PARAMS + NB_PSY + 2)
		{
			for (size_t p = 0; p < NB_CDD_PARAMS; p++)
				m_EOD[p] = parameters[c++].GetFloat();

			for (size_t p = 0; p < NB_PSY; p++)
				m_psy[p] = parameters[c++].GetFloat();
		}


		return msg;
	}





	//This method is called to compute the solution
	ERMsg CSpottedLanternflyModel::OnExecuteDaily()
	{
		ERMsg msg;

		//This is where the model is actually executed
		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));
		m_output.Init(p, NB_OUTPUTS, 0);

		//we simulate 2 years at a time. 
		//we also manager the possibility to have only one year
		for (size_t y = 0; y < m_weather.size(); y++)
		{
			ExecuteDaily(m_weather[y], m_output);
		}

		return msg;
	}

	void CSpottedLanternflyModel::ExecuteDaily(const CWeatherYear& weather, CModelStatVector& output)
	{
		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();


		CTPeriod p = weather.GetEntireTPeriod(CTM(CTM::DAILY));

		if (output.empty())
			output.Init(p, NB_OUTPUTS, 0);


		//Create stand
		CLDWStand stand(this);
		stand.InitStand(m_EOD, weather);

		for (size_t p = 0; p < NB_PSY; p++)
			stand.m_equations.m_psy[p] = m_psy[p];

		stand.m_bApplyAttrition = m_bApplyAttrition;
		stand.m_bApplyFrost = m_bApplyFrost;
		//stand.m_psy_factor= m_param[1];

		//Create host
		CLDWHostPtr pHost(new CLDWHost(&stand));

		pHost->Initialize<CSpottedLanternfly>(CInitialPopulation(p.Begin(), 0, 400, 100, EGG));

		//add host to stand			
		stand.m_host.push_front(pHost);




		for (CTRef d = p.Begin(); d <= p.End(); d++)
		{
			stand.Live(weather.GetDay(d));
			//if (output.IsInside(d))
			stand.GetStat(d, output[d]);
			//output[d][O_CDD_EOD] = stand.m_EOD_CDD[d][0];

			stand.AdjustPopulation();
			HxGridTestConnection();
		}


		//CStatistic stat = output.GetStat(S_EGG_HATCH, p);
		//if (stat.IsInit() && stat[SUM] > 0)
		//{
		//	output[p.Begin()][O_CUMUL_EGG_HATCH] = 0;
		//	for (CTRef TRef = p.Begin() + 1; TRef <= p.End(); TRef++)
		//		output[TRef][O_CUMUL_EGG_HATCH] = output[TRef - 1][O_CUMUL_EGG_HATCH] + 100 * output[TRef][S_EGG_HATCH] / stat[SUM];
		//}

		//stat = output.GetStat(S_ADULT, p);
		//if (stat.IsInit() && stat[SUM] > 0)
		//{
		//	output[p.Begin()][O_CUMUL_ADULT] = 0;
		//	for (CTRef TRef = p.Begin() + 1; TRef <= p.End(); TRef++)
		//		output[TRef][O_CUMUL_ADULT] = output[TRef - 1][O_CUMUL_ADULT] + 100 * output[TRef][S_ADULT] / stat[SUM];
		//}
	}

	enum TInput { I_EGG_HATCH, I_CUMUL_EGG_HATCH = DEAD_ADULT, NB_INPUTS = DEAD_ADULT * 2 };
	void CSpottedLanternflyModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		ASSERT(data.size() == 14);

		CSAResult obs;

		obs.m_ref.FromFormatedString(data[1]);
		obs.m_obs.resize(NB_INPUTS);
		for (size_t s = 0; s < NB_INPUTS; s++)
		{
			obs.m_obs[s] = (data[s + 2] != "NA") ? stod(data[s + 2]) : -999.0;//EggHatch

			if (s < I_CUMUL_EGG_HATCH && obs.m_obs[s] >= 0)
				m_DOY[s] += obs.m_ref.GetJDay();
		}


		//if(!m_SAResult.empty())
		//obs.m_obs[I_CUMUL_EGG_HATCH] = stod(data[2]);//EggHatch
		//obs.m_obs[I_CUMUL_ADULT] = stod(data[7]);//Adult emergence

		m_years.insert(obs.m_ref.GetYear());


		m_SAResult.push_back(obs);

	}

	void CSpottedLanternflyModel::GetEggHacth(const array<double, NB_CDD_PARAMS >& P, const CWeatherYear& weather, CModelStatVector& output)const
	{
		//CModelDistribution dist(m_EOD);

	}

	bool CSpottedLanternflyModel::GetFValueDailyEggHacth(CStatisticXY& stat)
	{
		if (m_EOD[CDD_Τᴴ¹] > m_EOD[CDD_Τᴴ²])
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
			CModelDistribution::get_CDF(m_EOD, m_weather[year], cumcul_output);

			CTPeriod p = cumcul_output.GetTPeriod();

			for (size_t i = 0; i < m_SAResult.size(); i++)
			{
				if (m_SAResult[i].m_ref.GetYear() == year)
				{
					//flies catch 

					if (m_SAResult[i].m_obs[I_CUMUL_EGG_HATCH] > -999)
					{
						double obs = m_SAResult[i].m_obs[I_CUMUL_EGG_HATCH];
						double sim = cumcul_output[m_SAResult[i].m_ref][0];

						//for (size_t ii = 0; ii < log(Nd); ii++)
						stat.Add(obs, sim);
					}

				}
			}//for all results

		}

		return true;

	}

	bool CSpottedLanternflyModel::Overwintering(CStatisticXY& stat)
	{
		if (m_EOD[CDD_Τᴴ¹] > m_EOD[CDD_Τᴴ²])
			return false;

		//T  Time Survival   
		static double S[15][3] = {
{ 5 ,  7.0, 0.2},
{ 5 , 28.0, 1.2},
{ 5 , 56.0, 8.8},
{ 5 , 84.0,19.6},
{ 5 ,112.0,20.9},
{ 5 ,140.0,10.6},
{10 ,  7.0, 0.2},
{10 , 28.0, 2.4},
{10 , 56.0,34.3},
{10 , 84.0,52.7},
{10 ,216.0,30.6},
{10 ,241.5, 2.0},
{10 ,250.0, 1.3},
{15 , 93.4,58.4},
{20 , 69.3,10.8},
		};

		//int year = weather.GetTRef().GetYear();
		//CTPeriod pp(CTRef(year, JANUARY, DAY_01), CTRef(year, DECEMBER, DAY_31));

		//if (CHCDD.empty())
		//	CHCDD.Init(pp, 1, 0);


		//double CHDD = 0;
		//for (CTRef TRef = pp.Begin(); TRef <= pp.End(); TRef++)
		//{
		//	//if(TRef.GetMonth()<APRIL|| TRef.GetMonth()>SEPTEMBER)
		//	double Tair = weather.GetDay(TRef)[H_TNTX][MEAN];
		//	if (Tair > P[CDD_Τᴴ¹] && Tair < P[CDD_Τᴴ²])
		//		CHDD += -(Tair - P[CDD_Τᴴ²]);

		//	CHCDD[TRef][0] = CHDD;
		//}
		
		vector<pair<double, double>> SS;
		double totalS = 254.0;
		for (size_t i = 0; i < 15; i++)
		{
			//if(TRef.GetMonth()<APRIL|| TRef.GetMonth()>SEPTEMBER)
			//double Tair = weather.GetDay(TRef)[H_TNTX][MEAN];
			//if (Tair > m_EOD[CDD_Τᴴ¹] && Tair < m_EOD[CDD_Τᴴ²])
				//CHDD += -(Tair - m_EOD[CDD_Τᴴ²]);

			double CHDD = -(S[i][0] - m_EOD[CDD_Τᴴ²]) * S[i][1];
			SS.push_back(make_pair(CHDD,S[i][2]/ totalS));
		}

		sort(SS.begin(), SS.end());
		for (size_t i = 1; i < SS.size(); i++)
		{
			SS[i].second += SS[i - 1].second;
		}

		ASSERT(SS.back().second == 1);

		double LL = 0;
		for (size_t i = 0; i < SS.size(); i++)
		{
			double cdf = CModelDistribution::get_cdf(SS[i].first, CModelDistribution::TType(m_EOD[CDD_DIST]), m_EOD[CDD_P1], m_EOD[CDD_P2]);
			stat.Add(SS[i].second, cdf);
		}


		return true;
	}

	bool CSpottedLanternflyModel::GetFValueDaily(CStatisticXY& stat)
	{
		return Overwintering(stat);



		if (m_EOD[CDD_Τᴴ¹] > m_EOD[CDD_Τᴴ²])
			return false;

		//double Nd = 0;
		//for (size_t i = 0; i < m_SAResult.size(); i++)
		//{
		//	if (m_SAResult[i].m_obs[I_EGG_HATCH] > -999)
		//		Nd += m_SAResult[i].m_obs[I_EGG_HATCH];
		//}


		for (auto it = m_years.begin(); it != m_years.end(); it++)
		{
			int year = *it;

			CModelStatVector output;
			ExecuteDaily(m_weather[year], output);
			GetEggHacth(m_EOD, m_weather[year], output);

			CTPeriod p = output.GetTPeriod();
			CModelStatVector cumcul_output(p, DEAD_ADULT, 0);


			for (size_t s = 0; s < DEAD_ADULT; s++)
			{
				size_t ss = (s == 0) ? S_EGG_HATCH : S_EGG + s;
				CStatistic stat = output.GetStat(ss, p);
				if (stat.IsInit() && stat[SUM] > 0)
				{
					cumcul_output[p.Begin()][s] = 0;
					for (CTRef TRef = p.Begin() + 1; TRef <= p.End(); TRef++)
						cumcul_output[TRef][s] = cumcul_output[TRef - 1][s] + 100 * output[TRef][ss] / stat[SUM];
				}
			}

			for (size_t i = 0; i < m_SAResult.size(); i++)
			{
				if (m_SAResult[i].m_ref.GetYear() == year)
				{
					//flies catch 
					for (size_t s = N1; s <= N4; s++)
					{
						if (m_SAResult[i].m_obs[I_CUMUL_EGG_HATCH + s] > -999)
						{
							double obs = m_SAResult[i].m_obs[I_CUMUL_EGG_HATCH + s];
							double sim = cumcul_output[m_SAResult[i].m_ref][s];

							//	for (size_t ii = 0; ii < log(Nd); ii++)
							stat.Add(obs, sim);


							if (obs >= 0.5 && obs <= 99.5)
							{
								double sim_DOY = GetSimDOY(s, m_SAResult[i].m_ref, obs, cumcul_output);
								if (sim_DOY > -999)
								{
									double obs_DOYp = GetDOYPercent(s, m_SAResult[i].m_ref.GetJDay());
									double sim_DOYp = GetDOYPercent(s, sim_DOY);

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


	double CSpottedLanternflyModel::GetSimDOY(size_t s, CTRef TRefO, double obs, const CModelStatVector& output)
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

	double  CSpottedLanternflyModel::GetDOYPercent(size_t s, double DOY)const
	{
		//return value can be negative of greater than 100%
		return 100 * (DOY - m_DOY.at(s)[LOWEST]) / m_DOY.at(s)[RANGE];
	}

}
