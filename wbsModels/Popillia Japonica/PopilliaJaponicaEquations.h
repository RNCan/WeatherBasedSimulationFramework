//*****************************************************************************
// File: SBDevelopment.h
//*****************************************************************************
#pragma once

#include <crtdbg.h>
#include <array>

#include "ModelBase/EquationTableLookup.h"
#include "ModelBase/ModelDistribution.h"


namespace WBSF
{

	
	

	

	//Popillia japonica (Newman)
	namespace PJN
	{
		enum TStages { EGG, L1, L2, L3, PUPA, ADULT, DEAD_ADULT, NB_STAGES };
		enum TPsy { NB_PSY = NB_STAGES };
		enum TOther { ADULT_LONGEVITY, ADULT_SD, ADULT_SURVIVAL, L3_SD, LITTER, SNDH_MU, SNDH_S, SNDH_F, NB_OTHER_PARAMS };
	}
	

	//*****************************************************************************
	//CPopilliaJaponicaEquations
	class CPopilliaJaponicaEquations : public CEquationTableLookup
	{
	public:
		
		static const std::array<double, PJN::NB_PSY > PSY;
		static const std::vector<double>  EOD_NA;
		static const std::vector<double>  EOD_EU;
		//static const std::array<double, NB_CDD_PARAMS> EOD;
		static const std::array<double, NB_CDD_PARAMS> ACC;
		static const std::array<double, PJN::NB_OTHER_PARAMS> OTHER_NA;
		static const std::array<double, PJN::NB_OTHER_PARAMS> OTHER_EU;
		
		//double m_emergence[LDW::NB_PARAMS];//Cumulative Egg Creation (first oviposition) parameters
		//double m_adult[LDW::NB_PARAMS];//Aestival Diapause End parameters
		std::vector<double> m_EOD;
		std::array<double, NB_CDD_PARAMS> m_ACC;
		std::array<double, PJN::NB_PSY > m_psy;
		std::array<double, PJN::NB_OTHER_PARAMS> m_other;




		CPopilliaJaponicaEquations(const CRandomGenerator& RG);
		virtual double ComputeRate(size_t stage, double t)const;
		virtual bool HaveChange()const;

		

		//relative development
		double GetRelativeDevRate(size_t s)const;
		double GetRelativeDevRate(double sigma)const;
		//double GetFecondity()const;

		double GetDailySurvivalRate(size_t s, double T)const;
		//double GetEndOfDiapauseCDD()const;
		
		double GetAdultCatchCumul(double CDD)const;


		/*double GetRate(size_t s, double t, bool bInDiapuse)const
		{
			Init();
			return bInDiapuse ? GetRateDiapause(t) : CEquationTableLookup::GetRate(s, t);
		}*/

		double GetRateDiapause(double t)const;
		double GetCoolingPoint()const;
		double GetEndOfDiapauseCDD()const;
	
	protected:

		std::vector<double> m_last_EOD;
		std::array<double, NB_CDD_PARAMS> m_last_ACC;
		std::array<double, PJN::NB_PSY > m_last_psy;

	};

}