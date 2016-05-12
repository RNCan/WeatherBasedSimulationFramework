/*************************************************************************************
Modifications adopted as "best" from phenology point of view
	1. Cut off female egg laying after 50% spent (in CMountainPineBeetle::Develop(), line if( m_totalBrood/potentialFecundity >= 0.5) )
	2. Reduce development variability in immature life stages (in CMountainPineBeetle::AssignRelativeDevRate(), line if(s!=OVIPOSITING_ADULT) sigma=sigma/2.; 
*************************************************************************************/
#include <math.h>
#include "MountainPineBeetle.h"
#include "Basic/UtilMath.h"
#include "Basic/TimeStep.h"
#include "Basic/WeatherStation.h"

namespace WBSF
{


	//******************************************************************
	//CMountainPineBeetle
	const double CMountainPineBeetle::SEX_RATIO = 0.7;


	// Object creator
	CMountainPineBeetle::CMountainPineBeetle(CHost* pHost, CTRef creationDay, double age, size_t sex, bool bFertil, int generation, double scaleFactor) :
		CIndividual(pHost, creationDay, age, sex, bFertil, generation, scaleFactor)
	{
		//Only females are modeled (fecundity is reduced by SEX_RATIO in Develop() )
		m_sex = FEMALE;

		//Individual's "relative" development rate for each life stage, and female fecundity
		//These are lognormal, and are independent in successive life stages.
		AssignRelativeDevRate();

		//Individual's mortality attributes (cold tolerance, attrition, snow protection)
		AssignMortality();

		m_lastStatus = HEALTHY;
		m_lastAge = -1;
		m_bEmerging = false;
		m_bAttackToday = false;
		m_bSuccessAttackToday = false;


	}

	CMountainPineBeetle::CMountainPineBeetle(const CMountainPineBeetle& in) :CIndividual(in.m_pHost)
	{
		operator=(in);
	}

	CMountainPineBeetle& CMountainPineBeetle::operator=(const CMountainPineBeetle& in)
	{
		if (&in != this)
		{
			CIndividual::operator=(in);

			for (int i = 0; i < NB_STAGES; i++)
				m_relativeDevRate[i] = in.m_relativeDevRate[i];

			m_bEmerging = in.m_bEmerging;
			m_bAttackToday = in.m_bAttackToday;
			m_bSuccessAttackToday = in.m_bSuccessAttackToday;
			m_PCritical = in.m_PCritical;
			m_PCriticalP1 = in.m_PCriticalP1;
			m_PCriticalAttrition = in.m_PCriticalAttrition;
			m_height = in.m_height;

		}

		return *this;
	}


	// Object destructor
	CMountainPineBeetle::~CMountainPineBeetle(void)
	{}


	void CMountainPineBeetle::AssignUnicity()
	{
		//This is used when the number of objects is increased as the population drops below 100 (each object representing less than one individual) 
		AssignRelativeDevRate();
		AssignMortality();
	}

	//sets the relative development rate for all stages (Egg to Ovipositing Adult)
	void CMountainPineBeetle::AssignRelativeDevRate()
	{
		//Development rate variability function parameters from Régnière et al 2012. (mean is 1)
		for (int s = 0; s < NB_STAGES; ++s)
		{
			//Lognormal distribution of relative development rates
			const CMPBDevelopmentTable & rates = GetStand()->m_rates;
			double sigma = rates.GetRelativeSigma(s);
			if (s != OVIPOSITING_ADULT) sigma = sigma / 2; //A modification made to imporve observed/simulated adult emergence patterns 
			m_relativeDevRate[s] = RandomGenerator().RandLogNormal(0, sigma);

			//extremes of the distributions are avoided
			while (m_relativeDevRate[s]<0.4 || m_relativeDevRate[s]>2.5)
				m_relativeDevRate[s] = RandomGenerator().RandLogNormal(0, sigma);

		}

	}

