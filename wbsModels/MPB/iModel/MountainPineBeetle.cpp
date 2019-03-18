//*************************************************************************************
//Modifications adopted as "best" from phenology point of view
//	19-07-2016	Rémi Saint-Amant	New compilation wihth WBSF
//	1. Cut off female egg laying after 50% spent (in CMountainPineBeetle::Develop(), line if( m_totalBrood/potentialFecundity >= 0.5) )
//	2. Reduce development variability in immature life stages (in CMountainPineBeetle::AssignRelativeDevRate(), line if(s!=OVIPOSITING_ADULT) sigma=sigma/2.; 
//*************************************************************************************
#include <math.h>
#include <algorithm>
#include "MountainPineBeetle.h"
#include "Basic/UtilMath.h"
#include "Basic/TimeStep.h"
#include "Basic/WeatherStation.h"

using namespace WBSF::HOURLY_DATA;
using namespace std;


namespace WBSF
{

	class CMicroClimate 
	{
	public:

		CMicroClimate(const CWeatherDay& weather);
		//virtual void TransformWeather(CWeatherDay& weather, size_t hourTmax=16)const;
		//virtual double GetOverheat(const CWeatherDay& weather, size_t h, size_t hourTmax = 16)const;
		
		double GetT(size_t h, size_t hourTmax = 16)const;

	protected:

		double m_Tmin;
		double m_Tmax;
	};

	//void CPOverheat::TransformWeather(CWeatherDay& weather, size_t hourTmax)const
	CMicroClimate::CMicroClimate(const CWeatherDay& weather)
	{
		//const CWeatherDay& day = (h<hourTmax - 12) ? weather.GetPrevious() : h<hourTmax ? weather : weather.GetNext();
		double Tmin = weather[H_TMIN][MEAN];
		double Tmax = weather[H_TMAX][MEAN];
		double Trange = Tmax - Tmin;
		double Sin = sin(2 * 3.14159*(weather.GetTRef().GetJDay() / 365. - 0.25));

		//convertion de la température de l'air(station météo) à la température de l'air (sous-bois)
		//régression à Vienna 

		//convert air temperature to bark temperature
		m_Tmin = -0.1493 + 0.8359*Tmin + 0.5417*Sin + 0.16980*Trange + 0.00000*Tmin*Sin + 0.005741*Tmin*Trange + 0.02370*Sin*Trange;
		m_Tmax = 0.4196 + 0.9372*Tmax - 0.4265*Sin + 0.05171*Trange + 0.03125*Tmax*Sin - 0.004270*Tmax*Trange + 0.09888*Sin*Trange;

		if (m_Tmin > m_Tmax)
			Switch(m_Tmin, m_Tmax);

		//weather.SetStat(H_TAIR, (Tmin2 + Tmax2) / 2);
		//weather.SetStat(H_TRNG, Tmax2 - Tmin2);
		
		//if (weather.IsHourly())
		//{
		//	//updater hourly values
		//	//updater hourly values
		//	for (size_t h = 0; h < 24; h++)
		//		weather[h].SetStat(H_TAIR, GetT(weather, h, hourTmax));
		//}
	}

	double CMicroClimate::GetT(size_t h, size_t hourTmax)const
	{
		double OH = 0;

		double range = m_Tmax - m_Tmin;
		assert(range >= 0);

		int time_factor = (int)hourTmax - 6;  //  "rotates" the radian clock to put the hourTmax at the top  
		double theta = ((int)h - time_factor)*3.14159 / 12.0;
		double T = (m_Tmin + m_Tmax) / 2 + range / 2 * sin(theta);

		return T;
	}




	//******************************************************************
	//CMountainPineBeetle
	const double CMountainPineBeetle::SEX_RATIO = 0.7;


	// Object creator
	CMountainPineBeetle::CMountainPineBeetle(CHost* pHost, CTRef creationDay, double age, size_t sex, bool bFertil, size_t generation, double scaleFactor) :
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

