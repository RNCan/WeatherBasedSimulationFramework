//*********************************************************************
//21/01/2020	1.0.0	Rémi Saint-Amant	Creation
//*********************************************************************
#include "BudBurstChuine.h"
#include "ModelBase/EntryPoint.h"
#include "Basic\DegreeDays.h"
#include <boost/math/distributions/weibull.hpp>
#include <boost/math/distributions/beta.hpp>
#include <boost/math/distributions/Rayleigh.hpp>
#include <boost/math/distributions/logistic.hpp>
#include <boost/math/distributions/exponential.hpp>


using namespace std;
using namespace WBSF::HOURLY_DATA;

namespace WBSF
{




	size_t CU_STAT = H_TAIR;
	size_t FU_STAT = H_TAIR;


	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CBudBurstChuineModel::CreateObject);


	enum TOutput { O_CU, O_FU, O_PS, O_SDI, NB_OUTPUTS };


	double CBudBurstChuineModel::ChillingResponce(double T)const
	{
		double R = 0;
		switch (TRFunction(m_P[R_FUNCTION]))
		{
		case SIGMOID: R = 1 / (1 + exp(-(T - m_P[CU_µ]) / m_P[CU_σ¹])); break;
		case CHUINE: R = 1 / (1 + exp(-((T - m_P[CU_µ]) / m_P[CU_σ¹] + Square(T - m_P[CU_µ]) / m_P[CU_σ²]))); break;
		case RICHARDSON: R = max(0.0, min(m_P[CU_Thigh] - T, m_P[CU_Thigh] - m_P[CU_Tlow])); break;
		case UTAH:
		{
			ASSERT(m_P[CU_P_MIN] >= -1.0 && m_P[CU_P_MIN] <= 0.0);
			if (T < m_P[CU_T_MIN])
			{
				R = 1.0 / (1.0 + exp(-4 * ((T - m_P[CU_T_MIN]) / (m_P[CU_T_OPT] - m_P[CU_T_MIN]))));
			}
			else if (T >= m_P[CU_T_MIN] && T < m_P[CU_T_OPT])
			{
				R = 1 - 0.5 * Square(T - m_P[CU_T_OPT]) / Square(m_P[CU_T_OPT] - m_P[CU_T_MIN]);
			}
			else if (T >= m_P[CU_T_OPT] && T < m_P[CU_T_MAX])
			{
				R = 1 - (1 - m_P[CU_P_MIN]) * Square(T - m_P[CU_T_OPT]) / (2 * Square(m_P[CU_T_MAX] - m_P[CU_T_OPT]));
			}
			else
			{
				R = m_P[CU_P_MIN] + (1 - m_P[CU_P_MIN]) / (1.0 + exp(-4 * ((m_P[CU_T_MAX] - T) / (m_P[CU_T_MAX] - m_P[CU_T_OPT]))));
			}


			break;
		}
		default: ASSERT(false);
		};


		ASSERT(!_isnan(R) && _finite(R) );

		return R;
	}

	double CBudBurstChuineModel::ForcingResponce(double T)const
	{
		double R = 1 / (1 + exp(-(T - m_P[FU_µ]) / m_P[FU_σ]));
		ASSERT(!_isnan(R) && _finite(R) && R >= 0);

		return R;
	}





	static const double MIN_SDI = 0;
	static const double MAX_SDI = 6;


	CBudBurstChuineModel::CBudBurstChuineModel()
	{
		// initialize your variable here (optional)
		NB_INPUT_PARAMETER = -1;
		VERSION = "1.0.0 (2021)";

		m_species = 0;
		m_SDI_type = SDI_DHONT;

		m_P = { CHUINE_SEQUENTIAL_METHOD, 244,3.1,26.7,2298.8,0.244,13.46,21.5,8.00,7.69,0.0997,0.0888,0.0795,19.71,2.59,-217.3,-257.6,0.650,10,3.77,0.0464,-0.0633,0.856,2.64 };
	}



	CBudBurstChuineModel::~CBudBurstChuineModel()
	{
	};


	//this method is call to load your parameter in your variable
	ERMsg CBudBurstChuineModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		//transfer your parameter here
		size_t c = 0;
		m_species = parameters[c++].GetInt();
		m_SDI_type = parameters[c++].GetInt();
		m_defoliation = parameters[c++].GetReal();

		//ASSERT(m_species < HBB::PARAMETERS.size());


		if (parameters.size() == 3 + NB_PARAMS)
		{
			for (size_t i = 0; i < NB_PARAMS; i++)
				m_P[i] = parameters[c++].GetReal();
		}

		return msg;
	}





	//This method is call to compute solution
	ERMsg CBudBurstChuineModel::OnExecuteDaily()
	{
		ERMsg msg;

		CTPeriod pp(m_weather.GetEntireTPeriod(CTM::DAILY));
		m_output.Init(pp, NB_OUTPUTS, -999);

		ExecuteAllYears(m_weather, m_output);


		return msg;
	}



	void CBudBurstChuineModel::ExecuteAllYears(CWeatherYears& weather, CModelStatVector& output)
	{
		//boost::math::beta_distribution<double> SDI_dist(m_P[muSDI], m_P[ѕigmaSDI]);
		boost::math::weibull_distribution<double> SDI_dist(m_P[SDI_µ], m_P[SDI_σ]);


		CTPeriod pp(weather.GetEntireTPeriod(CTM::DAILY));
		if (mean_T_day.empty())
		{
			mean_T_day.resize(pp.GetNbDay());

			for (size_t y = 0, dd = 0; y < weather.GetNbYears(); y++)
			{
				for (size_t m = 0; m < weather[y].GetNbMonth(); m++)
				{
					for (size_t d = 0; d < weather[y][m].GetNbDays(); d++, dd++)
					{
						array<CStatistic, 3> T_CU_Days_stat;
						array<CStatistic, 3> T_FU_Days_stat;

						size_t p_CU = size_t(m_P[CU_DAYS]);
						size_t p_FU = size_t(m_P[FU_DAYS]);
						size_t max_days = max(p_CU, p_FU);


						CTRef TRef = weather[y][m][d].GetTRef();
						for (size_t i = 0; i < max_days; i++)//Charrier 2018 use the mean maximum of the last 14 days 
						{

							if (i < p_CU)
							{
								T_CU_Days_stat[0] += weather.GetDay(TRef - min(dd, p_CU))[H_TMIN];
								T_CU_Days_stat[1] += weather.GetDay(TRef - min(dd, p_CU))[H_TAIR];
								T_CU_Days_stat[2] += weather.GetDay(TRef - min(dd, p_CU))[H_TMAX];
							}
							if (i < p_FU)
							{
								T_FU_Days_stat[0] += weather.GetDay(TRef - min(dd, p_FU))[H_TMIN];
								T_FU_Days_stat[1] += weather.GetDay(TRef - min(dd, p_FU))[H_TAIR];
								T_FU_Days_stat[2] += weather.GetDay(TRef - min(dd, p_FU))[H_TMAX];
							}
						}

						for (size_t i = 0; i < 3; i++)
						{
							mean_T_day[dd].T_CU_days[i] = T_CU_Days_stat[i][MEAN];
							mean_T_day[dd].T_FU_days[i] = T_FU_Days_stat[i][MEAN];
						}
					}
				}
			}
		}

		for (size_t yy = 1; yy < weather.GetNbYears(); yy++)
		{
			double CU = 0;
			double FU = 0;
			if (TMethod(m_P[METHOD]) == CHUINE_ALTERNATING_METHOD)
				m_P[FU_crit] = -1;

			for (size_t yyy = 0; yyy < 2; yyy++)
			{
				size_t y = yy + yyy - 1;
				size_t m1 = yyy == 0 ? AUGUST : JANUARY;
				size_t m2 = yyy == 0 ? DECEMBER : JULY;
				size_t dd = 0;

				for (size_t m = m1; m <= m2; m++)
				{
					for (size_t d = 0; d < weather[y][m].GetNbDays(); d++, dd++)
					{
						CTRef TRef = weather[y][m][d].GetTRef();

						if (yyy == 1 || TRef.GetJDay() >= (m_P[To] - 1))
						{
							double T_CU = mean_T_day[TRef - pp.Begin()].T_CU_days[CU_STAT];
							double T_FU = mean_T_day[TRef - pp.Begin()].T_FU_days[FU_STAT];


							if (CU < m_P[CU_crit])
							{
								//CU += max(0.0, min(m_P[Thigh] - T_CU, m_P[Thigh] - m_P[Tlow]));
								CU += ChillingResponce(T_CU); 
								CU = min(CU, m_P[CU_crit]);
							}
							else
							{
								if (TMethod(m_P[METHOD]) == CHUINE_ALTERNATING_METHOD && m_P[FU_crit] == -1)
								{
									m_P[FU_crit] = max(1.0, m_P[FUw] * exp(-m_P[FUz] * CU));
								}

								FU += ForcingResponce(T_FU);
								FU = min(FU, m_P[FU_crit]);
							}



							//double Fdef = 1 - m_P[DEF_min] / (1 + exp(-(m_defoliation - m_P[DEF_µ]) / m_P[DEF_σ])); 

							double PS = CU / m_P[CU_crit] + FU / m_P[FU_crit];
							double SDI_Dhont = cdf(SDI_dist, max(0.0, min(1.0, PS - 1))) * MAX_SDI;//0 .. 6;
							double SDI_Auger = max(0.0, min(5.0, exp(log(5) * (SDI_Dhont - 2.5) / (5.6 - 2.5)) - 0.33));//0 .. 5;


							output[TRef][O_CU] = CU; 
							output[TRef][O_FU] = FU;
							output[TRef][O_PS] = PS;
							output[TRef][O_SDI] = m_SDI_type == SDI_DHONT ? SDI_Dhont : SDI_Auger;
						}
					}
				}
			}
		}
	}




	enum { I_SPECIES1, I_SOURCE1, I_SITE1, I_DATE1, I_SDI1, I_N1, NB_INPUTS1 };
	enum { I_SPECIES2, I_SOURCE2, I_SITE2, I_DATE2, I_STARCH2, I_SUGAR2, I_SDI2, I_DEFOL2, NB_INPUTS2 };
	void CBudBurstChuineModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		static const char* SPECIES_NAME[] = { "bf", "ws", "bs", "ns", "rs" };

		if (data.size() == NB_INPUTS1)
		{
			if (data[I_SPECIES1] == SPECIES_NAME[m_species])
			{
				CSAResult obs;

				obs.m_ref.FromFormatedString(data[I_DATE1]);
				obs.m_obs[0] = stod(data[I_SDI1]);
				obs.m_obs.push_back(stod(data[I_N1]));

				if (obs.m_obs[0] > -999)
				{
					m_years.insert(obs.m_ref.GetYear());
				}


				m_SAResult.push_back(obs);
			}
		}
		else if (data.size() == NB_INPUTS2)
		{
			if (data[I_SPECIES2] == SPECIES_NAME[m_species])
			{

				CSAResult obs;

				obs.m_ref.FromFormatedString(data[I_DATE2]);
				obs.m_obs[0] = stod(data[I_SDI2]);
				obs.m_obs.push_back(1);
				obs.m_obs.push_back(stod(data[I_DEFOL2]));
				

				if (obs.m_obs[0] > -999)
				{
					m_years.insert(obs.m_ref.GetYear());
				}

				m_SAResult.push_back(obs);
			}
		}

	}




	void CBudBurstChuineModel::GetFValueDaily(CStatisticXY& stat)
	{

		if (!m_SAResult.empty())
		{
			if ((m_P[CU_σ_PS] > -0.1 && m_P[CU_σ_PS] < 0.1) ||
				(m_P[FU_σ_PS] > -0.1 && m_P[FU_σ_PS] < 0.1))
				return;

			if (TRFunction(m_P[R_FUNCTION]) == UTAH)
			{
				if ((m_P[CU_T_MIN] > m_P[CU_T_OPT]) ||
					(m_P[CU_T_OPT] > m_P[CU_T_MAX]))
					return;
			}


			if (data_weather.GetNbYears() == 0)
			{
				CTPeriod pp((*m_years.begin()) - 1, JANUARY, DAY_01, *m_years.rbegin(), DECEMBER, DAY_31);
				pp.Transform(m_weather.GetEntireTPeriod());
				pp = pp.Intersect(m_weather.GetEntireTPeriod());
				if (pp.IsInit())
				{
					((CLocation&)data_weather) = m_weather;
					data_weather.SetHourly(m_weather.IsHourly());
					data_weather.CreateYears(pp);

					for (int year = pp.GetFirstYear(); year <= pp.GetLastYear(); year++)
					{
						data_weather[year] = m_weather[year];
					}
				}
				else
				{
					//remove these obs, no input weather
					m_SAResult.clear();
					return;
				}

			}

			CTPeriod pp(data_weather.GetEntireTPeriod(CTM::DAILY));
			CModelStatVector output(pp, NB_OUTPUTS, -999);

			ExecuteAllYears(data_weather, output);


			//boost::math::beta_distribution<double> SDI_dist(m_P[muSDI], m_P[ѕigmaSDI]);
			boost::math::weibull_distribution<double> SDI_dist(m_P[SDI_µ], m_P[SDI_σ]);

			for (size_t i = 0; i < m_SAResult.size(); i++)
			{
				if (output.IsInside(m_SAResult[i].m_ref))
				{
					if (m_SAResult[i].m_obs[0] > -999 && m_SAResult[i].m_ref.GetJDay() < 244 && output[m_SAResult[i].m_ref][O_SDI] > -999)
					{
						double obs_SDI = Round(m_SAResult[i].m_obs[0], 2);
						//double sim_SDI = Round(output[m_SAResult[i].m_ref][O_SDI], 2);
						// 

						double Fdef = 1;
						if (m_P[Used_DEF] != 0)
						{
							double defol = m_SAResult[i].m_obs[2] > -999 ? m_SAResult[i].m_obs[2] : 0;
							double Fdef = 1 - (1 - m_P[DEF_min]) / (1 + exp(-(defol - m_P[DEF_µ]) / m_P[DEF_σ]));
						}

						double CU = output[m_SAResult[i].m_ref][O_CU]; 
						double FU = output[m_SAResult[i].m_ref][O_FU];
						double PS = min(1.0,CU / (m_P[CU_crit] )) + min(1.0,(FU)/ (m_P[FU_crit] * Fdef));
						
						double SDI_Dhont = cdf(SDI_dist, max(0.0, min(1.0, PS - 1))) * MAX_SDI;//0 .. 6;
						double SDI_Auger = max(0.0, min(5.0, exp(log(5) * (SDI_Dhont - 2.5) / (5.6 - 2.5)) - 0.33));//0 .. 5;
						double SDI = m_SDI_type == SDI_DHONT ? SDI_Dhont : SDI_Auger;


						double sim_SDI = Round(SDI, 2);


						//for(size_t n=0; n< m_SAResult[i].m_obs[1]; n++)
						stat.Add(obs_SDI, sim_SDI);
					}
				}
			}//for all results
		}
	}








}