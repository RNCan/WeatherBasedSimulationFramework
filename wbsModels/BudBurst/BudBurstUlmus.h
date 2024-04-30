#pragma once

#include "ModelBase/BioSIMModelBase.h"
#include "Basic/ModelStat.h"
#include "ModelBase/ModelDistribution.h"

namespace WBSF
{

	enum TSpecies { AMERICANA, GLABRA, LAEVIS, MINOR, NB_SPECIES };
		//GLABRA, MACROCARPA, PARVIFOLIA, PUMILA, VILLOSA, NB_SPECIES};
	enum TParam { P_T1, P_T2, P_A, P_B, P_R, NB_PARAMS };

	class CBudBurstUlmusModel : public CBioSIMModelBase
	{
	public:


		CBudBurstUlmusModel();
		virtual ~CBudBurstUlmusModel();

		virtual ERMsg OnExecuteDaily()override;
		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;
		virtual void AddDailyResult(const StringVector& header, const StringVector& data)override;
		virtual bool GetFValueDaily(CStatisticXY& stat)override;


		static CBioSIMModelBase* CreateObject() { return new CBudBurstUlmusModel; }

		
		void ExecuteOneYear(size_t yy, CWeatherYears& weather, CModelStatVector& output)const;
		//void ExecuteAllYears(CWeatherYears& weather, CModelStatVector& output);

	protected:


		void CalibrateSDI(CStatisticXY& stat);


		size_t m_species;
		std::array<double, NB_PARAMS> m_P;


		static const std::array < std::array<double, NB_PARAMS >, NB_SPECIES> P;
		
		std::set<int> m_years;
		//CModelStatVector m_DD;
		CWeatherStation m_data_weather;
	};

}