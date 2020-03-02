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
		enum TStages{ EGG, PUPA, ADULT, NB_STAGES, DEAD_ADULT = NB_STAGES };
		enum TEquation { EQ_EGG_OBL, EQ_EGG_SBW, EQ_PUPA, EQ_ADULT, NB_EQUATIONS };
		enum THost { H_OBL, H_SBW, NB_HOSTS };
	}


	//*****************************************************************************
	//CSBDevelopment 
	class CActiaInterruptaEquations : public CEquationTableLookup
	{
	public:

		static const CDevRateEquation::TDevRateEquation EQ_TYPE[ActiaInterrupta::NB_EQUATIONS];
		static const double  EQ_P[ActiaInterrupta::NB_EQUATIONS][4];


		CActiaInterruptaEquations(const CRandomGenerator& RG);
		
		static size_t s2e(size_t s, size_t host){return (s == ActiaInterrupta::EGG) ? host : s + 1;}
		double GetRate(size_t s, size_t host, double t)const
		{
			return CEquationTableLookup::GetRate(s2e(s, host), t);
		}

		double Getδ(size_t s, size_t host)const { return Getδ(s2e(s, host)); }
		double Getδ(size_t e)const;


		double GetPmax()const;
		double GetEº()const;


		static double GetOᵗ(double T);
		static double GetRᵗ(double T);
	protected:

		//internal development rates
		virtual double ComputeRate(size_t e, double t)const;
	};



}