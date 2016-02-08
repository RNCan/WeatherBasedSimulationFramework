//*****************************************************************************
// File: WSBRates.h
//
// Class: CSpruceBarkBeetle
//          
//
// Description: the CSpruceBarkBeetle represent one western spruce budworm insect. 

//
//*****************************************************************************
// 25/09/2013   Rémi Saint-Amant    Creation 
//*****************************************************************************

#include "SpruceBarkBeetle.h"

#include "UtilMath.h"
#include "Weather.h"
#include <math.h>
#include "Evapotranspiration.h"

using namespace CFL;
using namespace std;
using namespace DAILY_DATA;


//Flight duration after swarming. After Öhrn 2014
static const double FLIGHT_DD_SUM = 33.4;
static const double FLIGHT_DD_THRESHOLD = 5.0; 


//Spring swarming maximum ari temperature normal distribution. After Faccoli 2009
static const double TMAX_THRESHOLD_MEAN = 19.7;
static const double TMAX_THRESHOLD_SD = 0.65;

static const double REEMERGENCE_T = 20.0;


//******************************************************************
//CSpruceBarkBeetleTree class

CSpruceBarkBeetleTree::CSpruceBarkBeetleTree(/*CBioSIMModelBase* pModel,*/ CStand* pStand):CSpruceBarkBeetleTreeBase(pStand)
{
	Reset();
}

void CSpruceBarkBeetleTree::Reset()
{
	CSpruceBarkBeetleTreeBase::Reset();

	//m_defoliation=0.5;
	//m_bEnergyLoss=true;
	//m_ddays=0;
	//m_probBudMineable=0;
	//m_ddShoot=0;
}

void CSpruceBarkBeetleTree::HappyNewYear()
{
	CSpruceBarkBeetleTreeBase::HappyNewYear();

	//m_ddays=0;
	//m_probBudMineable=0;
	//m_ddShoot=0;
}

class CTAir : public COverheat
{
public:
	
	CTAir(CSpruceBarkBeetleStand* pStand)
	{
		m_pStand=pStand;
	}
	CSpruceBarkBeetleStand* m_pStand;
	virtual void TransformWeather(CWeatherDay& weaDay)const
	{
		//Do nothing
	}


	
};


class CTOpenTopPhloem : public COverheat
{
public:
	
	CTOpenTopPhloem(CSpruceBarkBeetleStand* pStand)
	{
		m_pStand=pStand;
	}
	CSpruceBarkBeetleStand* m_pStand;

	virtual void TransformWeather(CWeatherDay& weaDay)const
	{
		ASSERT( weaDay[DAILY_DATA::TMIN] > -999 && weaDay[DAILY_DATA::TMAX] > -999);

		
		double Tmin   = weaDay.GetTMin();
		
		//double Tmax   = weaDay.GetTMean()*(1+weaDay[SRAD]/21)+4;
		double Tmax   = weaDay.GetTMax()*(1+weaDay[SRAD]/38)+2;
		double Trange = Tmax-Tmin;//weaDay.GetTRange();
		double Sin    = sin(2*3.14159*(weaDay.GetJDay()/365. -0.25));

		//convert air temperature to bark temperature
		weaDay(DAILY_DATA::TMIN)=-0.1493 + 0.8359*Tmin + 0.5417*Sin + 0.16980*weaDay.GetTRange() + 0.00000*Tmin*Sin + 0.005741*Tmin*weaDay.GetTRange() + 0.02370*Sin*weaDay.GetTRange();
		weaDay(DAILY_DATA::TMAX)= 0.4196 + 0.9372*Tmax - 0.4265*Sin + 0.05171*Trange + 0.03125*Tmax*Sin - 0.004270*Tmax*Trange + 0.09888*Sin*Trange;

		if( weaDay(DAILY_DATA::TMIN) > weaDay(DAILY_DATA::TMAX) )
			CFL::Switch( weaDay(DAILY_DATA::TMIN), weaDay(DAILY_DATA::TMAX) );
	}
};

class CTOpenBottomPhloem : public COverheat
{
public:
	CTOpenBottomPhloem(CSpruceBarkBeetleStand* pStand)
	{
		m_pStand=pStand;
	}
	CSpruceBarkBeetleStand* m_pStand;

	virtual void TransformWeather(CWeatherDay& weaDay)const
	{
		ASSERT( weaDay[DAILY_DATA::TMIN] > -999 && weaDay[DAILY_DATA::TMAX] > -999); 

		double Tmin   = weaDay.GetTMin();
		double Tmax   = weaDay.GetTMax()*(1+weaDay[SRAD]/139)+1;
		double Trange = Tmax-Tmin;
		double Sin    = sin(2*3.14159*(weaDay.GetJDay()/365. -0.25));

		//convert air temperature to bark temperature
		weaDay(DAILY_DATA::TMIN)=-0.1493 + 0.8359*Tmin + 0.5417*Sin + 0.16980*weaDay.GetTRange() + 0.00000*Tmin*Sin + 0.005741*Tmin*weaDay.GetTRange() + 0.02370*Sin*weaDay.GetTRange();
		weaDay(DAILY_DATA::TMAX)= 0.4196 + 0.9372*Tmax - 0.4265*Sin + 0.05171*Trange + 0.03125*Tmax*Sin - 0.004270*Tmax*Trange + 0.09888*Sin*Trange;

		if( weaDay(DAILY_DATA::TMIN) > weaDay(DAILY_DATA::TMAX) )
			CFL::Switch( weaDay(DAILY_DATA::TMIN), weaDay(DAILY_DATA::TMAX) );
	}
};

class CTClosePhloem : public COverheat
{
public:
	
	CTClosePhloem(CSpruceBarkBeetleStand* pStand)
	{
		m_pStand=pStand;
	}
	CSpruceBarkBeetleStand* m_pStand;

	virtual void TransformWeather(CWeatherDay& weaDay)const
	{
		ASSERT( weaDay[DAILY_DATA::TMIN] > -999 && weaDay[DAILY_DATA::TMAX] > -999);

		double Tmin   = weaDay.GetTMin()-1;
		double Tmax   = (3*weaDay.GetTMin()+weaDay.GetTMax())/4;

		double Trange = Tmax-Tmin;
		double Sin    = sin(2*3.14159*(weaDay.GetJDay()/365. -0.25));

		//convert air temperature to bark temperature
		weaDay(DAILY_DATA::TMIN)=-0.1493 + 0.8359*Tmin + 0.5417*Sin + 0.16980*Trange + 0.00000*Tmin*Sin + 0.005741*Tmin*Trange + 0.02370*Sin*Trange;
		weaDay(DAILY_DATA::TMAX)= 0.4196 + 0.9372*Tmax - 0.4265*Sin + 0.05171*Trange + 0.03125*Tmax*Sin - 0.004270*Tmax*Trange + 0.09888*Sin*Trange;

		if( weaDay(DAILY_DATA::TMIN) > weaDay(DAILY_DATA::TMAX) )
			CFL::Switch( weaDay(DAILY_DATA::TMIN), weaDay(DAILY_DATA::TMAX) );
	}
};

class CTSoil: public COverheat
{
public:
	
	CTSoil(CSpruceBarkBeetleStand* pStand)
	{
		m_pStand=pStand;
	}
	CSpruceBarkBeetleStand* m_pStand;

	virtual void TransformWeather(CWeatherDay& weaDay)const
	{
		ASSERT( weaDay[DAILY_DATA::TMIN] > -999 && weaDay[DAILY_DATA::TMAX] > -999);

		double Tmin   = weaDay.GetTMin()-1.1;
		double Tmax   = max(Tmin, weaDay.GetTMean()+1.0);
		double Trange = Tmax-Tmin;
		double Sin    = sin(2*3.14159*(weaDay.GetJDay()/365. -0.25));

		//convert air temperature to bark temperature
		weaDay(DAILY_DATA::TMIN)=-0.1493 + 0.8359*Tmin + 0.5417*Sin + 0.16980*Trange + 0.00000*Tmin*Sin + 0.005741*Tmin*Trange + 0.02370*Sin*Trange;
		weaDay(DAILY_DATA::TMAX)= 0.4196 + 0.9372*Tmax - 0.4265*Sin + 0.05171*Trange + 0.03125*Tmax*Sin - 0.004270*Tmax*Trange + 0.09888*Sin*Trange;

		if( weaDay(DAILY_DATA::TMIN) > weaDay(DAILY_DATA::TMAX) )
			CFL::Switch( weaDay(DAILY_DATA::TMIN), weaDay(DAILY_DATA::TMAX) );
	}
};
	
