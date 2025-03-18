//*****************************************************************************
//*****************************************************************************
// Class: CLeucotaraxisPiniperda 
//          
//
// Description: the CLeucotaraxisPiniperda represents a group of LNF insect. scale by m_ScaleFactor
//*****************************************************************************
// 18/10/2022   Rémi Saint-Amant    Creation
//*****************************************************************************

#include "LeucotaraxisPiniperdaEquations.h"
#include "LeucotaraxisPiniperda.h"
#include "Basic/DegreeDays.h"
#include <boost/math/distributions/weibull.hpp>
#include <boost/math/distributions/logistic.hpp>

using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::LPM;


namespace WBSF
{

	//static const bool CALIBRATE_PUPAE = true;
	//*********************************************************************************
	//CLeucotaraxisPiniperda class


	//*****************************************************************************
	// Object creator
	//
	// Input: See CIndividual creator
	//
	// Note: m_RDR (relative Development Rate)  member is init with random values.
	//*****************************************************************************
	CLeucotaraxisPiniperda::CLeucotaraxisPiniperda(CHost* pHost, CTRef creationDate, double age, TSex sex, bool bFertil, size_t generation, double scaleFactor) :
		CIndividual(pHost, creationDate, age, sex, bFertil, generation, scaleFactor)
	{
		//ASSERT(age == EGG || age == PUPAE);
		ASSERT(age == EGG || floor(age) == LARVAE);
		//reset creation date
		int year = creationDate.GetYear();

		m_bDiapause = false;
		//m_bDiapause = age == PUPAE;
		m_creationDate = creationDate;
		
		//if (age == PUPAE)
			//m_adult_emergence_date = GetAdultEmergence(year);

		for (size_t s = 0; s < NB_STAGES; s++)
			m_RDR[s] = Equations().GetRelativeDevRate(s);

		m_to = (m_sex == FEMALE) ? Equations().GetPreOvipPeriod() : 0.0;
		m_t = 0;
		m_Fi = (m_sex == FEMALE) ? Equations().GetFecundity(50.0 / m_RDR[ADULT]) : 0.0;
		m_bDeadByAttrition = false;
	}


	CTRef CLeucotaraxisPiniperda::GetAdultEmergence(int year)const
	{
		const CWeatherStation& weather_station = GetStand()->GetModel()->m_weather;
		CTPeriod p = weather_station[year].GetEntireTPeriod(CTM::DAILY);

		CTRef adult_emergence;
		double adult_emerging_CDD = Equations().GetAdultEmergingCDD();
		const CModelStatVector& CDD = GetStand()->m_adult_emergence_CDD;

		for (CTRef TRef = p.Begin(); TRef <= p.End() && !adult_emergence.IsInit(); TRef++)
		{
			if (CDD[TRef][0] >= adult_emerging_CDD)
			{
				adult_emergence = TRef;
			}
		}

		assert(adult_emergence.IsInit());
		return adult_emergence;
	}

	CLeucotaraxisPiniperda& CLeucotaraxisPiniperda::operator=(const CLeucotaraxisPiniperda& in)
	{
		if (&in != this)
		{
			CIndividual::operator=(in);

			//new relative development rate
			for (size_t s = 0; s < NB_STAGES; s++)
				m_RDR[s] = Equations().GetRelativeDevRate(s);


			m_adult_emergence_date = in.m_adult_emergence_date;
			m_reachDate = in.m_reachDate;
			m_to = in.m_to;
			m_t = in.m_t;
			m_Fi = in.m_Fi;
			m_bDeadByAttrition = in.m_bDeadByAttrition;
		}

		return *this;
	}

	//destructor
	CLeucotaraxisPiniperda::~CLeucotaraxisPiniperda(void)
	{}




	void CLeucotaraxisPiniperda::OnNewDay(const CWeatherDay& weather)
	{
		CIndividual::OnNewDay(weather);
	}

	//*****************************************************************************
	// Develops all stages for one time step
	// Input:	weather: weather of the hour
	//			timeStep: timeStep [h]
	//*****************************************************************************
	void CLeucotaraxisPiniperda::Live(const CHourlyData& weather, size_t timeStep)
	{
		assert(IsAlive());
		assert(m_status == HEALTHY);

		CLPMHost* pHost = GetHost();
		CLPMStand* pStand = GetStand();

		double nb_steps = (24.0 / timeStep);
		size_t h = weather.GetTRef().GetHour();
		size_t s = GetStage();

		double T = weather[H_TAIR];


		//Time step development rate
		double r = Equations().GetRate(s, T) / nb_steps;
		double time = 1.0 / r;

		//if (s == PUPAE)
			//r *= Equations().m_C_param[1];//for test only
		if (s == PUPAE )
			r = Equations().GetPupaRate(T) / nb_steps;


		//Relative development rate for this individual
		double rr = m_RDR[s];
		//if (s == PUPAE)
			//rr *= Equations().m_C_param[2];//for test only
				
				 
		if (s == PUPAE )
			rr = Equations().GetPupaRDR();

		


		//Time step development rate for this individual
		r *= rr;
		ASSERT(r >= 0 && r < 1);

		//Adjust age
		m_age += r;

		//evaluate attrition once a day
		if (GetStand()->m_bApplyAttrition)
		{
			if (IsDeadByAttrition(s, T, r))
				m_bDeadByAttrition = true;
		}


		if (m_sex == FEMALE && GetAge() >= ADULT&& m_Fi > 0)
		{

			double t = timeStep / 24.0;
			if (m_t > m_to)
			{
				double λ = 0.15;//Based on 10 000 random longevity, fecundity and pre-oviposition period
				double brood = m_Fi * (exp(-λ * (m_t - m_to)) - exp(-λ * (m_t + t - m_to)));
				ASSERT(brood >= 0);

				m_broods += brood;
				m_totalBroods += brood;
			}

			m_t += t;
		}

	}