	void CMountainPineBeetle::Live(const CWeatherDay& weather)
	{
		if (IsCreated(weather.GetTRef()))
		{
			CIndividual::Live(weather);
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

			CMicroClimate MC(weather);


			CMPBStand* pStand = (CMPBStand*)GetStand();
			//if this individual is alive, proceed with development, reproduction and attrition
			size_t nbSteps = GetTimeStep().NbSteps();
			for (size_t step = 0; step < nbSteps&&m_age < DEAD_ADULT; step++)
			{
				size_t h = step*GetTimeStep();
				double T = weather[h][H_TAIR];
				if (pStand->m_bMicroClimate)
					T = MC.GetT(h);
				

				//CMPBStand* pStand = (CMPBStand*)GetStand();
				//CDailyWaveVector hVector;// hourly temperature array 
				//if (pStand->m_bMicroClimate)
				//{
				//	CPOverheat overheat;
				//	overheat.TransformWeather(weather);
				//}

				Develop(weather.GetTRef(), T);
			}

			//for (int h = 0; h < T.size() && IsAlive(); h++)
				//Develop(T.GetFirstTRef(), T[h], weaDay);
		}
	}

	//Creates an individual's brood (eggs laid that are female and survive attrition), on a daily basis
	void CMountainPineBeetle::Brood(const CWeatherDay& weather)
	{
		if (m_bFertil && m_broods > 0)
		{
			//When the bugs are killed by tree at the end of the day, they must not create eggs
			CMPBTree* pTree = GetTree();
			CMPBStand* pStand = GetStand(); ASSERT(pStand);
			//CMountainPineBeetleVector& bugs = pTree->GetBugs();
			//CMountainPineBeetle* pBug = !bugs.empty()?bugs[bugs.size()-1]:NULL;

			//attrition turned off,
			double scaleFactor = m_broods*SEX_RATIO*pStand->m_survivalRate*m_scaleFactor;
			double scaleFactorAttrition = m_broods*SEX_RATIO*(1 - pStand->m_survivalRate)*m_scaleFactor;
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
			pTree->push_back(std::make_shared<CMountainPineBeetle>(pTree, weather.GetTRef(), EGG, FEMALE, bFertilEgg, m_generation + 1, scaleFactor));
			

			if (scaleFactorAttrition > 0)
			{
				CMountainPineBeetle* pBug = new CMountainPineBeetle(pTree, weather.GetTRef(), EGG, FEMALE, bFertilEgg, m_generation + 1, scaleFactorAttrition);
				pBug->m_status = DEAD;
				pBug->m_death = ATTRITION;
				pTree->push_back(CIndividualPtr(pBug));
			}
			//}
		}
	}

	// kills by attrition, old age, frost and overwintering
	// Output:  Individual's state is updated to follow update
	void CMountainPineBeetle::Die(const CWeatherDay& weather)
	{
		//Attrition and cold-tolerance mortality
		if (IsAlive())
		{
			if (IsDeadByAttrition())
			{
				m_status = DEAD;
				m_death = ATTRITION;
			}
			else if (IsDeadByFrost(weather.GetTRef(), weather[H_TMIN][MEAN], weather[H_SNDH][MEAN]))//snow depth [cm]
			{
				m_status = DEAD;
				m_death = FROZEN;
			}
		}
	}

	void CMountainPineBeetle::Develop(CTRef date, double T)
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
		double dt = GetTimeStep().NbSteps();

		size_t s = GetStage();

		double r = GetStand()->m_rates.GetRate(s, T) / dt;
		//double r = RATES_DEBUG.ComputeRate(s,T)/dt;

		double RR = 0;
		if (s < OVIPOSITING_ADULT)//all stages except OVIPOSITING_ADULTs
		{
			RR = max(0.0, m_relativeDevRate[s] * r);

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
				double brood = r*max(0.0, potentialFecundity - m_totalBroods); //all available broods

				//to avoid excessive longevity, if a female has laid 95% of her eggs, we force her to lay the remainder
				if ((m_totalBroods + brood) / potentialFecundity > 0.95)
					brood = potentialFecundity - m_totalBroods;

				m_broods += brood;
				m_totalBroods += brood;

				RR = brood / potentialFecundity;

				//because of packing, m_totalBrood and m_age is sometime desynchronized. We force them to die of old age.
				//			if( m_totalBrood/potentialFecundity >= 1)
				if (m_totalBroods / potentialFecundity >= 0.5) //This modification made to improve observed/simulated fit of adult emergence patterns 
					RR = DEAD_ADULT - m_age;

			}