	void CMountainPineBeetle::AssignMortality()
	{
		//Individual's cold tolerance attributes ( UNIFORM ]0,1[ ). This attribute is constant throughout its life.
		m_PCritical = RandomGenerator().RanduExclusive();

		//Determines whether an individual is in larval cold tolerance State I (feeding) as opposed to States II or III (non-feeding)
		m_PCriticalP1 = RandomGenerator().Randu();

		m_PCriticalAttrition = RandomGenerator().Randu();

		//m_height = RandomGenerator().Rand(0, 1000);//position of bugs in the tree above ground, in centimeters (bark inhabited up to 10 m )
		m_height = RandomGenerator().Rand(0.0, GetStand()->m_heightMax);//position of bugs in the tree above ground, in centimeters (bark inhabited up to 10 m )
	}

	void CMountainPineBeetle::Live(const CWeatherDay& weaDay)
	{
		if (IsCreated(weaDay.GetTRef()))
		{
			CIndividual::Live(weaDay);
			//static const CTRef MyRef(1986, AUGUST, 15);

			//if( T.m_firstTRef == MyRef )
			//{
			//int i;
			//i=0;
			//}

			//m_lastAge = m_age;
			//m_lastStatus = m_status;
			m_bAttackToday = false;
			m_bSuccessAttackToday = false;


			//if this individual is alive, proceed with development, reproduction and attrition
			for (int h = 0; h < T.size() && IsAlive(); h++)
				Develop(T.GetFirstTRef(), T[h], weaDay);

			//Attrition and cold-tolerance mortality
			if (IsAlive())
			{
				if (IsDeadByAttrition())
				{
					m_status = DEAD;
					m_death = ATTRITION;
				}
				else if (IsDeadByFrost(T.GetFirstTRef(), weaDay.GetTMin(), weaDay[DAILY_DATA::SNDH]))//convert mm of snow water equivalent to snow height in cm (1 cm of snow/SWE mm)
				{
					m_status = DEAD;
					m_death = FROZEN;
				}
			}
		}
	}

	//Creates an individual's brood (eggs laid that are female and survive attrition), on a daily basis
	void CMountainPineBeetle::Brood(const CDailyWaveVector& T, const CWeatherDay& weaDay)
	{
		if (m_bFertil && m_brood > 0)
		{
			//When the bugs are killed by tree at the end of the day, they must not create eggs
			CMPBTree* pTree = GetTree();
			CMPBStand* pStand = GetStand(); ASSERT(pStand);
			//CMountainPineBeetleVector& bugs = pTree->GetBugs();
			//CMountainPineBeetle* pBug = !bugs.empty()?bugs[bugs.size()-1]:NULL;

			//attrition turned off,
			double scaleFactor = m_brood*SEX_RATIO*pStand->m_survivalRate*m_scaleFactor;
			double scaleFactorAttrition = m_brood*SEX_RATIO*(1 - pStand->m_survivalRate)*m_scaleFactor;
			//attrition turned on
			//		double scaleFactor = m_brood*SEX_RATIO*m_scaleFactor;

			//if( pBug!=NULL &&
			//	pBug->GetAge() == 0 && 
			//	pBug->GetCreationDate() == T.m_firstTRef &&
			//	pBug->GetGeneration() == m_generation+1 )
			//{
			//	//for optimization, we add new eggs into the last created egg under the same context
			//	pBug->SetScaleFactor(pBug->GetScaleFactor()+scaleFactor);
			//}
			//else
			//{
			ASSERT(scaleFactor > 0);

			//if it's not the case, we create a new egg...
			//pTree->AddBug( new CMountainPineBeetle(pTree, T.GetFirstTRef(), EGG, pStand->m_bFertilEgg, m_generation+1, scaleFactor) );
			bool bFertilEgg = pStand->m_nbFertilGeneration == -1 || pStand->m_nbFertilGeneration > m_generation;
			pTree->AddBug(new CMountainPineBeetle(pTree, T.GetFirstTRef(), EGG, bFertilEgg, m_generation + 1, scaleFactor));


			if (scaleFactorAttrition > 0)
			{
				CMountainPineBeetle* pBug = new CMountainPineBeetle(pTree, T.GetFirstTRef(), EGG, bFertilEgg, m_generation + 1, scaleFactorAttrition);
				pBug->m_status = DEAD;
				pBug->m_death = ATTRITION;
				pTree->AddBug(pBug);
			}
			//}
		}
	}

