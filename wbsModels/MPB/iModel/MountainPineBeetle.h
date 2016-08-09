#pragma once

#include "../MPBDevRates.h"
#include "../MPBColdTolerance.h"
#include "Basic/UtilTime.h"
#include "ModelBase\IndividualBase.h"



namespace WBSF
{


	enum TStage{
		S_EGG, S_L1, S_L2, S_L3, S_L4, S_PUPA, S_TENERAL_ADULT, S_OVIPOSITING_ADULT, S_DEAD_ADULT, S_BROOD,
		E_EGG, E_L1, E_L2, E_L3, E_L4, E_PUPA, E_TENERAL_ADULT, E_OVIPOSITING_ADULT, E_DEAD_ADULT, E_BROOD, E_TOTAL_BROOD, E_TOTAL_FEMALE, E_LONGIVITY,
		E_DEAD_FROZEN, E_DEAD_OLD_AGE, E_DEAD_ATTRITION, E_DEAD_BY_TREE, E_DEAD_DENSITY_DEPENDENCE, E_DEAD,
		S_DEAD_FROZEN, S_DEAD_OLD_AGE, S_DEAD_ATTRITION, S_DEAD_BY_TREE, S_DEAD_DENSITY_DEPENDENCE, S_DEAD,
		DF_EGG, DF_L1, DF_L2, DF_L3, DF_L4, DF_PUPA, DF_TENERAL_ADULT, DF_OVIPOSITING_ADULT,
		S_BIVOLTIN, S_UNIVOLTIN, S_SEMIVOLTIN, S_TRIENVOLTIN, S_NB_OBJECT, S_NB_OBJECT_ALIVE,

		//*************************************
		//Tree statistics
		E_NB_ATTACKS, E_NB_SUCESS_ATTACKS, E_NB_INFESTED_TREE,
		S_NB_PACK,
		//stand statistics
		S_DD_FACTOR,
		NB_STAT
	};

	typedef CModelStatVectorTemplate<NB_STAT> CMPBStatVector;
	typedef std::vector<CMPBStatVector> CMPBStatMatrix;

	class CWeatherDay;
	class CMPBTree;
	class CMPBStand;


	class CMountainPineBeetle : public CIndividual
	{
	public:

		enum TOvipAdultColdOption { OA_KILL_NONE, OA_KILL_COLD, OA_KILL_DECEMBER_31, OA_KILL_M18, NB_OA_OPTION };

		CMountainPineBeetle(CHost* pHost, CTRef creationDate = CTRef(), double age = EGG, size_t sex = NOT_INIT, bool bFertil = true, size_t generation = 0, double scaleFactor = 1);
		CMountainPineBeetle(const CMountainPineBeetle& in);
		~CMountainPineBeetle(void);

		void AssignUnicity();
		CMountainPineBeetle& operator=(const CMountainPineBeetle& in);

		virtual void Live(const CWeatherDay& weaDay);
		virtual void Brood(const CWeatherDay& weaDay);
		virtual void Die(const CWeatherDay& weaDay);
		virtual size_t GetNbStages()const{ return NB_STAGES; }
		virtual void GetStat(CTRef d, CModelStat& stat);
		virtual CIndividualPtr CreateCopy()const
		{
			//CMountainPineBeetle* pNewBug = new CMountainPineBeetle(m_creationDate, m_age, m_bFertil, m_generation, m_scaleFactor, m_pHost);
			CMountainPineBeetle* pNewBug = new CMountainPineBeetle(*this);
			pNewBug->AssignUnicity();
			return CIndividualPtr(pNewBug);
		}

		virtual bool CanPack(const CIndividualPtr in)const
		{
			return CIndividual::CanPack(in) && GetStage() != OVIPOSITING_ADULT;
		}

		virtual bool CanUnpack()const
		{
			return CIndividual::CanUnpack() && GetStage() != OVIPOSITING_ADULT;
		}