			ASSERT(m_broods >= 0);
			ASSERT(RR >= 0 && RR < 1);
			ASSERT(RR > 0 || r == 0);
			ASSERT(!m_bEmerging);
			ASSERT(m_age >= 7);
			ASSERT(m_totalBroods < 2.5*CMPBDevelopmentTable::POTENTIAL_FECUNDITY);

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
				//CMountainPineBeetle* pBug = (CMountainPineBeetle*)pTree->FindObject(m_generation, GetStage(), DEAD, FROZEN, 5).get();
				CIndividualPtr pBug = pTree->FindObject(m_generation, GetStage(), DEAD, FROZEN, 5);

				if (pBug == NULL)
				{
					//if we don't find it, create a new dead object
					pBug = CreateCopy();//new CMountainPineBeetle(*this);
					pBug->SetScaleFactor(0);
					pBug->SetStatus(DEAD);
					pBug->SetDeath(FROZEN);
					pTree->push_back(pBug);
				}

				//update scalefactor
				pBug->SetScaleFactor(pBug->GetScaleFactor() + numberKilled);
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

	void CMountainPineBeetle::CompleteEmergence(size_t death)
	{
		_ASSERTE(GetStage() == OVIPOSITING_ADULT);
		_ASSERTE(m_broods == 0 && m_totalBroods == 0);

		//When all object have been developed, now complete emergence
		//some objects will be killed and some objects will survive
		if (death != DENSITY_DEPENDENCE)
			m_bAttackToday = true;


		m_bSuccessAttackToday = (death == HEALTHY);
		if (death == HEALTHY)
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
			size_t stage = GetStage();


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
				double pMort = 1 - min(1.0, max(0.0, (T - T2) / (T1 - T2)));
				if (m_PCritical < pMort)
					bFrost = true;

			}
			//Ovipositing adults as per Lester & Irwin 2012 (equation fitted by Régnière)
			else
			{
				if (GetStand()->m_applyColdToleranceOvipAdult == OA_KILL_COLD)
				{
					size_t t = date.GetJDay();
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

		size_t stage = GetStage();

		//total and daily brood
		stat[S_BROOD] += m_totalBroods*m_scaleFactor;
		stat[E_BROOD] += m_broods*m_scaleFactor;

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
					stat[E_TOTAL_BROOD] += m_scaleFactor*m_totalBroods;
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
	//CHost::Initialise((int)(numInd/**m_treeSize*/), peakAttackDay, sigma, age, bFertil, generation);
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
			double nbBugsAlive = GetNbSpecimenAlive();
			bool bUseDefenselessTree = GetStand()->m_bUseDefenselessTree && nbBugsAlive < A;

			//for (size_t i = size() - 1; i < size(); i--)
			for (reverse_iterator it = rbegin(); it !=  rend(); it++)
			{
				CMountainPineBeetle* pBug = (CMountainPineBeetle*)it->get();
				if (pBug->IsEmerging())
				{
					//apply density dependence here
					if (GetStand()->GetNbInfestedTrees() < GetStand()->GetNbTrees())
					{
						double lostBug = pBug->GetScaleFactor()*DDFactor;
						if (lostBug > 0)
						{
							ASSERT(lostBug < pBug->GetScaleFactor());
							pBug->SetScaleFactor(pBug->GetScaleFactor() - lostBug);

							CMountainPineBeetle* pBugCopy = new CMountainPineBeetle(*pBug);
							pBugCopy->SetScaleFactor(lostBug);
							pBugCopy->CompleteEmergence(CIndividual::DENSITY_DEPENDENCE);
							push_back(CIndividualPtr(pBugCopy));
						}


						if (!bUseDefenselessTree)
						{
							//if we exceed the forest capacity, we kill all others
							double nbTreeNonInfested = GetStand()->GetNbTrees() - (GetStand()->GetNbInfestedTrees() + GetNbInfestedTreesToday());
							double nbAttackMax = nbTreeNonInfested*m_Amax;
							if (pBug->GetScaleFactor() - A > nbAttackMax)
								A = pBug->GetScaleFactor() - nbAttackMax;


							//apply tree mortality
							if (A > 0 && pBug->GetScaleFactor() > A)
							{
								//we "unpack" the object to kill only the right number of bugs
								CMountainPineBeetle* pBugCopy = new CMountainPineBeetle(*pBug);
								pBugCopy->SetScaleFactor(A);
								pBugCopy->CompleteEmergence(CIndividual::TREE_DEFENSE);

								pBug->SetScaleFactor(pBug->GetScaleFactor() - A);
								push_back(CIndividualPtr(pBugCopy));
								A = 0;
							}

							ASSERT(pBug->GetScaleFactor() > 0);

							if (A > 0)
							{
								ASSERT(pBug->GetScaleFactor() <= A);
								pBug->CompleteEmergence(CIndividual::TREE_DEFENSE);
								A -= pBug->GetScaleFactor();
							}
							else
							{
								ASSERT(A <= 0);
								pBug->CompleteEmergence(CIndividual::HEALTHY);
								m_nbSucessfullAttacksToday += pBug->GetScaleFactor();



								ASSERT(GetStand()->GetNbInfestedTrees() + GetNbInfestedTreesToday() <= GetStand()->GetNbTrees());
							}
						}
						else
						{
							pBug->CompleteEmergence(CIndividual::HEALTHY);
						}
					}
					else
					{
						//if the 
						pBug->CompleteEmergence(CIndividual::DENSITY_DEPENDENCE);
					}
				}
			}
		}
	}

	
	//void CMPBTree::Live(const CTRef& day, const CWeatherDay& weaDay)
	//{
	//	CMPBStand* pStand = (CMPBStand*)GetStand();
	//	CDailyWaveVector hVector;// hourly temperature array 
	//	if (pStand->m_bMicroClimate)
	//	{
	//		CPOverheat overheat;
	//		weaDay.GetAllenWave(hVector, 16, gTimeStep, overheat);//16 is a factor of 4. Number usually used
	//	}
	//	else
	//	{
	//		weaDay.GetAllenWave(hVector, 16, gTimeStep);//16 is a factor of 4. Number usually used
	//	}

