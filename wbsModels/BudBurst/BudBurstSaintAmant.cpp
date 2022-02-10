//*********************************************************************
//21/01/2020	1.0.0	Rémi Saint-Amant	Creation
//*********************************************************************
#include "BudBurstSaintAmant.h"
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

	enum TOutput { O_S_CONC, O_ST_CONC, O_CU, O_FU, O_PS, O_SDI, NB_OUTPUTS };


	static const bool USE_SDI = true;
	static const bool USE_STARCH = true;
	static const bool USE_SUGAR = true;

	size_t CU_STAT = H_TAIR;
	size_t FU_STAT = H_TAIR;
	size_t S_IN_STAT = H_TMIN;
	size_t S_OUT_STAT = H_TMAX;
	size_t ST_IN_STAT = H_TMIN;
	size_t ST_OUT_STAT = H_TMAX;


	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CBudBurstSaintAmantModel::CreateObject);





	static const double MIN_SDI = 0;
	static const double MAX_SDI = 5;
	static const double MIN_STRACH = 5;//Deslauriers data 
	static const double MAX_STRACH = 70;//Deslauriers data 
	static const double MIN_SUGAR = 7;//Deslauriers data 
	static const double MAX_SUGAR = 85;//Schaberg


	const std::array < std::array<double, NB_PARAMS>, NB_HOST_SPECIES> CBudBurstSaintAmantModel::P =
	{ {
		{244,-10.8123,29.7523,3594.3427,0.2155,11.5025,91.575,21,20.0743,2.0793,8.1645,7,73.2509,10.6854,3.6068,8.7509,28,72.6097,10.3658,9.8962,28,80.0256,1.6606,0.7881,7.1179,4.433,27.1071,3.3205,0.1719,0.8734,0.1079,1.4943,-0.1426,0.6752,-1.2756,1},
		{244,10.4811,26.2659,3046.057,0.0912,39.7753,5.18,14,63.4546,-28.5214,1,7,46.3503,12.5385,4.2104,86.3254,28,52.2646,16.1682,15,28,78.5641,0.0936,2.1094,2.2919,12.1458,19.8303,3.7505,0.5726,1.1058,0.1014,1.4713,0.1956,1.2882,-0.1,1},
		{244,-3.3462,28.4546,2985.5869,0.3514,8.5807,88.2506,14,83.4974,10.9287,11.0639,7,54.478,7.7965,1.7549,17.4918,28,104.1436,7.5859,4.5074,28,90.3712,1.9024,2.8107,6.0349,1.6314,26.8442,2.459,0.3127,0.8861,0.1003,0.7392,0.2213,1.3714,-0.4584,1},
		{244,0.7084,26.8622,1367.4532,0.3492,14.507,54.9488,21,92.8352,22.991,9.1517,7,96.8089,13.5009,5.5921,38.1528,28,37.0826,1.5321,3.1383,28,85.6357,1.8027,0.6707,7.4595,2.8543,10.3072,2.0302,0.1043,1.674,0.4683,1.7506,-1.468,1.1904,-0.8132,1},
		{0},

	} };

	CBudBurstSaintAmantModel::CBudBurstSaintAmantModel()
	{
		// initialize your variable here (optional)
		NB_INPUT_PARAMETER = -1;
		VERSION = "1.0.0 (2021)";
		m_P = { 244,3.1,26.7,2298.8,0.244,13.46,21.5,8.00,7.69,0.0997,0.0888,0.0795,19.71,2.59,-217.3,-257.6,0.650,10,3.77,0.0464,-0.0633,0.856,2.64,-165.5,0.924,0.693 };

		m_last_p_S_in = -1;
		m_last_p_S_out = -1;
		m_last_p_St_in = -1;
		m_last_p_St_out = -1;

	}



	CBudBurstSaintAmantModel::~CBudBurstSaintAmantModel()
	{
	};


	//this method is call to load your parameter in your variable
	ERMsg CBudBurstSaintAmantModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		//transfer your parameter here
		size_t c = 0;
		m_species = parameters[c++].GetInt();

		//ASSERT(m_species < HBB::PARAMETERS.size());


		if (parameters.size() == 1 + NB_PARAMS + 1)
		{
			for (size_t i = 0; i < NB_PARAMS; i++)
				m_P[i] = parameters[c++].GetReal();
		}
		else
		{
			m_P = P[m_species];
		}

		//m_SDI_type = parameters[c++].GetInt();
		m_SDI_type = SDI_AUGER;
		ASSERT(m_SDI_type < NB_SDI_TYPE);

		return msg;
	}



	double CBudBurstSaintAmantModel::ChillingResponce(double T)const
	{
		double R = 0;// 1 / (1 + exp(-(T - m_P[CU_µ]) / m_P[CU_σ]));

		//switch (RICHARDSON)
		//{
		//c//ase SIGMOID: R = 1 / (1 + exp(-(T - m_P[CU_µ]) / m_P[CU_σ])); break;
			//case CHUINE: R = 1 / (1 + exp(-((T - m_P[CU_µ]) / m_P[CU_σ¹] + Square(T - m_P[CU_µ]) / m_P[CU_σ²]))); break;
		//case RICHARDSON: 
			
			R = max(0.0, min(m_P[CU_σ] - T, m_P[CU_σ] - m_P[CU_µ])); 
			//break;
			//case UTAH:
			//{
			//	ASSERT(m_P[CU_P_MIN] >= -1.0 && m_P[CU_P_MIN] <= 0.0);
			//	if (T < m_P[CU_T_MIN])
			//	{
			//		R = 1.0 / (1.0 + exp(-4 * ((T - m_P[CU_T_MIN]) / (m_P[CU_T_OPT] - m_P[CU_T_MIN]))));
			//	}
			//	else if (T >= m_P[CU_T_MIN] && T < m_P[CU_T_OPT])
			//	{
			//		R = 1 - 0.5 * Square(T - m_P[CU_T_OPT]) / Square(m_P[CU_T_OPT] - m_P[CU_T_MIN]);
			//	}
			//	else if (T >= m_P[CU_T_OPT] && T < m_P[CU_T_MAX])
			//	{
			//		R = 1 - (1 - m_P[CU_P_MIN]) * Square(T - m_P[CU_T_OPT]) / (2 * Square(m_P[CU_T_MAX] - m_P[CU_T_OPT]));
			//	}
			//	else
			//	{
			//		R = m_P[CU_P_MIN] + (1 - m_P[CU_P_MIN]) / (1.0 + exp(-4 * ((m_P[CU_T_MAX] - T) / (m_P[CU_T_MAX] - m_P[CU_T_OPT]))));
			//	}
			//
			//
			//	break;
			//}
		//default: ASSERT(false);
		//};


		ASSERT(!_isnan(R) && _finite(R));

		return R;
	}

	double CBudBurstSaintAmantModel::ForcingResponce(double T)const
	{
		double R = 1 / (1 + exp(-(T - m_P[FU_µ]) / m_P[FU_σ]));
		ASSERT(!_isnan(R) && _finite(R) && R >= 0);

		return R;
	}


	//This method is call to compute solution
	ERMsg CBudBurstSaintAmantModel::OnExecuteDaily()
	{
		ERMsg msg;

		CTPeriod pp(m_weather.GetEntireTPeriod(CTM::DAILY));
		m_output.Init(pp, NB_OUTPUTS, -999);

		ExecuteAllYears(m_weather, m_output);

		return msg;
	}


	double CBudBurstSaintAmantModel::Weibull(size_t stage, double  SDI, const array < double, 2>& p, size_t first_stage, size_t last_stage)
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

	array<double, 5> CBudBurstSaintAmantModel::SDI_2_Sx(double SDI, bool bCumul)
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

		array<double, 5> Sx = { 1 - F1, F1 - F2, F2 - F3, F3 - F4,  F5 };
		if (bCumul)
			Sx = { 1 - F1, F1, F2, F3,  F5 };

		return Sx;
	}

	void CBudBurstSaintAmantModel::ExecuteAllYears(CWeatherYears& weather, CModelStatVector& output)
	{
		//boost::math::beta_distribution<double> SDI_dist(m_P[µ_SDI], m_P[σ_SDI]);
		boost::math::weibull_distribution<double> SDI_dist(m_P[µ_SDI], m_P[σ_SDI]);


		CTPeriod pp(weather.GetEntireTPeriod(CTM::DAILY));
		


		size_t p_S_in = size_t(m_P[S_IN_DAYS]);
		size_t p_S_out = size_t(m_P[S_OUT_DAYS]);
		size_t p_St_in = size_t(m_P[ST_IN_DAYS]);
		size_t p_St_out = size_t(m_P[ST_OUT_DAYS]);
		size_t p_CU = 14;
		size_t p_FU = 5;

		if (m_Tmean.empty() || m_last_p_S_in != p_S_in || m_last_p_S_out != p_S_out || m_last_p_St_in != p_St_in || m_last_p_St_out != p_St_out)
		{
			m_last_p_S_in = p_S_in;
			m_last_p_S_out = p_S_out;
			m_last_p_St_in = p_St_in;
			m_last_p_St_out = p_St_out;
			m_last_p_CU = p_CU;
			m_last_p_FU = p_FU;
			

			m_Tmean.resize(pp.GetNbDay());

			for (size_t y = 0, dd = 0; y < weather.GetNbYears(); y++)
			{
				for (size_t m = 0; m < weather[y].GetNbMonth(); m++)
				{
					for (size_t d = 0; d < weather[y][m].GetNbDays(); d++, dd++)
					{
						array<CStatistic, 3> T_S_in_Days_stat;
						array<CStatistic, 3> T_S_out_Days_stat;
						array<CStatistic, 3> T_St_in_Days_stat;
						array<CStatistic, 3> T_St_out_Days_stat;
						array<CStatistic, 3> T_CU_days;
						array<CStatistic, 3> T_FU_days;

						CTRef TRef = weather[y][m][d].GetTRef();


						size_t max_days = max(p_S_in, max(p_S_out, max(p_St_in, p_St_out)));
						for (size_t i = 0; i < max_days; i++)//Charrier 2018 use the mean maximum of the last 14 days 
						{

							if (i < p_S_in)
							{
								T_S_in_Days_stat[0] += weather.GetDay(TRef - min(dd, p_S_in))[H_TMIN];
								T_S_in_Days_stat[1] += weather.GetDay(TRef - min(dd, p_S_in))[H_TNTX];
								T_S_in_Days_stat[2] += weather.GetDay(TRef - min(dd, p_S_in))[H_TMAX];
							}
							if (i < p_S_out)
							{
								T_S_out_Days_stat[0] += weather.GetDay(TRef - min(dd, p_S_out))[H_TMIN];
								T_S_out_Days_stat[1] += weather.GetDay(TRef - min(dd, p_S_out))[H_TNTX];
								T_S_out_Days_stat[2] += weather.GetDay(TRef - min(dd, p_S_out))[H_TMAX];
							}

							if (i < p_St_in)
							{
								T_St_in_Days_stat[0] += weather.GetDay(TRef - min(dd, p_St_in))[H_TMIN];
								T_St_in_Days_stat[1] += weather.GetDay(TRef - min(dd, p_St_in))[H_TNTX];
								T_St_in_Days_stat[2] += weather.GetDay(TRef - min(dd, p_St_in))[H_TMAX];
							}
							if (i < p_St_out)
							{
								T_St_out_Days_stat[0] += weather.GetDay(TRef - min(dd, p_St_out))[H_TMIN];
								T_St_out_Days_stat[1] += weather.GetDay(TRef - min(dd, p_St_out))[H_TNTX];
								T_St_out_Days_stat[2] += weather.GetDay(TRef - min(dd, p_St_out))[H_TMAX];
							}

							if (i < p_CU)
							{
								T_CU_days[0] += weather.GetDay(TRef - min(dd, p_St_out))[H_TMIN];
								T_CU_days[1] += weather.GetDay(TRef - min(dd, p_St_out))[H_TNTX];
								T_CU_days[2] += weather.GetDay(TRef - min(dd, p_St_out))[H_TMAX];
							}

							if (i < p_FU)
							{
								T_FU_days[0] += weather.GetDay(TRef - min(dd, p_St_out))[H_TMIN];
								T_FU_days[1] += weather.GetDay(TRef - min(dd, p_St_out))[H_TNTX];
								T_FU_days[2] += weather.GetDay(TRef - min(dd, p_St_out))[H_TMAX];
							}

						}

						for (size_t i = 0; i < 3; i++)
						{
							m_Tmean[dd].T_S_in[i] = T_S_in_Days_stat[i][MEAN];
							m_Tmean[dd].T_S_out[i] = T_S_out_Days_stat[i][MEAN];
							m_Tmean[dd].T_St_in[i] = T_St_in_Days_stat[i][MEAN];
							m_Tmean[dd].T_St_out[i] = T_St_out_Days_stat[i][MEAN];
							m_Tmean[dd].T_CU[i] = T_CU_days[i][MEAN];
							m_Tmean[dd].T_FU[i] = T_FU_days[i][MEAN];
						}
					}
				}
			}
		}


		//double GFS = m_P[S_0]; //mg/gDM
		//double Starch = m_P[ST_0];//mg/gDM



		for (size_t yy = 1; yy < weather.GetNbYears(); yy++)
		{
			double CU = 0;
			double FU = 0;
			double GFS = m_P[S_0]; //mg/gDM
			double Starch = m_P[ST_0];//mg/gDM



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
							double T_S_in_days = m_Tmean[TRef - pp.Begin()].T_S_in[S_IN_STAT];
							double T_S_out_days = m_Tmean[TRef - pp.Begin()].T_S_out[S_OUT_STAT];
							double T_St_in_days = m_Tmean[TRef - pp.Begin()].T_St_in[ST_IN_STAT];
							double T_St_out_days = m_Tmean[TRef - pp.Begin()].T_St_out[ST_OUT_STAT];
							
							double T_CU = m_Tmean[TRef - pp.Begin()].T_CU[CU_STAT];
							double T_FU = m_Tmean[TRef - pp.Begin()].T_FU[FU_STAT];
							//double T = weather[y][m][d][H_TNTX][MEAN];
							//double T_FU =weather[y][m][d][H_TNTX][MEAN];
							//double T_CU =weather[y][m][d][H_TNTX][MEAN];

							if (CU < m_P[CU_crit])
							{
								CU += ChillingResponce(T_CU);
								CU = min(CU, m_P[CU_crit]);
							}
							else
							{
								FU += ForcingResponce(T_FU);
								FU = min(FU, m_P[FU_crit]);
							}



							ASSERT(CU >= 0 && CU <= m_P[CU_crit]);
							ASSERT(FU >= 0 && FU <= m_P[FU_crit]);


							double PS = CU / m_P[CU_crit] + FU / m_P[FU_crit];

							ASSERT(PS >= 0 && PS <= 2);



							double S_in_psi_max = m_P[S_IN_σ_PS] < 0 ? 1 : 0;
							double S_in_psi = m_P[S_IN_PSI] * max(0.0, min(1.0, S_in_psi_max + exp(-0.5 * Square((PS - m_P[S_IN_µ_PS]) / m_P[S_IN_σ_PS])) / (m_P[S_IN_σ_PS] * sqrt(2 * PI))));
							double S_in_Rate = exp(-0.5 * Square((T_S_in_days - m_P[S_IN_µ_T]) / m_P[S_IN_σ_T])) / (m_P[S_IN_σ_T] * sqrt(2 * PI));
							double S_in_k = S_in_psi * S_in_Rate;

							double S_out_psi_max = m_P[S_OUT_σ_PS] < 0 ? 1 : 0;
							double S_out_psi = m_P[S_OUT_PSI] * max(0.0, min(1.0, S_out_psi_max + exp(-0.5 * Square((PS - m_P[S_OUT_µ_PS]) / m_P[S_OUT_σ_PS])) / (m_P[S_OUT_σ_PS] * sqrt(2 * PI))));
							double S_out_Rate = exp(-0.5 * Square((T_S_out_days - m_P[S_OUT_µ_T]) / m_P[S_OUT_σ_T])) / (m_P[S_OUT_σ_T] * sqrt(2 * PI));
							double S_out_k = S_out_psi * S_out_Rate;


							double St_in_psi_max = m_P[ST_IN_σ_PS] < 0 ? 1 : 0;
							double St_in_psi = m_P[ST_IN_PSI] * max(0.0, min(1.0, St_in_psi_max + exp(-0.5 * Square((PS - m_P[ST_IN_µ_PS]) / m_P[ST_IN_σ_PS])) / (m_P[ST_IN_σ_PS] * sqrt(2 * PI))));
							double St_in_Rate = exp(-0.5 * Square((T_St_in_days - m_P[ST_IN_µ_T]) / m_P[ST_IN_σ_T])) / (m_P[ST_IN_σ_T] * sqrt(2 * PI));
							double St_in_k = St_in_psi * St_in_Rate;


							double St_out_psi_max = m_P[ST_OUT_σ_PS] < 0 ? 1 : 0;
							double St_out_psi = m_P[ST_OUT_PSI] * max(0.0, min(1.0, St_out_psi_max + exp(-0.5 * Square((PS - m_P[ST_OUT_µ_PS]) / m_P[ST_OUT_σ_PS])) / (m_P[ST_OUT_σ_PS] * sqrt(2 * PI))));
							double St_out_Rate = exp(-0.5 * Square((T_St_out_days - m_P[ST_OUT_µ_T]) / m_P[ST_OUT_σ_T])) / (m_P[ST_OUT_σ_T] * sqrt(2 * PI));
							double St_out_k = St_out_psi * St_out_Rate;


							double delta_S = S_in_k - S_out_k;
							GFS += delta_S;
							double delta_St = St_in_k - St_out_k;


							Starch -= delta_St;

							GFS = max(5.0, min(120.0, GFS));
							Starch = max(5.0, min(120.0, Starch));

							ASSERT(GFS > 0);
							ASSERT(Starch > 0);


							//double SDI_Dhont = cdf(SDI_dist, max(0.0, min(1.0, PS - 1))) * MAX_SDI;//0 .. 6;
							//double SDI_Auger = min(5.0, max(0.0, -0.1767 + 5.5566 * (exp(-pow((6 - SDI_Dhont) / 1.9977, 1.1469)))));
							//double SDI = m_SDI_type == SDI_DHONT ? SDI_Dhont : SDI_Auger;

							double SDI = cdf(SDI_dist, max(0.0, min(1.0, PS - 1))) * 5;


							output[TRef][O_S_CONC] = GFS;
							output[TRef][O_ST_CONC] = Starch;
							output[TRef][O_CU] = CU;
							output[TRef][O_FU] = FU;
							output[TRef][O_PS] = PS;
							output[TRef][O_SDI] = SDI;

						}
					}
				}
			}
		}
	}






	
	enum { I_SPECIES2, I_SOURCE2, I_SITE2, I_LATITUDE, I_LONGITUDE, I_ELEVATION, I_DATE2, I_STARCH2, I_SUGAR2, I_SDI2, I_N2, I_DEF2, I_DEFEND_N12, I_DEFEND_N2, I_PROVINCE2, I_TYPE2, NB_INPUTS2 };
	void CBudBurstSaintAmantModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		static const char* SPECIES_NAME[] = { "bf", "ws", "bs", "ns", "rs", "rbs" };
		if (data.size() == NB_INPUTS2)
		{
			if (data[I_SPECIES2] == SPECIES_NAME[m_species] && data[I_TYPE2] == "C")
			{
				CSAResult obs;

				obs.m_ref.FromFormatedString(data[I_DATE2]);
				obs.m_obs[0] = stod(data[I_SDI2]);
				obs.m_obs.push_back(stod(data[I_STARCH2]));
				obs.m_obs.push_back(stod(data[I_SUGAR2]));
				//obs.m_obs.push_back(stod(data[I_DEFEND_N2]));

				if ((USE_SDI && obs.m_obs[0] > -999) ||
					(USE_STARCH && obs.m_obs[1] > -999) ||
					(USE_SUGAR && obs.m_obs[2] > -999))
				{
					m_years.insert(obs.m_ref.GetYear());
				}

				m_SAResult.push_back(obs);
			}
		}
	}





	static const int ROUND_VAL = 4;
	void CBudBurstSaintAmantModel::CalibrateSDI(CStatisticXY& stat)
	{

		//if (m_P[Tlow] >= m_P[Thigh])
			//return;


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



	void CBudBurstSaintAmantModel::GetFValueDaily(CStatisticXY& stat)
	{
		//return CalibrateSDI(stat); 
		if (!m_SAResult.empty())
		{
			if ((m_P[S_IN_σ_PS] > -0.1 && m_P[S_IN_σ_PS] < 0.1) ||
				(m_P[S_OUT_σ_PS] > -0.1 && m_P[S_OUT_σ_PS] < 0.1) ||
				(m_P[ST_IN_σ_PS] > -0.1 && m_P[ST_IN_σ_PS] < 0.1) ||
				(m_P[ST_OUT_σ_PS] > -0.1 && m_P[ST_OUT_σ_PS] < 0.1)

				)
				return;

			

			if (data_weather.GetNbYears() == 0)
			{
				CTPeriod pp((*m_years.begin()) - 1, JANUARY, DAY_01, *m_years.rbegin(), DECEMBER, DAY_31);
				pp = pp.Intersect(m_weather.GetEntireTPeriod(CTM::DAILY));
				if (pp.IsInit())
				{
					((CLocation&)data_weather) = m_weather;
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

			//verify S_conc and St_conc and return if invalid
			if (m_SAResult.front().m_obs.size() != 2)
			{
				for (size_t d = 0; d < output.size(); d++)
				{
					if (output[d][O_S_CONC] > 2 * MAX_SUGAR || output[d][O_ST_CONC] > 2 * MAX_STRACH)
					{
						return;//invalid solution
					}
				}
			}


			for (size_t i = 0; i < m_SAResult.size(); i++)
			{
				if (output.IsInside(m_SAResult[i].m_ref))
				{
					
					if (USE_SDI && m_SAResult[i].m_obs[0] > -999 && m_SAResult[i].m_ref.GetJDay() < 213 && output[m_SAResult[i].m_ref][O_SDI] > -999)
					{
						//double maxSDI = m_SDI_type == SDI_DHONT ? 6 : 5;
						double obs_SDI = m_SAResult[i].m_obs[0]/5;//(m_SAResult[i].m_obs[0] - MIN_SDI) / (MAX_SDI - MIN_SDI);
						double sim_SDI = output[m_SAResult[i].m_ref][O_SDI]/5;//(output[m_SAResult[i].m_ref][O_SDI] - MIN_SDI) / (MAX_SDI - MIN_SDI);
						stat.Add(obs_SDI, sim_SDI);
					}

					if (USE_STARCH && m_SAResult[i].m_obs[1] > -999 && output[m_SAResult[i].m_ref][O_ST_CONC] > -999)
					{
						double obs_starch = (m_SAResult[i].m_obs[1] - MIN_STRACH) / (MAX_STRACH - MIN_STRACH);
						double sim_starch = (output[m_SAResult[i].m_ref][O_ST_CONC] - MIN_STRACH) / (MAX_STRACH - MIN_STRACH);

						//if (!bSt_Valid)
						//	sim_starch *= Signe(Rand(-100, 100)) * exp(Rand(2.0, 3.0));

						stat.Add(obs_starch, sim_starch);
					}


					if (USE_SUGAR && m_SAResult[i].m_obs[2] > -999 && output[m_SAResult[i].m_ref][O_S_CONC] > -999)
					{
						double obs_GFS = (m_SAResult[i].m_obs[2] - MIN_SUGAR) / (MAX_SUGAR - MIN_SUGAR);
						double sim_GFS = (output[m_SAResult[i].m_ref][O_S_CONC] - MIN_SUGAR) / (MAX_SUGAR - MIN_SUGAR);

						//if (!bSt_Valid)
						//	sim_GFS *= Signe(Rand(-100, 100)) * exp(Rand(2.0, 3.0));

						stat.Add(obs_GFS, sim_GFS);
					}
				}
			}//for all results
		//}
		}
	}






}