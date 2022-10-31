//*****************************************************************************
//*****************************************************************************
// Class: CAplanipennis
//          
//
// Description: the CAplanipennis represents a group of EAB insect. scale by m_ScaleFactor
//*****************************************************************************
// 30/10/2022   Rémi Saint-Amant    Creation
//*****************************************************************************

#include "EABIndividualEquations.h"
#include "EABIndividual.h"
#include <boost/math/distributions/weibull.hpp>
#include <boost/math/distributions/logistic.hpp>

using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::EAB;


namespace WBSF
{

	//*********************************************************************************
	//CAplanipennis class


	//*****************************************************************************
	// Object creator
	//
	// Input: See CIndividual creator
	//
	// Note: m_RDR (relative Development Rate)  member is init with random values.
	//*****************************************************************************
	CAplanipennis::CAplanipennis(CHost* pHost, CTRef creationDate, double age, TSex sex, bool bFertil, size_t generation, double scaleFactor) :
		CIndividual(pHost, creationDate, age, sex, bFertil, generation, scaleFactor)
	{
		//reset creation date
		for (size_t s = 0; s < NB_STAGES; s++)
			m_RDR[s] = Equations().GetRelativeDevRate(s);

		//if (GetStand()->m_sigma > 0)
			//m_RDR[ADULT] = Equations().GetRelativeDevRate(GetStand()->m_sigma);
		//		m_Fi = (m_sex == FEMALE) ? Equations().GetFecondity() : 0;

	}

	CAplanipennis& CAplanipennis::operator=(const CAplanipennis& in)
	{
		if (&in != this)
		{
			CIndividual::operator=(in);

			//new relative development rate
			for (size_t s = 0; s < NB_STAGES; s++)
				m_RDR[s] = Equations().GetRelativeDevRate(s);

			m_adult_emergence = in.m_adult_emergence;
			m_reachDate = in.m_reachDate;
		}

		return *this;
	}

	//destructor
	CAplanipennis::~CAplanipennis(void)
	{}




	void CAplanipennis::OnNewDay(const CWeatherDay& weather)
	{
		CIndividual::OnNewDay(weather);
	}

	//*****************************************************************************
	// Develops all stages for one time step
	// Input:	weather: weather of the hour
	//			timeStep: timeStep [h]
	//*****************************************************************************
	void CAplanipennis::Live(const CHourlyData& weather, size_t timeStep)
	{
		assert(IsAlive());
		assert(m_status == HEALTHY);


		double nb_steps = (24.0 / timeStep);
		size_t h = weather.GetTRef().GetHour();
		size_t s = GetStage();

		double T = weather[H_TAIR];

		//Time step development rate
		double r = Equations().GetRate(s, T) / nb_steps;
		//if (s == ADULT)
			//r = r*GetStand()->m_psy_factor;

		//Relative development rate for this individual
		double rr = m_RDR[s];

		//Time step development rate for this individual
		r *= rr;
		ASSERT(r >= 0 && r < 1);

		//Adjust age
		m_age += r;


		//if (m_sex == FEMALE && GetStage() >= ACTIVE_ADULT)
		//{
		//	double to = 0;
		//	double t = timeStep / 24.0;
		//	double λ = Equations().GetFecondityRate(GetAge(), weather[H_TAIR]);
		//	double brood = m_Fi * (exp(-λ * (m_t - to)) - exp(-λ * (m_t + t - to)));

		//	m_broods += brood;
		//	m_totalBroods += brood;

		//	m_t += t;
		//}

	}




	//*****************************************************************************
	// Develops all stages, including adults
	// Input:	weather: the weather of the day
	//*****************************************************************************
	void CAplanipennis::Live(const CWeatherDay& weather)
	{
		CIndividual::Live(weather);

		ASSERT(IsCreated(weather.GetTRef()));

		if (!IsCreated(weather.GetTRef()))
			return;

		size_t nbSteps = GetTimeStep().NbSteps();
		for (size_t step = 0; step < nbSteps && IsAlive(); step++)
		{
			size_t h = step * GetTimeStep();
			Live(weather[h], GetTimeStep());
		}

		if (weather.GetTRef() == m_creationDate || HasChangedStage())
			m_reachDate[GetStage()] = weather.GetTRef();


	}


