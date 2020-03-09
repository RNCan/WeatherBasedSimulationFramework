//*****************************************************************************
// Class: CTranosema
//          
//
// Description: Biology of Tranosema rostrale
//*****************************************************************************
// 03/03/2020	Rémi Saint-Amant	bug correction in maximum adult longevity, was 25, now 150
//*****************************************************************************


#include "TranosemaEquations.h"
#include "Tranosema.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::Tranosema;

namespace WBSF
{

	//*********************************************************************************
	//CTranosema class



	//*********************************************************************************
	// Object creator
	//
	// Input: See CIndividual creator
	//
	// Note: m_relativeDevRate member is initialized with random values.
	//*****************************************************************************
	CTranosema::CTranosema(CHost* pHost, CTRef creationDate, double age, WBSF::TSex sex, bool bFertil, size_t generation, double scaleFactor) :
		CIndividual(pHost, creationDate, age, sex, bFertil, generation, scaleFactor)
	{
		// Each individual created gets the » attributes

		//Individual's "relative" development rate for each life stage
		//These are independent in successive life stages
		for (size_t s = 0; s < NB_STAGES; s++)
		{
			m_δ[s] = Equations().Getδ(s);
			//Stage-specific survival random draws
			m_luck[s] = Equations().GetLuck(s);
		}

		//oviposition
		//Random values of Pmax and Eº
		m_Pmax = Equations().GetPmax();
		double Eº = Equations().GetEº();
		//Initial values
		m_Pᵗ = Eº;
		m_Eᵗ = Eº;

		//Individuals are created as non-diapause individuals
		//m_bDiapause = false;
		m_badluck = false;
		m_Nh = 100000;
	}



	CTranosema& CTranosema::operator=(const CTranosema& in)
	{
		if (&in != this)
		{
			CIndividual::operator=(in);

			m_δ = in.m_δ;
			m_Pmax = in.m_Pmax;
			m_Pᵗ = in.m_Pᵗ;
			m_Eᵗ = in.m_Eᵗ;
			m_luck = in.m_luck;
			m_diapauseTRef = in.m_diapauseTRef;
			m_badluck = in.m_badluck;
		}

		return *this;
	}

	// Object destructor
	CTranosema::~CTranosema(void)
	{}


