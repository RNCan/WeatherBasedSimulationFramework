//*****************************************************************************
// File: HL.h
//
// Class: CHemlockLooper, CHLVector
//          
//
// Descrition: CHemlockLooper represent one HL insect. 
//			   CHLVector is a vector of HL insect. 
//*****************************************************************************

#pragma once

#define NOMINMAX
#include <crtdbg.h>
#include <vector>
#include "Basic/UtilTime.h"
#include "Basic/ModelStat.h"
#include "Basic/WeatherStation.h"
#include "Modelbase/IndividueBase.h"
#include "HemlockLooperEquations.h"

namespace WBSF
{

	
	enum THLStat{
		S_EGG, S_L1, S_L2, S_L3, S_L4, S_PUPA, S_ADULT, S_DEAD_ADULT, S_BROOD,
		E_EGG, E_L1, E_L2, E_L3, E_L4, E_PUPA, E_ADULT, E_DEAD_ADULT, E_BROOD, E_FEMALE, E_SWEIGHT, E_SENERGY, E_SCOLD, E_SHATCH, E_NB_HATCH,
		S_DEAD_ATTRITION, S_DEAD_OVERWINTER, S_DEAD, S_AVERAGE_INSTAR,
		NB_HL_STAT
	};


	class CHLTree;
	class CHLStand;
	class CHemlockLooper : public CIndividue
	{
	public:

		CHemlockLooper(CHost* pHost = NULL, CTRef creationDay = CTRef(), double age = HemlockLooper::EGG, size_t sex = NOT_INIT, bool bFertil = true, size_t generation = 0, double scaleFactor = 1);
		virtual ~CHemlockLooper(void){}

		virtual void Live(const CWeatherDay& weather);
		virtual void Brood(const CWeatherDay& weather);
		virtual void Die(const CWeatherDay& weather);
		virtual void GetStat(CTRef d, CModelStat& stat);
		virtual CIndividuePtr CreateCopy()const{ return std::make_shared<CHemlockLooper>(*this); }
		virtual bool NeedOverheating()const{ return GetStage() != HemlockLooper::EGG && GetStage() != HemlockLooper::ADULT; }
		virtual size_t GetNbStages()const{ return HemlockLooper::NB_STAGES; }

		double GetRelativeDevRate(int s)const { _ASSERTE(s >= 0 && s < HemlockLooper::NB_STAGES); return m_relativeDevRate[s]; } //Reports individual's relative development rate in "stage" 

	protected:

		CHLTree* GetTree();
		const CHLTree* GetTree()const;
		CHLStand* GetStand();
		const CHLStand* GetStand()const;

		void AssignRelativeDevRate(); //Sets indivivual's relative development rates in all stages, at creation

		void Develop(const CDailyWaveVector& T, const CWeatherDay& wDay);
		bool IsDeadByAttrition(double T, double r)const;
		bool ChangeStage(double RR){ return short(m_age + RR) != GetStage(); }


		//member
		double m_relativeDevRate[HemlockLooper::NB_STAGES];	//Individual's relative development rates in 9 stages
		double m_potentialFecundity;			//potential fecondity of this individu
		double m_overwinterLuck;				//for overwinter survival
		double m_adultAge;						//Adult age in days for oviposition
		double m_ʃT;							//accumulation of energy lost
		double m_Sh;							//hatch survival
		double m_hatchSurvival;
		double m_Tmin;							//minimum temperature experimented by insect
		double m_preDiapause;
		double m_preDiapauseRelativeDevRate;
		CTRef m_hatchTRef;

		static const double FREEZING_POINT;
		static const double PRE_OVIPOSITION;
	};


	class CHLTree : public CHost
	{
	public:

		CHLTree(CStand* pStand = NULL);

		virtual void GetStat(CTRef d, CModelStat& stat, size_t generation = NOT_INIT);

	};

	typedef std::shared_ptr<CHLTree> CHLTreePtr;
	//*******************************************************************************************************
	//*******************************************************************************************************
	// CHLStand
	class CHLStand : public CStand
	{
	public:

		//global variables of all bugs
		bool m_bApplyMortality;
		bool m_bFertilEgg;

		HemlockLooperEquations m_development;
		CHLOviposition m_oviposition;
		CHLSurvival m_survival;

		CHLStand(CBioSIMModelBase* pModel) :
			CStand(pModel),
			m_development(pModel->RandomGenerator()),
			m_oviposition(pModel->RandomGenerator()),
			m_survival(pModel->RandomGenerator())
		{
			m_bApplyMortality = true;
			m_bFertilEgg = false;
		}

	};

}