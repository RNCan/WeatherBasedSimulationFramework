#pragma once


#include "Basic/Evapotranspiration.h"
#include "ModelBase/BioSIMModelBase.h"

//#include "ModelStat.h"

namespace WBSF
{

	class CASCE_ETszModel : public CBioSIMModelBase
	{
	public:

		enum {SHORT, TALL};

		
		CASCE_ETszModel();
		virtual ~CASCE_ETszModel();

		virtual ERMsg OnExecuteHourly();
		virtual ERMsg OnExecuteDaily();
		virtual ERMsg OnExecuteMonthly();
		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);

		static CBioSIMModelBase* CreateObject(){ return new CASCE_ETszModel; }

		
	protected:

		size_t m_referenceType;
		bool m_extended;
//		CETOptions m_options;
		//size_t m_ETref;
		//bool m_bFull;

		//size_t m_Kc;
		//bool m_bFull;
	};
}