	void CMountainPineBeetle::Develop(CTRef date, const double& T, const CWeatherDay& weaDay)
	{
		//Develops all individuals, including ovipositing adults
		//Kills by attrition and old age
		//Computes brood produced by individual
		_ASSERTE(m_status == HEALTHY);


		//the end of the day after emerging is lost...
		if (m_bEmerging)
		{
			//ASSERT( m_totalBrood==0 );
			return;
		}

		//dt is the number of time steps per day (24h/step duration(h) )
		double dt = gTimeStep.GetDT();

		int s = GetStage();

		double r = GetStand()->m_rates.GetRate(s, T) / dt;
		//double r = RATES_DEBUG.ComputeRate(s,T)/dt;

		double RR = 0;
		if (s < OVIPOSITING_ADULT)//all stages except OVIPOSITING_ADULTs
		{
			RR = Max(0.0, m_relativeDevRate[s] * r);

			if (GetStand()->m_bArrestDevel && s >= L1 && s <= L4)
			{
				const CMPBColdTolerance& coldTolerance = GetStand()->GetColdTolerance();
				//"ARRESTED LARVAL DEVELOPMENT" not implemented (m_bArrestDevel=false)
				if (m_PCriticalP1 > coldTolerance[date].m_p1)
					RR = 0;
			}
			// Applies the "flight threshold" temperature for emerging adults
			if (s == TENERAL_ADULT && IsChangingStage(RR))
				if (T < GetStand()->m_emergenceThreshold)
					RR = 0;
		}
		else
		{
			//m_brood is a female's daily viable egg production
			//m_totalBrood is is a female's cumulative egg production
			//r is the time step's oviposition rate (a proportion)

			if (m_sex == FEMALE)
			{
				double potentialFecundity = CMPBDevelopmentTable::POTENTIAL_FECUNDITY*m_relativeDevRate[OVIPOSITING_ADULT];
				double brood = r*Max(0, potentialFecundity - m_totalBrood); //all available broods

				//to avoid excessive longevity, if a female has laid 95% of her eggs, we force her to lay the remainder
				if ((m_totalBrood + brood) / potentialFecundity > 0.95)
					brood = potentialFecundity - m_totalBrood;

				m_brood += brood;
				m_totalBrood += brood;

				RR = brood / potentialFecundity;

				//because of packing, m_totalBrood and m_age is sometime desynchronized. We force them to die of old age.
				//			if( m_totalBrood/potentialFecundity >= 1)
				if (m_totalBrood / potentialFecundity >= 0.5) //This modification made to improve observed/simulated fit of adult emergence patterns 
					RR = DEAD_ADULT - m_age;

			}

			ASSERT(m_brood >= 0);
			ASSERT(RR >= 0 && RR < 1);
			ASSERT(RR > 0 || r == 0);
			ASSERT(!m_bEmerging);
			ASSERT(m_age >= 7);
			ASSERT(m_totalBrood < 2.5*CMPBDevelopmentTable::POTENTIAL_FECUNDITY);

		}

		bool bChangingStage = IsChangingStage(RR);
		if (s == L4 && bChangingStage)
		{
			//compute mortality now before end of stage
			double pMortality = m_sumLarvalColdMortality[HIGHEST];
			//Kill by ratio
			if (pMortality > 0)
			{
				//number of bugs kill today
				double numberKilled = m_scaleFactor*pMortality;
				//try to find an object with the same properties
				CMPBTree* pTree = GetTree(); ASSERT(pTree);
				CMountainPineBeetle* pBug = (CMountainPineBeetle*)pTree->GetLastObject(m_generation, GetStage(), DEAD, FROZEN, 5);

				if (pBug == NULL)
				{
					//if we don't find it, create a new dead object
					pBug = new CMountainPineBeetle(*this);
					pBug->m_scaleFactor = 0;
					pBug->m_status = DEAD;
					pBug->m_death = FROZEN;
					pTree->AddBug(pBug);
				}

				//update scalefactor
				pBug->m_scaleFactor += numberKilled;
				m_scaleFactor -= numberKilled;
			}
		}


		m_age += RR;



		if (s == TENERAL_ADULT && bChangingStage)
		{
			PreEmerging();
		}
		else
		{
			if (GetStage() >= DEAD_ADULT)
			{
				m_status = DEAD;
				m_death = OLD_AGE;
			}
		}
	}

