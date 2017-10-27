//*****************************************************************************
// File: CObliqueBandedLeafrollerEquations.h
//*****************************************************************************
#pragma once

#include <crtdbg.h>
#include "ModelBase/EquationTableLookup.h"


namespace WBSF
{

	enum TStages{ EGG, L1, L2, L3, L3D, L4, L5, L6, PUPA, ADULT_PREOVIP, ADULT, DEAD_ADULT, NB_STAGES };


	//*****************************************************************************
	//CSBDevelopment 
	class CObliqueBandedLeafrollerEquations : public CEquationTableLookup
	{
	public:

		CObliqueBandedLeafrollerEquations(const CRandomGenerator& RG);


		//reltive developement
		double Getδ(size_t s)const;

		//fecondity
		double GetE°()const;
		double GetPmax()const;
		static double GetOᵗ(double T);
		static double GetRᵗ(double T);


		//survival rate
		static double GetSurvivalRate(size_t s, double T);
		double GetRelativeSurvivalRate()const;
		double GetLuck(size_t s);

	protected:

		//internal development rates
		virtual double ComputeRate(size_t e, double t)const;


		static double Equation1(size_t s, double T);
		static double Equation2(size_t s, double T);
		double Equation3(size_t s)const;
	};



}