//*****************************************************************************
// File: WSBRates.h
//
// Class: CSpruceBarkBeetleOhrn
//          
//
// Description: the CSpruceBarkBeetleOhrn represent one western spruce budworm insect. 

//
//*****************************************************************************
// 25/09/2013   Rémi Saint-Amant    Creation 
//*****************************************************************************

#include "SpruceBarkBeetleOhrn.h"

#include "UtilMath.h"
#include "Weather.h"
#include <math.h>
#include "Evapotranspiration.h"

using namespace CFL;
using namespace std;


static const double FLIGHT_DD_SUM = 33.4;
static const double FLIGHT_DD_THRESHOLD = 5.0; // DD >5°C, Öhrn 2014

//******************************************************************
//CSpruceBarkBeetleOhrnTree class

/*CSpruceBarkBeetleOhrnTree::CSpruceBarkBeetleOhrnTree( CStand* pStand):CSpruceBarkBeetleOhrnTreeBase(pStand)
{
	Reset();
}

void CSpruceBarkBeetleOhrnTree::Reset()
{
	CSpruceBarkBeetleOhrnTreeBase::Reset();

	//m_defoliation=0.5;
	//m_bEnergyLoss=true;
	//m_ddays=0;
	//m_probBudMineable=0;
	//m_ddShoot=0;
}

void CSpruceBarkBeetleOhrnTree::HappyNewYear()
{
	CSpruceBarkBeetleOhrnTreeBase::HappyNewYear();

	//m_ddays=0;
	//m_probBudMineable=0;
	//m_ddShoot=0;
}


class CPOverheat : public COverheat
{
public:
	
	virtual void TransformWeather(CWeatherDay& weaDay)const
	{
		ASSERT( weaDay[DAILY_DATA::TMIN] > -999 && weaDay[DAILY_DATA::TMAX] > -999);

		double Tmin   = weaDay.GetTMin();
		double Tmax   = weaDay.GetTMax();
		double Trange = weaDay.GetTRange();
		double Sin    = sin(2*3.14159*(weaDay.GetJDay()/365. -0.25));

		//convert air temperature to bark temperature
		weaDay(DAILY_DATA::TMIN)=-0.1493 + 0.8359*Tmin + 0.5417*Sin + 0.16980*Trange + 0.00000*Tmin*Sin + 0.005741*Tmin*Trange + 0.02370*Sin*Trange;
		weaDay(DAILY_DATA::TMAX)= 0.4196 + 0.9372*Tmax - 0.4265*Sin + 0.05171*Trange + 0.03125*Tmax*Sin - 0.004270*Tmax*Trange + 0.09888*Sin*Trange;

		if( weaDay(DAILY_DATA::TMIN) > weaDay(DAILY_DATA::TMAX) )
			CFL::Switch( weaDay(DAILY_DATA::TMIN), weaDay(DAILY_DATA::TMAX) );
	}
};

void CSpruceBarkBeetleOhrnTree::Live(const CTRef& day, const CWeatherDay& weaDay)
{
	//For optimisation, nothing happens when temperature is under -10
	if( day.GetJDay() != 0 && weaDay.GetTMax() < -10)
		return;

	//enlever temporairement
	CPOverheat overheat;
	//double overheat=0;

	CDailyWaveVector hVector;// hourly temperature array 
	weaDay.GetAllenWave(hVector, 16, gTimeStep, overheat );//16 is a factor of 4. Number usually used

	CSpruceBarkBeetleOhrnTreeBase::Live(hVector, weaDay);
}


void CSpruceBarkBeetleOhrnTree::GetStat(CTRef d, CModelStat& stat, int generation)
{
	CSpruceBarkBeetleOhrnTreeBase::GetStat(d, stat);
	
//	CSpruceBarkBeetleOhrnStat& SBStat = (CSpruceBarkBeetleOhrnStat& )stat; 
	//stat[S_AVERAGE_INSTAR] = SBStat.GetAverageInstar();
//  This line should be restored to work on the L2 synchrony test runs
	//stat[S_P_MINEABLE] = m_ddays;//m_probBudMineable;
//  This line should be restored to work on the L6 window test runs
	//stat[S_SHOOT_DEVEL] = m_ddShoot;
	
}
*/