	void CMountainPineBeetle::PreEmerging()
	{
		ASSERT(!m_bEmerging);

		//let the tree know the number of attacks for this day
		//This object knows that it is emerging
		GetTree()->EmergingBugs(m_scaleFactor);
		m_bEmerging = true;
	}

	void CMountainPineBeetle::CompleteEmergence(int death)
	{
		_ASSERTE(GetStage() == OVIPOSITING_ADULT);
		_ASSERTE(m_brood == 0 && m_totalBrood == 0);

		//When all object have been developed, now complete emergence
		//some objects will be killed and some objects will survive
		if (death != DENSITY_DEPENDENCE)
			m_bAttackToday = true;


		m_bSuccessAttackToday = death == LIVE;
		if (death == LIVE)
		{
			//update age only if the bug survives
			m_age = OVIPOSITING_ADULT;
		}
		else
		{
			m_status = DEAD;
			m_death = death;
		}

		m_bEmerging = false;

	}



	bool CMountainPineBeetle::IsDeadByAttrition()
	{

		bool bDeath = false;
		//attrition turned off
		return bDeath;
		//attrition turned on

		//If no attrition is used, kill at egg creation in CMountainPineBeetle::Brood
		//We can select the stage(s) to be killed here

		if (GetStage() == L4 && int(m_age) != int(m_lastAge))
		{
			const CMPBStand* pStand = GetStand(); ASSERT(pStand);

			//Computes attrition (probability of survival in a given time step, based on development rate)
			double Si = pStand->m_survivalRate;
			double pSurvival = pow(Si, 1 / m_relativeDevRate[L4]);
			if (m_PCriticalAttrition > pSurvival)
				bDeath = true;


			//CMPBTree* pTree = GetTree(); ASSERT( pTree);
			//double numberKilled = m_scaleFactor*(1.0-pSurvival);
			//if( numberKilled > 0)
			//{
			//	CMountainPineBeetle* pBug = (CMountainPineBeetle*) pTree->GetLastObject(GetStage(), DEAD, ATTRITION, 1);
			//	if( pBug && (pBug->m_lastStatus != m_lastStatus || pBug->m_lastAge != m_lastAge) )
			//		pBug = NULL;

			//	if( pBug== NULL)
			//	{
			//		pBug = new CMountainPineBeetle(*this);
			//		pBug->m_scaleFactor=0;
			//		pBug->m_status = DEAD;
			//		pBug->m_death = ATTRITION;
			//		pTree->AddBug(pBug);
			//	}

			//	pBug->m_scaleFactor += numberKilled;
			//	m_scaleFactor-= numberKilled;
			//}
		}

		return bDeath;
	}

