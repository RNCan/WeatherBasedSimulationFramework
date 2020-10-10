#pragma once

#include "ModelBase/BioSIMModelBase.h"


namespace WBSF
{
	
	class CSolarModel : public CBioSIMModelBase
	{
	public:


		CSolarModel();
		virtual ~CSolarModel();

		virtual ERMsg OnExecuteDaily()override;
		virtual ERMsg ProcessParameters(const CParameterVector& parameters)override;
		static CBioSIMModelBase* CreateObject() { return new CSolarModel; }


	};
}