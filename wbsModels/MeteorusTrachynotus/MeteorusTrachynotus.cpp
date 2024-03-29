﻿//*****************************************************************************
// Class: CMeteorusTrachynotus
//          
//
// Description: Biology of Meteorus trachynotus
//*****************************************************************************
#include "MeteorusTrachynotusEquations.h"
#include "MeteorusTrachynotus.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::MeteorusTrachynotus;

namespace WBSF
{

	//*********************************************************************************
	//CMeteorusTrachynotus class



	//*********************************************************************************
	// Object creator
	//
	// Input: See CIndividual creator
	//
	// Note: m_relativeDevRate member is initialized with random values.
	//*****************************************************************************
	CMeteorusTrachynotus::CMeteorusTrachynotus(CHost* pHost, CTRef creationDate, double age, WBSF::TSex sex, bool bFertil, size_t generation, double scaleFactor) :
		CIndividual(pHost, creationDate, age, sex, bFertil, generation, scaleFactor)
	{
		//host is actually unknowns, will be set later
		for (size_t s = 0; s < NB_STAGES; s++)
		{
			m_δ[s] = Equations().Getδ(s, generation);
		}

		//oviposition
		m_Pmax = Equations().GetPmax();

		//Individuals are created as non-diapause individuals
		m_Nh = 0;
	}



	CMeteorusTrachynotus& CMeteorusTrachynotus::operator=(const CMeteorusTrachynotus& in)
	{
		if (&in != this)
		{
			CIndividual::operator=(in);

			m_δ = in.m_δ;
			m_Pmax = in.m_Pmax;
			m_diapauseTRef = in.m_diapauseTRef;
		}

		return *this;
	}

	// Object destructor
	CMeteorusTrachynotus::~CMeteorusTrachynotus(void)
	{}


	//*****************************************************************************
	// Develops all stages, including adults
	// Input:	weather: the weather of the day
	//*****************************************************************************
	void CMeteorusTrachynotus::Live(const CWeatherDay& weather)
	{
		assert(IsAlive());
		assert(m_status == HEALTHY);
		assert(m_totalBroods + m_broods < m_Pmax);
		assert(m_broods == 0);//daily brood must be rest
		assert(m_creationDate.GetJDay() == 0 || GetHost()->m_diapause_age >= 1);
		
		CIndividual::Live(weather);

		//wait the end of host diapause before begginning Meteorus development
		if (m_creationDate.GetJDay() > 0 || GetHost()->m_diapause_age>=1)
		{
			double dayLength = weather.GetDayLength() / 3600.; //in hours
			CTRef TRef = weather.GetTRef();
			size_t JDay = TRef.GetJDay();
			size_t nbSteps = GetTimeStep().NbSteps();

			for (size_t step = 0; step < nbSteps&&m_age < DEAD_ADULT; step++)
			{
				size_t h = step * GetTimeStep();
				size_t s = GetStage();
				double T = weather[h][H_TAIR];

				//Relative development rate for time step
				double r = m_δ[s] * Equations().GetRate(s, m_generation, T) / nbSteps;

				//Adjust age
				if (weather.GetTRef().GetYear() != m_diapauseTRef.GetYear())
					m_age += r;


				if (!m_adultDate.IsInit() && m_age >= ADULT)
					m_adultDate = TRef;

				//compute brooding
				static const double pre_ovip_age = GetStand()->m_preOvip;


				if (s == ADULT && m_sex == FEMALE && GetStageAge() >= pre_ovip_age)
				{
					ASSERT(m_age >= ADULT);

					double wmax = 13.865 / nbSteps;
					double dtOnd20 = Equations().GetRate(ADULT, m_generation, T) / Equations().GetRate(ADULT, m_generation, 20.);
					double as = 2.8126;
					double th = 1.089e-5;

					//Densities adjusted to match densities in experimental conditions (petri dishes) with a factor of 16.67 (12*16.67=200)
					double w = as*(m_Nh/16.67) / (1 + as * th*(m_Nh/16.67)) * (1 - m_totalBroods / m_Pmax) * dtOnd20 / nbSteps; //Number of attacks per time step

					//eggs laid with successful attack is, at most, host find
					double broods = max(0.0, min(wmax, w));
					m_broods += broods;

					ASSERT(m_totalBroods + m_broods <= m_Pmax+0.01);
				}
			}//for all time steps


			//file.close();
			m_age = min(m_age, (double)DEAD_ADULT);
		}
	}