	//*****************************************************************************
	// Develops all stages, including adults
	// Input:	weather: the weather of the day
	//*****************************************************************************
	void CLeucotaraxisPiniperda::Live(const CWeatherDay& weather)
	{
		CIndividual::Live(weather);

		ASSERT(IsCreated(weather.GetTRef()));


		/*if (m_bDiapause && GetStage() == PUPAE && weather.GetTRef() == m_adult_emergence_date)
		{
			ASSERT(m_generation == 0);
			m_bDiapause = false;
			m_age = ADULT;
		}*/

		

		size_t nbSteps = GetTimeStep().NbSteps();
		for (size_t step = 0; step < nbSteps && IsAlive() && m_age < DEAD_ADULT && !m_bDiapause; step++)
		{
			size_t h = step * GetTimeStep();
			Live(weather[h], GetTimeStep());
			//if (m_generation == 1 && GetStage() >= PUPAE)
			if (m_generation == 1 && GetStage() >= LARVAE)
				m_bDiapause = true;
		}


		if (weather.GetTRef() == m_creationDate || HasChangedStage())
			m_reachDate[GetStage()] = weather.GetTRef();


	}


	void CLeucotaraxisPiniperda::Brood(const CWeatherDay& weather)
	{
		assert(m_sex == FEMALE);


		if (m_broods > 0 && !GetStand()->m_in_calibration)
		{
			ASSERT(m_generation == 0);
			ASSERT(m_age >= ADULT);
			CLPMStand* pStand = GetStand(); ASSERT(pStand);

			double attRate = 0.05;//5% of survival by default
			double scaleFactor = m_broods * m_scaleFactor * attRate;
			CIndividualPtr object = make_shared<CLeucotaraxisPiniperda>(m_pHost, weather.GetTRef(), EGG, RANDOM_SEX, true, m_generation + 1, scaleFactor);
			m_pHost->push_front(object);
		}
	}

	// kills by old age and frost
	// Output:  Individual's state is updated to follow update
	void CLeucotaraxisPiniperda::Die(const CWeatherDay& weather)
	{
		//attrition mortality. Killed at the end of time step 

		if (m_bDeadByAttrition)
		{
			m_status = DEAD;
			m_death = ATTRITION;
		}
		else if (GetStage() == DEAD_ADULT)
		{
			//Old age
			m_status = DEAD;
			m_death = OLD_AGE;
		}
		else
		{
			
		}
	}


	//s: stage
	//T: temperature for this time step
	//r: development rate for this time step
	bool CLeucotaraxisPiniperda::IsDeadByAttrition(size_t s, double T, double r)const
	{
		bool bDeath = false;

		//daily survival
		double ds = GetStand()->m_equations.GetDailySurvivalRate(s, T);

		//time step survival
		double S = pow(ds, r);

		//Computes attrition (probability of survival in a given time step, based on development rate)
		if (RandomGenerator().RandUniform() > S)
			bDeath = true;

		return bDeath;
	}



	//*****************************************************************************
	// GetStat gather information of this object
	//
	// Input: stat: the statistic object
	// Output: The stat is modified
	//*****************************************************************************
	void CLeucotaraxisPiniperda::GetStat(CTRef d, CModelStat& stat)
	{
		if (IsCreated(d))
		{
			size_t s = GetStage();
			ASSERT(s <= DEAD_ADULT);

			if (IsAlive() || (s == DEAD_ADULT))
			{
				if (m_generation == 0)
					stat[S_LARVA0 + s - LARVAE] += m_scaleFactor;
				else if (m_generation == 1)
					stat[S_EGG1 + s] += m_scaleFactor;
			}


			if (HasChangedStatus() && m_status == DEAD && m_death == ATTRITION)
				stat[S_DEAD_ATTRITION] += m_scaleFactor;

			if (HasChangedStage())
			{
				if (s == ADULT && m_generation == 0)
					stat[S_EMERGENCE0] += m_scaleFactor;
			}
		}
	}


	void CLeucotaraxisPiniperda::Pack(const CIndividualPtr& pBug)
	{
		CLeucotaraxisPiniperda* in = (CLeucotaraxisPiniperda*)(pBug.get());
		CIndividual::Pack(pBug);
	}

	double CLeucotaraxisPiniperda::GetInstar(bool includeLast)const
	{
		return (IsAlive() || m_death == OLD_AGE) ? GetStage() : CBioSIMModelBase::VMISS;
	}

	//*********************************************************************************************************************

	//*********************************************************************************
	//CLPMHost

	CLPMHost::CLPMHost(CStand* pStand) :
		CHost(pStand)
	{
	}


	void CLPMHost::Live(const CWeatherDay& weather)
	{
		CHost::Live(weather);
	}

	void CLPMHost::GetStat(CTRef d, CModelStat& stat, size_t generation)
	{
		CHost::GetStat(d, stat, generation);
	}

	//*************************************************
	//CLPMStand

	CLPMStand::CLPMStand(WBSF::CBioSIMModelBase* pModel) :
		WBSF::CStand(pModel),
		m_equations(pModel->RandomGenerator())
	{
		m_bApplyAttrition = false;
		m_in_calibration = false;
	}


	void CLPMStand::init(int year, const CWeatherYears& weather)
	{
		m_equations.GetAdultEmergenceCDD(weather, m_adult_emergence_CDD);
	}



	void CLPMStand::GetStat(CTRef d, CModelStat& stat, size_t generation)
	{
		CStand::GetStat(d, stat, generation);

		stat[S_CDD] = m_adult_emergence_CDD[d][0];
	}


}