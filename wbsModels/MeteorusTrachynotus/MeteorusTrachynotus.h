//*****************************************************************************
// Class: CMeteorusTrachynotus, CMeteorusTrachynotusHost, CMeteorusTrachynotusStand
//
//*****************************************************************************

#pragma once

#include "Basic/UtilTime.h"
#include "ModelBase/IndividualBase.h"
#include "MeteorusTrachynotusEquations.h"


namespace WBSF
{

	class CModelStat;
	class CWeatherDay;

	namespace MeteorusTrachynotus
	{
		enum TMeteorusTrachynotusStats
		{
			S_IMMATURE, S_PUPA, S_ADULT, S_DEAD_ADULT, S_OVIPOSITING_ADULT, S_BROODS_OBL, S_BROODS_SBW, S_ATTRITION, S_HOST_DIE, S_CUMUL_REATCH_ADULT, S_DIAPAUSE,
			M_IMMATURE, M_PUPA, M_ADULT, M_DEAD_ADULT, M_OVIPOSITING_ADULT, M_ATTRITION, M_FROZEN, M_HOST_DIE, M_OTHERS, M_DIAPAUSE, M_DIAPAUSE_AGE,
			NB_STATS
		};

	}


	typedef std::array<double, MeteorusTrachynotus::NB_STAGES> MeteorusTrachynotusArray;

	class CMeteorusTrachynotusHost;
	class CMeteorusTrachynotusStand;
	class CMeteorusTrachynotus : public CIndividual
	{
	public:

		CMeteorusTrachynotus(CHost* pHost, CTRef creationDate = CTRef(), double age = MeteorusTrachynotus::IMMATURE, WBSF::TSex sex = WBSF::RANDOM_SEX, bool bFertil = true, size_t generation = 0, double scaleFactor = 1);
		CMeteorusTrachynotus(const CMeteorusTrachynotus& in) :CIndividual(in){ operator=(in); }
		CMeteorusTrachynotus& operator=(const CMeteorusTrachynotus& in);
		~CMeteorusTrachynotus(void);

		virtual void Live(const CWeatherDay& weather);
		//virtual void Brood(const CWeatherDay& weather);
		virtual void Die(const CWeatherDay& weather);
		virtual void GetStat(CTRef d, CModelStat& stat);
		virtual bool CanPack(const CIndividualPtr& in)const;
		virtual void Pack(const CIndividualPtr& in);
		virtual size_t GetNbStages()const{ return MeteorusTrachynotus::NB_STAGES; }
		virtual CIndividualPtr CreateCopy()const{ return std::make_shared<CMeteorusTrachynotus>(*this); }

	protected:

		//member
		MeteorusTrachynotusArray m_δ;		//Individual's relative development rates
		double	m_Pmax;			//Potential fecundity
		CTRef	m_adultDate;
		CTRef	m_diapauseTRef;
		double	m_Nh;			//Number of hosts (C. rosaceana and C. fumiferana) that are in larval stages, excluding L3D;

		

		inline CMeteorusTrachynotusHost* GetHost();
		inline const CMeteorusTrachynotusHost* GetHost()const;
		inline CMeteorusTrachynotusStand* GetStand();
		inline const CMeteorusTrachynotusStand* GetStand()const;
		inline CMeteorusTrachynotusEquations& Equations();
		//inline COBLPostDiapauseEquations& OBL_Equations();
	};
	
	
	//*******************************************************************************************************
	//*******************************************************************************************************
	// CMeteorusTrachynotusHost
	class CMeteorusTrachynotusHost : public CHost
	{
	public:
		
		CMeteorusTrachynotusHost(WBSF::CStand* pStand);

		double m_diapause_age; //actual state of overwintering post diapause host
		double m_δ;//Individual's relative overwintering post diapause host

		virtual void Live(const CWeatherDay& weaDay)override;
		inline COBLPostDiapauseEquations& OBL_Equations();
	};

	typedef std::shared_ptr<CMeteorusTrachynotusHost> CMeteorusTrachynotusHostPtr;

	
	//*******************************************************************************************************
	//*******************************************************************************************************
	// CMeteorusTrachynotusStand
	class CMeteorusTrachynotusStand : public CStand
	{
	public:

		//global variables of all bugs
		double	m_generationAttrition;
		double m_lethalTemp;
		double m_criticalDaylength;
		double m_preOvip;


		CMeteorusTrachynotusStand(CBioSIMModelBase* pModel) :
			CStand(pModel),
			m_equations(pModel->RandomGenerator()),
			m_OBL_equations(pModel->RandomGenerator())
		{
			m_generationAttrition = 0.01;
			m_lethalTemp = -5.0;
			m_preOvip = 3.0 /22.4;
			m_criticalDaylength = 13.5;
		}

		CMeteorusTrachynotusEquations m_equations;
		COBLPostDiapauseEquations m_OBL_equations;
	};


	//WARNING: cast must be defined here to avoid bug
	inline CMeteorusTrachynotusHost* CMeteorusTrachynotus::GetHost(){ return dynamic_cast<CMeteorusTrachynotusHost*>(m_pHost); }
	inline const CMeteorusTrachynotusHost* CMeteorusTrachynotus::GetHost()const{ return dynamic_cast<const CMeteorusTrachynotusHost*>(m_pHost); }
	inline CMeteorusTrachynotusStand* CMeteorusTrachynotus::GetStand(){ ASSERT(m_pHost); return static_cast<CMeteorusTrachynotusStand*>(m_pHost->GetStand()); }
	inline const CMeteorusTrachynotusStand* CMeteorusTrachynotus::GetStand()const{ ASSERT(m_pHost); return static_cast<const CMeteorusTrachynotusStand*>(m_pHost->GetStand()); }
	inline CMeteorusTrachynotusEquations& CMeteorusTrachynotus::Equations(){ return GetStand()->m_equations; }
	inline COBLPostDiapauseEquations& CMeteorusTrachynotusHost::OBL_Equations() { return ((CMeteorusTrachynotusStand*)m_pStand)->m_OBL_equations; }
}