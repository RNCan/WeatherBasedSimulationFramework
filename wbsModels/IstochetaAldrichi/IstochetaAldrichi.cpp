//*****************************************************************************
//*****************************************************************************
// Class: CIstochetaAldrichi 
//          
//
// Description: the CIstochetaAldrichi represents a group of insect. scale by m_ScaleFactor
//*****************************************************************************
// 21/05/2026   Rémi Saint-Amant    Creation
//*****************************************************************************

#include "IstochetaAldrichiEquations.h"
#include "IstochetaAldrichi.h"
#include "Basic/DegreeDays.h"
#include <boost/math/distributions/weibull.hpp>
#include <boost/math/distributions/logistic.hpp>

using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::IAM;


namespace WBSF
{

	//*********************************************************************************
	//CIstochetaAldrichi class


	//*****************************************************************************
	// Object creator
	//
	// Input: See CIndividual creator
	//
	// Note: m_RDR (relative Development Rate)  member is init with random values.
	//*****************************************************************************
	CIstochetaAldrichi::CIstochetaAldrichi(CHost* pHost, CTRef creationDate, double age, TSex sex, bool bFertil, size_t generation, double scaleFactor) :
		CIndividual(pHost, creationDate, age, sex, bFertil, generation, scaleFactor)
	{
		ASSERT(age == EGG || age == PUPAE);
		int year = creationDate.GetYear();

		m_creationDate = creationDate;
		m_bInDiapause = false;

		m_end_of_diapause_factor = age == PUPAE? GetIndividualEndOfDiapauseFactor(year) :1;

		for (size_t s = 0; s < NB_STAGES; s++)
			m_RDR[s] = Equations().GetRelativeDevRate(s);

		m_to = (m_sex == FEMALE) ? Equations().GetPreOvipPeriod() : 0.0;
		m_t = 0;
		m_Fi = (m_sex == FEMALE) ? Equations().GetFecundity() : 0.0;
		m_bDeadByAttrition = false;

	}


	double CIstochetaAldrichi::GetIndividualEndOfDiapauseFactor(int year)const
	{
		//return CTRef(year, JANUARY, DAY_10);


		
		double min_Jan = GetModel()->m_weather[year][JANUARY].GetStat(H_TMIN)[MEAN];


		//const CWeatherStation& weather_station = GetStand()->m_soil_temperature;
		//CTPeriod p = weather_station[year].GetEntireTPeriod(CTM::DAILY);

		//CTRef EOD = CTRef(year,JANUARY, DAY_01, 0) + int(Equations().m_adult_emerg[delta]*24);
		//double NCDD = GetStand()->m_EOD_NCDD[EOD][0]; 
		double NCDD_Factor = Equations().GetEmergenceFactor(min_Jan);
		
		return NCDD_Factor;
	}

	CIstochetaAldrichi& CIstochetaAldrichi::operator=(const CIstochetaAldrichi& in)
	{
		if (&in != this)
		{
			CIndividual::operator=(in);

			//new relative development rate
			for (size_t s = 0; s < NB_STAGES; s++)
				m_RDR[s] = Equations().GetRelativeDevRate(s);


			m_end_of_diapause_factor = in.m_end_of_diapause_factor;
			m_bInDiapause = in.m_bInDiapause;
			m_to = in.m_to;
			m_t = in.m_t;
			m_Fi = in.m_Fi;
			m_bDeadByAttrition = in.m_bDeadByAttrition;
		}

		return *this;
	}

	//destructor
	CIstochetaAldrichi::~CIstochetaAldrichi(void)
	{
	}




	void CIstochetaAldrichi::OnNewDay(const CWeatherDay& weather)
	{
		CIndividual::OnNewDay(weather);
	}

