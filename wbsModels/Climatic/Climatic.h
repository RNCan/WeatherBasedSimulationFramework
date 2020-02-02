#pragma once
#include <array>
#include "ModelBase/BioSIMModelBase.h"


namespace WBSF
{

	class CClimaticModel : public CBioSIMModelBase
	{
	public:


		CClimaticModel();
		virtual ~CClimaticModel();

		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;

		virtual ERMsg OnExecuteAnnual()override;
		virtual ERMsg OnExecuteMonthly()override;
		virtual ERMsg OnExecuteDaily()override;
		virtual ERMsg OnExecuteHourly()override;

		static CBioSIMModelBase* CreateObject() { return new CClimaticModel; }

	protected:

		bool m_bEx;//extended model
		double m_prcp_thres;

		static size_t GetNbDayWithPrcp(const CWeatherYear& weather, double prcp_thres=0.2);
		static size_t GetNbDayWithPrcp(const CWeatherMonth& weather, double prcp_thres = 0.2);
		static size_t GetNbFrostDay(const CWeatherYear& weather);
		static size_t GetNbFrostDay(const CWeatherMonth& weather);

	};



}