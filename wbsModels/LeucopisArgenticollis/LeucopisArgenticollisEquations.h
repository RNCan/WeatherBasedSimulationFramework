﻿//*****************************************************************************
// File: SBDevelopment.h
//*****************************************************************************
#pragma once

#include "crtdbg.h"
#include "ModelBase/EquationTableLookup.h"
#include "Basic/DegreeDays.h"
//#include "ModelBase/ModelDistribution.h"


namespace WBSF
{

	namespace LAZ//Leucopis argenticollis Zetterstedt, 1848
	{
		//DIAPAUSE_PUPAE,
		enum TStages{ EGG, LARVAE, PUPAE, ADULT, DEAD_ADULT, NB_STAGES= DEAD_ADULT};

		
		enum TRDR { σ, NB_RDR_PARAMS }; //Relative Development Rate
		enum TEmergence{ μ, ѕ, delta, Τᴴ¹, Τᴴ², NB_EMERGENCE_PARAMS };//Emergence of Adult parameters
		enum TPUPA { NB_PUPA_DEV=6, PUPA_S= NB_PUPA_DEV, NB_PUPA_PARAMS };//
		//enum TADULT { NB_ADULT_PARAMS = 5 };//
		enum TOVIP { NB_OVIP_PARAMS = 3 };//
	}


	//*****************************************************************************
	//CLeucopisArgenticollisEquations
	class CLeucopisArgenticollisEquations : public CEquationTableLookup
	{
	public:

		static const std::array<double, LAZ::NB_EMERGENCE_PARAMS> ADULT_EMERG;
		//static const double ADULT_EMERG[LAZ::NB_EMERGENCE_PARAMS];//Emergence of Adult parameters
		static const double PUPA_PARAM[LAZ::NB_PUPA_PARAMS];//Cumulative Egg Creation
		//static const double ADULT_PARAM[LAZ::NB_ADULT_PARAMS];//Cumulative Egg Creation
		static const double OVIP_PARAM[LAZ::NB_OVIP_PARAMS];//Cumulative Egg Creation

		double m_adult_emerg[LAZ::NB_EMERGENCE_PARAMS];//emergence of adult parameters
		//double m_adult_param[LAZ::NB_ADULT_PARAMS];//Cumulative Egg Creation parameters
		double m_pupa_param[LAZ::NB_PUPA_PARAMS];//Pupa parameters
		double m_ovip_param[LAZ::NB_OVIP_PARAMS];//Cumulative Egg Creation parameters
		
		
		CLeucopisArgenticollisEquations(const CRandomGenerator& RG);
		
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
		double GetAdultEmergingCDD()const;

	};

}