		double GetRelativeDevRate(int s)const { _ASSERTE(s >= 0 && s < NB_STAGES); return m_relativeDevRate[s]; } //Reports individual's relative development rate in "stage" 
		bool IsEmerging(){ return m_bEmerging; }
		void CompleteEmergence(size_t death);

		bool GetSuccessAttackToday()const{ return m_bSuccessAttackToday; }

	private:

		void AssignRelativeDevRate(); //Sets indivivual's relative development rates in each stage, at creation
		void AssignMortality(); //Sets indivivual's relative cold tolerance (same in all stages), at creation

		void Develop(CTRef date, double T);

		double ComputeBrood(double Ai_1, double Ai);
		bool IsDeadByAttrition();
		void PreEmerging();
		void Emerging();
		CMPBTree* GetTree();
		const CMPBTree* GetTree()const;
		CMPBStand* GetStand();
		const CMPBStand* GetStand()const;

		double m_relativeDevRate[NB_STAGES]; //Individual's relative development rates in 9 stages



		bool m_bEmerging;
		bool m_bAttackToday;
		bool m_bSuccessAttackToday;
		CStatistic m_sumLarvalColdMortality;

		//Cold tolerance variables and methods
		double m_PCritical;
		double m_PCriticalP1;
		double m_PCriticalAttrition;
		double m_height;
		bool IsDeadByFrost(CTRef day, const double& T, double snowDepth);

		static const double SEX_RATIO;
		static const double EMERGING_THRESHOLD;
	};

	//*******************************************************************************************************
	//*******************************************************************************************************
	// CMPBTree
	//typedef CHostTemplate<CMountainPineBeetle> CMPBTreeBase;
	//typedef CMPBTreeBase::CTBugVector CMountainPineBeetleVector;
	class CMPBTree : public CHost
	{
	public:

		double m_n0;//females/m�
		double m_A0;//tree resistance
		double m_Amax; //maximum attack/day before adding an additional tree



		CMPBTree(CStand* pStand = NULL) : CHost(pStand)
		{
			m_n0 = 60;
			m_A0 = 30;
			m_Amax = 120;

			m_nbSucessfullAttacksToday = 0;
			m_emergingToday = 0;
		}


		//void Initialise(long numInd, CTRef peakAttackDay, double sigma, double age=0, bool bFertil=true, int generation=0);
		//void UpdateScaleFactor(double nbTreeInfested);

		virtual void Live(const CWeatherDay& weaDay);
		//virtual void Live(CDailyWaveVector& hVector, const CWeatherDay& weaDay);

		//virtual void GetStat(CTRef d, CModelStat& stat, int generation = -1);
		//virtual void GetStat(CTRef d, CModelStat& stat);

		void EmergingBugs(double nbBugs);
		bool AttackedByBug(CMountainPineBeetle* pBug);

		//	double GetA0()const{ return m_A0; }
		//void SetA0(double A0){ m_A0 = A0; }
		//double GetAmax()const{ return m_Amax;} 
		//void SetAmax(double Amax){ m_Amax = Amax;}
		//double GetTreeSize()const{ return m_treeSize;}
		//void SetTreeSize(double treeSize){ m_treeSize = treeSize;}


		virtual void HappyNewYear()
		{
			CHost::HappyNewYear();
		}

		double GetNbInfestedTreesToday()const{ return GetNbInfestedTrees(m_nbSucessfullAttacksToday); }
		double GetNbInfestedTrees(double nbSucessfullAttacksToday)const{ return nbSucessfullAttacksToday / m_Amax; }
		double GetNbInitialAttack(double nbTree)const
		{
			return GetNbInitialAttack(nbTree, m_Amax, m_n0);
		}

		static double GetNbInitialAttack(double nbTree, double Amax, double n0)
		{
			ASSERT(nbTree >= 1);
			return (nbTree - 1)*Amax + n0;
		}

		void CompleteEmergence();
		CMPBStand* GetStand();

		CIndividualPtr FindObject(size_t g, size_t s, size_t, size_t, size_t);