//******************************************************************
//CSpruceBarkBeetleOhrn class

//static CSpruceBarkBeetleOhrnDevelopmentTable RATES;

//bool CSpruceBarkBeetleOhrn::AUTO_BALANCE_EGG=false;
//const double CSpruceBarkBeetleOhrn::EGG_FREEZING_POINT=-10;
//const double CSpruceBarkBeetleOhrn::ADULT_FREEZING_POINT=-10;

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
CSpruceBarkBeetleOhrn::CSpruceBarkBeetleOhrn(CHost* pHost, CTRef creationDate, double age, bool bFertil, int generation, double scaleFactor):
	CBug(pHost, creationDate, age, bFertil, generation, scaleFactor)
{
//	m_bSwarming=false;
	m_hibernationDD;
	m_curEmergence=0;
	m_parentEmergence=0;

	

	m_nbEmergence = 1;
	m_hibernationDD=0;
	m_reemergenceDD=0;
	m_flightDD=0;
	m_ovipositionAge=0;
	m_lastDayLength=-1;
	m_reemergenceDDRequerd[0]=0;
	m_reemergenceDDRequerd[1]=0;
	
	
	//A creation date Bis assigned to each individual
	//Individual's "relative" development rate for each life stage
	//These are independent in successive life stages
	AssignRelativeDevRate();

	//m_nbDayTeneral=-1;

	if( age==ADULT && creationDate.GetJDay()==0)
	{
		
		m_ecdysisDate=CTRef( creationDate.GetYear()-1, SEPTEMBER, 14);
		//m_swarmingDate=m_ecdysisDate+16;
		m_diapauseDate=m_ecdysisDate+16;
	}


	//bool m_bGroundOverwinter = GetModel()->GetInfo().m_loc.m_lat>GetStand()->m_p[4];
	
}

