//*****************************************************************************
// Class: CObliqueBandedLeafroller
//          
//
// Description: Biology of Tranosema rostrale
//*****************************************************************************
// 22/01/2016	Rémi Saint-Amant	Using Weather-Based Simulation Framework (WBSF)
// 11/12/2015   Rémi Saint-Amant    Creation from paper
//*****************************************************************************

#include "ObliqueBandedLeafrollerEquations.h"
#include "ObliqueBandedLeafroller.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::OBL;


namespace WBSF
{

	//*********************************************************************************
	//CObliqueBandedLeafroller class



	//*********************************************************************************
	// Object creator
	//
	// Input: See CIndividual creator
	//
	// Note: m_relativeDevRate member is initialized with random values.
	//*****************************************************************************
	CObliqueBandedLeafroller::CObliqueBandedLeafroller(CHost* pHost, CTRef creationDate, double age, size_t sex, bool bFertil, size_t generation, double scaleFactor) :
		CIndividual(pHost, creationDate, age, sex, bFertil, generation, scaleFactor)
	{
		// Each individual created gets the » attributes

		//Individual's "relative" development rate for each life stage
		//These are independent in successive life stages
		for (size_t s = 0; s < NB_STAGES; s++)
		{
			m_δ[s] = Equations().Getδ(s);
		}

		
		//Individuals are created as non-diapause individuals
		m_bRequireDiapause = FALSE;
		m_ovipAge = 0;

	}



	CObliqueBandedLeafroller& CObliqueBandedLeafroller::operator=(const CObliqueBandedLeafroller& in)
	{
		if (&in != this)
		{
			CIndividual::operator=(in);

			m_δ = in.m_δ;
		}

		return *this;
	}

	// Object destructor
	CObliqueBandedLeafroller::~CObliqueBandedLeafroller(void)
	{}


	//*****************************************************************************
	// Develops all stages, including adults
	// Input:	weather: the weather of the day
	//*****************************************************************************
	void CObliqueBandedLeafroller::Live(const CWeatherDay& weather)
	{
		assert(IsAlive());
		assert(m_status == HEALTHY);

		CIndividual::Live(weather);

		double DayLength = weather.GetDayLength() / 3600.; //in hours
		CTRef TRef = weather.GetTRef();
		size_t JDay = TRef.GetJDay();
		size_t nbSteps = GetTimeStep().NbSteps();
	
		for (size_t step = 0; step < nbSteps&&m_age<DEAD_ADULT; step++)
		{
			size_t h = step*GetTimeStep();
			size_t s = GetStage();
			double T = weather[h][H_TAIR];

			//Relative development rate for time step
			double r = m_δ[s] * Equations().GetRate(s, m_sex, T) / nbSteps;

			//Check if individual's diapause trigger is set this time step. As soon as an L1 or first 50% of L2 is exposed to short daylength after solstice, diapause is set. It occurs in the L3D stage
			if ((s == L1 || (s == 2 && m_age < 2.5)) && JDay > 173 && DayLength < GetStand()->m_criticalDaylength)
				m_bRequireDiapause = TRUE;
			
            
			//Check if this individual enters diapause this time step (it was triggered in the L1 or L2). If it does, it skips L3, becomes L3D and stops developing
			if (s == L2 && IsChangingStage(r) && m_bRequireDiapause)
			{
				m_diapauseTRef = TRef;
				m_age = L3D;
			}

			//Skip the L3D stage in non-diapausing individuals
			if (s == L3 && IsChangingStage(r) && !m_diapauseTRef.IsInit())
			{
				m_age += 1;
			}

			//skip ovip adult for male
			if (s == PUPA && IsChangingStage(r) && m_sex == MALE)
			{
				m_age += 1;
			}

			//Diapausing L3D do not develop
			if (s == L3D && TRef.GetYear() == m_diapauseTRef.GetYear())
				r = 0;

			//Adjust age
			m_age += r;

			
			//compute brooding only once per day
			if (m_sex == FEMALE && GetStage() == ADULT )
			{
				double Eᵗ = Equations().GetEᵗ(m_ovipAge, m_ovipAge + r);
				m_broods += Eᵗ;
				m_ovipAge += r;
			}
		}

		m_age = min(m_age, (double)DEAD_ADULT);
	}


