//*****************************************************************************
// Class: CSpruceBudworm, CSBWTree, CSBWStand
//
// Description:	CSpruceBudworm represent a spruce budworm insect or a group of insect with same carractéristics. 
//				CSBWTree represent the tree that contain CSpruceBudworm. 
//				CSBWStand represent the tree that contain CSWBTree. 
//*****************************************************************************

#pragma once

#include "Basic/UtilTime.h"
#include "ModelBase/IndividueBase.h"
#include "SpruceBudwormEquations.h"

namespace WBSF
{

	class CModelStat;
	class CWeatherDay;

	namespace SBW
	{
		enum TSpruceBudwormStats
		{
			S_L2o, S_L2, S_L3, S_L4, S_L5, S_L6, S_PUPA, S_ADULT, S_DEAD_ADULT, S_OVIPOSITING_ADULT, S_BROOD, S_EGG, S_L1,
			S_L2o2, S_L22, S_AVERAGE_INSTAR, S_MALE_FLIGHT_ACTIVITY, S_FEMALE_FLIGHT_ACTIVITY, NB_STATS
		};
	}



	class CSBWTree;
	class CSBWStand;
	class CSpruceBudworm : public CIndividue
	{
	public:

		CSpruceBudworm(WBSF::CHost* pHost, CTRef creationDate = CTRef(), double age = SBW::EGG, size_t sex = NOT_INIT, bool bFertil = true, size_t generation = 0, double scaleFactor = 1);
		CSpruceBudworm(const CSpruceBudworm& in) :WBSF::CIndividue(in){ operator=(in); }
		CSpruceBudworm& operator=(const CSpruceBudworm& in);
		~CSpruceBudworm(void);

		virtual void Live(const CWeatherDay& weather);
		virtual void Brood(const CWeatherDay& weather);
		virtual void Die(const CWeatherDay& weather);
		virtual void GetStat(CTRef d, CModelStat& stat);
		virtual double GetInstar(bool includeLast)const	{ return (IsAlive() || m_death == OLD_AGE) ? std::min(m_age < SBW::L2o ? m_age : std::max(double(SBW::L2o), m_age - 1), double(SBW::NB_STAGES) - (includeLast ? 0.0 : 1.0)) : WBSF::CBioSIMModelBase::VMISS; }
		virtual void FixAI(double delta){ m_age = std::min(double(SBW::ADULT), std::max(double(SBW::L2o), m_age + delta)); }
		virtual void Pack(const WBSF::CIndividuePtr& in);
		virtual size_t GetNbStages()const{ return SBW::NB_STAGES; }
		virtual WBSF::CIndividuePtr CreateCopy()const{ return std::make_shared<CSpruceBudworm>(*this); }
		virtual bool NeedOverheating()const{ return !(GetStage() == SBW::L2o || GetStage() == SBW::ADULT); }

		double GetRelativeDevRate(size_t s)const { _ASSERTE(s >= 0 && s < SBW::NB_STAGES); return m_relativeDevRate[s]; } //Reports individual's relative development rate in "stage" 
		void ResetRelativeDevRate();

		inline CSBWTree* GetTree();
		inline const CSBWTree* GetTree()const;
		inline CSBWStand* GetStand();
		inline const CSBWStand* GetStand()const;
		inline CSpruceBudwormEquations& Equations();

	protected:

		static double GetAttritionRate(size_t s, double Tin);
		double GetRelativeDevRate(double T, double r)const;

		bool IsDeadByAttrition(double RR)const;
		bool IsDeadByMissingEnergy();
		double GetEatenFoliage(double RR)const;
		double GetFlightActivity(const CWeatherDay& weather);

		//member
		double m_relativeDevRate[SBW::NB_STAGES]; //Individual's relative development rates in 9 stages

		CTRef m_overwinteringDate;			//When individual pass from Egg to OW, they must stop develop until next spring
		CTRef m_emergingDate;				//When individual pass from Egg to OW, they must stop develop until next spring
		double m_eatenFoliage;
		double m_flightActivity;
		double m_OWEnergy;					//survival of overwintering
		bool m_bMissingEnergyAlreadyApplied;
		bool m_bKillByAttrition;

		static const double WHITE_SPRUCE_FACTOR[SBW::NB_STAGES];
		static const double SURVIVAL_RATE[SBW::NB_STAGES];
		static double GetEnergyLost(double T);
	};

	class CSBWTree : public WBSF::CHost
	{
	public:

		enum TKind{ BALSAM_FIR, WHITE_SPRUCE, NB_KIND };

		//public members
		size_t m_kind;//kind of tree

		CSBWTree(WBSF::CStand* pStand);

		virtual void Live(const CWeatherDay& weaDay);
		virtual void GetStat(CTRef d, CModelStat& stat, size_t generation = NOT_INIT);

		void CleanUp();
		bool GetHatchDate(CTRef& ref, double& d)const;


		//Defoliation
		//	double m_budDensity;//number of bud by branche
		//	double GetDefoliation()const{ return 1-m_foliageRatio; }
		//	void Eated(double feding){ m_bugsFeeding += feding;}
		//	void UpdateDefoliation();

		//	static double GetHU(CDailyWaveVector& T);
		//	int m_initialNumInd;
		//	double m_bugsFeeding;
		//	double m_foliageRatio;
		//	double m_hu;
	};

	typedef std::shared_ptr<CSBWTree> CSBWTreePtr;

	//*******************************************************************************************************
	//*******************************************************************************************************
	// CSBWStand
	class CSBWStand : public WBSF::CStand
	{
	public:

		//global variables of all bugs
		bool m_bApplyAttrition;
		bool m_bFertilEgg;
		bool m_bStopL22; //stop to L22 stage to get accumulation

		CSBWStand(WBSF::CBioSIMModelBase* pModel) :
			WBSF::CStand(pModel),
			m_equations(pModel->RandomGenerator())
		{
			m_bApplyAttrition = true;
			m_bFertilEgg = false;
		}

		CSpruceBudwormEquations m_equations;

	};

	typedef std::shared_ptr<CSBWStand> CSBWStandPtr;

	//WARNING: cast must be defined here to avoid bug
	inline CSBWTree* CSpruceBudworm::GetTree(){ return static_cast<CSBWTree*>(m_pHost); }
	inline const CSBWTree* CSpruceBudworm::GetTree()const{ return static_cast<const CSBWTree*>(m_pHost); }
	inline CSBWStand* CSpruceBudworm::GetStand(){ ASSERT(m_pHost); return static_cast<CSBWStand*>(GetTree()->GetStand()); }
	inline const CSBWStand* CSpruceBudworm::GetStand()const{ ASSERT(m_pHost); return static_cast<const CSBWStand*>(GetTree()->GetStand()); }
	inline CSpruceBudwormEquations& CSpruceBudworm::Equations(){ return GetStand()->m_equations; }


}