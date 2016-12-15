//*****************************************************************************
// File: WSBRates.h
//
// Class: CWSpruceBudworm
//          
//
// Descrition: the CWSpruceBudworm represent one western spruce budworm insect. 
//*****************************************************************************
// 21/01/2016   Rémi Saint-Amant	Update with BioSIM 11.0
// 11/03/2013   Jacques Régnière    Update synchrony equations
// 18/02/2013   Rémi Saint-Amant    Update with new BioSIM Model Base
// 12/10/2012   Rémi Saint-Amant    Update with BioSIM 10.2
// 15/08/2011   Jacques Regniere    Cleanup. Mortality sources restored
// 04/06/2011	Jacques Régnière	Add mineable
// 02/02/2011   Rémi Saint-Amant    Add overheating
// 28/01/2011	Rémi Saint-Amant    Update with new BioSIMModelBase
// 23/01/2009   Rémi Saint-Amant    Creation
//*****************************************************************************

#include <math.h>
#include "basic/UtilMath.h"
#include "basic/WeatherStation.h"
#include "WSpruceBudworm.h"

using namespace WBSF;
using namespace WBSF::HOURLY_DATA;
using namespace std;

namespace WBSF
{

	//******************************************************************
	//CWSpruceBudworm class
	const double CWSpruceBudworm::EGG_FREEZING_POINT = -10;
	const double CWSpruceBudworm::ADULT_FREEZING_POINT = -10;

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
	CWSpruceBudworm::CWSpruceBudworm(CHost* pHost, CTRef creation, double age, size_t sex, bool bFertil, size_t generation, double scaleFactor) :
		CIndividual(pHost, creation, age, sex, bFertil, generation, scaleFactor)
	{
		//A creation date is assigned to each individual
		//Individual's "relative" development rate for each life stage
		//These are independent in successive life stages
		for (size_t s = 0; s < NB_STAGES; s++)
			m_relativeDevRate[s] = CWSBRelativeDevRate::GetRate(s, m_sex);
		
		m_potentialFecundity = 0; //fecundity is determined at moult from L5 to L6, based on L6 survival probability (in CWSpruceBudworm::IsDeadByWindow(double T, double dt) )
		m_eggAge = 0;
		m_bKillByAttrition = false;
		m_bKillByOverwintering = false;

		// Each individual created Initial Energy Level, the same for everyone
		static const double IEL = 4.6833; //SAS Survival.lst
		m_energy = IEL;
		//UniformRandom ]0,1[  not including 0 or 1;
		m_overwinterLuck = RandomGenerator().Randu(true, true); //for overwinter survival
		m_synchLuck = RandomGenerator().Randu(true, true); //random "synchrony luck" of an individual with respect to L2 emergence synchrony
		m_windowLuck = RandomGenerator().Randu(true, true); //random "window luck" of an individual with respect to shoot development
		m_onBole = RandomGenerator().Randu(true, true); //random overwintering location on the tree (branches or bole)
	}

	
	// Object destructor
	CWSpruceBudworm::~CWSpruceBudworm(void)
	{}



	//*****************************************************************************
	// Live is for one day development
	//
	// Input: CDailyWaveVector T: the temperature for all time step
	//
	// Output: return true if the bug continue to be in the population, false otherwise
	//*****************************************************************************
	void CWSpruceBudworm::Live(const CWeatherDay& weather)
	{
		CIndividual::Live(weather);

		double def = GetStand()->m_defoliation; //Defoliation affects distribution of L2o, proportion on bole, buffered
		double prop_onBole = def > 0.1 ? 0.7 : 0.4;

		bool onBranch = (m_onBole>prop_onBole); //Decision as to where individual overwinters
		double Tmean = weather[H_TNTX][MEAN];

		size_t nbSteps = GetTimeStep().NbSteps();
		for (size_t step = 0; step < nbSteps&&m_age < DEAD_ADULT; step++)
		{
			//Develops all individuals, including ovipositing adults
			ASSERT(m_status == HEALTHY);

			size_t h = step*GetTimeStep();
			size_t s = GetStage();
			double T = weather[h][H_TAIR2];


			const CWSBTableLookup& equations = Equations();
			double RR = m_relativeDevRate[s] * equations.GetRate(s, T) / nbSteps;
			ASSERT(RR >= 0);

			//Postdiapause development of wSBW starts accumulating on Julian day 60 (1 March).
			if (s == L2o && weather.GetTRef().GetJDay() < 60)
				RR = 0.0;

			if (s == EGG && ChangeStage(RR))
				m_OWDate = weather.GetTRef();

			if (s == EGG)
				m_eggAge += 1.0 / nbSteps;

			//If we became OVER_WINTER this year, then we stop the 
			//developement until the next year
			if (!((s == L2o && m_OWDate.GetYear() == weather.GetTRef().GetYear()) || (s == L3 && m_generation == 1)))
				m_age += RR;

			if (IsDeadByAttrition(T, RR))
				m_bKillByAttrition = true;
			
			if (IsDeadByOverwintering(onBranch?T:Tmean, 1.0 / nbSteps))
				m_bKillByOverwintering = true;
		}
	}

