//***********************************************************
// 20/09/2016	1.2.0	Rémi Saint-Amant    Include with WBSF
//   This program takes as argument the current project path
//   and set file name from which it should read its set 
//   parameters. If not provided, current.cfs in the current
//   directory is used
//***********************************************************
#include "ForestTentCaterpillarModel.h"
#include "ModelBase/EntryPoint.h"
#include "Basic\DegreeDays.h"

namespace WBSF
{


	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CForestTentCaterpillarModel::CreateObject);




	CForestTentCaterpillarModel::CForestTentCaterpillarModel()
	{
		//NB_INPUT_PARAMETER is used to determine if the dll
		//uses the same number of parameters than the model interface
		NB_INPUT_PARAMETER = 1;
		VERSION = "2.0.1 (2018)";
	}

	CForestTentCaterpillarModel::~CForestTentCaterpillarModel()
	{
	}


	//this method is call to load your parameter in your variable
	ERMsg CForestTentCaterpillarModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		int c = 0;

		m_type = parameters[c++].GetBool();
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

	//typedef CModelStatVectorTemplate<CFTCStat::NB_VARIABLES> CFTCStatVector;
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

}