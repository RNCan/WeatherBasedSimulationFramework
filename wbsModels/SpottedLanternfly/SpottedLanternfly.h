//*****************************************************************************
// Class: CSpottedLanternfly, CLDWHost, CLDWStand
//
// Description:	CSpottedLanternfly represent a LDW insect or a group of insect with same characteristics. 
//				CLDWHost represent the Host that contain CSpottedLanternfly. 
//				CLDWStand represent the Host that contain CSWBHost. 
//*****************************************************************************

#pragma once

#include "Basic/UtilTime.h"
#include "ModelBase/IndividualBase.h"
//#include "ModelBase/ModelDistribution.h"
#include "SpottedLanternflyEquations.h"


namespace WBSF
{

	class CModelStat;
	class CWeatherDay;

	namespace LDW
	{


		enum TSpottedLanternflyStats
		{
			S_EGG, S_N1, S_N2, S_N3, S_N4, S_ADULT, S_DEAD_ADULT, S_EGG_HATCH, S_ADULT_EMERGENCE, S_AVEARGE_INSTAR,
			S_DEAD_BY_FROST, S_DEAD_ATTRITION, NB_STATS
		};
	}


	//*******************************************************************************************************
	// CSpottedLanternfly

	class CLDWHost;
	class CLDWStand;
	class CSpottedLanternfly : public CIndividual
	{
	public:


		CSpottedLanternfly(WBSF::CHost* pHost, CTRef creationDate = CTRef(), double age = LDW::EGG, TSex sex = RANDOM_SEX, bool bFertil = true, size_t generation = 0, double scaleFactor = 1);
		CSpottedLanternfly(const CSpottedLanternfly& in) :WBSF::CIndividual(in){ operator=(in); }
		CSpottedLanternfly& operator=(const CSpottedLanternfly& in);
		~CSpottedLanternfly(void);

		virtual void OnNewDay(const CWeatherDay& weather)override;
		virtual void Live(const CHourlyData& weather, size_t dt)override;
		virtual void Live(const CWeatherDay& weather)override;
		virtual void Brood(const CWeatherDay& weather)override;
		virtual void Die(const CWeatherDay& weather)override;
		virtual void GetStat(CTRef d, CModelStat& stat)override;
		virtual double GetInstar(bool includeLast)const override;//	{ return (IsAlive() || m_death == OLD_AGE) ? std::min(m_age < LOF::L2o ? m_age : std::max(double(LOF::L2o), m_age - 1), double(LOF::NB_STAGES) - (includeLast ? 0.0 : 1.0)) : WBSF::CBioSIMModelBase::VMISS; }
		virtual bool NeedOverheating()const override { return true; }

		virtual void Pack(const WBSF::CIndividualPtr& in)override;
		virtual size_t GetNbStages()const override { return LDW::NB_STAGES; }
		virtual WBSF::CIndividualPtr CreateCopy()const override { return std::make_shared<CSpottedLanternfly>(*this); }

		inline CLDWHost* GetHost();
		inline const CLDWHost* GetHost()const;
		inline CLDWStand* GetStand();
		inline const CLDWStand* GetStand()const;
		inline const CSpottedLanternflyEquations& Equations()const;

		bool IsDeadByAttrition(size_t s, double T, double r)const;

	protected:

		//member
		double m_EOD; //End of diapause CDD
		CTRef m_EODDate;
		double m_RDR[LDW::NB_STAGES]; //Individual's relative development rates for all stages

		//CTRef m_adult_emergence;
		std::array<CTRef, LDW::NB_STAGES> m_reachDate;
	
//		double m_Fi; //fecundity
		bool m_bDeadByAttrition;
	};

	//*******************************************************************************************************
	// CLDWHost

	class CLDWHost : public CHost
	{
	public:

		//public members
		CLDWHost(WBSF::CStand* pStand);

		virtual void Live(const CWeatherDay& weaDay);
		virtual void GetStat(CTRef d, CModelStat& stat, size_t generation = NOT_INIT)override;

	protected:

	};

	typedef std::shared_ptr<CLDWHost> CLDWHostPtr;


	//*******************************************************************************************************
	// CLDWStand
	class CLDWStand : public CStand
	{
	public:

		//global variables of all bugs
		CLDWStand(WBSF::CBioSIMModelBase* pModel);

		void InitStand(const std::array<double, NB_CDD_PARAMS>& P, const CWeatherYear& weather);
		
		CSpottedLanternflyEquations m_equations;
//		double m_sigma;
		
		bool m_bApplyAttrition;
		bool m_bApplyFrost;
		//CModelStatVector m_EOD_CDD;
		//CModelStatVector m_EOD_CDF;
		double m_overwinter_s;
		//CModelStatVector m_diapause;
		
	};


	//WARNING: cast must be defined here to avoid bug
	inline CLDWHost* CSpottedLanternfly::GetHost(){ return static_cast<CLDWHost*>(m_pHost); }
	inline const CLDWHost* CSpottedLanternfly::GetHost()const{ return static_cast<const CLDWHost*>(m_pHost); }
	inline CLDWStand* CSpottedLanternfly::GetStand(){ ASSERT(m_pHost); return static_cast<CLDWStand*>(GetHost()->GetStand()); }
	inline const CLDWStand* CSpottedLanternfly::GetStand()const{ ASSERT(m_pHost); return static_cast<const CLDWStand*>(GetHost()->GetStand()); }
	inline const CSpottedLanternflyEquations& CSpottedLanternfly::Equations()const{ return GetStand()->m_equations; }


}