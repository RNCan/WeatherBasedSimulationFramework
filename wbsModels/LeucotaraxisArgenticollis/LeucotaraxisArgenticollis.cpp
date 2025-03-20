//*****************************************************************************
//*****************************************************************************
// Class: CLeucotaraxisArgenticollis
//          
//
// Description: the CLeucotaraxisArgenticollis represents a group of L. Argenticollis insects scale by m_ScaleFactor
//*****************************************************************************

#include "LeucotaraxisArgenticollisEquations.h"
#include "LeucotaraxisArgenticollis.h"
#include "Basic/DegreeDays.h"
#include <boost/math/distributions/weibull.hpp>
#include <boost/math/distributions/logistic.hpp>

using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::LAZ;


namespace WBSF
{


	//*********************************************************************************
	//CLeucotaraxisArgenticollis class


	//*****************************************************************************
	// Object creator
	//
	// Input: See CIndividual creator
	//
	// Note: m_RDR (relative Development Rate)  member is init with random values.
	//*****************************************************************************
	CLeucotaraxisArgenticollis::CLeucotaraxisArgenticollis(CHost* pHost, CTRef creationDate, double age, TSex sex, bool bFertil, size_t generation, double scaleFactor) :
		CIndividual(pHost, creationDate, age, sex, bFertil, generation, scaleFactor)
	{
		ASSERT(age == EGG || age == PUPAE);

		
		//reset creation date
		int year = creationDate.GetYear();


		m_bDiapause = age == PUPAE;
		m_bDiapause1 = GetStand()->RandomGenerator().Rand(0.0, 1.0) <= 0.75;//diapause in generation 1 (75% of diapause)
		if (GetStand()->CALIBRATE_PUPAE_AND_EMERGENCE_G2)
			m_bDiapause1 = false;


		m_creationDate = creationDate;
		if (age == PUPAE)
			m_adult_emergence_date = GetAdultEmergence(year);

		for (size_t s = 0; s < NB_STAGES; s++)
			m_RDR[s] = Equations().GetRelativeDevRate(s);

		m_to = (m_sex == FEMALE) ? Equations().GetPreOvipPeriod() : 0.0;//adjusted to avoid unrealistic rate for pupa
		m_t = 0;
		m_Fi = (m_sex == FEMALE) ? Equations().GetFecondity((22.5 / m_RDR[ADULT])) : 0.0;//median longivity of adult = 22.5
		m_bDeadByAttrition = false;
		m_generationSurvival = { 0.05, 0.08 };

//for Preston 2021 and Tonya 2024 experiment, we trick the input to mimic the experimental protocol
		if (age == PUPAE)
		{

			static const array<string, 4> LOC_NAME = { "Ithaca(NY)","Bland(VA)","Bland(VA)","Scriba(NY)" };
			static const array<CTRef, 4> LOC_DATE = { CTRef(2021, MARCH, DAY_30) , CTRef(2021, FEBRUARY, DAY_15), CTRef(2022, MARCH, DAY_03), CTRef(2024, MARCH, DAY_21) };

			for (size_t i = 0; i < 4; i++)
			{
				if (GetStand()->GetModel()->GetInfo().m_loc.m_ID == LOC_NAME[i] && creationDate.GetYear() == LOC_DATE[i].GetYear())
					m_adult_emergence_date = LOC_DATE[i];
			}

			if (GetStand()->GetModel()->GetInfo().m_loc.m_ID == LOC_NAME[0] && creationDate.GetYear() == LOC_DATE[0].GetYear())
			{
				m_bDiapause1 = GetStand()->RandomGenerator().Rand(0.0, 1.0) <= 0.77;
			}
			else if (GetStand()->GetModel()->GetInfo().m_loc.m_ID == LOC_NAME[2] && creationDate.GetYear() == LOC_DATE[2].GetYear())
			{
				m_bDiapause1 = GetStand()->RandomGenerator().Rand(0.0, 1.0) <= 0.69;
			}
		}

	} 


