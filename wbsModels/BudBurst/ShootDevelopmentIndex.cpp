//*********************************************************************
//21/01/2020	1.0.0	Rémi Saint-Amant	Creation
//*********************************************************************
#include "ShootDevelopmentIndex.h"
#include "ModelBase/EntryPoint.h"
#include "ModelBase/SimulatedAnnealingVector.h"
#include "Basic/DegreeDays.h"
#include "Basic/NormalsDatabase.h"
#include <boost/math/distributions/weibull.hpp>
#include <boost/math/distributions/beta.hpp>
#include <boost/math/distributions/Rayleigh.hpp>
#include <boost/math/distributions/logistic.hpp>
#include <boost/math/distributions/exponential.hpp>

#include "Simulation/WeatherGenerator.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;

namespace WBSF
{

	static const double MIN_SDI = 0;
	static const double MAX_SDI_DHONT = 7;
	static const double MAX_SDI_AUGER = 5;
	static const double MIN_SDI_DOY = 0.25;
	static const double MAX_SDI_DHONT_DOY = 5.75;
	static const double MAX_SDI_AUGER_DOY = 4.75;
	static const size_t CU_STAT = H_TAIR;
	static const size_t FU_STAT = H_TAIR;


	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CShootDevelopmentIndexModel::CreateObject);

