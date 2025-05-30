//*****************************************************************************
//*****************************************************************************
// Class: CLeucotaraxisArgenticollis
//          
//
// Description: the CLeucotaraxisArgenticollis represents a group of L. Argenticollis insects scale by m_ScaleFactor
//*****************************************************************************
//salut

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
		m_Fi = (m_sex == FEMALE) ? Equations().GetFecondity((22.5 / m_RDR[ADULT])) : 0.0;//median longivity of adult = 22.5
		m_bDeadByAttrition = false;
		m_generationSurvival = { 0.05, 0.08 };

//for Preston 2021 and Tonya 2024 experiment, we trick the input to mimic the experimental protocol
		//if (age == PUPAE)
		{

			static const size_t NB_OBS = 5;
			static const array<string, NB_OBS > LOC_NAME = { "Ithaca(NY)","Bland(VA)","Bland(VA)","Scriba(NY)", "Oswego(NY)" };
			static const array<CTRef, NB_OBS > LOC_DATE = { CTRef(2021, MARCH, DAY_30) , CTRef(2021, FEBRUARY, DAY_15), CTRef(2022, MARCH, DAY_03), CTRef(2024, MARCH, DAY_21), CTRef(2024, MARCH, DAY_10) };

			for (size_t i = 0; i < NB_OBS; i++)
			{
				if (GetStand()->GetModel()->GetInfo().m_loc.m_ID == LOC_NAME[i] && creationDate.GetYear() == LOC_DATE[i].GetYear())
					m_adult_emergence_date = LOC_DATE[i];
			}

			//if (GetStand()->GetModel()->GetInfo().m_loc.m_ID == LOC_NAME[0] && creationDate.GetYear() == LOC_DATE[0].GetYear())
			//{
			//	m_bDiapause1 = GetStand()->RandomGenerator().Rand(0.0, 1.0) <= 0.77;
			//}
			//else if (GetStand()->GetModel()->GetInfo().m_loc.m_ID == LOC_NAME[2] && creationDate.GetYear() == LOC_DATE[2].GetYear())
			//{
			//	m_bDiapause1 = GetStand()->RandomGenerator().Rand(0.0, 1.0) <= 0.69;
			//}
		}

	} 


	/*CTRef CLeucotaraxisArgenticollis::GetAdultEmergence(int year)const
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
	}*/

	double CLeucotaraxisArgenticollis::GetPupaAge(int year)const
	{

		const CWeatherStation& weather_station = GetStand()->GetModel()->m_weather;
		const CLeucotaraxisArgenticollisEquations& equations = GetStand()->m_equations;

		//double i_age = 0.5;


		//Get mean January minimum temperature
		//double Tjan = weather_station[year][JANUARY].GetStat(H_TMIN)[MEAN];

		//double Tsummer = weather_station[year].GetStat(H_TNTX, CTPeriod(year,JUNE,DAY_01, year, DECEMBER, DAY_31))[MEAN];
		//double Tdelta = weather_station[year][JULY].GetStat(H_TMAX)[MEAN] - weather_station[year][JANUARY].GetStat(H_TMIN)[MEAN];

		//boost::math::logistic_distribution<double> age_dist(equations.m_C_param[C_P0], equations.m_C_param[C_P1]);
		double m = equations.m_C_param[C_P0];//0.9 - 0.8 * boost::math::cdf(age_dist, Tdelta);
		double s = equations.m_C_param[C_P3];

		//double a = m * (m * (1 - m) / (s * s) - 1);
		//double b = (1 - m) * (m * (1 - m) / (s * s) - 1);
		double a = equations.m_C_param[C_P1];
		double b = equations.m_C_param[C_P2];

		boost::math::beta_distribution<double> i_age_dist(a,b);
		double i_age = boost::math::quantile(i_age_dist, GetStand()->RandomGenerator().Rand(0.001, 0.999));
		
		//Version larve et pupe
		/*boost::math::logistic_distribution<double> age_dist(equations.m_C_param[C_P0], equations.m_C_param[C_P1]);
		double m = 0.95 - 0.9*boost::math::cdf(age_dist, Tjan);
		double s = equations.m_C_param[C_P3];
		
		double a = m * (m * (1 - m) / (s*s) - 1); 
		double b = (1 - m) * (m * (1 - m) / (s * s) - 1);
		
		boost::math::beta_distribution<double> i_age_dist(a, b);
		double i_age = -0.3 + 1.25 * boost::math::quantile(i_age_dist, GetStand()->RandomGenerator().Rand(0.001, 0.999));
		*/

		//boost::math::logistic_distribution<double> age_dist(equations.m_C_param[C_P0], equations.m_C_param[C_P1]);
		//double m = 0.05 + 0.90 * boost::math::cdf(age_dist, Tsummer);
		//double s = equations.m_C_param[C_P3];
		//
		////double m = equations.m_C_param[C_P0];
		////double s = equations.m_C_param[C_P3];
		//
		//double a = m * (m * (1 - m) / (s * s) - 1);
		//double b = (1 - m) * (m * (1 - m) / (s * s) - 1);
		//
		//boost::math::beta_distribution<double> i_age_dist(a, b);
		//double i_age = 0.9*boost::math::quantile(i_age_dist, GetStand()->RandomGenerator().Rand(0.001, 0.999));
		//double i_age = -equations.m_adult_emerg[μ] + (0.95 - -equations.m_adult_emerg[μ]) * boost::math::quantile(i_age_dist, GetStand()->RandomGenerator().Rand(0.001, 0.999));

		//version pupe seulement
		//R²=0.84
		//boost::math::logistic_distribution<double> age_dist(equations.m_C_param[C_P0], equations.m_C_param[C_P1]);
		//double m = 0.95 - 0.9*boost::math::cdf(age_dist, Tjan);//[0.05,0.95]
		//double s = equations.m_C_param[C_P3];
		//
		//double a = m * (m * (1 - m) / (s * s) - 1);
		//double b = (1 - m) * (m * (1 - m) / (s * s) - 1);
		//
		//boost::math::beta_distribution<double> i_age_dist(a, b);
		//double i_age = boost::math::quantile(i_age_dist, GetStand()->RandomGenerator().Rand(0.001, 0.999));


		

		//R²=86.2
		//double random = GetStand()->RandomGenerator().RandNormal(0.0, equations.m_C_param[C_P3]);
		//boost::math::logistic_distribution<double> age_dist(equations.m_C_param[C_P0], equations.m_C_param[C_P1]);
		//double i_age = 1 - boost::math::cdf(age_dist, Tjan) + random;
		//while (i_age < 0 || i_age>0.95)
		//{
		//	random = GetStand()->RandomGenerator().RandNormal(0.0, equations.m_C_param[C_P3]);
		//	i_age = 1 - boost::math::cdf(age_dist, Tjan) + random;
		//}
		
		
		//double random = 0;// GetStand()->RandomGenerator().RandNormal(0.0, equations.m_C_param[C_P3]);
		//boost::math::logistic_distribution<double> age_dist(equations.m_C_param[C_P0], equations.m_C_param[C_P1]);
		//double i_age = 1 - 2*boost::math::cdf(age_dist, Tjan) + random;
		//while (i_age < -1 || i_age>0.95)
		//{
		//	random = GetStand()->RandomGenerator().RandNormal(0.0, equations.m_C_param[C_P3]);
		//	i_age = 1 - 2 * boost::math::cdf(age_dist, Tjan) + random;
		//}

		

		return i_age;
	}

	CTRef CLeucotaraxisArgenticollis::GetEndOfDiapause(int year)const
	{
		const CWeatherStation& weather_station = GetStand()->GetModel()->m_weather;
		const CLeucotaraxisArgenticollisEquations& equations = GetStand()->m_equations;
		double Tjan = weather_station[year][JANUARY].GetStat(H_TMIN)[MEAN];

		//boost::math::logistic_distribution<double> EOD_dist(equations.m_C_param[C_P2], equations.m_EOD_param[EOD_A]);
		//double median_EOD = equations.m_adult_emerg[μ] + equations.m_adult_emerg[ѕ] * boost::math::cdf(EOD_dist, Tjan);


		double median_EOD = equations.m_adult_emerg[μ] + equations.m_adult_emerg[ѕ] * Tjan;
		boost::math::lognormal_distribution<double> i_EOD_dist(log(max(0.01, median_EOD)), equations.m_EOD_param[EOD_B]);
		//size_t DOY = max(0.0, min(180.0, boost::math::quantile(i_EOD_dist, GetStand()->RandomGenerator().Rand(0.001, 0.999))));
		size_t DOY = max(0.0, min(180.0, 2 * median_EOD - boost::math::quantile(i_EOD_dist, GetStand()->RandomGenerator().Rand(0.001, 0.999))));


		//R²=86.2
		//double median_EOD = max(0.0, equations.m_adult_emerg[μ] + equations.m_adult_emerg[ѕ] * Tjan);
		//boost::math::lognormal_distribution<double> EOD_dist(log(max(0.01, median_EOD)), equations.m_EOD_param[EOD_B]);
		//double DOY = max(0.0, min(180.0, boost::math::quantile(EOD_dist, GetStand()->RandomGenerator().Rand(0.001, 0.999))));
		
		
		
		
		
		
		//for(size_t i =0; DOY<0&&i<10; i++)
		//	DOY = min(180.0, 2 * equations.m_C_param[C_P2] - boost::math::quantile(EOD_dist, GetStand()->RandomGenerator().Rand(0.001, 0.999)));
		//
		//if (DOY < 0)
		//	DOY = 0;
		


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
		//double day_length = weather.GetLocation().GetDayLength(weather.GetTRef()) / 3600.0;//[h]


		//Time step development rate 
		double r = Equations().GetRate(s, T) / nb_steps; 
		

		if (s == PUPAE && !m_bInDiapause)
			r = Equations().GetUndiapausedPupaRate(T, m_generation) / nb_steps;

		//if (s == PUPAE && m_generation == 1 && !m_bInDiapause)
		if (s == LARVAE && m_generation == 1 && !m_bInDiapause)
			r *= Equations().m_adult_emerg[delta];
		
		

		//Relative development rate for this individual
		double rr = m_RDR[s];
		if (s == PUPAE && !m_bInDiapause)
			rr = Equations().GetUndiapausedPupaRDR(m_generation);

		//Time step development rate for this individual
		r *= rr;
		ASSERT(r >= 0 && r < 1);

		//if (s == PUPAE)
			//r *= pStand->m_equations.m_C_param[C_P1];


		//Adjust age
		//if (!m_bDiapause0)
		
		if (m_bInDiapause && IsChangingStage(r))//Want to change to adult, but is int diapause!!!!!
		{	//r = 0;
			assert(s == PUPAE);
			m_bInDiapause = false;
		}

		if (!m_adult_emergence_date.IsInit())
		{
			if (!(m_generation == 2 && m_age >= PUPAE))
				m_age += min(0.04, r);
		}
		else
		{
			//for prestion 2023 exception
			if (s == PUPAE && m_generation == 0 && weather.GetTRef().as(CTM::DAILY) == m_adult_emergence_date)
			{
				m_age = ADULT;
			}
			else
			{
				if (!(m_generation == 2 && m_age >= PUPAE))
					m_age += min(0.04, r);
			}
		}

		
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