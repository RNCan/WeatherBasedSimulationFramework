﻿//*****************************************************************************
// File: SBDevelopment.h
//*****************************************************************************
#pragma once

#include "crtdbg.h"
#include "ModelBase/EquationTableLookup.h"

namespace WBSF
{

	namespace LOF
	{
		enum TStages{ EGG, LARVAE1, LARVAE2, LARVAE3, LARVAE4, PREPUPAE, PUPAE, AESTIVAL_DIAPAUSE_ADULT, ACTIVE_ADULT, DEAD_ADULT, NB_STAGES = AESTIVAL_DIAPAUSE_ADULT};
		enum TRDR { σ, NB_RDR_PARAMS }; //relative development parameter
		enum TOvip{ μ, ѕ, Τᴴ¹, Τᴴ², NB_OVP_PARAMS };//Creation (original oviposition) parameters
		enum TAAD{ ʎ0, ʎ1, ʎ2, ʎ3, ʎa, ʎb, NB_ADE_PARAMS };//AestivalDiapauseEnd (Emerging adult begin)
		enum TEAS{ ʎ, к, Τᴴ, NB_EAS_PARAMS=3 };//EmergingAdult from soil
	}


	//*****************************************************************************
	//CLaricobiusOsakensisEquations
	class CLaricobiusOsakensisEquations : public CEquationTableLookup
	{
	public:

		//static const double RDR[LOF::NB_STAGES][LOF::NB_RDR_PARAMS]; //relative development parameter
		
		static const double OVP[LOF::NB_OVP_PARAMS];//oviposition parameters
		static const double ADE[LOF::NB_ADE_PARAMS];//AdultAestivalDiapauseEnd parameters
		static const double EAS[LOF::NB_EAS_PARAMS];//AdultAestivalDiapauseEnd parameters
		
		//double m_RDR[LOF::NB_STAGES][LOF::NB_RDR_PARAMS]; //relative development parameter
		double m_OVP[LOF::NB_OVP_PARAMS];//Creation (initial oviposition) parameters
		double m_ADE[LOF::NB_ADE_PARAMS];//AestivalDiapauseEnd parameters
		double m_EAS[LOF::NB_EAS_PARAMS];//Emerging Adult from Soil parameters
		
		CLaricobiusOsakensisEquations(const CRandomGenerator& RG);
		
		virtual double ComputeRate(size_t stage, double t)const;

		//relative development
		double GetRelativeDevRate(size_t s)const;
		double GetDailySurvivalRate(size_t s, double T)const;
		double GetAdultLongevity(size_t sex)const;
		double GetFecondity(double l)const;
		double GetCreationCDD()const;
		double GetAdultEmergingCDD()const;
		//double GetAestivalDiapauseEndCDD()const;
		//double GetTimeInSoil(double T, double day_length);
		//double GetAdultAestivalDiapauseRate(double T, double day_length, double creation_day, double pupation_time);
		//double GetAdultAbundance(double T, size_t j_day_since_jan);

	};

}