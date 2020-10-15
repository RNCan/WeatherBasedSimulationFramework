#pragma once


#include "ModelBase/BioSIMModelBase.h"


namespace WBSF
{




	class CFBPModel : public CBioSIMModelBase
	{
	public:


		CFBPModel();
		virtual ~CFBPModel();

		virtual ERMsg OnExecuteHourly();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);

		static CBioSIMModelBase* CreateObject() { return new CFBPModel; }

	protected:

		ERMsg ExecuteFWIHourly(CModelStatVector& output);

		bool m_bAutoSelect;

		//auto select parameters
		size_t m_nbDaysStart;
		size_t m_TtypeStart;
		double m_thresholdStart;
		size_t m_nbDaysEnd;
		size_t m_TtypeEnd;
		double m_thresholdEnd;

		//manual parameters
		//default value
		CMonthDay m_firstDay;//0
		CMonthDay m_lastDay;//0
		double m_FFMC;	//85.0
		double m_DMC;	//6.0
		double m_DC;	//15.0
		//value for each location/years
		CInitialValues m_init_values;


		//common parameters
		double m_carryOverFraction;//1.0
		double m_effectivenessOfWinterPrcp;//0.75
		size_t m_VanWagnerType;//VanWagner 1987
		bool m_fbpMod;//false

		size_t m_method;


		CTRef m_ignition;
		size_t m_fuel_type;
		//size_t m_D0;
		//size_t m_h;//hour of ignition


		static void toOutput(CTRef TRef, const CFBPOutput& in, CModelStatVector& out);
	};
}