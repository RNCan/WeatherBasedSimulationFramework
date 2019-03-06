//***********************************************************
// 20/09/2016	1.2.0	Rémi Saint-Amant   WBSF
//***********************************************************
#include "ForestTentCaterpillarModel.h"
#include "ModelBase/EntryPoint.h"
#include "Basic\DegreeDays.h"
#include "ForestTentCaterpillar.h"

using namespace WBSF::HOURLY_DATA;
using namespace WBSF::FTC;
using namespace std;

namespace WBSF
{

	

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CForestTentCaterpillarModel::CreateObject);




	CForestTentCaterpillarModel::CForestTentCaterpillarModel()
	{
		//NB_INPUT_PARAMETER is used to determine if the dll
		//uses the same number of parameters than the model interface
		NB_INPUT_PARAMETER = -1;
		VERSION = "3.0.0 (2019)";
	}

	CForestTentCaterpillarModel::~CForestTentCaterpillarModel()
	{
	}


	//this method is call to load your parameter in your variable
	ERMsg CForestTentCaterpillarModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		int c = 0;

		m_type = parameters[c++].GetInt();
		m_treeKind = parameters[c++].GetInt();
		//m_threshold = parameters[c++].GetBool();
		//m_firsdtDay = parameters[c++].GetBool();
		//m_sumDD= parameters[c++].GetBool();

		return msg;
	}


	class CFTCStat
	{
	public:

		enum{ HATCH_PEAK, PUPATION_PEAK, EMERGENCE_PEAK, NB_VARIABLES };
	};

	
	ERMsg CForestTentCaterpillarModel::OnExecuteAnnual()
	{
		_ASSERTE(m_weather.size() > 1);

		ERMsg msg;

		//CFTCStatVector stat(m_weather.GetNbYears() - 1, CTRef(m_weather[size_t(1)].GetTRef().GetYear()));
		CTPeriod outputPeriod = m_weather.GetEntireTPeriod(CTM::ANNUAL);
		outputPeriod.Begin()++;//begin output at the second year
		CModelStatVector stat(outputPeriod, CFTCStat::NB_VARIABLES);
		
		for (size_t y = 0; y < m_weather.GetNbYears() - 1; y++)
		{
			int year = m_weather.GetFirstYear() + int(y);

			CTRef begin;
			double threshold = 0;
			double sumDD = 0;
			switch (m_type)
			{
			case ORIGINAL:
				begin = CTRef(year + 1, FIRST_MONTH, FIRST_DAY);
				threshold = 2.2;
				sumDD = 222.2;
				break;//orginal
			case REMI_PARAMETER:
				begin = CTRef(year, NOVEMBER, DAY_06);
				threshold = 3.6;
				sumDD = 99.14;
				break;
			default:_ASSERTE(false);
			}

			CTRef hatchDay;
			double DD = 0;
			CDegreeDays DDhatch(CDegreeDays::DAILY_AVERAGE, threshold);
			
			CTPeriod period = m_weather[y + 1].GetEntireTPeriod(CTM::DAILY);
			for (CTRef d = begin; d <= period.End(); d++)
			{
				DD += DDhatch.GetDD(m_weather.GetDay(d));
				if (DD >= sumDD)
				{
					hatchDay = d;
					break;
				}
			}

			int Jday = 366;
			if (hatchDay.IsInit())
			{
				Jday = -1;
				if (hatchDay.GetYear() == period.Begin().GetYear())
					Jday = (int)hatchDay.GetJDay();
			}

			stat[y][CFTCStat::HATCH_PEAK] = Jday + 1;
			//***************************************************

			CTRef pupationDay;
			double DDPupation = 0;
			CDegreeDays DD0(CDegreeDays::DAILY_AVERAGE, 0);
			
			for (CTRef d = period.Begin(); d <= period.End(); d++)
			{
				DDPupation += DD0.GetDD( m_weather.GetDay(d) );
				if (DDPupation >= 800)
				{
					pupationDay = d;
					break;
				}
			}

			int Jday2 = 366;
			if (pupationDay.IsInit())
			{
				Jday2 = -1;
				if (pupationDay.GetYear() == period.Begin().GetYear())
					Jday2 = (int)pupationDay.GetJDay();
			}

			stat[y][CFTCStat::PUPATION_PEAK] = Jday2 + 1;

			//***************************************************
			CTRef flightDay;
			double DDFlight = 0;

			//for (CTRef d = m_weather[y + 1].GetFirstTRef(); d <= m_weather[y + 1].GetLastTRef(); d++)
			for (CTRef d = period.Begin(); d <= period.End(); d++)
			{
				//DDFlight += m_weather[d].GetDD(0);
				DDFlight += DD0.GetDD(m_weather.GetDay(d));
				if (DDFlight >= 1009)
				{
					flightDay = d;
					break;
				}
			}

			int Jday3 = 366;
			if (flightDay.IsInit())
			{
				Jday3 = -1;
				//if (flightDay.GetYear() == m_weather[y + 1].GetYear())
				if (flightDay.GetYear() == period.Begin().GetYear())
					Jday3 = (int)flightDay.GetJDay();
			}

			stat[y][CFTCStat::EMERGENCE_PEAK] = Jday3 + 1;
		}

		SetOutput(stat);

		return msg;

	}
	
	

	//ERMsg CForestTentCaterpillarModel::OnExecuteDaily()
	//{
	//	_ASSERTE(m_weather.size() > 1);

	//	ERMsg msg;

	//	//CFTCStatVector stat(m_weather.GetNbYears() - 1, CTRef(m_weather[size_t(1)].GetTRef().GetYear()));
	//	CTPeriod outputPeriod = m_weather.GetEntireTPeriod(CTM::DAILY);
	//	//outputPeriod.Begin()++;//begin output at the second year
	//	m_output.Init(outputPeriod, 1, 0);

	//	for (size_t y = 0; y < m_weather.GetNbYears(); y++)
	//	{
	//		int year = m_weather.GetFirstYear() + int(y);

	//		//Ives 1973
	//		CTRef begin= CTRef(year, APRIL, DAY_27);
	//		double t = 4.4;//40°F
	//		double sumDDThreshold = 61.1;// 110°F

	//		//Hodson 1977
	//		//CTRef begin = CTRef(year, MARCH, DAY_01);
	//		//double t = 7.8;//46°F
	//		//double sumDDThreshold = 44.4;// 80 DD°F --> DD°C (x 5/9)

	//		//CDegreeDays DDhatch(CDegreeDays::SINGLE_TRIANGLE, threshold);

	//		CTRef hatchDay;
	//		double sumDD = 0;
	//		CTPeriod period = m_weather[y].GetEntireTPeriod(CTM::DAILY);
	//		for (CTRef d = begin; d <= period.End(); d++)
	//		{
	//			const CWeatherDay& w_day = m_weather.GetDay(d);
	//			double m = w_day[H_TMIN][MEAN];
	//			double h = w_day[H_TMAX][MEAN];
	//			double DD = 0;
	//			if (t < m)
	//				DD = (h + m) / 2 - t;
	//			else if (m <= t && t < h)
	//				DD = Square(h - t) / (2 * (h - m));

	//			sumDD += DD;
	//			m_output[d][0] = sumDD;
	//			/*if (sumDD >= sumDDThreshold)
	//			{
	//				hatchDay = d;
	//				break;
	//			}*/
	//		}
	//	}
	//	

	//	return msg;

	//}

	ERMsg CForestTentCaterpillarModel::OnExecuteDaily()
	{
		ERMsg msg;

		ExecuteDaily(m_output);

		return msg;
	}

	void CForestTentCaterpillarModel::ExecuteDaily(CModelStatVector& stat)
	{
		if (m_weather.IsDaily())
			m_weather.ComputeHourlyVariables();

		//This is where the model is actually executed
		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));
		p.Begin().m_year++;//skip the first year in result
		stat.Init(p, FTC::NB_STATS, 0);

		//we simulate 2 years at a time. 
		//we also manager the possibility to have only one year
		for (size_t y1 = 0; y1 < m_weather.size()-1; y1++)
		{
			int year1 = m_weather.GetFirstYear() + int(y1);
			int year2 = year1 + 1;
			CTPeriod p(year1, NOVEMBER, DAY_13, year2, DECEMBER, DAY_31);

			//CTPeriod p = m_weather[y1].GetEntireTPeriod(CTM(CTM::DAILY));

			CFTCStand stand(this);
			//Create stand
			//warning, to not change stand and tree order because insect create used stand defoliation
			//stand.m_bApplyAttrition = m_bApplyAttrition;

			//Create tree
			CFTCTreePtr pTree(new CFTCTree(&stand));

			pTree->m_kind = m_treeKind;
			pTree->m_nbMinObjects = 100;
			pTree->m_nbMaxObjects = 1000;
			//pTree->Initialize<CForestTentCaterpillar>(CInitialPopulation(p.Begin(), 0, 4, 100, EGG));
			pTree->Initialize<CForestTentCaterpillar>(CInitialPopulation(p.Begin(), 0, 400, 100, EGG));

			//add tree to stand			
			stand.m_host.push_front(pTree);

			//if Simulated Annealing, set 
			//if (ACTIVATE_PARAMETRIZATION)
			//{
				//stand.m_rates.SetRho25(m_rho25Factor);
				//stand.m_rates.Save("D:\\Rates.csv");
			//}


			//size_t nbYear = 2;
			//for (size_t y = 0; y < nbYear && y1 + y < m_weather.size(); y++)
			//{
				//size_t yy = y1 + y;
				

				//CTPeriod p = m_weather[yy].GetEntireTPeriod(CTM(CTM::DAILY));

			for (CTRef d = p.Begin(); d <= p.End(); d++)
			{
				stand.Live(m_weather.GetDay(d));
				if(stat.IsInside(d))
					stand.GetStat(d, stat[d]);

				stand.AdjustPopulation();
				HxGridTestConnection();
			}

				//stand.HappyNewYear();
			//}
		}
	}



	//simulated annealing 
	void CForestTentCaterpillarModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		//transform value to date/stage
		ASSERT(header[3] == "Year");
		ASSERT(header[4] == "Month");
		ASSERT(header[5] == "Day");
		ASSERT(header[12] == "NbInds");
		ASSERT(header[13] == "AI");

		CTRef ref(ToShort(data[3]), ToShort(data[4]) - 1, ToShort(data[5]) - 1);
		std::vector<double> obs;
		obs.push_back(ToDouble(data[13]));
		obs.push_back(ToDouble(data[12]));
		m_SAResult.push_back(CSAResult(ref, obs));
	}

	void CForestTentCaterpillarModel::GetFValueDaily(CStatisticXY& stat)
	{
		ERMsg msg;
		CModelStatVector statSim;

		if (m_SAResult.size() > 0)
		{
			//int m_firstYear = m_SAResult.begin()->m_ref.GetYear();
			//int m_lastYear = m_SAResult.rbegin()->m_ref.GetYear();

			//while( m_weather.GetFirstYear() < m_firstYear )
			//	m_weather.RemoveYear(0);

			//while( m_weather.GetLastYear() > m_lastYear )
			//	m_weather.RemoveYear(m_weather.GetNbYear()-1);

			CStatistic years;
			for (CSAResultVector::const_iterator p = m_SAResult.begin(); p < m_SAResult.end(); p++)
				years += p->m_ref.GetYear();

			int m_firstYear = (int)years[LOWEST];
			int m_lastYear = (int)years[HIGHEST];
			/*while( m_weather.GetNbYear() > 1 && m_weather.GetFirstYear() < m_firstYear )
				m_weather.RemoveYear(0);

				while( m_weather.GetNbYear() > 1 && m_weather.GetLastYear() > m_lastYear )
				m_weather.RemoveYear(m_weather.GetNbYear()-1);
				*/

			ASSERT(m_weather.GetFirstYear() == m_firstYear);
			ASSERT(m_weather.GetLastYear() == m_lastYear);
			ExecuteDaily(statSim);


			for (int i = 0; i < (int)m_SAResult.size(); i++)
			{
				if (statSim.IsInside(m_SAResult[i].m_ref))
				{
					double AISim = statSim[m_SAResult[i].m_ref][S_AVERAGE_INSTAR];
					//_ASSERTE( AISim >= 2&&AISim<=8);
					//when all bug die, a value of -9999 can be compute
					if (AISim >= 2 && AISim <= 8)
					{
						for (int j = 0; j < m_SAResult[i].m_obs[1]; j++)
							stat.Add(AISim, m_SAResult[i].m_obs[0]);
					}
				}
			}
		}
	}

}