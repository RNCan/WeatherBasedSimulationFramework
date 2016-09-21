//*****************************************************************************
// File: WSB.h
//
// Class: CWSpruceBudworm, CWSBVector
//          
//
// Description: CWSpruceBudworm represent one WSB insect. 
//			   CWSBVector is a vector of WSB insect. 
//*****************************************************************************

#pragma once

#include <crtdbg.h>
#include <vector>
#include "Basic/UtilTime.h"
#include "Basic/ModelStat.h"
#include "ModelBase/IndividualBase.h"
#include "WSBEquations.h"

namespace WBSF
{

	enum TWSBStat{
		S_EGG, S_L2o, S_L2, S_L3, S_L4, S_L5, S_L6, S_PUPAE, S_ADULT, S_DEAD_ADULT, S_OVIPOSITING_ADULT, S_BROOD, S_EGG2, S_L2o2, S_L22, S_L32,
		E_EGG, E_L2o, E_L2, E_L3, E_L4, E_L5, E_L6, E_PUPAE, E_ADULT, E_DEAD_ADULT, E_BROOD, E_TOTAL_BROOD, E_TOTAL_FEMALE, E_EGG2, E_L2o2, E_L22, E_L32,
		S_DEAD_ATTRITION, S_DEAD_FROZEN, S_DEAD_MISSING_ENERGY, S_DEAD_SYNCH, S_DEAD_WINDOW, S_DEAD,
		S_AVERAGE_INSTAR, S_P_MINEABLE, S_SHOOT_DEVEL,
		NB_WSB_STAT
	};


	class CWSBTree;
	class CWSBStand;
	class CWSpruceBudworm : public CIndividual
	{
	public:

		CWSpruceBudworm(CHost* pHost = NULL, CTRef creationDay = CTRef(), double age = EGG, size_t sex=NOT_INIT, bool bFertil = true, size_t generation = 0, double scaleFactor = 1);
		~CWSpruceBudworm(void);

		virtual void Live(const CWeatherDay& weather);
		virtual void Brood(const CWeatherDay& weather);
		virtual void Die(const CWeatherDay& weather);
		virtual void GetStat(CTRef d, CModelStat& stat);
		virtual size_t GetNbStages(void) const{ return NB_STAGES; }
		virtual double GetInstar(bool includeLast)const;
		virtual CIndividualPtr CreateCopy()const{ return std::make_shared<CWSpruceBudworm>(*this); }
		virtual bool NeedOverheating()const{ return GetStage() != L2o && GetStage() != ADULT; }


		inline CWSBTree* GetTree();
		inline const CWSBTree* GetTree()const;
		inline CWSBStand* GetStand();
		inline const CWSBStand* GetStand()const;
		inline CWSBTableLookup& Equations();

	protected:

		bool IsDeadByAttrition(double T, double r)const;
		bool IsDeadByOverwintering(double T, double dt);
		bool IsDeadByAsynchrony();
		bool IsDeadByWindow();
		bool ChangeStage(double RR){ return short(m_age + RR) != GetStage(); }

		//member
		double m_relativeDevRate[NB_STAGES]; //Individual's relative development rates in 9 stages
		CTRef m_OWDate; //When individue pass from Egg to OW, they must stop devel until next spring
		double m_potentialFecundity;
		bool m_bKillByAttrition;
		bool m_bKillByOverwintering;

		//survival
		double m_eggAge;
		double m_energy;
		double m_overwinterLuck;		//for overwinter survival
		double m_synchLuck;	//for synchrony survival
		double m_windowLuck;//for shoot development survival
		double m_onBole; //for overwintering survival (buffering of temperature)
		static const double EGG_FREEZING_POINT;
		static const double ADULT_FREEZING_POINT;
	};



	class CWSBTree : public CHost
	{
	public:

		CWSBTree(CStand* pStand = NULL);
		void clear();

		virtual void Live(const CWeatherDay& weather);
		virtual void GetStat(CTRef d, CModelStat& stat, size_t generation = -1);
		virtual void HappyNewYear();

		double GetDDays()const{ return m_ddays; }//Accumulated degree days with a base temp and a max temp.
		double GetProbBudMineable()const{ return m_probBudMineable; } //Proportion of buds that can be mined by a budworm
		double GetShootDevel()const{ return m_ddShoot; } //Proportion of buds that can be mined by a budworm


	protected:

		void ComputeMineable(const CWeatherDay& weather);
		void ComputeShootDevel(const CWeatherDay& weather);
		//Tree variables
		double m_ddays; //Accumulated degree days with a base temp and a max temp.
		double m_ddShoot; //Accumulated degree days for shoot development
		double m_probBudMineable; //Proportion of buds that can be mined by a budworm
	};

	//*******************************************************************************************************
	//*******************************************************************************************************
	// CWSBStand
	class CWSBStand : public CStand
	{
	public:

		


		//global variables of all bugs
		bool m_bApplyAttrition;
		bool m_bApplyWinterMortality;
		bool m_bApplyAsynchronyMortality;
		bool m_bApplyWindowMortality;
		bool m_bFertilEgg;
		double m_survivalRate;
		double m_defoliation;



		CWSBStand(CBioSIMModelBase* pModel) :
			CStand(pModel), 
			m_equations(pModel->RandomGenerator())
		{
			m_bApplyAttrition = true;
			m_bApplyWinterMortality = true;
			m_bApplyAsynchronyMortality = true;
			m_bApplyWindowMortality = true;
			m_bFertilEgg = false;
			m_survivalRate = 0.1;
			m_defoliation = 0.5;

		}


		CWSBTableLookup m_equations;

	protected:
	
	};



	//WARNING: cast must be defined here to avoid bug in cast
	inline CWSBTree* CWSpruceBudworm::GetTree(){ return static_cast<CWSBTree*>(m_pHost); }
	inline const CWSBTree* CWSpruceBudworm::GetTree()const{ return static_cast<const CWSBTree*>(m_pHost); }
	inline CWSBStand* CWSpruceBudworm::GetStand(){ ASSERT(m_pHost); return static_cast<CWSBStand*>(GetTree()->GetStand()); }
	inline const CWSBStand* CWSpruceBudworm::GetStand()const{ ASSERT(m_pHost); return static_cast<const CWSBStand*>(GetTree()->GetStand()); }
	inline CWSBTableLookup& CWSpruceBudworm::Equations(){ return GetStand()->m_equations; }

}