	//*****************************************************************************
	// Develops all stages, including adults
	// Input:	weather: the weather of the day
	//*****************************************************************************
	void CTranosema::Live(const CWeatherDay& weather)
	{
		assert(IsAlive());
		assert(m_status == HEALTHY);

		CIndividual::Live(weather);

		double dayLength = weather.GetDayLength() / 3600.; //in hours
		CTRef TRef = weather.GetTRef();
		size_t JDay = TRef.GetJDay();
		size_t nbSteps = GetTimeStep().NbSteps();

		/*WBSF::ofStream file;
		if (JDay == 0)
		{
			file.open("g:/Tranosema.csv");
			file.imbue(std::locale(std::locale::classic(), new std::codecvt_utf8<size_t>()));
			file << u8"Year,Month,Day,Hour,Pmax,broods,Total,Oᵗ,Rᵗ,Nh,Na,Pᵗ,Eᵗ,Bust" << endl;
			file.close();
		}*/

		
		
		
		for (size_t step = 0; step < nbSteps&&m_age<DEAD_ADULT; step++)
		{
			size_t h = step*GetTimeStep();
			size_t s = GetStage();
			double T = weather[h][H_TAIR];

			//Relative development rate for time step
			double r = m_δ[s] * Equations().GetRate(s, T) / nbSteps;
			
			//Check if individual enters diapause this time step
			
			if (GetStand()->m_bAutoComputeDiapause)
			{
				if (m_age < GetStand()->m_diapauseAge && (m_age + r) > GetStand()->m_diapauseAge)
				{
					//Individual crosses the m_diapauseAge threshold this time step, and post-solstice daylength is shorter than critical daylength
					if (JDay > 173 && dayLength < GetStand()->m_criticalDaylength)
					{
						m_diapauseTRef = weather.GetTRef();
						//m_bDiapause = true;
						m_age = GetStand()->m_diapauseAge; //Set age exactly to diapause age (development stops precisely there until spring...
					}
				}
			}
			
			if (s == ADULT) //Set maximum longevity's to 150 days
				r = max(1.0 / (150 * nbSteps), r);//By RSA 03/03/2020
				//r = max(0.00667, r);

			if (GetStand()->m_bApplyAttrition)
			{
				if (IsChangingStage(r))
					m_badluck = RandomGenerator().Randu() > m_luck[s];
				else
					m_badluck = IsDeadByAttrition(s, T);
			}


			//Adjust age
			//if(!m_bDiapause)
			if (weather.GetTRef().GetYear() != m_diapauseTRef.GetYear())
				m_age += r;

			//compute brooding
			if (m_sex == FEMALE && m_age >= ADULT)
			{
				//if (!file.is_open())
					//file.open("g:/Tranosema.csv", ios::out | ios::app);

				//Ot: rate of oogenesis
				//Rᵗ: rate of resorption
				double Oᵗ = max(0.0, ((m_Pmax - m_Pᵗ) / m_Pmax)*Equations().GetOᵗ(T)) / nbSteps;
				double Rᵗ = max(0.0, (m_Pᵗ / m_Pmax)*Equations().GetRᵗ(T)) / nbSteps;

	//			Possible host attack module here
				double as = 0.05;
				double th = 0.8;
				double Nh = m_Nh / nbSteps;  // Number of hosts (C. rosaceana) that are in larval stages, excluding L3D;
				double Na=as*Nh*Equations().GetOᵗ(T)/(1+as*th*Nh);
				
				//CTRef TRef2 = TRef.as(CTM::HOURLY) + h;
				//file << TRef2.GetFormatedString() << "," << m_Pmax << "," << m_broods << "," << (m_totalBroods + m_broods) << "," << Oᵗ << "," << Rᵗ << "," << Nh << "," << Na << "," << m_Pᵗ << "," << m_Eᵗ << "," << ((m_totalBroods + m_broods) > m_Pmax ? "1" : "0") << endl;
				
				//the actual number of eggs laid is, at most, Attacks, at least m_Eᵗ + Oᵗ - Rᵗ:
				double broods = max(0.0, min(m_Eᵗ + Oᵗ - Rᵗ, Na));

				//m_Pᵗ: egg production
				//m_Eᵗ: eggs in the oviducts
				m_Pᵗ = max(0.0, m_Pᵗ + Oᵗ - 0.8904*Rᵗ);
				m_Eᵗ = max(0.0, m_Eᵗ + Oᵗ - Rᵗ - broods);//correction 09/03/2020

				//adjust daily brood
				m_broods += broods;
				ASSERT(m_totalBroods + m_broods < m_Pmax*1.5);
			}
		}

		//file.close();

		m_age = min(m_age, (double)DEAD_ADULT);
	}


	void CTranosema::Brood(const CWeatherDay& weather)
	{
		ASSERT(IsAlive() && m_sex == FEMALE);
		ASSERT(m_totalBroods <= m_Pmax*1.5);

		m_totalBroods += m_broods;

		//Oviposition module after Régniere 1983
		if (m_bFertil && m_broods > 0)
		{
			ASSERT(m_age >= ADULT);
			CTranosemaStand* pStand = GetStand(); ASSERT(pStand);

			double attRate = GetStand()->m_bApplyAttrition ? pStand->m_generationAttrition : 1;//10% of survival by default
			double scaleFactor = m_broods*m_scaleFactor*attRate;
			CIndividualPtr object = make_shared<CTranosema>(m_pHost, weather.GetTRef(), EGG, FEMALE, true, m_generation + 1, scaleFactor);
			m_pHost->push_front(object);
		}
	}

	// kills by attrition, old age and end of season
	// Output:  Individual's state is updated to follow update
	void CTranosema::Die(const CWeatherDay& weather)
	{
		//attrition mortality. Killed at the end of time step 
		if (GetStage() == DEAD_ADULT)
		{
			//Old age
			m_status = DEAD;
			m_death = OLD_AGE;
		}
		else if (m_badluck)
		{
			//kill by attrition
			m_status = DEAD;
			m_death = ATTRITION;
		}
		else if (m_generation>0 && weather[H_TMIN][MEAN] < GetStand()->m_lethalTemp && !m_diapauseTRef.IsInit())
		{
			m_status = DEAD;
			m_death = FROZEN;
		}
		else if (!m_diapauseTRef.IsInit() && weather.GetTRef().GetMonth() == DECEMBER && weather.GetTRef().GetDay() == DAY_31)
		{
			//all individual not in diapause are kill at the end of the season
			m_status = DEAD;
			m_death = OTHERS;
		}
	}

