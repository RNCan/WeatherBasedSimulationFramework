//*****************************************************************************
// File: WSB.h
//
// Class: CSpruceBarkBeetle, CSpruceBarkBeetleVector
//          
//
// Descrition: CSpruceBarkBeetle represent one WSB insect. 
//			   CSpruceBarkBeetleVector is a vector of WSB insect. 
//*****************************************************************************

#pragma once

#include <crtdbg.h>
#include <vector>
#include <deque>
#include "basic/UtilTime.h"
#include "basic/ModelStat.h"
#include "modelBase/IndividueBase.h"
#include "SBBEquations.h"

namespace WBSF
{

	enum TTemperature{ T_AIR, T_BARK_TOP_OPEN, T_BARK_BOTTOM_OPEN, T_BARK_CLOSE, NB_TEMPERATURES };

	class CWeatherDay;
	class CDailyWaveVector;
	class CSpruceBarkBeetleTree;
	class CSpruceBarkBeetleStand;


	enum TWSBStat{
		S_ADULT_0, S_DEAD_ADULT_0, S_EMERGENCE_0, S_SWARMING_0_P1, S_SWARMING_0_P2, S_SWARMING_0_P3, S_TOTAL_FEMALE_0, S_BROOD_0_P1, S_BROOD_0_P2, S_BROOD_0_P3, S_TOTAL_BROOD_0, S_DIAPAUSE_0,
		S_EGG_1, S_L1_1, S_L2_1, S_L3_1, S_PUPAE_1, S_TENERAL_ADULT_1, S_ADULT_1, S_DEAD_ADULT_1, S_SWARMING_1_F1_i, S_SWARMING_1_F1_ii, S_SWARMING_1_F1_iii, S_SWARMING_1_F2_i, S_SWARMING_1_F2_ii, S_SWARMING_1_F2_iii, S_SWARMING_1_F3_i, S_SWARMING_1_F3_ii, S_SWARMING_1_F3_iii, S_TOTAL_FEMALE_1, S_BROOD_1, S_TOTAL_BROOD_1, S_DIAPAUSE_1,
		S_EGG_2, S_L1_2, S_L2_2, S_L3_2, S_PUPAE2, S_TENERAL_ADULT_2, S_ADULT_2,//, S_DEAD_ADULT_2, S_SWARMING_2_F1, S_SWARMING_2_F2, S_SWARMING_2_F3, S_TOTAL_FEMALE_2, S_BROOD_2, S_TOTAL_BROOD_2, S_DIAPAUSE_2, 
		//S_EGG_3, S_L1_3, S_L2_3, S_L3_3, S_PUPAE3, S_TENERAL_ADULT_3, S_ADULT_3, 
		S_DEAD_ATTRITION, S_DEAD_FROZEN, S_DEAD,
		S_W_STAT, S_DT_STAT, S_DDL_STAT, S_DAY_LENGTH, S_DI50, S_T_MEAN, S_TI50,

		S_SWARMING_0_P2_MALE, S_SWARMING_0_P2_FEMALE,
		//E_ADULT0, E_DEAD_ADULT0, E_OVIPOSITING_ADULT0, E_BROOD0, E_TOTAL_BROOD0, E_TOTAL_FEMALE0, 
		//E_EGG1, E_LARVAE1, E_PUPAE1, E_ADULT1, E_DEAD_ADULT1, E_OVIPOSITING_ADULT1, E_BROOD1, E_TOTAL_BROOD1, E_TOTAL_FEMALE1, 
		//E_EGG2, E_LARVAE2, E_PUPAE2, E_ADULT2, E_DEAD_ADULT2, 
		NB_STAT
	};


	typedef CModelStatVectorTemplate<NB_STAT> CSpruceBarkBeetleStatVector;

	class CSpruceBarkBeetle : public CIndividue
	{
	public:
		CSpruceBarkBeetle(CHost* pHost = NULL, CTRef creationDay = CTRef(), double age = ADULT, size_t sex=NOT_INIT, bool bFertil = true, int generation = 0, double scaleFactor = 1);
		CSpruceBarkBeetle(const CSpruceBarkBeetle& in);
		~CSpruceBarkBeetle(void);
		CSpruceBarkBeetle& operator=(const CSpruceBarkBeetle& in);



		virtual void Live(const CWeatherDay& wDay);
		virtual void Brood(const CWeatherDay& wDay);
		virtual void Die(const CWeatherDay& wDay);
		virtual void GetStat(CTRef d, CModelStat& stat);
		virtual size_t GetNbStages(void) const{ return NB_STAGES; }
		virtual CIndividuePtr CreateCopy()const{ return std::make_shared<CSpruceBarkBeetle>(*this); }
		virtual bool CanPack(const CIndividuePtr in)const;
		virtual void Pack(const CIndividuePtr in);