	void CWSpruceBudworm::Brood(const CWeatherDay& weather)
	{
		ASSERT(IsAlive() && m_sex == FEMALE);
		
		CIndividual::Brood(weather);
		//Each day t, at temperature T
		//Females begin oviposition after 1 day out of 15 at 20 C, 1 days out of 30 at 12 C: 6.7% of lifespan
		if (m_age >= ADULT + 0.067)
		{
			_ASSERTE(IsAlive());

			double T = weather[H_TNTX][MEAN];

			double eggLeft = m_potentialFecundity - m_totalBroods;
			double broods = eggLeft*CWSBOviposition::GetRate(T, eggLeft);
			//Don't apply survival here. Survival must be apply in brouding
			m_broods += broods;
			m_totalBroods += broods;


			if (m_bFertil && m_broods > 0)
			{
				//When the bugs are killed by tree at the end of the day, they must not create eggs
				CWSBStand* pStand = GetStand(); ASSERT(pStand);

				//attrition turned off,
				double scaleFactor = m_broods*pStand->m_survivalRate*m_scaleFactor;
				ASSERT(scaleFactor > 0);

				CIndividualPtr pBug = make_shared<CWSpruceBudworm>(GetHost(), weather.GetTRef(), EGG, NOT_INIT, pStand->m_bFertilEgg, m_generation + 1, scaleFactor);
				GetHost()->push_front(pBug);
			}
		}
	}

	//*****************************************************************************
	// Develop is for one time step development
	// Input: double T: is the temperature for one time step
	//        short nbStep: is the number of time steps per day (24h/step duration(h) )
	//*****************************************************************************
	
	void CWSpruceBudworm::Die(const CWeatherDay& weather)
	{
		size_t s = GetStage();
		bool bLookAsynchrony = (s == L2 ) && IsChangingStage();
		bool bLookWindow = (s == L5) && IsChangingStage();
		CTRef TRef = weather.GetTRef();
		
		
		//Mortality
		if (GetStage() == DEAD_ADULT)
		{
			m_status = DEAD;
			m_death = OLD_AGE;
		}
		else if ((GetStage() == EGG && (weather[H_TMIN2][MEAN] < CWSpruceBudworm::EGG_FREEZING_POINT || m_eggAge>30)))
		{
			m_status = DEAD;
			m_death = FROZEN;
		}
		else if ((GetStage() == ADULT && weather[H_TMIN2][MEAN] < CWSpruceBudworm::ADULT_FREEZING_POINT))
		{
			m_age += 1;
			m_status = DEAD;
			m_death = OLD_AGE;
		}
		else if (m_bKillByAttrition)
		{
			m_status = DEAD;
			m_death = ATTRITION;
		}
		else if (bLookAsynchrony && IsDeadByAsynchrony())
		{
			//Bug dies if its synchrony luck is greater than the synchrony survival value at the time of its emergence from ovewintering
			m_status = DEAD;
			m_death = ASYNCHRONY;
		}
		else if (bLookWindow && IsDeadByWindow())
		{
			//Bug dies if its Window luck is greater than the Window survival value at the time of its moult to the L6
			m_status = DEAD;
			m_death = WINDOW;
		}
		else if (m_bKillByOverwintering)
		{
			m_status = DEAD;
			m_death = MISSING_ENERGY;
		}
		else if (GetStage() != L2o && weather[H_TMIN2][MEAN] < -10)
		{
			//all non l2o are kill by frost under -10°C
			m_status = DEAD;
			m_death = OTHERS;
			//			m_death = FROZEN;
		}
		else if (weather.GetTRef().GetMonth() == DECEMBER && weather.GetTRef().GetDay()==30)
		{
			m_status = DEAD;
			m_death = OTHERS;
		}
	}



