//*****************************************************************************
// Class: CForestTentCaterpillar, CFTCTree, CFTCStand
//
// Description:	CForestTentCaterpillar represent a FTC insect or a group of insect with same characteristics. 
//				CFTCTree represent the tree that contain CForestTentCaterpillar. 
//				CFTCStand represent the tree that contain CSWBTree. 
//*****************************************************************************

#pragma once

#include "Basic/UtilTime.h"
#include "ModelBase/IndividualBase.h"
#include "ForestTentCaterpillarEquations.h"
#include "Basic\DegreeDays.h"

namespace WBSF
{

	class CModelStat;
	class CWeatherDay;

	namespace FTC
	{
		enum TForestTentCaterpillarStats
		{
			S_EGG, S_L1, S_L2, S_L3, S_L4, S_L5, S_PUPA, S_ADULT, S_DEAD_ADULT, S_EMERGING_FEMALE, S_BROOD, S_AVERAGE_INSTAR, S_DD68, NB_STATS
		};
	}


	//*******************************************************************************************************
	// CForestTentCaterpillar

	class CFTCTree;
	class CFTCStand;
	class CForestTentCaterpillar : public CIndividual
	{
	public:

		CForestTentCaterpillar(WBSF::CHost* pHost, CTRef creationDate = CTRef(), double age = FTC::EGG, size_t sex = NOT_INIT, bool bFertil = true, size_t generation = 0, double scaleFactor = 1);
		CForestTentCaterpillar(const CForestTentCaterpillar& in) :WBSF::CIndividual(in){ operator=(in); }
		CForestTentCaterpillar& operator=(const CForestTentCaterpillar& in);
		~CForestTentCaterpillar(void);

		virtual void Live(const CHourlyData& weather, size_t dt);
		virtual void Live(const CWeatherDay& weather);
		virtual void Brood(const CWeatherDay& weather);
		virtual void Die(const CWeatherDay& weather);
		virtual void GetStat(CTRef d, CModelStat& stat);
		virtual double GetInstar(bool includeLast)const;//	{ return (IsAlive() || m_death == OLD_AGE) ? std::min(m_age < FTC::L2o ? m_age : std::max(double(FTC::L2o), m_age - 1), double(FTC::NB_STAGES) - (includeLast ? 0.0 : 1.0)) : WBSF::CBioSIMModelBase::VMISS; }
		virtual bool NeedOverheating()const { return true; }

		virtual void Pack(const WBSF::CIndividualPtr& in);
		virtual size_t GetNbStages()const{ return FTC::NB_STAGES; }
		virtual WBSF::CIndividualPtr CreateCopy()const{ return std::make_shared<CForestTentCaterpillar>(*this); }

		inline CFTCTree* GetTree();
		inline const CFTCTree* GetTree()const;
		inline CFTCStand* GetStand();
		inline const CFTCStand* GetStand()const;
		inline CForestTentCaterpillarEquations& Equations();

	protected:

		//double GetRelativeDevRate(double T, double r)const;

		//member
		double m_RDR[FTC::NB_STAGES]; //Individual's relative development rates for all stages
		double m_F; //fecundity
		static const double TREE_FACTOR[FTC::NB_TREES];
	};

	//*******************************************************************************************************
	// CFTCTree

	class CFTCTree : public CHost
	{
	public:

		//public members
		size_t m_kind;//kind of tree

		CFTCTree(WBSF::CStand* pStand);

		virtual void Live(const CWeatherDay& weaDay);
		virtual void GetStat(CTRef d, CModelStat& stat, size_t generation = NOT_INIT);

	protected:

		CDegreeDays m_DD;
		double m_sumDD;
	};

	typedef std::shared_ptr<CFTCTree> CFTCTreePtr;


	//*******************************************************************************************************
	// CFTCStand
	class CFTCStand : public CStand
	{
	public:

		//global variables of all bugs
		bool m_bApplyAttrition;

		CFTCStand(WBSF::CBioSIMModelBase* pModel) :
			WBSF::CStand(pModel),
			m_equations(pModel->RandomGenerator())
		{
			m_bApplyAttrition = false;
		}

		CForestTentCaterpillarEquations m_equations;

	};


	//WARNING: cast must be defined here to avoid bug
	inline CFTCTree* CForestTentCaterpillar::GetTree(){ return static_cast<CFTCTree*>(m_pHost); }
	inline const CFTCTree* CForestTentCaterpillar::GetTree()const{ return static_cast<const CFTCTree*>(m_pHost); }
	inline CFTCStand* CForestTentCaterpillar::GetStand(){ ASSERT(m_pHost); return static_cast<CFTCStand*>(GetTree()->GetStand()); }
	inline const CFTCStand* CForestTentCaterpillar::GetStand()const{ ASSERT(m_pHost); return static_cast<const CFTCStand*>(GetTree()->GetStand()); }
	inline CForestTentCaterpillarEquations& CForestTentCaterpillar::Equations(){ return GetStand()->m_equations; }


}