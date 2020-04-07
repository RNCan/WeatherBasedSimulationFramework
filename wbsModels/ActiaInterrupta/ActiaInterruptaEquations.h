//*****************************************************************************
// File: CActiaInterruptaEquations.h
//*****************************************************************************
#pragma once

#include <crtdbg.h>
#include "ModelBase/EquationTableLookup.h"
#include "ModelBase/DevRateEquation.h"


namespace WBSF
{

	namespace ActiaInterrupta
	{
		enum TStages{ MAGGOT, PUPA, ADULT, NB_STAGES, DEAD_ADULT = NB_STAGES };
		enum TEquation { EQ_OBL_POST_DIAPAUSE, EQ_MAGGOT_OBL, EQ_MAGGOT_SBW, EQ_PUPA, EQ_ADULT, NB_EQUATIONS };
		enum THost { H_OBL, H_SBW, NB_HOSTS };
	}


	//*****************************************************************************
	//CSBDevelopment 
	class CActiaInterruptaEquations : public CEquationTableLookup
	{
	public:

		static const CDevRateEquation::TDevRateEquation EQ_TYPE[ActiaInterrupta::NB_EQUATIONS];
		static const double  EQ_P[ActiaInterrupta::NB_EQUATIONS][6];


		CActiaInterruptaEquations(const CRandomGenerator& RG);
		
		static size_t s2e(size_t s, size_t host){return ActiaInterrupta::EQ_MAGGOT_OBL + ((s == ActiaInterrupta::MAGGOT) ? host : s + 1);}

		using CEquationTableLookup::GetRate;
		double GetRate(size_t s, size_t host, double t)const
		{
			return CEquationTableLookup::GetRate(s2e(s, host), t);
		}
		
		

		double Getδ(size_t s, size_t host)const { return Getδ(s2e(s, host)); }
		double Getδ(size_t e)const;


		double GetPmax()const;

		double GetLuck(size_t s)const;
		double GetSurvivalRate(size_t s, double T);


	protected:

		//internal development rates
		virtual double ComputeRate(size_t e, double t)const;
	};



}