//*****************************************************************************
// Class: CLeucotaraxisPiniperda, CLPMHost, CLPMStand
//
// Description:	CLeucotaraxisPiniperda represent a LNF insect or a group of insect with same characteristics. 
//				CLPMHost represent the Host that contain CLeucotaraxisPiniperda. 
//				CLPMStand represent the Host that contain CSWBHost. 
//*****************************************************************************

#pragma once

#include "Basic/UtilTime.h"
#include "ModelBase/IndividualBase.h"
#include "LeucotaraxisPiniperdaEquations.h"
#include "Basic\DegreeDays.h"

namespace WBSF
{

	class CModelStat;
	class CWeatherDay;

	namespace LPM // Leucotaraxis piniperda (Malloch) 
	{
		enum TLeucotaraxisPiniperdaStats
		{
			S_CDD, S_LARVA0, S_PUPA0, S_ADULT0, S_DEAD_ADULT0, S_EGG1, S_LARVA1, S_EMERGENCE0, S_DEAD_ATTRITION, NB_STATS
		}; 

		enum { NB_CUMUL_STATS = 5 };
		static const size_t CUM_STAT[NB_CUMUL_STATS] = { S_PUPA0, S_ADULT0, S_EGG1, S_EMERGENCE0, S_DEAD_ATTRITION };
	}


	//*******************************************************************************************************
	// CLeucotaraxisPiniperda

	class CLPMHost;
	class CLPMStand;
	class CLeucotaraxisPiniperda : public CIndividual
	{
	public:


		CLeucotaraxisPiniperda(WBSF::CHost* pHost, CTRef creationDate = CTRef(), double age = LPM::EGG, TSex sex = RANDOM_SEX, bool bFertil = true, size_t generation = 0, double scaleFactor = 1);
		CLeucotaraxisPiniperda(const CLeucotaraxisPiniperda& in) :WBSF::CIndividual(in){ operator=(in); }
		CLeucotaraxisPiniperda& operator=(const CLeucotaraxisPiniperda& in);
		~CLeucotaraxisPiniperda(void);

		virtual void OnNewDay(const CWeatherDay& weather)override;
		virtual void Live(const CHourlyData& weather, size_t dt)override;
		virtual void Live(const CWeatherDay& weather)override;
		virtual void Brood(const CWeatherDay& weather)override;
		virtual void Die(const CWeatherDay& weather)override;
		virtual void GetStat(CTRef d, CModelStat& stat)override;
		virtual double GetInstar(bool includeLast)const override;//	{ return (IsAlive() || m_death == OLD_AGE) ? std::min(m_age < LPM::L2o ? m_age : std::max(double(LPM::L2o), m_age - 1), double(LPM::NB_STAGES) - (includeLast ? 0.0 : 1.0)) : WBSF::CBioSIMModelBase::VMISS; }
		virtual bool NeedOverheating()const override { return true; }

		virtual void Pack(const WBSF::CIndividualPtr& in)override;
		virtual size_t GetNbStages()const override { return LPM::NB_STAGES; }
		virtual WBSF::CIndividualPtr CreateCopy()const override { return std::make_shared<CLeucotaraxisPiniperda>(*this); }

		inline CLPMHost* GetHost();
		inline const CLPMHost* GetHost()const;
		inline CLPMStand* GetStand();
		inline const CLPMStand* GetStand()const;
		inline const CLeucotaraxisPiniperdaEquations& Equations()const;

		
		CTRef GetAdultEmergence(int year)const;
		bool IsDeadByAttrition(size_t s, double T, double r)const;

	protected:

		//member
		
		double m_RDR[LPM::NB_STAGES]; //Individual's relative development rates for all stages
		CTRef m_adult_emergence_date;
		bool m_bDiapause;
		std::array<CTRef, LPM::NB_STAGES + 1> m_reachDate;

		double m_to; //pre oviposition period [days]
		double m_t; // decimal time since adult emergence [days]
		double m_Fi; //fecundity
		bool m_bDeadByAttrition;
	};

	//*******************************************************************************************************
	// CLPMHost

	class CLPMHost : public CHost
	{
	public:

		//public members
		CLPMHost(WBSF::CStand* pStand);

		virtual void Live(const CWeatherDay& weaDay);
		virtual void GetStat(CTRef d, CModelStat& stat, size_t generation = NOT_INIT)override;

	protected:

	};

	typedef std::shared_ptr<CLPMHost> CLPMHostPtr;


	//*******************************************************************************************************
	// CLPMStand
	class CLPMStand : public CStand
	{
	public:

		//global variables of all bugs
		bool m_bApplyAttrition;

		CLPMStand(WBSF::CBioSIMModelBase* pModel);

		virtual void GetStat(CTRef d, CModelStat& stat, size_t generation = NOT_INIT)override;


		void init(int year, const CWeatherYears& weather);
		
		CLeucotaraxisPiniperdaEquations m_equations;
		CModelStatVector m_adult_emergence_CDD;

		bool m_in_calibration;
	};


	//WARNING: cast must be defined here to avoid bug
	inline CLPMHost* CLeucotaraxisPiniperda::GetHost(){ return static_cast<CLPMHost*>(m_pHost); }
	inline const CLPMHost* CLeucotaraxisPiniperda::GetHost()const{ return static_cast<const CLPMHost*>(m_pHost); }
	inline CLPMStand* CLeucotaraxisPiniperda::GetStand(){ ASSERT(m_pHost); return static_cast<CLPMStand*>(GetHost()->GetStand()); }
	inline const CLPMStand* CLeucotaraxisPiniperda::GetStand()const{ ASSERT(m_pHost); return static_cast<const CLPMStand*>(GetHost()->GetStand()); }
	inline const CLeucotaraxisPiniperdaEquations& CLeucotaraxisPiniperda::Equations()const{ return GetStand()->m_equations; }


}