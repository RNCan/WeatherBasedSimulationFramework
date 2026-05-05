//*****************************************************************************
//*****************************************************************************
// Class: CLeucotaraxisArgenticollis
//          
//
// Description: the CLeucotaraxisArgenticollis represents a group of L. Argenticollis insects scale by m_ScaleFactor
//*****************************************************************************
// 01/05/2026   Rémi Saint-Amant    Bug correction in the attrition 
//									Clean up


#include "LeucotaraxisArgenticollisEquations.h"
#include "LeucotaraxisArgenticollis.h"
#include "Basic/DegreeDays.h"
#include <boost/math/distributions/weibull.hpp>
#include <boost/math/distributions/logistic.hpp>
#include <boost/math/distributions/lognormal.hpp>
#include <boost/math/distributions/beta.hpp>
#include <random>


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


		m_bInDiapause = age == PUPAE;
		m_bWillDiapause = GetStand()->RandomGenerator().Rand(0.0, 1.0) <= 0.75;//diapause in generation 1 (75% of diapause)
		//if (GetStand()->CALIBRATE_PUPAE_AND_EMERGENCE_G2)
		//	m_bWillDiapause = false;


		m_creationDate = creationDate;
		if (age == PUPAE)
		{
			m_age = PUPAE + GetPupaAge(year);
			m_end_of_diapause = GetEndOfDiapause(year);

			//m_age = PUPAE + GetPupaAge(year);
			//m_end_of_diapause = CTRef(year, DECEMBER, DAY_31);
		}
			
			

		for (size_t s = 0; s < NB_STAGES; s++)
			m_RDR[s] = Equations().GetRelativeDevRate(s);

		m_to = (m_sex == FEMALE) ? Equations().GetPreOvipPeriod() : 0.0;//adjusted to avoid unrealistic rate for pupa
		m_t = 0;
		m_Fi = (m_sex == FEMALE) ? Equations().GetFecondity((22.5 / m_RDR[ADULT])) : 0.0;//median longevity of adult = 22.5
		m_bDeadByAttrition = false;
		m_generationSurvival = { 0.05, 0.08 };

