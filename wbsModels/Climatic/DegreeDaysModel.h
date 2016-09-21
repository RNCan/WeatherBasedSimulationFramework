//*****************************************************************************
// File: DegreeDay.h
//
// Class: CDegreeDaysModel
//*****************************************************************************

#pragma once

#include "ModelBase/BioSIMModelBase.h"
#include "Basic/DegreeDays.h"
#include "Basic/ModelStat.h"

namespace WBSF
{

	//**********************************************************
	class CDegreeDaysModel : public CBioSIMModelBase
	{
	public:

		enum TSummation{ NON_CUMULATIVE, CUMULATIVE, NB_SUMMATION_TYPE };

		CDegreeDaysModel();
		virtual ~CDegreeDaysModel();

		
		virtual ERMsg OnExecuteDaily();
		virtual ERMsg OnExecuteMonthly();
		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);

		virtual ERMsg GetErrorMessage(int errorID);

		static CBioSIMModelBase* CreateObject(){ return new CDegreeDaysModel; }

	private:

		void ComputeFinal(CTM TM, const CModelStatVector& input, CModelStatVector& output);

		CDegreeDays m_DD;
		CMonthDay m_firstDate;
		CMonthDay m_lastDate;
		int m_summationType;


	};

}