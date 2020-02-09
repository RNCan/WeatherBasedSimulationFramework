//*****************************************************************************
// Class: CLaricobiusNigrinus, CLNFHost, CLNFStand
//
// Description:	CLaricobiusNigrinus represent a LNF insect or a group of insect with same characteristics. 
//				CLNFHost represent the Host that contain CLaricobiusNigrinus. 
//				CLNFStand represent the Host that contain CSWBHost. 
//*****************************************************************************

#pragma once

#include "Basic/UtilTime.h"
#include "ModelBase/IndividualBase.h"
#include "LaricobiusNigrinusEquations.h"
#include "Basic\DegreeDays.h"

namespace WBSF
{

	class CModelStat;
	class CWeatherDay;

	namespace LNF
	{
		enum TLaricobiusNigrinusStats
		{
			S_EGG, S_LARVAE, S_PREPUPA, S_PUPA, S_AESTIVAL_DIAPAUSE_ADULT, S_ACTIVE_ADULT, S_DEAD_ADULT, S_DEAD_FROST, S_ADULT_EMERGENCE, S_EGG_CREATION_CDD, NB_STATS
		};
	}


	//*******************************************************************************************************
	// CLaricobiusNigrinus

	class CLNFHost;
	class CLNFStand;
	class CLaricobiusNigrinus : public CIndividual
	{
	public:

		static double AdjustTLab(const std::string& name, size_t s, CTRef TRef, double T);
		static double AdjustDLLab(const std::string& name, size_t s, CTRef TRef, double day_length);


		CLaricobiusNigrinus(WBSF::CHost* pHost, CTRef creationDate = CTRef(), double age = LNF::EGG, TSex sex = RANDOM_SEX, bool bFertil = true, size_t generation = 0, double scaleFactor = 1);
		CLaricobiusNigrinus(const CLaricobiusNigrinus& in) :WBSF::CIndividual(in){ operator=(in); }
		CLaricobiusNigrinus& operator=(const CLaricobiusNigrinus& in);
		~CLaricobiusNigrinus(void);

		virtual void OnNewDay(const CWeatherDay& weather)override;
		virtual void Live(const CHourlyData& weather, size_t dt)override;
		virtual void Live(const CWeatherDay& weather)override;
		virtual void Brood(const CWeatherDay& weather)override;
		virtual void Die(const CWeatherDay& weather)override;
		virtual void GetStat(CTRef d, CModelStat& stat)override;
		virtual double GetInstar(bool includeLast)const override;//	{ return (IsAlive() || m_death == OLD_AGE) ? std::min(m_age < LNF::L2o ? m_age : std::max(double(LNF::L2o), m_age - 1), double(LNF::NB_STAGES) - (includeLast ? 0.0 : 1.0)) : WBSF::CBioSIMModelBase::VMISS; }
		virtual bool NeedOverheating()const override { return true; }

		virtual void Pack(const WBSF::CIndividualPtr& in)override;
		virtual size_t GetNbStages()const override { return LNF::NB_STAGES; }
		virtual WBSF::CIndividualPtr CreateCopy()const override { return std::make_shared<CLaricobiusNigrinus>(*this); }

		inline CLNFHost* GetHost();
		inline const CLNFHost* GetHost()const;
		inline CLNFStand* GetStand();
		inline const CLNFStand* GetStand()const;
		inline const CLaricobiusNigrinusEquations& Equations()const;

		inline CTRef GetAdultEmergenceBegin(size_t y = 1)const;
		CTRef GetParentAdultEmergence()const;
		CTRef GetCreationDate(CTRef parentAdultEmergence)const;


	protected:

		//member
		
		//double m_creationCDD;//CDD need to create individual
		//double m_CDD;//actual CDD
		double m_RDR[LNF::NB_STAGES]; //Individual's relative development rates for all stages
		CTRef m_dropToGroundDate;
		CTRef m_adultDate;
		CTRef m_reachDate[LNF::NB_STAGES+1];
		//double m_CDD_ADE;//cumulative negative CDD for aestival diapause end
		//double m_aestivalDiapauseEndCDD;//CDD need to create individual
		//double m_aestivalDiapauseEndTavg30;
		
		//double m_CDD_AE;//adult emerging CDD
		//CTRef m_adultEmegenceBegin;
		CTRef m_parentAdultEmergence;
		double m_adultEmergingCDD;

		//size_t m_ii;

		double m_adult_longevity; //adult longevity [days]
		double m_F; //fecundity

		
		


	};

	//*******************************************************************************************************
	// CLNFHost

	class CLNFHost : public CHost
	{
	public:

		//public members
		CLNFHost(WBSF::CStand* pStand);

		virtual void Live(const CWeatherDay& weaDay);
		virtual void GetStat(CTRef d, CModelStat& stat, size_t generation = NOT_INIT)override;

	protected:

	};

	typedef std::shared_ptr<CLNFHost> CLNFHostPtr;


	//*******************************************************************************************************
	// CLNFStand
	class CLNFStand : public CStand
	{
	public:

		//global variables of all bugs
		bool m_bApplyAttrition;

		CLNFStand(WBSF::CBioSIMModelBase* pModel, double DDThresholdLo, double DDThresholdHi) :
			WBSF::CStand(pModel),
			m_equations(pModel->RandomGenerator()),
			m_DD(CDegreeDays::MODIFIED_ALLEN_WAVE, DDThresholdLo, DDThresholdHi),
			m_DD4(CDegreeDays::MODIFIED_ALLEN_WAVE, 4.0)
		{
			m_bApplyAttrition = false;
			m_egg_creation_CDD = 0;
		}

		virtual void GetStat(CTRef d, CModelStat& stat, size_t generation = NOT_INIT)override;


		void init(int year, const CWeatherYears& weather);
		

		void ComputeTavg30(int year, const CWeatherYears& weather);
		CTRef ComputeAdultEmergenceBegin(const CWeatherYear& weather)const;
		
		CModelStatVector m_Tavg30;
		CLaricobiusNigrinusEquations m_equations;
		CDegreeDays m_DD;
		CDegreeDays m_DD4;

		double m_egg_creation_CDD;
		std::array<CTRef, 2>  m_adultEmergenceBegin;
		
	};


	//WARNING: cast must be defined here to avoid bug
	inline CLNFHost* CLaricobiusNigrinus::GetHost(){ return static_cast<CLNFHost*>(m_pHost); }
	inline const CLNFHost* CLaricobiusNigrinus::GetHost()const{ return static_cast<const CLNFHost*>(m_pHost); }
	inline CLNFStand* CLaricobiusNigrinus::GetStand(){ ASSERT(m_pHost); return static_cast<CLNFStand*>(GetHost()->GetStand()); }
	inline const CLNFStand* CLaricobiusNigrinus::GetStand()const{ ASSERT(m_pHost); return static_cast<const CLNFStand*>(GetHost()->GetStand()); }
	inline const CLaricobiusNigrinusEquations& CLaricobiusNigrinus::Equations()const{ return GetStand()->m_equations; }


}