	CTRef CLeucotaraxisArgenticollis::GetAdultEmergence(int year)const
	{
		const CWeatherStation& weather_station = GetStand()->GetModel()->m_weather;
		CTPeriod p = weather_station[year].GetEntireTPeriod(CTM::DAILY);
		double Tjan = weather_station[year][JANUARY].GetStat(H_TMIN)[MEAN];

		CTRef adult_emergence;
		double adult_emerging_CDD = Equations().GetAdultEmergingCDD(Tjan);
		const CModelStatVector& CDD = GetStand()->m_adult_emergence_CDD[0];

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

	CLeucotaraxisArgenticollis& CLeucotaraxisArgenticollis::operator=(const CLeucotaraxisArgenticollis& in)
	{
		if (&in != this)
		{
			CIndividual::operator=(in);

			//new relative development rate
			for (size_t s = 0; s < NB_STAGES; s++)
				m_RDR[s] = Equations().GetRelativeDevRate(s);

			m_bDiapause = in.m_bDiapause;
			m_bDiapause1 = in.m_bDiapause1;
			m_generationSurvival = in.m_generationSurvival;
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
	CLeucotaraxisArgenticollis::~CLeucotaraxisArgenticollis(void)
	{}




	void CLeucotaraxisArgenticollis::OnNewDay(const CWeatherDay& weather)
	{
		CIndividual::OnNewDay(weather);

	}

	//*****************************************************************************
	// Develops all stages for one time step
	// Input:	weather: weather of the hour
	//			timeStep: timeStep [h]
	//*****************************************************************************
	void CLeucotaraxisArgenticollis::Live(const CHourlyData& weather, size_t timeStep)
	{
		assert(IsAlive());
		assert(m_status == HEALTHY);

		CLAZHost* pHost = GetHost();
		CLAZStand* pStand = GetStand();

		double nb_steps = (24.0 / timeStep);
		size_t h = weather.GetTRef().GetHour();
		size_t s = GetStage();

		double T = weather[H_TAIR];
		//double day_length = weather.GetLocation().GetDayLength(weather.GetTRef()) / 3600.0;//[h]


		//Time step development rate 
		double r = Equations().GetRate(s, T) / nb_steps;
		if (s < PUPAE || m_bDiapause1)//|| m_bDiapause1
			r *= Equations().m_C_param[1];
		else if (s == PUPAE && !m_bDiapause1)
			r = Equations().GetPupaRate(T) / nb_steps;





		//Relative development rate for this individual
		double rr = m_RDR[s];
		if (s == PUPAE && !m_bDiapause1)
			rr = Equations().GetPupaRDR();

		//Time step development rate for this individual
		r *= rr;
		ASSERT(r >= 0 && r < 1);

		//Adjust age
		if (!m_bDiapause)
			m_age += r;

		//evaluate attrition
		if (GetStand()->m_bApplyAttrition)
		{
			if (IsDeadByAttrition(s, T, r))
				m_bDeadByAttrition = true;
		}


		if (m_sex == FEMALE && GetAge() >= ADULT && m_Fi > 0)
		{

			double t = timeStep / 24.0;
			if (m_t > m_to)
			{
				double λ = 0.3;//Based on 10 000 random longevity, fecundity and pre-oviposition period
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
	void CLeucotaraxisArgenticollis::Live(const CWeatherDay& weather)
	{
		CIndividual::Live(weather);

		ASSERT(IsCreated(weather.GetTRef()));

		if (m_bDiapause && GetStage() == PUPAE && weather.GetTRef() == m_adult_emergence_date)
		{
			ASSERT(m_generation == 0);
			m_bDiapause = false;
			m_age = ADULT;
		}


		size_t nbSteps = GetTimeStep().NbSteps();
		for (size_t step = 0; step < nbSteps && IsAlive() && m_age < DEAD_ADULT/* && !m_bDiapause*/; step++)
		{
			size_t h = step * GetTimeStep();
			Live(weather[h], GetTimeStep());
			if (m_generation == 2 && GetStage() >= PUPAE)
				m_bDiapause = true;
		}


		if (weather.GetTRef() == m_creationDate || HasChangedStage())
			m_reachDate[GetStage()] = weather.GetTRef();


	}


	void CLeucotaraxisArgenticollis::Brood(const CWeatherDay& weather)
	{
		assert(m_sex == FEMALE);

		bool bCreateEgg = m_generation == 0 || m_generation == 1;
		if (GetStand()->CALIBRATE_PUPAE_AND_EMERGENCE_G2)
			bCreateEgg = m_generation == 0;


		if (m_broods > 0 && bCreateEgg)
		{
			ASSERT(m_age >= ADULT);
			CLAZStand* pStand = GetStand(); ASSERT(pStand);

			double scaleFactor = m_broods * m_scaleFactor * m_generationSurvival[m_generation];
			CIndividualPtr object = make_shared<CLeucotaraxisArgenticollis>(m_pHost, weather.GetTRef(), EGG, RANDOM_SEX, true, m_generation + 1, scaleFactor);
			m_pHost->push_front(object);
		}
	}

	// kills by old age and frost
	// Output:  Individual's state is updated to follow update
	void CLeucotaraxisArgenticollis::Die(const CWeatherDay& weather)
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
	bool CLeucotaraxisArgenticollis::IsDeadByAttrition(size_t s, double T, double r)const
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
	void CLeucotaraxisArgenticollis::GetStat(CTRef d, CModelStat& stat)
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
					stat[S_EGG1 + s] += m_scaleFactor;
				else if (m_generation == 2)
					stat[S_EGG2 + s] += m_scaleFactor;
			}


			if (HasChangedStatus() && m_status == DEAD && m_death == ATTRITION)
				stat[S_DEAD_ATTRITION] += m_scaleFactor;

			if (HasChangedStage())
			{
				if (s == ADULT && m_generation == 0)
					stat[S_EMERGENCE0] += m_scaleFactor;

				if (s == ADULT && m_generation == 1 && !m_bDiapause1)
					stat[S_EMERGENCE1a] += m_scaleFactor;

				if (s == ADULT && m_generation == 1 && m_bDiapause1)
					stat[S_EMERGENCE1b] += m_scaleFactor;
			}

			stat[S_BROOD0 + m_generation] += m_broods;// *m_scaleFactor;

		}
	}


	void CLeucotaraxisArgenticollis::Pack(const CIndividualPtr& pBug)
	{
		CLeucotaraxisArgenticollis* in = (CLeucotaraxisArgenticollis*)(pBug.get());
		CIndividual::Pack(pBug);
	}

	double CLeucotaraxisArgenticollis::GetInstar(bool includeLast)const
	{
		return (IsAlive() || m_death == OLD_AGE) ? GetStage() : CBioSIMModelBase::VMISS;
	}

	//*********************************************************************************************************************
	//CLAZHost

	CLAZHost::CLAZHost(CStand* pStand) :
		CHost(pStand)
	{
	}


	void CLAZHost::Live(const CWeatherDay& weather)
	{
		CHost::Live(weather);
	}

	void CLAZHost::GetStat(CTRef d, CModelStat& stat, size_t generation)
	{
		CHost::GetStat(d, stat, generation);
	}

	//*********************************************************************************************************************
	//CLAZStand

	CLAZStand::CLAZStand(WBSF::CBioSIMModelBase* pModel) :
		WBSF::CStand(pModel),
		m_equations(pModel->RandomGenerator())
	{
		CALIBRATE_PUPAE_AND_EMERGENCE_G2 = false;
		m_bApplyAttrition = false;
	}


	void CLAZStand::init(int year, const CWeatherYears& weather)
	{
		m_equations.GetAdultEmergenceCDD(weather, m_adult_emergence_CDD);
	}

	void CLAZStand::GetStat(CTRef d, CModelStat& stat, size_t generation)
	{
		CStand::GetStat(d, stat, generation);

		stat[S_CDD0] = m_adult_emergence_CDD[0][d][0];
		stat[S_CDD1] = m_adult_emergence_CDD[1][d][0];
	}


}