#pragma once

#include "ModelBase/BioSIMModelBase.h"
#include "SpruceBeetle.h"

namespace WBSF
{

	class CSpruceBeetleModel : public CBioSIMModelBase
	{
	public:


		CSpruceBeetleModel();
		virtual ~CSpruceBeetleModel();

		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg ProcessParameter(const CParameterVector& parameters);
		static CBioSIMModelBase* CreateObject(){ return new CSpruceBeetleModel; }

	};


}