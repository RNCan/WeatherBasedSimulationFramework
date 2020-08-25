//*****************************************************************************
// File: CMeteorusTrachynotusEquations.h
//*****************************************************************************
#pragma once

#include <crtdbg.h>
#include "ModelBase/EquationTableLookup.h"
#include "ModelBase/DevRateEquation.h"


namespace WBSF
{

	namespace MeteorusTrachynotus
	{
		enum TOBLStages { OBL_POST_DIAPAUSE, NB_OBL_STAGES};
		enum TStages { IMMATURE, PUPA, ADULT, NB_STAGES, DEAD_ADULT = NB_STAGES };
		enum TEquations { EQ_IMMATURE0, EQ_IMMATURE, EQ_PUPA, EQ_ADULT, NB_EQUATIONS};
		enum THost { H_OBL, H_SBW, NB_HOSTS };
	}

	//*****************************************************************************
	//COBLPostDiapauseEquations
	class COBLPostDiapauseEquations : public CEquationTableLookup
	{
	public:

		static const CDevRateEquation::TDevRateEquation EQ_TYPE[MeteorusTrachynotus::NB_OBL_STAGES];
		static const double  EQ_P[MeteorusTrachynotus::NB_OBL_STAGES][5];


		COBLPostDiapauseEquations(const CRandomGenerator& RG);
		double Getδ(size_t e)const;


	protected:

		//internal development rates
		virtual double ComputeRate(size_t e, double t)const;
	};


	//*****************************************************************************
	//CMeteorusTrachynotusEquations
	class CMeteorusTrachynotusEquations : public CEquationTableLookup
	{
	public:

		static const CDevRateEquation::TDevRateEquation EQ_TYPE[MeteorusTrachynotus::NB_EQUATIONS];
		static const double  EQ_P[MeteorusTrachynotus::NB_EQUATIONS][5];


		CMeteorusTrachynotusEquations(const CRandomGenerator& RG);
		
		static size_t s2e(size_t s, size_t g) { return s == MeteorusTrachynotus::IMMATURE ? g == 0 ? MeteorusTrachynotus::EQ_IMMATURE0 : MeteorusTrachynotus::EQ_IMMATURE : s + 1; }


		double Getδ(size_t s, size_t g)const { return Getδ(s2e(s, g)); }
		double GetRate(size_t s, size_t g, double t)const { return CEquationTableLookup::GetRate(s2e(s, g), t); }
		double GetPmax()const;

		

	protected:

		double Getδ(size_t e)const;

		//internal development rates
		virtual double ComputeRate(size_t e, double t)const;
	};



}