	void CObliqueBandedLeafroller::Brood(const CWeatherDay& weather)
	{
		ASSERT(IsAlive() && m_sex == FEMALE);
		//ASSERT(m_totalBroods <= m_Pmax+1);

		m_totalBroods += m_broods;

		//Oviposition module after Régniere 1983
		if (m_bFertil && m_broods > 0)
		{
			ASSERT(m_age >= ADULT);
			CObliqueBandedLeafrollerStand* pStand = GetStand(); ASSERT(pStand);

			double attRate = GetStand()->m_bApplyAttrition ? pStand->m_generationAttrition : 1;//1% of survival by default
			double scaleFactor = m_broods*m_scaleFactor*attRate;
			CIndividualPtr object = make_shared<CObliqueBandedLeafroller>(m_pHost, weather.GetTRef(), EGG, RANDOM_SEX, true, m_generation + 1, scaleFactor);
			m_pHost->push_front(object);
		}
	}

	// kills by old age and frost
	// Output:  Individual's state is updated to follow update
	void CObliqueBandedLeafroller::Die(const CWeatherDay& weather)
	{
		//attrition mortality. Killed at the end of time step 
		if (GetStage() == DEAD_ADULT)
		{
			//Old age
			m_status = DEAD;
			m_death = OLD_AGE;
		}
		else if (!m_bRequireDiapause && GetStage() != L3D && weather[H_TMIN][MEAN] < GetStand()->m_lethalTemp)
		{
			m_status = DEAD;
			m_death = FROZEN;
		}
	}

	//*****************************************************************************
	// GetStat gather information of this object
	//
	// Input: stat: the statistic object
	// Output: The stat is modified
	//*****************************************************************************
	void CObliqueBandedLeafroller::GetStat(CTRef d, CModelStat& stat)
	{
		if (IsCreated(d))
		{
			//if (GetGeneration() == 0)//test temporaire RSA
			//{
				size_t s = GetStage();
				stat[S_BROOD] += m_broods*m_scaleFactor;


				if (IsAlive())
				{
					if (s >= EGG && s < DEAD_ADULT)
						stat[s] += m_scaleFactor;


					if (s == ADULT)
					{
						if (m_sex == FEMALE)
						{
							stat[S_OVIPOSITING_ADULT] += m_scaleFactor;
						}
					}
				}
				else
				{
					if (m_death == OLD_AGE)
						stat[DEAD_ADULT] += m_scaleFactor;

					if (m_death == FROZEN)
						stat[S_FROZEN] += m_scaleFactor;
				}


				if (GetStage() != GetLastStage())
				{
					stat[E_EGG + s] += m_scaleFactor;
					if (s == ADULT && m_sex == FEMALE)
						stat[E_OVIPOSITING_ADULT] += m_scaleFactor;

				}

			//}
		}
	}

	//*****************************************************************************
	// IsDeadByAttrition is for one time step development
	// Output: TRUE if the insect dies, FALSE otherwise
	//*****************************************************************************
	//bool CObliqueBandedLeafroller::IsDeadByAttrition(size_t s, double T)
	//{
	//	bool bDeath = false;


	//	//Computes attrition (probability of survival in a given time step, based on daily rate)
	//	double survival = pow(Equations().GetSurvivalRate(s, T), 1.0 / GetTimeStep().NbSteps());
	//	if (RandomGenerator().Randu() > survival)
	//		bDeath = true;

	//	return bDeath;
	//}

	bool CObliqueBandedLeafroller::CanPack(const CIndividualPtr& in)const
	{
		CObliqueBandedLeafroller* pIn = static_cast<CObliqueBandedLeafroller*>(in.get());
		return CIndividual::CanPack(in) && (GetStage() != ADULT || GetSex() != FEMALE);
	}

	void CObliqueBandedLeafroller::Pack(const CIndividualPtr& pBug)
	{
		CObliqueBandedLeafroller* in = (CObliqueBandedLeafroller*)pBug.get();

		//m_Pmax = (m_Pmax*m_scaleFactor + in->m_Pmax*in->m_scaleFactor) / (m_scaleFactor + in->m_scaleFactor);
		//m_Pᵗ = (m_Pᵗ*m_scaleFactor + in->m_Pᵗ*in->m_scaleFactor) / (m_scaleFactor + in->m_scaleFactor);
		//m_Eᵗ = (m_Eᵗ*m_scaleFactor + in->m_Eᵗ*in->m_scaleFactor) / (m_scaleFactor + in->m_scaleFactor);

		CIndividual::Pack(pBug);
	}

	//*********************************************************************************************************************
}