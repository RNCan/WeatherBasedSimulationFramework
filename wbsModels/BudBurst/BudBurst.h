#pragma once

#include "ModelBase/BioSIMModelBase.h"
#include "Basic/ModelStat.h"

namespace WBSF
{


	class CBudBurst : public CBioSIMModelBase
	{
	public:

		enum TSpecies { SP_BALSAM_FIR, S_WHITE_SPUCE, S_BLACK_SPURCE, S_NORWAY_SPUCE, S_MAPLE, NB_SPECIES };

		CBudBurst();
		virtual ~CBudBurst();

		virtual ERMsg OnExecuteDaily()override;
		virtual ERMsg OnExecuteAnnual()override;
		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;
		void AddDailyResult(const StringVector& header, const StringVector& data)override;
		virtual void AddAnnualResult(const StringVector& header, const StringVector& data)override;
		virtual bool GetFValueDaily(CStatisticXY& stat)override;
		virtual bool GetFValueAnnual(CStatisticXY& stat)override;


		static CBioSIMModelBase* CreateObject() { return new CBudBurst; }
	
	protected:

		CTRef ExecuteDaily(CWeatherYear& weather, CModelStatVector& output);
		ERMsg OnExecuteAnnualMaple();
		bool OnExecuteAnnualOther(CWeatherStation& weather, CModelStatVector& output);
		std::set<int> m_years;
		CWeatherStation m_data_weather;



		size_t m_species;
		size_t m_beginDOY;
		double m_Sw;
		double m_α2;
		double m_thresholdT;
		double m_thresholdCD;

		CStatistic m_BB_DOY_stat;
	};

}