typedef auto_ptr<COverheat> COverheatPtr;
void CSpruceBarkBeetleTree::Live(const CTRef& day, const CWeatherDay& weaDay)
{
	//For optimisation, nothing happens when temperature is under -10
	if( day.GetJDay() != 0 && weaDay.GetTMax() < -10)
		return;

	m_last4Days.push_back(weaDay);
	if( m_last4Days.size()>4)
		m_last4Days.pop_front();


	CSpruceBarkBeetleStand* pStand = (CSpruceBarkBeetleStand*)GetStand();
	COverheatPtr overheat[NB_TEMPERATURES] = {COverheatPtr (new CTAir(pStand)), COverheatPtr (new CTOpenTopPhloem(pStand)), COverheatPtr(new CTOpenBottomPhloem(pStand)), COverheatPtr (new CTClosePhloem(pStand)) };
	//const COverheat bark[NB_POSITION] = {CTopOpenPhloem(pStand),CBottomOpenPhloem(pStand),CClosePhloem(pStand),CTSoil(pStand)};
	//CBarkPhloem bark;
	//CTopOpenPhloem topBark;
	//CBottomOpenPhloem bottomBark;
	//CClosePhloem closeBark;
	//double overheat=0;

	
	CDailyWaveVector T[NB_TEMPERATURES];// hourly temperature array 

	for(int i=0; i<NB_TEMPERATURES; i++)
		weaDay.GetAllenWave(T[i], 16, gTimeStep, *(overheat[i]) );//16 is a factor of 4. Number usually used
	//weaDay.GetAllenWave(T[0], 16, gTimeStep );//16 is a factor of 4. Number usually used

	if( m_initialSize==-1)
		m_initialSize=m_bugs.size();

	for(int i=(int)m_bugs.size()-1; i>=0; i--)
	{
		CSpruceBarkBeetle* pBug = (CSpruceBarkBeetle* )m_bugs[i];
		int pos= pBug->GetTemperatureType(day);
		m_bugs[i]->Live(T[pos], weaDay);
	}

	for(int i=(int)m_bugs.size()-1; i>=0; i--)
	{
		CSpruceBarkBeetle* pBug = (CSpruceBarkBeetle* )m_bugs[i];
		int pos=pBug->GetTemperatureType(day);
		
		m_bugs[i]->Brood(T[pos], weaDay);
	}

	//CSpruceBarkBeetleTreeBase::Live(hVector, weaDay);
}

double CSpruceBarkBeetleTree::GetLast4Days()const
{
	CStatistic last4Days;
	
	for(size_t i=0; i<m_last4Days.size(); i++)
		last4Days += m_last4Days[i].GetTMean();

	return last4Days[MEAN];
}

void CSpruceBarkBeetleTree::GetStat(CTRef d, CModelStat& stat, int generation)
{
	CSpruceBarkBeetleTreeBase::GetStat(d, stat);
	
//	CSpruceBarkBeetleStat& SBStat = (CSpruceBarkBeetleStat& )stat; 
	//stat[S_AVERAGE_INSTAR] = SBStat.GetAverageInstar();
//  This line should be restored to work on the L2 synchrony test runs
	//stat[S_P_MINEABLE] = m_ddays;//m_probBudMineable;
//  This line should be restored to work on the L6 window test runs
	//stat[S_SHOOT_DEVEL] = m_ddShoot;
	
}
/*
void CSpruceBarkBeetleTree::ComputeMineable(const CDailyWaveVector& T)
{
	//Bud development 
	//This is calculated for the first summer, then reset on December 31.
	//This is part of the tree's daily "Live" loop, starting on m_startDate

	//Constants and parameters 
	static const int    START_DATE = 92-1; //First day (0- based) for degree day summation for bud develpment
	static const double BASE_TEMP  = 8.2 ; //dd summation of bud development
	static const double MAX_TEMP   = 14.4; //dd summation of bud development
	static const double a          = 95.9; //dd for 50% of buds mineable
	static const double b          = 2.123;//dd variance around the 50% dd sum

	if(T.GetFirstTRef().GetJDay() >= START_DATE)
	{
		// Loop over every time step in one day
		for (int h=0; h<(int)T.size(); h++)
		{
			//Linear DDays with upper threshold
			m_ddays += Max(0,(Min(T[h],MAX_TEMP)-BASE_TEMP)/T.size());
		}

		//At end of day, compute proportion of buds mineable
		m_probBudMineable=0;

		if(m_ddays > 0) 
		{
			m_probBudMineable=1/(1+exp(-((m_ddays-a)/(b*sqrt(m_ddays))))); //A sigmoid inreasing function (from 0 to 1) of ddays
		}
	}
}

void CSpruceBarkBeetleTree::ComputeShootDevel(const CDailyWaveVector& T)
{
	//Shoot development 
	//This is calculated for the first summer, then reset on December 31.
	//This is part of the tree's daily "Live" loop, starting on START_DATE

	//Constants and parameters 
	static const int    START_DATE = 92-1;    //First day (0- based) for degree day summation for bud develpment
	static const double BASE_TEMP  = 11.5;    //dd summation of bud development
	static const double MAX_TEMP   = 35.;     //dd summation of bud development

	if(T.GetFirstTRef().GetJDay() >= START_DATE)
	{
		// Loop over every time step in one day
		for (int h=0; h<(int)T.size(); h++)
		{
			//Linear DDays with upper threshold
			m_ddShoot += Max(0,(Min(T[h],MAX_TEMP)-BASE_TEMP)/T.size());
		}

	}
}
*/
//******************************************************************
//CSpruceBarkBeetle class

//static CSpruceBarkBeetleDevelopmentTable RATES;

//bool CSpruceBarkBeetle::AUTO_BALANCE_EGG=false;
//const double CSpruceBarkBeetle::EGG_FREEZING_POINT=-10;
//const double CSpruceBarkBeetle::ADULT_FREEZING_POINT=-10;

//*****************************************************************************
// Object creator
//
// Input: int creationDay: the day of the creation
//		  int stage: the stage of the insect when the object will be created
//        
//
// Output: 
//
// Note: m_relativeDevRate member is modified.
//*****************************************************************************
CSpruceBarkBeetle::CSpruceBarkBeetle(CHost* pHost, CTRef creationDate, double age, bool bFertil, int generation, double scaleFactor):
	CBug(pHost, creationDate, age, bFertil, generation, scaleFactor)
{
//	m_bSwarming=false;
	m_hibernationDD;
	m_curEmergence=0;
	m_parentEmergence=0;
	m_broodIndex=0;
	m_lastDayLength = -1;
	m_automnP=0;
	m_automnF=0;
//	m_diapauseDayLength = 0;
	

	m_nbEmergence = GetStand()->m_nbReemergeMax;
	m_hibernationDD=0;
	m_reemergenceDD=0;
	m_bFlight=false;
	m_flightDD=0;
	m_ovipositionAge=0;
	m_reemergenceDDRequired.fill(0);
	m_reemergenceT.fill(0);
	m_emergenceDD=0;
	m_emergenceTmax=0;
	m_stageDate = creationDate;
//	m_januaryDayLength=-1;
	
	
	//A creation date Bis assigned to each individual
	//Individual's "relative" development rate for each life stage
	//These are independent in successive life stages
	AssignRelativeDevRate();

	//m_nbDayTeneral=-1;

	if( age==ADULT && creationDate.GetJDay()==0)
	{
		m_ecdysisDate=CTRef( creationDate.GetYear()-1, SEPTEMBER, 14);
		m_stageDate = m_ecdysisDate; 
		m_diapauseDate=m_ecdysisDate+16;
	}


	
	
}

// Object destructor
CSpruceBarkBeetle::~CSpruceBarkBeetle(void)
{
}

CSpruceBarkBeetle::CSpruceBarkBeetle(const CSpruceBarkBeetle& in):CBug(in)
{
	operator=(in);
}

