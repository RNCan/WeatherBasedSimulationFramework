#pragma once

#include "ModelBase/BioSIMModelBase.h"
#include "Basic/ModelStat.h"

namespace WBSF
{


	class CSBWHostBudBurstModel : public CBioSIMModelBase
	{
	public:


		CSBWHostBudBurstModel();
		virtual ~CSBWHostBudBurstModel();

		virtual ERMsg OnExecuteDaily()override;
		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;

		static CBioSIMModelBase* CreateObject() { return new CSBWHostBudBurstModel; }
	
	protected:

		size_t m_species;
		double m_defoliation;
	};

}