	protected:

		double m_emergingToday;
		double m_nbSucessfullAttacksToday;
	};

	//*******************************************************************************************************
	//*******************************************************************************************************
	// CMPBStand
	class CMPBStand : public CStand
	{
	public:

		//global variables of all bugs
		bool m_bArrestDevel;
		bool m_applyColdTolerance;
		int m_applyColdToleranceOvipAdult;
		bool m_bFertilEgg;
		int m_nbFertilGeneration;
		double m_survivalRate;
		double m_emergenceThreshold;
		bool m_bMicroClimate;
		bool m_bSnowProtection;
		bool m_bUseDefenselessTree;
		double m_initialInfestation;	// km�
		double m_forestDensity;			// trees/km�
		double m_forestSize;			// km� 
		double m_DDAlpha;
		double m_heightMax;  // height max in cm
		double m_pupeTeneralColdT; // cold min egg in 캜

		CMPBStand(CBioSIMModelBase* pModel) : 
			CStand(pModel),
			m_rates(pModel->RandomGenerator())
		{
			m_bArrestDevel = false;
			m_applyColdTolerance = true;
			m_applyColdToleranceOvipAdult = true;
			m_bFertilEgg = false;
			m_nbFertilGeneration = 0;
			m_survivalRate = 0.1;
			m_emergenceThreshold = 18;
			m_bMicroClimate = true;
			m_bSnowProtection = true;
			m_bUseDefenselessTree = true;
			m_initialInfestation = 0;
			m_forestDensity = 100000;
			m_forestSize = 1850;
			m_nbInfestedTrees = 0;
			m_DDAlpha = 1;
			m_heightMax = 500;  // height max in cm
			m_pupeTeneralColdT = -28; // cold min egg in 캜

			m_bInit = false;
		}

		CMPBTree* GetTree(){ return (CMPBTree*)m_pTree.get(); }
		const CMPBTree* GetTree()const{ return (CMPBTree*)m_pTree.get(); }
		void SetTree(CMPBTree* pTree);


		void Init(const CWeatherStation& weather);

		double GetNbTrees()const { return m_forestSize*m_forestDensity; }
		double GetNbInitialInfestedTrees()const { return m_initialInfestation*m_forestDensity; }
		double GetNbInfestedTrees()const { return m_nbInfestedTrees; }

		double GetDDFactor()const;
		double GetDDFactorTest(double nbPotentialAttacksToday)const;

		double GetNbInitialAttack()const{ return GetTree()->GetNbInitialAttack(GetNbInitialInfestedTrees()); }

		virtual void Live(const CWeatherDay& weaDay)
		{
			ASSERT(m_pTree.get());
			if (!m_bInit)
			{
				m_nbInfestedTrees = GetNbInitialInfestedTrees();
				//m_pTree->UpdateScaleFactor(m_nbInfestedTrees);
				m_bInit = true;
			}

			m_pTree->Live(weaDay);

			m_nbInfestedTrees += GetTree()->GetNbInfestedTreesToday();
		}

		virtual void GetStat(CTRef d, CModelStat& stat, size_t generation = NOT_INIT)
		{
			m_pTree->GetStat(d, stat, generation);
		}

		virtual CHostPtr GetNearestHost(CHost* pHost){ return m_pTree; }

		const CMPBColdTolerance& GetColdTolerance()const{ ASSERT(m_coldTolerance.GetResult().size() > 0); return m_coldTolerance; }


		void AdjustPopulation()
		{
			m_pTree->AdjustPopulation();
		}

		size_t GetNbObjectAlive()const
		{
			return m_pTree->GetNbObjectAlive();
		}


		virtual void HappyNewYear()
		{
			m_pTree->HappyNewYear();
		}


		CMPBDevelopmentTable m_rates;


	protected:

		CHostPtr m_pTree;

		CMPBColdTolerance m_coldTolerance;

		double m_nbInfestedTrees;

		bool m_bInit;
	};

}