CSpruceBarkBeetle& CSpruceBarkBeetle::operator=(const CSpruceBarkBeetle& in)
{
	if( &in !=this)
	{
		CBug::operator=(in);
		
		m_relativeDevRate = m_relativeDevRate; 
		m_relativeOviposition = in.m_relativeOviposition ;
		m_relativeFecondity = in.m_relativeFecondity ;
//		m_relativeWingsMaturation = in.m_relativeWingsMaturation ;
		m_springSwarmingT = in.m_springSwarmingT ;
		m_reemergenceDDRequired = in.m_reemergenceDDRequired ;
		m_reemergenceT = in.m_reemergenceT;
		
//		m_diapauseDayLength = in.m_diapauseDayLength ;
		m_broodIndex = in.m_broodIndex ;
	
	
		//m_januaryDayLength = in.m_januaryDayLength ;
		m_temperatureType = in.m_temperatureType ;

	
		m_swarmingDate = in.m_swarmingDate ;
		m_diapauseDate = in.m_diapauseDate ;
		m_emergenceDate = in.m_emergenceDate ;
		m_ecdysisDate = in.m_ecdysisDate ;
		m_stageDate = in.m_stageDate ;
	

		m_nbEmergence = in.m_nbEmergence;
		m_curEmergence = in.m_curEmergence ;
		m_parentEmergence = in.m_parentEmergence ;
		m_reemergenceDD = in.m_reemergenceDD ;
		m_bFlight = in.m_bFlight;
		m_flightDD = in.m_flightDD ;
		m_ovipositionAge = in.m_ovipositionAge ;
		m_hibernationDD = in.m_hibernationDD ;
		m_hibernationDDrequired = in.m_hibernationDDrequired ;
	
		m_TStat = in.m_TStat ;
		m_TI50Stat = in.m_TI50Stat ;
		m_dayLength = in.m_dayLength ;
		m_DI50Stat = in.m_DI50Stat ;

		m_absDiapauseStatI = in.m_absDiapauseStatI ;
		m_absDiapauseStatII = in.m_absDiapauseStatII ;
		m_wStat = in.m_wStat;
		m_TAFStat = in.m_TAFStat;
		m_RStat = in.m_RStat;
		m_dTStat = in.m_dTStat;
		m_dDLStat = in.m_dDLStat;
		m_awakeStat = in.m_awakeStat;
		m_emergenceDD = in.m_emergenceDD;
		m_emergenceTmax = in.m_emergenceTmax;

	}

	return *this;
}

//*****************************************************************************
// AssignRelativeDevRate sets the relative development rate for all stages
//
// Input: 
//
// Output: 
//
// Note: m_relativeDevRate member is modified.
//*****************************************************************************
void CSpruceBarkBeetle::AssignRelativeDevRate()
{
	for(int s=0; s<NB_STAGES; s++)
		m_relativeDevRate[s] = CSBBDevelopmentTable::GetRelativeRate(RandomGenerator(), s, m_sex);

	m_relativeOviposition = CSBBOviposition::GetRelativeRate(RandomGenerator(), CSBBOviposition::DEVELOPEMENT_RATE); 
	m_relativeFecondity = CSBBOviposition::GetRelativeRate(RandomGenerator(), CSBBOviposition::BROOD_RATE); //fecundity is determined at moult from L5 to L6, based on L6 survival probability (in CSpruceBarkBeetle::IsDeadByWindow(double T, double dt) )
	

	//some beetle hibernate in tree and some hibernate in the soil
	//find proportion
	
	//bark position after Podlesnik 2012
	//hot: 46%, mid 38% and cold: 16%
	double treePosition = RandomGenerator().Randu();
	m_temperatureType = treePosition<0.46?T_BARK_TOP_OPEN:treePosition<0.84?T_BARK_BOTTOM_OPEN:T_BARK_CLOSE;
	//m_temperatureType = testPos<0.333?T_BARK_TOP_OPEN:testPos<0.666?T_BARK_BOTTOM_OPEN:T_BARK_CLOSE;

	//m_temperatureType = testPos<0.5?T_BARK_TOP_OPEN:T_BARK_CLOSE;
	
		//double p1[4]={164.20 ,58.00  ,19.7, 0.65 }; old
	
		
	//compute DD summation required for spring swarming
	
	//double p1[2]={GetStand()->m_p[2], GetStand()->m_p[3]};
	double p1[2]={170.2, 54.6};
	m_hibernationDDrequired = RandomGenerator().RandNormal(p1[0], p1[1]);
	while( m_hibernationDDrequired<20 || m_hibernationDDrequired>400 )
		m_hibernationDDrequired = RandomGenerator().RandNormal(p1[0], p1[1]);

	double s[3] = {1102.0,-3.4,0.58};
	double ΔElev = (GetModel()->GetInfo().m_loc.m_elev - s[0])/100;
	double ΔT = s[1]*(1/(1+exp(-s[2]*ΔElev))-0.5);
		
	//compute maximum air temperature threshold
	m_springSwarmingT = RandomGenerator().RandNormal(TMAX_THRESHOLD_MEAN+ΔT, TMAX_THRESHOLD_SD);
	while( m_springSwarmingT<16 || m_springSwarmingT>30)
		m_springSwarmingT = RandomGenerator().RandNormal(TMAX_THRESHOLD_MEAN+ΔT, TMAX_THRESHOLD_SD); 
	

//	double reemerge1 = GetStand()->m_p[3]/100;

	
	//if(GetStand()->m_nbReemergeMax>=2 && RandomGenerator().Randu()<reemerge1 )
	{
		
		//reemergence need a normal distribution and a relative reemergence rate

		


		//first re-emergence


		//double MEAN_DD = GetSex()==MALE? GetStand()->m_p[2]: GetStand()->m_p[3];
		double MEAN_DD = GetSex()==MALE?  47.9: 226.6;
		double REEMERGE_TMAX_THRESHOLD_MEAN =  18.0;//GetStand()->m_k[2];//
		double REEMERGE_TMAX_THRESHOLD_SD =  3.213;//GetStand()->m_k[3];//
		static const double TOP_SUN_ACCELERATION = 0.516;


		//after Anderbrant 1986
		double R = RandomGenerator().Rand(0.0, 0.84499);
		double D = pow(log((R-0.845)/(-1.2547))/-1.3909, 1/1.69)-0.475;
		double Tvar = treePosition<0.46?0.58:treePosition<0.84?1.00:1.64;
			


		m_reemergenceDDRequired[0] = Tvar*D*MEAN_DD;
		m_reemergenceT[0] = RandomGenerator().RandNormal(REEMERGE_TMAX_THRESHOLD_MEAN, REEMERGE_TMAX_THRESHOLD_SD);
		while( m_reemergenceT[0]<16 || m_reemergenceT[0]>30)
			m_reemergenceT[0] = RandomGenerator().RandNormal(REEMERGE_TMAX_THRESHOLD_MEAN, REEMERGE_TMAX_THRESHOLD_SD);
		 
		m_femaleIndex[0] = RandomGenerator().RandNormal( 1.77,  2.586);
		//m_femaleIndex[0] = RandomGenerator().RandNormal( GetStand()->m_k[2],  GetStand()->m_k[3]);
		

		m_automnP = RandomGenerator().RandNormal(15,2);
		//m_automnP = RandomGenerator().RandNormal(GetStand()->m_s[0], GetStand()->m_s[1]);

		m_automnF = RandomGenerator().RandNormal(11.0,2.31);
		//m_automnF = RandomGenerator().RandNormal(GetStand()->m_s[0], GetStand()->m_s[1]);


		//m_femaleIndex[0] = RandomGenerator().RandNormal(  GetStand()->m_s[5],   GetStand()->m_s[6]);

		//second re-emergence
		treePosition = RandomGenerator().Randu();
		Tvar = treePosition<0.46?0.58:treePosition<0.84?1.00:1.64;

 		R = RandomGenerator().Rand(0.0, 0.84499);
		D = pow(log((R-0.845)/(-1.2547))/-1.3909, 1/1.69)-0.475;
		
		m_reemergenceDDRequired[1]=Tvar*12.5*2.2*D*MEAN_DD;//after Öhrn, second reemergence need 2.2 x DD5 of the first reemergence
		
		m_reemergenceT[1] = RandomGenerator().RandNormal(REEMERGE_TMAX_THRESHOLD_MEAN, REEMERGE_TMAX_THRESHOLD_SD);
		while( m_reemergenceT[1]<16 || m_reemergenceT[1]>30)
			m_reemergenceT[1] = RandomGenerator().RandNormal(REEMERGE_TMAX_THRESHOLD_MEAN, REEMERGE_TMAX_THRESHOLD_SD);

		m_femaleIndex[1] = RandomGenerator().RandNormal( 1.77,  2.586);
	}
	
	//double R = RandomGenerator().Rand(0.0, 0.84499);
	//double D = pow(log((R-0.845)/(-1.2547))/-1.3909, 1/1.69)-0.475;
	//double Tvar = treePosition<0.46?0.58:treePosition<0.84?1.00:1.64;
	

	//double ΔBI = (m_broodIndex - GetStand()->m_k[0])*10;
	//double pEmerge = 1-1/(1+exp(-GetStand()->m_k[1]*ΔBI));
	double ΔBI = (m_broodIndex - 0.026)*10;
	double pEmerge = 1-1/(1+exp(-0.6*ΔBI));

	m_emergenceTmax = 100;
	if( RandomGenerator().Randu()<pEmerge )
		m_emergenceTmax = RandomGenerator().RandNormal(20.2, 3.08);
	//	m_emergenceTmax = RandomGenerator().RandNormal(GetStand()->m_s[5], GetStand()->m_s[6]);
	
	//m_relativeWingsMaturation = RandomGenerator().RandNormal(12.5, 1.2);
	//while( m_relativeWingsMaturation<9 || m_relativeWingsMaturation > 16.5)
	//	m_relativeWingsMaturation = RandomGenerator().RandNormal(12.5, 1.2);
}




