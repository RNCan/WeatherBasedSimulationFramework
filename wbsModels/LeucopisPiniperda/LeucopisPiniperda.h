﻿//*****************************************************************************
// Class: CLeucopisPiniperda, CLPMHost, CLPMStand
//
// Description:	CLeucopisPiniperda represent a LNF insect or a group of insect with same characteristics. 
//				CLPMHost represent the Host that contain CLeucopisPiniperda. 
//				CLPMStand represent the Host that contain CSWBHost. 
//*****************************************************************************

#pragma once

#include "Basic/UtilTime.h"
#include "ModelBase/IndividualBase.h"
#include "LeucopisPiniperdaEquations.h"
#include "Basic\DegreeDays.h"

namespace WBSF
{

	class CModelStat;
	class CWeatherDay;

	namespace LPM // Leucopis piniperda (Malloch) 
	{
		enum TLeucopisPiniperdaStats
		{
			S_CDD, S_PUPA0, S_ADULT0, S_DEAD_ADULT0, S_EGG1, S_LARVA1, S_PUPA1, S_EMERGENCE0, S_DEAD_ATTRITION, NB_STATS
		}; 

		enum { NB_CUMUL_STATS = 5 };
		static const size_t CUM_STAT[NB_CUMUL_STATS] = { S_ADULT0, S_EGG1, S_LARVA1, S_EMERGENCE0, S_DEAD_ATTRITION };
	}


	//*******************************************************************************************************
	// CLeucopisPiniperda

	class CLPMHost;
	class CLPMStand;
	class CLeucopisPiniperda : public CIndividual
	{
	public:


		CLeucopisPiniperda(WBSF::CHost* pHost, CTRef creationDate = CTRef(), double age = LPM::EGG, TSex sex = RANDOM_SEX, bool bFertil = true, size_t generation = 0, double scaleFactor = 1);
		CLeucopisPiniperda(const CLeucopisPiniperda& in) :WBSF::CIndividual(in){ operator=(in); }
		CLeucopisPiniperda& operator=(const CLeucopisPiniperda& in);
		~CLeucopisPiniperda(void);

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
		virtual WBSF::CIndividualPtr CreateCopy()const override { return std::make_shared<CLeucopisPiniperda>(*this); }

		inline CLPMHost* GetHost();
		inline const CLPMHost* GetHost()const;
		inline CLPMStand* GetStand();
		inline const CLPMStand* GetStand()const;
		inline const CLeucopisPiniperdaEquations& Equations()const;

		//CTRef GetCreationDate(int year)const;
		CTRef GetAdultEmergence(int year)const;
		bool IsDeadByAttrition(size_t s, double T, double r)const;

	protected:

		//member
		
		//double m_creationDateCDD;
		//double m_creationCDD;//CDD need to create individual
		//double m_CDD;//actual CDD
		double m_RDR[LPM::NB_STAGES]; //Individual's relative development rates for all stages
		//CTRef m_dropToGroundDate;
		CTRef m_adult_emergence_date;
		bool m_bDiapause;
		//std::array < CTRef, 2> m_adult_emergence;
		std::array<CTRef, LPM::NB_STAGES + 1> m_reachDate;
		//double m_CDD_ADE;//cumulative negative CDD for aestival diapause end
		//double m_aestivalDiapauseEndCDD;//CDD need to create individual
		//double m_aestivalDiapauseEndTavg30;
		
		//double m_CDD_AE;//adult emerging CDD
		//CTRef m_adultEmegenceBegin;
		//CTRef m_parentAdultEmergence;
		//double m_adult_emerging_CDD;

		//size_t m_ii;

		//double m_adult_longevity; //adult longevity [days]
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
		
	//	CTRef ComputeDiapauseEnd(const CWeatherYear& weather)const;
		
		//CModelStatVector m_Tavg30;
		CLeucopisPiniperdaEquations m_equations;
		CModelStatVector m_adult_emergence_CDD;


		//CDegreeDays m_DD;
		//CDegreeDays m_DD4;

		//double m_egg_creation_CDD;
		//std::map<int, double> m_diapause_end_NCDD;
		//std::map<int, double> m_adult_emergence_CDD;
		//std::map<int, CTRef>  m_adultEmergenceBegin;
		//std::map<int,  CTRef> m_diapause_end;

		//double m_diapause_end_NCDD;
		//double m_adult_emergence_CDD;
		//CTRef  m_adultEmergenceBegin;
		//CTRef  m_diapause_end;


		//CTRef m_diapause_end;  
		
	};


	//WARNING: cast must be defined here to avoid bug
	inline CLPMHost* CLeucopisPiniperda::GetHost(){ return static_cast<CLPMHost*>(m_pHost); }
	inline const CLPMHost* CLeucopisPiniperda::GetHost()const{ return static_cast<const CLPMHost*>(m_pHost); }
	inline CLPMStand* CLeucopisPiniperda::GetStand(){ ASSERT(m_pHost); return static_cast<CLPMStand*>(GetHost()->GetStand()); }
	inline const CLPMStand* CLeucopisPiniperda::GetStand()const{ ASSERT(m_pHost); return static_cast<const CLPMStand*>(GetHost()->GetStand()); }
	inline const CLeucopisPiniperdaEquations& CLeucopisPiniperda::Equations()const{ return GetStand()->m_equations; }


}