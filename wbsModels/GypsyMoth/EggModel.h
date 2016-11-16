#pragma once

#include <crtdbg.h>
#include <vector>
#include "Basic/ERMsg.h"
#include "Basic/UtilTime.h"
#include "Basic/ModelStat.h"

namespace WBSF
{
	class CWeatherStation;
	
	class CGMEggParam
	{
	public:

		CGMEggParam()
		{
			Reset();
		}

		void Reset()
		{
			m_sawyerModel = 1;
			m_ovipDate.Reset();
		}

		int m_sawyerModel;
		CTRef m_ovipDate;
	};

	static const int MAXEGGS = 250;
	enum TPhase{ PREDIAPAUSE, DIAPAUSE, POSDIAPAUSE, HATCH, NB_PHASES, HATCHING = NB_PHASES, NB_EGG_OUTPUT };
	typedef CModelStatVectorTemplate<NB_EGG_OUTPUT> CEggOutputVector;

	class CEggModel
	{
	public:

		CEggModel(const CGMEggParam& param)
		{
			Reset();
			m_param = param;
		}

	
		void Reset();
	
		virtual ERMsg ComputeHatch(const CWeatherStation& weather, const CTPeriod& p) = 0;

		CTRef GetFirstHatch()const;
		CTRef GetLastHatch()const;
		CTRef GetFirstDay()const;
		CTRef GetMedian(int s)const;


		double GetEggsPourcent(CTRef day, int phase)const
		{
			_ASSERTE(m_eggState.IsInside(day));
			return m_eggState[day][phase] / MAXEGGS*100.0;
		}

		const CEggOutputVector& GetEggs()const{ return m_eggState; }

		CGMEggParam m_param;

	protected:

		CEggOutputVector m_eggState;
	};


}
