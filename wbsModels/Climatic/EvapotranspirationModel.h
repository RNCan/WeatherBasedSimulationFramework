#pragma once
#include "Basic/Evapotranspiration.h"
#include "ModelBase/BioSIMModelBase.h"

namespace WBSF
{


	class CEvapotranspirationModel : public CBioSIMModelBase
	{
	public:

		CEvapotranspirationModel();
		virtual ~CEvapotranspirationModel();

		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg OnExecuteMonthly();
		virtual ERMsg OnExecuteDaily();
		virtual ERMsg OnExecuteHourly();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		static CBioSIMModelBase* CreateObject(){ return new CEvapotranspirationModel; }

	protected:

		std::string m_ETModelName;
		CETOptions m_options;
	};

}