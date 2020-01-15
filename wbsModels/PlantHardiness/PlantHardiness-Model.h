#pragma once

#include "ModelBase/BioSIMModelBase.h"
#include "Basic/ModelStat.h"

namespace WBSF
{


	class CPlantHardinessModel : public CBioSIMModelBase
	{
	public:

		enum TCountry { C_CANADA, C_USA, NB_COUNTRY };

		CPlantHardinessModel();
		virtual ~CPlantHardinessModel();

		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);

		static CBioSIMModelBase* CreateObject() { return new CPlantHardinessModel; }


	protected:

		size_t m_country;
	
	};

}