//*****************************************************************************
// Live is for one day development
//
// Input: CDailyWaveVector T: the temperature for all time step
//
// Output: return true if the bug continue to be in the population, false otherwise
//*****************************************************************************
void CSpruceBarkBeetle::Live(const CDailyWaveVector& T, const CWeatherDay& wDay)
{
	if( IsCreated(T.GetFirstTRef()) ) 
	{
		CBug::Live(T, wDay);

		if(m_lastDayLength==-1)
			m_lastDayLength= GetModel()->GetInfo().m_loc.GetDayLength(T.GetFirstTRef()-1)/3600;


		for(int h=0; h<(int)T.size()&&IsAlive()&&T.GetFirstTRef()!=m_swarmingDate; h++)
			Develop(T.GetFirstTRef(), T[h], wDay, T.NbStep());

		if(T.GetFirstTRef() == m_swarmingDate)
		{
			m_curEmergence++;
			m_stageDate=T.GetFirstTRef();
			m_ovipositionAge=0;
			m_emergenceDD=0;
			m_reemergenceDD=0;
			m_bFlight = true;
			m_flightDD = 0;
		}

		m_lastDayLength = GetModel()->GetInfo().m_loc.GetDayLength(T.GetFirstTRef())/3600;
	}
}

//void CSpruceBarkBeetle::Brood(const CDailyWaveVector& T, const CWeatherDay& wDay)
//{
//	
//	if( GetStage() == ADULT && m_sex == FEMALE )
//	{
//		CSpruceBarkBeetleTree* pTree = (CSpruceBarkBeetleTree*) m_pHost;
//
//		//For optimization, if AUTO_BALANCE_EGG is active, we reduce the number of eggs created
//		double pSurv = AUTO_BALANCE_EGG?GetOverallSurvival(pTree->GetEnergyLoss()):1;
//
//		//determine individual's egg production this time step, 
//		int newEggs=int(m_brood*pSurv);
//		for(int i=0; i<newEggs; i++)
//		{
//			if( m_bFertil )
//			{
//				//if( RandomGenerator().Randu() < pSurv)
//				pTree->AddBug( new CSpruceBarkBeetle(pTree, T.GetFirstTRef(), EGG, true, m_generation+1, m_scaleFactor) );
//			}
//
//			//std::vector<int>::push_back(
//			m_nbEgg++;
//		}
//	}
//}

void CSpruceBarkBeetle::Brood(const CDailyWaveVector& T, const CWeatherDay& weaDay)
{
	if( m_bFertil && m_brood>0)
	{
		
		//When the bugs are killed by tree at the end of the day, they must not create eggs
		CSpruceBarkBeetleTree* pTree = GetTree();
		CSpruceBarkBeetleStand* pStand = GetStand(); ASSERT( pStand );
		
		//attrition turned off,
		double scaleFactor = m_brood*pStand->m_survivalRate*m_scaleFactor;
		double scaleFactorAttrition = m_brood*(1-pStand->m_survivalRate)*m_scaleFactor;
		ASSERT(scaleFactor>0);
		ASSERT( m_curEmergence>=1 && m_curEmergence<=3 );

		CSpruceBarkBeetle* pBug = new CSpruceBarkBeetle(pTree, T.GetFirstTRef(), EGG, pStand->m_bFertilEgg, m_generation+1, scaleFactor);
		pBug->m_parentEmergence = m_curEmergence;
		pBug->m_broodIndex = (m_curEmergence-1)/3 + m_ovipositionAge/3;
		pTree->AddBug(pBug);

		if( scaleFactorAttrition>0)
		{
			CSpruceBarkBeetle* pBug = new CSpruceBarkBeetle(pTree, T.GetFirstTRef(), EGG, pStand->m_bFertilEgg, m_generation+1, scaleFactorAttrition);
			pBug->m_status = DEAD;
			pBug->m_death = ATTRITION;
			pTree->AddBug(pBug);
		}


		ASSERT(pBug->m_broodIndex>=0 && pBug->m_broodIndex<=1);
		
	}
}


void CSpruceBarkBeetle::ComputetDiapause(int s, double T, double DL)
{
	//NbVal=    24	Bias= 0.01396	MAE= 0.08396	RMSE= 0.23382	CD= 0.94169	R²= 0.94276
	//p0                  	=  25.0284  
	//p1                  	=   0.30783  
	//p2                  	=   0.73825  
	//k4                  	=   0.46431  
	//s0                  	=   0.06167  
	//s1                  	=   0.00816  
	//s2                  	=   0.08534  
	//s3                  	=   0.07214  
	//s4                  	=   0.53467  
	//s5                  	=   0.65856  
	//s6                  	=   0.01276  

	static const double Th1_T = 12.4;
	static const double Th2_T = 22.8;
	static const double MU_DL  = 15.7; 
	static const double PHI_DL =  1.0417;
	static const double rangeT = Th2_T-Th1_T;
	static const double meanT = (Th1_T+Th2_T)/2;
	static const double LIMIT_HI = 26.749;
	static const double LIMIT_LO = 4.5689;
	static const double rangeDL = LIMIT_HI-LIMIT_LO;

//1- ΔDL: day length shift caused by overheating factor
	const double To_OH = 25.0284;
	const double PHI_OH = 0.30783;
	const double K_OH = 0.73825;
	double ΔOH = T-To_OH;
	double ΔDL_OH = (ΔOH>0)?K_OH*(exp(-PHI_OH*ΔOH)-1):0;

//2- DI50 : day length (with overheating factor) at witch 50% of the beetle diapause 
	double p = 0.5 + (T-meanT)/rangeT;
	double logit = p<=0?rangeDL:p>=1?-rangeDL:-log(p/(1-p))/PHI_DL;
	double DI50 = MU_DL+ΔDL_OH+logit;

//3- compute DL_OH: day length with overheating at witch 50% of the beetle diapause ????
	//double DL_OH = MU_DL+ΔDL_OH;

//3- ΔT: compute the shift of the temperature du to the shift of day length overheating 
	double delta_DL = DL - (MU_DL + ΔDL_OH);
	double ΔT = rangeT*(0.5-1/(1+exp(-PHI_DL*delta_DL)));
	
//4- TI50: temperature at witch 50% of the beetle diapause
	double TI50 = meanT + ΔT;

//5- distance
	double d_T = T-TI50;
	double d_DL = DL - DI50; 

	const double Kd = 0.80866;
	double d = Kd*d_T + (1-Kd)*d_DL;
	ASSERT( Signe(d_T) == Signe(d_DL) );

//6-stage weight
	static const double S[NB_STAGES] = {0.06167,0.00816,0.08534,0.07214,0.53467,0.65856,0};
	double sumS = 0;
	for(int i =0; i<=TENERAL_ADULT; i++)
		sumS+=S[i];

//7- statistics	
	m_dTStat += d_T*S[s]/sumS;
	m_dDLStat += d_DL*S[s]/sumS;
	//m_wStat += d*S[s]/sumS;

	double fixedT = meanT+rangeT*(0.5-1/(1+exp(-PHI_DL*(DL-MU_DL))));
	double fixeDL = MU_DL + logit;

	double weight = S[s]/sumS;
	//double weightDL = d_DL*S[s]/sumS;

	m_TStat+= weight*T;
	m_TI50Stat += weight*fixedT;
	m_dayLength += weight*DL;
	m_DI50Stat += weight*fixeDL;
	m_absDiapauseStatI += abs(weight);
	//m_absDiapauseStatII += abs(weightDL);

}

