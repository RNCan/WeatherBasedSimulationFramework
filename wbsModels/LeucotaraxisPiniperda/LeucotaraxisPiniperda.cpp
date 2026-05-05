//*****************************************************************************
//*****************************************************************************
// Class: CLeucotaraxisPiniperda 
//          
//
// Description: the CLeucotaraxisPiniperda represents a group of insect. scale by m_ScaleFactor
//*****************************************************************************
// 01/05/2026   Rémi Saint-Amant    Bug correction in the attrition 
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

		m_bQuiescence = false;
		m_creationDate = creationDate;
		
		//if (age == PUPAE)
			//m_adult_emergence_date = GetAdultEmergence(year);

		for (size_t s = 0; s < NB_STAGES; s++)
			m_RDR[s] = Equations().GetRelativeDevRate(s);

		m_to = (m_sex == FEMALE) ? Equations().GetPreOvipPeriod() : 0.0;
		m_t = 0;
		m_Fi = (m_sex == FEMALE) ? Equations().GetFecundity(50.0 / m_RDR[ADULT]) : 0.0;
		m_bDeadByAttrition = false;


		//for Preston 2021 and Bittner 2024, Farley 2019 experiment, we trick the input to mimic the experimental protocol
		if (m_generation == 0)
		{

			static const size_t NB_OBS = 2;
			static const array<string, NB_OBS > LOC_NAME = { "ICF(NY)" };
			static const array<CTRef, NB_OBS > LOC_DATE = { CTRef(2026, APRIL, DAY_23)};

			//“ICF” 42.47144572941413, -76.5507394695125
			//“P” 42.4610547586098, -76.43973321766447
			//Release date, adult LA : 22 April 2026
			//Release date, adult LP : 23 April 2026

			for (size_t i = 0; i < NB_OBS; i++)
			{
				if (GetStand()->GetModel()->GetInfo().m_loc.m_ID == LOC_NAME[i] && creationDate.GetYear() == LOC_DATE[i].GetYear())
					m_adult_emergence_date = LOC_DATE[i];
			}
		}

	
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


		//Daily development rate
		double dr = (s == PUPAE)? Equations().GetPupaRate(T):Equations().GetRate(s, T) ;
		//Relative development rate for this individual
		double rdr = (s == PUPAE) ? Equations().GetPupaRDR() : m_RDR[s];



		//double time = 1.0 / dr;

		//if
			//dr = Equations().GetPupaRate(T);

		//Time step development rate
		double ts_r = dr / nb_steps;

		
				 
		//if (s == PUPAE )
			//rdr = Equations().GetPupaRDR();

		//Time step development rate for this individual
		double i_r = ts_r * rdr;


		//Time step development rate for this individual
		//r *= rr;
		ASSERT(i_r >= 0 && i_r < 1);

		if (!m_adult_emergence_date.IsInit())
		{
			//Adjust age
			m_age += i_r;
		}
		else
		{
			//for Bittner exception
			if (s == LARVAE && m_generation == 0)
			{
				if (weather.GetTRef().as(CTM::DAILY) == m_adult_emergence_date)
					m_age = ADULT;
			}
			else
			{
				m_age += i_r;
			}
		}
		

		//evaluate attrition once a day
		if (GetStand()->m_bApplyAttrition)
		{
			if (IsDeadByAttrition(s, T, i_r))
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

		

		size_t nbSteps = GetTimeStep().NbSteps();
		for (size_t step = 0; step < nbSteps && IsAlive() && m_age < DEAD_ADULT && !m_bQuiescence; step++)
		{
			size_t h = step * GetTimeStep();
			Live(weather[h], GetTimeStep());
			
			if (m_generation == 1 && GetStage() >= LARVAE)
				m_bQuiescence = true;
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


	//stage: stage
	//T: temperature for this time step
	//time_step: time step in (hour)
	//bool CLeucotaraxisPiniperda::IsDeadByAttrition(size_t stage, double T, size_t time_step)const
	//{
	//	bool bDeath = false;

	//	double d_s = Equations().GetDailySurvivalRate(stage, T);//daily survival
	//	double ts_s = pow(d_s, time_step / 24.0);

	//	//Computes attrition (probability of survival in a given time step)
	//	if (RandomGenerator().RandUniform() > ts_s)
	//		bDeath = true;

	//	return bDeath;
	//}

	//stage: stage
	//T: temperature for this time step
	//rr: time step development rate 
	bool CLeucotaraxisPiniperda::IsDeadByAttrition(size_t stage, double T, double rr)const
	{
		bool bDeath = false;

		//daily survival at this temperature
		double ds = Equations().GetDailySurvivalRate(stage, T);
		//overall (stage) survival at this temperature
		double S = pow(ds, 1 / Equations().GetRate(stage, T));

		//time step survival, limit at 1% survival to avoid kill all 
		double ts_s = pow(max(0.01, S), rr);

		//Computes attrition (probability of survival in a given time step, based on development rate)
		if (RandomGenerator().RandUniform() > ts_s)
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
				//else if (m_generation == 2)
					//stat[S_EGG2 + s] += m_scaleFactor;
			}


			if (HasChangedStatus() && m_status == DEAD && m_death == ATTRITION)
				stat[S_DEAD_ATTRITION] += m_scaleFactor;

			if (HasChangedStage())
			{
				if (s == ADULT )
					stat[S_EMERGENCE0+ m_generation] += m_scaleFactor;
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