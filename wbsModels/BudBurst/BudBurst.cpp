//*********************************************************************
//17/04/2019	1.0.0	Rémi Saint-Amant	Creation
//*********************************************************************
#include "BudBurst.h"
#include "Basic/WeatherDefine.h"
#include "ModelBase/EntryPoint.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;

namespace WBSF
{

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CBudBurst::CreateObject);

	enum TOutput { O_DC, O_SW, O_BUDBURST, NB_OUTPUTS };
	

	CBudBurst::CBudBurst()
	{
		// initialize your variable here (optional)
		NB_INPUT_PARAMETER = 6;
		VERSION = "2.0.0 (2022)";



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
		m_beginDOY = parameters[c++].GetInt()-1;
		m_thresholdT = parameters[c++].GetReal();
		m_thresholdCD = parameters[c++].GetReal();
		m_Sw = parameters[c++].GetReal();
		m_α2 = parameters[c++].GetReal();
		

		return msg;
	}


	//This method is call to compute solution
	ERMsg CBudBurst::OnExecuteAnnual()
	{
		ERMsg msg;

		if (m_species == S_MAPLE)
			msg = OnExecuteAnnualMaple();
		else
			msg = OnExecuteAnnualOther(m_species, m_weather, m_output);
			
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

		for (size_t y = 0; y < m_weather.GetNbYears()-1; y++)
		{ 
			CTRef budBurst;
			double sum = 0;
			int dc = 0;

			int year = m_weather[y].GetTRef().GetYear();
			CTPeriod p(CTRef(year, DECEMBER, DAY_01), CTRef(year+1, DECEMBER, DAY_01));
			for (CTRef TRef = p.Begin(); TRef < p.End()&& !budBurst.IsInit(); TRef++)
			{
				const CWeatherDay&  wDay = m_weather.GetDay(TRef);
				
				if (wDay[H_TAIR][MEAN] < 10)
					dc++;
				else
					sum+= wDay[H_TAIR][MEAN] - 10;

				double Sw = Sw150 * exp(α2*(dc - 150));
				if(sum >= Sw)
				{ 
					budBurst = TRef;
				}
			}

			if (budBurst.IsInit())
			{
				m_output[y][O_DC] = dc;
				m_output[y][O_SW] = Sw150 * exp(α2*(dc - 150.0));
				m_output[y][O_BUDBURST] = budBurst.GetRef();
			}
		}
		

		return msg;
	}

	ERMsg CBudBurst::OnExecuteAnnualOther(size_t species, CWeatherStation& weather, CModelStatVector& output)
	{
		ERMsg msg;


		CTPeriod pp(weather.GetEntireTPeriod(CTM::ANNUAL));
		pp.Begin().m_year++;
		m_output.Init(pp, 3);


		for (size_t y = 0; y < weather.GetNbYears() - 1; y++)
		{
			CTRef budBurst;
			double sum = 0;
			int CD = 0; //Chilling days

			int year = weather[y].GetTRef().GetYear();
			CTPeriod p(CJDayRef(year, m_beginDOY), CJDayRef(year + 1, m_beginDOY-1));

			for (CTRef TRef = p.Begin(); TRef <= p.End() && !budBurst.IsInit(); TRef++)
			{
				const CWeatherDay& wDay = weather.GetDay(TRef);
				
				if (wDay[H_TAIR][MEAN] < m_thresholdT)
					CD++;
				else
					sum += wDay[H_TAIR][MEAN] - m_thresholdT;

				double Sw = m_Sw * exp(m_α2 * (CD - m_thresholdCD));
				if (sum >= Sw)
				{
					budBurst = TRef;
					output[y][O_DC] = CD;
					output[y][O_SW] = Sw;
					output[y][O_BUDBURST] = budBurst.GetRef();
				}
			}
		}


		return msg;
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
				obs.m_obs[0] = stod(data[I_BUDBURST]);


				if (obs.m_obs[0] > -999)
				{
					m_years.insert(obs.m_ref.GetYear());
				}

				m_SAResult.push_back(obs);
			}
		}
	}


	void CBudBurst::GetFValueAnnual(CStatisticXY& stat)
	{

		if (!m_SAResult.empty())
		{

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

			CTPeriod pp(data_weather.GetEntireTPeriod(CTM::ANNUAL));
			CModelStatVector output(pp, NB_OUTPUTS, -999);

			OnExecuteAnnualOther(m_species, data_weather, output);
			

			
			for (size_t i = 0; i < m_SAResult.size(); i++)
			{
				if (output.IsInside(m_SAResult[i].m_ref))
				{
					if (m_SAResult[i].m_obs[0] > -999 && output[m_SAResult[i].m_ref][O_BUDBURST] > -999)
					{
						CTRef TRef;
						TRef.SetRef(int(output[m_SAResult[i].m_ref][O_BUDBURST]), CTM(CTM::DAILY));

						double obs_BB = m_SAResult[i].m_obs[0];
						double sim_BB = TRef.GetJDay();
						stat.Add(obs_BB, sim_BB);
					}
				}
			}//for all results
		}
	}



}