void CSpruceBarkBeetle::ComputeSwarming(int s, double T, double DL)
{

	//NbVal=    12	Bias=-0.54498	MAE= 2.93293	RMSE= 4.12957	CD= 0.95846	R²= 0.96195
	//p0                  	=  22.96510  
	//p1                  	=   0.74469  
	//p2                  	=   2.69680  
	//p3                  	=   0.73520  
	//k0                  	=   3.94158  
	//k1                  	=   0.37604  
	//k2                  	=   3.04991  
	//k3                  	=   0.30004  
	//k4                  	=   0.20939  
	//s0                  	=   0.40061  
	//s1                  	=   0.01438  
	//s2                  	=   0.04765  
	//s3                  	=   0.00792  
	//s4                  	=   0.94654  
	//s5                  	=   0.10148  
	//s6                  	=   1.75665  

	static const double Th1_T = 12.4;
	static const double Th2_T = 22.8;
	static const double MU_DL  = 15.7; 
	static const double PHI_DL =  1.0417;
	static const double rangeT = Th2_T-Th1_T;
	static const double meanT = (Th1_T+Th2_T)/2;
	static const double LIMIT_LO = 4.5689;
	static const double LIMIT_HI = 26.749;
	static const double rangeDL = LIMIT_HI-LIMIT_LO;


//1- ΔDL: change DI50 by a overheating factor
	const double Th_OH  =22.96510;
	const double PHI_OH = 0.74469;
	const double K_OH   = 2.69680;


	double ΔOH = T-Th_OH;
	double ΔDL_OH = (ΔOH>0)?K_OH*(exp(-PHI_OH*ΔOH)-1):0;

//2- DI50: day length (with overheating) at witch 50% of the beetle diapause
	double DI50 = MU_DL+ΔDL_OH;

//3- ΔT: compute the shift of the temperature du to the day length
	double ΔDL = DL - DI50;
	double ΔT = rangeT*(0.5-1/(1+exp(-PHI_DL*ΔDL)));
	
	
//4- compute TI50: temperature at witch 50% of the beetle diapause
	double TI50 = meanT + ΔT;

	
//5- distance
	double d_T = T-TI50;

	double p = 0.5 + (T-meanT)/rangeT;
	double logit = p<=0?rangeDL:p>=1?-rangeDL:-log(p/(1-p))/PHI_DL;
	double d_DL = DL - (MU_DL+logit+ΔDL_OH); 
	ASSERT( Signe(d_T) == Signe(d_DL) );

	double P3 = 0.73520;
	double w =  (P3*d_T + (1-P3)*d_DL);
		

	static const double S[NB_STAGES] = {0.40061, 0.01438, 0.04765, 0.00792, 0.94654, 0.10148, 0};
	double sumS = 0;
	for(int i=0; i<=TENERAL_ADULT; i++)
		sumS+=S[i];

	

	//const CSBBDevelopmentTable& rates = GetStand()->m_rates;
	double R = GetStand()->m_rates.GetRate(s,T);
	m_RStat += R*S[s]/sumS;

	static const double K0 = 3.94158;
	static const double K1 = 0.37604;
		
	double taf = Max(0, (1 - K0*exp(-K1*w))*S[s]/sumS);
	m_TAFStat += taf;
	
	//m_wStat += w*S[s]/sumS;
}


