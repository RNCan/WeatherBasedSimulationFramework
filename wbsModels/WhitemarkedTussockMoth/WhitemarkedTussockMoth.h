//*****************************************************************************
// Class: CWhitemarkedTussockMoth, CWTMHost, CWTMStand
//
// Description:	CWhitemarkedTussockMoth represent a WTM insect or a group of insect with same characteristics. 
//				CWTMHost represent the Host that contain CWhitemarkedTussockMoth. 
//				CWTMStand represent the Host that contain CSWBHost. 
//*****************************************************************************

#pragma once

#include "Basic/UtilTime.h"
#include "ModelBase/IndividualBase.h"
#include "WhitemarkedTussockMothEquations.h"
#include "Basic\DegreeDays.h"

namespace WBSF
{

	class CModelStat;
	class CWeatherDay;

	namespace WTM
	{
		enum TWhitemarkedTussockMothStats
		{
			S_GDD, S_EGG0, S_LARVAE0, S_PUPA0, S_ADULT0, S_DEAD_ADULT0, S_EGG_MASS0, S_EMERGENCE0, S_EGG1, S_LARVAE1, S_PUPA1, S_ADULT1, S_DEAD_ADULT1, S_EGG_MASS1, S_EMERGENCE1, NB_STATS
		};
	}


	//*******************************************************************************************************
	// CWhitemarkedTussockMoth

	class CWTMHost;
	class CWTMStand;
	class CWhitemarkedTussockMoth : public CIndividual
	{
	public:

		CWhitemarkedTussockMoth(WBSF::CHost* pHost, CTRef creationDate = CTRef(), double age = WTM::EGG, TSex sex = RANDOM_SEX, bool bFertil = true, size_t generation = 0, double scaleFactor = 1);
		CWhitemarkedTussockMoth(const CWhitemarkedTussockMoth& in) :WBSF::CIndividual(in){ operator=(in); }
		CWhitemarkedTussockMoth& operator=(const CWhitemarkedTussockMoth& in);
		~CWhitemarkedTussockMoth(void);

		virtual void OnNewDay(const CWeatherDay& weather)override;
		virtual void Live(const CHourlyData& weather, size_t dt)override;
		virtual void Live(const CWeatherDay& weather)override;
		virtual void Brood(const CWeatherDay& weather)override;
		virtual void Die(const CWeatherDay& weather)override;
		virtual void GetStat(CTRef d, CModelStat& stat)override;
		virtual double GetInstar(bool includeLast)const override;//	{ return (IsAlive() || m_death == OLD_AGE) ? std::min(m_age < WTM::L2o ? m_age : std::max(double(WTM::L2o), m_age - 1), double(WTM::NB_STAGES) - (includeLast ? 0.0 : 1.0)) : WBSF::CBioSIMModelBase::VMISS; }
		virtual bool NeedOverheating()const override { return true; }

		virtual void Pack(const WBSF::CIndividualPtr& in)override;
		virtual size_t GetNbStages()const override { return WTM::NB_STAGES; }
		virtual WBSF::CIndividualPtr CreateCopy()const override { return std::make_shared<CWhitemarkedTussockMoth>(*this); }

		inline CWTMHost* GetHost();
		inline const CWTMHost* GetHost()const;
		inline CWTMStand* GetStand();
		inline const CWTMStand* GetStand()const;
		inline CWhitemarkedTussockMothEquations& Equations();

	protected:

		//member
		double m_diapauseGDD;//GDD need to hatch
		double m_GDD;//actual GDD
		double m_RDR[WTM::NB_STAGES]; //Individual's relative development rates for all stages
//		double m_AL; //adult longevity [days]
		double m_F; //fecundity
		CTRef m_transit[WTM::NB_STAGES+1];
	};

	//*******************************************************************************************************
	// CWTMHost

	class CWTMHost : public CHost
	{
	public:

		//public members
		CWTMHost(WBSF::CStand* pStand);

		virtual void Live(const CWeatherDay& weaDay);
		virtual void GetStat(CTRef d, CModelStat& stat, size_t generation = NOT_INIT);

	protected:

		//CDegreeDays m_DD;
		//double m_sumDD;
	};

	typedef std::shared_ptr<CWTMHost> CLNFHostPtr;


	//*******************************************************************************************************
	// CWTMStand
	class CWTMStand : public CStand
	{
	public:

		//global variables of all bugs
		bool m_bApplyAttrition;

		CWTMStand(WBSF::CBioSIMModelBase* pModel, double DDThreshold) :
			WBSF::CStand(pModel),
			m_equations(pModel->RandomGenerator()),
			m_DD(CDegreeDays::SINGLE_SINE, DDThreshold)
		{
			m_bApplyAttrition = false;
		}
		
		CWhitemarkedTussockMothEquations m_equations;
		CDegreeDays m_DD;
		double m_egg_factor[2];
		
	};


	//WARNING: cast must be defined here to avoid bug
	inline CWTMHost* CWhitemarkedTussockMoth::GetHost(){ return static_cast<CWTMHost*>(m_pHost); }
	inline const CWTMHost* CWhitemarkedTussockMoth::GetHost()const{ return static_cast<const CWTMHost*>(m_pHost); }
	inline CWTMStand* CWhitemarkedTussockMoth::GetStand(){ ASSERT(m_pHost); return static_cast<CWTMStand*>(GetHost()->GetStand()); }
	inline const CWTMStand* CWhitemarkedTussockMoth::GetStand()const{ ASSERT(m_pHost); return static_cast<const CWTMStand*>(GetHost()->GetStand()); }
	inline CWhitemarkedTussockMothEquations& CWhitemarkedTussockMoth::Equations(){ return GetStand()->m_equations; }


}