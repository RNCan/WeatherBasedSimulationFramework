//*****************************************************************************
// File: WSB.h
//
// Class: CSpruceBarkBeetleOhrn, CSpruceBarkBeetleOhrnVector
//          
//
// Descrition: CSpruceBarkBeetleOhrn represent one WSB insect. 
//			   CSpruceBarkBeetleOhrnVector is a vector of WSB insect. 
//*****************************************************************************

#pragma once

#include <crtdbg.h>
#include <vector>
#include "UtilTime.h"
#include "ModelStat.h"
#include "BugVector.h"
#include "SBBDevelopment.h"


class CWeatherDay;
class CDailyWaveVector;
class CSpruceBarkBeetleOhrnTree;
class CSpruceBarkBeetleOhrnStand;


enum TWSBStat{	S_ADULT_0, S_DEAD_ADULT_0, S_SWARMING_0_P1, S_SWARMING_0_P2, S_SWARMING_0_P3, S_TOTAL_FEMALE_0, S_BROOD_0, S_TOTAL_BROOD_0, S_DIAPAUSE_0,
				S_EGG_1, S_L1_1, S_L2_1, S_L3_1, S_PUPAE_1, S_TENERAL_ADULT_1, S_ADULT_1, S_DEAD_ADULT_1, S_SWARMING_1_F1_i, S_SWARMING_1_F1_ii, S_SWARMING_1_F1_iii, S_SWARMING_1_F2_i, S_SWARMING_1_F2_ii, S_SWARMING_1_F2_iii, S_SWARMING_1_F3_i, S_SWARMING_1_F3_ii, S_SWARMING_1_F3_iii, S_TOTAL_FEMALE_1, S_BROOD_1, S_TOTAL_BROOD_1, S_DIAPAUSE_1,
				S_EGG_2, S_L1_2, S_L2_2, S_L3_2, S_PUPAE2, S_TENERAL_ADULT_2, S_ADULT_2,//, S_DEAD_ADULT_2, S_SWARMING_2_F1, S_SWARMING_2_F2, S_SWARMING_2_F3, S_TOTAL_FEMALE_2, S_BROOD_2, S_TOTAL_BROOD_2, S_DIAPAUSE_2, 
				S_DEAD_ATTRITION, S_DEAD_FROZEN, S_DEAD,
				NB_STAT };


typedef CModelStatVectorTemplate<NB_STAT> CSpruceBarkBeetleOhrnStatVector;

class CSpruceBarkBeetleOhrn : public CBug
{
public:
	CSpruceBarkBeetleOhrn(CHost* pHost=NULL, CTRef creationDay=CTRef(), double age=ADULT, bool bFertil=true, int generation=0, double scaleFactor=1);
	~CSpruceBarkBeetleOhrn(void);

	virtual void Live(const CDailyWaveVector& T, const CWeatherDay& wDay);
	virtual void Brood(const CDailyWaveVector& T, const CWeatherDay& wDay);
	virtual void GetStat(CTRef d, CModelStat& stat);
	virtual CBug* CreateCopy()const{return new CSpruceBarkBeetleOhrn(*this); }
	virtual bool CanPack(const CBug* in)const;

	double GetRelativeDevRate(int s)const {_ASSERTE(s>=0 && s<NB_STAGES); return m_relativeDevRate[s];} //Reports individual's relative development rate in "stage" 

private:

	CSpruceBarkBeetleOhrnTree* GetTree();
	const CSpruceBarkBeetleOhrnTree* GetTree()const;
	CSpruceBarkBeetleOhrnStand* GetStand();
	const CSpruceBarkBeetleOhrnStand* GetStand()const;

	void AssignRelativeDevRate(); //Sets indivivual's relative development rates in all stages, at creation

	void Develop(CTRef date, double T, const CWeatherDay& wDay, short nbStep);
	bool IsDeadByAttrition(double T, double r)const;
	bool IsDeadByOverwintering(double T, double dt);
	bool ChangeStage(double RR){ return short(m_age+RR) != GetStage();}
	void ComputetDiapause(int s, double T, double DL);
	void ComputetSwarming(int s, double T, double DL);

	//member
	double m_relativeDevRate[NB_STAGES]; //Individual's relative development rates in 9 stages
	double m_relativeOviposition;
	double m_relativeFecondity;
	double m_relativeWingsMaturation;
	double m_springSwarmingT;
	double m_reemergenceDDRequerd[2];
	double m_reemergenceT;
	double m_emergenceT;
	bool m_bGroundOverwinter;
	CTRef m_lastSnow;

	//CTRef m_swarmingPredictedDate;
	CTRef m_swarmingDate;
	CTRef m_diapauseDate;	//When individual pass from Pupae to Teneral Adult, they can stop development until next spring
	CTRef m_awakeDate;	
	CTRef m_ecdysisDate;
	//CTRef m_rebeginEating;

	int m_nbEmergence;		
	int m_curEmergence;
	int m_parentEmergence;
	double m_reemergenceDD;
	double m_flightDD;
	double m_ovipositionAge;
	
	double m_hibernationDD;
	double m_hibernationDDrequired;
	double m_lastDayLength;

	
	CFL::CStatistic m_TStat;
	CFL::CStatistic m_TI50Stat;
	CFL::CStatistic m_dayLength;
	CFL::CStatistic m_DI50Stat;

	CFL::CStatistic m_absDiapauseStatI;
	CFL::CStatistic m_absDiapauseStatII;
	CFL::CStatistic m_wStat;
	CFL::CStatistic m_TAFStat;
	CFL::CStatistic m_RStat;
	CFL::CStatistic m_dTStat;
	CFL::CStatistic m_dDLStat;
	CFL::CStatistic m_awakeStat;
};



typedef CHostTemplate<CSpruceBarkBeetleOhrn> CSpruceBarkBeetleOhrnTree;
//class CSpruceBarkBeetleOhrnTree: public CSpruceBarkBeetleOhrnTreeBase
//{
//public:
//
//	CSpruceBarkBeetleOhrnTree(CStand* pStand=NULL);
//	void Reset();
//
//	virtual void HappyNewYear();
//	virtual void Live(const CTRef& day, const CWeatherDay& weaDay);
//	virtual void GetStat(CTRef d, CModelStat& stat, int generation=-1);
//	
//protected:
//
//};

//typedef CSpruceBarkBeetleOhrnTree::CTBugVector CSpruceBarkBeetleOhrnVector;


//*******************************************************************************************************
//*******************************************************************************************************
// CSpruceBarkBeetleOhrnStand
//typedef CStand CSpruceBarkBeetleOhrnStand;
class CSpruceBarkBeetleOhrnStand: public CStand
{
public:


	CSpruceBarkBeetleOhrnStand(CBioSIMModelBase* pModel):CStand(pModel)
	{
	}

	CSpruceBarkBeetleOhrnTree* GetTree(){ return m_pTree.get(); }
	const CSpruceBarkBeetleOhrnTree* GetTree()const{return m_pTree.get(); }
	void SetTree(CSpruceBarkBeetleOhrnTree* pTree);

	virtual CHost* GetHeadHost(){return GetTree();}
	virtual const CHost* GetHeadHost()const{return GetTree();}
	virtual void GetStat(CTRef d, CModelStat& stat, int generation=-1);

protected:
	
	
	std::auto_ptr<CSpruceBarkBeetleOhrnTree> m_pTree;
};