	//snowDepth in mm of snow
	bool CMountainPineBeetle::IsDeadByFrost(CTRef date, const double& T, double snowDepth)
	{
		bool bFrost = false;

		bool bSkipCold = GetStand()->m_bSnowProtection && (m_height < snowDepth);
		if (GetStand()->m_applyColdTolerance && !bSkipCold)
		{
			int stage = GetStage();


			if (stage == EGG) //Reid & Gates 1970. Any exposure < -18 C is death
			{
				if (T < -18.0)
					bFrost = true;
			}
			//Larval stages
			else if (stage >= L1 && stage <= L4) //After Regniere & Bentz 2007
			{
				const CMPBColdTolerance& coldTolerance = GetStand()->GetColdTolerance();

				//kill by probability
				//if( coldTolerance[date].m_Pmort>-999 && 
				//	m_PCritical < coldTolerance[date].m_Pmort )
				//{
				//	bFrost=true;
				//}

				//Kill by ratio
				if (coldTolerance[date].m_Pmort>-999)
				{
					//sum of mortality
					m_sumLarvalColdMortality += coldTolerance[date].m_Pmort;
					//	if( coldTolerance[date].m_Pmort>=1)
					//	{
					//		//kill the object
					//		bFrost=true;
					//	}
					//	else if( coldTolerance[date].m_Pmort > 0)
					//	{
					//		//number of bugs kill today
					//		double numberKilled = m_scaleFactor*coldTolerance[date].m_Pmort;
					//		//try to find an object with the same properties
					//		CMPBTree* pTree = GetTree(); ASSERT( pTree);
					//		CMountainPineBeetle* pBug = (CMountainPineBeetle*) pTree->GetLastObject(m_generation, GetStage(), DEAD, FROZEN, 5);

					//		if( pBug==NULL)
					//		{
					//			//if we don't find it, create a new dead object
					//			pBug = new CMountainPineBeetle(*this);
					//			pBug->m_scaleFactor=0;
					//			pBug->m_status = DEAD;
					//			pBug->m_death = FROZEN;
					//			pTree->AddBug(pBug);
					//		}

					//		//update scalefactor
					//		pBug->m_scaleFactor += numberKilled;
					//		m_scaleFactor-= numberKilled;

					//	}
				}


			}
			//Pupae and teneral adults (from Table 1 in Amman, Logan etc. loose leafs)
			else if (stage >= PUPA && stage <= TENERAL_ADULT)
			{
				//if(T<-18.0)
				//{
				//				double pMort = 1-Min( 1, Max( 0, (T+34)/16));
				//				if( m_PCritical < pMort )
				//		bFrost=true;
				//}

				double T1 = -18;
				double T2 = GetStand()->m_pupeTeneralColdT;
				double pMort = 1 - Min(1, Max(0, (T - T2) / (T1 - T2)));
				if (m_PCritical < pMort)
					bFrost = true;

			}
			//Ovipositing adults as per Lester & Irwin 2012 (equation fitted by Régnière)
			else
			{
				if (GetStand()->m_applyColdToleranceOvipAdult == OA_KILL_COLD)
				{
					int t = date.GetJDay();
					double SCPa = -20.2 - 6.09*cos(Deg2Rad(2 * PI*pow(t / 365, 1.365)));
					if (T < SCPa)
						bFrost = true;
				}
				else if (GetStand()->m_applyColdToleranceOvipAdult == OA_KILL_DECEMBER_31 &&
					date.GetMonth() == DECEMBER && date.GetDay() == 30)
				{
					bFrost = true;
				}
				else if (GetStand()->m_applyColdToleranceOvipAdult == OA_KILL_M18 &&
					T < -18)
				{
					bFrost = true;
				}
			}
		}

		return bFrost;
	}


