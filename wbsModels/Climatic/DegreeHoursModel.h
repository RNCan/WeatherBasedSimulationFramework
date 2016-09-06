//*****************************************************************************
// File: DegreeDay.h
//
// Class: CDegreeHoursModel
//*****************************************************************************

#pragma once

#include "ModelBase/BioSIMModelBase.h"
#include "Basic/DegreeDays.h"
#include "Basic/ModelStat.h"

namespace WBSF
{

	//**********************************************************
	class CDegreeHoursModel : public CBioSIMModelBase
	{
	public:

		enum TSummation{ NON_CUMULATIVE, CUMULATIVE, NB_SUMMATION_TYPE };

		CDegreeHoursModel();
		virtual ~CDegreeHoursModel();

		virtual ERMsg OnExecuteHourly();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);

		virtual ERMsg GetErrorMessage(int errorID);

		static CBioSIMModelBase* CreateObject(){ return new CDegreeHoursModel; }

	private:

		void ComputeFinal(CTM TM, const CModelStatVector& input, CModelStatVector& output);

		CDegreeHours m_DH;
		CMonthDay m_firstDate;
		CMonthDay m_lastDate;

	};

}