//*****************************************************************************
// File: SBDevelopment.h
//*****************************************************************************
#pragma once

#include <cassert>
#include "ModelBase/EquationTableLookup.h"
#include "Basic/DegreeDays.h"


namespace WBSF
{

	namespace LAZ//Leucotaraxis argenticollis (Zetterstedt, 1848)
	{
		enum TStages{ EGG, LARVAE, PUPAE, ADULT, DEAD_ADULT, NB_STAGES= DEAD_ADULT};

		
		enum TRDR { σ, NB_RDR_PARAMS }; //Relative Development Rate
		enum TEmergence{ μ, ѕ, delta, Τᴴ¹, Τᴴ², NB_EMERGENCE_PARAMS };//Emergence of Adult parameters
		enum TPUPA { NB_PUPA_DEV=6, PUPA_S= NB_PUPA_DEV, NB_PUPA_PARAMS };//
		enum TCorrection { C_P0, C_P1, C_P2, C_P3, NB_C_PARAMS };//Correction param 
		enum TEOD { EOD_A, EOD_B, NB_EOD_PARAMS };
	}


	//*****************************************************************************
	//CLeucotaraxisArgenticollisEquations
	class CLeucotaraxisArgenticollisEquations : public CEquationTableLookup
	{
	public:

		static const std::array<double, LAZ::NB_EMERGENCE_PARAMS> ADULT_EMERG;
		static const std::array<double, LAZ::NB_PUPA_PARAMS> PUPA_PARAM;//Pupa (without diapause) parameters
		static const std::array<double, LAZ::NB_C_PARAMS> C_PARAM;//Correctionfactor
		static const std::array<double, LAZ::NB_EOD_PARAMS> EOD_PARAM;//End of diapause correction


		std::array<double, LAZ::NB_EMERGENCE_PARAMS> m_adult_emerg;
		std::array<double, LAZ::NB_PUPA_PARAMS> m_pupa_param;
		std::array<double, LAZ::NB_C_PARAMS> m_C_param;
		std::array<double, LAZ::NB_EOD_PARAMS > m_EOD_param;
		
		
		CLeucotaraxisArgenticollisEquations(const CRandomGenerator& RG);
		
		virtual double ComputeRate(size_t stage, double t)const;

		
		//double GetAdultAging(double T)const;
		double GetUndiapausedPupaRate(double T, size_t g)const;
		double GetUndiapausedPupaRDR(size_t g)const;
		double GetRelativeDevRate(size_t s)const;
		double GetDailySurvivalRate(size_t s, double T)const;
		double GetPreOvipPeriod()const;
		double GetFecondity(double L)const;
		
		//void GetAdultEmergenceCDD(const CWeatherYears& weather, std::array < CModelStatVector, 2>& CDD)const;
		//double GetAdultEmergingCDD(double Tjan)const;

	};

}