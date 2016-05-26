#pragma once


#include "Basic/Evapotranspiration.h"
#include "ModelBase/BioSIMModelBase.h"

//#include "ModelStat.h"

namespace WBSF
{


	class CASCE_ETcModel : public CBioSIMModelBase
	{
	public:


		CASCE_ETcModel();
		virtual ~CASCE_ETcModel();

		//virtual ERMsg OnExecuteHourly();
		virtual ERMsg OnExecuteDaily();
		virtual ERMsg OnExecuteMonthly();
		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);

		static CBioSIMModelBase* CreateObject(){ return new CASCE_ETcModel ; }

	protected:

		//member
		CMonthDay m_Jplant;
		TCrop m_crop;
		TSoil m_soil;
		TGranularity m_granularity;
		TWettingEvent m_irrigation;
		bool m_bExtended;
	};
}