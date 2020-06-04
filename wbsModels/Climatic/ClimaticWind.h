#pragma once
#include <array>
#include "ModelBase/BioSIMModelBase.h"


namespace WBSF
{


	class CClimaticWindModel : public CBioSIMModelBase
	{
	public:

		CClimaticWindModel();
		~CClimaticWindModel();


		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;

		virtual ERMsg OnExecuteAnnual()override;
		virtual ERMsg OnExecuteMonthly()override;

		static CBioSIMModelBase* CreateObject() { return new CClimaticWindModel; }

	protected:

		static std::array<double, 36> GetWindD(const CWeatherYear& weather);
		static std::array<double, 36> GetWindD(const CWeatherMonth& weather);

	};
}