	//*****************************************************************************
	// GetStat: GetStat is call daily to get ther states of the object
	//
	// Input: CTRef d: the actual day
	// CWSBStat& stat: the statistic object
	//
	// Output: The stat is modified
	//*****************************************************************************
	void CWSpruceBudworm::GetStat(CTRef d, CModelStat& stat)
	{
		size_t oldStage = size_t(m_lastAge);
		size_t stage = GetStage();

		if (m_generation == 0)
		{
			//total and daily brood
			stat[S_BROOD] += m_totalBroods*m_scaleFactor;
			stat[E_BROOD] += m_broods*m_scaleFactor;

			if (IsAlive())
			{
				stat[S_EGG + stage] += m_scaleFactor;

				if (stage == ADULT && m_sex == FEMALE)
					stat[S_OVIPOSITING_ADULT] += m_scaleFactor;

			}
			else
			{
				if (stage == DEAD_ADULT)
					stat[S_DEAD_ADULT] += m_scaleFactor;
			}

			if (stage != oldStage)
			{
				stat[E_EGG + stage] += m_scaleFactor;
				if (stage == ADULT && m_sex == FEMALE)
					stat[E_TOTAL_FEMALE] += m_scaleFactor;
			}

		}
		else if (m_generation == 1)
		{
			if (stage >= EGG && stage <= L3)
			{
				if (IsAlive())
					stat[S_EGG2 + stage] += m_scaleFactor;

				if (stage != oldStage)
					stat[E_EGG2 + stage] += m_scaleFactor;
			}
		}

		if (!IsAlive())
		{
			stat[S_DEAD] += m_scaleFactor;

			if (m_death == ATTRITION)
				stat[S_DEAD_ATTRITION] += m_scaleFactor;
			else if (m_death == FROZEN)
				stat[S_DEAD_FROZEN] += m_scaleFactor;
			else if (m_death == MISSING_ENERGY)
				stat[S_DEAD_MISSING_ENERGY] += m_scaleFactor;
			else if (m_death == ASYNCHRONY)
				stat[S_DEAD_SYNCH] += m_scaleFactor;
			else if (m_death == WINDOW)
				stat[S_DEAD_WINDOW] += m_scaleFactor;

		}
	}

	double CWSpruceBudworm::GetInstar(bool includeLast)const
	{ 
		double AI = CBioSIMModelBase::VMISS;
		if (IsAlive() || m_death == OLD_AGE)
		{
			ASSERT(L2o == 1);
			AI = std::min(m_age < L2o ? m_age : max(2.0, m_age), double(NB_STAGES) - (includeLast ? 0.0 : 1.0));
		}
		return AI; 
	}
	//*****************************************************************************
	// IsDeadByAttrition is for one time step development
	//
	// Input: double T: is the temperature for one time step
	//        double r: developement rate 
	//
	// Output: The new status of the insect after attrition
	//*****************************************************************************
	bool CWSpruceBudworm::IsDeadByAttrition(double T, double r)const
	{
		bool bDead = false;
/*
	if (GetStand()->m_bApplyAttrition && GetStage() >= L2 && GetStage() <= PUPAE)
		{
			if (!(GetStage() == L2 && m_generation == 1))//???
			{
				//Computes attrition (probability of survival in a given time step, based on development rate)
				double p_survival = pow(CWSBAttrition::GetRate(GetStage(), T), r);
				double u = RandomGenerator().Randu();
				if (u > p_survival)
					bDead = true;
			}

		}
*/
		return bDead;
	}