	//GetStat is the main information gatherer
	void CMountainPineBeetle::GetStat(CTRef d, CModelStat& stat)
	{
		//Generic information (for tree and objects). We put the tree information here to have attack sorted by generation
		if (m_bAttackToday)
			stat[E_NB_ATTACKS] += m_scaleFactor;

		if (m_bSuccessAttackToday)
		{
			stat[E_NB_SUCESS_ATTACKS] += m_scaleFactor;
			stat[E_NB_INFESTED_TREE] += GetTree()->GetNbInfestedTrees(m_scaleFactor);
		}

		stat[S_DD_FACTOR] += GetStand()->GetDDFactor();
		stat[S_NB_PACK] += GetTree()->GetNbPacked();
		stat[S_NB_OBJECT]++;

		if (IsAlive())
			stat[S_NB_OBJECT_ALIVE]++;


		//Specific bug information
		//no information are extracted from initial attack
		if (!IsCreated(d) || m_generation == 0)
			return;

		short stage = GetStage();

		//total and daily brood
		stat[S_BROOD] += m_totalBrood*m_scaleFactor;
		stat[E_BROOD] += m_brood*m_scaleFactor;

		if (IsAlive())
		{
			//information about stage
			stat[S_EGG + stage] += m_scaleFactor;
		}
		else
		{
			//information about dead (cumulatif)
			stat[S_DEAD] += m_scaleFactor;

			if (m_death == OLD_AGE)
			{
				stat[S_DEAD_ADULT] += m_scaleFactor;
				stat[S_DEAD_OLD_AGE] += m_scaleFactor;
			}
			else if (m_death == FROZEN)
			{
				stat[S_DEAD_FROZEN] += m_scaleFactor;
			}
			else if (m_death == ATTRITION)
			{
				stat[S_DEAD_ATTRITION] += m_scaleFactor;
			}
			else if (m_death == TREE_DEFENSE)
			{
				stat[S_DEAD_BY_TREE] += m_scaleFactor;
			}
			else if (m_death == DENSITY_DEPENDENCE)
			{
				stat[S_DEAD_DENSITY_DEPENDENCE] += m_scaleFactor;
			}

			//information about dead (instant)
			if (m_status != m_lastStatus)
			{
				stat[E_DEAD] += m_scaleFactor;


				if (stage >= OVIPOSITING_ADULT && (m_death == FROZEN || m_death == OLD_AGE))
				{
					ASSERT(m_death == FROZEN || m_death == OLD_AGE);
					stat[E_TOTAL_BROOD] += m_scaleFactor*m_totalBrood;
					stat[E_TOTAL_FEMALE] += m_scaleFactor;
					stat[E_LONGIVITY] += m_scaleFactor*(d - m_creationDate);
				}

				if (m_death == OLD_AGE)
				{
					stat[E_DEAD_OLD_AGE] += m_scaleFactor;
				}
				else if (m_death == FROZEN)
				{
					stat[E_DEAD_FROZEN] += m_scaleFactor;
					stat[DF_EGG + stage] += m_scaleFactor;



				}
				else if (m_death == ATTRITION)
				{
					stat[E_DEAD_ATTRITION] += m_scaleFactor;
				}
				else if (m_death == TREE_DEFENSE)
				{
					stat[E_DEAD_BY_TREE] += m_scaleFactor;
				}
				else if (m_death == DENSITY_DEPENDENCE)
				{
					stat[E_DEAD_DENSITY_DEPENDENCE] += m_scaleFactor;
				}
			}
		}

		//Stage transition
		if (int(m_age) != int(m_lastAge))
		{
			stat[E_EGG + int(m_age)] += m_scaleFactor;

			if (stage == OVIPOSITING_ADULT)
			{
				CTRef firstWinter(m_creationDate.GetYear(), DECEMBER, LAST_DAY);
				CTRef secondWinter(m_creationDate.GetYear() + 1, DECEMBER, LAST_DAY);
				CTRef thirdWinter(m_creationDate.GetYear() + 2, DECEMBER, LAST_DAY);

				if (d < firstWinter)
					stat[S_BIVOLTIN] += m_scaleFactor;
				else if (d < secondWinter)
					stat[S_UNIVOLTIN] += m_scaleFactor;
				else if (d < thirdWinter)
					stat[S_SEMIVOLTIN] += m_scaleFactor;
				else stat[S_TRIENVOLTIN] += m_scaleFactor;
			}
		}



	}




	//WARNING: cast must be defined here to avoid bug
	CMPBTree* CMountainPineBeetle::GetTree(){ return static_cast<CMPBTree*>(m_pHost); }
	const CMPBTree* CMountainPineBeetle::GetTree()const{ return static_cast<const CMPBTree*>(m_pHost); }
	CMPBStand* CMountainPineBeetle::GetStand(){ ASSERT(m_pHost); return (CMPBStand*)m_pHost->GetStand(); }
	const CMPBStand* CMountainPineBeetle::GetStand()const{ ASSERT(m_pHost); return (const CMPBStand*)m_pHost->GetStand(); }
	//*******************************************************************************************************
	//*******************************************************************************************************
	//CMPBTree
	//void CMPBTree::Initialise(long numInd, CTRef peakAttackDay, double sigma, double age, bool bFertil, int generation)
	//{
	//CMPBTreeBase::Initialise((int)(numInd/**m_treeSize*/), peakAttackDay, sigma, age, bFertil, generation);
	//}


	//void CMPBTree::UpdateScaleFactor(double nbTreeInfested)
	//{
	//ASSERT( nbTreeInfested>=1);

	//double initialAttack = GetStand()->GetNbInitialAttack();
	//double scaleFactor = initialAttack/m_bugs.size();
	//for(int i=0; i<m_bugs.size(); i++)
	//	m_bugs[i]->SetScaleFactor(scaleFactor);
	//}