//*****************************************************************************
// Develop is for one time step development
// Input: double T: is the temperature for one time step
//        short nbStep: is the number of time steps per day (24h/step duration(h) )
//*****************************************************************************
void CSpruceBarkBeetle::Develop(CTRef date, double T, const CWeatherDay& wDay, short nbSteps)
{
	//Develops all individuals, including ovipositing adults
	_ASSERTE(m_status==HEALTHY);

	int s = GetStage();
	const CSBBDevelopmentTable& rates = GetStand()->m_rates;
	double dayLength = GetModel()->GetInfo().m_loc.GetDayLength(date)/3600;
	
	

	//for calibration purpose
	//if( GetStand()->m_longDayLength>=0 )
	//{
	//	dayLength = GetStand()->m_longDayLength;
	//	//if( s>=GetStand()->m_firstStageShortDay )//&& s<=GetStand()->m_lastStageShortDay)
	//	 
	//	if( GetStand()->m_nbShortDayBegin.IsInit() && date-GetStand()->m_nbShortDayBegin<=GetStand()->m_nbShortDay)
	//	{
	//		dayLength = GetStand()->m_shortDayLength;
	//	}
	//}

	/*
	static const double Th1_T = 12.4;
	static const double Th2_T = 22.8;
	static const double MU_DL  = 15.7; 
	static const double PHI_DL =  1.0417;
	static const double rangeT = Th2_T-Th1_T;
	static const double meanT = (Th1_T+Th2_T)/2;
	static const double LIMIT_LO = 4.5689;
	static const double LIMIT_HI = 26.749;
	static const double rangeDL = LIMIT_HI-LIMIT_LO;


//1- ΔDL: change DI50 by a overheating factor
	const double Th_OH = GetStand()->m_p[0];
	const double PHI_OH = GetStand()->m_p[1];
	const double K_OH = GetStand()->m_p[2];


	double ΔOH = T-Th_OH;
	double ΔDL_OH = (ΔOH>0)?K_OH*(exp(-PHI_OH*ΔOH)-1):0;

//2- compute DI50: day length at witch 50% of the beetle diapause
	
	double DI50 = MU_DL+ΔDL_OH;

//3- ΔT: compute the shift of the temperature du to the day length
	
	double ΔDL = dayLength - DI50;
	double ΔT = rangeT*(0.5-1/(1+exp(-PHI_DL*ΔDL)));
	
	
//4- compute TI50: temperatrue at witch 50% of the beetle diapause
	double TI50 = meanT + ΔT;

	
//5- distance
	double d_T = T-TI50;

	double p = 0.5 + (T-meanT)/rangeT;
	double logit = p<=0?rangeDL:p>=1?-rangeDL:-log(p/(1-p))/PHI_DL;
	double d_DL = dayLength - (MU_DL+logit+ΔDL_OH); 
	ASSERT( Signe(d_T) == Signe(d_DL) );


		
	if( s<=TENERAL_ADULT && !m_diapauseDate.IsInit() && !m_swarmingDate.IsInit() )
	{
		double sumS= 0;
		for(int i =0;i<=TENERAL_ADULT;i++)
			sumS+=GetStand()->m_s[i];

		//m_dTStat += d_T*GetStand()->m_s[s]/sumS;
		//m_dDLStat += d_DL*GetStand()->m_s[s]/sumS;

		double P3 = GetStand()->m_p[3];
		double w =  (P3*d_T + (1-P3)*d_DL);
		
		
		m_wStat += w;
		
		//if(s<TENERAL_ADULT)
		m_RStat += rates.GetRate(s,T)*GetStand()->m_s[s]/sumS;
		
		double K0 = GetStand()->m_k[0];
		double K1 = GetStand()->m_k[1];
		
		m_TAFStat += Max(0, (1 - K0*exp(-K1*w))*GetStand()->m_s[s]/sumS);
		

			

		//m_TAFStat += d_DL*GetStand()->m_s[s]/sumS;
		//m_TAFStat += 1 - K0*exp(-K1*w);



		double weight = d_T*GetStand()->m_s[s]/sumS;
		m_TStat+= abs(weight)*(T);
		m_TI50Stat += abs(weight)*(T+(meanT-TI50));
		m_dayLength += abs(weight)*dayLength;
		m_DI50Stat += abs(weight)*(dayLength+(MU_DL-DI50));
		 
		m_absDiapauseStatI += abs(weight);

		//m_TAFStat += 1 - Max(0, Min(1, K0*exp(-K1*w) ));
		

		//bool bComputeDiapause = true;
		//if( bComputeDiapause )
		//{
		//	m_dTStat += d_T*GetStand()->m_s[s]/sumS;
		//	ComputeSwarming(s, T, dayLength);
		//}
		//else 
		//{
		//	ComputetDiapause(s, T, dayLength);
		//	m_wStat += d*GetStand()->m_s[s]/sumS;
		//	//m_dDLStat += d_DL*GetStand()->m_s[s]/sumS;
		//}
	}
	*/
 


	double TAF = 1;
	double dailyR = rates.GetRate(s,T);

	/*if( s<=TENERAL_ADULT && 
		m_ecdysisDate.GetYear() == date.GetYear() &&
		!m_diapauseDate.IsInit() )
		ComputeSwarming(s, T, dayLength);

	
	
	if( s>=TENERAL_ADULT && 
		m_ecdysisDate.GetYear() == date.GetYear() &&
		!m_diapauseDate.IsInit() )
	{
		//swarming computation
		double rate = m_RStat[MEAN];
		double taf = m_TAFStat[MEAN];

		
		double P0 = 3.04991;
		double P1 = 0.30004;
		double P2 = 0.20939;
		//double P0 = GetStand()->m_p[0];
		//double P1 = GetStand()->m_p[1];
		//double P2 = GetStand()->m_p[2];
			
		if( dailyR>0)
			dailyR = Max(0, Min(1, P0*(P1*dailyR + (1-P1)*rate) + P2 ));

		//double S6 = GetStand()->m_s[6];
		//double S6 = 1.75665;
		//double P3 = 1.75665;
		//double P3 = GetStand()->m_p[3];
		//TAF =Max(0, P3*taf);

		//double ΔDL = dayLength - GetStand()->m_k[0];
		//double ΔT = GetStand()->m_k[1]*(1-1/(1+exp(-GetStand()->m_k[2]*ΔDL)));
		//TAF =Max(0, Min(1, ΔT*taf));
					
		m_wStat += dailyR*TAF;
	}
	*/
	
	double RR = TAF*m_relativeDevRate[s]*dailyR/nbSteps;
	_ASSERTE(RR >=0 );
	
	//after Öhrn 2014
	m_flightDD+=wDay.GetDD(FLIGHT_DD_THRESHOLD)/nbSteps;//at 5°C
	if( m_bFlight && m_flightDD<FLIGHT_DD_SUM )
		RR=0;//stop development for 2-3 days after swarming: the time to find and attack a tree
	else 
		m_bFlight=false;

	if( ChangeStage(RR) )
		m_stageDate = date;


	if( s == PUPAE && ChangeStage(RR) )
		m_ecdysisDate = date;


	//compute if beetle diapause
	if( !m_diapauseDate.IsInit() )
	{
		int TADays = (date-m_stageDate);
		if( TADays>=14 &&
			date.GetJDay()>172)//never diapause before the summer)
		{
			//if the expected length after two week to finish his stage if longer than 85 days, we diapause
			if(s<ADULT)
			{
				double relDev = GetRelDev();
				if( relDev== 0 || 
					TADays/relDev>=90 )//||
					//dayLength < ?? )
				{
					m_diapauseDate = date;
					m_emergenceDate.Reset();
				}
			}
			else
			{
				if( m_ovipositionAge==0 || 
					TADays/m_ovipositionAge>=130)
				{
					m_diapauseDate = date;
					m_ovipositionAge=0;//female is full after winter
					m_emergenceDate.Reset();
					m_swarmingDate.Reset();
				}
			}
		}
	}


	if( m_diapauseDate.IsInit() )
	{
		//emergence

		RR=0;
		double K[5] = {12.8, 14.0, 19.2, -2.8, 0.46};
		CTRef lastSnow = GetStand()->m_lastSnow[date.GetYear()];
		if(	m_diapauseDate.GetYear() != date.GetYear() &&	 //it's not the year of diapause
			dayLength>=K[0] && 
			(!lastSnow.IsInit() || date>(lastSnow+int(K[1]))) )
		{
			//compute DD summation threshold in function of day length
			double ΔDL1 = dayLength - K[2];
			double ΔT = K[3]*(1/(1+exp(-K[4]*ΔDL1)));
				
			m_hibernationDD += Max(0, T - ( 5+ΔT) )/nbSteps; 

			if( !m_emergenceDate.IsInit() && 
				m_hibernationDD >= m_hibernationDDrequired)
			{
				m_emergenceDate=date;
				m_diapauseDate.Reset();
			}
		}
	}//if diapause
	else
	{
		ASSERT( !m_swarmingDate.IsInit() || date!=m_swarmingDate );
		
		//compute if beetle swarm
		if( s==ADULT && 
			RR>0 )
		{
			CTRef firstSnow = GetStand()->m_firstSnow[date.GetYear()];
			
			if( m_ecdysisDate.GetYear() == date.GetYear() ) // newly beetle
			{
				//insect is not in diapause: look for swarming
				if(m_swarmingDate.IsInit())
				{
					//Re-emergence and swarming of newly filial beetle (F*_ii, F*_iii) of beetle
					
					
					if( m_curEmergence<m_nbEmergence &&
						m_ovipositionAge==1 && 
						( !firstSnow.IsInit() || date<firstSnow-14) )
					{
						double ΔΔDL = ((dayLength-m_lastDayLength)*3600 - 56.6)/10;
						double ΔT = (1/(1+exp(-0.52*ΔΔDL)));
						double last4 = GetTree()->GetLast4Days();
						double femaleIndex = ΔT*last4;
						
						m_reemergenceDD += Max(0, T-7.5)/nbSteps;
						
						if( m_reemergenceDD>=m_reemergenceDDRequired[m_curEmergence-1] )
							
						{
							if( (wDay.GetTMax() >= m_reemergenceT[m_curEmergence-1]) &&
								( GetSex()==MALE || femaleIndex >=  m_femaleIndex[m_curEmergence-1] ) )//female have an extra criterion
							{
								m_swarmingDate=date;
							}
							else
							{
								//autumn swarming 
								if(	date.GetJDay() > CTRef(-999, JUNE, 21).GetJDay() && 
									wDay.GetTMax() >= 5 &&
									dayLength < m_automnF)
									m_swarmingDate=date;
							}
						}
					}
				}
				else
				{
					///emergence and swarming of filial beetle (F*_i)
					if( wDay.GetTMax() >= m_emergenceTmax && 
						( !firstSnow.IsInit() || date<firstSnow-14) )
					{
						m_swarmingDate=date;
					}
					else
					{
						//add autumn swarming
						if(	date.GetJDay() > CTRef(-999, JUNE, 21).GetJDay() && 
							wDay.GetTMax() >= 5 &&
							dayLength < m_automnF)
							m_swarmingDate=date;
					}
					
				}//if swarming
			}//if newly beetle
			else
			{
				//old parental beetle
				ASSERT( m_emergenceDate.IsInit() );

				if(m_swarmingDate.IsInit())
				{
					//re-emergence and swarming of parent beetle (p2, p3)

					if( m_curEmergence<m_nbEmergence &&
						m_ovipositionAge==1 && 
						( !firstSnow.IsInit() || date<firstSnow-14) )
					{
						//double ΔΔDL = ((dayLength-m_lastDayLength)*3600 - GetStand()->m_k[0])/10;
						//double ΔΔDLIndex = (1/(1+exp(-GetStand()->m_k[1]*ΔΔDL)));

						double ΔΔDL = ((dayLength-m_lastDayLength)*3600 - 56.6)/10;
						double ΔΔDLIndex = (1/(1+exp(-0.52*ΔΔDL)));
						double TmeanLast4days = GetTree()->GetLast4Days();
						double femaleIndex = ΔΔDLIndex*TmeanLast4days;
						
						m_reemergenceDD += Max(0, T-7.5)/nbSteps;
						
						if( m_reemergenceDD>=m_reemergenceDDRequired[m_curEmergence-1] )
							
						{
							if( (wDay.GetTMax() >= m_reemergenceT[m_curEmergence-1]) &&
								( GetSex()==MALE || femaleIndex >=  m_femaleIndex[m_curEmergence-1] ) )//female have an extra criterion
							{
								m_swarmingDate=date;
							}
							else
							{
								//automn swarming 
								if(	date.GetJDay() > CTRef(-999, JUNE, 21).GetJDay() && 
									wDay.GetTMax() >= 5 &&
									dayLength < m_automnP)
									m_swarmingDate=date;
							}
						}
					}//emergence
				}
				else
				{
					//swarming of parent beetle (P1)
					double S[3] = {3.2, -5.1, 0.3};
					double ΔDL2 = dayLength - S[0];
					double ΔTmax = S[1]*(1/(1+exp(-S[2]*ΔDL2)));

					if(	m_curEmergence<m_nbEmergence &&
						wDay.GetTMax() >= m_springSwarmingT+ΔTmax )
					{
						m_swarmingDate=date;
					}
				}
			}
		}//if diapause
	}//adult

	
	m_age += RR;

	//Each day t, at temperature T
	//Females begin oviposition after 1 day out of 15 at 20 C, 1 days out of 30 at 12 C: 6.7% of lifespan
	if( RR>0 && s==ADULT  )
	{
		ASSERT( IsAlive() );
		ASSERT( m_curEmergence>=0 && m_curEmergence<=3 );
		ASSERT( m_ovipositionAge>=0 && m_ovipositionAge<=1 );
		//compute oviposition age

		
		if( m_ovipositionAge<1 )
		{
			

			//age development rate in function of temperature (days-1)
			double OAr = m_relativeOviposition*CSBBOviposition::GetRate(CSBBOviposition::DEVELOPEMENT_RATE, T)/nbSteps;
			m_ovipositionAge = Min(1, m_ovipositionAge+OAr);

			//ovipositing adult begin brood after 18.2 % of there age
			static const double BEGIN_BROOD = 0.182;
			
			
			
			if( m_ovipositionAge > BEGIN_BROOD)
			{
				//Oviposition rate in function of temperature (Eggs/days)
				double Or = CSBBOviposition::GetRate(CSBBOviposition::BROOD_RATE, T)/nbSteps;
			
				//Relative Oviposition Rate (days) in function of is relative oviposition development 
				double relOage = (m_ovipositionAge-BEGIN_BROOD)/(1-BEGIN_BROOD);
				double relOr = (0.244*pow(relOage,-3.30))/Square(0.096*pow(relOage,-2.3)+1);
			
				//Relative Oviposition Broods in function of the number of reemergence
				static const double RELATIVE_REEMERGENCE_BROOD[4] = {0, 1, 0.75, 0.5};
				double relOb = RELATIVE_REEMERGENCE_BROOD[m_curEmergence];

				if( m_sex==FEMALE )
				{
					//time step oviposition brood
					double brood = m_relativeFecondity*Or*relOr*relOb;
					m_brood += brood;
					m_totalBrood += brood;
				}

				ASSERT( relOage>=0 && relOage<=1 );
				
				
			}
		}
	}

	//Mortality
	if(GetStage() >= DEAD_ADULT)
	{
		m_status=DEAD;
		m_death=OLD_AGE;
	}
	else if( m_curEmergence==m_nbEmergence && m_ovipositionAge==1)
	{
		m_age = DEAD_ADULT;
		m_status=DEAD;
		m_death=OLD_AGE;
	}
/*	else if( (GetStage() == EGG && (T<CSpruceBarkBeetle::EGG_FREEZING_POINT||m_eggAge>30)))
	{
		m_status=DEAD;
		m_death=FROZEN;
	}
	else if( (GetStage() == ADULT && T<CSpruceBarkBeetle::ADULT_FREEZING_POINT) )
	{
		m_age +=1;
		m_status=DEAD;
		m_death=OLD_AGE;
	}
	else if(IsDeadByAttrition(T, RR)) 
	{
		if( !(GetStage() == L2 && m_generation == 1))
		{
			m_status=DEAD;
			m_death=ATTRITION;
		}
	}
	else if( m_generation == 0 && bLookAnynchrony && IsDeadByAsynchrony(T, 1.0/nbStep) )
	{
		//Bug dies if its synchrony luck is greater than the synchrony survival value at the time of its emergence from ovewintering
		m_status=DEAD;
		m_death=ASYNCHRONY;
	}
	else if(IsDeadByOverwintering(T, 1.0/nbStep) )
	{
		m_status=DEAD;
		m_death=MISSING_ENERGY;
	}
	else if( bLookWindow && IsDeadByWindow(T, 1.0/nbStep) )
	{
		//Bug dies if its Window luck is greater than the Window survival value at the time of its moult to the L6
		m_status=DEAD;
		m_death=WINDOW;
	}
	*/	


	
}




