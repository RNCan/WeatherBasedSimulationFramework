//*****************************************************************************
// Class: CObliqueBandedLeafroller, CObliqueBandedLeafrollerHost, CObliqueBandedLeafrollerStand
//
// Description:	CObliqueBandedLeafroller represent a insect or a group of insect with same carractéristics. 
//				CObliqueBandedLeafrollerHost represent the tree that contain CObliqueBandedLeafroller. 
//				CObliqueBandedLeafrollerStand represent the tree that contain CSWBTree. 
//*****************************************************************************

#pragma once

#include "Basic/UtilTime.h"
#include "ModelBase/IndividualBase.h"
#include "ObliqueBandedLeafrollerEquations.h"


namespace WBSF
{

	class CModelStat;
	class CWeatherDay;

	namespace OBL
	{
		enum TObliqueBandedLeafrollerStats
		{
			S_EGG, S_L1, S_L2, S_L3, S_L3D, S_L4, S_L5, S_L6, S_PUPA, S_OVIP_ADULT, S_ADULT, S_DEAD_ADULT, S_OVIPOSITING_ADULT, S_BROOD, S_FROZEN,
			E_EGG, E_L1, E_L2, E_L3, E_L3D, E_L4, E_L5, E_L6, E_PUPA, E_OVIP_ADULT, E_ADULT, E_DEAD_ADULT, E_OVIPOSITING_ADULT, E_DIAPAUSE,
			NB_STATS
		};
	}
	
	//ObliqueBandedLeafroller
	class CObliqueBandedLeafrollerHost;
	class CObliqueBandedLeafrollerStand;
	class CObliqueBandedLeafroller : public CIndividual
	{
	public:

		CObliqueBandedLeafroller(CHost* pHost, CTRef creationDate = CTRef(), double age = OBL::EGG, WBSF::TSex sex = WBSF::RANDOM_SEX, bool bFertil = true, size_t generation = 0, double scaleFactor = 1);
		CObliqueBandedLeafroller(const CObliqueBandedLeafroller& in) :CIndividual(in){ operator=(in); }
		CObliqueBandedLeafroller& operator=(const CObliqueBandedLeafroller& in);
		~CObliqueBandedLeafroller(void);

		virtual void Live(const CWeatherDay& weather);
		virtual void Brood(const CWeatherDay& weather);
		virtual void Die(const CWeatherDay& weather);
		virtual void GetStat(CTRef d, CModelStat& stat);
		virtual bool CanPack(const CIndividualPtr& in)const;
		virtual void Pack(const CIndividualPtr& in);
		virtual size_t GetNbStages()const{ return OBL::NB_STAGES; }
		virtual CIndividualPtr CreateCopy()const{ return std::make_shared<CObliqueBandedLeafroller>(*this); }
		virtual bool IsInDiapause(CTRef TRef)const{ return m_bRequireDiapause || (GetStage() == OBL::L3D && TRef.GetYear() == m_diapauseTRef.GetYear()); }

	protected:

		
		//member
		std::array<double, OBL::NB_STAGES> m_δ;		//Individual's relative development rates
		CTRef	m_diapauseTRef;
		bool	m_bRequireDiapause;
		double  m_ovipAge;

		inline CObliqueBandedLeafrollerStand* GetStand();
		inline const CObliqueBandedLeafrollerStand* GetStand()const;
		inline CObliqueBandedLeafrollerEquations& Equations();
	};



	//*******************************************************************************************************
	//*******************************************************************************************************
	// CObliqueBandedLeafrollerStand
	class CObliqueBandedLeafrollerStand : public CStand
	{
	public:

		//global variables of all bugs
		bool m_bApplyAttrition;
		double m_generationAttrition;
		//double m_diapauseAge;
		double m_lethalTemp;
		double m_criticalDaylength;


		CObliqueBandedLeafrollerStand(CBioSIMModelBase* pModel) :
			CStand(pModel),
			m_equations(pModel->RandomGenerator())
		{
			m_bApplyAttrition = true;
			m_generationAttrition = 0.01;
			m_lethalTemp=-5;
			m_criticalDaylength=14.5;
		}

		CObliqueBandedLeafrollerEquations m_equations;
	};

	typedef std::shared_ptr<CObliqueBandedLeafrollerStand> CObliqueBandedLeafrollerStandPtr;

	//WARNING: cast must be defined here to avoid bug
	inline CObliqueBandedLeafrollerStand* CObliqueBandedLeafroller::GetStand(){ ASSERT(m_pHost); return static_cast<CObliqueBandedLeafrollerStand*>(m_pHost->GetStand()); }
	inline const CObliqueBandedLeafrollerStand* CObliqueBandedLeafroller::GetStand()const{ ASSERT(m_pHost); return static_cast<const CObliqueBandedLeafrollerStand*>(m_pHost->GetStand()); }
	inline CObliqueBandedLeafrollerEquations& CObliqueBandedLeafroller::Equations(){ return GetStand()->m_equations; }
}