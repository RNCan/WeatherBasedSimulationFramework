//*****************************************************************************
// Class: CTranosema, CTranosemaHost, CTranosemaStand
//
// Description:	CTranosema represent a spruce budworm insect or a group of insect with same carractéristics. 
//				CTranosemaHost represent the tree that contain CTranosema. 
//				CTranosemaStand represent the tree that contain CSWBTree. 
//*****************************************************************************

#pragma once

#include "Basic/UtilTime.h"
#include "ModelBase/IndividueBase.h"
#include "TranosemaEquations.h"


namespace WBSF
{

	class CModelStat;
	class CWeatherDay;

	namespace Tranosema
	{
		enum TTranosemaStats
		{
			S_EGG, S_PUPA, S_ADULT, S_DEAD_ADULT, S_OVIPOSITING_ADULT, S_BROOD, S_ATTRITION,
			E_EGG, E_PUPA, E_ADULT, E_DEAD_ADULT, E_OVIPOSITING_ADULT,
			NB_STATS
		};

	}


	typedef std::array<double, Tranosema::NB_STAGES> TranosemaArray;

	class CTranosemaHost;
	class CTranosemaStand;
	class CTranosema : public CIndividue
	{
	public:

		CTranosema(CHost* pHost, CTRef creationDate = CTRef(), double age = Tranosema::EGG, size_t sex = NOT_INIT, bool bFertil = true, size_t generation = 0, double scaleFactor = 1);
		CTranosema(const CTranosema& in) :CIndividue(in){ operator=(in); }
		CTranosema& operator=(const CTranosema& in);
		~CTranosema(void);

		virtual void Live(const CWeatherDay& weather);
		virtual void Brood(const CWeatherDay& weather);
		virtual void Die(const CWeatherDay& weather);
		virtual void GetStat(CTRef d, CModelStat& stat);
		virtual void Pack(const CIndividuePtr& in);
		virtual size_t GetNbStages()const{ return Tranosema::NB_STAGES; }
		virtual CIndividuePtr CreateCopy()const{ return std::make_shared<CTranosema>(*this); }


	protected:

		bool IsDeadByAttrition(size_t s, double T);

		//member
		TranosemaArray m_δ;		//Individual's relative development rates
		TranosemaArray m_luck;	//survival between stage
		bool	m_badluck;		//killed by attrition
		double	m_Pmax;			//Potential fecondity
		double	m_Pᵗ;			//Energy
		double	m_Eᵗ;			//Actual number of eggs

		inline CTranosemaStand* GetStand();
		inline const CTranosemaStand* GetStand()const;
		inline CTranosemaEquations& Equations();
	};


	//typedef CHostTemplate<CTranosema> CTranosemaHostBase;
	//class CTranosemaHost: public CTranosemaHostBase
	//{
	//public:
	//
	//	CTranosemaHost(CStand* pStand=NULL);
	//
	//	virtual void Live(const CWeatherDay& weaDay);
	//	virtual void GetStat(CTRef d, CModelStat& stat, size_t generation = NOT_INIT);
	//
	//	void CleanUp();
	//	bool GetHatchDate(CTRef& ref, double& d)const;
	//	CTRef GetFirstHatchDate()const;
	//
	//};
	//
	//typedef CTranosemaHost::CTBugVector CTranosemaBugVector;

	//*******************************************************************************************************
	//*******************************************************************************************************
	// CTranosemaStand
	class CTranosemaStand : public CStand
	{
	public:

		//global variables of all bugs
		bool m_bApplyAttrition;
		double m_generationAttrition;


		CTranosemaStand(CBioSIMModelBase* pModel) :
			CStand(pModel),
			m_equations(pModel->RandomGenerator())
		{
			m_bApplyAttrition = true;
			m_generationAttrition = 0.10;
		}

		CTranosemaEquations m_equations;
	};

	typedef std::shared_ptr<CTranosemaStand> CTranosemaStandPtr;

	//WARNING: cast must be defined here to avoid bug
	//inline CIndividuePtrContainer* CTranosema::GetHost(){ return dynamic_cast<CIndividuePtrContainer*>(m_pHost); }
	//inline const CIndividuePtrContainer* CTranosema::GetHost()const{ return dynamic_cast<const CIndividuePtrContainer*>(m_pHost); }
	inline CTranosemaStand* CTranosema::GetStand(){ ASSERT(m_pHost); return static_cast<CTranosemaStand*>(m_pHost->GetStand()); }
	inline const CTranosemaStand* CTranosema::GetStand()const{ ASSERT(m_pHost); return static_cast<const CTranosemaStand*>(m_pHost->GetStand()); }
	inline CTranosemaEquations& CTranosema::Equations(){ return GetStand()->m_equations; }
}