// O_CU, O_FU, O_PS, O_SDI_DHONT, 
	enum TOutput {O_F0, O_F1, O_F2, O_F3, O_F4, O_F5, O_SDI, NB_OUTPUTS };
	//enum TOutputA { O_A_BUDBURST, NB_OUTPUTS_A };

	double GetSimDOY(const CModelStatVector& output, size_t col, CTRef& TRef, double SDI)
	{
		double DOYsim = -999.0;
		CTPeriod p(TRef.GetYear(), JANUARY, DAY_01, TRef.GetYear(), DECEMBER, DAY_31);
		int pos = output.GetFirstIndex(col, ">", SDI, 1, p);
		if (pos > 0)
		{

			//BB = Round((output.GetFirstTRef() + pos).GetJDay() + (SDI - output[pos][col]) / (output[pos + 1][col] - output[pos][col]), 1);
			double f = (SDI - output[pos][col]) / (output[pos + 1][col] - output[pos][col]);
			DOYsim = Round(double((output.GetFirstTRef() + pos).GetJDay()) + (SDI - output[pos][col]) / (output[pos + 1][col] - output[pos][col]), 1);
		}

		return DOYsim;
	}

	/*double Get_PS_DOY(int year, double threshold, const CModelStatVector& output)
	{
		CTPeriod p(year, JANUARY, DAY_01, year, DECEMBER, DAY_31);
		int pos = output.GetFirstIndex(O_PS, ">", threshold, 0, p);
		return pos >= 0 ? double((output.GetFirstTRef() + pos).GetJDay()) : -999.0;
	}*/

	double Dhont2Auger(double SDI_Dhont, double p0, double p1, double p2, double p3)
	{
		//double SDI_Auger = max(MIN_SDI, min(MAX_SDI_AUGER, p0 + p1 * exp(-pow((MAX_SDI_DHONT - SDI_Dhont) / p2, p3))));
		//double SDI_Auger = (SDI_Dhont < 3.0) ? SDI_Dhont / 3.0 : SDI_Dhont - 2.0;
		double SDI_Auger = SDI_Dhont * 5 / 6;

		//double SDI_Auger = max(MIN_SDI, min(MAX_SDI_AUGER, -0.5 + 5.5 * exp(-pow((MAX_SDI_DHONT - SDI_Dhont) / 4.0, 1.6))));
		return SDI_Auger;
	}


	double CShootDevelopmentIndexModel::ChillingResponce(double T)const
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


		ASSERT(!_isnan(R) && _finite(R));

		return R;
	}

	double CShootDevelopmentIndexModel::ForcingResponce(double T)const
	{
		double R = 1 / (1 + exp(-(T - m_P[FU_µ]) / m_P[FU_σ]));
		ASSERT(!_isnan(R) && _finite(R) && R >= 0);

		return R;
	}







	CShootDevelopmentIndexModel::CShootDevelopmentIndexModel()
	{
		// initialize your variable here (optional)
		NB_INPUT_PARAMETER = -1;
		VERSION = "1.0.3 (2023)";

		m_species = 0;
		m_bCumulative = false;

		m_P = { CHUINE_SEQUENTIAL_METHOD, 244,3.1,26.7,2298.8,0.244,13.46,21.5,8.00,7.69,0.0997,0.0888,0.0795,19.71,2.59,-217.3,-257.6,0.650,10,3.77,0.0464,-0.0633,0.856,2.64 };
		m_CU_DAY_last = -1;
		m_FU_DAY_last = -1;

	}



	CShootDevelopmentIndexModel::~CShootDevelopmentIndexModel()
	{
	};


	//this method is call to load your parameter in your variable
	ERMsg CShootDevelopmentIndexModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		//transfer your parameter here
		size_t c = 0;
		m_species = parameters[c++].GetInt();
		m_bCumulative = parameters[c++].GetInt();
		m_defoliation = parameters[c++].GetReal();

		//ASSERT(m_species < HBB::PARAMETERS.size());


		if (parameters.size() == 3 + NB_PARAMS)
		{
			for (size_t i = 0; i < NB_PARAMS; i++)
				m_P[i] = parameters[c++].GetReal();
		}
		else
		{
			//m_SDI_type = SDI_AUGER;
			if (m_normals_filepath.empty())
			{
				static const array < array<double, 4>, 4> P1 =
				{ {
					{12.8, 5.5, 15.05, 1.78},//bf
					{11.4, 7.9, 23.34, 2.01},//ws
					{14.8, 9.8, 24.12, 2.98},//bs
					{08.0, 8.6, 30.56, 2.85},//ns
				} };


				
				m_P[FU_µ] = P1[m_species][0];
				m_P[FU_σ] = P1[m_species][1];
				m_P[SDI_µ] = P1[m_species][2];
				m_P[SDI_σ] = P1[m_species][3];
				m_P[CU_σ¹] = 0;
			}
			else
			{
				static const array < array<double, 5>, 4> P2 =
				{ {
					{12.8, 5.5, 15.05, 1.78, 0.00128},//bf
					{12.0, 8.5, 30.20, 1.89, 0.01460},//ws
					{12.0, 5.3, 16.64, 1.92, 0.02050},//bs
					{10.0, 9.6, 27.88, 2.59, 1e-8 },  //ns
				} };
				
				m_P[FU_µ] = P2[m_species][0];
				m_P[FU_σ] = P2[m_species][1];
				m_P[SDI_µ] = P2[m_species][2];
				m_P[SDI_σ] = P2[m_species][3];
				m_P[CU_σ¹] = P2[m_species][4];
			}

		}

		return msg;
	}





	//This method is call to compute solution
	ERMsg CShootDevelopmentIndexModel::OnExecuteDaily()
	{
		ERMsg msg;

		ExecuteAllYears(m_weather, m_output);


		return msg;
	}

	//ERMsg CShootDevelopmentIndexModel::OnExecuteAnnual()
	//{
	//	ERMsg msg;

	//	CTPeriod pp(m_weather.GetEntireTPeriod(CTM::ANNUAL));
	//	m_output.Init(pp, NB_OUTPUTS_A, -999);

	//	CModelStatVector output;
	//	ExecuteAllYears(m_weather, output);

	//	for (size_t y = 1; y < m_output.size(); y++)
	//	{
	//		//CSAResult tmp;
	//		//tmp.m_ref = m_output.GetFirstTRef()+y;
	//		//tmp.m_obs[0] = 4.0;

	//		double sim_BB = GetSimDOY(output, O_SDI_DHONT, m_output.GetFirstTRef() + y, 5.5);
	//		//int year = int(m_output.GetFirstTRef().GetYear() + y);
	//		//double sim_BB = Get_PS_DOY(year, 1.99, output) + 1;
	//		if (sim_BB > -999)
	//			m_output[y][O_A_BUDBURST] = sim_BB + 1;//+1 DOY in base one
	//	}

	//	return msg;
	//}



	double CShootDevelopmentIndexModel::Weibull(size_t stage, double  SDI, const array < double, 2>& p, size_t first_stage, size_t last_stage)
	{
		//scale, shape
		enum TParam { lambda, k };

		double Fx = -999;
		if (stage < last_stage)
			Fx = 1.0 - exp(-pow(p[lambda] * (SDI - first_stage), p[k]));
		else if (stage == last_stage)
			Fx = exp(-pow(p[lambda] * (last_stage - SDI), p[k]));

		return Fx;
	}

	array<double, 6> CShootDevelopmentIndexModel::SDI_2_Sx(double SDI, bool bCumul)
	{
		static const array< array< double, 2>, 5> P =
		{ {
			{1.561, 1.501},//F1
			{0.552, 3.022},//F2
			{0.372, 4.365},//F3
			{0.273, 6.994},//F4
			{1.444, 1.390},//F5
		} };


		double F1 = Weibull(1, SDI, P[0]);
		double F2 = Weibull(2, SDI, P[1]);
		double F3 = Weibull(3, SDI, P[2]);
		double F4 = Weibull(4, SDI, P[3]);
		double F5 = Weibull(5, SDI, P[4]);

		array<double, 6> Sx = { 0 };

		if (bCumul)
			Sx = { 1 - F1, F1, F2, F3, F4, F5 };
		else
			Sx = { 1 - F1, F1 - F2, F2 - F3, F3 - F4, F4 - F5, F5 };

		return Sx;
	}


	ERMsg CShootDevelopmentIndexModel::ExecuteAllYears(CWeatherYears& weather, CModelStatVector& output)
	{
		ERMsg msg;
		CTPeriod pp(weather.GetEntireTPeriod(CTM::DAILY));
		output.Init(pp, NB_OUTPUTS, -999);


		if (m_Tjan.empty()&&!m_normals_filepath.empty())
		{
			CNormalsDatabasePtr pDB(new CNormalsDatabase);
			msg = pDB->Open(m_normals_filepath);
			if (!msg)
				return msg;

			CWeatherGenerator WG;
			WG.SetNormalDB(pDB);


			WG.SetTarget(m_info.m_loc);
			msg = WG.Initialize();//create gradient

			if (!msg)
				return msg;

			CNormalsStation simStation;
			WG.GetNormals(simStation, CCallback::DEFAULT_CALLBACK);
			m_Tjan = simStation[JANUARY][NORMALS_DATA::TMIN_MN];

			//compute January mean temperature overall all available years
			//for (size_t y = 0; y < m_weather.size(); y++)
				//m_Tjan += m_weather[y][JANUARY].GetStat(H_TMIN);
		}

		double fRegion1 = m_Tjan.empty()?1.0:min(1.0, exp(m_Tjan[MEAN] * m_P[CU_σ¹]));
		boost::math::logistic_distribution<double> SDI_dist(m_P[SDI_µ] * fRegion1, m_P[SDI_σ]);



		/*
		CDegreeDays DDmodel(CDegreeDays::ALLEN_WAVE, CU_Tlow, CU_Thigh);
		CModelStatVector DD;
		DDmodel.Execute(weather, DD);*/


		//size_t p_CU = size_t(size_t(m_P[FU_DAYS]) * m_P[CU_DAYS]);
		//size_t p_CU = size_t(m_P[CU_DAYS]);
		//size_t p_FU = size_t(m_P[FU_DAYS]);
		//if (mean_T_day.empty() || p_CU != m_CU_DAY_last || p_FU != m_FU_DAY_last)
		//{
		//	m_CU_DAY_last = p_CU;
		//	m_FU_DAY_last = p_FU;



		//	mean_T_day.resize(pp.GetNbDay());

		//	for (size_t y = 0, dd = 0; y < weather.GetNbYears(); y++)
		//	{
		//		for (size_t m = 0; m < weather[y].GetNbMonth(); m++)
		//		{
		//			for (size_t d = 0; d < weather[y][m].GetNbDays(); d++, dd++)
		//			{
		//				array<CStatistic, 3> T_CU_Days_stat;
		//				array<CStatistic, 3> T_FU_Days_stat;


		//				size_t max_days = max(p_CU, p_FU);


		//				CTRef TRef = weather[y][m][d].GetTRef();
		//				for (size_t i = 0; i < max_days; i++)//Charrier 2018 use the mean maximum of the last 14 days 
		//				{

		//					if (i < p_CU)
		//					{
		//						T_CU_Days_stat[0] += weather.GetDay(TRef - min(dd, p_CU))[H_TMIN];
		//						T_CU_Days_stat[1] += weather.GetDay(TRef - min(dd, p_CU))[H_TAIR];
		//						T_CU_Days_stat[2] += weather.GetDay(TRef - min(dd, p_CU))[H_TMAX];
		//					}
		//					if (i < p_FU)
		//					{
		//						T_FU_Days_stat[0] += weather.GetDay(TRef - min(dd, p_FU))[H_TMIN];
		//						T_FU_Days_stat[1] += weather.GetDay(TRef - min(dd, p_FU))[H_TAIR];
		//						T_FU_Days_stat[2] += weather.GetDay(TRef - min(dd, p_FU))[H_TMAX];
		//					}
		//				}

		//				for (size_t i = 0; i < 3; i++)
		//				{
		//					mean_T_day[dd].T_CU_days[i] = T_CU_Days_stat[i][MEAN];
		//					mean_T_day[dd].T_FU_days[i] = T_FU_Days_stat[i][MEAN];
		//				}
		//			}
		//		}
		//	}
		//}

		//boost::math::weibull_distribution<double> FU_dist(m_P[FU_µ], m_P[FU_σ]);
		//for (size_t yy = 1; yy < weather.GetNbYears(); yy++)
		for (size_t yy = 0; yy < weather.GetNbYears(); yy++)
		{
			double CU = 0;
			double FU = 0;
			double CDD = 0;

			if (TMethod(m_P[METHOD]) == CHUINE_ALTERNATING_METHOD)
				m_P[FU_crit] = -1;

			//for (size_t yyy = 0; yyy < 2; yyy++)
			{
				//size_t y = yy + yyy - 1;
				//size_t m1 = yyy == 0 ? AUGUST : JANUARY;
				//size_t m2 = yyy == 0 ? DECEMBER : JULY;
				size_t y = yy;
				size_t m1 = JANUARY;
				size_t m2 = DECEMBER;
				size_t dd = 0;

				for (size_t m = m1; m <= m2; m++)
				{
					for (size_t d = 0; d < weather[y][m].GetNbDays(); d++, dd++)
					{
						CTRef TRef = weather[y][m][d].GetTRef();


						if (/*yyy == 1 ||*/ TRef.GetJDay() >= (m_P[To] - 1))
						{
							//CDD += DD[TRef][0];
							//double T_CU = mean_T_day[TRef - pp.Begin()].T_CU_days[CU_STAT];
							//double T_FU = mean_T_day[TRef - pp.Begin()].T_FU_days[FU_STAT];
							double T_FU = weather.GetDay(TRef)[H_TNTX];


							/*if (CU < m_P[CU_crit])
							{
								CU += ChillingResponce(T_CU);
								CU = min(CU, m_P[CU_crit]);
							}
							else
							{
								if (TMethod(m_P[METHOD]) == CHUINE_ALTERNATING_METHOD && m_P[FU_crit] == -1)
								{
									m_P[FU_crit] = max(1.0, m_P[FUw] * exp(-m_P[FUz] * CU));
								}*/

							FU += ForcingResponce(T_FU);


							//FU += cdf(FU_dist, T_FU);
							//FU = min(FU, m_P[FU_crit]);
							//}
						}



						//double fRegion = m_species==2? 1.0 / (1.0 + exp(-(m_Tjan[TRef.GetYear()][MEAN] - m_P[CU_µ]) / m_P[CU_σ¹])):1;
						//double fRegion = 1;

						//double fRegion = exp(m_P[CU_σ²] * (1.0 / (1.0 + exp(-(m_Tjan[MEAN] - m_P[CU_µ]) / m_P[CU_σ¹])) - 0.5));
						//double fRegion = exp(1.0 / (1.0 + exp(-(m_Tjan[MEAN] - m_P[CU_µ]) / m_P[CU_σ¹])) - 0.5);
						//double PS = CU / m_P[CU_crit] + FU / m_P[FU_crit];
						//double SDI = cdf(SDI_dist, max(0.0, min(1.0, PS - 1))) * MAX_SDI_DHONT;
						//double PS = max(0.0, min(1.0, FU / m_P[FU_crit]));
						//double PS = max(0.0, min(1.0, FU / m_P[FU_crit]));


						//double SDI = cdf(SDI_dist, CDD/fRegion) * 5;
						double SDI = cdf(SDI_dist, FU) * 5;

						//output[TRef][O_CU] = CDD;
						//output[TRef][O_FU] = FU;
						//output[TRef][O_PS] = PS;
						//output[TRef][O_F5] = SDI_2_Sx(m_SDI_type, SDI, 5);

						array<double, 6> Sx = SDI_2_Sx(SDI, m_bCumulative);
						output[TRef][O_F5] = Round(Sx[5]*100,1);
						output[TRef][O_F4] = Round(Sx[4]*100,1);
						output[TRef][O_F3] = Round(Sx[3]*100,1);
						output[TRef][O_F2] = Round(Sx[2]*100,1);
						output[TRef][O_F1] = Round(Sx[1]*100,1);
						output[TRef][O_F0] = Round(Sx[0]*100,1);
						//output[TRef][O_SDI_DHONT] = SDI * 6 / 5;
						output[TRef][O_SDI] = SDI;// Dhont2Auger(SDI, m_P[CU_µ_T], m_P[CU_σ_T], m_P[CU_µ_PS], m_P[CU_σ_PS]);



					}
				}
			}
		}

		return msg;
	}

	//simple DD model
	//void CShootDevelopmentIndexModel::ExecuteAllYears(CWeatherYears& weather, CModelStatVector& output)
	//{
	//	//boost::math::weibull_distribution<double> SDI_dist(m_P[SDI_µ], m_P[SDI_σ]);
	//
	//	CTPeriod pp(weather.GetEntireTPeriod(CTM::DAILY));
	//	output.Init(pp, NB_OUTPUTS, -999);
	//
	//	if (m_P[CU_µ]>= m_P[FU_µ])
	//		return;
	//
	//	CDegreeDays CDD(CDegreeDays::DOUBLE_SINE, m_P[CU_µ], m_P[FU_µ]);
	//	CModelStatVector DD;
	//	
	//	CDD.Execute(weather, DD);
	//	for (size_t y = 0; y < weather.GetNbYears(); y++)
	//	{
	//		double FU =0;
	//		CJDayRef begin(weather[y].GetTRef().GetYear(), m_P[To] - 1);
	//		CTRef end = weather[y].GetEntireTPeriod().End();
	//		for (CTRef TRef= begin; TRef <= end; TRef++)
	//		{
	//			FU += DD[TRef][0];
	//			FU = min(FU, m_P[FU_crit]);
	//
	//			double PS = FU / m_P[FU_crit];
	//			output[TRef][O_FU] = FU;
	//			output[TRef][O_PS] = PS;
	//			output[TRef][O_SDI] = PS*4.51;
	//		}
	//	}
	//}


	enum { I_SPECIES1, I_SOURCE1, I_SITE1, I_LATITUDE1, I_LONGITUDE1, I_ELEVATION1, I_DATE1, I_STARCH1, I_SUGAR1, I_B_LENGTH1, I_B_MASS1, I_N_MASS1, I_SDI1, I_N1, I_DEF1, I_DEF_END_N11, I_DEF_END_N1, I_PROVINCE1, I_TYPE1, NB_INPUTS1 };
	enum { I_SPECIES2, I_SOURCE2, I_SITE2, I_LATITUDE2, I_LONGITUDE2, I_ELEVATION2, I_YEAR2, I_BUDBURST2, I_PROVINCE2, I_TYPE2, NB_INPUTS2 };
	enum { I_SPECIES3, I_SOURCE3, I_SITE3, I_LATITUDE3, I_LONGITUDE3, I_ELEVATION3, I_DATE3, I_SDI_DHONT3, I_SDI_AUGER3, I_N3, I_TYPE3, I_PROVINCE3, I_SDI3, NB_INPUTS3 };
	enum { I_SPECIES4, I_SOURCE4, I_SITE4, I_LATITUDE4, I_LONGITUDE4, I_ELEVATION4, I_TYPE4, I_PROVINCE4, I_YEAR4, I_DOY4, I_BB04, I_BB14, NB_INPUTS4 };
	enum { I_SPECIES5, I_SOURCE5, I_SITE5, I_LATITUDE5, I_LONGITUDE5, I_ELEVATION5, I_TYPE5, I_PROVINCE5, I_YEAR5, I_DOY5, I_SDI5, I_n5, I_S0_5, I_S1_5, I_S2_5, I_S3_5, I_S4_5, I_S5_5, I_S6_5, I_SDI_Type5, I_SDI_DA5, NB_INPUTS5 };
	void CShootDevelopmentIndexModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		static const char* SPECIES_NAME[] = { "bf", "ws", "bs", "ns", "rs", "rbs" };
		if (data.size() == NB_INPUTS1)
		{
			m_inputType = INPUT1;
			if (data[I_SPECIES1] == SPECIES_NAME[m_species] && data[I_TYPE1] == "C")
			{

				CSAResult obs;

				obs.m_ref.FromFormatedString(data[I_DATE1]);
				obs.m_obs[0] = stod(data[I_SDI1]);


				if (obs.m_obs[0] > -999)
				{
					if (obs.m_ref.GetJDay() < 243)
						m_years.insert(obs.m_ref.GetYear());
					else
						m_years.insert(obs.m_ref.GetYear() + 1);

				}

				m_SAResult.push_back(obs);
			}
		}
		else if (data.size() == NB_INPUTS2)
		{
			m_inputType = INPUT2;
			if (data[I_SPECIES2] == SPECIES_NAME[m_species] && data[I_TYPE2] == "C")
			{

				CSAResult obs;

				obs.m_ref = CTRef(stoi(data[I_YEAR2]));
				obs.m_obs[0] = stod(data[I_BUDBURST2]);//DOY one base


				if (obs.m_obs[0] > -999)
				{
					m_years.insert(obs.m_ref.GetYear());
				}

				m_SAResult.push_back(obs);
			}
		}
		else if (data.size() == NB_INPUTS3)
		{
			m_inputType = INPUT3;
			if (data[I_SPECIES3] == SPECIES_NAME[m_species] && data[I_TYPE3] == "C")
			{

				CSAResult obs;

				obs.m_ref.FromFormatedString(data[I_DATE3]);
				obs.m_obs[0] = stod(data[I_SDI_DHONT3]);
				obs.m_obs.push_back(stod(data[I_SDI_AUGER3]));
				obs.m_obs.push_back(stod(data[I_N3]));

				m_years.insert(obs.m_ref.GetYear());
				m_SAResult.push_back(obs);
			}
		}
		else if (data.size() == NB_INPUTS4)
		{
			m_inputType = INPUT4;
			if (data[I_SPECIES4] == SPECIES_NAME[m_species] && data[I_TYPE4] == "C")
			{

				CSAResult obs;

				obs.m_ref = CJDayRef(stoi(data[I_YEAR4]), stoi(data[I_DOY4]) - 1);
				//#obs.m_obs[0] = stod(data[I_BB14]) / (stod(data[I_BB04]) + stod(data[I_BB14]));
				obs.m_obs[0] = stod(data[I_BB04]);
				obs.m_obs.push_back(stod(data[I_BB14]));

				m_years.insert(obs.m_ref.GetYear());
				m_SAResult.push_back(obs);
			}
		}
		else if (data.size() == NB_INPUTS5)
		{
			m_inputType = INPUT5;
			if (data[I_SPECIES5] == SPECIES_NAME[m_species] && data[I_TYPE5] == "C")
			{

				CSAResult obs;

				obs.m_ref = CJDayRef(stoi(data[I_YEAR5]), stoi(data[I_DOY5]) - 1);
				//#obs.m_obs[0] = stod(data[I_BB14]) / (stod(data[I_BB04]) + stod(data[I_BB14]));
				obs.m_obs[0] = stod(data[I_SDI5]);
				obs.m_obs.push_back(stod(data[I_n5]));

				m_years.insert(obs.m_ref.GetYear());
				m_SAResult.push_back(obs);
			}
		}
	}



	bool CShootDevelopmentIndexModel::GetFValueDaily(CStatisticXY& stat)
	{

		if (!m_SAResult.empty())
		{

			if (TRFunction(m_P[R_FUNCTION]) == UTAH)
			{
				if ((m_P[CU_T_MIN] > m_P[CU_T_OPT]) ||
					(m_P[CU_T_OPT] > m_P[CU_T_MAX]))
					return false;
			}

			if (TRFunction(m_P[R_FUNCTION]) == RICHARDSON)
			{
				if (m_P[CU_Tlow] > m_P[CU_Thigh])
					return false;
			}

			if (m_P[CU_Tlow] > m_P[CU_Thigh])
				return false;

			/*if (Dhont2Auger(0.0, m_P[CU_µ_T], m_P[CU_σ_T], m_P[CU_µ_PS], m_P[CU_σ_PS]) > 0)
				return false;

			if (Dhont2Auger(5.0, m_P[CU_µ_T], m_P[CU_σ_T], m_P[CU_µ_PS], m_P[CU_σ_PS]) > 4)
				return false;

			if (Dhont2Auger(6.0, m_P[CU_µ_T], m_P[CU_σ_T], m_P[CU_µ_PS], m_P[CU_σ_PS]) < 4)
				return false;

			if (Dhont2Auger(7.0, m_P[CU_µ_T], m_P[CU_σ_T], m_P[CU_µ_PS], m_P[CU_σ_PS]) < 5)
				return false;*/

				//boost::math::beta_distribution<double> SDI_dist(m_P[SDI_µ], m_P[SDI_σ]);
				//if( cdf(SDI_dist, 0.0) );

				//if (size_t(size_t(m_P[FU_DAYS]) * m_P[CU_DAYS]) > 45)
					//return;


			if (!m_SDI_DOY_stat.IsInit())
			{
#pragma omp critical  
				{
					const CSimulatedAnnealingVector& all_results = GetSimulatedAnnealingVector();

					for (auto it = all_results.begin(); it != all_results.end(); it++)
					{
						const CSAResultVector& results = (*it)->GetSAResult();
						for (auto iit = results.begin(); iit != results.end(); iit++)
						{
							if (m_inputType == INPUT3)
							{
								if (iit->m_obs[0] >= MIN_SDI_DOY && iit->m_obs[0] <= MAX_SDI_DHONT_DOY)
									m_SDI_DOY_stat += iit->m_ref.GetJDay();
								if (iit->m_obs[1] >= MIN_SDI_DOY && iit->m_obs[1] <= MAX_SDI_AUGER_DOY)
									m_SDI_DOY_stat += iit->m_ref.GetJDay();
							}
							else if (m_inputType == INPUT5)
							{
								if (iit->m_obs[0] >= MIN_SDI_DOY && iit->m_obs[0] <= MAX_SDI_AUGER_DOY)
									m_SDI_DOY_stat += iit->m_ref.GetJDay();
							}
						}
					}

					//if (m_inputType == INPUT5 && m_X.empty())
					//{
					//	std::deque<pair<double, size_t>> D;

					//	double sum = 0;
					//	for (auto it = all_results.begin(); it != all_results.end(); it++)
					//	{
					//		const CSAResultVector& results = (*it)->GetSAResult();
					//		for (auto iit = results.begin(); iit != results.end(); iit++)
					//		{
					//			sum += iit->m_obs[1];
					//			D.push_back(make_pair(iit->m_obs[0] / 5, iit->m_ref.GetJDay()));
					//		}
					//	}
					//	ASSERT(sum == size_t(sum)); //integer


					//	sort(D.begin(), D.end());



					//	for (size_t i = D.size() - 1; i > 0; i--)
					//		D[i].first = Round((D[i].first - D[i - 1].first) * sum);

					//	D[0].first = Round(D[0].first * sum);


					//	//m_X.resize(size_t(sum));
					//	for (size_t i = 0; i < D.size(); i++)
					//	{
					//		for (size_t ii = 0; ii < D[i].first; ii++)
					//			m_X.push_back(D[i].second);
					//	}

					//}
				}
			}


			if (data_weather.GetNbYears() == 0)
			{
#pragma omp critical  
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
						return false;
					}



				}
			}

			CModelStatVector output;
			ExecuteAllYears(data_weather, output);

			//boost::math::beta_distribution<double> SDI_dist(m_P[muSDI], m_P[ѕigmaSDI]);
			//boost::math::weibull_distribution<double> SDI_dist(m_P[SDI_µ], m_P[SDI_σ]);
			double NLL = 0;
			for (size_t i = 0; i < m_SAResult.size(); i++)
			{

				if (m_inputType == INPUT1)
				{
					//if (output.IsInside(m_SAResult[i].m_ref))
					//{
					//	if (m_SAResult[i].m_obs[0] > -999 && m_SAResult[i].m_ref.GetJDay() < 244 && output[m_SAResult[i].m_ref][O_SDI_DHONT] > -999)
					//	{
					//		double obs_SDI = m_SAResult[i].m_obs[0];
					//		double sim_SDI_Dhont = output[m_SAResult[i].m_ref][O_SDI_DHONT];
					//		double sim_SDI = Dhont2Auger(sim_SDI_Dhont, m_P[CU_µ_T], m_P[CU_σ_T], m_P[CU_µ_PS], m_P[CU_σ_PS]);


					//		stat.Add(obs_SDI / MAX_SDI_AUGER, sim_SDI / MAX_SDI_AUGER);

					//		if (obs_SDI >= MIN_SDI_DOY && obs_SDI <= MAX_SDI_AUGER_DOY)
					//		{
					//			double DOY = GetSimDOY(output, O_SDI_DHONT, m_SAResult[i].m_ref, m_SAResult[i].m_obs[0]);
					//			if (DOY > -999)
					//			{
					//				double obs_DOY = (m_SAResult[i].m_ref.GetJDay() - m_SDI_DOY_stat[LOWEST]) / m_SDI_DOY_stat[RANGE];
					//				double sim_DOY = (DOY - m_SDI_DOY_stat[LOWEST]) / m_SDI_DOY_stat[RANGE];
					//				stat.Add(obs_DOY, sim_DOY);
					//			}
					//			else
					//			{
					//				stat.clear();
					//				return false;//reject this solution
					//			}
					//		}


					//	}
					//}
				}
				else if (m_inputType == INPUT2)
				{
					//ASSERT(output.IsInside(m_SAResult[i].m_ref.as(CTM::DAILY, CTRef::MID_TREF)));
					//ASSERT(m_SAResult[i].m_obs[0] > -999);

					////CSAResult tmp;
					////tmp.m_ref = m_SAResult[i].m_ref;
					////tmp.m_obs[0] = 4.0;

					////int year = m_SAResult[i].m_ref.GetYear();
					////double sim_BB = Get_PS_DOY(year, 1.99, output) + 1;//+1 DOY in base one

					//double sim_BB = GetSimDOY(output, O_SDI_DHONT, m_SAResult[i].m_ref, 6.0);
					//double obs_BB = m_SAResult[i].m_obs[0];


					//if (sim_BB >= 0)
					//{
					//	stat.Add(obs_BB, sim_BB + 1);
					//}
					//else
					//{
					//	stat.clear();
					//	return false;//reject this solution
					//}

				}
				else if (m_inputType == INPUT3)
				{
					//if (output.IsInside(m_SAResult[i].m_ref))
					//{
					//	if (m_SAResult[i].m_ref.GetJDay() < 244 && output[m_SAResult[i].m_ref][O_SDI_DHONT] > -999)
					//	{
					//		size_t N = m_SAResult[i].m_obs[2];

					//		if (m_SAResult[i].m_obs[0] > -999)
					//		{
					//			double obs_SDI_Dhont = m_SAResult[i].m_obs[0];
					//			double sim_SDI_Dhont = output[m_SAResult[i].m_ref][O_SDI_DHONT];
					//			if (!isnan(sim_SDI_Dhont) && sim_SDI_Dhont > -999)
					//			{
					//				for (size_t n = 0; n < N; n++)
					//					stat.Add(min(6.0, obs_SDI_Dhont) / (MAX_SDI_DHONT - 1), min(6.0, sim_SDI_Dhont) / (MAX_SDI_DHONT - 1));
					//			}
					//			else
					//			{
					//				stat.clear();
					//				return false;//reject this solution
					//			}



					//			//double obs_SDI = Dhont2Auger(obs_SDI_Dhont, m_P[CU_µ_T], m_P[CU_σ_T], m_P[CU_µ_PS], m_P[CU_σ_PS]);
					//			//double sim_SDI = Dhont2Auger(sim_SDI_Dhont, m_P[CU_µ_T], m_P[CU_σ_T], m_P[CU_µ_PS], m_P[CU_σ_PS]);
					//			//stat.Add(obs_SDI / MAX_SDI_AUGER, sim_SDI / MAX_SDI_AUGER);

					//			if (obs_SDI_Dhont >= MIN_SDI_DOY && obs_SDI_Dhont <= MAX_SDI_DHONT_DOY)
					//			{
					//				double DOY = GetSimDOY(output, O_SDI_DHONT, m_SAResult[i].m_ref, obs_SDI_Dhont);
					//				if (DOY > -999)
					//				{
					//					double obs_DOY = (m_SAResult[i].m_ref.GetJDay() - m_SDI_DOY_stat[LOWEST]) / m_SDI_DOY_stat[RANGE];
					//					double sim_DOY = (DOY - m_SDI_DOY_stat[LOWEST]) / m_SDI_DOY_stat[RANGE];
					//					for (size_t n = 0; n < N; n++)
					//						stat.Add(obs_DOY, sim_DOY);
					//				}
					//				else
					//				{
					//					stat.clear();
					//					return false;//reject this solution 
					//				}
					//			}
					//		}

					//		if (m_SAResult[i].m_obs[1] > -999)
					//		{
					//			double obs_SDI_Auger = m_SAResult[i].m_obs[1];
					//			double sim_SDI_Auger = output[m_SAResult[i].m_ref][O_SDI_AUGER];
					//			if (!isnan(sim_SDI_Auger) && sim_SDI_Auger > -999)
					//			{
					//				for (size_t n = 0; n < N; n++)
					//					stat.Add(obs_SDI_Auger / MAX_SDI_AUGER, sim_SDI_Auger / MAX_SDI_AUGER);
					//			}
					//			else
					//			{
					//				stat.clear();
					//				return false;//reject this solution
					//			}



					//			if (obs_SDI_Auger >= MIN_SDI_DOY && obs_SDI_Auger <= MAX_SDI_AUGER_DOY)
					//			{
					//				double DOY = GetSimDOY(output, O_SDI_AUGER, m_SAResult[i].m_ref, obs_SDI_Auger);
					//				if (DOY > -999)
					//				{
					//					double obs_DOY = (m_SAResult[i].m_ref.GetJDay() - m_SDI_DOY_stat[LOWEST]) / m_SDI_DOY_stat[RANGE];
					//					double sim_DOY = (DOY - m_SDI_DOY_stat[LOWEST]) / m_SDI_DOY_stat[RANGE];

					//					for (size_t n = 0; n < N; n++)
					//						stat.Add(obs_DOY, sim_DOY);
					//				}
					//				else
					//				{
					//					stat.clear();
					//					return false;//reject this solution
					//				}
					//			}
					//		}
					//	}
					//}
				}
				else if (m_inputType == INPUT4)
				{

					//ASSERT(output.IsInside(m_SAResult[i].m_ref));

					////double obs_BB = m_SAResult[i].m_obs[0];
					////double sim_BB = output[m_SAResult[i].m_ref][O_PS];
					////stat.Add(obs_BB, sim_BB);

					//double p = output[m_SAResult[i].m_ref][O_PS];
					//ASSERT(p >= 0 && p <= 1);

					//double obs_BB0 = m_SAResult[i].m_obs[0];
					//double b0 = -obs_BB0 * log(max(DBL_MIN, (1 - p)));
					//double obs_BB1 = m_SAResult[i].m_obs[1];
					//double b1 = -obs_BB1 * log(max(DBL_MIN, p));
					//NLL += b0 + b1;



				}
				else if (m_inputType == INPUT5)
				{
					if (output.IsInside(m_SAResult[i].m_ref))
					{

						size_t N = m_SAResult[i].m_obs[1];


						double obs_SDI_Auger = m_SAResult[i].m_obs[0];
						double sim_SDI_Auger = output[m_SAResult[i].m_ref][O_SDI];
						if (!isnan(sim_SDI_Auger) && sim_SDI_Auger > -999)
						{
							//for (size_t n = 0; n < N; n++)
							stat.Add(obs_SDI_Auger / 5.0, sim_SDI_Auger / 5.0);
						}
						else
						{
							stat.clear();
							return false;//reject this solution
						}

						if (obs_SDI_Auger >= MIN_SDI_DOY && obs_SDI_Auger <= MAX_SDI_AUGER_DOY)
						{
							double SDI_DOY = GetSimDOY(output, O_SDI, m_SAResult[i].m_ref, obs_SDI_Auger);
							if (SDI_DOY > -999)
							{
								double obs_DOY = (m_SAResult[i].m_ref.GetJDay() - m_SDI_DOY_stat[LOWEST]) / m_SDI_DOY_stat[RANGE];
								double sim_DOY = (SDI_DOY - m_SDI_DOY_stat[LOWEST]) / m_SDI_DOY_stat[RANGE];
								//for (size_t n = 0; n < N; n++)
								stat.Add(obs_DOY, sim_DOY);
							}
							else
							{
								stat.clear();
								return false;//reject this solution 
							}
						}

					}
				}

			}//for all results

			if (m_inputType == INPUT4)
			{
				if (isnan(NLL) || !isfinite(NLL))
					return false;

				stat.Add(0, NLL);
			}
		}

		return true;
	}








}