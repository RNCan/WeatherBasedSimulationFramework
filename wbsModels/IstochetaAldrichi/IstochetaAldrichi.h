//*****************************************************************************
// Class: CIstochetaAldrichi, CIAMHost, CIAMStand
//
// Description:	CIstochetaAldrichi represent a LNF insect or a group of insect with same characteristics. 
//				CIAMHost represent the Host that contain CIstochetaAldrichi. 
//				CIAMStand represent the Host that contain CSWBHost. 
//*****************************************************************************

#pragma once

#include "Basic/UtilTime.h"
#include "ModelBase/IndividualBase.h"
#include "IstochetaAldrichiEquations.h"
#include "Basic/DegreeDays.h"

namespace WBSF
{

	class CModelStat;
	class CWeatherDay;

	namespace IAM // Istocheta aldrichi (Mesnil) 
	{
		enum TIstochetaAldrichiStats
		{
			S_CDD, S_PUPA0, S_ADULT, S_DEAD_ADULT, S_EGG, S_LARVA, S_PUPA1, S_EMERGENCE, S_DEAD_ATTRITION, S_END_OF_DIAPAUSE, NB_STATS
		}; 

		enum { NB_CUMUL_STATS = 8 };
		static const size_t CUM_STAT[NB_CUMUL_STATS] = { S_PUPA0, S_ADULT, S_EGG, S_LARVA, S_PUPA1, S_EMERGENCE, S_DEAD_ATTRITION, S_END_OF_DIAPAUSE };
		
	}


	//*******************************************************************************************************
	// CIstochetaAldrichi

	class CIAMHost;
	class CIAMStand;
	class CIstochetaAldrichi : public CIndividual
	{
	public:


		CIstochetaAldrichi(WBSF::CHost* pHost, CTRef creationDate = CTRef(), double age = IAM::EGG, TSex sex = RANDOM_SEX, bool bFertil = true, size_t generation = 0, double scaleFactor = 1);
		CIstochetaAldrichi(const CIstochetaAldrichi& in) :WBSF::CIndividual(in){ operator=(in); }
		CIstochetaAldrichi& operator=(const CIstochetaAldrichi& in);
		~CIstochetaAldrichi(void);

		virtual void OnNewDay(const CWeatherDay& weather)override;
		void Live(const CHourlyData& weather, double Tsoil, size_t dt);
		virtual void Live(const CWeatherDay& weather)override;
		virtual void Brood(const CWeatherDay& weather)override;
		virtual void Die(const CWeatherDay& weather)override;
		virtual void GetStat(CTRef d, CModelStat& stat)override;
		virtual double GetInstar(bool includeLast)const override;//	{ return (IsAlive() || m_death == OLD_AGE) ? std::min(m_age < IAM::L2o ? m_age : std::max(double(IAM::L2o), m_age - 1), double(IAM::NB_STAGES) - (includeLast ? 0.0 : 1.0)) : WBSF::CBioSIMModelBase::VMISS; }
		virtual bool NeedOverheating()const override { return true; }

		virtual void Pack(const WBSF::CIndividualPtr& in)override;
		virtual size_t GetNbStages()const override { return IAM::NB_STAGES; }
		virtual WBSF::CIndividualPtr CreateCopy()const override { return std::make_shared<CIstochetaAldrichi>(*this); }

		inline CIAMHost* GetHost();
		inline const CIAMHost* GetHost()const;
		inline CIAMStand* GetStand();
		inline const CIAMStand* GetStand()const;
		inline const CIstochetaAldrichiEquations& Equations()const;

		
		double GetIndividualEndOfDiapauseFactor(int year)const;
		bool IsDeadByAttrition(size_t stage, double T, double i_r)const;

	protected:

		//member
		
		double m_RDR[IAM::NB_STAGES]; //Individual's relative development rates for all stages
		//CTRef m_end_of_diapause;
		double m_end_of_diapause_factor;
		bool m_bInDiapause;
		//std::array<CTRef, IAM::NB_STAGES + 1> m_reachDate;

		double m_to; //pre oviposition period [days]
		double m_t; // decimal time since adult emergence [days]
		double m_Fi; //fecundity
		bool m_bDeadByAttrition;
	};

	//*******************************************************************************************************
	// CIAMHost

	class CIAMHost : public CHost
	{
	public:

		//public members
		CIAMHost(WBSF::CStand* pStand);

		virtual void Live(const CWeatherDay& weaDay);
		virtual void GetStat(CTRef d, CModelStat& stat, size_t generation = NOT_INIT)override;

	protected:

	};

	typedef std::shared_ptr<CIAMHost> CIAMHostPtr;


	//*******************************************************************************************************
	// CIAMStand
	class CIAMStand : public CStand
	{
	public:

		//global variables of all bugs
		bool m_bApplyAttrition;

		CIAMStand(WBSF::CBioSIMModelBase* pModel, const CModelStatVector& Tsoil);

		virtual void GetStat(CTRef d, CModelStat& stat, size_t generation = NOT_INIT)override;


		void init(int year, const CWeatherYears& weather);
		
		CIstochetaAldrichiEquations m_equations;
		CModelStatVector m_EOD_NCDD;
		const CModelStatVector& m_soil_temperature;

		bool m_in_calibration;
	};


	//WARNING: cast must be defined here to avoid bug
	inline CIAMHost* CIstochetaAldrichi::GetHost(){ return static_cast<CIAMHost*>(m_pHost); }
	inline const CIAMHost* CIstochetaAldrichi::GetHost()const{ return static_cast<const CIAMHost*>(m_pHost); }
	inline CIAMStand* CIstochetaAldrichi::GetStand(){ ASSERT(m_pHost); return static_cast<CIAMStand*>(GetHost()->GetStand()); }
	inline const CIAMStand* CIstochetaAldrichi::GetStand()const{ ASSERT(m_pHost); return static_cast<const CIAMStand*>(GetHost()->GetStand()); }
	inline const CIstochetaAldrichiEquations& CIstochetaAldrichi::Equations()const{ return GetStand()->m_equations; }


}