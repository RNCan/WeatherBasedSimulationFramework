#pragma once
#include <array>
#include "ModelBase/BioSIMModelBase.h"


namespace WBSF
{

	class CSnowMelt : public CBioSIMModelBase
	{
	public:


		CSnowMelt();
		virtual ~CSnowMelt();

		virtual ERMsg ProcessParameters(const CParameterVector& parameters);

		//virtual ERMsg OnExecuteAnnual();
		virtual ERMsg OnExecuteMonthly();
		//virtual ERMsg OnExecuteDaily();
		//virtual ERMsg OnExecuteHourly();
		
		static CBioSIMModelBase* CreateObject(){ return new CSnowMelt; }
	};
}