// Object destructor
CSpruceBarkBeetleOhrn::~CSpruceBarkBeetleOhrn(void)
{
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
void CSpruceBarkBeetleOhrn::AssignRelativeDevRate()
{
//	for(int s=0; s<NB_STAGES; s++)
		//m_relativeDevRate[s] = 1;
	//	m_relativeDevRate[s] = CSBBDevelopmentTable::GetRelativeRate(RandomGenerator(), s, m_sex);

	//m_relativeOviposition = CSBBOviposition::GetRelativeRate(RandomGenerator(), CSBBOviposition::DEVELOPEMENT_RATE); 
	//m_relativeFecondity = CSBBOviposition::GetRelativeRate(RandomGenerator(), CSBBOviposition::BROOD_RATE); //fecundity is determined at moult from L5 to L6, based on L6 survival probability (in CSpruceBarkBeetleOhrn::IsDeadByWindow(double T, double dt) )
	
	//Petter Ohrn: Seasonal flight patterns of the Spruce Barck Beetle (PDS Typographus) in Sweden
	//static const double RE_EMERGENCE_VARIANCE[3]={Square(19), Square(46), Square(107)};
	//for(int i=0; i<3; i++)
	//m_relativeEmergence[i] = RandomGenerator().RandLogNormal(0, RE_EMERGENCE_VARIANCE[i]);

	
	//m_hibernationDDrequired = GetStand()->m_p[0];
	//m_hibernationDDrequired = RandomGenerator().RandNormal(563.3, 19.2)
		//while( m_hibernationDDrequired<=563.3/2 || m_hibernationDDrequired>563.3*2)
			//m_hibernationDDrequired = RandomGenerator().RandNormal(563.3, 19.2)

	//NbVal=   166	Bias=-0.03348	MAE= 6.19020	RMSE= 7.82964	CD= 0.81773	R²= 0.82196


	//

//	double p1[4]={GetStand()->m_p[0],GetStand()->m_p[1],GetStand()->m_p[2],GetStand()->m_p[3]};
	//double p1[4]={475.6, 87.0, 24.6,  2.2};
	//m_hibernationDDrequired = RandomGenerator().RandNormal(p1[0], p1[1]);
/*
	CStdStdioFile file;
	file.Open("D:\\TestLogNormal.csv", CStdFile::modeWrite|CStdFile::modeCreate);

	for(int i=0; i<10000; i++)
	{
		double v = RandomGenerator().RandLogNormal(log(20.0), 0.15);
		if( v<16.8 || v>30)
		{
			v = RandomGenerator().RandLogNormal(log(20.0), 0.15);
			if( v<16.8 || v>30)
				v = RandomGenerator().RandLogNormal(log(20.0), 0.15);
		}

		file.WriteString(stdString::ToString(v) +"\n");
	}

	file.Close();
*/	
	m_hibernationDDrequired = RandomGenerator().RandLogNormal(3.692348, 0.5889347);
	
	while( m_hibernationDDrequired)<47/2 || log(m_hibernationDDrequired)>3.692348*2)
		m_hibernationDDrequired = RandomGenerator().RandLogNormal(3.692348, 0.5889347);
		
	

	//m_springSwarmingT = RandomGenerator().RandNormal(p1[2], p1[3]);
	
	m_springSwarmingT = RandomGenerator().RandLogNormal(log(p1[2]), p1[3]);
	if( m_springSwarmingT<16.8 || m_springSwarmingT>30)
	{
		m_springSwarmingT = RandomGenerator().RandLogNormal(log(p1[2]), p1[3]);
		if( m_springSwarmingT<16.8 || m_springSwarmingT>30)
			m_springSwarmingT = RandomGenerator().RandLogNormal(log(p1[2]), p1[3]);
	}


	//while( m_springSwarmingT<16.8 || m_springSwarmingT>30)
		//m_springSwarmingT = RandomGenerator().RandNormal(p1[2], p1[3]);

	m_emergenceT = p1[2]*RandomGenerator().RandNormal(0, p1[3]);
	if( m_emergenceT<16.8 || m_emergenceT>30)
		m_emergenceT = p1[2]*RandomGenerator().RandNormal(0, p1[3]);

	//while( m_emergenceT<16.8 || m_emergenceT>30)
		//m_emergenceT = RandomGenerator().RandNormal(p1[2], p1[3]);

	//Equation from Anderbrant 1985
	//double low= 0.845-1.2547;
	//double hight=1;
	double R1 = RandomGenerator().Rand(0,1);
	
	//B. Wermelinger, M. Seifert (1999), table 1.
	//if( RandomGenerator().Randu()<0.6)
	if(R1<0.8449 && GetStand()->m_nbReemergeMax>=2)
	{
		m_nbEmergence++;

		double p2[4]={GetStand()->m_p[0],GetStand()->m_p[1],GetStand()->m_p[2],GetStand()->m_p[3]};
		double D2 = pow(log((R1-0.845)/(-1.2547))/-1.3909,1/1.69);
		m_reemergenceDDRequerd[0]= p2[1]*D2;
		

		//if( RandomGenerator().Randu()<0.666666)//est-ce 40% de 60%. Je ne crois pas
		double R2 = RandomGenerator().Randu();
		if(R2<0.8449/2&& GetStand()->m_nbReemergeMax>=3)
		{
			double D3 = pow(log((R2-0.845/2)/(-1.2547))/-1.3909,1/1.69);
			m_reemergenceDDRequerd[1]=p2[1]*D3;
			m_nbEmergence++;
		}


		m_reemergenceT=p2[2];
	}

	//const static double test[11] = {0.38,0.52,0.66,0.74,0.82,0.89,0.98,1.08,1.18,1.40,2.10};



	//m_hibernationDDrequired = RandomGenerator().RandNormal(GetStand()->m_p[0], GetStand()->m_p[1]);
	//while( m_hibernationDDrequired<=250 || m_hibernationDDrequired>1000)
	//	m_hibernationDDrequired = RandomGenerator().RandNormal(GetStand()->m_p[0], GetStand()->m_p[1]);

	//m_springSwarmingT = RandomGenerator().RandNormal(GetStand()->m_p[2], GetStand()->m_p[3]);
	//while( m_springSwarmingT<16.8 || m_springSwarmingT>26.2)//16.?
	//	m_springSwarmingT = RandomGenerator().RandNormal(GetStand()->m_p[2], GetStand()->m_p[3]);


	m_relativeWingsMaturation = RandomGenerator().RandNormal(12.5, 1.2);
	while( m_relativeWingsMaturation<9 || m_relativeWingsMaturation > 165)
		m_relativeWingsMaturation = RandomGenerator().RandNormal(12.5, 1.2);
}




