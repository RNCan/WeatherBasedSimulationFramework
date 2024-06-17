//*****************************************************************************
// Class: CLeucopisArgenticollis, CLAZHost, CLAZStand
//
// Description:	CLeucopisArgenticollis represent a LNF insect or a group of insect with same characteristics. 
//				CLAZHost represent the Host that contain CLeucopisArgenticollis. 
//				CLAZStand represent the Host that contain CSWBHost. 
//*****************************************************************************

#pragma once

#include "Basic/UtilTime.h"
#include "ModelBase/IndividualBase.h"
#include "LeucopisArgenticollisEquations.h"
#include "Basic\DegreeDays.h"

namespace WBSF
{

	class CModelStat;
	class CWeatherDay;

	namespace LAZ
	{
		enum TLeucopisArgenticollisStats
		{
			S_CDD, S_PUPA0, S_ADULT0, S_DEAD_ADULT0, S_EGG1, S_LARVA1, S_PUPA1, S_ADULT1, S_DEAD_ADULT1, S_EGG2, S_LARVA2, S_PUPA2, 
			S_EMERGENCE0, S_EMERGENCE1a, S_EMERGENCE1b, S_BROOD0, S_BROOD1, S_DEAD_ATTRITION, NB_STATS
		}; 


		//enum { NB_CUMUL_STATS = 10 };
		//static const size_t CUM_STAT[NB_CUMUL_STATS] = { S_ADULT0, S_EGG1, S_LARVA1, S_PUPA1, S_ADULT1, S_EGG2, S_LARVA2, S_EMERGENCE0, S_EMERGENCE1, S_DEAD_ATTRITION };
		static const bool CUM_STAT[NB_STATS] = { false, false, true, false, true, true, true, true, false, true, true, false, true, true, true, true, true};
	}


	//*******************************************************************************************************
	// CLeucopisArgenticollis

	class CLAZHost;
	class CLAZStand;
	class CLeucopisArgenticollis : public CIndividual
	{
	public:


		CLeucopisArgenticollis(WBSF::CHost* pHost, CTRef creationDate = CTRef(), double age = LAZ::EGG, TSex sex = RANDOM_SEX, bool bFertil = true, size_t generation = 0, double scaleFactor = 1);
		CLeucopisArgenticollis(const CLeucopisArgenticollis& in) :WBSF::CIndividual(in){ operator=(in); }
		CLeucopisArgenticollis& operator=(const CLeucopisArgenticollis& in);
		~CLeucopisArgenticollis(void);

		virtual void OnNewDay(const CWeatherDay& weather)override;
		virtual void Live(const CHourlyData& weather, size_t dt)override;
		virtual void Live(const CWeatherDay& weather)override;
		virtual void Brood(const CWeatherDay& weather)override;
		virtual void Die(const CWeatherDay& weather)override;
		virtual void GetStat(CTRef d, CModelStat& stat)override;
		virtual double GetInstar(bool includeLast)const override;//	{ return (IsAlive() || m_death == OLD_AGE) ? std::min(m_age < LAZ::L2o ? m_age : std::max(double(LAZ::L2o), m_age - 1), double(LAZ::NB_STAGES) - (includeLast ? 0.0 : 1.0)) : WBSF::CBioSIMModelBase::VMISS; }
		virtual bool NeedOverheating()const override { return true; }

		virtual void Pack(const WBSF::CIndividualPtr& in)override;
		virtual size_t GetNbStages()const override { return LAZ::NB_STAGES; }
		virtual WBSF::CIndividualPtr CreateCopy()const override { return std::make_shared<CLeucopisArgenticollis>(*this); }

		inline CLAZHost* GetHost();
		inline const CLAZHost* GetHost()const;
		inline CLAZStand* GetStand();
		inline const CLAZStand* GetStand()const;
		inline const CLeucopisArgenticollisEquations& Equations()const;

		CTRef GetCreationDate(int year)const;
		CTRef GetAdultEmergence(int year)const;
		bool IsDeadByAttrition(size_t s, double T, double r)const;

	protected:

		//member
		double m_RDR[LAZ::NB_STAGES]; //Individual's relative development rates for all stages
		CTRef m_adult_emergence_date;
		bool m_bDiapause;
		bool m_bDiapause1;//diapause in generation 1
		std::array<CTRef, LAZ::NB_STAGES + 1> m_reachDate;

		double m_to; //pre oviposition period [days]
		double m_t; // decimal time since adult emergence [days]
		double m_Fi; //fecundity
		bool m_bDeadByAttrition;
		std::array<double,2> m_generationSurvival;


	};

	//*******************************************************************************************************
	// CLAZHost

	class CLAZHost : public CHost
	{
	public:

		//public members
		CLAZHost(WBSF::CStand* pStand);

		virtual void Live(const CWeatherDay& weaDay);
		virtual void GetStat(CTRef d, CModelStat& stat, size_t generation = NOT_INIT)override;

	protected:

	};

	typedef std::shared_ptr<CLAZHost> CLAZHostPtr;


	//*******************************************************************************************************
	// CLAZStand
	class CLAZStand : public CStand
	{
	public:

		//global variables of all bugs
		bool m_bApplyAttrition;

		CLAZStand(WBSF::CBioSIMModelBase* pModel);

		virtual void GetStat(CTRef d, CModelStat& stat, size_t generation = NOT_INIT)override;


		void init(int year, const CWeatherYears& weather);
		
	//	CTRef ComputeDiapauseEnd(const CWeatherYear& weather)const;
		
		//CModelStatVector m_Tavg30;
		CLeucopisArgenticollisEquations m_equations;
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
	inline CLAZHost* CLeucopisArgenticollis::GetHost(){ return static_cast<CLAZHost*>(m_pHost); }
	inline const CLAZHost* CLeucopisArgenticollis::GetHost()const{ return static_cast<const CLAZHost*>(m_pHost); }
	inline CLAZStand* CLeucopisArgenticollis::GetStand(){ ASSERT(m_pHost); return static_cast<CLAZStand*>(GetHost()->GetStand()); }
	inline const CLAZStand* CLeucopisArgenticollis::GetStand()const{ ASSERT(m_pHost); return static_cast<const CLAZStand*>(GetHost()->GetStand()); }
	inline const CLeucopisArgenticollisEquations& CLeucopisArgenticollis::Equations()const{ return GetStand()->m_equations; }


}