//*********************************************************************
// File: MPBiModel.h
//
// Class: CMPBiModel
//
// Description: CMPBiModel is a BioSIM model that computes 
//              mountain pine beetle seasonal biology
//
//*********************************************************************

#pragma once

#include "ModelBase/BioSIMModelBase.h"
#include "MountainPineBeetle.h"
namespace WBSF
{


	enum TDaily{ D_EGG, D_L1, D_L2, D_L3, D_L4, D_PUPA, D_TENERAL_ADULT, D_OVIPOSITING_ADULT, D_DEAD_ADULT, D_BROOD, D_NB_ATTACKS, D_NB_SUCESS_ATTACKS, D_INFESTED_KM², D_INFESTED_KM²_CUMUL, D_DEAD_BY_TREE, D_DEAD_DENSITY_DEPENDENCE, D_DEAD_ATTRITION, D_EGG_DEAD_FROZEN, D_LARVAL_DEAD_FROZEN, D_PUPA_DEAD_FROZEN, D_TENERAL_DEAD_FROZEN, D_OVIPADULT_DEAD_FROZEN, D_BIVOLTIN, D_UNIVOLTIN, D_SEMIVOLTIN, D_TRIENVOLTIN, D_NB_OBJECTS, D_NB_PACK, D_DD_FACTOR, NB_DAILY_OUTPUT };
	enum TAnnual { A_ATTACK, A_SUCCESS_ATTACK, A_R, A_EGG, A_LARVAL, A_PUPA, A_TENERAL, A_OVIPADULT, A_EGG_CREATED, A_BROOD, A_FECONDITY, A_DEAD_FROZEN, A_DEAD_OLD_AGE, A_DEAD_BY_TREE, A_DEAD_DENSITY_DEPENDENCE, A_DEAD_ATTRITION, A_INFESTED_KM², A_INFESTED_KM²_CUMUL, A_EGG_DEAD_FROZEN, A_LARVAL_DEAD_FROZEN, A_PUPA_DEAD_FROZEN, A_TENERAL_DEAD_FROZEN, A_OVIPADULT_DEAD_FROZEN, A_BIVOLTIN, A_UNIVOLTIN, A_SEMIVOLTIN, A_TRIENVOLTIN, A_NB_OBJECT_ALIVE, A_NB_PACK, A_DD_FACTOR, A_LONGIVITY, A_YEAR, A_MONTH, A_DAY, A_YEAR_END, A_MONTH_END, A_DAY_END/*, A_MONTH_PEAK, A_DAY_PEAK*/, NB_ANNUAL_OUTPUT };
	typedef CModelStatVectorTemplate<NB_DAILY_OUTPUT> CDailyOutputVector;
	typedef CModelStatVectorTemplate<NB_ANNUAL_OUTPUT> CAnnualOutputVector;


	//**********************************************************
	class CMPBiModel : public CBioSIMModelBase
	{
	public:

		CMPBiModel();
		virtual ~CMPBiModel();


		virtual ERMsg OnExecuteDaily();
		virtual ERMsg OnExecuteAnnual();
		virtual ERMsg OnExecuteAtemporal();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);

		virtual void AddSAResult(const StringVector& header, const StringVector& data);
		virtual void GetFValueAnnual(CStatisticXY& stat);
		virtual void FinalizeStat(CStatisticXY& stat);

		static CBioSIMModelBase* CreateObject(){ return new CMPBiModel; }

	private:

		void DoSimulation(CMPBStatVector& stat, const CInitialPopulation& attacks, size_t y1 = NOT_INIT, size_t y2 = NOT_INIT);
		void DoSimulation(CMPBStatMatrix& stat, const CInitialPopulation& attacks, size_t y1 = NOT_INIT, size_t y2 = NOT_INIT, bool bAllGeneration = false);
		CTRef GetInitialPeakAttack(int nbRunMax, int y = 0);
		//void DoSimulation(const CMountainPineBeetleVector& initialAttack, CMPBStatVector& stat, bool resetStandEachYear);


		void ComputeRegularValue(CMPBStatVector& stat, CDailyOutputVector& output);
		//void ComputeCumulativeValue(CMPBStatVector& stat, CDailyOutputVector& output);
		void ComputeRegularValue(CMPBStatVector& stat, CAnnualOutputVector& output);
		void ComputeGenerationValue(CMPBStatVector& stat, CModelStat& output);
		//std::vector<CNormalAttack> GetAttacks(const CMPBStatVector& stat);

		//the bugs
		CInitialPopulation m_attacks;
		CTRef m_peakDate;
		double m_attackStDev;
		int m_n0;

		bool m_bFertilEgg;
		int m_nbFertilGeneration;
		bool m_bAutoBalanceObject;
		double m_survivalRate;
		bool m_applyColdTolerance;
		int m_applyColdToleranceOvipAdult;
		bool m_bMicroClimate;
		double m_emergenceThreshold;
		bool m_bSnowProtection;
		bool m_bUseDefenselessTree;
		int m_OptimizeOn;

		//the tree
		double m_A0;
		double m_Amax;



		//the stand
		double m_forestSize;		//km²
		double m_forestDensity;		//tree/km²
		double m_initialInfestation;//km²
		double m_totalInitialAttack;

		double m_DDAlpha;
		double m_heightMax;  // height max in cm
		double m_pupeTeneralColdT; // cold min egg in °C

		int m_nbObjects;
		int m_minObjects;
		int m_maxObjects;


		static CTRef GetInitialPeakAttack(const CMPBStatVector& stat);
	};

}