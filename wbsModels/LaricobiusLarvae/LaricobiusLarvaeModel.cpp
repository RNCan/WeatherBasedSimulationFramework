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
		NB_INPUT_PARAMETER = 0;
		VERSION = "1.0.0 (2019)";
	}

	CLaricobiusLarvaeModel::~CLaricobiusLarvaeModel()
	{
	}


	//this method is call to load your parameter in your variable
	ERMsg CLaricobiusLarvaeModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;
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
			CTRef end = CTRef(year+1, NOVEMBER, DAY_01);

			double threshold = 0;
			double CDD = 0;

			CTRef day717;
			double actualCDD=0;
			CDegreeDays DD(CDegreeDays::DAILY_AVERAGE, 0);
			
			for (CTRef d = begin; d < end; d++)
			{
				CDD += DD.GetDD(m_weather.GetDay(d));
				if (CDD >= 717 && !day717.IsInit())
					day717 = d;

				if (d == today)
					actualCDD = CDD;
			}
			
			m_output[y][ACTUAL_CDD] = actualCDD;
			if (day717.IsInit())
			{
				m_output[y][DATE_DD717] = day717.GetRef();
				m_output[y][DIFF_DAY] = (day717.GetYear() == today.GetYear())?day717 - today:-999;
			}
		}

		return msg;
	}
	
	

}