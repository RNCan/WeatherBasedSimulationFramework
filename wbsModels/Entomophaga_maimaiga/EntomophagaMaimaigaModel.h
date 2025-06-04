#pragma once

#include "ModelBase/BioSIMModelBase.h"

namespace WBSF
{
	
	class CEntomophagaMaimaigaModel : public CBioSIMModelBase
	{
	public:


		CEntomophagaMaimaigaModel();
		virtual ~CEntomophagaMaimaigaModel();

		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		virtual ERMsg OnExecuteDaily();
		virtual ERMsg OnExecuteAnnual();
		
		static CBioSIMModelBase* CreateObject(){ return new CEntomophagaMaimaigaModel; }

		
		ERMsg ExecuteDaily(CWeatherYears& weather, CModelStatVector& output);

	protected:

		double m_host_density;
		double m_fungus_density;
	};
}