	//*****************************************************************************
	// Develops all stages for one time step
	// Input:	weather: weather of the hour
	//			timeStep: timeStep [h]
	//*****************************************************************************
	void CIstochetaAldrichi::Live(const CHourlyData& weather, double Tsoil, size_t timeStep)
	{
		assert(IsAlive());
		assert(m_status == HEALTHY);

		CIAMHost* pHost = GetHost();
		CIAMStand* pStand = GetStand();

		double nb_steps = (24.0 / timeStep);
		size_t h = weather.GetTRef().GetHour();
		size_t s = GetStage();

		double T = weather[H_TAIR];

		//Daily development rate. Note that Pupae will develop with soil temperature
		double dr = Equations().GetDailyDevlopmentRate(s, s == PUPAE ? Tsoil : T);
		//double dr = Equations().GetDailyDevlopmentRate(s, T);
		
		if (s == PUPAE)//if pupa, take into account the diapause modification factor
			dr *= m_end_of_diapause_factor;


		//Time step development rate
		double ts_r = dr / nb_steps;
		//Time step development rate for this individual
		double i_r = ts_r * m_RDR[s];
		ASSERT(i_r >= 0);

		//Adjust age
		m_age += i_r;

		//evaluate attrition once a day
		if (GetStand()->m_bApplyAttrition)
		{
			if (IsDeadByAttrition(s, T, i_r))
				m_bDeadByAttrition = true;
		}


		if (m_sex == FEMALE && GetAge() >= ADULT && m_Fi > 0)
		{
			double t = timeStep / 24.0;
			if (m_t > m_to)
			{
				double λ = 0.15;//To be determined for this insect
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
	void CIstochetaAldrichi::Live(const CWeatherDay& weather)
	{
		CIndividual::Live(weather);

		ASSERT(IsCreated(weather.GetTRef()));



		size_t nbSteps = GetTimeStep().NbSteps();
		for (size_t step = 0; step < nbSteps && IsAlive() && m_age < DEAD_ADULT && !m_bInDiapause; step++)
		{
			//if (!m_end_of_diapause.IsInit() || weather.GetTRef() >= m_end_of_diapause)
			//{
				size_t h = step * GetTimeStep();
				double Tsoil = GetStand()->m_soil_temperature[weather[h].GetTRef()][0];
				Live(weather[h], Tsoil, GetTimeStep());
			//}

			if (m_generation == 1 && GetStage() >= PUPAE)
				m_bInDiapause = true;
		}


		//if (weather.GetTRef() == m_creationDate || HasChangedStage())
			//m_reachDate[GetStage()] = weather.GetTRef();


	}


	void CIstochetaAldrichi::Brood(const CWeatherDay& weather)
	{
		assert(m_sex == FEMALE);


		if (m_broods > 0 && !GetStand()->m_in_calibration)
		{
			ASSERT(m_age >= ADULT);
			CIAMStand* pStand = GetStand(); ASSERT(pStand);


			double attRate = GetStand()->m_bApplyAttrition ? 0.25 : 0.05;
			double scaleFactor = m_broods * m_scaleFactor * attRate;
			CIndividualPtr object = make_shared<CIstochetaAldrichi>(m_pHost, weather.GetTRef(), EGG, RANDOM_SEX, true, m_generation + 1, scaleFactor);
			m_pHost->push_front(object);
		}
	}

	// kills by old age and frost
	// Output:  Individual's state is updated to follow update
	void CIstochetaAldrichi::Die(const CWeatherDay& weather)
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
	//i_r: Individual time step development rate 
	bool CIstochetaAldrichi::IsDeadByAttrition(size_t stage, double T, double i_r)const
	{
		bool bDeath = false;


		//Get stage (overall) survival at this temperature
		double S = Equations().GetStageSurvival(stage, T);

		//Compute time step survival, limit at 1% survival to avoid annihilation
		double i_s = pow(max(0.01, S), i_r);

		//Computes attrition (probability of survival in a given time step, based on development rate)
		if (RandomGenerator().RandUniform() > i_s)
			bDeath = true;

		return bDeath;
	}



	//*****************************************************************************
	// GetStat gather information of this object
	//
	// Input: stat: the statistic object
	// Output: The stat is modified
	//*****************************************************************************
	void CIstochetaAldrichi::GetStat(CTRef d, CModelStat& stat)
	{
		if (IsCreated(d))
		{
			size_t s = GetStage();
			ASSERT(s <= DEAD_ADULT);

			if (IsAlive() || (s == DEAD_ADULT))
			{
				if (m_generation == 0)
					stat[S_PUPA0 + s - PUPAE] += m_scaleFactor;
				else if (m_generation == 1)
					stat[S_EGG + s] += m_scaleFactor;
			}


			if (HasChangedStatus() && m_status == DEAD && m_death == ATTRITION)
				stat[S_DEAD_ATTRITION] += m_scaleFactor;

			if (HasChangedStage() && s == ADULT)
			{
				stat[S_EMERGENCE] += m_scaleFactor;
			}
		}
	}


	void CIstochetaAldrichi::Pack(const CIndividualPtr& pBug)
	{
		CIstochetaAldrichi* in = (CIstochetaAldrichi*)(pBug.get());
		CIndividual::Pack(pBug);
	}

	double CIstochetaAldrichi::GetInstar(bool includeLast)const
	{
		return (IsAlive() || m_death == OLD_AGE) ? GetStage() : CBioSIMModelBase::VMISS;
	}

	//*********************************************************************************************************************

	//*********************************************************************************
	//CIAMHost

	CIAMHost::CIAMHost(CStand* pStand) :
		CHost(pStand)
	{
	}


	void CIAMHost::Live(const CWeatherDay& weather)
	{
		CHost::Live(weather);
	}

	void CIAMHost::GetStat(CTRef d, CModelStat& stat, size_t generation)
	{
		CHost::GetStat(d, stat, generation);
	}

	//*************************************************
	//CIAMStand

	CIAMStand::CIAMStand(WBSF::CBioSIMModelBase* pModel, const CModelStatVector& Tsoil) :
		WBSF::CStand(pModel),
		m_equations(pModel->RandomGenerator()),
		m_soil_temperature(Tsoil)
	{
		m_bApplyAttrition = false;
		m_in_calibration = false;
	}


	void CIAMStand::init(int year, const CWeatherYears& weather)
	{
		m_equations.GetEndOfDiapauseNCDD(m_soil_temperature, m_EOD_NCDD);
	}



	void CIAMStand::GetStat(CTRef d, CModelStat& stat, size_t generation)
	{
		CStand::GetStat(d, stat, generation);

		stat[S_CDD] = m_EOD_NCDD[d.as(CTM::HOURLY)][0];
	}


}