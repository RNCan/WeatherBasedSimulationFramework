#pragma once
#include <array>
#include "ModelBase/BioSIMModelBase.h"


namespace WBSF
{

	class CVPDModel : public CBioSIMModelBase
	{
	public:


		CVPDModel();
		virtual ~CVPDModel();

		virtual ERMsg ProcessParameters(const CParameterVector& parameters);

		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg OnExecuteMonthly();
		virtual ERMsg OnExecuteDaily();
		virtual ERMsg OnExecuteHourly();
		

		static CBioSIMModelBase* CreateObject(){ return new CVPDModel; }

	};
}