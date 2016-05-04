#pragma once


#include "ModelBase/BioSIMModelBase.h"


namespace WBSF
{
	class CFWI;

	class CFWIModel : public CBioSIMModelBase
	{
	public:


		CFWIModel();
		virtual ~CFWIModel();

		virtual ERMsg OnExecuteDaily();
		virtual ERMsg OnExecuteMonthly();
		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg ProcessParameter(const CParameterVector& parameters);

		static CBioSIMModelBase* CreateObject(){ return new CFWIModel; }

	private:

		ERMsg ExecuteDaily(CFWI& FWI, CModelStatVector& output);

		bool m_bAutoSelect;

		//auto select parameters
		size_t m_nbDaysStart;
		size_t m_TtypeStart;
		double m_thresholdStart;
		size_t m_nbDaysEnd;
		size_t m_TtypeEnd;
		double m_thresholdEnd;

		//manual parameters
		CMonthDay m_firstDay;//0
		CMonthDay m_lastDay;//0
		double m_FFMC;	//85.0
		double m_DMC;	//6.0
		double m_DC;	//15.0
		//double m_startThreshold;//12 C°

		//common parameters
		double m_carryOverFraction;//1.0
		double m_effectivenessOfWinterPrcp;//0.75

		static double ComputeIndice(int year, int m, double& DCMo, double Rm, double Tm);
	};
}