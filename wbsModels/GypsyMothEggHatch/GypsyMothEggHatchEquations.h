//*****************************************************************************
// File: SBDevelopment.h
//*****************************************************************************
#pragma once

#include <array>
#include <cassert>
#include "ModelBase/EquationTableLookup.h"
#include "Basic/UtilTime.h"
#include "Basic/ModelStat.h"
#include "Basic/WeatherStation.h"

#define RSA_MODEL 1


namespace WBSF
{

	


	//Lymantria dispar dispar(European gypsy moth)
	namespace LDD
	{
		enum TStages{ DIAPAUSE_EGG, EGG, LARVAE, NB_STAGES};
		
		enum TEOD{ Distribution, DOYb, DOYe, μ, ѕ, NCDD_TYPE, NCDDb, NCDDe, Τᴴ¹, Τᴴ², SIGMAb, SIGMAe, NB_EOD_PARAMS};//End Of Diapause (egg development begin)
		//enum TEOD { /*ʎ0, ʎ1, ʎ2, ʎ3, ʎa, ʎb,*/ NB_EOD_PARAMS = 6 };//End Of Diapause (egg development begin)
		enum TEDP{ /*ʎ0, ʎ1, ʎ2, ʎ3, ʎa, ʎb,*/ NB_EDP_PARAMS=6};//Egg development parameters
		enum TRDR { Ϙ1, Ϙ2, Ϙ3, NB_RDR_PARAMS }; //relative development parameter
	}


	//*****************************************************************************
	//CGypsyMothEggHatchEquations
	class CGypsyMothEggHatchEquations : public CEquationTableLookup
	{
	public:

		//default parameters
		static const std::array<double, LDD::NB_EOD_PARAMS> EOD;//Default End Of Diapause (egg development begin) parameters
		static const std::array<double, LDD::NB_EDP_PARAMS> EDP;//Default Egg development parameters
		static const std::array<double, LDD::NB_RDR_PARAMS> RDR;//Default End Of Diapause (egg development begin) parameters
		
		//variable parameters for calibration
		std::array<double, LDD::NB_EOD_PARAMS> m_EOD;//End Of Diapause (egg development begin) parameters
		std::array<double, LDD::NB_EDP_PARAMS> m_EDP;//Egg development parameters
		std::array<double, LDD::NB_RDR_PARAMS> m_RDR;//End Of Diapause (egg development begin) parameters
		
		
		CGypsyMothEggHatchEquations(const CRandomGenerator& RG);
		
		//relative development
		double GetRelativeDevlopmentRate(size_t stage)const;
		double GetColdTolerence()const;

		//CTRef ComputeEndOfDiapause(const CWeatherYear& weather, CModelStatVector& diapause_end_NCDD)const;
		void ComputeEndOfDiapause(const CWeatherYear& weather, CTRef& mu, double& sigma, CModelStatVector& diapause_end_NCDD)const;
		//double GetEndOfDiapauseNCDD()const;
		
		//CTRef GetEndOfDiapause(CTRef mu, double sigma)const;
	protected:

		virtual double ComputeDailyDevlopmentRate(size_t e, double T)const override;
		virtual double ComputeDailySurvivalRate(size_t e, double T)const override;
	};

}