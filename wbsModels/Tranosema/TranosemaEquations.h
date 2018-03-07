//*****************************************************************************
// File: CTranosemaEquations.h
//*****************************************************************************
#pragma once

#include <crtdbg.h>
#include "ModelBase/EquationTableLookup.h"


namespace WBSF
{

	namespace Tranosema
	{
		enum TStages{ EGG, PUPA, ADULT, NB_STAGES, DEAD_ADULT = NB_STAGES };
	}


	//*****************************************************************************
	//CSBDevelopment 
	class CTranosemaEquations : public CEquationTableLookup
	{
	public:

		CTranosemaEquations(const CRandomGenerator& RG);


		//reltive developement
		double Getδ(size_t s)const;

		//fecondity
		double GetEº()const;
		double GetPmax()const;
		static double GetOᵗ(double T);
		static double GetRᵗ(double T);


		//survival rate
		static double GetSurvivalRate(size_t s, double T);
		//double GetRelativeSurvivalRate()const;
		double GetLuck(size_t s);

	protected:

		//internal development rates
		virtual double ComputeRate(size_t e, double t)const;


		static double Equation1(size_t s, double T);
		static double Equation2(size_t s, double T);
		double Equation3(size_t s)const;
	};



}