//*****************************************************************************
// Live is for one day development
//
// Input: CDailyWaveVector T: the temperature for all time step
//
// Output: return true if the bug continue to be in the population, false otherwise
//*****************************************************************************
void CSpruceBarkBeetleOhrn::Live(const CDailyWaveVector& T, const CWeatherDay& wDay)
{
	if( IsCreated(T.GetFirstTRef()) ) 
	{
		if(T.GetFirstTRef() == m_swarmingDate+1)
		{
			m_curEmergence++;
			m_ovipositionAge=0;
			m_reemergenceDD=0;
			m_flightDD = 0;
			
//			m_rebeginEating.Reset();
			//m_bSwarming=false;
		}

		//CDay day;
		
		CBug::Live(T, wDay);

		//double z = GetStand()->GetModel()->GetInfo().m_loc.m_elev;
		//double lat = GetStand()->GetModel()->GetInfo().m_loc.m_lat;
		//int J = T.GetFirstTRef().GetJDay()+1;//Julian day
		//double Rs = wDay[DAILY_DATA::SRAD];
		//double Ra = CASCE2005PET::GetExtraterrestrialRadiation(lat, J);
		//double Rso = CASCE2005PET::GetClearSkySolarRadiation (Ra, z);
		//double Fcd = CASCE2005PET::GetCloudinessFunction (Rs, Rso);
		//double Rns = CASCE2005PET::GetNetSolarRadiation(Rs);
		//double Ea = CASCE2005PET::GetActualVaporPressure(wDay[DAILY_DATA::RELH], wDay[DAILY_DATA::TMIN], wDay[DAILY_DATA::TMAX]);
		//double Rnl = CASCE2005PET::GetNetLongWaveRadiation(wDay[DAILY_DATA::TMIN], wDay[DAILY_DATA::TMAX], Ea, Fcd);
		//double Rn = CASCE2005PET::GetNetRadiation(Rns, Rnl);

		//CWeatherDay& wD = const_cast<CWeatherDay&>(wDay);
		//wD(DAILY_DATA::ADD1) = Rn;
		
		
		//Development, reproduction and mortality
		//&&(m_swarmingDate!=T.GetFirstTRef())
		for(int h=0; h<(int)T.size()&&IsAlive(); h++)
			Develop(T.GetFirstTRef(), T[h], wDay, T.NbStep());
	}
}

//void CSpruceBarkBeetleOhrn::Brood(const CDailyWaveVector& T, const CWeatherDay& wDay)
//{
//	
//	if( GetStage() == ADULT && m_sex == FEMALE )
//	{
//		CSpruceBarkBeetleOhrnTree* pTree = (CSpruceBarkBeetleOhrnTree*) m_pHost;
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
//				pTree->AddBug( new CSpruceBarkBeetleOhrn(pTree, T.GetFirstTRef(), EGG, true, m_generation+1, m_scaleFactor) );
//			}
//
//			//std::vector<int>::push_back(
//			m_nbEgg++;
//		}
//	}
//}

void CSpruceBarkBeetleOhrn::Brood(const CDailyWaveVector& T, const CWeatherDay& weaDay)
{
	if( m_bFertil && m_brood>0)
	{
		//When the bugs are killed by tree at the end of the day, they must not create eggs
		CSpruceBarkBeetleOhrnTree* pTree = GetTree();
		CSpruceBarkBeetleOhrnStand* pStand = GetStand(); ASSERT( pStand );
		
		//attrition turned off,
		double scaleFactor = m_brood*pStand->m_survivalRate*m_scaleFactor;
		double scaleFactorAttrition = m_brood*(1-pStand->m_survivalRate)*m_scaleFactor;
		ASSERT(scaleFactor>0);

		CSpruceBarkBeetleOhrn* pBug = new CSpruceBarkBeetleOhrn(pTree, T.GetFirstTRef(), EGG, pStand->m_bFertilEgg, m_generation+1, scaleFactor);
		pBug->m_parentEmergence = m_curEmergence;
		pTree->AddBug(pBug);

		if( scaleFactorAttrition>0)
		{
			CSpruceBarkBeetleOhrn* pBug = new CSpruceBarkBeetleOhrn(pTree, T.GetFirstTRef(), EGG, pStand->m_bFertilEgg, m_generation+1, scaleFactorAttrition);
			pBug->m_status = DEAD;
			pBug->m_death = ATTRITION;
			pTree->AddBug(pBug);
		}
	}
}


