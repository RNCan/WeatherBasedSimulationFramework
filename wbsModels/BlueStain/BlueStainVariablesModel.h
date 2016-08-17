#pragma once

#include "ModelBase/BioSIMModelBase.h"
#include "Basic/ModelStat.h"

namespace WBSF
{

	class CBlueStainVariablesModel : public CBioSIMModelBase
	{
	public:


		CBlueStainVariablesModel();
		virtual ~CBlueStainVariablesModel();
		virtual ERMsg OnExecuteAnnual();
		static CBioSIMModelBase* CreateObject(){ return new CBlueStainVariablesModel; }


	};
}