//*****************************************************************************
// Class: CLaricobiusOsakensis, CLNFHost, CLNFStand
//
// Description:	CLaricobiusOsakensis represent a LNF insect or a group of insect with same characteristics. 
//				CLNFHost represent the Host that contain CLaricobiusOsakensis. 
//				CLNFStand represent the Host that contain CSWBHost. 
//*****************************************************************************

#pragma once

#include "Basic/UtilTime.h"
#include "ModelBase/IndividualBase.h"
#include "LaricobiusOsakensisEquations.h"
#include "Basic\DegreeDays.h"

namespace WBSF
{

	class CModelStat;
	class CWeatherDay;

	namespace LOF
	{
		enum TLaricobiusOsakensisStats
		{
			S_EGG, S_L1, S_L2, S_L3, S_L4, S_PREPUPA, S_PUPA, S_AESTIVAL_DIAPAUSE_ADULT, S_ACTIVE_ADULT, S_DEAD_ADULT,
			S_M_EGG, S_M_L1, S_M_L2, S_M_L3, S_M_L4, S_M_PREPUPA, S_M_PUPA, S_M_AESTIVAL_DIAPAUSE_ADULT, S_M_ACTIVE_ADULT, S_M_DEAD_ADULT,
			S_DEAD_ATTRTION, S_EGG_CREATION_CDD, S_DIAPAUSE_END_NCDD, S_ADULT_EMERGENCE_CDD, NB_STATS
			//S_ADULT_EMERGENCE = S_M_ACTIVE_ADULT
		};
	}


	//*******************************************************************************************************
	// CLaricobiusOsakensis

	class CLNFHost;
	class CLNFStand;
	class CLaricobiusOsakensis : public CIndividual
	{
	public:


		CLaricobiusOsakensis(WBSF::CHost* pHost, CTRef creationDate = CTRef(), double age = LOF::EGG, TSex sex = RANDOM_SEX, bool bFertil = true, size_t generation = 0, double scaleFactor = 1);
		CLaricobiusOsakensis(const CLaricobiusOsakensis& in) :WBSF::CIndividual(in){ operator=(in); }
		CLaricobiusOsakensis& operator=(const CLaricobiusOsakensis& in);
		~CLaricobiusOsakensis(void);

		virtual void OnNewDay(const CWeatherDay& weather)override;
		virtual void Live(const CHourlyData& weather, size_t dt)override;
		virtual void Live(const CWeatherDay& weather)override;
		virtual void Brood(const CWeatherDay& weather)override;
		virtual void Die(const CWeatherDay& weather)override;
		virtual void GetStat(CTRef d, CModelStat& stat)override;
		virtual double GetInstar(bool includeLast)const override;//	{ return (IsAlive() || m_death == OLD_AGE) ? std::min(m_age < LOF::L2o ? m_age : std::max(double(LOF::L2o), m_age - 1), double(LOF::NB_STAGES) - (includeLast ? 0.0 : 1.0)) : WBSF::CBioSIMModelBase::VMISS; }
		virtual bool NeedOverheating()const override { return true; }

		virtual void Pack(const WBSF::CIndividualPtr& in)override;
		virtual size_t GetNbStages()const override { return LOF::NB_STAGES; }
		virtual WBSF::CIndividualPtr CreateCopy()const override { return std::make_shared<CLaricobiusOsakensis>(*this); }

		inline CLNFHost* GetHost();
		inline const CLNFHost* GetHost()const;
		inline CLNFStand* GetStand();
		inline const CLNFStand* GetStand()const;
		inline const CLaricobiusOsakensisEquations& Equations()const;

		//inline CTRef GetAdultEmergenceBegin(size_t y = 1)const;
		//CTRef GetParentAdultEmergence()const;
		CTRef GetCreationDate(int year)const;
		CTRef GetAdultEmergence(int year)const;
		bool IsDeadByAttrition(size_t s, double T, double r)const;

	protected:

		//member
		
		//double m_creationCDD;//CDD need to create individual
		//double m_CDD;//actual CDD
		double m_RDR[LOF::NB_STAGES]; //Individual's relative development rates for all stages
		CTRef m_dropToGroundDate;
		//CTRef m_adult_emergence;
		//CTRef m_adultDate;
		CTRef m_adult_emergence;
		std::array<CTRef, LOF::NB_STAGES + 1> m_reachDate;
		//double m_CDD_ADE;//cumulative negative CDD for aestival diapause end
		//double m_aestivalDiapauseEndCDD;//CDD need to create individual
		//double m_aestivalDiapauseEndTavg30;
		
		//double m_CDD_AE;//adult emerging CDD
		//CTRef m_adultEmegenceBegin;
		//CTRef m_parentAdultEmergence;
		//double m_adult_emerging_CDD;

		//size_t m_ii;

		//double m_adult_longevity; //adult longevity [days]
		double m_t; // decimal time since adult emergence  [days]
		double m_Fi; //fecundity
		bool m_bDeadByAttrition;
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

		CLNFStand(WBSF::CBioSIMModelBase* pModel, double Th1, double Th2) :
			WBSF::CStand(pModel),
			m_equations(pModel->RandomGenerator()),
			m_DD(CDegreeDays::MODIFIED_ALLEN_WAVE, Th1, Th2)
//			m_DD4(CDegreeDays::MODIFIED_ALLEN_WAVE, 4.0)
		{
			m_bApplyAttrition = false;
			m_egg_creation_CDD = 0;
			m_diapause_end_NCDD = 0;
			m_adult_emergence_CDD = 0;
		}

		virtual void GetStat(CTRef d, CModelStat& stat, size_t generation = NOT_INIT)override;


		void init(int year, const CWeatherYears& weather);
		
		CTRef ComputeDiapauseEnd(const CWeatherYear& weather)const;
		
		CModelStatVector m_Tavg30;
		CLaricobiusOsakensisEquations m_equations;
		CDegreeDays m_DD;
		//CDegreeDays m_DD4;

		double m_egg_creation_CDD;
		double m_diapause_end_NCDD;
		double m_adult_emergence_CDD;
		//std::array<CTRef, 2>  m_adultEmergenceBegin;
		CTRef m_diapause_end; // or adult emergence begin;
		
	};


	//WARNING: cast must be defined here to avoid bug
	inline CLNFHost* CLaricobiusOsakensis::GetHost(){ return static_cast<CLNFHost*>(m_pHost); }
	inline const CLNFHost* CLaricobiusOsakensis::GetHost()const{ return static_cast<const CLNFHost*>(m_pHost); }
	inline CLNFStand* CLaricobiusOsakensis::GetStand(){ ASSERT(m_pHost); return static_cast<CLNFStand*>(GetHost()->GetStand()); }
	inline const CLNFStand* CLaricobiusOsakensis::GetStand()const{ ASSERT(m_pHost); return static_cast<const CLNFStand*>(GetHost()->GetStand()); }
	inline const CLaricobiusOsakensisEquations& CLaricobiusOsakensis::Equations()const{ return GetStand()->m_equations; }


}