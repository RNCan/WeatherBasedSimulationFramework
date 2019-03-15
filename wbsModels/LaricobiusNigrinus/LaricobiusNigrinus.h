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
			S_EGG, S_LARVAE, S_PREPUPA, S_PUPA, S_ADULT, S_DEAD_ADULT, S_AVERAGE_INSTAR, NB_STATS
		};
	}


	//*******************************************************************************************************
	// CLaricobiusNigrinus

	class CLNFHost;
	class CLNFStand;
	class CLaricobiusNigrinus : public CIndividual
	{
	public:

		CLaricobiusNigrinus(WBSF::CHost* pHost, CTRef creationDate = CTRef(), double age = LNF::EGG, size_t sex = NOT_INIT, bool bFertil = true, size_t generation = 0, double scaleFactor = 1);
		CLaricobiusNigrinus(const CLaricobiusNigrinus& in) :WBSF::CIndividual(in){ operator=(in); }
		CLaricobiusNigrinus& operator=(const CLaricobiusNigrinus& in);
		~CLaricobiusNigrinus(void);

		virtual void Live(const CHourlyData& weather, size_t dt);
		virtual void Live(const CWeatherDay& weather);
		virtual void Brood(const CWeatherDay& weather);
		virtual void Die(const CWeatherDay& weather);
		virtual void GetStat(CTRef d, CModelStat& stat);
		virtual double GetInstar(bool includeLast)const;//	{ return (IsAlive() || m_death == OLD_AGE) ? std::min(m_age < LNF::L2o ? m_age : std::max(double(LNF::L2o), m_age - 1), double(LNF::NB_STAGES) - (includeLast ? 0.0 : 1.0)) : WBSF::CBioSIMModelBase::VMISS; }
		virtual bool NeedOverheating()const { return true; }

		virtual void Pack(const WBSF::CIndividualPtr& in);
		virtual size_t GetNbStages()const{ return LNF::NB_STAGES; }
		virtual WBSF::CIndividualPtr CreateCopy()const{ return std::make_shared<CLaricobiusNigrinus>(*this); }

		inline CLNFHost* GetHost();
		inline const CLNFHost* GetHost()const;
		inline CLNFStand* GetStand();
		inline const CLNFStand* GetStand()const;
		inline CLaricobiusNigrinusEquations& Equations();

	protected:

		//member
		double m_RDR[LNF::NB_STAGES]; //Individual's relative development rates for all stages
		double m_AL; //adult longevity [days]
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
		virtual void GetStat(CTRef d, CModelStat& stat, size_t generation = NOT_INIT);

	protected:

		//CDegreeDays m_DD;
		//double m_sumDD;
	};

	typedef std::shared_ptr<CLNFHost> CLNFHostPtr;


	//*******************************************************************************************************
	// CLNFStand
	class CLNFStand : public CStand
	{
	public:

		//global variables of all bugs
		bool m_bApplyAttrition;
		double m_maxTsoil;

		CLNFStand(WBSF::CBioSIMModelBase* pModel) :
			WBSF::CStand(pModel),
			m_equations(pModel->RandomGenerator())
		{
			m_bApplyAttrition = false;
			m_maxTsoil = 5.3;
		}

		CLaricobiusNigrinusEquations m_equations;

	};


	//WARNING: cast must be defined here to avoid bug
	inline CLNFHost* CLaricobiusNigrinus::GetHost(){ return static_cast<CLNFHost*>(m_pHost); }
	inline const CLNFHost* CLaricobiusNigrinus::GetHost()const{ return static_cast<const CLNFHost*>(m_pHost); }
	inline CLNFStand* CLaricobiusNigrinus::GetStand(){ ASSERT(m_pHost); return static_cast<CLNFStand*>(GetHost()->GetStand()); }
	inline const CLNFStand* CLaricobiusNigrinus::GetStand()const{ ASSERT(m_pHost); return static_cast<const CLNFStand*>(GetHost()->GetStand()); }
	inline CLaricobiusNigrinusEquations& CLaricobiusNigrinus::Equations(){ return GetStand()->m_equations; }


}