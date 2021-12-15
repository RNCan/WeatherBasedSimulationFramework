//*********************************************************************
//21/01/2020	1.0.0	Rémi Saint-Amant	Creation
//*********************************************************************
#include "BudBurstNew.h"
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

	enum { VAR_SDI, VAR_STARCH, VAR_GFS };
	static const bool USE_SDI = true;
	static const bool USE_STARCH = true;
	static const bool USE_SUGAR = true;

	size_t S_IN_STAT = H_TMIN;
	size_t S_OUT_STAT = H_TMAX;
	size_t ST_IN_STAT = H_TMIN;
	size_t ST_OUT_STAT = H_TMAX;


	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CBudBurstNewModel::CreateObject);


	enum TOutput { O_GFS_CONC, O_S_CONC = O_GFS_CONC, O_ST_CONC, O_CU, O_FU, O_PS, O_WC, O_R, O_BUDBURST, O_SDI, NB_OUTPUTS };
	enum TMethod { CHUINE_SEQUENTIAL_METHOD, CHUINE_ALTERNATING_METHOD, CHARRIER_SUGAR_STARCH, SAINT_AMANT_SUGAR_STARCH, NB_METHOD };
	TMethod METHOD = SAINT_AMANT_SUGAR_STARCH;


	static const double MIN_SDI = 0;
	static const double MAX_SDI = 6;
	static const double MAX_STRACH = 120;//Deslauriers data 
	static const double MIN_STRACH = 5;//Deslauriers data 
	static const double MAX_SUGAR = 100;//Schaberg
	static const double MIN_SUGAR = 7;//Deslauriers data 



	CBudBurstNewModel::CBudBurstNewModel()
	{
		// initialize your variable here (optional)
		NB_INPUT_PARAMETER = -1;
		VERSION = "1.0.0 (2021)";
		m_P = { 244,3.1,26.7,2298.8,0.244,13.46,21.5,8.00,7.69,0.0997,0.0888,0.0795,19.71,2.59,-217.3,-257.6,0.650,10,3.77,0.0464,-0.0633,0.856,2.64,-165.5,0.924,0.693 };
	}



	CBudBurstNewModel::~CBudBurstNewModel()
	{
	};


	//this method is call to load your parameter in your variable
	ERMsg CBudBurstNewModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		//transfer your parameter here
		size_t c = 0;
		m_species = parameters[c++].GetInt();

		//ASSERT(m_species < HBB::PARAMETERS.size());


		if (parameters.size() == 1 + NB_PARAMS)
		{
			for (size_t i = 0; i < NB_PARAMS; i++)
				m_P[i] = parameters[c++].GetReal();
		}

		return msg;
	}





	//estimate of water Content (WC) g/gDM
	//
	double GetWC(CTRef TRef, const CWeatherYears& weather)
	{
		//Adjusted R-squared:  0.4999 
		//F - statistic: 36.99 on 1 and 35 DF, p - value : 6.023e-07
		// 
		//mean of the last 23 days
		CStatistic stat;
		for (size_t d = 0; d < 23; d++)
		{
			stat += weather.GetDay(TRef - d)[H_TNTX][MEAN];
		}

		double WC = stat[MEAN] * 0.034600 + 0.914589;


		return WC;
	}

	//This method is call to compute solution
	ERMsg CBudBurstNewModel::OnExecuteDaily()
	{
		ERMsg msg;

		//if (!m_weather.IsHourly())
		//	m_weather.ComputeHourlyVariables();

		//compute input
		// Re sample Daily Climate





		CTPeriod pp(m_weather.GetEntireTPeriod(CTM::DAILY));
		m_output.Init(pp, NB_OUTPUTS, -999);


		/*for (size_t yy = 1; yy < m_weather.GetNbYears(); yy++)
		{

		ExecuteOneYear(yy, m_weather, m_output);
		}*/

		ExecuteAllYears(m_weather, m_output);


		return msg;
	}


	//void CBudBurstNewModel::ExecuteOneYear(size_t yy, CWeatherYears& weather, CModelStatVector& output)
	//{
	//	ASSERT(yy > 0 && yy < weather.GetNbYears());

	//	//boost::math::beta_distribution<double> SDI_dist(m_P[muSDI], m_P[ѕigmaSDI]);
	//	boost::math::weibull_distribution<double> SDI_dist(m_P[muSDI], m_P[ѕigmaSDI]);


	//	//boost::math::weibull_distribution<double> k1c_dist(m_P[µ1c], m_P[σ1c]);
	//	//boost::math::weibull_distribution<double> k1m_dist(m_P[µ1m], m_P[σ1m]);
	//	//boost::math::weibull_distribution<double> k2_dist(m_P[µ2], m_P[σ2]);


	//	double CU = 0;
	//	double FU = 0;
	//	double KU = 0;
	//	double GFS = m_P[GFS0]; //mg/gDM
	//	double Starch = m_P[Starch0];//mg/gDM
	//	size_t dd = 0;

	//	enum TMethod { SEQUENTIAL_METHOD, ALTERNATING_METHOD, SUGAR_STARCH };
	//	TMethod METHOD = SUGAR_STARCH;

	//	for (size_t yyy = 0; yyy < 2; yyy++)
	//	{
	//		size_t y = yy + yyy - 1;
	//		size_t m1 = yyy == 0 ? SEPTEMBER : JANUARY;
	//		size_t m2 = yyy == 0 ? DECEMBER : DECEMBER;
	//		for (size_t m = m1; m <= m2; m++)
	//		{
	//			for (size_t d = 0; d < m_weather[y][m].GetNbDays(); d++, dd++)
	//			{
	//				CTRef TRef = m_weather[y][m][d].GetTRef();



	//				if (METHOD == SEQUENTIAL_METHOD)
	//				{
	//					if (yyy == 1 || TRef.GetJDay() >= (m_P[To] - 1))
	//					{
	//						double T = m_weather[y][m][d][H_TNTX][MEAN];

	//						if (CU < m_P[CUcrit])
	//						{
	//							CU += max(0.0, min(m_P[Thigh] - T, m_P[Thigh] - m_P[Tlow]));
	//							CU = min(CU, m_P[CUcrit]);
	//						}
	//						else
	//						{
	//							FU += 1 / (1 + exp(-m_P[slp] * (T - m_P[T50])));
	//							FU = min(FU, m_P[FUcrit]);
	//						}

	//						double PS = CU / m_P[CUcrit] + FU / m_P[FUcrit];

	//						output[TRef][O_GFS] = 0;
	//						output[TRef][O_STARCH] = T;
	//						output[TRef][O_CU] = CU;
	//						output[TRef][O_FU] = FU;
	//						output[TRef][O_PS] = 0;
	//						output[TRef][O_WC] = 0;
	//						output[TRef][O_R] = 0;
	//						output[TRef][O_BUDBURST] = 0;// max(0.0, min(100.0, FU / m_P[FUcrit] * 100));//[%]
	//						output[TRef][O_SDI] = cdf(SDI_dist, FU / m_P[FUcrit]) * 5;//0 .. 5;
	//					}
	//				}
	//				else if (METHOD == ALTERNATING_METHOD)
	//				{
	//					if (yyy == 1 || TRef.GetJDay() >= (m_P[To] - 1))
	//					{
	//						double T = m_weather[y][m][d][H_TNTX][MEAN];



	//						if (dd < size_t(m_P[To]))
	//						{
	//							CU += max(0.0, min(m_P[Thigh] - T, m_P[Thigh] - m_P[Tlow]));
	//						}
	//						else
	//						{
	//							if (dd == size_t(m_P[To]))
	//								m_P[FUcrit] = max(1.0, m_P[a3] * exp(-m_P[b3] * CU));

	//							ASSERT(1 / (1 + exp(-m_P[slp] * (T - m_P[T50]))) >= 0);
	//							FU += 1 / (1 + exp(-m_P[slp] * (T - m_P[T50])));

	//							if (FU > m_P[FUcrit])
	//								FU = m_P[FUcrit];
	//						}


	//						output[TRef][O_GFS] = 0;
	//						output[TRef][O_STARCH] = T;
	//						output[TRef][O_CU] = CU;
	//						output[TRef][O_FU] = FU;
	//						output[TRef][O_PS] = 0;
	//						output[TRef][O_WC] = 0;
	//						output[TRef][O_R] = 0;
	//						output[TRef][O_BUDBURST] = 0;// max(0.0, min(100.0, FU / m_P[FUcrit] * 100));//[%]
	//						output[TRef][O_SDI] = cdf(SDI_dist, FU / m_P[FUcrit]) * 5;//0 .. 5;
	//					}
	//				}
	//				else if (METHOD == SUGAR_STARCH)
	//				{
	//					if (yyy == 1 || TRef.GetJDay() >= (m_P[To] - 1))
	//					{
	//						double T = m_weather[y][m][d][H_TNTX][MEAN];

	//						if (CU < m_P[CUcrit])
	//						{
	//							CU += max(0.0, min(m_P[Thigh] - T, m_P[Thigh] - m_P[Tlow]));
	//							CU = min(CU, m_P[CUcrit]);
	//						}
	//						else 
	//						{
	//							FU += 1 / (1 + exp(-m_P[slp] * (T - m_P[T50])));
	//							//FU = min(FU, m_P[FUcrit]);
	//						}
	//						

	//						ASSERT(CU >= 0 && CU <= m_P[CUcrit]);
	//						ASSERT(FU >= 0 && FU <= m_P[FUcrit]);
	//						double PS = CU / m_P[CUcrit] + FU / m_P[FUcrit];
	//						//ASSERT(PS >= 0 && PS <= 2);

	//						//double a1c = PS < 1 ? (m_P[k1c_trans] + m_P[dendo1c]) / 2 : (m_P[k1c_trans] + m_P[deco1c]) / 2;
	//						//double b1c = PS < 1 ? (m_P[k1c_trans] - m_P[dendo1c]) / 2 : (m_P[k1c_trans] - m_P[deco1c]) / 2;
	//						//double k1c = max(0.0, (a1c * PS + b1c) * exp(-Square(T - m_P[µ1c])) / (m_P[σ1c] * sqrt(2 * PI)));
	//						//
	//						//double a1m = PS < 1 ? (m_P[k1m_trans] + m_P[dendo1m]) / 2 : (m_P[k1m_trans] + m_P[deco1m]) / 2;
	//						//double b1m = PS < 1 ? (m_P[k1m_trans] - m_P[dendo1m]) / 2 : (m_P[k1m_trans] - m_P[deco1m]) / 2;
	//						//double k1m = max(0.0, (a1m * PS + b1m) * exp(-Square(T - m_P[µ1m])) / (m_P[σ1m] * sqrt(2 * PI)));
	//						//
	//						//double a2 = PS < 1 ? (m_P[k2_trans] + m_P[dendo2]) / 2 : (m_P[k2_trans] + m_P[deco2]) / 2;
	//						//double b2 = PS < 1 ? (m_P[k2_trans] - m_P[dendo2]) / 2 : (m_P[k2_trans] - m_P[deco2]) / 2;
	//						//double k2 = (a2 * PS + b2) * exp(-Square(T - m_P[µ2])) / (m_P[σ2] * sqrt(2 * PI));

	//						//double WC = GetWC(TRef, m_weather);
	//						//static const double Tref = 15.0;
	//						//double R = m_P[RMax] / (1 + exp(m_P[a3] * (WC - m_P[b3]))) * pow(m_P[Q10], (T - Tref) / 10);
	//						//double R = m_P[RMax] / (1 + exp(m_P[a3])* (WC - m_P[b3])) * pow(m_P[Q10], (T - Tref) / 10);
	//						//GFS += (k1c + k1m) * Starch/GFS0 - k2 * GFS/Starch0 - R;
	//						//Starch += k2 * GFS/ Starch0 - (k1c + k1m) * Starch/ GFS0;
	//						//GFS += (k1c + k1m) - k2 - R;
	//						//Starch += k2 - (k1c + k1m);


	//						double psy_1c = PS < 1? m_P[dendo1c]: PS < 2 ? m_P[deco1c]: m_P[k1c_trans];
	//						double psy_1m = PS < 1 ? m_P[dendo1m] : PS < 2 ? m_P[deco1m]: m_P[k1m_trans];
	//						double psy_2 = PS < 1 ? m_P[dendo2] : PS < 2 ? m_P[deco2]: m_P[k2_trans];
	//						double k1c = psy_1c *exp(-(T - m_P[µ1c]) / m_P[σ1c])/(m_P[σ1c]*Square(1 + exp(-(T - m_P[µ1c]) / m_P[σ1c])));
	//						double k1m = psy_1m * exp(-(T - m_P[µ1m]) / m_P[σ1m]) / (m_P[σ1m] * Square(1 + exp(-(T - m_P[µ1m]) / m_P[σ1m])));
	//						double k2 = psy_2 * exp(-(T - m_P[µ2]) / m_P[σ2]) / (m_P[σ2] * Square(1 + exp(-(T - m_P[µ2]) / m_P[σ2])));

	//						double delta = (k1c + k1m) - k2;
	//						GFS += (delta>=0? m_P[a3]:m_P[b3])* delta;
	//						Starch -= (delta <= 0 ? m_P[RMax] : m_P[Q10]) * delta;


	//						GFS = max(7.0, min(45.0,GFS));
	//						Starch = max(1.0, min(120.0, Starch));

	//						ASSERT(GFS > 0);
	//						ASSERT(Starch > 0);


	//						output[TRef][O_GFS] = GFS;
	//						output[TRef][O_STARCH] = Starch;
	//						output[TRef][O_CU] = CU;
	//						output[TRef][O_FU] = FU;
	//						output[TRef][O_PS] = PS;
	//						output[TRef][O_WC] = (delta >= 0 ? m_P[a3] : m_P[b3]) * delta;
	//						output[TRef][O_R] = (delta <= 0 ? m_P[RMax] : m_P[Q10]) * delta;
	//						output[TRef][O_BUDBURST] = max(0.0, PS - 1) * 100;//[%]
	//						output[TRef][O_SDI] = cdf(SDI_dist, max(0.0, min(1.0, PS - 1))) * 6;//0 .. 6;
	//					}
	//				}
	//			}
	//		}
	//	}
	//}

	class CCharrierInput
	{
	public:

		array<double, 3> T_S_in_days;
		array<double, 3> T_S_out_days;
		array<double, 3> T_St_in_days;
		array<double, 3> T_St_out_days;

	};

	void CBudBurstNewModel::ExecuteAllYears(CWeatherYears& weather, CModelStatVector& output)
	{
		//boost::math::beta_distribution<double> SDI_dist(m_P[muSDI], m_P[ѕigmaSDI]);
		boost::math::weibull_distribution<double> SDI_dist(m_P[muSDI], m_P[ѕigmaSDI]);


		//boost::math::weibull_distribution<double> k1c_dist(m_P[µ1c], m_P[σ1c]);
		//boost::math::weibull_distribution<double> k1m_dist(m_P[µ1m], m_P[σ1m]);
		//boost::math::weibull_distribution<double> k2_dist(m_P[µ2], m_P[σ2]);



		CTPeriod pp(weather.GetEntireTPeriod(CTM::DAILY));
		deque<CCharrierInput> input(pp.GetNbDay());
		for (size_t y = 0, dd = 0; y < weather.GetNbYears(); y++)
		{
			for (size_t m = 0; m < weather[y].GetNbMonth(); m++)
			{
				for (size_t d = 0; d < weather[y][m].GetNbDays(); d++, dd++)
				{
					CStatistic PN_stat;
					CStatistic Tair_stat;
					CStatistic RC_G_stat;
					CStatistic RC_F_stat;
					CStatistic RC_M_stat;


					array<CStatistic, 3> T_S_in_Days_stat;
					array<CStatistic, 3> T_S_out_Days_stat;
					array<CStatistic, 3> T_St_in_Days_stat;
					array<CStatistic, 3> T_St_out_Days_stat;

					CTRef TRef = weather[y][m][d].GetTRef();
					for (size_t i = 0; i < 30; i++)//Charrier 2018 use the mean maximum of the last 14 days 
					{
						size_t p_S_in = size_t(m_P[D0]);
						size_t p_S_out = size_t(m_P[D1]);
						size_t p_St_in = size_t(m_P[var_aval]);
						size_t p_St_out = size_t(m_P[deco1c]);
						if (i < p_S_in)
						{
							T_S_in_Days_stat[0] += weather.GetDay(TRef - min(dd, p_S_in))[H_TMIN];
							T_S_in_Days_stat[1] += weather.GetDay(TRef - min(dd, p_S_in))[H_TAIR];
							T_S_in_Days_stat[2] += weather.GetDay(TRef - min(dd, p_S_in))[H_TMAX];
						}
						if (i < p_S_out)
						{
							T_S_out_Days_stat[0] += weather.GetDay(TRef - min(dd, p_S_out))[H_TMIN];
							T_S_out_Days_stat[1] += weather.GetDay(TRef - min(dd, p_S_out))[H_TAIR];
							T_S_out_Days_stat[2] += weather.GetDay(TRef - min(dd, p_S_out))[H_TMAX];
						}

						if (i < p_St_in)
						{
							T_St_in_Days_stat[0] += weather.GetDay(TRef - min(dd, p_St_in))[H_TMIN];
							T_St_in_Days_stat[1] += weather.GetDay(TRef - min(dd, p_St_in))[H_TAIR];
							T_St_in_Days_stat[2] += weather.GetDay(TRef - min(dd, p_St_in))[H_TMAX];
						}
						if (i < p_St_out)
						{
							T_St_out_Days_stat[0] += weather.GetDay(TRef - min(dd, p_St_out))[H_TMIN];
							T_St_out_Days_stat[1] += weather.GetDay(TRef - min(dd, p_St_out))[H_TAIR];
							T_St_out_Days_stat[2] += weather.GetDay(TRef - min(dd, p_St_out))[H_TMAX];
						}


					}

					for (size_t i = 0; i < 3; i++)
					{
						input[dd].T_S_in_days[i] = T_S_in_Days_stat[i][MEAN];
						input[dd].T_S_out_days[i] = T_S_out_Days_stat[i][MEAN];
						input[dd].T_St_in_days[i] = T_St_in_Days_stat[i][MEAN];
						input[dd].T_St_out_days[i] = T_St_out_Days_stat[i][MEAN];
					}


				}
			}
		}




		double GFS = m_P[GFS0]; //mg/gDM
		double Starch = m_P[Starch0];//mg/gDM



		for (size_t yy = 1; yy < weather.GetNbYears(); yy++)
		{
			double CU = 0;
			double FU = 0;
			//double GFS = m_P[GFS0]; //mg/gDM
			//double Starch = m_P[Starch0];//mg/gDM



			for (size_t yyy = 0; yyy < 2; yyy++)
			{
				size_t y = yy + yyy - 1;
				size_t m1 = yyy == 0 ? SEPTEMBER : JANUARY;
				size_t m2 = yyy == 0 ? DECEMBER : AUGUST;
				size_t dd = 0;

				for (size_t m = m1; m <= m2; m++)
				{
					for (size_t d = 0; d < weather[y][m].GetNbDays(); d++, dd++)
					{
						CTRef TRef = weather[y][m][d].GetTRef();

						if (yyy == 1 || TRef.GetJDay() >= (m_P[To] - 1))
						{
							double T_S_in_days = input[TRef - pp.Begin()].T_S_in_days[S_IN_STAT];
							double T_S_out_days = input[TRef - pp.Begin()].T_S_out_days[S_OUT_STAT];
							double T_St_in_days = input[TRef - pp.Begin()].T_St_in_days[ST_IN_STAT];
							double T_St_out_days = input[TRef - pp.Begin()].T_St_out_days[ST_OUT_STAT];
							double T = weather[y][m][d][H_TNTX][MEAN];
							//double sRad = weather[y][m][d][H_SRMJ][SUM];

							if (METHOD == CHUINE_SEQUENTIAL_METHOD)
							{

								if (CU < m_P[CUcrit])
								{
									CU += max(0.0, min(m_P[Thigh] - T, m_P[Thigh] - m_P[Tlow]));
									CU = min(CU, m_P[CUcrit]);
								}
								else
								{
									FU += 1 / (1 + exp(-m_P[slp] * (T - m_P[T50])));
									FU = min(FU, m_P[FUcrit]);
								}

								double PS = CU / m_P[CUcrit] + FU / m_P[FUcrit];

								output[TRef][O_GFS_CONC] = 0;
								output[TRef][O_ST_CONC] = T;
								output[TRef][O_CU] = CU;
								output[TRef][O_FU] = FU;
								output[TRef][O_PS] = 0;
								output[TRef][O_WC] = 0;
								output[TRef][O_R] = 0;
								output[TRef][O_BUDBURST] = 0;// max(0.0, min(100.0, FU / m_P[FUcrit] * 100));//[%]
								output[TRef][O_SDI] = cdf(SDI_dist, FU / m_P[FUcrit]) * MAX_SDI;//0 .. 5;

							}
							else if (METHOD == CHUINE_ALTERNATING_METHOD)
							{
								if (dd < size_t(m_P[D0]))
								{
									CU += max(0.0, min(m_P[Thigh] - T, m_P[Thigh] - m_P[Tlow]));
								}
								else
								{
									if (dd == size_t(m_P[D0]))
										m_P[FUcrit] = max(1.0, m_P[a3] * exp(-m_P[b3] * CU));

									ASSERT(1 / (1 + exp(-m_P[slp] * (T - m_P[T50]))) >= 0);
									FU += 1 / (1 + exp(-m_P[slp] * (T - m_P[T50])));

									FU = min(FU, m_P[FUcrit]);
								}


								output[TRef][O_GFS_CONC] = 0;
								output[TRef][O_ST_CONC] = T;
								output[TRef][O_CU] = CU;
								output[TRef][O_FU] = FU;
								output[TRef][O_PS] = 0;
								output[TRef][O_WC] = 0;
								output[TRef][O_R] = 0;
								output[TRef][O_BUDBURST] = 0;// max(0.0, min(100.0, FU / m_P[FUcrit] * 100));//[%]
								output[TRef][O_SDI] = cdf(SDI_dist, FU / m_P[FUcrit]) * MAX_SDI;//0 .. 5;

							}
							else if (METHOD == CHARRIER_SUGAR_STARCH)
							{


								if (CU < m_P[CUcrit])
								{
									CU += max(0.0, min(m_P[Thigh] - T, m_P[Thigh] - m_P[Tlow]));
									CU = min(CU, m_P[CUcrit]);
								}
								else
								{
									FU += 1 / (1 + exp(-m_P[slp] * (T - m_P[T50])));
									FU = min(FU, m_P[FUcrit]);
								}
								

								ASSERT(CU >= 0 && CU <= m_P[CUcrit]);
								ASSERT(FU >= 0 && FU <= m_P[FUcrit]);


								double PS = CU / m_P[CUcrit] + FU / m_P[FUcrit];
								ASSERT(PS >= 0 && PS <= 2);

								
								double a1c = PS < 1 ? (m_P[k1c_trans] + m_P[dendo1c]) / 2 : (m_P[k1c_trans] + m_P[deco1c]) / 2;
								double b1c = PS < 1 ? (m_P[k1c_trans] - m_P[dendo1c]) / 2 : (m_P[k1c_trans] - m_P[deco1c]) / 2;
								double k1c = max(0.0, (a1c * PS + b1c) * exp(-Square(T - m_P[µ1c])) / (m_P[σ1c] * sqrt(2 * PI)));
								
								double a1m = PS < 1 ? (m_P[k1m_trans] + m_P[dendo1m]) / 2 : (m_P[k1m_trans] + m_P[deco1m]) / 2;
								double b1m = PS < 1 ? (m_P[k1m_trans] - m_P[dendo1m]) / 2 : (m_P[k1m_trans] - m_P[deco1m]) / 2;
								double k1m = max(0.0, (a1m * PS + b1m) * exp(-Square(T - m_P[µ1m])) / (m_P[σ1m] * sqrt(2 * PI)));
								
								double a2 = PS < 1 ? (m_P[k2_trans] + m_P[dendo2]) / 2 : (m_P[k2_trans] + m_P[deco2]) / 2;
								double b2 = PS < 1 ? (m_P[k2_trans] - m_P[dendo2]) / 2 : (m_P[k2_trans] - m_P[deco2]) / 2;
								double k2 = max(0.0, (a2 * PS + b2) * exp(-Square(T - m_P[µ2])) / (m_P[σ2] * sqrt(2 * PI)));
								
								double WC = GetWC(TRef, weather);
								static const double T_REF = 15.0;
								//double R = m_P[RMax] / (1 + exp(m_P[a3] * (WC - m_P[b3]))) * pow(m_P[Q10], (T - T_REF) / 10);
								double R = m_P[RMax] / (1 + exp(m_P[a3]) * (WC - m_P[b3])) * pow(m_P[Q10], (T - T_REF) / 10);//original? erreur???

								GFS += (k1c + k1m)  - k2 - R;
								Starch += k2  - (k1c + k1m);

								//GFS += (k1c + k1m) * Starch - k2 * GFS - R;
								//Starch += k2 * GFS/ Starch0 - (k1c + k1m) * Starch/ GFS0;


								GFS = max(0.0, min(150.0, GFS));
								Starch = max(0.0, min(150.0, Starch));


								ASSERT(GFS >= 0);
								ASSERT(Starch >= 0);


								output[TRef][O_GFS_CONC] = GFS;
								output[TRef][O_ST_CONC] = Starch;
								output[TRef][O_CU] = CU;
								output[TRef][O_FU] = FU;
								output[TRef][O_PS] = PS;
								output[TRef][O_WC] = 0;
								output[TRef][O_R] = 0;
								output[TRef][O_BUDBURST] = 0;//[%]
								output[TRef][O_SDI] = cdf(SDI_dist, max(0.0, min(1.0, PS - 1))) * MAX_SDI;//0 .. 6;


								output[TRef][O_SDI] = cdf(SDI_dist, max(0.0, min(1.0, PS - 1))) * 6;//0 .. 6;

							}
							else if (METHOD == SAINT_AMANT_SUGAR_STARCH)
							{


								if (CU < m_P[CUcrit])
								{
									CU += max(0.0, min(m_P[Thigh] - T, m_P[Thigh] - m_P[Tlow]));
									CU = min(CU, m_P[CUcrit]);
								}
								else if (FU < m_P[FUcrit])
								{
									FU += 1 / (1 + exp(-m_P[slp] * (T - m_P[T50])));
									//FU = min(FU, m_P[FUcrit]);
								}
								/*else
								{
									if (KU0 == -1)
									{
										KU0 = TRef.GetJDay();
										KUcrit = m_P[To] - KU0;
									}
									else
									{
										KU = TRef.GetJDay() - KU0;
									}
								}*/


								ASSERT(CU >= 0 && CU <= m_P[CUcrit]);
								ASSERT(FU >= 0 && FU <= m_P[FUcrit]);


								double PS = CU / m_P[CUcrit] + FU / m_P[FUcrit];

								ASSERT(PS >= 0 && PS <= 2);



								//double k1c = max(0.0, PS * m_P[dendo1c] + m_P[k1c_trans] )*exp(-Square(T_x_days - m_P[µ1c])) / (m_P[σ1c] * sqrt(2 * PI));
								//double k2 = max(0.0, PS* m_P[dendo2] + m_P[k2_trans] )*exp(-Square(T_x_days - m_P[µ2])) / (m_P[σ2] * sqrt(2 * PI));
								//double k1m = max(0.0, PS * m_P[dendo1m] + m_P[k1m_trans]) * exp(-Square(T_x_days - m_P[µ1m])) / (m_P[σ1m] * sqrt(2 * PI));
								//double k3= max(0.0, PS * m_P[deco2] + m_P[deco1m]) * exp(-Square(T_x_days - m_P[mu1])) / (m_P[ѕigma1] * sqrt(2 * PI));

								double psi_S_in = m_P[a3];// / (1 + exp(-(PS - m_P[dendo1m]) / m_P[k1m_trans]));
								//double Rate_S_in = exp(-Square(T_y_days - m_P[µ1m])) / (m_P[σ1m] * sqrt(2 * PI));
								double Rate_S_in = exp(-0.5*Square((T_S_in_days - m_P[µ1m])/ m_P[σ1m])) / (m_P[σ1m] * sqrt(2 * PI));
								//double Rate_S_in = exp(-(T_y_days - m_P[µ1m]) / m_P[σ1m]) / (m_P[σ1m] * Square(1 + exp(-(T_y_days - m_P[µ1m]) / m_P[σ1m])));
								double k_S_in = psi_S_in * Rate_S_in;

								double psi_S_out = m_P[b3];// / (1 + exp(-(PS - m_P[deco2]) / m_P[deco1m]));
								//double Rate_S_out = exp(-Square(T_y_days - m_P[mu1])) / (m_P[sigma1] * sqrt(2 * PI));
								double Rate_S_out = exp(-0.5*Square((T_S_out_days - m_P[mu1])/ m_P[sigma1])) / (m_P[sigma1] * sqrt(2 * PI));
								//double Rate_S_out = exp(-(T_y_days - m_P[mu1]) / m_P[sigma1]) / (m_P[sigma1] * Square(1 + exp(-(T - m_P[mu1]) / m_P[sigma1])));
								double k_S_out = psi_S_out * Rate_S_out;

								double psi_St_in = m_P[Q10];// / (1 + exp(-(PS - m_P[dendo1c]) / m_P[k1c_trans]));
								//double Rate_St_in = exp(-Square(T_x_days - m_P[µ1c])) / (m_P[σ1c] * sqrt(2 * PI));
								double Rate_St_in = exp(-0.5*Square((T_St_in_days - m_P[µ1c])/ m_P[σ1c])) / (m_P[σ1c] * sqrt(2 * PI));
								//double Rate_St_in = exp(-(T_x_days - m_P[µ1c]) / m_P[σ1c]) / (m_P[σ1c] * Square(1 + exp(-(T - m_P[µ1c]) / m_P[σ1c])));
								double k_St_in = psi_St_in * Rate_St_in;

								double psi_St_out = m_P[RMax] / (1 + exp(-(PS - m_P[dendo2]) / m_P[k2_trans]));
								//double Rate_St_out = exp(-Square(T_x_days - m_P[µ2])) / (m_P[σ2] * sqrt(2 * PI));
								double Rate_St_out = exp(-0.5*Square((T_St_out_days - m_P[µ2])/ m_P[σ2])) / (m_P[σ2] * sqrt(2 * PI));
								//double Rate_St_out = exp(-(T_x_days - m_P[µ2]) / m_P[σ2]) / (m_P[σ2] * Square(1 + exp(-(T - m_P[µ2]) / m_P[σ2])));
								double k_St_out = psi_St_out * Rate_St_out;


								double delta_S = k_S_in - k_S_out;
								GFS += delta_S;
								double delta_St = k_St_in - k_St_out;
								Starch -= delta_St;

								GFS = max(5.0, min(120.0, GFS));
								Starch = max(5.0, min(120.0, Starch));

								//double Smin = m_P[a3];
//double Smax = m_P[b3];
//double GFS_mean = Smax - (Smax - Smin) / (1 + exp(-(T_x_days - m_P[mu0]) / m_P[ѕigma0]));
//GFS = max(GFS_mean-10, min(GFS_mean+10, GFS));



								ASSERT(GFS > 0);
								ASSERT(Starch > 0);


								output[TRef][O_GFS_CONC] = GFS;
								output[TRef][O_ST_CONC] = Starch;
								output[TRef][O_CU] = CU;
								output[TRef][O_FU] = FU;
								output[TRef][O_PS] = PS;
								output[TRef][O_WC] = 0;
								output[TRef][O_R] = 0;
								output[TRef][O_BUDBURST] = 0;//[%]
								output[TRef][O_SDI] = cdf(SDI_dist, max(0.0, min(1.0, PS - 1))) * MAX_SDI;//0 .. 6;


								//output[TRef][O_SDI] = cdf(SDI_dist, max(0.0, min(1.0, PS - 1))) * 6;//0 .. 6;

							}
						}
					}
				}
			}
		}
	}



	enum { I_SPECIES1, I_SOURCE1, I_SITE1, I_DATE1, I_SDI1, I_N1, NB_INPUTS1 };
	enum { I_SPECIES2, I_SOURCE2, I_SITE2, I_DATE2, I_STARCH2, I_SUGAR2, I_SDI2, NB_INPUTS2 };
	void CBudBurstNewModel::AddDailyResult(const StringVector& header, const StringVector& data)
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
				obs.m_obs.push_back(stod(data[I_STARCH2]));
				obs.m_obs.push_back(stod(data[I_SUGAR2]));

				if (obs.m_obs[0] > -999)
				{
					m_years.insert(obs.m_ref.GetYear());
				}


				m_SAResult.push_back(obs);
			}
		}

	}



	static const int MAX_STAGE = 6;
	static const int ROUND_VAL = 4;
	void CBudBurstNewModel::CalibrateSDI(CStatisticXY& stat)
	{

		if (m_P[Tlow] >= m_P[Thigh])
			return;


		if (m_SAResult.empty())
			return;

		//if (!m_weather.IsHourly())
			//m_weather.ComputeHourlyVariables();



		//for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		//{
		//	int year = m_weather[y].GetTRef().GetYear();
		//	if (m_years.find(year) == m_years.end())
		//		continue;



		//	double sumDD = 0;
		//	vector<double> CDD;
		//	CTPeriod p;

		//
		//	p = m_weather[year].GetEntireTPeriod(CTM(CTM::DAILY));

		//	CDD.resize(p.size(), 0);
		//	CDegreeDays DDModel(CDegreeDays::MODIFIED_ALLEN_WAVE, m_SDI[Τᴴ¹], m_SDI[Τᴴ²]);

		//	for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
		//	{
		//		const CWeatherDay& wday = m_weather.GetDay(TRef);
		//		size_t ii = TRef - p.Begin();
		//		sumDD += DDModel.GetDD(wday);
		//		CDD[ii] = sumDD;
		//	}
		//



		//	for (size_t i = 0; i < m_SAResult.size(); i++)
		//	{
		//		size_t ii = m_SAResult[i].m_ref - p.Begin();
		//		if (m_SAResult[i].m_ref.GetYear() == year && ii < CDD.size())
		//		{
		//			double obs_y = m_SAResult[i].m_obs[0];

		//			if (obs_y > -999)
		//			{

		//				double sim_y = 0;

		//						
		//				//boost::math::logistic_distribution<double> SDI_dist(m_SDI[μ], m_SDI[ѕ]);
		//				boost::math::weibull_distribution<double> SDI_dist(m_SDI[μ], m_SDI[ѕ]);
		//				sim_y = Round(cdf(SDI_dist, CDD[ii]) * MAX_STAGE, ROUND_VAL);
		//				//double sim = m_SDI[ʎa] - m_SDI[ʎb] * cdf(begin_dist, sumDD), 0);



		//				if (sim_y < 0.05)
		//					sim_y = 0;
		//				if (sim_y > MAX_STAGE-0.05)
		//					sim_y = MAX_STAGE;

		//				stat.Add(obs_y, sim_y);
		//			}
		//		}
		//	}
		//}//for all years


	}



	void CBudBurstNewModel::GetFValueDaily(CStatisticXY& stat)
	{


		//return CalibrateSDI(stat);
		if (!m_SAResult.empty())
		{

			//low and hi relative development rate must be approximatively the same
			//if (!IsParamValid())
				//return;

			if (data_weather.GetNbYears() == 0)
			{
				CTPeriod pp(*m_years.begin(), JANUARY, DAY_01, *m_years.rbegin(), DECEMBER, DAY_31);
				pp = pp.Intersect(m_weather.GetEntireTPeriod(CTM::DAILY));

				((CLocation&)data_weather) = m_weather;
				//data_weather.SetHourly(false);
				data_weather.CreateYears(pp);

				for (int year = pp.GetFirstYear(); year <= pp.GetLastYear(); year++)
				{
					data_weather[year] = m_weather[year];
				}
			}

			CTPeriod pp(data_weather.GetEntireTPeriod(CTM::DAILY));
			CModelStatVector output(pp, NB_OUTPUTS, -999);

			ExecuteAllYears(data_weather, output);

			//verify S_conc and St_conc and return if invalid
			bool bS_Valid = true;
			bool bSt_Valid = true;

			for (size_t d = 0; d < output.size() && (bS_Valid || bSt_Valid); d++)
			{
				//&& S_invalid_factor >0&& St_invalid_factor>0
				if (output[d][O_S_CONC] > 2 * MAX_SUGAR)
				{
					bS_Valid = false;
				}
				else if (output[d][O_ST_CONC] > 2 * MAX_STRACH)
				{
					bSt_Valid = false;
				}

			}


			for (size_t i = 0; i < m_SAResult.size(); i++)
			{
				if (output.IsInside(m_SAResult[i].m_ref))
				{
					if (m_SAResult.front().m_obs.size() == 2)
					{
						if (m_SAResult[i].m_obs[0] > -999 && m_SAResult[i].m_ref.GetJDay() < 244 && output[m_SAResult[i].m_ref][O_SDI] > -999)
						{
							double obs_SDI = Round((m_SAResult[i].m_obs[0] - MIN_SDI) / (MAX_SDI - MIN_SDI), ROUND_VAL);
							double sim_SDI = Round((output[m_SAResult[i].m_ref][O_SDI] - MIN_SDI) / (MAX_SDI - MIN_SDI), ROUND_VAL);
							//for(size_t n=0; n< m_SAResult[i].m_obs[1]; n++)
							stat.Add(obs_SDI, sim_SDI);
						}
					}
					else
					{
						if (USE_SDI && m_SAResult[i].m_obs[0] > -999 && m_SAResult[i].m_ref.GetJDay() < 213 && output[m_SAResult[i].m_ref][O_SDI] > -999)
						{
							double obs_SDI = (m_SAResult[i].m_obs[0] - MIN_SDI) / (MAX_SDI - MIN_SDI);
							double sim_SDI = (output[m_SAResult[i].m_ref][O_SDI] - MIN_SDI) / (MAX_SDI - MIN_SDI);
							stat.Add(obs_SDI, sim_SDI);
						}

						if (USE_STARCH && m_SAResult[i].m_obs[1] > -999 && output[m_SAResult[i].m_ref][O_ST_CONC] > -999)
						{
							double obs_starch = (m_SAResult[i].m_obs[1] - MIN_STRACH) / (MAX_STRACH - MIN_STRACH);
							double sim_starch = (output[m_SAResult[i].m_ref][O_ST_CONC] - MIN_STRACH) / (MAX_STRACH - MIN_STRACH);
							if (!bSt_Valid)
								sim_starch *= sim_starch;
							stat.Add(obs_starch, sim_starch);
						}


						if (USE_SUGAR && m_SAResult[i].m_obs[2] > -999 && output[m_SAResult[i].m_ref][O_S_CONC] > -999)
						{
							double obs_GFS = (m_SAResult[i].m_obs[2] - MIN_SUGAR) / (MAX_SUGAR - MIN_SUGAR);
							double sim_GFS = (output[m_SAResult[i].m_ref][O_S_CONC] - MIN_SUGAR) / (MAX_SUGAR - MIN_SUGAR);

							if (!bSt_Valid)
								sim_GFS *= sim_GFS;

							stat.Add(obs_GFS, sim_GFS);
						}
					}
				}
			}//for all results
		//}
		}
	}






}