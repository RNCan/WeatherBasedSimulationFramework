#pragma once

#include "ModelBase/BioSIMModelBase.h"


namespace WBSF
{
	
	class CUnderbarkTemperatureModel : public CBioSIMModelBase
	{
	public:

		enum TModel{ ASH_VERMUNT, PINE_REGNIERE };

		CUnderbarkTemperatureModel();
		virtual ~CUnderbarkTemperatureModel();

		virtual ERMsg OnExecuteDaily()override;
		virtual ERMsg OnExecuteHourly()override;
		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;
		static CBioSIMModelBase* CreateObject() { return new CUnderbarkTemperatureModel; }

		void ExecuteHourly(CModelStatVector& output);

	protected:

		size_t m_model;

	};
}