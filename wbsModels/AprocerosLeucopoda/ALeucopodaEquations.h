//*****************************************************************************
// File: SBDevelopment.h
//*****************************************************************************
#pragma once

#include <array>
#include "crtdbg.h"
#include "ModelBase/EquationTableLookup.h"

namespace WBSF
{

	namespace TZZ//Aproceros Leucopoda
	{
		enum TStages{ EGG, LARVA, PREPUPA, PUPA, ADULT, DEAD_ADULT, NB_STAGES = DEAD_ADULT};
		enum TRDR { σ, NB_RDR_PARAMS }; //relative development parameter
		//enum TOvip{ μ, ѕ, Τᴴ¹, Τᴴ², NB_OVP_PARAMS };//longevity/ovipositing parameters
		enum TEWD{ ʎ0, ʎ1, ʎ2, ʎ3, ʎa, ʎb, NB_EWD_PARAMS };//entering winter diapause 
		enum TEAS{ μ, ѕ, Τᴴ¹, Τᴴ², NB_EAS_PARAMS };//Emerging Adult from Soil (spring)
		//Τᴴ, ʎ, к
	}


	//*****************************************************************************
	//CAprocerosLeucopodaEquations
	
	class CAprocerosLeucopodaEquations : public CEquationTableLookup
	{
	public:

		//static const double RDR[LOF::NB_STAGES][LOF::NB_RDR_PARAMS]; //relative development parameter
		
		//static const double OVP[TZZ::NB_OVP_PARAMS];//longevity/oviposition parameters
		static const double EWD[TZZ::NB_EWD_PARAMS];//entering winter diapause default parameters
		static const double EAS[TZZ::NB_EAS_PARAMS];//Emerging Adult from Soil (spring) default  parameters
		
		
		//double m_OVP[TZZ::NB_OVP_PARAMS];//longevity/oviposition parameters
//		double m_EWD[TZZ::NB_EWD_PARAMS];//entering winter diapause  parameters
	//	double m_EAS[TZZ::NB_EAS_PARAMS];//Emerging Adult from Soil (spring) parameters
		std::array<double, TZZ::NB_EWD_PARAMS> m_EWD;//entering winter diapause  parameters
		std::array<double, TZZ::NB_EAS_PARAMS> m_EAS;//Emerging Adult from Soil (spring) parameters

		
		CAprocerosLeucopodaEquations(const CRandomGenerator& RG);
		
		virtual double ComputeRate(size_t stage, double t)const;

		//relative development
		double GetRelativeDevRate(size_t s)const;
		double GetDailySurvivalRate(size_t s, double T)const;
		//double GetAdultLongevity()const;
		double GetFecondity()const;
		double GetBrood(double Fi, double T, double t, double delta_t)const;

		//double GetCreationCDD()const;
		double GetAdultEmergingCDD()const;

	};

}