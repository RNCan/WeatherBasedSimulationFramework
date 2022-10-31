//*****************************************************************************
// Class: CAplanipennis, CEABHost, CEABStand
//
// Description:	CAplanipennis represent a EAB insect or a group of insect with same characteristics. 
//				CEABHost represent the Host that contain CAplanipennis. 
//				CEABStand represent the Host that contain CSWBHost. 
//*****************************************************************************

#pragma once

#include "Basic/UtilTime.h"
#include "ModelBase/IndividualBase.h"
#include "EABIndividualEquations.h"
//#include "Basic/DegreeDays.h"

namespace WBSF
{

	class CModelStat;
	class CWeatherDay;

	namespace EAB
	{
		enum TAplanipennisStats
		{
			S_PUPAE, S_ADULT, S_DEAD_ADULT, S_ADULT_EMERGENCE, NB_STATS
		};
	}


	//*******************************************************************************************************
	// CAplanipennis

	class CEABHost;
	class CEABStand;
	class CAplanipennis : public CIndividual
	{
	public:


		CAplanipennis(WBSF::CHost* pHost, CTRef creationDate = CTRef(), double age = EAB::PUPAE, TSex sex = RANDOM_SEX, bool bFertil = true, size_t generation = 0, double scaleFactor = 1);
		CAplanipennis(const CAplanipennis& in) :WBSF::CIndividual(in){ operator=(in); }
		CAplanipennis& operator=(const CAplanipennis& in);
		~CAplanipennis(void);

		virtual void OnNewDay(const CWeatherDay& weather)override;
		virtual void Live(const CHourlyData& weather, size_t dt)override;
		virtual void Live(const CWeatherDay& weather)override;
		virtual void Brood(const CWeatherDay& weather)override;
		virtual void Die(const CWeatherDay& weather)override;
		virtual void GetStat(CTRef d, CModelStat& stat)override;
		virtual double GetInstar(bool includeLast)const override;//	{ return (IsAlive() || m_death == OLD_AGE) ? std::min(m_age < LOF::L2o ? m_age : std::max(double(LOF::L2o), m_age - 1), double(LOF::NB_STAGES) - (includeLast ? 0.0 : 1.0)) : WBSF::CBioSIMModelBase::VMISS; }
		virtual bool NeedOverheating()const override { return true; }

		virtual void Pack(const WBSF::CIndividualPtr& in)override;
		virtual size_t GetNbStages()const override { return EAB::NB_STAGES; }
		virtual WBSF::CIndividualPtr CreateCopy()const override { return std::make_shared<CAplanipennis>(*this); }

		inline CEABHost* GetHost();
		inline const CEABHost* GetHost()const;
		inline CEABStand* GetStand();
		inline const CEABStand* GetStand()const;
		inline const CAplanipennisEquations& Equations()const;

//		bool IsDeadByAttrition(size_t s, double T, double r)const;

	protected:

		//member
		double m_RDR[EAB::NB_STAGES]; //Individual's relative development rates for all stages

		CTRef m_adult_emergence;
		std::array<CTRef, EAB::NB_STAGES + 1> m_reachDate;
	
//		double m_Fi; //fecundity
		//bool m_bDeadByAttrition;
	};

	//*******************************************************************************************************
	// CEABHost

	class CEABHost : public CHost
	{
	public:

		//public members
		CEABHost(WBSF::CStand* pStand);

		virtual void Live(const CWeatherDay& weaDay);
		virtual void GetStat(CTRef d, CModelStat& stat, size_t generation = NOT_INIT)override;

	protected:

	};

	typedef std::shared_ptr<CEABHost> CEABHostPtr;


	//*******************************************************************************************************
	// CEABStand
	class CEABStand : public CStand
	{
	public:

		//global variables of all bugs
		CEABStand(WBSF::CBioSIMModelBase* pModel);

		
		CAplanipennisEquations m_equations;
		double m_sigma;
		double m_psy_factor;
		
	};


	//WARNING: cast must be defined here to avoid bug
	inline CEABHost* CAplanipennis::GetHost(){ return static_cast<CEABHost*>(m_pHost); }
	inline const CEABHost* CAplanipennis::GetHost()const{ return static_cast<const CEABHost*>(m_pHost); }
	inline CEABStand* CAplanipennis::GetStand(){ ASSERT(m_pHost); return static_cast<CEABStand*>(GetHost()->GetStand()); }
	inline const CEABStand* CAplanipennis::GetStand()const{ ASSERT(m_pHost); return static_cast<const CEABStand*>(GetHost()->GetStand()); }
	inline const CAplanipennisEquations& CAplanipennis::Equations()const{ return GetStand()->m_equations; }


}