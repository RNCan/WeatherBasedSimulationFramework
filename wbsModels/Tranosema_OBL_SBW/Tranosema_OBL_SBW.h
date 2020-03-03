//*****************************************************************************
// Class: CTranosema, CTranosemaHost, CTranosemaStand
//
// Description:	CTranosema represent a spruce budworm insect or a group of insect with same characteristics. 
//				CTranosemaHost represent the tree that contain CTranosema. 
//				CTranosemaStand represent the tree that contain CSWBTree. 
//*****************************************************************************

#pragma once

#include "Basic/UtilTime.h"
#include "ModelBase/IndividualBase.h"
//#include "TranosemaEquations.h"
#include "Tranosema.h"
#include "ObliqueBandedLeafroller.h"
#include "SpruceBudworm.h"


namespace WBSF
{
	
	namespace Tranosema
	{
		enum TTranosema_OBL_SBW_Stats
		{
			S_NB_OBL = TTranosemaStats::NB_STATS, S_NB_OBL_L3D, S_NB_SBW,
			NB_STATS_EX
		};

	}


	class CTranosema_OBL_SBW_Host;
	class CTranosema_OBL_SBW_Stand;
	class CTranosema_OBL_SBW : public CTranosema
	{
	public:

		CTranosema_OBL_SBW(CHost* pHost, CTRef creationDate = CTRef(), double age = Tranosema::EGG, WBSF::TSex sex = WBSF::RANDOM_SEX, bool bFertil = true, size_t generation = 0, double scaleFactor = 1, CIndividualPtr& pAssociateHost = CIndividualPtr());
		CTranosema_OBL_SBW(const CTranosema_OBL_SBW& in) :CTranosema(in){ operator=(in); }
		CTranosema_OBL_SBW& operator=(const CTranosema_OBL_SBW& in);

		virtual void Live(const CWeatherDay& weather);
		virtual void Brood(const CWeatherDay& weather);
		virtual void Die(const CWeatherDay& weather);
		virtual void GetStat(CTRef d, CModelStat& stat);
		virtual size_t GetNbStages()const{ return Tranosema::NB_STAGES; }
		virtual CIndividualPtr CreateCopy()const{ return std::make_shared<CTranosema_OBL_SBW>(*this); }


		inline CTranosema_OBL_SBW_Host* GetHost();
		inline const CTranosema_OBL_SBW_Host* GetHost()const;
		inline CTranosema_OBL_SBW_Stand* GetStand();
		inline const CTranosema_OBL_SBW_Stand* GetStand()const;

		inline CTranosemaEquations & Equations();
		std::weak_ptr<CIndividual> m_pAssociateHost;
	};

	class CModelStat;
	class CWeatherDay;

	class CTranosema_OBL_SBW_Host : public CHost
	{
	public:

		CTranosema_OBL_SBW_Host(WBSF::CStand* pStand):
			CHost(pStand)
		{}
		
		void Initialize(const CInitialPopulation& initValue);

		inline CTranosema_OBL_SBW_Stand* GetStand();
		inline const CTranosema_OBL_SBW_Stand* GetStand()const;
	};

	//*******************************************************************************************************
	// CTranosemaStand
	class CTranosema_OBL_SBW_Stand : public CTranosemaStand
	{
	public:

		CObliqueBandedLeafrollerStand m_OBLStand;
		CSBWStand m_SBWStand;

		//global variables of all bugs
		CTranosema_OBL_SBW_Stand(CBioSIMModelBase* pModel) :
			CTranosemaStand(pModel),
			m_OBLStand(pModel),
			m_SBWStand(pModel)
		{
		}


		virtual void Live(const CWeatherDay& weather);
		virtual void GetStat(CTRef d, CModelStat& stat, size_t generation = NOT_INIT);
		virtual bool AdjustPopulation();
		virtual size_t GetNbObjectAlive()const;
		virtual void HappyNewYear();
		virtual double GetAI(bool bIncludeLast)const;

		virtual CHostPtr GetNearestHost(CHost* pHost);
		
		CIndividualPtr SelectRandomHost(bool bUseSBW);
	};

	//WARNING: cast must be defined here to avoid bug
	inline CTranosema_OBL_SBW_Host* CTranosema_OBL_SBW::GetHost(){ return static_cast<CTranosema_OBL_SBW_Host*>(m_pHost); }
	inline const CTranosema_OBL_SBW_Host* CTranosema_OBL_SBW::GetHost()const{ return static_cast<const CTranosema_OBL_SBW_Host*>(m_pHost); }
	inline CTranosema_OBL_SBW_Stand* CTranosema_OBL_SBW::GetStand(){ ASSERT(m_pHost); return static_cast<CTranosema_OBL_SBW_Stand*>(GetHost()->GetStand()); }
	inline const CTranosema_OBL_SBW_Stand* CTranosema_OBL_SBW::GetStand()const{ ASSERT(m_pHost); return static_cast<const CTranosema_OBL_SBW_Stand*>(GetHost()->GetStand()); }
	inline CTranosema_OBL_SBW_Stand* CTranosema_OBL_SBW_Host::GetStand(){ ASSERT(m_pStand); return static_cast<CTranosema_OBL_SBW_Stand*>(m_pStand); }
	inline const CTranosema_OBL_SBW_Stand* CTranosema_OBL_SBW_Host::GetStand()const{ ASSERT(m_pStand); return static_cast<const CTranosema_OBL_SBW_Stand*>(m_pStand); }

	inline CTranosemaEquations& CTranosema_OBL_SBW::Equations(){ return GetStand()->m_equations; }
}