//for Preston 2021 and Bittner 2024, Farley 2019 experiment, we trick the input to mimic the experimental protocol
		if (m_generation==0)
		{

			static const size_t NB_OBS = 8;
			static const array<string, NB_OBS > LOC_NAME = { "Ithaca(NY)","Bland(VA)","Bland(VA)","Scriba(NY)", "Oswego(NY)", "Dupont(NC)", "Celo(NC)", "ICF(NY)" };
			static const array<CTRef, NB_OBS > LOC_DATE = { CTRef(2021, MARCH, DAY_30) , CTRef(2021, FEBRUARY, DAY_15), CTRef(2022, MARCH, DAY_03), CTRef(2024, MARCH, DAY_21), CTRef(2024, MARCH, DAY_10), CTRef(2019, MAY, DAY_22), CTRef(2019, MAY, DAY_22), CTRef(2026, APRIL, DAY_22) };

			for (size_t i = 0; i < NB_OBS; i++)
			{
				if (GetStand()->GetModel()->GetInfo().m_loc.m_ID == LOC_NAME[i] && creationDate.GetYear() == LOC_DATE[i].GetYear())
					m_adult_emergence_date = LOC_DATE[i];
			}
		}

	} 



	double CLeucotaraxisArgenticollis::GetPupaAge(int year)const
	{

		const CWeatherStation& weather_station = GetStand()->GetModel()->m_weather;
		const CLeucotaraxisArgenticollisEquations& equations = GetStand()->m_equations;
		
		double m = equations.m_C_param[C_P0];
		double s = equations.m_C_param[C_P3];

		double a = equations.m_C_param[C_P1];
		double b = equations.m_C_param[C_P2];

		boost::math::beta_distribution<double> i_age_dist(a,b);
		double i_age = boost::math::quantile(i_age_dist, GetStand()->RandomGenerator().Rand(0.001, 0.999));

		return i_age;
	}

	CTRef CLeucotaraxisArgenticollis::GetEndOfDiapause(int year)const
	{
		const CWeatherStation& weather_station = GetStand()->GetModel()->m_weather;
		const CLeucotaraxisArgenticollisEquations& equations = GetStand()->m_equations;
		double Tjan = weather_station[year][JANUARY].GetStat(H_TMIN)[MEAN];

		double median_EOD = equations.m_adult_emerg[μ] + equations.m_adult_emerg[ѕ] * Tjan;
		boost::math::lognormal_distribution<double> i_EOD_dist(log(max(0.01, median_EOD)), equations.m_EOD_param[EOD_B]);
		size_t DOY = max(0.0, min(180.0, 2 * median_EOD - boost::math::quantile(i_EOD_dist, GetStand()->RandomGenerator().Rand(0.001, 0.999))));

		//size_t DOY = equations.m_C_param[C_P2];
		CJDayRef end_of_diapause(year, DOY);
		assert(end_of_diapause.IsInit());
		return end_of_diapause;

		
	}

	CLeucotaraxisArgenticollis& CLeucotaraxisArgenticollis::operator=(const CLeucotaraxisArgenticollis& in)
	{
		if (&in != this)
		{
			CIndividual::operator=(in);

			//new relative development rate
			for (size_t s = 0; s < NB_STAGES; s++)
				m_RDR[s] = Equations().GetRelativeDevRate(s);

			m_bInDiapause = in.m_bInDiapause;
			m_bWillDiapause = in.m_bWillDiapause;
			m_generationSurvival = in.m_generationSurvival;
			m_end_of_diapause = in.m_end_of_diapause;
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

		//Daily development rate 
		double dr = Equations().GetRate(s, T); 
		
		if (s == PUPAE && !m_bInDiapause)
			dr = Equations().GetUndiapausedPupaRate(T, m_generation);

		if (s == LARVAE && m_generation == 1 && !m_bInDiapause)
			dr *= Equations().m_adult_emerg[delta];
		

		//Relative development rate for this individual
		double rdr = m_RDR[s];
		if (s == PUPAE && !m_bInDiapause)
			rdr = Equations().GetUndiapausedPupaRDR(m_generation);

		//Time step development rate
		double ts_r = dr / nb_steps;

		//Time step development rate for this individual
		double i_r = ts_r * rdr;
		//Time step development rate for this individual
		//r *= rr;
		ASSERT(i_r >= 0 && i_r < 1);

		if (m_bInDiapause && IsChangingStage(i_r))//Want to change to adult, but is in diapause!!!!!
		{	
			assert(s == PUPAE);
			m_bInDiapause = false;
		}

		if (!m_adult_emergence_date.IsInit())
		{
			if (!(m_generation == 2 && m_age >= PUPAE))
				m_age += min(0.04, i_r);
		}
		else
		{
			//for Preston 2023 exception
			if (s == PUPAE && m_generation == 0 )
			{
				if( weather.GetTRef().as(CTM::DAILY) == m_adult_emergence_date)
					m_age = ADULT;
			}
			else
			{
				if (!(m_generation == 2 && m_age >= PUPAE))
					m_age += min(0.04, i_r);
			}
		}

		
		//evaluate attrition
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

		if (m_bInDiapause && weather.GetTRef() >= m_end_of_diapause)
		{
			//ASSERT(GetStage() >= PUPAE && GetStage() < ADULT);
			ASSERT(m_generation <= 2);
			m_bInDiapause = false;
		}


		size_t nbSteps = GetTimeStep().NbSteps();
		for (size_t step = 0; step < nbSteps && IsAlive() && m_age < DEAD_ADULT; step++)
		{
			size_t h = step * GetTimeStep();
			Live(weather[h], GetTimeStep());
			
			if (m_generation == 0 && GetStage() == PUPAE && HasChangedStage() && m_bInDiapause)
				m_bInDiapause = false;//remove diapause when passing winter in larval stage


			if (m_generation == 1 && GetStage() == PUPAE && HasChangedStage() && m_bWillDiapause)
				m_bInDiapause = true;

			if (m_generation == 2 && GetStage() >= PUPAE)
				m_bInDiapause = true;
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


	//stage: stage
	//T: temperature for this time step
	//time_step: time step in (hour)
	//bool CLeucotaraxisArgenticollis::IsDeadByAttrition(size_t stage, double T, size_t time_step)const
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
	bool CLeucotaraxisArgenticollis::IsDeadByAttrition(size_t stage, double T, double rr)const
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

				if (s == ADULT && m_generation == 1 && !m_bWillDiapause)
					stat[S_EMERGENCE1a] += m_scaleFactor;

				if (s == ADULT && m_generation == 1 && m_bWillDiapause)
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
		//m_equations.GetAdultEmergenceCDD(weather, m_adult_emergence_CDD);
	}

	void CLAZStand::GetStat(CTRef d, CModelStat& stat, size_t generation)
	{
		CStand::GetStat(d, stat, generation);
		stat[S_CDD0] = -999;
//		stat[S_CDD1] = -999;


		//stat[S_CDD0] = m_adult_emergence_CDD[0][d][0];
		//stat[S_CDD1] = m_adult_emergence_CDD[1][d][0];
	}


}