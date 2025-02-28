﻿//*****************************************************************************
// File: SBDevelopment.h
//*****************************************************************************
#pragma once

#include "crtdbg.h"
#include "ModelBase/EquationTableLookup.h"

namespace WBSF
{

	namespace SBW
	{
		enum TStages{ EGG, L1, L2o, L2, L3, L4, L5, L6, PUPAE, ADULT, NB_STAGES, DEAD_ADULT = NB_STAGES };
		enum TAdultLongevity{ NO_MAX_ADULT_LONGEVITY= 0 };
	}


	//*****************************************************************************
	//CSpruceBudwormEquations
	class CSpruceBudwormEquations : public CEquationTableLookup
	{
	public:

		

		CSpruceBudwormEquations(const CRandomGenerator& RG);

		static size_t GetEquationIndex(size_t stage, size_t sex);
		virtual double ComputeRate(size_t e, double t)const;

		double GetRate(size_t stage, size_t sex, double T)const
		{
			size_t e = GetEquationIndex(stage, sex);
			return WBSF::CEquationTableLookup::GetRate(e, T);
		}

		//relative development
		double RelativeDevRate(size_t s)const;

		double get_Fº(double A)const;
		double get_A(size_t sex)const;
		double get_M(size_t sex, double A, double G)const;
		double get_ξ(size_t sex, double A)const;
		double get_P(double T)const;
		static double get_L(double A);
		
		double get_p_exodus()const;
		double get_defoliation(double defoliation)const;

		
	protected:

		enum TEquation{ E_EGG, E_L1, E_L2o, E_L2, E_L3, E_L4, E_L5, E_L6_MALE, E_L6_FEMALE, E_PUPAE_MALE, E_PUPAE_FEMALE, E_ADULT, NB_EQUATION };
		enum TParameters{ PB1, PB2, PB3, PB4, PTB, PTM, NB_PARAMETER };

		enum TSex{ MALE, FEMALE, NB_SEX };

		static double Equation1(size_t s, double T);
		static double Equation2(size_t s, double T);
		static double Equation3(size_t s, double T);
		static size_t e2s(size_t e);

		static const double P[NB_EQUATION][NB_PARAMETER];
		static double b1Factor[SBW::NB_STAGES];
		

		//relative development
		enum TRelDevParameters{ A1, A2, NB_REL_DEV_PARAMETERS };

		double RelativeDevRateBase(size_t s)const;
		double Equation4(const double p[NB_REL_DEV_PARAMETERS])const;
		double Equation5(const double p[NB_REL_DEV_PARAMETERS])const;

	};

}