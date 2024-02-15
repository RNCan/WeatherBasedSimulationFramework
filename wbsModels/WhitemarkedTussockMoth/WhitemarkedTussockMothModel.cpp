//***********************************************************
// 22/04/2019	1.0.0	Rémi Saint-Amant   Creation
//***********************************************************
#include "WhitemarkedTussockMothModel.h"
#include "ModelBase/EntryPoint.h"
#include "Basic\DegreeDays.h"

using namespace WBSF::HOURLY_DATA;
using namespace WBSF::WTM;
using namespace std;

namespace WBSF
{
	static const CDegreeDays::TDailyMethod DD_METHOD = CDegreeDays::MODIFIED_ALLEN_WAVE;
	enum { ACTUAL_CDD, DATE_DD717, DIFF_DAY, NB_OUTPUTS };

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CWhitemarkedTussockMothModel::CreateObject);

	CWhitemarkedTussockMothModel::CWhitemarkedTussockMothModel()
	{
		//NB_INPUT_PARAMETER is used to determine if the dll
		//uses the same number of parameters than the model interface
		NB_INPUT_PARAMETER = -1;
		VERSION = "1.0.0 (2019)";

		m_bCumul = false;
		for (size_t s = 0; s < NB_STAGES; s++)
		{
			for (size_t p = 0; p < NB_DEV_RATE_PARAMS; p++)
				m_P[s][p] = CWhitemarkedTussockMothEquations::P[s][p];

			for (size_t p = 0; p < NB_RDR_PARAMS; p++)
				m_D[s][p] = CWhitemarkedTussockMothEquations::D[s][p];

		}

		for (size_t p = 0; p < NB_HATCH_PARAMS; p++)
		{
			m_H[p] = CWhitemarkedTussockMothEquations::H[p];
		}

		
		m_egg_factor = {1.0,0.59};

		m_col_weight.fill({ {0,0,0,0,0} });

	}

	CWhitemarkedTussockMothModel::~CWhitemarkedTussockMothModel()
	{
	}

	double Zalom83(double Tmin, double Tmax, double Tresh)
	{
		double Q = asin((Tresh - (Tmax + Tmin) / 2.0) / ((Tmax - Tmin) / 2.0));

		double DD = 0;
		if (Tmax > Tresh)
		{
			if (Tmin >= Tresh)
			{
				DD = (Tmax + Tmin) / 2.0 - Tresh;
			}
			else
			{
				DD = 1.0 / PI * (((Tmax + Tmin) / 2.0 - Tresh)*(PI / 2.0 - Q) + (Tmax - Tmin) / 2.0 * cos(Q));
			}
		}

		return DD;
	}

	double Zalom83(double Tmin, double Tmax, double Tlo, double Thi)
	{
		double DDlo = Zalom83(Tmin, Tmax, Tlo);
		double DDhi = Zalom83(Tmin, Tmax, Thi);
		return DDlo - DDhi;
	}


	//this method is call to load your parameter in your variable
	ERMsg CWhitemarkedTussockMothModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		size_t c = 0;

		m_bCumul = parameters[c++].GetBool();
		if (parameters.size() == 1 + NB_STAGES * (NB_DEV_RATE_PARAMS + NB_RDR_PARAMS) + NB_HATCH_PARAMS +2)
		{
			for (size_t s = 0; s < NB_STAGES; s++)
			{
				for (size_t p = 0; p < NB_DEV_RATE_PARAMS; p++)
					m_P[s][p] = parameters[c++].GetFloat();
			}

			for (size_t s = 0; s < NB_STAGES; s++)
			{
				for (size_t p = 0; p < NB_RDR_PARAMS; p++)
					m_D[s][p] = parameters[c++].GetFloat();
			}

			for (size_t p = 0; p < NB_HATCH_PARAMS; p++)
			{
				m_H[p] = parameters[c++].GetFloat();
			}
			
			for (size_t p = 0; p < 2; p++)
				m_egg_factor[p] = parameters[c++].GetFloat();
		}

		return msg;
	}

	//This method is called to compute the solution
	ERMsg CWhitemarkedTussockMothModel::OnExecuteDaily()
	{
		ERMsg msg;

		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();

		//This is where the model is actually executed
		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));
		m_output.Init(p, NB_STATS, 0);

		for (size_t y = 0; y < m_weather.size(); y++)
		{
			ExecuteDaily(m_weather[y], m_output);
		}

		return msg;
	}

	void CWhitemarkedTussockMothModel::ExecuteDaily(const CWeatherYear& weather, CModelStatVector& output)
	{
		//Create stand
		CWTMStand stand(this, m_H[Τᴴ]);

		for (size_t s = 0; s < NB_STAGES; s++)
		{
			for (size_t p = 0; p < NB_DEV_RATE_PARAMS; p++)
				stand.m_equations.m_P[s][p] = m_P[s][p];
		}

		//Set parameters to equation
		for (size_t s = 0; s < NB_STAGES; s++)
		{
			for (size_t p = 0; p < NB_RDR_PARAMS; p++)
			{
				stand.m_equations.m_D[s][p] = m_D[s][p];
			}
		}


		for (size_t p = 0; p < NB_HATCH_PARAMS; p++)
		{
			stand.m_equations.m_H[p] = m_H[p];
		}

		for (size_t p = 0; p < 2; p++)
			stand.m_egg_factor[p] = m_egg_factor[p];

		CTPeriod p = weather.GetEntireTPeriod(CTM(CTM::DAILY));
		int year = weather.GetTRef().GetYear();
		CTRef March1(year, MARCH, DAY_01);

		//Create host
		CLNFHostPtr pHost(new CWTMHost(&stand));

		pHost->m_nbMinObjects = 10;
		pHost->m_nbMaxObjects = 1000;
		pHost->Initialize<CWhitemarkedTussockMoth>(CInitialPopulation(p.Begin(), 0, 400, 100, EGG));

		//add host to stand			
		stand.m_host.push_front(pHost);



		//double GDD = 0;
		for (CTRef d = p.Begin(); d <= p.End(); d++)
		{

			//if (d >= March1)
			//{
			stand.Live(weather.GetDay(d));
			stand.GetStat(d, output[d]);

			//double Tmin = m_weather.GetDay(d)[H_TMIN][MEAN];
			//double Tmax = m_weather.GetDay(d)[H_TMAX][MEAN];
			//double DDAY = Zalom83(Tmin, Tmax, 12.8, 999);
			//output[d][S_GDD] = output[d - 1][S_GDD] + DDAY;
		//}



			stand.AdjustPopulation();
			HxGridTestConnection();
		}


		if (m_bCumul)
		{
			CTPeriod p = weather.GetEntireTPeriod(CTM(CTM::DAILY));

			//cumulative result
			for (size_t s = LARVAE; s < DEAD_ADULT; s++)
			{
				//size_t stat_type = s == DEAD_ADULT ? HIGHEST : SUM;
				CStatistic stat = output.GetStat(S_EGG0 + s, p);
				if (stat[SUM] > 0)
				{
					for (CTRef d = p.Begin() + 1; d <= p.End(); d++)
						output[d][S_EGG0 + s] = output[d - 1][S_EGG0 + s] + output[d][S_EGG0 + s] * 100 / stat[SUM];
				}

				stat = output.GetStat(S_EGG1 + s, p);
				if (stat[SUM] > 0)
				{
					for (CTRef d = p.Begin() + 1; d <= p.End(); d++)
						output[d][S_EGG1 + s] = output[d - 1][S_EGG1 + s] + output[d][S_EGG1 + s] * 100 / stat[SUM];
				}
			}

			CStatistic stat = output.GetStat(S_DEAD_ADULT0, p);
			if (stat[HIGHEST] > 0)
			{
				for (CTRef d = p.Begin() + 1; d <= p.End(); d++)
					output[d][S_DEAD_ADULT0] = output[d][S_DEAD_ADULT0] * 100 / stat[HIGHEST];
			}

			stat = output.GetStat(S_EGG1, p);
			if (stat[SUM] > 0)
			{
				for (CTRef d = p.Begin() + 1; d <= p.End(); d++)
					output[d][S_EGG1] = output[d - 1][S_EGG1] + output[d][S_EGG1] * 100 / stat[SUM];
			}


			stat = output.GetStat(S_DEAD_ADULT1, p);
			if (stat[HIGHEST] > 0)
			{
				for (CTRef d = p.Begin() + 1; d <= p.End(); d++)
					output[d][S_DEAD_ADULT1] = output[d][S_DEAD_ADULT1] * 100 / stat[HIGHEST];
			}

			stat = output.GetStat(S_EGG_MASS0, p);
			if (stat[SUM] > 0)
			{
				for (CTRef d = p.Begin() + 1; d <= p.End(); d++)
					output[d][S_EGG_MASS0] = output[d - 1][S_EGG_MASS0] + output[d][S_EGG_MASS0] * 100 / stat[SUM];
			}

			stat = output.GetStat(S_EMERGENCE0, p);
			if (stat[SUM] > 0)
			{
				for (CTRef d = p.Begin() + 1; d <= p.End(); d++)
					output[d][S_EMERGENCE0] = output[d - 1][S_EMERGENCE0] + output[d][S_EMERGENCE0] * 100 / stat[SUM];
			}

			stat = output.GetStat(S_EGG_MASS1, p);
			if (stat[SUM] > 0)
			{
				for (CTRef d = p.Begin() + 1; d <= p.End(); d++)
					output[d][S_EGG_MASS1] = output[d - 1][S_EGG_MASS1] + output[d][S_EGG_MASS1] * 100 / stat[SUM];
			}

			stat = output.GetStat(S_EMERGENCE1, p);
			if (stat[SUM] > 0)
			{
				for (CTRef d = p.Begin() + 1; d <= p.End(); d++)
					output[d][S_EMERGENCE1] = output[d - 1][S_EMERGENCE1] + output[d][S_EMERGENCE1] * 100 / stat[SUM];
			}

		}
	}

	enum { I_GENERATION, I_STAGEID, I_GDD, I_N, I_CUMUL, I_N2, NB_INPUTS };
	void CWhitemarkedTussockMothModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		//SiteID	Stage	StageID	Year	Generation	GDD12.8	Corrected N	Cumul
		ASSERT(data.size() == NB_INPUTS + 2);

		CSAResult obs;
		obs.m_ref.FromFormatedString(data[1]);
		obs.m_obs.resize(NB_INPUTS);
		for (size_t i = 0; i < NB_INPUTS; i++)
		{
			obs.m_obs[i] = stod(data[i + 2]);
			ASSERT(obs.m_obs[i] > -999);
		}

		size_t g = obs.m_obs[I_GENERATION];
		size_t s = obs.m_obs[I_STAGEID];
		ASSERT(s <= 4 && g < 2);
		m_Jday[g][s] += obs.m_ref.GetJDay();

		m_SAResult.push_back(obs);

		//m_years.insert(obs.m_ref.GetYear());

	}

	double GetSimX(size_t s, size_t g, CTRef TRefO, double obsY, const CModelStatVector& output)
	{
		ASSERT(obsY > 0 && obsY < 100);//0 and 100 can't be estimated
		double x = -999;

		//if (obsY > 0 && obsY < 100)//0 and 100 can't be estimated
		//{
			//if (obs > 0.01 && obs < 99.99)
		//	if (obsY >= 100)
			//	obsY = 99.99;//to avoid some problem of truncation

			long index = output.GetFirstIndex(s, ">=", obsY, 1, CTPeriod(TRefO.GetYear(), FIRST_MONTH, FIRST_DAY, TRefO.GetYear(), LAST_MONTH, LAST_DAY));
			if (index >= 1)
			{
				double simX1 = output.GetFirstTRef().GetJDay() + index;
				double simX2 = output.GetFirstTRef().GetJDay() + index + 1;

				double simY1 = output[index][s];
				double simY2 = output[index + 1][s];
				if (simY2 != simY1)
				{
					double slope = (simX2 - simX1) / (simY2 - simY1);
					double obsX = simX1 + (obsY - simY1)*slope;
					ASSERT(!_isnan(obsX) && _finite(obsX));

					x = obsX;
				}
			}
		//}

		return x;
	}

	//this code verify that the relative development rate is not distorted.
	//we would not modify the development rate with a uncalibrated RDR
	bool CWhitemarkedTussockMothModel::IsParamValid()const
	{
		bool equal = true;
		for (size_t s = 0; s <= ADULT && equal; s++)
		{
			CStatistic rL;
			for (double Э = 0.01; Э < 0.5; Э += 0.01)
			{
				double r = 1.0 - log((pow(Э, m_D[s][Ϙ]) - 1.0) / (pow(0.5, m_D[s][Ϙ]) - 1.0)) / m_D[s][к];
				if (r >= 0.4 && r <= 2.5)
					rL += 1.0 / r;//reverse for comparison
			}

			CStatistic rH;
			for (double Э = 0.51; Э < 1.0; Э += 0.01)
			{
				double r = 1.0 - log((pow(Э, m_D[s][Ϙ]) - 1.0) / (pow(0.5, m_D[s][Ϙ]) - 1.0)) / m_D[s][к];
				if (r >= 0.4 && r <= 2.5)
					rH += r;
			}

			if (rL.IsInit() && rH.IsInit())
				equal = fabs(rL[SUM] - rH[SUM]) < 5.3; //in Régnière (2012) obtain a max of 5.3. We multiply by 3 because it was not enought variable
			else
				equal = false;
		}

		return equal;
	}

	bool CWhitemarkedTussockMothModel::GetFValueDaily(CStatisticXY& stat)
	{
		ASSERT(!m_SAResult.empty());

		static const size_t COLPOS[2][5] = { { S_EGG0, S_LARVAE0, S_PUPA0, S_ADULT0, S_EGG_MASS0 },{ S_EGG1, S_LARVAE1, S_PUPA1, S_ADULT1, S_EGG_MASS1 } };

		//static const double HATCH_GDD[3][2] = 
		//{
		//	{72.0,0.9},//mean of SE = 1.3, 
		//	{71.5,1.8},
		//	{67.7,1.2}
		//};

		if (m_col_weight[0][LARVAE]==0)
		{
			//compute weight of each variables
			std::array< std::array<CStatistic, 5>, 2> sss;
			CStatistic ss;
			for (size_t i = 0; i < m_SAResult.size(); i++)
			{
				size_t s = size_t(m_SAResult[i].m_obs[I_STAGEID]);
				size_t g = size_t(m_SAResult[i].m_obs[I_GENERATION]);

				ss += m_SAResult[i].m_obs[I_N];
				sss[g][s] += m_SAResult[i].m_obs[I_N];
			}

			for (size_t g = 0; g < sss.size(); g++)
			{
				for (size_t s = 0; s < sss[g].size(); s++)
				{
					if (sss[g][s][SUM] > 0)
						m_col_weight[g][s] = (size_t)max(1.0, 1000 * sss[g][s][SUM] /(sss[g][s][NB_VALUE]*ss[SUM]));
				}
			}

		}

		//for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		//{
		//	CDegreeDays DD(CDegreeDays::SINGLE_SINE, m_H[Τᴴ]);
		//	CStatistic statHatch;
		//	double GDD = 0;
		//	int year = m_weather[y].GetTRef().GetYear();
		//	CTPeriod p = m_weather[y].GetEntireTPeriod(CTM(CTM::DAILY));
		//	CTRef March1(year, MARCH, DAY_01);
		//	for (CTRef d = p.Begin(); d <= p.End(); d++)
		//	{
		//		if (d.GetJDay() >= m_H[start])
		//		{
		//			double Tmin = m_weather.GetDay(d)[H_TMIN][MEAN];
		//			double Tmax = m_weather.GetDay(d)[H_TMAX][MEAN];
		//			//double DDAY = Zalom83(Tmin, Tmax, m_H[Τᴴ], 999);
		//			double DDAY = DD.GetDD(m_weather.GetDay(d));
		//			GDD += DDAY;

		//			double EggHatch = 1.0 / (1.0 + exp(-(GDD - m_H[μ]) / m_H[ѕ]));
		//			if (EggHatch >= 0.01 && EggHatch <= 0.99)
		//			{
		//				statHatch += GDD;
		//			}
		//		}
		//	}
		//	
		//	stat.Add((HATCH_GDD[y][0] - 70.4) / 2.351, (statHatch[MEAN] - 70.4) / 2.351);
		//	stat.Add((HATCH_GDD[y][1] - 1.3) / 0.458, (statHatch[STD_ERR] - 1.3) / 0.458);
		//	
		//}
		//return;


		if (!m_bCumul)
			m_bCumul = true;//SA always cumulative

		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();

		//low and hi relative development rate must be valid
		if (!IsParamValid())
			return false;


		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			int year = m_weather[y].GetTRef().GetYear();
			//				if (m_years.find(year) == m_years.end())
				//				continue;

			CModelStatVector output;
			output.Init(m_weather[y].GetEntireTPeriod(CTM(CTM::DAILY)), NB_STATS, 0);
			ExecuteDaily(m_weather[y], output);

			
			for (size_t i = 0; i < m_SAResult.size(); i++)
			{
				if (m_SAResult[i].m_ref.GetYear() == year)
				{

					size_t s = size_t(m_SAResult[i].m_obs[I_STAGEID]);
					size_t g = size_t(m_SAResult[i].m_obs[I_GENERATION]);

					//if (g==0 /*s > LARVAE */)
					{
						double obs_y = m_SAResult[i].m_obs[I_CUMUL];
						double sim_y = output[m_SAResult[i].m_ref][COLPOS[g][s]];

						//if (obs_y > -999)
						//{
							//for (int j = 0; j < min(30.0,m_SAResult[i].m_obs[I_N]); j++)
						//for (int j = 0; j < m_col_weight[g][s]; j++)
//								stat.Add(obs_y, sim_y);
						//}


		
						if (obs_y > 0 && obs_y<100)
						{
							double obs_x = m_SAResult[i].m_ref.GetJDay();
							double sim_x = GetSimX(COLPOS[g][s], g, m_SAResult[i].m_ref, obs_y, output);

							obs_x = 100 * (obs_x - m_Jday[g][s][LOWEST]) / m_Jday[g][2][RANGE];
							sim_x = 100 * (sim_x - m_Jday[g][s][LOWEST]) / m_Jday[g][2][RANGE];

						//for (int j = 0; j < min(30.0, m_SAResult[i].m_obs[I_N]); j++)
							//for (int j = 0; j < m_col_weight[g][s]; j++)
								stat.Add(obs_x, sim_x);
						}
					}
				}
			}
		}

		return true;
	}

	//void CWhitemarkedTussockMothModel::AddAnnualResult(const StringVector& header, const StringVector& data)
	//{
	//	ASSERT(data.size() == 2);

	//	CSAResult obs;
	//	obs.m_ref.FromFormatedString(data[1]);
	//	m_SAResult.push_back(obs);

	//	m_years.insert(obs.m_ref.GetYear());

	//}

	//void CWhitemarkedTussockMothModel::GetFValueAnnual(CStatisticXY& stat)
	//{
	//	if (!m_SAResult.empty())
	//	{


	//		//for (size_t y = 0; y < m_weather.GetNbYears() - 1; y++)
	//		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
	//		{
	//			int year = m_weather[y].GetTRef().GetYear();
	//			if (m_years.find(year) == m_years.end())
	//				//if (m_years.find(year + 1) == m_years.end())
	//				continue;

	//			//CTRef begin = m_weather[y].GetEntireTPeriod().Begin();
	//			//CTRef end = m_weather[y].GetEntireTPeriod().End();
	//			//CTRef begin = CTRef(year, m_start.GetMonth(), m_start.GetDay());
	//			//CTRef end = CTRef(year, DECEMBER, DAY_31);



	//			CTRef begin = CTRef(year, m_start.GetMonth(), m_start.GetDay());
	//			CTRef end = CTRef(year + 1, DECEMBER, DAY_31);


	//			//CTRef begin = CTRef(year, NOVEMBER, DAY_01);
	//			//CTRef end = CTRef(year + 1, NOVEMBER, DAY_01);

	//			double CDD = 0;
	//			CDegreeDays DD(DD_METHOD, m_threshold);

	//			for (CTRef d = begin; d < end && CDD < m_sumDD; d++)
	//			{
	//				CDD += DD.GetDD(m_weather.GetDay(d));
	//				if (CDD >= m_sumDD)
	//				{
	//					for (size_t j = 0; j < m_SAResult.size(); j++)
	//					{
	//						if (m_SAResult[j].m_ref.GetYear() == d.GetYear())
	//						{
	//							stat.Add(d.GetJDay(), m_SAResult[j].m_ref.GetJDay());
	//						}
	//					}
	//				}
	//			}
	//		}
	//	}
	//}

}


