//*****************************************************************************
// File: SBDevelopment.h
//*****************************************************************************
#pragma once

#include "crtdbg.h"
#include "ModelBase/EquationTableLookup.h"

namespace WBSF
{

	namespace LNF
	{
		enum TStages{ EGG, LARVAE, PREPUPAE, PUPAE, AESTIVAL_DIAPAUSE_ADULT, ACTIVE_ADULT, DEAD_ADULT, NB_STAGES = DEAD_ADULT};
		enum TRDR { Ϙ, к, NB_RDR_PARAMS }; //relative development parameter
		enum TOvip{ μ, ѕ, Τᴴ, NB_OVP_PARAMS };//oviposition parameters
		enum TAAD{ ʎ0, ʎ1, ʎ2, NB_AAD_PARAMS };//AdultAestivalDiapause
	}


	//*****************************************************************************
	//CLaricobiusNigrinusEquations
	class CLaricobiusNigrinusEquations : public CEquationTableLookup
	{
	public:

		static const double RDR[LNF::NB_STAGES][LNF::NB_RDR_PARAMS]; //relative development parameter
		static const double OVP[LNF::NB_OVP_PARAMS];//oviposition parameters
		static const double AAD[LNF::NB_AAD_PARAMS];//AdultAestivalDiapause parameters
		
		double m_RDR[LNF::NB_STAGES][LNF::NB_RDR_PARAMS]; //relative development parameter
		double m_OVP[LNF::NB_OVP_PARAMS];//oviposition parameters
		double m_AAD[LNF::NB_AAD_PARAMS];//AdultAestivalDiapause parameters
		
		CLaricobiusNigrinusEquations(const CRandomGenerator& RG);
		
		virtual double ComputeRate(size_t stage, double t)const;

		//relative development
		double GetRelativeDevRate(size_t s)const;
		double GetAdultLongevity(size_t sex)const;
		double GetFecondity(double l)const;
		double GetOvipositionDD()const;
		double GetAdultAestivalDiapauseRate(double T, double dayLength);
		double GetAdultAbundance(double T, size_t j_day_since_jan);
		
	protected:
		
		static double Eq7(size_t s, double T);

	};

}