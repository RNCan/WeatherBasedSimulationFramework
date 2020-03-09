//*****************************************************************************
// Class: CTranosema, CTranosemaHost, CTranosemaStand
//
// Description:	CTranosema represent a spruce budworm insect or a group of insect with same carractéristics. 
//				CTranosemaHost represent the tree that contain CTranosema. 
//				CTranosemaStand represent the tree that contain CSWBTree. 
//*****************************************************************************

#pragma once

#include "Basic/UtilTime.h"
#include "ModelBase/IndividualBase.h"
#include "TranosemaEquations.h"


namespace WBSF
{

	class CModelStat;
	class CWeatherDay;

	namespace Tranosema
	{
		enum TTranosemaStats
		{
			S_EGG, S_PUPA, S_ADULT, S_DEAD_ADULT, S_OVIPOSITING_ADULT, S_BROOD, S_ATTRITION, S_HOST_DIE, S_CUMUL_REATCH_ADULT, S_DIAPAUSE, 
			E_EGG, E_PUPA, E_ADULT, E_DEAD_ADULT, E_OVIPOSITING_ADULT, E_BROOD, E_ATTRITION, E_FROZEN, E_HOST_DIE, E_OTHERS, E_DIAPAUSE, E_DIAPAUSE_AGE,
			NB_STATS
		};

	}


	typedef std::array<double, Tranosema::NB_STAGES> TranosemaArray;

	class CTranosemaHost;
	class CTranosemaStand;
	class CTranosema : public CIndividual
	{
	public:

		CTranosema(CHost* pHost, CTRef creationDate = CTRef(), double age = Tranosema::EGG, WBSF::TSex sex = WBSF::RANDOM_SEX, bool bFertil = true, size_t generation = 0, double scaleFactor = 1);
		CTranosema(const CTranosema& in) :CIndividual(in){ operator=(in); }
		CTranosema& operator=(const CTranosema& in);
		~CTranosema(void);

		virtual void Live(const CWeatherDay& weather);
		virtual void Brood(const CWeatherDay& weather);
		virtual void Die(const CWeatherDay& weather);
		virtual void GetStat(CTRef d, CModelStat& stat);
		virtual bool CanPack(const CIndividualPtr& in)const;
		virtual void Pack(const CIndividualPtr& in);
		virtual size_t GetNbStages()const{ return Tranosema::NB_STAGES; }
		virtual CIndividualPtr CreateCopy()const{ return std::make_shared<CTranosema>(*this); }


	protected:

		bool IsDeadByAttrition(size_t s, double T);

		//member
		TranosemaArray m_δ;		//Individual's relative development rates
		TranosemaArray m_luck;	//survival between stage
		bool	m_badluck;		//killed by attrition
		double	m_Pmax;			//Potential fecundity
		double	m_Pᵗ;			//Energy
		double	m_Eᵗ;			//Actual number of eggs

		CTRef	m_diapauseTRef;
		double	m_Nh;			//Number of hosts (C. rosaceana) that are in larval stages, excluding L3D;

		inline CTranosemaStand* GetStand();
		inline const CTranosemaStand* GetStand()const;
		inline CTranosemaEquations& Equations();
	};



	//*******************************************************************************************************
	//*******************************************************************************************************
	// CTranosemaStand
	class CTranosemaStand : public CStand
	{
	public:

		//global variables of all bugs
		bool	m_bApplyAttrition;
		double	m_generationAttrition;
		bool	m_bAutoComputeDiapause;
		double	m_diapauseAge;
		double	m_lethalTemp;
		double	m_criticalDaylength;


		CTranosemaStand(CBioSIMModelBase* pModel) :
			CStand(pModel),
			m_equations(pModel->RandomGenerator())
		{
			m_bApplyAttrition = true;
			m_generationAttrition = 0.10;
			m_bAutoComputeDiapause = true;
			m_diapauseAge = 0.1;
			m_lethalTemp = -5.0;
			m_criticalDaylength = 13.5;
		}

		CTranosemaEquations m_equations;
	};

	typedef std::shared_ptr<CTranosemaStand> CTranosemaStandPtr;

	//WARNING: cast must be defined here to avoid bug
	//inline CIndividualPtrContainer* CTranosema::GetHost(){ return dynamic_cast<CIndividualPtrContainer*>(m_pHost); }
	//inline const CIndividualPtrContainer* CTranosema::GetHost()const{ return dynamic_cast<const CIndividualPtrContainer*>(m_pHost); }
	inline CTranosemaStand* CTranosema::GetStand(){ ASSERT(m_pHost); return static_cast<CTranosemaStand*>(m_pHost->GetStand()); }
	inline const CTranosemaStand* CTranosema::GetStand()const{ ASSERT(m_pHost); return static_cast<const CTranosemaStand*>(m_pHost->GetStand()); }
	inline CTranosemaEquations& CTranosema::Equations(){ return GetStand()->m_equations; }
}