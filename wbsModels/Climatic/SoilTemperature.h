#pragma once
#include <array>
#include "ModelBase/BioSIMModelBase.h"


namespace WBSF
{

	class CSoilTemperatureModel : public CBioSIMModelBase
	{
	public:


		CSoilTemperatureModel();
		virtual ~CSoilTemperatureModel();

		virtual ERMsg ProcessParameters(const CParameterVector& parameters);

		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg OnExecuteMonthly();
		virtual ERMsg OnExecuteDaily();
		virtual ERMsg OnExecuteHourly();
		

		static CBioSIMModelBase* CreateObject(){ return new CSoilTemperatureModel; }

	};
}