	void CAplanipennis::Brood(const CWeatherDay& weather)
	{
		assert(/*IsAlive() &&*/ m_sex == FEMALE);


		if (GetStage() == ADULT)
		{
			//no brood process done

			//brooding
			//m_broods = m_F;
			//m_totalBroods = m_F;
			//m_F = 0;
			//m_F -= m_F * r;
		}
	}

	// kills by old age and frost
	// Output:  Individual's state is updated to follow update
	void CAplanipennis::Die(const CWeatherDay& weather)
	{
		//attrition mortality. Killed at the end of time step 

		if (GetStage() == DEAD_ADULT)
		{
			//Old age
			m_status = DEAD;
			m_death = OLD_AGE;
		}
		else
		{
			size_t s = GetStage();

			if (s == ADULT && weather[H_TMIN][MEAN] < -5)
			{
				m_status = DEAD;
				m_death = FROZEN;
			}
		}
	}


	//s: stage
	//T: temperature for this time step
	//r: development rate for this time step
	//bool CAplanipennis::IsDeadByAttrition(size_t s, double T, double r)const
	//{
	//	bool bDeath = false;

	//	//daily survival
	//	double ds = GetStand()->m_equations.GetDailySurvivalRate(s, T);

	//	//time step survival
	//	double S = pow(ds, r);

	//	//Computes attrition (probability of survival in a given time step, based on development rate)
	//	if (RandomGenerator().RandUniform() > S)
	//		bDeath = true;

	//	return bDeath;
	//}



	//*****************************************************************************
	// GetStat gather information of this object
	//
	// Input: stat: the statistic object
	// Output: The stat is modified
	//*****************************************************************************
	void CAplanipennis::GetStat(CTRef d, CModelStat& stat)
	{
		if (IsCreated(d))
		{
			size_t s = GetStage();
			ASSERT(s <= DEAD_ADULT);

			if (IsAlive() || (s == DEAD_ADULT))
				stat[S_PUPAE + s] += m_scaleFactor;



			//if (HasChangedStatus() && m_status == DEAD && m_death == ATTRITION)
				//stat[S_DEAD_ATTRTION] += m_scaleFactor;

			if (HasChangedStage() && s == ADULT)
				stat[S_ADULT_EMERGENCE] += m_scaleFactor;


			//if (s == ACTIVE_ADULT)
			//{
			//	stat[S_ADULT_ABUNDANCE] += m_scaleFactor * m_adult_abundance;
			//}
		}
	}


	void CAplanipennis::Pack(const CIndividualPtr& pBug)
	{
		assert(m_sex == pBug->GetSex());

		CAplanipennis* in = (CAplanipennis*)(pBug.get());
		CIndividual::Pack(pBug);
	}

	double CAplanipennis::GetInstar(bool includeLast)const
	{
		return (IsAlive() || m_death == OLD_AGE) ? GetStage() : CBioSIMModelBase::VMISS;
	}

	//*********************************************************************************************************************

	//*********************************************************************************
	//CEABHost

	CEABHost::CEABHost(CStand* pStand) :
		CHost(pStand)
	{
	}


	void CEABHost::Live(const CWeatherDay& weather)
	{
		CHost::Live(weather);
	}

	void CEABHost::GetStat(CTRef d, CModelStat& stat, size_t generation)
	{
		CHost::GetStat(d, stat, generation);
	}

	//*************************************************
	//CEABStand

	CEABStand::CEABStand(WBSF::CBioSIMModelBase* pModel) :
		WBSF::CStand(pModel),
		m_equations(pModel->RandomGenerator())
	{
		m_sigma = 0;
		m_psy_factor = 1;
	}


}