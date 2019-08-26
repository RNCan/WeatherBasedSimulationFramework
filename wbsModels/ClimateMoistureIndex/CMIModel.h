#pragma once

#include "ModelBase/BioSIMModelBase.h"
#include "Basic/UtilTime.h"

namespace WBSF
{
	enum TAnnualOutput { O_GDD_CUM, O_GDD_WYR, O_CMI_WYR, O_PPT_WYR, O_PET_WYR, O_TMAX_WYR, O_TMIN_WYR, O_PPT_SUMMER, NB_A_OUTPUT };
	typedef CModelStatVectorTemplate<NB_A_OUTPUT> CAnnualOutput;

	enum TMonthlyOutput { O_TMAX_MEAN, O_TMIN_MEAN, O_PPT_SUM, O_PET_SUM, O_CMI, NB_M_OUTPUT };
	typedef CModelStatVectorTemplate<NB_M_OUTPUT> CMonthlyOutput;

	class CCMIModel : public CBioSIMModelBase
	{
	public:


		CCMIModel();

		virtual ERMsg OnExecuteAnnual()override;
		virtual ERMsg OnExecuteMonthly()override;
		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;
		

		static CBioSIMModelBase* CreateObject(){ return new CCMIModel; }

	protected:

		bool m_bLimitToZero;

		static double GetSPMPET(const CWeatherMonth& weather);
		static double GetSPMPET(const CWeatherStation& weather, CTPeriod& p);

	};

}