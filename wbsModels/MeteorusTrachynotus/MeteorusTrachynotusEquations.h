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
		//enum TStages{ EGG, LARVA, PUPA, ADULT, NB_STAGES, DEAD_ADULT = NB_STAGES };
		//enum TEquation { EQ_OBL_POST_DIAPAUSE, EQ_IMMATURE, EQ_PUPA, EQ_ADULT, NB_EQUATIONS };
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

		//static size_t s2e(size_t s){return MeteorusTrachynotus::EQ_IMMATURE_EGG + s;}

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

		static const CDevRateEquation::TDevRateEquation EQ_TYPE[MeteorusTrachynotus::NB_STAGES];
		static const double  EQ_P[MeteorusTrachynotus::NB_STAGES][5];


		CMeteorusTrachynotusEquations(const CRandomGenerator& RG);
		
		//static size_t s2e(size_t s){return MeteorusTrachynotus::EQ_IMMATURE_EGG + s;}

		//using CEquationTableLookup::GetRate;
		//double GetRate(size_t s, double t)const;
		
		
		double Getδ(size_t e)const; 
		double GetPmax()const;
		

	protected:

		//internal development rates
		virtual double ComputeRate(size_t e, double t)const;
	};



}