	//*****************************************************************************
	// GetStat gather information of this object
	//
	// Input: stat: the statistic object
	// Output: The stat is modified
	//*****************************************************************************
	void CTranosema::GetStat(CTRef d, CModelStat& stat)
	{
		if (IsCreated(d))
		{
			size_t s = GetStage();
			stat[S_BROOD] += m_broods*m_scaleFactor;
			stat[E_BROOD] += m_broods*m_scaleFactor; //E_BROOD is the same as S_BROOD
			
			


			if (s >= ADULT)//individuals that reach adult stage (alive or dead)
				stat[S_CUMUL_REATCH_ADULT] += m_scaleFactor;

			if (IsAlive())
			{
				if (s >= EGG && s < DEAD_ADULT)
					stat[S_EGG+s] += m_scaleFactor;


				if (s == ADULT)
				{
					if (m_sex == FEMALE)
					{
						stat[S_OVIPOSITING_ADULT] += m_scaleFactor;
					}
				}
				
				if (m_diapauseTRef.IsInit())
					stat[S_DIAPAUSE] += m_scaleFactor;

				//because attrition is affected when the object change stage,
				//we need to take only insect alive
				if (GetStage() != GetLastStage())
				{
					stat[E_EGG + s] += m_scaleFactor;
					if (s == ADULT && m_sex == FEMALE)
						stat[E_OVIPOSITING_ADULT] += m_scaleFactor;
				}
			}
			else
			{
				if (m_death == OLD_AGE)
				{
					stat[S_DEAD_ADULT] += m_scaleFactor;
					if (GetStage() != GetLastStage())
						stat[E_DEAD_ADULT] += m_scaleFactor;
				}


				if (m_death == ATTRITION)
					stat[S_ATTRITION] += m_scaleFactor;

				if (m_lastStatus == HEALTHY && m_status == DEAD && m_death == ATTRITION)
					stat[E_ATTRITION] += m_scaleFactor;

				if (m_lastStatus == HEALTHY && m_status == DEAD && m_death == FROZEN)
					stat[E_FROZEN] += m_scaleFactor;

				if (m_lastStatus == HEALTHY && m_status == DEAD && m_death == OTHERS)
					stat[E_OTHERS] += m_scaleFactor;
			}

			//if (m_lastAge < GetStand()->m_diapauseAge && m_age >= GetStand()->m_diapauseAge)
			if (d == m_diapauseTRef)
			{
				stat[E_DIAPAUSE] += m_scaleFactor;
				stat[E_DIAPAUSE_AGE] += m_scaleFactor*m_age;
				
			}
		}
	}

	//*****************************************************************************
	// IsDeadByAttrition is for one time step development
	// Output: TRUE if the insect dies, FALSE otherwise
	//*****************************************************************************
	bool CTranosema::IsDeadByAttrition(size_t s, double T)
	{
		bool bDeath = false;


		//Computes attrition (probability of survival in a given time step, based on daily rate)
		double survival = pow(Equations().GetSurvivalRate(s, T), 1.0 / GetTimeStep().NbSteps());
		if (RandomGenerator().Randu() > survival)
			bDeath = true;

		return bDeath;
	}

	bool CTranosema::CanPack(const CIndividualPtr& in)const
	{
		CTranosema* pIn = static_cast<CTranosema*>(in.get());
		return CIndividual::CanPack(in) && (GetStage() != ADULT || GetSex() != FEMALE) && pIn->m_diapauseTRef.IsInit() == m_diapauseTRef.IsInit();
	}

	void CTranosema::Pack(const CIndividualPtr& pBug)
	{
		CTranosema* in = (CTranosema*)pBug.get();

		//do not make the mean. take the first
		//m_Pmax = (m_Pmax*m_scaleFactor + in->m_Pmax*in->m_scaleFactor) / (m_scaleFactor + in->m_scaleFactor);
		//m_Pᵗ = (m_Pᵗ*m_scaleFactor + in->m_Pᵗ*in->m_scaleFactor) / (m_scaleFactor + in->m_scaleFactor);
		//m_Eᵗ = (m_Eᵗ*m_scaleFactor + in->m_Eᵗ*in->m_scaleFactor) / (m_scaleFactor + in->m_scaleFactor);

		CIndividual::Pack(pBug);
	}

	//*********************************************************************************************************************
}