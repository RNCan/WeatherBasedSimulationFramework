//*****************************************************************************
// File: DegreeDay.h
//
// Class: CReverseDegreeDayModel
//
// Description: CReverseDegreeDayModel is a BioSIM model that computes DegreeDay 
//
//*****************************************************************************

#pragma once

#include "Basic/DegreeDays.h"
#include "ModelBase/BioSIMModelBase.h"



namespace WBSF
{

	enum TOuputForEachYear { O_JDAY, NB_OUTPUT_FOR_EACH_YEAR };
	enum TOuputOverallYears { O_JDAY_SD = NB_OUTPUT_FOR_EACH_YEAR, O_JDAY_N, NB_OPUTPUT_OVERALL_YEARS };

	typedef CModelStatVectorTemplate<NB_OUTPUT_FOR_EACH_YEAR> CForEachYearStat;
	typedef CModelStatVectorTemplate<NB_OPUTPUT_OVERALL_YEARS> COverallYearsStat;


	//**********************************************************
	class CReverseDegreeDayModel : public CBioSIMModelBase
	{
	public:

		//enum TSummation{ NON_CUMULATIVE, CUMULATIVE, NB_SUMMATION_TYPE };
		enum SUMMATION_TYPE{ YEAR_BY_YEAR, OVERALL_YEARS, NB_SUMMATION_TYPE };

		CReverseDegreeDayModel();
		virtual ~CReverseDegreeDayModel();

		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		static CBioSIMModelBase* CreateObject(){ return new CReverseDegreeDayModel; }

	private:

		ERMsg OnExecuteAnnualOverallYears();
		ERMsg OnExecuteAnnualForEachYear();
		void ExecuteAnnual(CForEachYearStat& stat)const;

		CDegreeDays m_DD;
		CMonthDay m_firstDate;
		CMonthDay m_lastDate;
		int m_summationType;
		double m_DDSummation;

	};

}