#pragma once

#include "ModelBase/BioSIMModelBase.h"
#include "Basic/ModelStat.h"

namespace WBSF
{


	class CPlantHardinessModel : public CBioSIMModelBase
	{
	public:


		CPlantHardinessModel();
		virtual ~CPlantHardinessModel();

		virtual ERMsg OnExecuteAnnual();
		//virtual ERMsg ProcessParameters(const CParameterVector& parameters);

		static CBioSIMModelBase* CreateObject() { return new CPlantHardinessModel; }
	private:
	};

}