		double GetRelativeDevRate(int s)const { _ASSERTE(s >= 0 && s < NB_STAGES); return m_relativeDevRate[s]; } //Reports individual's relative development rate in "stage" 
		int GetTemperatureType(CTRef date)const{ return GetStage() == ADULT ? T_AIR : m_temperatureType; }

		int m_nbEmergence;
		int m_curEmergence;

	protected:

		CSpruceBarkBeetleTree* GetTree();
		const CSpruceBarkBeetleTree* GetTree()const;
		CSpruceBarkBeetleStand* GetStand();
		const CSpruceBarkBeetleStand* GetStand()const;

		void AssignRelativeDevRate(); //Sets indivivual's relative development rates in all stages, at creation

		void Develop(CTRef date, double T, const CWeatherDay& wDay, short nbStep);
		bool IsDeadByAttrition(double T, double r)const;
		bool IsDeadByOverwintering(double T, double dt);
		//bool IsDeadByAsynchrony(double T, double dt);
		//bool IsDeadByWindow(double T, double dt);
		bool ChangeStage(double RR){ return short(m_age + RR) != GetStage(); }
		void ComputetDiapause(int s, double T, double DL);
		void ComputeSwarming(int s, double T, double DL);

		//member
		std::array<double, NB_STAGES> m_relativeDevRate; //Individual's relative development rates in 9 stages
		double m_relativeOviposition;
		double m_relativeFecondity;
		//double m_relativeWingsMaturation;
		double m_springSwarmingT;
		std::array<double, 2> m_reemergenceDDRequired;
		std::array<double, 2> m_reemergenceT;
		std::array<double, 2> m_femaleIndex;
		double m_automnP;
		double m_automnF;
		//double m_reemergenceT;




		int m_parentEmergence;
		double m_broodIndex;
		//double m_diapauseDayLength;
		double m_emergenceDD;
		double m_emergenceTmax;


		double m_lastDayLength;
		int m_temperatureType;

		CTRef m_swarmingDate;
		CTRef m_diapauseDate;	//When individual pass from Pupae to Teneral Adult, they can stop development until next spring
		CTRef m_emergenceDate;
		CTRef m_ecdysisDate;
		CTRef m_stageDate;


		double m_reemergenceDD;
		bool m_bFlight;
		double m_flightDD;
		double m_ovipositionAge;

		double m_hibernationDD;
		double m_hibernationDDrequired;


		CStatistic m_TStat;
		CStatistic m_TI50Stat;
		CStatistic m_dayLength;
		CStatistic m_DI50Stat;

		CStatistic m_absDiapauseStatI;
		CStatistic m_absDiapauseStatII;
		CStatistic m_wStat;
		CStatistic m_TAFStat;
		CStatistic m_RStat;
		CStatistic m_dTStat;
		CStatistic m_dDLStat;
		CStatistic m_awakeStat;
	};



	
	class CSpruceBarkBeetleTree : public CHost
	{
	public:

		CSpruceBarkBeetleTree(CStand* pStand = NULL);
		void Reset();

		virtual void HappyNewYear();
		virtual void Live(const CTRef& day, const CWeatherDay& weaDay);
		virtual void GetStat(CTRef d, CModelStat& stat, int generation = -1);
		double GetLast4Days()const;

	protected:
		
		std::deque<CWeatherDay> m_last4Days;
	};


	//*******************************************************************************************************
	//*******************************************************************************************************
	// CSpruceBarkBeetleStand
	class CSpruceBarkBeetleStand : public CStand
	{
	public:

		//global variables of all bugs
		bool m_bApplyAttrition;
		bool m_bApplyWinterMortality;
		bool m_bFertilEgg;
		double m_survivalRate;

		//calibration variable
		double m_longDayLength;
		double m_firstStageShortDay;
		double m_lastStageShortDay;
		int m_nbShortDay;
		double m_shortDayLength;

		//weibull function of the diapausing insects
		std::array<double, 5> m_p;
		std::array<double, 5> m_k;
		std::array<double, NB_STAGES> m_s;
		int m_nbReemergeMax;

		std::map<int, CTRef> m_lastSnow;
		std::map<int, CTRef> m_firstSnow;
		double m_DD5;

		CSpruceBarkBeetleStand(CBioSIMModelBase* pModel);


		CSpruceBarkBeetleTree* GetTree(){ return m_pTree.get(); }
		const CSpruceBarkBeetleTree* GetTree()const{ return m_pTree.get(); }
		void SetTree(CSpruceBarkBeetleTree* pTree);

		CSBBTableLookup m_equations;

		virtual CHost* GetHeadHost(){ return GetTree(); }
		virtual const CHost* GetHeadHost()const{ return GetTree(); }
		virtual void GetStat(CTRef d, CModelStat& stat, int generation = -1);

		double GetP2AutomnProbability(int sex);

	protected:

		std::array<double, 2> m_P2Prob;
		std::auto_ptr<CSpruceBarkBeetleTree> m_pTree;
	};

}