//
//
//double sinec(double sum, double diff, double fk1)
//{ // SUBROUTINE USED BY THE SINE CURVE METHODS
//
//	static const double twopi = 6.2834;
//	static const double pihlf = 1.5708;
//
//	double d2 = fk1 - sum;
//	double theta = atan2(d2, sqrt(diff * diff - d2 * d2));
//	if ((d2 < 0) && (theta > 0)) {
//		theta = theta - 3.1416;
//	}
//
//	double heat = (diff * cos(theta) - d2 * (pihlf - theta)) / twopi;
//
//	return heat;
//}
//
//double DDSine(double min, double max, double TLOW, double THI)
//{
//	// SINGLE AND DOUBLE SINE CURVE METHODS
//
//	double heat = 0.0;
//	if (min > THI)
//	{
//		heat = THI - TLOW;
//	}
//	else
//	{
//		if (max <= TLOW)
//		{
//			heat = 0;
//		}
//		else
//		{
//			double fk1 = 2 * TLOW;
//			double diff = max - min;
//			double sum = max + min;
//			if (min >= TLOW)
//			{
//				heat = (sum - fk1) / 2;
//			}
//			else
//			{
//				heat = sinec(sum, diff, fk1);
//			}
//
//			if (max > THI)
//			{
//				double fk1 = 2 * THI;
//				heat = heat - sinec(sum, diff, fk1);
//			}
//		}
//	}
//
//	return heat / 2;
//}
//
//void SISINE(bool AB, float MIN, float MAX, float THRESH, float& DDAY, int& Error)
//{
//	//     PURPOSE :
//	//      TO CALCULATE DEGREE - DAYS ACCUMULATED ABOVE or BELOW A
//	//     THRESHOLD USING A SINE WAVE ESTIMATION OF AREA UNDER THE CURVE.
//	//
//	//     COMPILE AS : FL % 4I2 % 4Nt %FPi  SISINE.FOR
//	//                  MICROSOFT FORTRAN 5.0
//	//
//	//     CALLS : NO EXTERNAL ROUTINES
//	//
//	//    *****************************************************************
//	//
//
//	//Copyright 1985 - Regents of the University of California.All rights reserved.
//
//	//LOGICAL * 2 AB                     //T = ABOVE; F = BELOW
//	//INTEGER * 2 ERROR                  
//	//C
//	//REAL * 4 ARG,
//	//*DDAY, //DEGREE - DAYS
//	//*MAX, //MAXIMUM TEMPERATURE
//	//*MIN, !MINIMUM TEMPERATURE
//	//*       MN, !LOCAL COMPUTATIONAL MIN
//	//*       MX, !LOCAL COMPUTATIONAL MAX
//	//*       PI, !
//	//*       Q, !
//	//*       TA, !
//	//*       TM, !
//	//*       THRESH, !THRESHOLD
//	//*       XA                        !
//	//
//	static const float PI = 3.14159265f;
//
//	//                  INITIALIZE VARIABLES
//
//	float MN = 0; //LOCAL COMPUTATIONAL MIN
//	float MX = 0; //LOCAL COMPUTATIONAL MAX
//
//	DDAY = 0.0;
//	Error = 0;
//	if (AB)
//	{
//		//DEGREE - DAYS ABOVE
//		MN = MIN;
//		MX = MAX;
//	}
//	else                             //DEGREE - DAYS BELOW(USE MIRROR IMAGE)
//	{
//		MN = THRESH - (MAX - THRESH);
//		MX = THRESH + (THRESH - MIN);
//	}
//
//	//                  COMPUTE DEGREE - DAYS
//
//	if (MN < MX)
//	{
//		if (MX < THRESH) return;
//
//		float TM = 0.5 * (MX + MN);
//		float TA = 0.5 * (MX - MN);
//
//		float ARG = (THRESH - TM) / TA;
//		if (ARG > 1.0) ARG = 1.0;
//		if (ARG < -1.0) ARG = -1.0;
//
//		//            APPROXIMATE VALUE OF THETA AT ARG
//
//		float XA = abs(ARG);
//		float Q = 1.57079632 - sqrt(1.0 - XA) * (1.5707288 + XA * (-0.2121144 + XA * (0.0745610 - XA * 0.0187293)));
//		Q = abs(Q);
//		if (ARG < 0) Q = -Q;
//
//		float THETA = Q;               //THETA = ARCSIN(ARG)
//		THETA = THETA + 1.57079632;		//THETA = ARCCOS(ARG)
//
//		float THETA2 = acos(ARG);
//
//		DDAY = ((TM - THRESH) * (PI - THETA) + TA * sin(THETA)) / PI;
//	}
//	else
//	{
//		if (MN == MX)
//		{
//			if (MX > THRESH)
//			{
//				DDAY = MX - THRESH;
//			}
//		}
//		else  //MN.GT.MX
//		{
//			Error = 1;
//		}
//	}
//}



//I'm the developer of BioSIM (insect development simulator) at Québec city. I want to create a model to simulate the phenology of the Whitemarqued Tussock Moth (WMTM). Do you have some information about phenology of this insect for example development rate in function of temperature. Any information can help me in the creation of the model. Thanks
