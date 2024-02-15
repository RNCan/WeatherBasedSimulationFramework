#pragma once

#include "ModelBase/BioSIMModelBase.h"

namespace WBSF
{

	class CWorldClimVarsModel : public CBioSIMModelBase
	{
	public:


		CWorldClimVarsModel();
		virtual ~CWorldClimVarsModel();

		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		virtual ERMsg OnExecuteAnnual();
		
		static CBioSIMModelBase* CreateObject(){ return new CWorldClimVarsModel; }

	protected:

	};
}