void CSpruceBarkBeetleOhrn::ComputetDiapause(int s, double T, double DL)
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

void CSpruceBarkBeetleOhrn::ComputetSwarming(int s, double T, double DL)
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
void CSpruceBarkBeetleOhrn::Develop(CTRef date, double T, const CWeatherDay& wDay, short nbStep)
{
	//Develops all individuals, including ovipositing adults
	_ASSERTE(m_status==HEALTHY);

	int s = GetStage();
	const CSBBDevelopmentTable& rates = GetStand()->m_rates;
	double dayLength = GetModel()->GetInfo().m_loc.GetDayLength(date)/3600;

	//for calibration purpose
	if( GetStand()->m_longDayLength>=0 )
	{
		dayLength = GetStand()->m_longDayLength;
		//if( s>=GetStand()->m_firstStageShortDay )//&& s<=GetStand()->m_lastStageShortDay)

		if( GetStand()->m_nbShortDayBegin.IsInit() && date-GetStand()->m_nbShortDayBegin<=GetStand()->m_nbShortDay)
		{
			dayLength = GetStand()->m_shortDayLength;
		}
	}

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
		//	ComputetSwarming(s, T, dayLength);
		//}
		//else 
		//{
		//	ComputetDiapause(s, T, dayLength);
		//	m_wStat += d*GetStand()->m_s[s]/sumS;
		//	//m_dDLStat += d_DL*GetStand()->m_s[s]/sumS;
		//}
	}
	*/



	//if( s<=TENERAL_ADULT && !m_diapauseDate.IsInit() && !m_swarmingDate.IsInit() )
	//{
	//	ComputetDiapause(s, T, dayLength);
	//	ComputetSwarming(s, T, dayLength);
	//}

	double TAF = 1;
	double dailyR = rates.GetRate(s,T);

	/*
	//no modification of the devel rate for the moment
	if( s==TENERAL_ADULT && !m_diapauseDate.IsInit() && !m_swarmingDate.IsInit())
	{
		//diapause computation
		double dT = m_dTStat[MEAN];
		double dDL = m_dDLStat[MEAN];

		//swarming computation
		double rate = m_RStat[MEAN];
		double taf = m_TAFStat[MEAN];

		if( dT>0 || dDL>0.46431)
		{
			//double K2 = GetStand()->m_k[2];
			//double K3 = GetStand()->m_k[3];
			//double K4 = GetStand()->m_k[4];
			double K2 = 3.04991;
			double K3 = 0.30004;
			double K4 = 0.20939;
			
			if( dailyR>0)
				dailyR = Max(0, Min(1, K2*(K3*dailyR + (1-K3)*rate) + K4 ));

			//double S6 = GetStand()->m_s[6];
			double S6 = 1.75665;
			TAF =Max(0, Min(1, S6*taf));

			m_wStat += dailyR*TAF;
		}
		else
		{
			//if(m_nbDayTeneral==-1)
				//m_nbDayTeneral = 150;

			//m_diapauseDate = date;
		}

		int TADays = (date-m_ecdysisDate);
		if( TADays>=14 )
		{
			//if the expected length after one week is more than 115 days, we diapause
			//
			double P4 = 117;
			//if( GetRelDev()== 0 || TADays/GetRelDev()>=P4)
				//m_diapauseDate = date;
		}
	}
	*/

	if( s==ADULT && !m_diapauseDate.IsInit() && !m_swarmingDate.IsInit())
	{
		int TADays = (date-m_swarmingDate);
		if( TADays>=7 )
		{
			//if the expected length after one week is more than 85 days, we diapause
			//if( m_ovipositionAge== 0 || TADays/m_ovipositionAge>=40)
				//m_diapauseDate = date;
		}
	}
	
	double RR = TAF*m_relativeDevRate[s]*dailyR/nbStep;
	_ASSERTE(RR >=0 );

	
	double K[5] = {GetStand()->m_k[0],GetStand()->m_k[1],GetStand()->m_k[2],GetStand()->m_k[3],GetStand()->m_k[4]};
	double S[5] = {GetStand()->m_s[0],GetStand()->m_s[1],GetStand()->m_s[2],GetStand()->m_s[3],GetStand()->m_s[4]};
	//double K[4] = {13.70000,  6.04439,  4.60000,  1.82000};
	//double S[4] = {17.64551,  7.41664,  1.33799, 11.90000};


	//swarming of adults
	if( s==ADULT && date!=m_swarmingDate)//if not the swarming day
	{
		if((date-m_ecdysisDate)<16)
			RR=0;

	
		if( m_diapauseDate.IsInit() && !m_awakeDate.IsInit() )
		{

			//insects are in diapause
//	NbVal=   142	Bias= 0.38647	MAE= 6.51167	RMSE= 8.50807	CD= 0.68203	R²= 0.72677
//double p[3] = {509.30314 , 48.80000, 21.64436,  0.94057  };
//double k[5] = {15.63275,  9.50000 ,   2.10000,  0.45000 , 11.30000  };
//double s[3] = {2.20000  ,  9.61172 ,  1.50645  };

			


			//double k[5] = {15.5, 7.1,-2.7, 4.93,12.2};
			//double s[4] = {8.4,8.6,0.32,7.1};
			RR=0;

			if(m_lastDayLength==-1)
				m_lastDayLength=dayLength;

			if( !m_lastSnow.IsInit() || wDay[DAILY_DATA::SNDH]>0 ) 
				m_lastSnow = date;

			if( dayLength >= K[4] && date>m_lastSnow+(int)S[4] )
			{
				double ΔDL = dayLength - K[0];
				double ΔT1 = K[1]*(1/(1+exp(-K[2]*ΔDL))-0.5);
				
				double ΔRad = wDay[DAILY_DATA::SRAD] - S[0];
				double ΔT2 = S[1]*(1/(1+exp(-S[2]*ΔRad)));
				
				double TT = T+ΔT1+ΔT2;
				
				m_hibernationDD += Max(0, TT-K[3])/nbStep;

				if( m_hibernationDD					>= m_hibernationDDrequired && 
					(wDay[DAILY_DATA::TMAX]/*+ΔT1+ΔT2*/)>= m_springSwarmingT && 
					wDay[DAILY_DATA::PRCP] < S[3]
					) 
				{
					m_awakeDate=date;
					m_swarmingDate=date;
				}
			}
		}
		else
		{
			//insect is not in diapause: look for swarming
			if( RR>0 )
			{
				//if(m_curEmergence<m_nbEmergence)
				//{
				if(m_curEmergence==0 )
				{
					//F1
					//double k[5] = {GetStand()->m_k[0],GetStand()->m_k[1],GetStand()->m_k[2],GetStand()->m_k[3],GetStand()->m_k[4]};
					//double s[4] = {GetStand()->m_s[0],GetStand()->m_s[1],GetStand()->m_s[2],GetStand()->m_s[3]};
					
						
					double ΔDL = dayLength - K[0];
					double ΔT1 = K[1]*(1/(1+exp(-K[2]*ΔDL)) - 0.5);
				
					double ΔRad = wDay[DAILY_DATA::SRAD] - S[0];
					double ΔT2 = S[1]*(1/(1+exp(-S[2]*ΔRad)));
							
					if( (wDay[DAILY_DATA::TMAX]+ΔT1+ΔT2)>=m_emergenceT && 
						wDay[DAILY_DATA::PRCP] < S[3]
						)
					{
						m_swarmingDate=date;
					}
				}
				else
				{
						
					if(m_ovipositionAge==1)/*GetStand()->m_p[3]*/
					{
						//P2
						//double p[4] = {GetStand()->m_p[0],GetStand()->m_p[1],GetStand()->m_p[2],GetStand()->m_p[3]};
						//double k[5] = {GetStand()->m_k[0],GetStand()->m_k[1],GetStand()->m_k[2],GetStand()->m_k[3],GetStand()->m_k[4]};
						//double s[4] = {GetStand()->m_s[0],GetStand()->m_s[1],GetStand()->m_s[2],GetStand()->m_s[3]};
						//double k[5] = {14.5614, 9.52755, 2.12841, 0.44918, 11.29058};
						//double s[4] = {2.23400  ,9.67805 ,0.50875,11.4  };
						
						double ΔDL = dayLength - K[0];
						double ΔT1 = K[1]*(1/(1+exp(-K[2]*ΔDL)) - 0.5);
				
						double ΔRad = wDay[DAILY_DATA::SRAD] - S[0];
						double ΔT2 = S[1]*(1/(1+exp(-S[2]*ΔRad)));
				
						double TT = T+ΔT1+ΔT2;

						m_reemergenceDD += Max(0, TT-GetStand()->m_k[1])/nbStep;

						if( m_reemergenceDD>=m_reemergenceDDRequerd[m_curEmergence-1] && 
							(wDay[DAILY_DATA::TMAX]+ΔT1+ΔT2)>=m_reemergenceT && 
							wDay[DAILY_DATA::PRCP] < S[3]
							)
						{
							m_swarmingDate=date;
						}
					}
						
				}
			}
			//}
		}
		
	}//adult
	
	m_lastDayLength=dayLength;
	
	//diapause 
	//if( s >= TENERAL_ADULT && !m_awakeDate.IsInit())
	//{
	//	if( m_diapauseDate.IsInit() )
	//	{
	//		//chilling accumulation
	//		//m_diapauseIIStat += w;

	//		double awake = m_awakeStat[MEAN];
	//		//if( awake > GetStand()->m_s[3])
	//			//m_awakeDate = date;
	//			
	//	}
	//	else
	//	{
	//		//look to see if the insect enter in diapause 
	//		//if( m_diapauseStat[MEAN] > 0)
	//			//m_diapauseDate = date;
	//	}
	//}

	
	//m_curEmergence==1 && 
	m_flightDD+=wDay.GetDD(FLIGHT_DD_THRESHOLD);//at 5°C
	if( m_swarmingDate.IsInit() && m_flightDD<FLIGHT_DD_SUM )
		RR=0;//stop development for 2-3 days after swarming: the time to find and attack a tree

	
	//static double MAX_DAILY_R[NB_STAGES] = {0.38409, 0.27291, 0.27291, 0.27291, 0.44908, 0.09975, 0.18292};
	//double vv = dailyR/MAX_DAILY_R[s];
	//m_dailyRStat += vv;
	
	if( s == PUPAE && ChangeStage(RR) )
		m_ecdysisDate = date;


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
			double OAr = m_relativeOviposition*CSBBOviposition::GetRate(CSBBOviposition::DEVELOPEMENT_RATE, T)/nbStep;
			m_ovipositionAge = Min(1, m_ovipositionAge+OAr);

			//ovipositing adult begin brood after 18.2 % of there age
			static const double BEGIN_BROOD = 0.182;
			
			
			
			if( m_ovipositionAge > BEGIN_BROOD)
			{
				//Oviposition rate in function of temperature (Eggs/days)
				double Or = CSBBOviposition::GetRate(CSBBOviposition::BROOD_RATE, T)/nbStep;
			
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
/*	else if( (GetStage() == EGG && (T<CSpruceBarkBeetleOhrn::EGG_FREEZING_POINT||m_eggAge>30)))
	{
		m_status=DEAD;
		m_death=FROZEN;
	}
	else if( (GetStage() == ADULT && T<CSpruceBarkBeetleOhrn::ADULT_FREEZING_POINT) )
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
bool CSpruceBarkBeetleOhrn::IsDeadByAttrition(double T, double r)const
{
	bool bDead=false;
	if( GetStand()->m_bApplyAttrition)
	{
		//Computes attrition (probability of survival in a given time step, based on development rate)
		//double p_survival = pow(CSpruceBarkBeetleOhrnAttrition::GetRate(GetStage(), T), r);
		//double u = RandomGenerator().Randu();
		//if(u>p_survival)
			//bDead=true;

	}

	return bDead;
}


bool CSpruceBarkBeetleOhrn::IsDeadByOverwintering(double T, double dt)
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
		else if( GetStage()<PUPAE && T<-13 )//viiri 2012
		{
			bDead = true;
		}
		else if( GetStage()<PUPAE && T<-13 )//viiri
		{
			bDead = true;
		}
		else if( GetRelDev() < 0.5 && T<-13 )//new emerged adult will die at -13
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
// CSpruceBarkBeetleOhrnStat& stat: the statistic object
//
// Output: The stat is modified
//*****************************************************************************
void CSpruceBarkBeetleOhrn::GetStat(CTRef d, CModelStat& stat)
{
	short oldStage = short(m_lastAge);
	short stage = GetStage();

	if( m_generation==0 )
	{
		//total and daily brood
		stat[S_BROOD_0] += m_brood*m_scaleFactor;
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

		if( m_swarmingDate==d )
		{
			ASSERT( m_curEmergence>=0 && m_curEmergence<3);//m_curEmergence is not updated yet
			stat[S_SWARMING_0_P1+m_curEmergence]+=m_scaleFactor;
			ASSERT( stage>=ADULT);

			if( m_sex == FEMALE )
				stat[S_TOTAL_FEMALE_0]+=m_scaleFactor;
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

			stat[S_AGE] += m_age*m_scaleFactor;
			stat[S_NB_BUGS] += m_scaleFactor;

		}
		else 
		{
			if( stage == DEAD_ADULT )
				stat[S_DEAD_ADULT_1]+=m_scaleFactor;
		}

		if( m_swarmingDate==d )
		{
			ASSERT( m_curEmergence>=0 && m_curEmergence<3);//m_curEmergence is not updated yet
			stat[S_SWARMING_1_F1_i+(m_parentEmergence-1)*3+m_curEmergence]+=m_scaleFactor;
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


bool CSpruceBarkBeetleOhrn::CanPack(const CBug* In)const
{
	const CSpruceBarkBeetleOhrn* in = static_cast<const CSpruceBarkBeetleOhrn*>(In);
	return CBug::CanPack(in)&& m_parentEmergence == in->m_parentEmergence && m_nbEmergence == in->m_nbEmergence && m_curEmergence == in->m_curEmergence;
}


//***************************************************************************************************************


//WARNING: cast must be defined here to avoid bug
CSpruceBarkBeetleOhrnTree* CSpruceBarkBeetleOhrn::GetTree(){ return static_cast<CSpruceBarkBeetleOhrnTree*>(m_pHost);}
const CSpruceBarkBeetleOhrnTree* CSpruceBarkBeetleOhrn::GetTree()const{ return static_cast<const CSpruceBarkBeetleOhrnTree*>(m_pHost);}
CSpruceBarkBeetleOhrnStand* CSpruceBarkBeetleOhrn::GetStand(){  ASSERT(m_pHost); return (CSpruceBarkBeetleOhrnStand* )m_pHost->GetStand(); }
const CSpruceBarkBeetleOhrnStand* CSpruceBarkBeetleOhrn::GetStand()const{  ASSERT(m_pHost); return (const CSpruceBarkBeetleOhrnStand* )m_pHost->GetStand(); }

void CSpruceBarkBeetleOhrnStand::SetTree(CSpruceBarkBeetleOhrnTree* pTree)
{
	m_pTree.reset(pTree); 
	m_pTree->SetStand(this);
}




void CSpruceBarkBeetleOhrnStand::GetStat(CTRef d, CModelStat& stat, int generation)
{
	GetTree()->GetStat(d, stat, generation);

	double meanAge = stat[S_NB_BUGS]>0?stat[S_AGE]/stat[S_NB_BUGS]:-9999;
	if( !m_nbShortDayBegin.IsInit() && meanAge>=m_firstStageShortDay && m_nbShortDay>0)
	{
		m_nbShortDayBegin = d;
	}
}