	bool CWSpruceBudworm::IsDeadByOverwintering(double T, double dt)
	{
		//L2o Survival module
		bool bDead = false;
		if (GetStand()->m_bApplyWinterMortality)
		{
			if (GetStage() == L2o && T > 0 && m_OWDate.IsInit())
			{
				m_energy += -0.03295 * pow(max(0.0, T / 10), 3.5675) * dt / 7; //SAS Survival.lst
				bDead = (m_overwinterLuck >= (1 / (1 + exp(-(m_energy))))*0.6939); //0.4536/0.6537; if true, insect dies. Otherwise, it survives time step.
			}
		}

		return bDead;
	}

	//Larval survival probablity, part of the daily "Live" function of a L2 larva
	//Each larva (L2) receives at creation a uniformly-distributed random number between 0 and 1, representing its "synchrony luck"
	bool CWSpruceBudworm::IsDeadByAsynchrony()
	{
		bool bDead = false;
		if (GetStand()->m_bApplyAsynchronyMortality)
		{
			CWSBTree* pTree = (CWSBTree*)m_pHost;

			//This is alculated ONCE, when an L2 larva emerges from overwintering
			//Compute larval survival value of today's amount of bud development and defoliation
			double def = GetStand()->m_defoliation; //Defoliation affects synchrony survival.
			double probBudMineable = pTree->GetProbBudMineable();
			double p[5] = { 0.2691, 0.7859, -0.7367, 1.3118, 1.0606 };
			double logit = p[0] + p[1] * probBudMineable + p[2] * def + p[3] * probBudMineable*probBudMineable + p[4] * probBudMineable*def;
			double survSynch = 1. / (1 + exp(-logit));
			bDead = m_synchLuck > survSynch;
		}

		return bDead;
	}

	//L6 survival probablity, part of the daily "Live" function of a L6 larva
	//Each L6 receives at creation a uniformly-distributed random number between 0 and 1, representing its "window luck"
	bool CWSpruceBudworm::IsDeadByWindow()
	{
		CWSBTree* pTree = (CWSBTree*)m_pHost;

		//This is calculated ONCE, when an L5 larva moults to L6
		//Compute survival value of today's amount of bud development (Degree-days)

		double ddShoot = pTree->GetShootDevel();
		double def = GetStand()->m_defoliation; //Defoliation affects synchrony survival, range [0,1]

		//	static const double P0         = 75.79;   //Amplitude of Lognormal L6 survival
		//	static const double P1         = -52.18;  //Amplitude of Lognormal L6 survival
		//	static const double P2         = 0.246;   //Amplitude of Lognormal L6 survival
		//	static const double A = 4.812;   //dd center of lognormal L6 survival
		//	static const double B = 0.791;   //dd variance of lognormal L6 survival
		static const double P0 = 122.63;   //Amplitude of Lognormal L6 survival
		static const double P1 = -90.94;  //Amplitude of Lognormal L6 survival
		static const double P2 = 0.3086;   //Amplitude of Lognormal L6 survival
		static const double A = 4.8144;   //dd center of lognormal L6 survival
		static const double B = 0.7921;   //dd variance of lognormal L6 survival
		static const double SQRT2PI = 2.50663; //square root of 2*PI
		static const double a_wt = 0.04645; //intercept of weight vs survival regression
		static const double b_wt = 0.09373; //slope of weight vs survival regression
		static const double s_wt = 0.0166 / 2;  //st dev of random term of weight vs survival regression, halved
		static const double a_fec = 0;       //intercept of fecundity vs weigh regression
		static const double b_fec = 2088.2;  //slope of fecundity vs weight regression
		static const double s_fec = 91.97 / 2;   //st dev of random term of fecundity vs weight regression halved

		//Lognormal survival function
		double psurv = 0;
		if (ddShoot > 0.) 
			psurv = (P0 + P1*pow((def + 0.01), P2)) / (B*ddShoot*SQRT2PI) * exp(-0.5*pow((log(ddShoot) - A) / B, 2));
		//	if(ddShoot>0.) psurv = (P0+P1*def+P2/(def+0.01))/(B*ddShoot*SQRT2PI) * exp(-0.5*pow((log(ddShoot)-A)/B,2));


		//pupal weight (mg) is correlated with psurv (L6 bioassay relationship) Régnière at Nealis 2016, Insect Science, P. 7 in text
		double wt = 0;
		while(wt <= 0.01)
			wt = a_wt + b_wt*psurv + RandomGenerator().RandNormal(0.0, s_wt);

		//si wt = 0.2 alors m_potentialFecundity peut varier de 200 à 600... ce qui donne de grosse valeurs Régnière at Nealis 2016, Insect Science, P. 7 in text
		while (m_potentialFecundity <= 0)
			m_potentialFecundity = a_fec + b_fec*wt + RandomGenerator().RandNormal(0.0, s_fec); //Prevent negative fecundities

		bool bDead = false;
		if (GetStand()->m_bApplyAsynchronyMortality)
		{
			//Bug dies if its luck is > probability of survival at the time it becomes L6
			bDead = m_windowLuck > psurv;
		}


		return bDead;
	}



