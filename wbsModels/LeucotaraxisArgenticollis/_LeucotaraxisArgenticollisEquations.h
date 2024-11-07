//*****************************************************************************
// File: SBDevelopment.h
//*****************************************************************************
#pragma once

#include "crtdbg.h"
#include "ModelBase/EquationTableLookup.h"
#include "Basic/DegreeDays.h"
//#include "ModelBase/ModelDistribution.h"


namespace WBSF
{

	namespace LAZ//Leucotaraxis argenticollis Zetterstedt, 1848
	{
		//DIAPAUSE_PUPAE,
		enum TStages{ EGG, LARVAE, PUPAE, ADULT, DEAD_ADULT, NB_STAGES= DEAD_ADULT};

		
		enum TRDR { σ, NB_RDR_PARAMS }; //Relative Development Rate
		enum TEmergence{ μ, ѕ, delta, Τᴴ¹, Τᴴ², NB_EMERGENCE_PARAMS };//Emergence of Adult parameters
		enum TPUPA { NB_PUPA_DEV=6, PUPA_S= NB_PUPA_DEV, NB_PUPA_PARAMS };//
		enum TOVIP { NB_OVIP_PARAMS = 3 };//
		enum TEOD { EOD_A, EOD_B, NB_EOD_PARAMS };
	}


	//*****************************************************************************
	//CLeucotaraxisArgenticollisEquations
	class CLeucotaraxisArgenticollisEquations : public CEquationTableLookup
	{
	public:

		static const std::array<double, LAZ::NB_EMERGENCE_PARAMS> ADULT_EMERG;
		static const std::array<double, LAZ::NB_PUPA_PARAMS> PUPA_PARAM;//Cumulative Egg Creation
		static const std::array<double, LAZ::NB_OVIP_PARAMS> OVIP_PARAM;//Cumulative Egg Creation
		static const std::array<double, LAZ::NB_EOD_PARAMS> EOD_PARAM;//End of diapause correction


		std::array<double, LAZ::NB_EMERGENCE_PARAMS> m_adult_emerg;//emergence of adult parameters
		std::array<double, LAZ::NB_PUPA_PARAMS> m_pupa_param;//Pupa parameters
		std::array<double, LAZ::NB_OVIP_PARAMS> m_ovip_param;//Cumulative Egg Creation parameters
		std::array<double, LAZ::NB_EOD_PARAMS > m_EOD_param;//End of diapause correction
		
		
		CLeucotaraxisArgenticollisEquations(const CRandomGenerator& RG);
		
		virtual double ComputeRate(size_t stage, double t)const;

		//relative development
		double GetPupaRate(double T)const;
		double GetPupaRDR()const;
		double GetRelativeDevRate(size_t s)const;
		double GetDailySurvivalRate(size_t s, double T)const;
		double GetPreOvipPeriod()const;
		double GetFecondity(double L)const;
		//double GetFecondityRate(double T)const;

		//double GetCumulativeEggCreationCDD()const;

		//CDegreeDays GetAdultEmergingDDModel()const;
		void GetAdultEmergingCDD(const CWeatherYears& weather, CModelStatVector& CDD)const;
		double GetAdultEmergingCDD(double Tjan)const;

	};

}