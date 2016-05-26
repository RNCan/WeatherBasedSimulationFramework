#pragma once

#include "ModelBase/BioSIMModelBase.h"
#include "Basic/ModelStat.h"

namespace WBSF
{

	class CGrowingSeasonModel : public CBioSIMModelBase
	{
	public:

		enum TInput{ BEGIN, END, NB_INPUT };
		//enum TTempType{IN_TMIN, IN_TMEAN, IN_TMAX, NB_TEMP};

		CGrowingSeasonModel();
		virtual ~CGrowingSeasonModel();

		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg ProcessParameter(const CParameterVector& parameters);

		static CBioSIMModelBase* CreateObject(){ return new CGrowingSeasonModel; }

	private:


		//CTPeriod GetGrowingSeason(const CWeatherYear& weather, bool bAlwaysFillPeriod=true)const;

		short m_nbDays[NB_INPUT];
		short m_Ttype[NB_INPUT];
		double m_threshold[NB_INPUT];



		//static double GetT(const CWeatherDay& Wday, int t);
	};

}