//*****************************************************************************
// IsDeadByAttrition is for one time step development
//
// Input: double T: is the temperature for one time step
//        double r: developement rate 
//
// Output: The new status of the insect after attrition
//*****************************************************************************
bool CSpruceBarkBeetle::IsDeadByAttrition(double T, double r)const
{
	bool bDead=false;
	if( GetStand()->m_bApplyAttrition)
	{
		//Computes attrition (probability of survival in a given time step, based on development rate)
		//double p_survival = pow(CSpruceBarkBeetleAttrition::GetRate(GetStage(), T), r);
		//double u = RandomGenerator().Randu();
		//if(u>p_survival)
			//bDead=true;

	}

	return bDead;
}


bool CSpruceBarkBeetle::IsDeadByOverwintering(double T, double dt)
{
	//L2o Survival module
	bool bDead = false;
	if(GetStand()->m_bApplyWinterMortality)
	{
		if( GetStage()==EGG && T<0 )
		{
			bDead = true;
			//m_energy += -0.03328 * pow(max(0.0,T/10),3.5602) * dt/7; //SAS Survival.lst
			//bDead = (m_overwinterLuck >= (1/(1+exp(-(m_energy))))*0.6939); //0.4536/0.6537; if true, insect dies. Otherwise, it survives time step.
		}
		else if( GetStage()<PUPAE  && T<-13 )//viiri 2012
		{
			bDead = true;
		}
		else if( GetStage()==PUPAE && T<-17 )//viiri
		{
			bDead = true;
		}
		else if( GetStage()==TENERAL_ADULT && GetRelDev() < 0.5 && T<-13 )//new emerged adult will die at -13
		{
			bDead = true;
		}
		else
		{
			//adult
		}
	}

	return bDead;
}


//*****************************************************************************
// GetStat: GetStat is call daily to get ther states of the object
//
// Input: CTRef d: the actual day
// CSpruceBarkBeetleStat& stat: the statistic object
//
// Output: The stat is modified
//*****************************************************************************
void CSpruceBarkBeetle::GetStat(CTRef d, CModelStat& stat)
{
	short oldStage = short(m_lastAge);
	short stage = GetStage();

	if( m_generation==0 )
	{
		//total and daily brood
		if( m_curEmergence>=1 && m_curEmergence<=3)
			stat[S_BROOD_0_P1+m_curEmergence-1] += m_brood*m_scaleFactor;


		stat[S_TOTAL_BROOD_0] += m_totalBrood*m_scaleFactor;

		if( IsAlive() )
		{
			stat[S_ADULT_0]+=m_scaleFactor;
		}
		else 
		{
			if( stage == DEAD_ADULT )
				stat[S_DEAD_ADULT_0]+=m_scaleFactor;
		}

		
		if( m_emergenceDate==d )
		{
			stat[S_EMERGENCE_0]+=m_scaleFactor;
		}

		
		if( m_swarmingDate==d )
		{
			ASSERT( m_curEmergence>=1 && m_curEmergence<=3);//m_curEmergence is not updated yet
			stat[S_SWARMING_0_P1+m_curEmergence-1]+=m_scaleFactor;
			ASSERT( stage>=ADULT);

			if( m_sex == FEMALE )
				stat[S_TOTAL_FEMALE_0]+=m_scaleFactor;

			
			if( m_curEmergence==2 )
			{
				if(GetSex()==MALE)
					stat[S_SWARMING_0_P2_MALE]+=m_scaleFactor;
				else
					stat[S_SWARMING_0_P2_FEMALE]+=m_scaleFactor;
			}
		}


		if( d == m_diapauseDate )
			stat[S_DIAPAUSE_0]+=m_scaleFactor;
		

	}
	else if( m_generation==1 )
	{ 
		stat[S_BROOD_1] += m_brood*m_scaleFactor;
		stat[S_TOTAL_BROOD_1] += m_totalBrood*m_scaleFactor;

		if( IsAlive() )
		{
			stat[S_EGG_1+stage]+=m_scaleFactor;

//			stat[S_AGE] += m_age*m_scaleFactor;
			//stat[S_NB_BUGS] += m_scaleFactor;

		}
		else 
		{
			if( stage == DEAD_ADULT )
				stat[S_DEAD_ADULT_1]+=m_scaleFactor;
		}

		
		if( m_swarmingDate==d )
		{
			ASSERT( m_curEmergence>=1 && m_curEmergence<=3);//m_curEmergence is not updated yet	
			ASSERT( m_parentEmergence>=1 && m_parentEmergence<=3);//m_curEmergence is not updated yet	
			stat[S_SWARMING_1_F1_i+(m_parentEmergence-1)*3+m_curEmergence-1]+=m_scaleFactor;
			ASSERT( stage>=ADULT);

			if( m_sex == FEMALE )
				stat[S_TOTAL_FEMALE_1]+=m_scaleFactor;
		}

		//ouput only once
		//if( d.m_month==MAY && d.m_day==30)
		{
			if( m_wStat[NB_VALUE]>0)
				stat[S_W_STAT] = m_wStat[MEAN];

			stat[S_DT_STAT] = m_dTStat[MEAN];
			stat[S_DDL_STAT] = m_dDLStat[MEAN];
			stat[S_DAY_LENGTH] = m_dayLength[SUM]/m_absDiapauseStatI[SUM];
			stat[S_DI50] = (m_DI50Stat[SUM]/m_absDiapauseStatI[SUM]) + stat[S_DDL_STAT];
			stat[S_T_MEAN] = m_TStat[SUM]/m_absDiapauseStatI[SUM];
			stat[S_TI50] = m_TI50Stat[SUM]/m_absDiapauseStatI[SUM] + stat[S_DT_STAT];

			
			ASSERT( stat[S_DAY_LENGTH]>=0 && stat[S_DAY_LENGTH]<=24);
		}


		if( stage != oldStage )
		{
			if(stage==ADULT)
			{
				
			}
			//stat[E_EGG1+stage]+=m_scaleFactor;
			//if( stage == ADULT && m_sex == FEMALE )
				//stat[S_TOTAL_FEMALE_1]+=m_scaleFactor;
		}


		if( d == m_diapauseDate )
			stat[S_DIAPAUSE_1]+=m_scaleFactor;
	}
	/*else if( m_generation==2 )
	{ 
		stat[S_BROOD_2] += m_brood*m_scaleFactor;
		stat[S_TOTAL_BROOD_2] += m_totalBrood*m_scaleFactor;

		if( IsAlive() )
		{
			stat[S_EGG_2+stage]+=m_scaleFactor;
		}
		else 
		{
			if( stage == DEAD_ADULT )
				stat[S_DEAD_ADULT_2]+=m_scaleFactor;
		}

		if( m_swarmingDate==d )
		{
			stat[S_SWARMING_2]+=m_scaleFactor;
			ASSERT( stage>=ADULT);

			if( m_sex == FEMALE )
				stat[S_TOTAL_FEMALE_2]+=m_scaleFactor;
		}


		if( d == m_diapauseDate )
			stat[S_DIAPAUSE_2]+=m_scaleFactor;

		//if( stage != oldStage )
		//{
		//	//stat[E_EGG1+stage]+=m_scaleFactor;
		//	if( stage == ADULT && m_sex == FEMALE )
		//		stat[S_TOTAL_FEMALE_1]+=m_scaleFactor;
		//}
	}*/
	else if(m_generation==2 )
	{
		if( IsAlive() )
		{
			stat[S_EGG_2+stage]+=m_scaleFactor;
		}

		//if( stage != oldStage )
		//{
			//stat[E_EGG2+stage]+=m_scaleFactor;
			//if( stage == ADULT && m_sex == FEMALE )
				//stat[S_TOTAL_FEMALE_2]+=m_scaleFactor;
		//}

		
	}

	


	if( !IsAlive() )
	{
		stat[S_DEAD]+=m_scaleFactor;

		if(m_death==ATTRITION )
			stat[S_DEAD_ATTRITION]+=m_scaleFactor;
		else if(m_death==FROZEN )
			stat[S_DEAD_FROZEN]+=m_scaleFactor;
		//else if(m_death==MISSING_ENERGY)
		//	stat[S_DEAD_MISSING_ENERGY]+=m_scaleFactor;
		//else if( m_death==ASYNCHRONY )
		//	stat[S_DEAD_SYNCH]+=m_scaleFactor;
		//else if( m_death==WINDOW )
		//	stat[S_DEAD_WINDOW]+=m_scaleFactor;
		
	}
}


