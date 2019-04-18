#pragma once

#include "ModelBase/BioSIMModelBase.h"
#include "Basic/ModelStat.h"

namespace WBSF
{


	class CBudBurst : public CBioSIMModelBase
	{
	public:


		CBudBurst();
		virtual ~CBudBurst();

		virtual ERMsg OnExecuteAnnual()override;
		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;

		static CBioSIMModelBase* CreateObject() { return new CBudBurst; }
	
	protected:

		size_t m_species;
	};

}