	void CMPBTree::EmergingBugs(double nbBugs)
	{
		m_emergingToday += nbBugs;
	}

	void CMPBTree::CompleteEmergence()
	{

		if (m_emergingToday > 0)
		{
			//double nbKm² = GetNbInfestedTrees(m_emergingToday)/GetStand()->m_forestDensity;
			//double DDFactor = pow(Max(0,Min(1, nbKm² /GetStand()->m_forestSize)), GetStand()->m_DDAlpha);
			double DDFactor = GetStand()->GetDDFactor();
			ASSERT(DDFactor <= 1);

			double nbAttacksToday = m_emergingToday*(1 - DDFactor);
			double nbTrees = ceil(nbAttacksToday / m_Amax);
			double A = m_A0*nbTrees;

			//if there are fewer than A females, they attack defenseless (dying) tree
			double nbBugsAlive = GetNbBugsAlive();
			bool bUseDefenselessTree = GetStand()->m_bUseDefenselessTree && nbBugsAlive < A;

			for (int i = m_bugs.size() - 1; i >= 0; i--)
			{
				if (m_bugs[i]->IsEmerging())
				{
					//apply density dependence here

					if (GetStand()->GetNbInfestedTrees() < GetStand()->GetNbTrees())
					{
						double lostBug = m_bugs[i]->GetScaleFactor()*DDFactor;
						if (lostBug > 0)
						{
							ASSERT(lostBug < m_bugs[i]->GetScaleFactor());
							m_bugs[i]->SetScaleFactor(m_bugs[i]->GetScaleFactor() - lostBug);

							CMountainPineBeetle* pBugCopy = new CMountainPineBeetle(*m_bugs[i]);
							pBugCopy->SetScaleFactor(lostBug);
							pBugCopy->CompleteEmergence(CIndividual::DENSITY_DEPENDENCE);
							m_bugs.push_back(pBugCopy);
						}


						if (!bUseDefenselessTree)
						{
							//if we exceed the forest capacity, we kill all others
							double nbTreeNonInfested = GetStand()->GetNbTrees() - (GetStand()->GetNbInfestedTrees() + GetNbInfestedTreesToday());
							double nbAttackMax = nbTreeNonInfested*m_Amax;
							if (m_bugs[i]->GetScaleFactor() - A > nbAttackMax)
								A = m_bugs[i]->GetScaleFactor() - nbAttackMax;


							//apply tree mortality
							if (A > 0 && m_bugs[i]->GetScaleFactor() > A)
							{
								//we "unpack" the object to kill only the right number of bugs
								CMountainPineBeetle* pBugCopy = new CMountainPineBeetle(*m_bugs[i]);
								pBugCopy->SetScaleFactor(A);
								pBugCopy->CompleteEmergence(CIndividual::TREE_DEFENSE);

								m_bugs[i]->SetScaleFactor(m_bugs[i]->GetScaleFactor() - A);
								m_bugs.push_back(pBugCopy);
								A = 0;
							}

							ASSERT(m_bugs[i]->GetScaleFactor() > 0);

							if (A > 0)
							{
								ASSERT(m_bugs[i]->GetScaleFactor() <= A);
								m_bugs[i]->CompleteEmergence(CIndividual::TREE_DEFENSE);
								A -= m_bugs[i]->GetScaleFactor();
							}
							else
							{
								ASSERT(A <= 0);
								m_bugs[i]->CompleteEmergence(CIndividual::LIVE);
								m_nbSucessfullAttacksToday += m_bugs[i]->GetScaleFactor();



								ASSERT(GetStand()->GetNbInfestedTrees() + GetNbInfestedTreesToday() <= GetStand()->GetNbTrees());
							}
						}
						else
						{
							m_bugs[i]->CompleteEmergence(CIndividual::LIVE);
						}
					}
					else
					{
						//if the 
						m_bugs[i]->CompleteEmergence(CIndividual::DENSITY_DEPENDENCE);
					}
				}
			}
		}
	}

	void CMPBTree::GetStat(CTRef date, CModelStat& stat, int generation)
	{
		CMPBTreeBase::GetStat(date, stat, generation);
	}


