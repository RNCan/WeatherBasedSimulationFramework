//***********************************************************
// 01/03/2023	1.0.0	Rémi Saint-Amant   Creation
//***********************************************************
#include "SPBInfestationIndexModel.h"
#include "ModelBase/EntryPoint.h"
#include "Basic/WeatherDefine.h"
#include <boost/math/distributions/normal.hpp>

using namespace std;
using namespace WBSF::HOURLY_DATA;

namespace WBSF
{
	enum SeasonalVariables { V_WINTER_TMIN, V_SPRING_T, V_SUMMER_T, V_FALL_T, V_SPRING_P, V_WINTER_P, NB_SEASONAL_VARIABLES };

	enum TOutput { O_TMIN, O_TMAX, O_T1, O_T2, O_T3, O_P1, O_P2, O_P3, NB_OUTPUTS };
	enum TOutputM2 { O_SPTt, O_FTPt, O_SPPt1, O_WNPt1, O_MinWNTt1, O_SPTt2, O_SMPt2, O_SPBII, NB_OUTPUTS2 };
	//enum TDailyOutput { O_TMIN, O_TMAX, O_T1, O_T2, O_T3, O_P1, O_P2, O_P3, NB_DAILY_OUTPUTS};
	

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CSPBInfestationIndexModel::CreateObject);

	CSPBInfestationIndexModel::CSPBInfestationIndexModel()
	{
		//NB_INPUT_PARAMETER is used to determine if the dll
		//uses the same number of parameters than the model interface
		NB_INPUT_PARAMETER = 0;
		VERSION = "1.0.0 (2023)";
	}

	CSPBInfestationIndexModel::~CSPBInfestationIndexModel()
	{
	}


	//this method is call to load your parameter in your variable
	ERMsg CSPBInfestationIndexModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		size_t c = 0;
		//m_bApplyAttrition = parameters[c++].GetBool();
		//m_bApplyFrost = parameters[c++].GetBool();
		//m_bCumul = parameters[c++].GetBool();



		return msg;
	}





	//This method is called to compute the solution
	ERMsg CSPBInfestationIndexModel::OnExecuteAnnual()
	{
		ERMsg msg;

		
		bool bAn2002 = m_info.m_modelName == "SouthernPineBeetleInfestationsIndex(An2022)";
		
		if (bAn2002)//An (2022) version
		{
			//This is where the model is actually executed

			//lnSPTt(log average spring temperature in the current year) 0.0154 0.0052 0.003
			//lnFLTt(log average fall temperature in the current year)   0.0141 0.0053 0.008
			//lnSPPt1(log spring total precipitation one year ago)      0.0010 0.0004 0.006
			//lnWNPt1(log winter total precipitation one year ago)      0.0011 0.0003 0.000
			//lnMinWNTt1(log winter min temperature one year ago)       0.0047 0.0017 0.007
			//lnSPTt2(log average spring temperature two years ago)     0.0096 0.0049 0.049
			//lnSMTt2(log average summer temperature two years ago)     0.0218

			static const array<double, 7> P =
			{
				0.0154,0.0141,0.0010,0.0011,0.0047,0.0096,0.0218
			};

			CModelStatVector seasonal_variable;
			ComputeSeasonalVariable(m_weather, seasonal_variable);

			CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::ANNUAL));
			m_output.Init(p, NB_OUTPUTS, -9999);

			for (size_t y = 0; y < p.GetNbYears(); y++)
			{
				m_output[y][O_SPTt] = seasonal_variable[y][V_SPRING_T];
				m_output[y][O_FTPt] = seasonal_variable[y][V_FALL_T];
				if (y >= 1)
				{
					m_output[y][O_SPPt1] = seasonal_variable[y - 1][V_SPRING_P];
					m_output[y][O_WNPt1] = seasonal_variable[y - 1][V_WINTER_P];
					m_output[y][O_MinWNTt1] = seasonal_variable[y - 1][V_WINTER_TMIN];
				}



				if (y >= 2)
				{
					m_output[y][O_SPTt2] = seasonal_variable[y - 2][V_SPRING_T];
					m_output[y][O_SMPt2] = seasonal_variable[y - 2][V_SUMMER_T];

					double probit = 
						P[0] * seasonal_variable[y][V_SPRING_T] +
						P[1] * seasonal_variable[y][V_FALL_T] +
						P[2] * seasonal_variable[y - 1][V_SPRING_P] +
						P[3] * seasonal_variable[y - 1][V_WINTER_P] + 
						P[4] * seasonal_variable[y - 1][V_WINTER_TMIN] + 
						P[5] * seasonal_variable[y - 2][V_SPRING_T] + 
						P[5] * seasonal_variable[y - 2][V_SUMMER_T];

					boost::math::normal_distribution<double> dist(0.0, 1.0);
					double prob = boost::math::cdf(dist, probit);
					

					m_output[y][O_SPBII] = prob;
				}
			}
		}
		else//Lesk (2017) version
		{
			
			if (!m_weather.IsHourly())
				m_weather.ComputeHourlyVariables();

			//This is where the model is actually executed
			//steam diameter
			static const array <double, 3> K =
			{
				0.221,// 15 cm
				0.154,// 45 cm(mean)
				0.077// 51 cm
			};


			CTPeriod p_h = m_weather.GetEntireTPeriod(CTM(CTM::HOURLY));
			CModelStatVector T;
			T.Init(p_h, 3, -999);

			for (size_t h = 0; h < p_h.size(); h++)
			{
				const CHourlyData& data = m_weather.GetHour(p_h.Begin() + h);
				for (size_t v = 0; v < 3; v++)
				{
					if (h == 0)
					{
						T[h][v] = data[H_TAIR] + 4;
					}
					else
					{
						double deltaT = data[H_TAIR] - T[h - 1][v];
						T[h][v] = T[h - 1][v] + K[v] * deltaT;
					}
				}
			}

			boost::math::normal_distribution<double> dist(-10.5,2.58);
			
			

			CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::ANNUAL));
			m_output.Init(p, NB_OUTPUTS, -9999);

			for (size_t y = 0; y < p.GetNbYears(); y++)
			{
				CTPeriod pyh = m_weather[y].GetEntireTPeriod(CTM(CTM::HOURLY));

				array<CStatistic, 3> stat;
				for (CTRef TRef = pyh.Begin(); TRef <= pyh.End(); TRef++)
				{
					for (size_t v = 0; v < 3; v++)
						stat[v] += T[TRef][v];
				}

				m_output[y][O_TMIN] = m_weather[y].GetStat(H_TMIN)[LOWEST];
				m_output[y][O_TMAX] = m_weather[y].GetStat(H_TMAX)[HIGHEST];
				for (size_t v = 0; v < 3; v++)
				{
					double T = stat[v][LOWEST];
					m_output[y][O_T1 + v] = T;
					//double P = max(0.0, -0.00425 * T * T - 0.0888 * T - 0.305);
					double P = boost::math::pdf(dist, T);
					m_output[y][O_P1 + v] = P;
				}
				
			}
		}


		return msg;
	}

	ERMsg CSPBInfestationIndexModel::OnExecuteDaily()
	{
		ERMsg msg;


		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();

		//This is where the model is actually executed
		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));
		m_output.Init(p, NB_OUTPUTS, 0);

		static const array <double, 3> K =
		{
			0.221,// 15 cm
			0.154,// (mean)45 cm(mean)
			0.077// 51 cm
		};


		CTPeriod p_h = m_weather.GetEntireTPeriod(CTM(CTM::HOURLY));
		CModelStatVector T;
		T.Init(p_h, 3, -999);

		for (size_t h = 0; h < p_h.size(); h++)
		{
			const CHourlyData& data = m_weather.GetHour(p_h.Begin()+h);
			for (size_t v = 0; v < 3; v++)
			{
				if (h == 0)
				{
					T[h][v] = data[H_TAIR]+4;
				}
				else
				{
					double deltaT = data[H_TAIR] - T[h - 1][v];
					T[h][v] = T[h - 1][v] + K[v] * deltaT;
				}
			}
		}

		size_t h = 0;
		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
		{
			array<CStatistic,3> stat;
			for (size_t hh = 0; hh < 24; h++,hh++)
			{
				for (size_t v = 0; v < 3; v++)
					stat[v] += T[h][v];
			}

			m_output[TRef][O_TMIN] = m_weather.GetDay(TRef)[H_TMIN][MEAN];
			m_output[TRef][O_TMAX] = m_weather.GetDay(TRef)[H_TMAX][MEAN];

			for (size_t v = 0; v < 3; v++)
			{
				m_output[TRef][O_T1+v] = stat[v][MEAN];
			}
		}

		return msg;
	}

	void CSPBInfestationIndexModel::ComputeSeasonalVariable(const CWeatherYears& weather, CModelStatVector& output)
	{
		CTPeriod p = weather.GetEntireTPeriod(CTM(CTM::ANNUAL));

		output.Init(p, NB_SEASONAL_VARIABLES, 0);

		for (size_t y = 0; y < p.GetNbYears(); y++)
		{
			int year = p[y].GetYear();
			for (size_t v = 0; v < NB_SEASONAL_VARIABLES; v++)
			{
				switch (v)
				{
				case V_WINTER_TMIN:
				{
					CTPeriod p(CTRef(year - 1, DECEMBER, FIRST_DAY), CTRef(year, FEBRUARY, LAST_DAY), CTPeriod::YEAR_BY_YEAR);
					CStatistic stat = weather.GetStat(H_TMIN, p);
					output[y][V_WINTER_TMIN] = stat[LOWEST];
					break;
				}
				case V_SPRING_T:
				{
					CTPeriod p(CTRef(year, MARCH, FIRST_DAY), CTRef(year, MAY, LAST_DAY), CTPeriod::YEAR_BY_YEAR);
					output[y][V_SPRING_T] = weather.GetStat(H_TNTX, p)[MEAN];
					break;
				}
				case V_SUMMER_T:
				{
					CTPeriod p(CTRef(year - 1, JUNE, FIRST_DAY), CTRef(year, AUGUST, LAST_DAY), CTPeriod::YEAR_BY_YEAR);
					output[y][V_SUMMER_T] = weather.GetStat(H_TNTX, p)[MEAN];
					break;
				}
				case V_FALL_T:
				{
					CTPeriod p(CTRef(year - 1, SEPTEMBER, FIRST_DAY), CTRef(year, NOVEMBER, LAST_DAY), CTPeriod::YEAR_BY_YEAR);
					output[y][V_FALL_T] = weather.GetStat(H_TNTX, p)[MEAN];
					break;
				}
				case V_SPRING_P:
				{
					CTPeriod p(CTRef(year, MARCH, FIRST_DAY), CTRef(year, MAY, LAST_DAY), CTPeriod::YEAR_BY_YEAR);
					output[y][V_SPRING_P] = weather.GetStat(H_PRCP, p)[SUM];
					break;
				}
				case V_WINTER_P:
				{
					CTPeriod p(CTRef(year - 1, DECEMBER, FIRST_DAY), CTRef(year, FEBRUARY, LAST_DAY), CTPeriod::YEAR_BY_YEAR);
					output[y][V_WINTER_P] = weather.GetStat(H_PRCP, p)[SUM];
					break;
				}
				default: ASSERT(false);
				}
			}

		}




	}


}