	//****************************************************************************************
	//CWSBTree class

	CWSBTree::CWSBTree(CStand* pStand) :
		CHost(pStand)
	{
		clear();
	}

	void CWSBTree::clear()
	{
		CHost::clear();

		m_ddays = 0;
		m_probBudMineable = 0;
		m_ddShoot = 0;
	}

	void CWSBTree::HappyNewYear()
	{
		CHost::HappyNewYear();

		m_ddays = 0;
		m_probBudMineable = 0;
		m_ddShoot = 0;
	}

	void CWSBTree::Live(const CWeatherDay& weather)
	{
		//For optimisation, nothing happens when temperature is under -10
		if (weather.GetTRef().GetJDay() != 0 && weather[H_TMAX2][MEAN] < -10)
			return;

		ComputeMineable(weather);
		ComputeShootDevel(weather);
		CHost::Live(weather);
	}


	void CWSBTree::GetStat(CTRef d, CModelStat& stat, size_t generation)
	{
		CHost::GetStat(d, stat); 
		
		stat[S_AVERAGE_INSTAR] = GetAI(true); //SBStat.GetAverageInstar();

		//  This line should be restored to work on the L2 synchrony test runs
		stat[S_P_MINEABLE] = m_ddays;//m_probBudMineable;
		//  This line should be restored to work on the L6 window test runs
		stat[S_SHOOT_DEVEL] = m_ddShoot;

	}

	void CWSBTree::ComputeMineable(const CWeatherDay& weather)
	{
		//Bud development 
		//This is calculated for the first summer, then reset on December 31.
		//This is part of the tree's daily "Live" loop, starting on m_startDate

		//Constants and parameters 
		static const int    START_DATE = 92 - 1; //First day (0- based) for degree day summation for bud develpment
		static const double BASE_TEMP = 8.2; //dd summation of bud development
		static const double MAX_TEMP = 14.4; //dd summation of bud development
		static const double a = 95.9; //dd for 50% of buds mineable
		static const double b = 2.123;//dd variance around the 50% dd sum

		if (weather.GetTRef().GetJDay() >= START_DATE)
		{
			// Loop over every time step in one day
			for (size_t h = 0; h < weather.size(); h++)
			{
				//Linear DDays with upper threshold
				m_ddays += max(0.0, (min((double)weather[h][H_TAIR2], MAX_TEMP) - BASE_TEMP) / weather.size());
			}

			//At end of day, compute proportion of buds mineable
			m_probBudMineable = 0;

			if (m_ddays > 0)
			{
				m_probBudMineable = 1 / (1 + exp(-((m_ddays - a) / (b*sqrt(m_ddays))))); //A sigmoid inreasing function (from 0 to 1) of ddays
			}
		}
	}

	void CWSBTree::ComputeShootDevel(const CWeatherDay& weather)
	{
		//Shoot development 
		//This is calculated for the first summer, then reset on December 31.
		//This is part of the tree's daily "Live" loop, starting on START_DATE

		//Constants and parameters 
		static const int    START_DATE = 92 - 1;    //First day (0- based) for degree day summation for bud develpment
		static const double BASE_TEMP = 11.5;    //dd summation of bud development
		static const double MAX_TEMP = 35.;     //dd summation of bud development

		if (weather.GetTRef().GetJDay() >= START_DATE)
		{
			// Loop over every time step in one day
			for (size_t h = 0; h < weather.size(); h++)
			{
				//Linear DDays with upper threshold
				m_ddShoot += max(0.0, (min((double)weather[h][H_TAIR2], MAX_TEMP) - BASE_TEMP) / weather.size());
			}
		}
	}


}