	void CMPBTree::Live(CDailyWaveVector& hVector, const CWeatherDay& weaDay)
	{
		m_emergingToday = 0;
		m_nbSucessfullAttacksToday = 0;

		if (m_initialSize == -1)
			m_initialSize = m_bugs.size();

		for (int i = (int)m_bugs.size() - 1; i >= 0; i--)
			m_bugs[i]->Live(hVector, weaDay);

		CompleteEmergence();


		for (int i = (int)m_bugs.size() - 1; i >= 0; i--)
			m_bugs[i]->Brood(hVector, weaDay);



	}

	class CPOverheat : public COverheat
	{
	public:

		virtual void TransformWeather(CWeatherDay& weaDay)const;
	};

	void CPOverheat::TransformWeather(CWeatherDay& weaDay)const
	{
		ASSERT(weaDay[DAILY_DATA::TMIN] > -999 && weaDay[DAILY_DATA::TMAX] > -999);

		double Tmin = weaDay.GetTMin();
		double Tmax = weaDay.GetTMax();
		double Trange = weaDay.GetTRange();
		double Sin = sin(2 * 3.14159*(weaDay.GetJDay() / 365. - 0.25));

		//	weaDay(DAILY_DATA::TMAX)=weaDay.GetTMax()+3.25*((weaDay.GetTMax()-weaDay.GetTMin())/24.4); //Equation [11] (max overheating)
		//	weaDay(DAILY_DATA::TMIN)=weaDay.GetTMin()+1.8; //Equation [12] (min damping)


		//convertion de la température de l'air(station météo) à la température de l'air (sous-bois)
		//régression à Vienna 

		//Tmin = 0.8722702*Tmin + 3.6607040;//R² = 0.8729454
		//Tmax = 0.9392273*Tmax + 0.3095136; //R² = 0.9369456

		//convert air temperature to bark temperature
		weaDay(DAILY_DATA::TMIN) = -0.1493 + 0.8359*Tmin + 0.5417*Sin + 0.16980*Trange + 0.00000*Tmin*Sin + 0.005741*Tmin*Trange + 0.02370*Sin*Trange;
		weaDay(DAILY_DATA::TMAX) = 0.4196 + 0.9372*Tmax - 0.4265*Sin + 0.05171*Trange + 0.03125*Tmax*Sin - 0.004270*Tmax*Trange + 0.09888*Sin*Trange;

		if (weaDay(DAILY_DATA::TMIN) > weaDay(DAILY_DATA::TMAX))
			CFL::Switch(weaDay(DAILY_DATA::TMIN), weaDay(DAILY_DATA::TMAX));


	}

	void CMPBTree::Live(const CTRef& day, const CWeatherDay& weaDay)
	{
		CMPBStand* pStand = (CMPBStand*)GetStand();
		CDailyWaveVector hVector;// hourly temperature array 
		if (pStand->m_bMicroClimate)
		{
			CPOverheat overheat;
			weaDay.GetAllenWave(hVector, 16, gTimeStep, overheat);//16 is a factor of 4. Number usually used
		}
		else
		{
			weaDay.GetAllenWave(hVector, 16, gTimeStep);//16 is a factor of 4. Number usually used
		}

		Live(hVector, weaDay);

	}

	CMPBStand* CMPBTree::GetStand(){ ASSERT(m_pStand); return (CMPBStand*)m_pStand; }
	//*******************************************************************************************************
	//*******************************************************************************************************
	//CMPBStand
	void CMPBStand::Init(const CWeather& weather)
	{
		m_coldTolerance.m_bMicroClimate = m_bMicroClimate;
		if (m_applyColdTolerance)
		{
			m_coldTolerance.ComputeDaily(weather);

		}


	}

	void CMPBStand::SetTree(CMPBTree* pTree)
	{
		m_pTree.reset(pTree);
		m_pTree->SetStand(this);
	}

	//Density dependence factor
	double CMPBStand::GetDDFactor()const
	{
		double nbTrees = GetNbTrees();
		double DDFactor = pow(m_nbInfestedTrees / nbTrees, m_DDAlpha);

		ASSERT(DDFactor >= 0 && DDFactor <= 1);

		return DDFactor;
	}

}