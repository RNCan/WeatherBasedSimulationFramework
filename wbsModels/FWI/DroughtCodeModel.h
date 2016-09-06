#pragma once

#include "ModelBase/BioSIMModelBase.h"
#include "Basic/ModelStat.h"

namespace WBSF
{
	class CFWI;

	class CDroughtCode : public CBioSIMModelBase
	{
	public:


		CDroughtCode();
		virtual ~CDroughtCode();

		virtual ERMsg OnExecuteDaily();
		virtual ERMsg OnExecuteMonthly();
		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg ProcessParameter(const CParameterVector& parameters);

		static CBioSIMModelBase* CreateObject(){ return new CDroughtCode; }
	
	protected:

		ERMsg ExecuteDaily(CFWI& FWI);

		bool m_bAutoSelect;
		//Fixed season paramns
		short m_firstDay;//91
		short m_lastDay;//305
		double m_FFMC;	//85.0
		double m_DMC;	//6.0
		double m_DC;	//15.0
		//float m_startThreshold;//12 C°

		//Automatic season computation
		short m_nbDaysStart;
		short m_TtypeStart;
		double m_thresholdStart;
		short m_nbDaysEnd;
		short m_TtypeEnd;
		double m_thresholdEnd;

		double m_carryOverFraction;//1
		double m_effectivenessOfWinterPrcp;//0.75


		static double ComputeIndice(int year, int m, double& DCMo, double Rm, double Tm);
	};
}