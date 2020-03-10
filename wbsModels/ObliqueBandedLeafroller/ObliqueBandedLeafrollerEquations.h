//*****************************************************************************
// File: CObliqueBandedLeafrollerEquations.h
//*****************************************************************************
#pragma once

#include <crtdbg.h>
#include "ModelBase/EquationTableLookup.h"


namespace WBSF
{

	namespace OBL
	{
		enum TStages{ EGG, L1, L2, L3, L3D, L4, L5, L6, PUPA, ADULT_PREOVIP, ADULT, NB_STAGES, DEAD_ADULT = NB_STAGES };
	}

	//*****************************************************************************
	//CSBDevelopment 
	class CObliqueBandedLeafrollerEquations : public CEquationTableLookup
	{
	public:

		CObliqueBandedLeafrollerEquations(const CRandomGenerator& RG);


		//relative development
		double Getδ(size_t s)const;

		//fecundity
		double GetEᵗ(double A0, double A1);

		double GetRate(size_t s, size_t sex, double t)const
		{
			return CEquationTableLookup::GetRate(sex*OBL::NB_STAGES + s, t);
		}

	protected:

		//internal development rates
		virtual double ComputeRate(size_t e, double t)const;


		static double Equation1(size_t s, double T);
		double Equation2()const;
	};



}