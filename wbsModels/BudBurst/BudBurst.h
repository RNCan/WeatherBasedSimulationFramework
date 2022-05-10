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

		virtual ERMsg OnExecuteAnnual()override;
		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;
		virtual void AddAnnualResult(const StringVector& header, const StringVector& data)override;
		virtual void GetFValueAnnual(CStatisticXY& stat)override;


		static CBioSIMModelBase* CreateObject() { return new CBudBurst; }
	
	protected:

		ERMsg OnExecuteAnnualMaple();
		ERMsg OnExecuteAnnualOther(size_t species, CWeatherStation& weather, CModelStatVector& output);
		std::set<int> m_years;
		CWeatherStation data_weather;



		size_t m_species;
		size_t m_beginDOY;
		double m_Sw;
		double m_α2;
		double m_thresholdT;
		double m_thresholdCD;

	};

}