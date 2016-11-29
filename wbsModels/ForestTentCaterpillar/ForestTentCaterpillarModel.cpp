//***********************************************************
// 20/09/2016	1.2.0	Rémi Saint-Amant    Include with WBSF
//   This program takes as argument the current project path
//   and set file name from which it should read its set 
//   parameters. If not provided, current.cfs in the current
//   directory is used
//***********************************************************
#include "ForestTentCaterpillarModel.h"
#include "MOdelBase/EntryPoint.h"

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
		VERSION = "2.0.0 (2016)";
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

	typedef CModelStatVectorTemplate<CFTCStat::NB_VARIABLES> CFTCStatVector;
	ERMsg CForestTentCaterpillarModel::OnExecuteAnnual()
	{
		_ASSERTE(m_weather.GetNbYear() > 1);

		ERMsg msg;

		CFTCStatVector stat(m_weather.GetNbYears() - 1, CTRef(m_weather[1].GetTRef().GetYear()));
		/*
			CTRef hatchDate[8] =
			{
			CTRef(1967, MAY, 20),
			CTRef(1968, APRIL, 29),
			CTRef(1969, MAY,  0),
			CTRef(1970, MAY,  11),
			CTRef(1971, MAY,  05),
			CTRef(1972, MAY,  10),
			CTRef(1973, MAY,  05),
			CTRef(1974, MAY,  15)
			};
			ASSERT( m_weather[0].GetYear() == 1966);
			for(int y=0; y<m_weather.GetNbYear()-1; y++)
			{
			CTRef begin(m_weather[y].GetYear(), NOVEMBER, 12);
			double DD=0;
			for(CTRef d=begin; d<=hatchDate[y]; d++)
			DD+=m_weather[d].GetDD(0);

			stat[y][0] = DD;
			}

			SetOutput(stat);

			return msg;
			*/
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
				begin = CTRef(year, NOVEMBER, DAY_6);
				threshold = 3.6;
				sumDD = 99.14;
				break;
			default:_ASSERTE(false);
			}

			//CTRef begin(m_weather[y].GetYear()+1,	FIRST_MONTH, FIRST_DAY);//orginal
			//CTRef begin(m_weather[y].GetYear()+1,	JANUARY, 26);//eastern and forest 
			//CTRef begin(m_weather[y].GetYear(),	NOVEMBER, 5);//forest only

			CTRef hatchDay;
			double DD = 0;


			for (CTRef d = begin; d <= m_weather[y + 1].GetLastTRef(); d++)
			{
				//DD+=m_weather[d].GetDD(2.2);//original
				//DD+=m_weather[d].GetDD(-7.2);//eastern and forest
				//DD+=m_weather[d].GetDD(3.6);//forest only
				DD += m_weather[d].GetDD(threshold);
				//if( DD>=222.2)//original
				//if( DD>=574.7)//eastern and forest
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
				if (hatchDay.GetYear() == m_weather[y + 1].GetYear())
					Jday = hatchDay.GetJDay();
			}

			stat[y][CFTCStat::HATCH_PEAK] = Jday + 1;
			//***************************************************

			CTRef pupationDay;
			double DDPupation = 0;

			for (CTRef d = m_weather[y + 1].GetFirstTRef(); d <= m_weather[y + 1].GetLastTRef(); d++)
			{
				DDPupation += m_weather[d].GetDD(0);
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
				if (pupationDay.GetYear() == m_weather[y + 1].GetYear())
					Jday2 = pupationDay.GetJDay();
			}

			stat[y][CFTCStat::PUPATION_PEAK] = Jday2 + 1;

			//***************************************************
			CTRef flightDay;
			double DDFlight = 0;

			for (CTRef d = m_weather[y + 1].GetFirstTRef(); d <= m_weather[y + 1].GetLastTRef(); d++)
			{
				DDFlight += m_weather[d].GetDD(0);
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
				if (flightDay.GetYear() == m_weather[y + 1].GetYear())
					Jday3 = flightDay.GetJDay();
			}

			stat[y][CFTCStat::EMERGENCE_PEAK] = Jday3 + 1;
		}

		SetOutput(stat);

		return msg;

	}

}