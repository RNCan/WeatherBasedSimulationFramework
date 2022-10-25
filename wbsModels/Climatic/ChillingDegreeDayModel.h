//*****************************************************************************
// File: DegreeDay.h
//
// Class: CChillingDegreeDaysModel
//*****************************************************************************

#pragma once

#include "ModelBase/BioSIMModelBase.h"
#include "Basic/ModelStat.h"

namespace WBSF
{

	//**********************************************************
	class CChillingDegreeDaysModel : public CBioSIMModelBase
	{
	public:
	
		CChillingDegreeDaysModel();
		virtual ~CChillingDegreeDaysModel();

		
		virtual ERMsg OnExecuteDaily();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		static CBioSIMModelBase* CreateObject(){ return new CChillingDegreeDaysModel; }

	private:


	
		CMonthDay m_firstDate;
		CMonthDay m_lastDate;
		double m_threshold;



	};

}