	// kills by attrition, old age and end of season
	// Output:  Individual's state is updated to follow update
	void CMeteorusTrachynotus::Die(const CWeatherDay& weather)
	{
		//attrition mortality. Killed at the end of time step 
		if (GetStage() == DEAD_ADULT)
		{
			//Old age
			m_status = DEAD;
			m_death = OLD_AGE;
		}
		else if (m_generation > 0 && weather[H_TMIN][MEAN] < GetStand()->m_lethalTemp && !m_diapauseTRef.IsInit())
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
	void CMeteorusTrachynotus::GetStat(CTRef d, CModelStat& stat)
	{
		if (IsCreated(d))
		{
			size_t s = GetStage();

			if (s >= ADULT)//individuals that reach adult stage (alive or dead)
				stat[S_CUMUL_REATCH_ADULT] += m_scaleFactor;

			if (IsAlive())
			{
				if (s < DEAD_ADULT)
					stat[S_IMMATURE + s] += m_scaleFactor;


				if (s == ADULT && m_sex == FEMALE && GetStageAge() >= GetStand()->m_preOvip)
				{
					stat[S_OVIPOSITING_ADULT] += m_scaleFactor;
				}

				if (m_diapauseTRef.IsInit())
					stat[S_DIAPAUSE] += m_scaleFactor;

				//because attrition is affected when the object change stage,
				//we need to take only insect alive
				if (GetStage() != GetLastStage())
				{
					stat[M_IMMATURE + s] += m_scaleFactor;
				}

				if (s == ADULT && m_sex == FEMALE && GetStageAge() >= GetStand()->m_preOvip)
					stat[M_OVIPOSITING_ADULT] += m_scaleFactor;
			}
			else
			{
				if (m_death == OLD_AGE)
				{
					stat[S_DEAD_ADULT] += m_scaleFactor;
					if (GetStage() != GetLastStage())
						stat[M_DEAD_ADULT] += m_scaleFactor;
				}


				if (m_death == ATTRITION)
					stat[S_ATTRITION] += m_scaleFactor;

				if (m_lastStatus == HEALTHY && m_status == DEAD && m_death == ATTRITION)
					stat[M_ATTRITION] += m_scaleFactor;

				if (m_lastStatus == HEALTHY && m_status == DEAD && m_death == FROZEN)
					stat[M_FROZEN] += m_scaleFactor;

				if (m_lastStatus == HEALTHY && m_status == DEAD && m_death == OTHERS)
					stat[M_OTHERS] += m_scaleFactor;
			}

			//if (m_lastAge < GetStand()->m_diapauseAge && m_age >= GetStand()->m_diapauseAge)
			if (d == m_diapauseTRef)
			{
				stat[M_DIAPAUSE] += m_scaleFactor;
				stat[M_DIAPAUSE_AGE] += m_scaleFactor * m_age;
			}
		}
	}

	
	bool CMeteorusTrachynotus::CanPack(const CIndividualPtr& in)const
	{
		CMeteorusTrachynotus* pIn = static_cast<CMeteorusTrachynotus*>(in.get());
		return CIndividual::CanPack(in) && (GetStage() != ADULT || GetSex() != FEMALE) && pIn->m_diapauseTRef.IsInit() == m_diapauseTRef.IsInit();
	}

	void CMeteorusTrachynotus::Pack(const CIndividualPtr& pBug)
	{
		CMeteorusTrachynotus* in = (CMeteorusTrachynotus*)pBug.get();
		CIndividual::Pack(pBug);
	}



	//*********************************************************************************************************************



	CMeteorusTrachynotusHost::CMeteorusTrachynotusHost(WBSF::CStand* pStand/*, size_t hostType*/) :WBSF::CHost(pStand)
	{
		m_diapause_age = 0; //actual state of overwintering post diapause host
		m_δ = OBL_Equations().Getδ(OBL_POST_DIAPAUSE);//Individual's relative overwintering post diapause host
	}

	void CMeteorusTrachynotusHost::Live(const CWeatherDay& weather)
	{
		if (m_diapause_age < 1)
		{
			CTRef TRef = weather.GetTRef();
			size_t JDay = TRef.GetJDay();
			size_t nbSteps = GetTimeStep().NbSteps();

			for (size_t step = 0; step < nbSteps; step++)
			{

				size_t h = step * GetTimeStep();
				double T = weather[h][H_TAIR];

				//Relative development rate for time step
				double r = OBL_Equations().GetRate(OBL_POST_DIAPAUSE, T) / nbSteps;
				ASSERT(r >= 0);

				m_diapause_age += r * m_δ;
			}//for all time steps
		}

		CHost::Live(weather);
	}

	//*********************************************************************************************************************

}