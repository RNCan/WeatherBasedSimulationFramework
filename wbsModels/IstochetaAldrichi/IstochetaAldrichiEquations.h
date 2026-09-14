//*****************************************************************************
// File: SBDevelopment.h
//*****************************************************************************
#pragma once

#include "crtdbg.h"
#include "ModelBase/EquationTableLookup.h"
#include "Basic/DegreeDays.h"





namespace WBSF
{

	namespace IAM// Istocheta aldrichi (Malloch) 
	{
		enum TStages{ EGG, LARVAE, PUPAE, ADULT, DEAD_ADULT, NB_STAGES};

		
		enum TRDR { σ, NB_RDR_PARAMS }; //Relative Development Rate
		enum TEmergence{ μ, ѕ, delta, Τᴴ¹, Τᴴ², NB_EMERGENCE_PARAMS };//Emergence of Adult parameters
		enum TPUPA { NB_PUPA_DEV=6, PUPA_S= NB_PUPA_DEV, NB_PUPA_PARAMS };//
		enum TOVIP { NB_C_PARAMS = 3 };//
	}


	//*****************************************************************************
	//CIstochetaAldrichiEquations
	class CIstochetaAldrichiEquations : public CEquationTableLookup
	{
	public:

		static const std::array<double, IAM::NB_EMERGENCE_PARAMS> ADULT_EMERG;
		static const std::array<double, IAM::NB_PUPA_PARAMS> PUPA_PARAM;//Pupa (without diapause) param
		static const std::array<double, IAM::NB_C_PARAMS> C_PARAM;//Correction factor

		std::array<double, IAM::NB_EMERGENCE_PARAMS> m_adult_emerg;//emergence of adult parameters
		std::array<double, IAM::NB_PUPA_PARAMS> m_pupa_param;//Pupa parameters
		std::array<double, IAM::NB_C_PARAMS> m_C_param;//Cumulative Egg Creation parameters
		
		
		CIstochetaAldrichiEquations(const CRandomGenerator& RG);
		
		//virtual double ComputeRate(size_t stage, double t)const;

		//relative development
		double GetRelativeDevRate(size_t s)const;
		double GetPreOvipPeriod()const;
		double GetFecundity()const;

		void GetEndOfDiapauseNCDD(const CModelStatVector& weather, CModelStatVector& CDD)const;
		//double GetIndividualEndOfDiapauseNCDD(double NCDD)const;
		double GetEmergenceFactor(double NCDD)const;
		//double GetPupaRate(double T)const;
		//double GetPupaRDR()const;

	protected:

		virtual double ComputeDailyDevlopmentRate(size_t e, double T)const override;
		virtual double ComputeDailySurvivalRate(size_t e, double T)const override;
	};

}