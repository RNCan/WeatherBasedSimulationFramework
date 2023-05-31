//*********************************************************************
//17/04/2019	1.0.0	Rémi Saint-Amant	Creation
//*********************************************************************
#include "BudBurst.h"
#include "Basic/WeatherDefine.h"
#include "Basic/DegreeDays.h"
#include "ModelBase/EntryPoint.h"
#include "ModelBase/SimulatedAnnealingVector.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;

namespace WBSF
{

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CBudBurst::CreateObject);

	enum TDailyOutput { O_D_DD, O_D_BB, NB_DAILY_OUTPUTS };
	enum TOutput { O_A_DC, O_A_SW, O_A_BUDBURST, NB_ANNUAL_OUTPUTS };


	CBudBurst::CBudBurst()
	{
		// initialize your variable here (optional)
		NB_INPUT_PARAMETER = -1;
		VERSION = "2.0.1 (2023)";



		m_Sw = 16.891;
		m_α2 = -0.0675;
		m_thresholdT = 10;
		m_thresholdCD = 150;


	}

	CBudBurst::~CBudBurst()
	{
	}


	//this method is call to load your parameter in your variable
	ERMsg CBudBurst::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		//transfer your parameter here
		short c = 0;
		m_species = parameters[c++].GetInt();
		if (m_species < NB_SPECIES)
		{

			static std::array<std::array<double, 5>, 5> P =
			{ {
				{ 73, -2.5, 12.5, 436.7, 30.62},//bf
				{ 73, -2.5, 12.5, 475.1, 25.22},//ws
				{ 73, -2.5, 12.5, 502.5, 57.25},//bs
				{ 73, -2.5, 12.5, 600.1, 36.49},//ns
				{ 0, 10, 150,694.2, 44.02}//maple
			} };






			m_beginDOY = P[m_species][0];
			m_thresholdT = P[m_species][1];
			m_thresholdCD = P[m_species][2];
			m_Sw = P[m_species][3];
			m_α2 = P[m_species][4];



			if (parameters.size() == 6)
			{
				m_beginDOY = parameters[c++].GetInt() - 1;
				m_thresholdT = parameters[c++].GetReal();
				m_thresholdCD = parameters[c++].GetReal();
				m_Sw = parameters[c++].GetReal();
				m_α2 = parameters[c++].GetReal();
			}
		}
		else
		{
			msg.ajoute("Invalid species");
		}


		return msg;
	}


	//This method is call to compute solution
	ERMsg CBudBurst::OnExecuteAnnual()
	{
		ERMsg msg;

		if (m_species == S_MAPLE)
		{
			msg = OnExecuteAnnualMaple();
		}
		else
		{
			OnExecuteAnnualOther(m_weather, m_output);
			/*m_output.Init(m_weather.GetEntireTPeriod(CTM::ANNUAL), NB_ANNUAL_OUTPUTS);

			for (size_t y = 0; y < m_weather.GetNbYears(); y++)
			{
				CModelStatVector output;
				CTRef BB = ExecuteDaily(m_weather[y], output);

				if (BB.IsInit())
				{
					m_output[y][O_A_BUDBURST] = BB.GetRef();
				}
			}*/

		}

		return msg;
	}

	ERMsg CBudBurst::OnExecuteAnnualMaple()
	{
		ERMsg msg;

		CTPeriod pp(m_weather.GetEntireTPeriod(CTM::ANNUAL));
		pp.Begin().m_year++;
		m_output.Init(pp, 3);


		static const double Sw150 = 16.891;
		static const double α2 = -0.0675;

		for (size_t y = 0; y < m_weather.GetNbYears() - 1; y++)
		{
			CTRef budBurst;
			double sum = 0;
			int dc = 0;

			int year = m_weather[y].GetTRef().GetYear();
			CTPeriod p(CTRef(year, DECEMBER, DAY_01), CTRef(year + 1, DECEMBER, DAY_01));
			for (CTRef TRef = p.Begin(); TRef < p.End() && !budBurst.IsInit(); TRef++)
			{
				const CWeatherDay& wDay = m_weather.GetDay(TRef);

				if (wDay[H_TAIR][MEAN] < 10)
					dc++;
				else
					sum += wDay[H_TAIR][MEAN] - 10;

				double Sw = Sw150 * exp(α2 * (dc - 150));
				if (sum >= Sw)
				{
					budBurst = TRef;
				}
			}

			if (budBurst.IsInit())
			{
				m_output[y][O_A_DC] = dc;
				m_output[y][O_A_SW] = Sw150 * exp(α2 * (dc - 150.0));
				m_output[y][O_A_BUDBURST] = budBurst.GetRef();
			}
		}


		return msg;
	}

	bool CBudBurst::OnExecuteAnnualOther(CWeatherStation& weather, CModelStatVector& output)
	{



		CTPeriod pp(weather.GetEntireTPeriod(CTM::ANNUAL));
		output.Init(pp, NB_ANNUAL_OUTPUTS); 


		//for (size_t y = 0; y < weather.GetNbYears() - 1; y++)
		//{
			//CTRef budBurst;
			//double sum = 0;
			//int CD = 0; //Chilling days

			//int year = weather[y].GetTRef().GetYear();
			//CTPeriod p(CJDayRef(year, m_beginDOY), CJDayRef(year + 1, m_beginDOY - 1));

			//for (CTRef TRef = p.Begin(); TRef <= p.End() && !budBurst.IsInit(); TRef++)
			//{
			//	const CWeatherDay& wDay = weather.GetDay(TRef);

			//	if (wDay[H_TAIR][MEAN] < m_thresholdT)
			//		CD++;
			//	else
			//		sum += wDay[H_TAIR][MEAN] - m_thresholdT;

			//	double Sw = m_Sw * exp(m_α2 * (CD - m_thresholdCD));
			//	if (sum >= Sw)
			//	{
			//		budBurst = TRef;
			//		output[y][O_A_DC] = CD;
			//		output[y][O_A_SW] = Sw;
			//		output[y][O_A_BUDBURST] = budBurst.GetRef();
			//	}




		//}

		CDegreeDays DDmodel(CDegreeDays::ALLEN_WAVE, m_thresholdT, m_thresholdCD);
		CModelStatVector DD;
		DDmodel.Execute(weather, DD);


		for (size_t y = 0; y < weather.GetNbYears(); y++)
		{
			CTRef budBurst;
			double CDD = 0;

			//int year = weather.GetTRef().GetYear();
			CTPeriod p = weather[y].GetEntireTPeriod(CTM::DAILY);

			for (CTRef TRef = p.Begin(); TRef <= p.End() & !budBurst.IsInit(); TRef++)
			{
				const CWeatherDay& wDay = weather[y].GetDay(TRef);

				
				if (TRef.GetJDay() >= m_beginDOY)
					CDD += DD[TRef][0];

				//double BB = 1 / (1 + exp(-(CDD - m_Sw) / m_α2));
				//output[TRef][O_D_DD] = CDD;
				//output[TRef][O_D_BB] = BB;

				if (CDD >= m_Sw)
				{
					budBurst = TRef;
				}
			}

			if (!budBurst.IsInit())
				return false;


			//double pD = 1 - (CDD - m_Sw) / (DD[budBurst][0]);
			double pD = ( m_Sw - (CDD- DD[budBurst][0])) / (DD[budBurst][0]);
			ASSERT(pD >= 0 && pD <= 1);
			//output[y][O_A_DC] = budBurst.GetJDay() + 1 + pD;
			output[y][O_A_DC] = budBurst.GetJDay() + 1 - 1 + pD;



			output[y][O_A_BUDBURST] = budBurst.GetRef();
		}






		return true;
	}


	ERMsg CBudBurst::OnExecuteDaily()
	{
		ERMsg msg;
		//		if (!m_weather.IsHourly())
			//		m_weather.ComputeHourlyVariables();

		m_output.Init(m_weather.GetEntireTPeriod(), NB_DAILY_OUTPUTS);

		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			ExecuteDaily(m_weather[y], m_output);
		}

		return msg;
	}


	CTRef CBudBurst::ExecuteDaily(CWeatherYear& weather, CModelStatVector& output)
	{
		CTPeriod pp(weather.GetEntireTPeriod(CTM::DAILY));
		//pp.Begin().m_year++;

		if (output.empty())
			output.Init(pp, NB_DAILY_OUTPUTS);


		CDegreeDays DDmodel(CDegreeDays::ALLEN_WAVE, m_thresholdT, m_thresholdCD);
		CModelStatVector DD;
		DDmodel.Execute(weather, DD);


		CTRef budBurst;
		double CDD = 0;


		int year = weather.GetTRef().GetYear();
		CTPeriod p = weather.GetEntireTPeriod(CTM::DAILY);

		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
		{
			const CWeatherDay& wDay = weather.GetDay(TRef);

			if (TRef.GetJDay() >= m_beginDOY)
				CDD += DD[TRef][0];

			double BB = 1 / (1 + exp(-(CDD - m_Sw) / m_α2));
			output[TRef][O_D_DD] = CDD;
			output[TRef][O_D_BB] = BB;

			if (BB >= 0.5 && !budBurst.IsInit())
			{
				budBurst = TRef;
			}
		}


		return budBurst;
	}


	enum { I_D_SPECIES, I_D_SOURCE, I_D_SITE, I_D_LATITUDE, I_D_LONGITUDE, I_D_ELEVATION, I_D_TYPE, I_D_PROVINCE, I_D_YEAR, I_D_DOY, I_D_BB0, I_D_BB1, NB_INPUTS_DAILY };

	void CBudBurst::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		static const char* SPECIES_NAME[] = { "bf", "ws", "bs", "ns" };
		if (data.size() == NB_INPUTS_DAILY)
		{
			if (data[I_D_SPECIES] == SPECIES_NAME[m_species] && data[I_D_TYPE] == "C")
			{
				CSAResult obs;

				obs.m_ref = CJDayRef(stoi(data[I_D_YEAR]), stoi(data[I_D_DOY]) - 1);//convert DOY fropm 1 base to zero base
				//obs.m_obs[0] = stod(data[I_D_BB1]) / (stod(data[I_D_BB0])+ stod(data[I_D_BB1]));
				//obs.m_obs.push_back(stod(data[I_D_BB_DOY]));
				//ASSERT(obs.m_obs[0] >= 0 && obs.m_obs[0] <= 1);
				//ASSERT(obs.m_obs[1]==-999 || (obs.m_obs[1] >= 100 && obs.m_obs[1] <= 220));

				obs.m_obs[0] = stod(data[I_D_BB0]);
				obs.m_obs.push_back(stod(data[I_D_BB1]));


				m_years.insert(obs.m_ref.GetYear());

				m_SAResult.push_back(obs);
			}
		}
	}


	enum { I_SPECIES, I_SOURCE, I_SITE, I_LATITUDE, I_LONGITUDE, I_ELEVATION, I_YEAR, I_BUDBURST, I_PROVINCE, I_TYPE, NB_INPUTS };
	void CBudBurst::AddAnnualResult(const StringVector& header, const StringVector& data)
	{
		static const char* SPECIES_NAME[] = { "bf", "ws", "bs", "ns" };
		if (data.size() == NB_INPUTS)
		{
			if (data[I_SPECIES] == SPECIES_NAME[m_species] && data[I_TYPE] == "C")
			{
				CSAResult obs;

				obs.m_ref = CTRef(stoi(data[I_YEAR]));
				obs.m_obs[0] = stod(data[I_BUDBURST]);//WARNING: Julian day in 1 base


				if (obs.m_obs[0] > -999)
				{
					m_years.insert(obs.m_ref.GetYear());
				}

				m_SAResult.push_back(obs);
			}
		}
	}

	double GetSimDOY(const CModelStatVector& output, size_t col, CTRef& TRef, double BB)
	{
		double DOY = -999.0;
		CTPeriod p(TRef.GetYear(), JANUARY, DAY_01, TRef.GetYear(), DECEMBER, DAY_31);
		int pos = output.GetFirstIndex(col, ">", BB, 1, p);
		if (pos > 0)
		{
			DOY = Round((output.GetFirstTRef() + pos).GetJDay() + (BB - output[pos][col]) / (output[pos + 1][col] - output[pos][col]), 1);
		}

		return DOY;
	}

	bool CBudBurst::GetFValueDaily(CStatisticXY& stat)
	{
		if (m_thresholdT > m_thresholdCD)
			return false;

		if (!m_SAResult.empty())
		{

			if (m_data_weather.GetNbYears() == 0)
			{
				CTPeriod pp((*m_years.begin()) - 1, JANUARY, DAY_01, *m_years.rbegin(), DECEMBER, DAY_31);
				pp.Transform(m_weather.GetEntireTPeriod());
				pp = pp.Intersect(m_weather.GetEntireTPeriod());
				if (pp.IsInit())
				{
					((CLocation&)m_data_weather) = m_weather;
					m_data_weather.SetHourly(m_weather.IsHourly());
					m_data_weather.CreateYears(pp);

					for (int year = pp.GetFirstYear(); year <= pp.GetLastYear(); year++)
					{
						m_data_weather[year] = m_weather[year];
					}
				}
				else
				{
					//remove these obs, no input weather
					m_SAResult.clear();
					return false;
				}

			}

			static const double MIN_BB_DOY = 0.05;
			static const double MAX_BB_DOY = 0.95;
			//
			//			if (!m_BB_DOY_stat.IsInit() )
			//			{
			//#pragma omp critical  
			//				{
			//					const CSimulatedAnnealingVector& all_results = GetSimulatedAnnealingVector();
			//
			//					for (auto it = all_results.begin(); it != all_results.end(); it++)
			//					{
			//						const CSAResultVector& results = (*it)->GetSAResult();
			//						for (auto iit = results.begin(); iit != results.end(); iit++)
			//						{
			//							//if (iit->m_obs[0] >= MIN_BB_DOY && iit->m_obs[0] <= MAX_BB_DOY)
			//							//m_BB_DOY_stat += iit->m_ref.GetJDay();
			//							m_BB_DOY_stat += iit->m_obs[1];
			//						}
			//					}
			//				}
			//
			//			}



			CTPeriod pp = m_data_weather.GetEntireTPeriod(CTM::DAILY);
			CModelStatVector output(pp, NB_DAILY_OUTPUTS, -999);

			//for(auto it= m_years.begin(); it!= m_years.end(); it++)
			for (int year = pp.GetFirstYear(); year <= pp.GetLastYear(); year++)
				ExecuteDaily(m_data_weather[year], output);

			double NLL = 0;
			for (size_t i = 0; i < m_SAResult.size(); i++)
			{
				ASSERT(output.IsInside(m_SAResult[i].m_ref));

				/*	double obs_BB = m_SAResult[i].m_obs[0];
					double sim_BB = output[m_SAResult[i].m_ref][O_D_BB];
					stat.Add(obs_BB, sim_BB);*/

					//double DOY = m_SAResult[i].m_ref.GetJDay();
					//double p = 1 / (1 + exp(-(DOY- m_Sw)/ m_α2));

				double p = output[m_SAResult[i].m_ref][O_D_BB];
				ASSERT(p >= 0 && p <= 1);

				double obs_BB0 = m_SAResult[i].m_obs[0];
				double b0 = -obs_BB0 * log(max(DBL_MIN, (1 - p)));
				double obs_BB1 = m_SAResult[i].m_obs[1];
				double b1 = -obs_BB1 * log(max(DBL_MIN, p));
				NLL += b0 + b1;

				//stat.Add(0, b1);


				//if (obs_BB >= MIN_BB_DOY && obs_BB <= MAX_BB_DOY)
				//{
				//double BB_DOY = GetSimDOY(output, O_D_BB, m_SAResult[i].m_ref, 0.5);
				//if (BB_DOY > -999)
				//{
				//	double obs_DOY = (m_SAResult[i].m_obs[1] - m_BB_DOY_stat[LOWEST]) / m_BB_DOY_stat[RANGE];
				//	double sim_DOY = (BB_DOY - m_BB_DOY_stat[LOWEST]) / m_BB_DOY_stat[RANGE];

				//	//for (size_t n = 0; n < N; n++)
				//		stat.Add(obs_DOY, sim_DOY);
				//}
				//else
				//{
				//	stat.clear();
				//	return false;//reject this solution
				//}
				//}
			}//for all results

			if (isnan(NLL) || !isfinite(NLL))
				return false;

			stat.Add(0, NLL);
		}

		return true;
	}


	bool CBudBurst::GetFValueAnnual(CStatisticXY& stat)
	{

		if (m_thresholdT > m_thresholdCD)
			return false;


		if (!m_SAResult.empty())
		{

			if (m_data_weather.GetNbYears() == 0)
			{
				CTPeriod pp((*m_years.begin()) - 1, JANUARY, DAY_01, *m_years.rbegin(), DECEMBER, DAY_31);
				pp.Transform(m_weather.GetEntireTPeriod());
				pp = pp.Intersect(m_weather.GetEntireTPeriod());
				if (pp.IsInit())
				{
					((CLocation&)m_data_weather) = m_weather;
					m_data_weather.SetHourly(m_weather.IsHourly());
					m_data_weather.CreateYears(pp);

					for (int year = pp.GetFirstYear(); year <= pp.GetLastYear(); year++)
					{
						m_data_weather[year] = m_weather[year];
					}
				}
				else
				{
					//remove these obs, no input weather
					m_SAResult.clear();
					return false;
				}

			}

			CTPeriod pp(m_data_weather.GetEntireTPeriod(CTM::ANNUAL));
			CModelStatVector output(pp, NB_ANNUAL_OUTPUTS, -999);

			if (!OnExecuteAnnualOther(m_data_weather, output))
				return false;


			for (size_t i = 0; i < m_SAResult.size(); i++)
			{
				ASSERT(output.IsInside(m_SAResult[i].m_ref));

				//ASSERT(m_SAResult[i].m_obs[0] > -999 && output[m_SAResult[i].m_ref][O_BUDBURST] > -999);

				//CTRef TRef;
				//TRef.SetRef(int(output[m_SAResult[i].m_ref][O_A_BUDBURST]), CTM(CTM::DAILY));

				double obs_BB = m_SAResult[i].m_obs[0];
				//double sim_BB = TRef.GetJDay();
				double sim_BB = output[m_SAResult[i].m_ref][O_A_DC];
				stat.Add(obs_BB, sim_BB);


			}//for all results


		}

		return true;
	}



}