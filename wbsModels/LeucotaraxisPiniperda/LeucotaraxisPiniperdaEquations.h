//*****************************************************************************
// File: SBDevelopment.h
//*****************************************************************************
#pragma once

#include "crtdbg.h"
#include "ModelBase/EquationTableLookup.h"
#include "Basic/DegreeDays.h"



namespace WBSF
{

	namespace LPM// Leucotaraxis piniperda (Malloch) 
	{
		enum TStages{ EGG, LARVAE, PUPAE, ADULT, DEAD_ADULT, NB_STAGES= DEAD_ADULT};

		
		enum TRDR { σ, NB_RDR_PARAMS }; //Relative Development Rate
		enum TEmergence{ μ, ѕ, delta, Τᴴ¹, Τᴴ², NB_EMERGENCE_PARAMS };//Emergence of Adult parameters
		enum TPUPA { NB_PUPA_DEV=6, PUPA_S= NB_PUPA_DEV, NB_PUPA_PARAMS };//
		enum TOVIP { NB_C_PARAMS = 3 };//
	}


	//*****************************************************************************
	//CLeucotaraxisPiniperdaEquations
	class CLeucotaraxisPiniperdaEquations : public CEquationTableLookup
	{
	public:

		static const std::array<double, LPM::NB_EMERGENCE_PARAMS> ADULT_EMERG;
		static const double PUPA_PARAM[LPM::NB_PUPA_PARAMS];//Pupa (without diapause) param
		static const double C_PARAM[LPM::NB_C_PARAMS];//Correction factor

		double m_adult_emerg[LPM::NB_EMERGENCE_PARAMS];//emergence of adult parameters
		double m_pupa_param[LPM::NB_PUPA_PARAMS];//Pupa parameters
		double m_C_param[LPM::NB_C_PARAMS];//Cumulative Egg Creation parameters
		
		
		CLeucotaraxisPiniperdaEquations(const CRandomGenerator& RG);
		
		virtual double ComputeRate(size_t stage, double t)const;

		//relative development
		double GetRelativeDevRate(size_t s)const;
		double GetDailySurvivalRate(size_t s, double T)const;
		double GetPreOvipPeriod()const;
		double GetFecundity(double L)const;

		void GetAdultEmergenceCDD(const CWeatherYears& weather, CModelStatVector& CDD)const;
		double GetAdultEmergingCDD()const;

	};

}