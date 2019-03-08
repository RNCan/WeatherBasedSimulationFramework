//***********************************************************
// 07/03/2019	1.0.0	Rémi Saint-Amant   Creation
//***********************************************************
#include "LaricobiusLarvaeModel.h"
#include "ModelBase/EntryPoint.h"
#include "Basic\DegreeDays.h"

using namespace WBSF::HOURLY_DATA;
using namespace std;

namespace WBSF
{

	enum { ACTUAL_CDD, DATE_DD717, DIFF_DAY, NB_OUTPUTS };

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CLaricobiusLarvaeModel::CreateObject);

	CLaricobiusLarvaeModel::CLaricobiusLarvaeModel()
	{
		//NB_INPUT_PARAMETER is used to determine if the dll
		//uses the same number of parameters than the model interface
		NB_INPUT_PARAMETER = 2;
		VERSION = "1.0.0 (2019)";

		m_threshold = -3;
		m_sumDD = 1055;

	}

	CLaricobiusLarvaeModel::~CLaricobiusLarvaeModel()
	{
	}


	//this method is call to load your parameter in your variable
	ERMsg CLaricobiusLarvaeModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		size_t c = 0;
		
		m_threshold = parameters[c++].GetFloat();
		m_sumDD = parameters[c++].GetFloat();

		return msg;
	}




	
	ERMsg CLaricobiusLarvaeModel::OnExecuteAnnual()
	{
		_ASSERTE(m_weather.size() > 1);

		ERMsg msg;
		CTRef today = CTRef::GetCurrentTRef();

		CTPeriod outputPeriod = m_weather.GetEntireTPeriod(CTM::ANNUAL);
		outputPeriod.Begin()++;//begin output at the second year
		m_output.Init(outputPeriod, NB_OUTPUTS);
		
		for (size_t y = 0; y < m_weather.GetNbYears() - 1; y++)
		{
			int year = m_weather.GetFirstYear() + int(y);

			CTRef begin = CTRef(year, NOVEMBER, DAY_01);
			//CTRef begin = CTRef(year, m_start.GetMonth(), m_start.GetDay());
			CTRef end = CTRef(year + 1, NOVEMBER, DAY_01);
			
			double CDD = 0;

			CTRef day717;
			double actualCDD=-999;
			CDegreeDays DD(CDegreeDays::DAILY_AVERAGE, m_threshold);
			
			for (CTRef d = begin; d < end; d++)
			{
				CDD += DD.GetDD(m_weather.GetDay(d));
				if (CDD >= m_sumDD && !day717.IsInit())
					day717 = d;

				if (d.as(CTM(CTM::DAILY,CTM::OVERALL_YEARS)) == today.as(CTM(CTM::DAILY, CTM::OVERALL_YEARS)))
					actualCDD = CDD;
			}
			
			m_output[y][ACTUAL_CDD] = actualCDD;
			if (day717.IsInit())
			{
				m_output[y][DATE_DD717] = day717.GetRef();
				m_output[y][DIFF_DAY] = day717.GetJDay() - today.GetJDay();
			}
		}

		return msg;
	}
	

	void CLaricobiusLarvaeModel::AddAnnualResult(const StringVector& header, const StringVector& data)
	{
		ASSERT(data.size()==2);

		CSAResult obs;
		obs.m_ref.FromFormatedString(data[1]);
		m_SAResult.push_back(obs);
	}

	void CLaricobiusLarvaeModel::GetFValueAnnual(CStatisticXY& stat)
	{
		if (!m_SAResult.empty())
		{
			for (size_t y = 0; y < m_weather.GetNbYears() - 1; y++)
			{
				int year = m_weather.GetFirstYear() + int(y);

				CTRef begin = CTRef(year, NOVEMBER, DAY_01);
				CTRef end = CTRef(year + 1, NOVEMBER, DAY_01);
				
				double CDD = 0;
				CDegreeDays DD(CDegreeDays::DAILY_AVERAGE, m_threshold);

				for (CTRef d = begin; d < end && CDD < m_sumDD; d++)
				{
					CDD += DD.GetDD(m_weather.GetDay(d));
					if (CDD >= m_sumDD )
					{
						for (size_t j = 0; j < m_SAResult.size(); j++)
						{
							if (m_SAResult[j].m_ref.GetYear() == d.GetYear())
							{
								stat.Add(d.GetJDay(), m_SAResult[j].m_ref.GetJDay());
							}
						}
					}
				}
			}
		}
	}

}