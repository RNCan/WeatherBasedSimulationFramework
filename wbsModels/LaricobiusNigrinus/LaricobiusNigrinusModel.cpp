//***********************************************************
// 07/03/2019	1.0.0	Rémi Saint-Amant   Creation
//***********************************************************
#include "LaricobiusNigrinusModel.h"
#include "ModelBase/EntryPoint.h"
#include "Basic\DegreeDays.h"
#include <boost/math/distributions/weibull.hpp>
#include <boost/math/distributions/beta.hpp>
#include <boost/math/distributions/Rayleigh.hpp>
#include <boost/math/distributions/logistic.hpp>
#include <boost/math/distributions/exponential.hpp>


using namespace WBSF::HOURLY_DATA;
using namespace WBSF::LNF;
using namespace std;

//static const bool BEGIN_NOVEMBER = false;
//static const size_t FIRST_Y = BEGIN_NOVEMBER ? 1 : 0;

namespace WBSF
{
	static const CDegreeDays::TDailyMethod DD_METHOD = CDegreeDays::MODIFIED_ALLEN_WAVE;
	enum { ACTUAL_CDD, DATE_DD717, DIFF_DAY, NB_OUTPUTS };

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CLaricobiusNigrinusModel::CreateObject);

	CLaricobiusNigrinusModel::CLaricobiusNigrinusModel()
	{
		//NB_INPUT_PARAMETER is used to determine if the dll
		//uses the same number of parameters than the model interface
		NB_INPUT_PARAMETER = -1;
		VERSION = "1.0.2 (2020)";

		//		m_start = CTRef(YEAR_NOT_INIT, JANUARY, DAY_01);
			//	m_threshold = 5.6;
				//m_sumDD = 540;

		m_bCumul = false;
		for (size_t s = 0; s < NB_STAGES; s++)
		{
			for (size_t p = 0; p < NB_RDR_PARAMS; p++)
			{
				m_RDR[s][p] = CLaricobiusNigrinusEquations::RDR[s][p];
			}
		}

		for (size_t p = 0; p < NB_OVP_PARAMS; p++)
		{
			m_OVP[p] = CLaricobiusNigrinusEquations::OVP[p];
		}

		for (size_t p = 0; p < NB_ADE_PARAMS; p++)
			m_ADE[p] = CLaricobiusNigrinusEquations::ADE[p];
	}

	CLaricobiusNigrinusModel::~CLaricobiusNigrinusModel()
	{
	}


	//this method is call to load your parameter in your variable
	ERMsg CLaricobiusNigrinusModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		size_t c = 0;

		m_bCumul = parameters[c++].GetBool();

		if (parameters.size() == 1 + NB_STAGES * NB_RDR_PARAMS + NB_OVP_PARAMS + NB_ADE_PARAMS)
		{
			for (size_t s = 0; s < NB_STAGES; s++)
			{
				for (size_t p = 0; p < NB_RDR_PARAMS; p++)
				{
					m_RDR[s][p] = parameters[c++].GetFloat();
				}
			}

			for (size_t p = 0; p < NB_OVP_PARAMS; p++)
			{
				m_OVP[p] = parameters[c++].GetFloat();
			}

			for (size_t p = 0; p < NB_ADE_PARAMS; p++)
				m_ADE[p] = parameters[c++].GetFloat();
		}

		return msg;
	}





	/*ERMsg CLaricobiusNigrinusModel::OnExecuteAnnual()
	{
		_ASSERTE(m_weather.size() > 1);

		ERMsg msg;
		CTRef today = CTRef::GetCurrentTRef();

		CTPeriod outputPeriod = m_weather.GetEntireTPeriod(CTM::ANNUAL);
		m_output.Init(outputPeriod, NB_OUTPUTS);


		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			int year = m_weather[y].GetTRef().GetYear();
			CTRef begin = CTRef(year, m_start.GetMonth(), m_start.GetDay());
			CTRef end = CTRef(year, DECEMBER, DAY_31);

			double CDD = 0;

			CTRef day717;
			double actualCDD = -999;
			CDegreeDays DD(DD_METHOD, m_threshold);

			for (CTRef d = begin; d < end; d++)
			{
				CDD += DD.GetDD(m_weather.GetDay(d));
				if (CDD >= m_sumDD && !day717.IsInit())
					day717 = d;

				if (d.as(CTM(CTM::DAILY, CTM::OVERALL_YEARS)) == today.as(CTM(CTM::DAILY, CTM::OVERALL_YEARS)))
					actualCDD = CDD;
			}

			m_output[y][ACTUAL_CDD] = actualCDD;
			if (day717.IsInit())
			{
				m_output[y][DATE_DD717] = day717.GetRef();
				m_output[y][DIFF_DAY] = (int)day717.GetJDay() - (int)today.GetJDay();
			}
		}

		return msg;
	}*/

	//This method is called to compute the solution
	ERMsg CLaricobiusNigrinusModel::OnExecuteDaily()
	{
		ERMsg msg;

		if (m_weather.GetNbYears() < 2)
		{
			msg.ajoute("Laricobius nigrinus model need at least 2 years of data");
			return msg;
		}

		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();

		//This is where the model is actually executed
		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));
		m_output.Init(p, NB_STATS, 0);

		//we simulate 2 years at a time. 
		//we also manager the possibility to have only one year
		for (size_t y = 1; y < m_weather.size(); y++)
		{
			ExecuteDaily(m_weather[y].GetTRef().GetYear(), m_weather, m_output);
		}

		return msg;
	}

	void CLaricobiusNigrinusModel::ExecuteDaily(int year, const CWeatherYears& weather, CModelStatVector& output)
	{
		//Create stand
		CLNFStand stand(this, m_OVP[Τᴴ¹], m_OVP[Τᴴ²] );

		//Set parameters to equation
		for (size_t s = 0; s < NB_STAGES; s++)
		{
			for (size_t p = 0; p < NB_RDR_PARAMS; p++)
			{
				stand.m_equations.m_RDR[s][p] = m_RDR[s][p];
			}
		}


		for (size_t p = 0; p < NB_OVP_PARAMS; p++)
			stand.m_equations.m_OVP[p] = m_OVP[p];


		for (size_t p = 0; p < NB_ADE_PARAMS; p++)
			stand.m_equations.m_ADE[p] = m_ADE[p];


		stand.init(year - 1, weather);

		//compute 30 days avg
		//stand.ComputeTavg30(year, weather);


		//Create host
		CLNFHostPtr pHost(new CLNFHost(&stand));

		pHost->m_nbMinObjects = 10;
		pHost->m_nbMaxObjects = 1000;


		pHost->Initialize<CLaricobiusNigrinus>(CInitialPopulation(400, 100, EGG));

		//add host to stand			
		stand.m_host.push_front(pHost);

		CTPeriod p = weather[year].GetEntireTPeriod(CTM(CTM::DAILY));

		//if have other year extend period to February
		//ASSERT(weather[year].HavePrevious());
		//if (BEGIN_NOVEMBER)
		//{
		//	p.Begin() = CTRef(year - 1, NOVEMBER, DAY_01);
		//}

		//if have other year extend period to February
		if (weather[year].HavePrevious())
			p.Begin() = CTRef(year - 1, JULY, DAY_01);


		//if have other year extend period to February
		if (weather[year].HaveNext())
			p.End() = CTRef(year + 1, JUNE, DAY_30);


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
			for (size_t s = S_EGG; s < S_AESTIVAL_DIAPAUSE_ADULT; s++)
			{
				CTPeriod p = weather[year].GetEntireTPeriod(CTM(CTM::DAILY));

				CStatistic stat = output.GetStat(s, p);
				if (stat.IsInit() && stat[SUM] > 0)
				{
					for (CTRef d = p.Begin() + 1; d <= p.End(); d++)
					{
						output[d][s] = output[d - 1][s] + output[d][s] * 100 / stat[SUM];
						_ASSERTE(!_isnan(output[d][s]));
					}
				}
			}

		}
	}


	void CLaricobiusNigrinusModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		ASSERT(data.size() == 5);

		CSAResult obs;
		//if (data[0] != "BlacksburgLab" && data[0] != "VictoriaLab")
		//if (data[0] != "VictoriaLab")
		//{
		CStatistic egg_creation_date;


		obs.m_ref.FromFormatedString(data[1]);
		obs.m_obs.resize(NB_INPUTS);
		for (size_t i = 0; i < NB_INPUTS; i++)
		{
			obs.m_obs[i] = stod(data[i + 2]);

			if (i == 0 && obs.m_obs[i] > -999)
				m_egg_creation_date[data[0] + "_" + to_string(obs.m_ref.GetYear())] += obs.m_ref.GetJDay();

			//if (i == 1 && obs.m_obs[i] <= -999 && stod(data[i + 5]) > -999)
				//obs.m_obs[i] = stod(data[i + 5]);//second method

			if (obs.m_obs[i] > -999)
			{
				m_nb_days[i] += obs.m_ref.GetJDay();
				m_years[i].insert(obs.m_ref.GetYear());
			}
		}

		m_SAResult.push_back(obs);


		//}


	}

	double GetSimX(size_t s, CTRef TRefO, double obs, const CModelStatVector& output)
	{
		double x = -999;

		if (obs > -999)
		{
			//if (obs > 0.01 && obs < 99.99)
			if (obs >= 100)
				obs = 99.99;//to avoid some problem of truncation

			long index = output.GetFirstIndex(s, ">=", obs, 1, CTPeriod(TRefO.GetYear(), FIRST_MONTH, FIRST_DAY, TRefO.GetYear(), LAST_MONTH, LAST_DAY));
			if (index >= 1)
			{
				double obsX1 = output.GetFirstTRef().GetJDay() + index;
				double obsX2 = output.GetFirstTRef().GetJDay() + index + 1;

				double obsY1 = output[index][s];
				double obsY2 = output[index + 1][s];
				if (obsY2 != obsY1)
				{
					double slope = (obsX2 - obsX1) / (obsY2 - obsY1);
					double obsX = obsX1 + (obs - obsY1)*slope;
					ASSERT(!_isnan(obsX) && _finite(obsX));

					x = obsX;
				}
			}
		}

		return x;
	}

	bool CLaricobiusNigrinusModel::IsParamValid()const
	{
		if (m_OVP[Τᴴ¹] >= m_OVP[Τᴴ²])
			return false;


		bool bValid = true;
		for (size_t s = 0; s <= NB_STAGES && bValid; s++)
		{
			if (s == EGG || s == LARVAE /*|| s == AESTIVAL_DIAPAUSE_ADULT*/)
			{
				CStatistic rL;
				for (double Э = 0.01; Э < 0.5; Э += 0.01)
				{
					double r = 1.0 - log((pow(Э, m_RDR[s][Ϙ]) - 1.0) / (pow(0.5, m_RDR[s][Ϙ]) - 1.0)) / m_RDR[s][к];
					if (r >= 0.4 && r <= 2.5)
						rL += 1.0 / r;//reverse for comparison
				}

				CStatistic rH;
				for (double Э = 0.51; Э < 1.0; Э += 0.01)
				{
					double r = 1.0 - log((pow(Э, m_RDR[s][Ϙ]) - 1.0) / (pow(0.5, m_RDR[s][Ϙ]) - 1.0)) / m_RDR[s][к];
					if (r >= 0.4 && r <= 2.5)
						rH += r;
				}

				if (rL.IsInit() && rH.IsInit())
					bValid = fabs(rL[SUM] - rH[SUM]) < 5.3; //in Régnière (2012) obtain a max of 5.3
				else
					bValid = false;
			}
		}

		return bValid;
	}
	


	void CLaricobiusNigrinusModel::CalibrateDiapauseEndTh(CStatisticXY& stat)
	{
		static const double DiapauseDuration[3][3] =
		{
			{128.1,127.9,134.0},
			{156.7,162.2,166.2},
			{194.8,203.7,-999}
		};

		static const double DiapauseDurationSD[3][3] =
		{
			{2.2,3,	3.8},
			{4.1,4.4,5.4},
			{4.2,3.4,10.8}
		};

		if (m_SAResult.size() != 8)
			return;


		for (size_t t = 0; t < 3; t++)
		{
			for (size_t dl = 0; dl < 3; dl++)
			{
				if (DiapauseDuration[t][dl] > -999)
				{
					//NbVal = 8	Bias = 0.00263	MAE = 0.95222	RMSE = 1.25691	CD = 0.99785	R² = 0.99786
					//lam0 = 15.81011 {  15.80907, 15.81142}	VM = { 0.00021,   0.00060 }
					//lam1 = 2.50857 {   2.50779, 2.50943}	VM = { 0.00021,   0.00073 }
					//lam2 = 6.64395 {   6.63745, 6.64922}	VM = { 0.00113,   0.00379 }
					//lam3 = 7.81911 {   7.80857, 7.82666}	VM = { 0.00183,   0.00492 }
					//lam_a = 0.16346 {   0.16328, 0.16369}	VM = { 0.00006,   0.00019 }
					//lam_b = 0.26484 {   0.26458, 0.26499}	VM = { 0.00007,   0.00020 }

					double T = 10 + 5 * t;
					double DL = 8 + dl * 4;
					double DD = 120.0 + (215.0 - 120.0) * 1 / (1 + exp(-(T - m_ADE[ʎ0]) / m_ADE[ʎ1]));
					double f = exp(-m_ADE[ʎa] + m_ADE[ʎb] * 1 / (1 + exp(-(DL - m_ADE[ʎ2]) / m_ADE[ʎ3])));

					stat.Add(DiapauseDuration[t][dl], DD * f);
				}
			}
		}
	}



	static const int ROUND_VAL = 4;
	CTRef CLaricobiusNigrinusModel::GetEmergingBegin(const CWeatherYear& weather)
	{
		CTPeriod p = weather.GetEntireTPeriod(CTM(CTM::DAILY));

		double sumDD = 0;
		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
		{
			size_t ii = TRef - p.Begin();
			const CWeatherDay& wday = m_weather.GetDay(TRef);
			double T = wday[H_TNTX][MEAN];

			T = CLaricobiusNigrinus::AdjustTLab(wday.GetWeatherStation()->m_name, NOT_INIT, wday.GetTRef(), T);
			T = Round(max(m_ADE[ʎa], T), ROUND_VAL);

			double DD = min(0.0, T - m_ADE[ʎb]);//DD is negative

			if (ii < m_ADE[ʎ0])
				sumDD += DD;
		}


		boost::math::logistic_distribution<double> begin_dist(m_ADE[ʎ2], m_ADE[ʎ3]);
		int begin = (int)Round(m_ADE[ʎ0] + m_ADE[ʎ1] * cdf(begin_dist, sumDD), 0);
		return  p.Begin() + begin;



	}


	void CLaricobiusNigrinusModel::CalibrateDiapauseEnd(CStatisticXY& stat)
	{
		const size_t EVALUATE_STAGE = I_EGGS;
		//const size_t EVALUATE_STAGE = I_LARVAE;
		//const size_t EVALUATE_STAGE = I_EMERGED_ADULT;

		if (m_OVP[Τᴴ¹] >= m_OVP[Τᴴ²])
			return ;


		if (m_SAResult.empty())
			return;

		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();



		for (size_t y = 1; y < m_weather.GetNbYears(); y++)
		{
			int year = m_weather[y].GetTRef().GetYear();
			if (m_years[EVALUATE_STAGE].find(year) == m_years[EVALUATE_STAGE].end())
				continue;



			double sumDD = 0;
			vector<double> CDD;
			CTPeriod p;

			if (EVALUATE_STAGE == I_EMERGED_ADULT)
			{

				p = m_weather[year].GetEntireTPeriod(CTM(CTM::DAILY));
				CDD.resize(p.size(), 0);

				CTRef emergingBegin = GetEmergingBegin(m_weather[year]);
				for (CTRef TRef = emergingBegin; TRef <= p.End(); TRef++)
				{
					const CWeatherDay& wday = m_weather.GetDay(TRef);
					double T = wday[H_TNTX][MEAN];
					T = CLaricobiusNigrinus::AdjustTLab(wday.GetWeatherStation()->m_name, NOT_INIT, wday.GetTRef(), T);

					double DD = max(0.0, T - 4.0);//DD is negative
					sumDD += DD;

					size_t ii = TRef - p.Begin();
					CDD[ii] = sumDD;
				}



				//for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
				//{

				//	//const CWeatherDay& wday = m_weather.GetDay(TRef);

				//	////double T = wday[H_TMIN][MEAN];
				//	//double T = wday[H_TNTX][MEAN];
				//	////double TT = max(10.0, min(20.0, wday[H_TNTX][MEAN]));
				//	//double day_length = wday.GetDayLength() / 3600.0;
				//	////double DD1 = (1 - 1 / (1 + exp(-(TT - m_ADE[ʎ2]) / m_ADE[ʎ3])));

				//	////double TT = max(thresLo, min(thresHi, T));
				//	////double DD1 = 1 - 1 / (1 + exp(-(T - m_ADE[ʎ0]) / m_ADE[ʎ1]));
				//	////double DD2 = 1 - 1 / (1 + exp(-(day_length - m_ADE[ʎ0]) / m_ADE[ʎ1]));

				//	////double DL = max(12, min(14, day_length));
				//	////double DD2 = 1 / (1 + exp(-(day_length - m_ADE[ʎ2]) / m_ADE[ʎ3]));
				//	////double thresLo = m_ADE[ʎ0];
				//	////double thresHi = m_ADE[ʎ1] * (1+DD2);

				//	////double TT = max(thresLo, min(thresHi, T));
				//	////double DD1 = 1 - (TT - thresLo) / (thresHi - thresLo);

				//	////double To = m_ADE[ʎ0] + m_ADE[ʎb]*(-0.5+ 1 / (1 + exp(-(day_length - m_ADE[ʎ2]) / m_ADE[ʎ3])));
				//	//double DD1 = 1-1 / (1 + exp(-(T - m_ADE[ʎ0]) / m_ADE[ʎ1]));
				//	//double DD2 = 1 - 1 / (1 + exp(-(day_length - m_ADE[ʎ2]) / m_ADE[ʎ3]));


				//	////size_t ii = TRef - CTRef(year, JANUARY, DAY_01);
				//	//size_t ii = TRef - p.Begin();
				//	//if (ii > m_ADE[ʎa])
				//	//	sumDD += /*m_ADE[ʎb] + */DD1*(m_ADE[ʎb]);


				//	//CDD[ii] = sumDD;










				//	//size_t ii = TRef - p.Begin();
				//	//if (ii >= 182)
				//	//{
				//	//	CStatistic Tavf30;
				//	//	for (CTRef TRef2 = TRef - int(m_ADE[ʎb]); TRef2 <= TRef; TRef2++)
				//	//	{
				//	//		const CWeatherDay& wday = m_weather.GetDay(TRef2);
				//	//		//double day_length = wday.GetDayLength() / 3600.0;

				//	//		//double T = wday[H_TNTX][MEAN];
				//	//		double T = wday[H_TMIN][MEAN];
				//	//		//double f = exp(m_ADE[ʎ0] + m_ADE[ʎ1] * 1.0 / (1.0 + exp(-(day_length - m_ADE[ʎ2]) / m_ADE[ʎ3])));//day length factor
				//	//		Tavf30 += T;
				//	//	}


				//	//	double day_length = m_weather.GetDay(TRef).GetDayLength() / 3600.0;
				//	//	double f = day_length*(1 - 2 * 1.0 / (1.0 + exp(-(ii - m_ADE[ʎ2]) / m_ADE[ʎ3])));//day factor
				//	//	//double f = exp((-1.0 + 2.0 / (1.0 + exp(-(day_length - m_ADE[ʎ0]) / m_ADE[ʎ1])))) * exp((1.0 - 2.0 / (1.0 + exp(-(ii - m_ADE[ʎ2]) / m_ADE[ʎ3]))));//day factor

				//	//	if (ii == 182)
				//	//		CDD[ii] = Tavf30[MEAN] + f;
				//	//	else
				//	//		CDD[ii] = min(CDD[ii - 1], Tavf30[MEAN] + f);

				//	//}



				//	//size_t ii = TRef - p.Begin();
				//	//const CWeatherDay& wday = m_weather.GetDay(TRef);
				//	//double day_length = Round(wday.GetDayLength() / 3600.0,1);
				//	//day_length = AdjustDLLab(wday.GetWeatherStation()->m_name, wday.GetTRef(), day_length);



				//	//float threshold = Round(m_ADE[ʎ0] +m_ADE[ʎ1] * 1 / (1 + exp(-(day_length - m_ADE[ʎ2]) / m_ADE[ʎ3])), 1);

				//	//float T = Round(wday[H_TNTX][MEAN],1);
				//	//T = AdjustTLab(wday.GetWeatherStation()->m_name, wday.GetTRef(), T);
				//	//float DD = max(0.0f, threshold - T);//DD can be negative
				//	//ASSERT(DD >= 0);
				//	//
				//	//if (ii >= m_ADE[ʎa])
				//	//	sumDD += m_ADE[ʎb] + DD;

				//	//CDD[ii] = Round(sumDD,1);


				//	size_t ii = TRef - p.Begin();
				//	const CWeatherDay& wday = m_weather.GetDay(TRef);
				//	double T = wday[H_TNTX][MEAN];

				//	T = AdjustTLab(wday.GetWeatherStation()->m_name, wday.GetTRef(), T);

				//	//double day_length = Round(wday.GetDayLength() / 3600.0, 1);
				//	//day_length = AdjustDLLab(wday.GetWeatherStation()->m_name, wday.GetTRef(), day_length);
				//	//double dlr = day_length / 16;
				//	//T *= dlr;
				//	T = Round(max(m_ADE[ʎa], T), ROUND_VAL);

				//	double DD = min(0.0, T - m_ADE[ʎb]);//DD is negative

				//	if (ii < m_ADE[ʎ0])
				//		sumDD += DD;


				//}

			}
			else
			{
				p = m_weather[year].GetEntireTPeriod(CTM(CTM::DAILY));
				//p.Begin() = GetEmergingBegin(m_weather[year - 1]);

				CDD.resize(p.size(), 0);

				CDegreeDays DDModel(CDegreeDays::MODIFIED_ALLEN_WAVE, m_OVP[Τᴴ¹], m_OVP[Τᴴ²]);

				for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
				{
					const CWeatherDay& wday = m_weather.GetDay(TRef);
					size_t ii = TRef - p.Begin();
					sumDD += DDModel.GetDD(wday);
					CDD[ii] = sumDD;
				}
			}



			for (size_t i = 0; i < m_SAResult.size(); i++)
			{
				size_t ii = m_SAResult[i].m_ref - p.Begin();
				if (m_SAResult[i].m_ref.GetYear() == year && ii < CDD.size())
				{
					double obs_y = m_SAResult[i].m_obs[EVALUATE_STAGE];

					if (obs_y > -999)
					{

						double sim_y = 0;

						if (EVALUATE_STAGE == I_EMERGED_ADULT)
						{
							double sumDD = CDD[ii];

							//boost::math::logistic_distribution<double> emerged_dist(m_ADE[μ], m_ADE[ѕ]);
							boost::math::weibull_distribution<double> emerged_dist(m_ADE[μ], m_ADE[ѕ]);
							sim_y = Round(cdf(emerged_dist, sumDD) * 100, 1);

						}
						else
						{
							boost::math::logistic_distribution<double> create_dist(m_OVP[μ], m_OVP[ѕ]);
							//boost::math::weibull_distribution<double> create_dist(m_OVP[μ], m_OVP[ѕ]);
							sim_y = Round(cdf(create_dist, CDD[ii]) * 100, 1);

						}


						if (sim_y < 0.1)
							sim_y = 0;
						if (sim_y > 99.9)
							sim_y = 100;

						stat.Add(obs_y, sim_y);
					}
				}
			}

		}
		return;

	}


	void CLaricobiusNigrinusModel::CalibrateOviposition(CStatisticXY& stat)
	{
		if (m_SAResult.empty())
			return;

		for (size_t y = 1; y < m_weather.GetNbYears(); y++)
		{
			int year = m_weather[y].GetTRef().GetYear();
			string key = m_info.m_loc.m_ID + "_" + to_string(year);

			if (m_egg_creation_date.find(key) == m_egg_creation_date.end())
				continue;

			//CTRef emergingBegin = GetEmergingBegin(m_weather[year-1]);


			ASSERT(m_weather[year].HavePrevious());

			if (m_weather[year].HavePrevious())
			{
				CStatistic Tmin;
				Tmin += m_weather[year - 1][NOVEMBER][H_TMIN];
				Tmin += m_weather[year - 1][DECEMBER][H_TMIN];
				Tmin += m_weather[year][JANUARY][H_TMIN];
				Tmin += m_weather[year][FEBRUARY][H_TMIN];

				CStatistic Tmean;
				Tmean += m_weather[year - 1][NOVEMBER][H_TNTX];
				Tmean += m_weather[year - 1][DECEMBER][H_TNTX];
				Tmean += m_weather[year][JANUARY][H_TNTX];
				Tmean += m_weather[year][FEBRUARY][H_TNTX];


				double obs = m_egg_creation_date.at(key);
				double sim = m_ADE[ʎa] - m_ADE[ʎb] * 1 / (1 + exp(-(Tmean[MEAN] - m_ADE[ʎ2]) / m_ADE[ʎ3]));

				if (sim < 0.1)
					sim = 0;
				if (sim > 99.9)
					sim = 100;

				stat.Add(obs, sim);
			}


		}
		return;

	}

	void CLaricobiusNigrinusModel::GetFValueDaily(CStatisticXY& stat)
	{
		bitset<3> test;
		test.reset();

		test.set(I_EGGS);
		test.set(I_LARVAE);
		//test.set(I_EMERGED_ADULT);

		//return CalibrateDiapauseEndTh(stat);
		//return CalibrateDiapauseEnd(stat); 
		//return CalibrateOviposition(stat);

		if (!m_SAResult.empty())
		{
			if (!m_bCumul)
				m_bCumul = true;//SA always cumulative

			if (!m_weather.IsHourly())
				m_weather.ComputeHourlyVariables();

			//low and hi relative development rate must be approximatively the same
			//if (!IsParamValid())
				//return;

			

			for (size_t y = 1; y < m_weather.GetNbYears(); y++)
			{
				int year = m_weather[y].GetTRef().GetYear();
				if ( (test[0] && m_years[I_EGGS].find(year) != m_years[I_EGGS].end() ) ||
					(test[1] && m_years[I_LARVAE].find(year) != m_years[I_LARVAE].end()) ||
					(test[2] && m_years[I_EMERGED_ADULT].find(year) != m_years[I_EMERGED_ADULT].end()))
				{

					CModelStatVector output;
					CTPeriod p = m_weather[y].GetEntireTPeriod(CTM(CTM::DAILY));
					//not possible to add a second year without having problem in evaluation....
					//if (m_weather[y].HaveNext())
						//p.End() = m_weather[y + 1].GetEntireTPeriod(CTM(CTM::DAILY)).End();

					output.Init(p, NB_STATS, 0);
					ExecuteDaily(m_weather[y].GetTRef().GetYear(), m_weather, output);

					static const size_t STAT_STAGE[3] = { S_EGG, S_LARVAE, S_ACTIVE_ADULT };

					for (size_t i = 0; i < m_SAResult.size(); i++)
					{
						if (output.IsInside(m_SAResult[i].m_ref))
						{

							for (size_t j = 0; j < NB_INPUTS; j++)
							{
								if (test[j])
								{
									double obs_y = Round(m_SAResult[i].m_obs[j],2);
									double sim_y = Round(output[m_SAResult[i].m_ref][STAT_STAGE[j]], 2);

									if (obs_y > -999)
									{
										stat.Add(obs_y, sim_y);

										double obs_x = m_SAResult[i].m_ref.GetJDay();
										double sim_x = GetSimX(STAT_STAGE[j], m_SAResult[i].m_ref, obs_y, output);

										/*if (sim_x > -999)
										{
											obs_x = Round(100 * (obs_x - m_nb_days[j][LOWEST]) / m_nb_days[j][RANGE],2);
											sim_x = Round(100 * (sim_x - m_nb_days[j][LOWEST]) / m_nb_days[j][RANGE],2);
											stat.Add(obs_x, sim_x);
										}*/
									}
								}
							}
						}
					}//for all results
				}//have data
			}
		}
	}
}
