#pragma once

#include "ModelBase/BioSIMModelBase.h"
#include "Basic/ModelStat.h"
#include "BudBurstSBWHost.h"

namespace WBSF
{



	class CSBWHostBudBurstModel : public CBioSIMModelBase
	{
	public:


		CSBWHostBudBurstModel();
		virtual ~CSBWHostBudBurstModel();

		virtual ERMsg OnExecuteDaily()override;
		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;
		virtual void AddDailyResult(const StringVector& header, const StringVector& data)override;
		virtual void GetFValueDaily(CStatisticXY& stat)override;


		static CBioSIMModelBase* CreateObject() { return new CSBWHostBudBurstModel; }
	
	protected:


		void CalibrateSDI(CStatisticXY& stat);


		size_t m_species;
		double m_defoliation;
		size_t m_SDI_type;

		HBB::CParameters m_P;
		static const std::array < std::array<double, NB_SDI_PARAMS>, HBB::NB_SBW_SPECIES> SDI;
		std::array<double, NB_SDI_PARAMS> m_SDI;
		std::set<int> m_years;
		CWeatherStation data_weather;
	};

}