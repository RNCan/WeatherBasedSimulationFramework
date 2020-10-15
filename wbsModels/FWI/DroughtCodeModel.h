#pragma once

#include "ModelBase/BioSIMModelBase.h"
#include "Basic/ModelStat.h"
#include "FWI.h"

namespace WBSF
{

	class CDroughtCode : public CBioSIMModelBase
	{
	public:


		CDroughtCode();
		virtual ~CDroughtCode();

		virtual ERMsg OnExecuteDaily();
		virtual ERMsg OnExecuteMonthly();
		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);

		static CBioSIMModelBase* CreateObject(){ return new CDroughtCode; }
	
	protected:

		ERMsg ExecuteDaily(CModelStatVector& output);

		bool m_bAutoSelect;
		//Fixed season paramns
		CMonthDay  m_firstDay;//91
		CMonthDay  m_lastDay;//305
		double m_FFMC;	//85.0
		double m_DMC;	//6.0
		double m_DC;	//15.0
		CInitialValues m_init_values;
		

		//Automatic season computation
		short m_nbDaysStart;
		short m_TtypeStart;
		double m_thresholdStart;
		short m_nbDaysEnd;
		short m_TtypeEnd;
		double m_thresholdEnd;

		double m_carryOverFraction;//1
		double m_effectivenessOfWinterPrcp;//0.75
		size_t m_method;
	};
}