	//	Live(hVector, weaDay);

	//}


	void CMPBTree::Live(const CWeatherDay& weather)
	{
		m_emergingToday = 0;
		m_nbSucessfullAttacksToday = 0;

		//if (m_initialSize == -1)
		//	m_initialSize = m_bugs.size();

		//for (int i = (int)m_bugs.size() - 1; i >= 0; i--)
		//	m_bugs[i]->Live(hVector, weaDay);

		//CompleteEmergence();


		//for (int i = (int)m_bugs.size() - 1; i >= 0; i--)
		//	m_bugs[i]->Brood(hVector, weaDay);

	//	CWeatherDay weather = weatherIn;


		//CMPBStand* pStand = (CMPBStand*)GetStand();
		//CDailyWaveVector hVector;// hourly temperature array 
		//if (pStand->m_bMicroClimate)
		//{
		//	CPOverheat overheat;
		//	overheat.TransformWeather(weather);
		//}


		for (iterator it = begin(); it != end(); it++)
		{
			(*it)->OnNewDay(weather);
			if ((*it)->IsCreated(weather.GetTRef()))
			{
				if ((*it)->IsAlive())
				{
					(*it)->Live(weather);
				}
			}
		}

		CompleteEmergence();

		for (iterator it = begin(); it != end(); it++)
		{
			if ((*it)->IsCreated(weather.GetTRef()))
			{
				if ((*it)->GetSex() == FEMALE)
					(*it)->Brood(weather);

				(*it)->Die(weather);
			}
		}
	}

	CIndividualPtr CMPBTree::FindObject(size_t g, size_t stage, size_t status, size_t death, size_t deep)
	{
		CIndividualPtr pBug;
		reverse_iterator it = rbegin();
		for (size_t d = 0; d < deep && it != rend() && !pBug; d++)
		{
			if ((*it)->GetGeneration() == g && (*it)->GetStage() == stage && (*it)->GetStatus() == status && (*it)->GetDeath() == death)
				pBug = *it;
		}

		return pBug;
	}

	CMPBStand* CMPBTree::GetStand(){ ASSERT(m_pStand); return (CMPBStand*)m_pStand; }
	//*******************************************************************************************************
	//*******************************************************************************************************
	//CMPBStand
	void CMPBStand::Init(const CWeatherStation& weather)
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