bool CSpruceBarkBeetle::CanPack(const CBug* In)const
{
	const CSpruceBarkBeetle* in = static_cast<const CSpruceBarkBeetle*>(In);
	return CBug::CanPack(in) && GetStage()<TENERAL_ADULT && m_parentEmergence == in->m_parentEmergence && m_nbEmergence == in->m_nbEmergence && m_curEmergence == in->m_curEmergence;
}

void CSpruceBarkBeetle::Pack(const CBug* In)
{
	const CSpruceBarkBeetle* in = static_cast<const CSpruceBarkBeetle*>(In);
	ASSERT( m_parentEmergence == in->m_parentEmergence );

	m_broodIndex = (m_broodIndex*m_scaleFactor + in->m_broodIndex*in->m_scaleFactor)/(m_scaleFactor + in->m_scaleFactor);

	CBug::Pack(In);
}

//***************************************************************************************************************


//WARNING: cast must be defined here to avoid bug
CSpruceBarkBeetleTree* CSpruceBarkBeetle::GetTree(){ return static_cast<CSpruceBarkBeetleTree*>(m_pHost);}
const CSpruceBarkBeetleTree* CSpruceBarkBeetle::GetTree()const{ return static_cast<const CSpruceBarkBeetleTree*>(m_pHost);}
CSpruceBarkBeetleStand* CSpruceBarkBeetle::GetStand(){  ASSERT(m_pHost); return (CSpruceBarkBeetleStand* )m_pHost->GetStand(); }
const CSpruceBarkBeetleStand* CSpruceBarkBeetle::GetStand()const{  ASSERT(m_pHost); return (const CSpruceBarkBeetleStand* )m_pHost->GetStand(); }


CSpruceBarkBeetleStand::CSpruceBarkBeetleStand(CBioSIMModelBase* pModel):CStand(pModel)
{
	m_bApplyAttrition=true;
	m_bApplyWinterMortality=true;
	//m_bApplyAsynchronyMortality=true;
	//m_bApplyWindowMortality=true;
	m_bFertilEgg=false;
	m_survivalRate=1;
	//m_defoliation=0.5;
		
	m_longDayLength=-1;
	m_firstStageShortDay=-1;
	m_lastStageShortDay=-1;
	m_shortDayLength=-1;
	m_nbShortDay=-1;
	m_nbReemergeMax=3;


	const CWeather& weather = GetModel()->GetWeather();
	//m_lastSnow.resize( weather.GetNbYear() );

	
	for(int y=0; y<weather.GetNbYear(); y++)
	{
		m_lastSnow[weather[y].GetYear()] = weather[y].GetLastSnowDay(2,5);
		m_firstSnow[weather[y].GetYear()] = weather[y].GetFirstSnowDay(2,5);
		m_DD5 += weather[y].GetDD(5.0)/weather.GetNbYear();
	}
	//m_lastSnow = wDay.GetParent()->GetParent()->GetLastSnowDay(1, 2);
	
	m_P2Prob.fill(-1);
}

void CSpruceBarkBeetleStand::SetTree(CSpruceBarkBeetleTree* pTree)
{
	m_pTree.reset(pTree); 
	m_pTree->SetStand(this);
}




void CSpruceBarkBeetleStand::GetStat(CTRef d, CModelStat& stat, int generation)
{
	GetTree()->GetStat(d, stat, generation);

	//double meanAge = stat[S_NB_BUGS]>0?stat[S_AGE]/stat[S_NB_BUGS]:-9999;
	//if( !m_nbShortDayBegin.IsInit() && meanAge>=m_firstStageShortDay && m_nbShortDay>0)
	//{
	//	m_nbShortDayBegin = d;
	//}

	
}



double CSpruceBarkBeetleStand::GetP2AutomnProbability(int sex)
{
	if( m_P2Prob[0]==-1)
	{
		double nbMale=0;
		double nbFemale=0;
		double nbSummerMale = 0;
		double nbSummerFemale = 0;

		const CSpruceBarkBeetleVector& bugs = GetTree()->GetBugs();
		for( CSpruceBarkBeetleVector::const_iterator it=bugs.begin(); it!=bugs.end(); it++)
		{
			if( (*it)->GetGeneration() == 0)
			{
				if( (*it)->GetSex() == CBug::MALE)
				{
					nbMale+=(*it)->GetScaleFactor();
					if((*it)->m_curEmergence>=2)
						nbSummerMale+=(*it)->GetScaleFactor();
				}
				else 
				{
					nbFemale+=(*it)->GetScaleFactor();
					if((*it)->m_curEmergence>=2)
						nbSummerFemale+=(*it)->GetScaleFactor();
				}
			}
		}

		if(nbSummerFemale==0)
			nbSummerFemale=0.1;
		
		//double x = nbSummerFemale/nbSummerMale;
		//double y = Max(0.1, Min(2.5, -0.88418*log(x) + 0.5783));
		double nbAutomnFemale = max(0.0, 1.85*nbSummerMale- 1.15*nbSummerFemale);
		double nbAutomnMale = 0.1*nbAutomnFemale;
		

		m_P2Prob[0] = Max(0, Min(1, nbAutomnMale/(nbMale-nbSummerMale)));
		m_P2Prob[1] = Max(0, Min(1, nbAutomnFemale/(nbFemale-nbSummerFemale)));
	}


	return (sex==CBug::MALE)?m_P2Prob[0]:m_P2Prob[1];
}
