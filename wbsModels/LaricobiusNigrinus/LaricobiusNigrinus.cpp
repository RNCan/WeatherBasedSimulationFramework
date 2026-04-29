//*****************************************************************************
// Class: CLaricobiusNigrinus
//          
//
// Description: the CLaricobiusNigrinus represents a group of LNF insect. scale by m_ScaleFactor
//*****************************************************************************
// 2016-04-28   Rémi Saint-Amant    Clean up for publication
// 2019-10-15   Rémi Saint-Amant    Creation
//*****************************************************************************

#include "LaricobiusNigrinusEquations.h"
#include "LaricobiusNigrinus.h"
#include <boost/math/distributions/logistic.hpp>

using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::LNF;


namespace WBSF
{

	//*********************************************************************************
	//CLaricobiusNigrinus class


	//*****************************************************************************
	// Object creator
	//
	// Input: See CIndividual creator
	//
	// Note: m_RDR (relative Development Rate)  member is init with random values.
	//*****************************************************************************
	CLaricobiusNigrinus::CLaricobiusNigrinus(CHost* pHost, CTRef creationDate, double age, TSex sex, bool bFertil, size_t generation, double scaleFactor) :
		CIndividual(pHost, creationDate, age, sex, bFertil, generation, scaleFactor)
	{
		//reset creation date
		int year = creationDate.GetYear();
		m_creationDate = GetCreationDate(year);
		m_adult_emergence = GetAdultEmergence(year);

		for (size_t s = 0; s < NB_STAGES; s++)
			m_RDR[s] = Equations().GetRelativeDevRate(s);

		m_t = 0;
		m_Fi = (m_sex == FEMALE) ? Equations().GetFecundity() : 0;
		m_cold_tolerence = Equations().GetColdTolerence();
	}

	CTRef CLaricobiusNigrinus::GetCreationDate(int year)const
	{
		CTRef creationDate;
		double creationCDD = Equations().GetCreationCDD();

		const CWeatherStation& weather_station = GetStand()->GetModel()->m_weather;
		CTRef begin = CTRef(year, JANUARY, DAY_01);
		CTRef end = CTRef(year, JUNE, DAY_30);

		double CDD = 0;
		for (CTRef TRef = begin; TRef <= end && !creationDate.IsInit(); TRef++)
		{
			const CWeatherDay& wDay = weather_station.GetDay(TRef);
			double DD = GetStand()->m_DD.GetDD(wDay);
			CDD += DD;
			if (CDD >= creationCDD)
			{
				creationDate = wDay.GetTRef();
			}
		}

		if (!creationDate.IsInit())
			creationDate = end;

		ASSERT(creationDate.IsInit());

		return creationDate;
	}

	CTRef CLaricobiusNigrinus::GetAdultEmergence(int year)const
	{
		const CWeatherStation& weather_station = GetStand()->GetModel()->m_weather;
		CTPeriod p = weather_station.GetEntireTPeriod(CTM::DAILY);

		CTRef adult_emergence;
		double adult_emerging_CDD = Equations().GetAdultEmergingCDD();

		CTRef begin = GetStand()->m_diapause_end;
		CTRef end = p.End();
		if (weather_station[year].HaveNext())
			end = min(p.End(), CTRef(begin.GetYear() + 1, JUNE, DAY_30));

		double CDD = 0;
		for (CTRef TRef = begin; TRef <= end && !adult_emergence.IsInit(); TRef++)
		{
			const CWeatherDay& wday = weather_station.GetDay(TRef);
			double T = wday[H_TNTX][MEAN];
			double DD = max(0.0, T - Equations().m_EAS[Τᴴ]);
			CDD += DD;
			if (CDD >= adult_emerging_CDD)
			{
				adult_emergence = wday.GetTRef();
			}
		}

		if (!adult_emergence.IsInit())
		{
			//When there is no adult emergence, set it to the last day of the year
			adult_emergence = end;
		}

		return adult_emergence;
	}

	CLaricobiusNigrinus& CLaricobiusNigrinus::operator=(const CLaricobiusNigrinus& in)
	{
		if (&in != this)
		{
			CIndividual::operator=(in);

			m_dropToGroundDate = in.m_dropToGroundDate;
			m_adult_emergence = in.m_adult_emergence;
			m_t = in.m_t;
			m_Fi = in.m_Fi;
			m_deadByAttrition = in.m_deadByAttrition;

		}

		return *this;
	}

	//destructor
	CLaricobiusNigrinus::~CLaricobiusNigrinus(void)
	{
	}



	void CLaricobiusNigrinus::OnNewDay(const CWeatherDay& weather)
	{
		CIndividual::OnNewDay(weather);

		if (weather.GetTRef() == m_creationDate)
		{
			m_age = EGG;
		}
	}

	//*****************************************************************************
	// Develops all stages for one time step
	// Input:	weather: weather of the hour
	//			timeStep: timeStep [h]
	//*****************************************************************************
	void CLaricobiusNigrinus::Live(const CHourlyData& weather, size_t time_step)
	{
		assert(IsAlive());
		assert(m_status == HEALTHY);

		CLNFHost* pHost = GetHost();
		CLNFStand* pStand = GetStand();

		double nb_steps = (24.0 / time_step);
		size_t h = weather.GetTRef().GetHour();
		size_t s = GetStage();

		double T = weather[H_TAIR];

		//daily development rate
		double dr = Equations().GetRate(s, T);
		if (s == AESTIVAL_DIAPAUSE_ADULT)
		{
			//for aestival diapause adult, we compute rate from adult emergence 
			if (!m_aestival_diapause_begin.IsInit())
				m_aestival_diapause_begin = weather.GetTRef().as(CTM::DAILY);

			assert(m_aestival_diapause_begin.IsInit());
			assert(m_adult_emergence.IsInit());
			assert(m_adult_emergence - m_aestival_diapause_begin > 0);

			dr = 1.0 / (m_adult_emergence - m_aestival_diapause_begin);
		}


		//Time step development rate
		double r = dr / nb_steps;

		//Relative development rate for this individual
		double rdr = m_RDR[s];

		//Time step development rate for this individual
		r *= rdr;
		ASSERT(r >= 0 && r < 1);

		//Adjust age
		m_age += r;

		//apply attrition
		if (GetStand()->m_bApplyAttrition)
		{
			if (IsDeadByAttrition(s, T, time_step))
				m_deadByAttrition = weather.GetTRef().as(CTM::DAILY);
		}


		//verify for larval drop
		if (!m_dropToGroundDate.IsInit() && m_age > LARVAE + 0.95)//drop to the soil when 95% completed (guess)
			m_dropToGroundDate = weather.GetTRef().as(CTM::DAILY);


		//Oviposition part
		if (m_sex == FEMALE && GetStage() >= ACTIVE_ADULT)
		{
			double to = 140;//pre-oviposition period of 140 days
			double t = time_step / 24.0;
			if (m_t > to)
			{
				double λ = Equations().GetFecundity(GetAge(), weather[H_TAIR]);
				double brood = m_Fi * (exp(-λ * (m_t - to)) - exp(-λ * (m_t + t - to)));

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
	void CLaricobiusNigrinus::Live(const CWeatherDay& weather)
	{
		CIndividual::Live(weather);

		ASSERT(IsCreated(weather.GetTRef()));

		if (!IsCreated(weather.GetTRef()))
			return;

		//Simulate the insect for all time steps
		size_t nbSteps = GetTimeStep().NbSteps();
		for (size_t step = 0; step < nbSteps && m_age < DEAD_ADULT; step++)
		{
			size_t h = step * GetTimeStep();
			Live(weather[h], GetTimeStep());
		}

		if (weather.GetTRef() == m_creationDate || HasChangedStage())
			m_reachDate[GetStage()] = weather.GetTRef();
	}


	void CLaricobiusNigrinus::Brood(const CWeatherDay& weather)
	{
		assert(IsAlive() && m_sex == FEMALE);


		if (GetStage() == ACTIVE_ADULT)
		{
			//all is done is live
		}
	}

	// kills by old age and frost
	// Output:  Individual's state is updated to follow update
	void CLaricobiusNigrinus::Die(const CWeatherDay& weather)
	{
		//attrition mortality. Killed at the end of time step 
		if (GetStage() == DEAD_ADULT)
		{
			//Old age
			m_status = DEAD;
			m_death = OLD_AGE;
		}
		else if (m_deadByAttrition.IsInit())
		{
			m_status = DEAD;
			m_death = ATTRITION;
		}
		else
		{
			size_t s = GetStage();

			//Preliminary assessment of the cold tolerance of Laricobius nigrinus, 
			//a winter - active predator of the hemlock woolly adelgid from Western Canada
			//Leland M.Humble
			static const double COLD_TOLERENCE_T[NB_STAGES] = { -27.5,-22.1, -99.0,-99.0,-19.0,-19.0 };
			//Toland:L. nigrinus was -13.6 oC (± 0.5) with temperatures that ranged from -6 oC to -21 oC.
			if (weather[H_TMIN][MEAN] < COLD_TOLERENCE_T[s])
			{
				m_status = DEAD;
				m_death = FROZEN;
			}

			//From Crandall 2023
			if (weather[H_TMIN][LOWEST] < m_cold_tolerence)
			{
				m_status = DEAD;
				m_death = FROZEN;
			}

		}
	}


	//stage: stage
	//T: temperature for this time step (°C)
	//time_step: time step (hours)
	bool CLaricobiusNigrinus::IsDeadByAttrition(size_t stage, double T, size_t time_step)const
	{
		bool bDeath = false;

		double d_s = Equations().GetDailySurvivalRate(stage, T);//daily survival
		double ts_s = pow(d_s, time_step / 24.0);

		//Computes attrition (probability of survival in a given time step)
		if (RandomGenerator().RandUniform() > ts_s)
			bDeath = true;

		return bDeath;
	}



	//stage: stage
	//T: temperature for this time step (°C)
	//dr: daily development rate
	//rdr: individual relative development rate
	//time_step: time step (hours)
	//bool CLaricobiusNigrinus::IsDeadByAttrition(size_t stage, double T, double dr, double rdr, size_t time_step)const
	//{
	//	bool bDeath = false;

	//	double nb_steps = 24.0 / time_step;

	//	//Get daily rate survival for T
	//	double d_r = Equations().GetRate(stage, T);//daily rate
	//	double d_s = Equations().GetDailySurvivalRate(stage, T);//daily survival
	//	double S = pow(d_s, (1 / d_r));//overall survival

	//	//compute time step survival
	//	double ts_r = dr * rdr / nb_steps;
	//	double ts_s = pow(S, ts_r);

	//	//Computes attrition (probability of survival in a given time step, based on development rate)
	//	if (RandomGenerator().RandUniform() > ts_s)
	//		bDeath = true;

	//	return bDeath;
	//}


	//*****************************************************************************
	// GetStat gather information of this object
	//
	// Input: stat: the statistic object
	// Output: The stat is modified
	//*****************************************************************************
	void CLaricobiusNigrinus::GetStat(CTRef d, CModelStat& stat)
	{
		if (IsCreated(d))
		{
			size_t s = GetStage();
			ASSERT(s <= DEAD_ADULT);

			if (IsAlive() || (s == DEAD_ADULT))
				stat[S_EGGS + s] += m_scaleFactor;


			if (m_status == DEAD && m_death == FROZEN)
				stat[S_DEAD_FROST] += m_scaleFactor;

			if (HasChangedStage())
				stat[S_M_EGGS + s] += m_scaleFactor;

			if (m_dropToGroundDate.IsInit() && m_dropToGroundDate == d)
				stat[S_LARVAL_DROP] += m_scaleFactor;


			stat[S_BROODS] += m_scaleFactor * m_broods;

			if (d == GetStand()->m_diapause_end)
				stat[S_M_AESTIVAL_DIAPAUSE_ADULT_END] += m_scaleFactor;
			
		}
	}


	void CLaricobiusNigrinus::Pack(const CIndividualPtr& pBug)
	{
		assert(m_sex == pBug->GetSex());

		CLaricobiusNigrinus* in = (CLaricobiusNigrinus*)(pBug.get());
		CIndividual::Pack(pBug);
	}

	double CLaricobiusNigrinus::GetInstar(bool includeLast)const
	{
		return (IsAlive() || m_death == OLD_AGE) ? GetStage() : CBioSIMModelBase::VMISS;
	}

	//*********************************************************************************************************************

	//*********************************************************************************
	//CLNFHost

	CLNFHost::CLNFHost(CStand* pStand) :
		CHost(pStand)
	{
	}


	void CLNFHost::Live(const CWeatherDay& weather)
	{
		CHost::Live(weather);
	}

	void CLNFHost::GetStat(CTRef d, CModelStat& stat, size_t generation)
	{
		CHost::GetStat(d, stat, generation);
	}

	//*************************************************
	//CLNFStand

	void CLNFStand::init(int year, const CWeatherYears& weather)
	{
		m_diapause_end = ComputeDiapauseEnd(weather[year]);
	}

	CTRef CLNFStand::ComputeDiapauseEnd(const CWeatherYear& weather)const
	{
		CTPeriod p = weather.GetEntireTPeriod(CTM::DAILY);

		double sumDD = 0;

		for (size_t ii = (m_equations.m_ADE[ʎ0] - 1); ii <= (m_equations.m_ADE[ʎ1] - 1); ii++)
		{
			CTRef TRef = p.Begin() + ii;
			const CWeatherDay& wday = weather.GetDay(TRef);
			double T = wday[H_TNTX][MEAN];

			double DD = min(0.0, T - m_equations.m_ADE[ʎb]);//DD is negative
			sumDD += DD;
		}

		boost::math::logistic_distribution<double> begin_dist(m_equations.m_ADE[ʎ2], m_equations.m_ADE[ʎ3]);
		int begin = (int)Round((m_equations.m_ADE[ʎ1] - 1) + m_equations.m_ADE[ʎa] * cdf(begin_dist, sumDD), 0);


		return p.Begin() + begin;
	}



	void CLNFStand::GetStat(CTRef d, CModelStat& stat, size_t generation)
	{
		CStand::GetStat(d, stat, generation);

		const CWeatherStation& weather_station = GetModel()->m_weather;
		const CWeatherDay& wday = weather_station.GetDay(d);

		//use year of diapause to compute correctly the adult emergence cdd
		int year = m_diapause_end.GetYear();
		CTRef begin = CTRef(year, JANUARY, DAY_01);
		CTRef end = CTRef(year, DECEMBER, DAY_31);

		if (d >= begin && d <= end)
		{
			//Egg creation DD (Allen 1976)
			m_egg_creation_CDD += m_DD.GetDD(wday);
			stat[S_EGGS_CREATION_CDD] = m_egg_creation_CDD;

			//diapause end negative DD
			double T = wday[H_TNTX][MEAN];
			double NDD = min(0.0, T - m_equations.m_ADE[ʎb]);//DD is negative

			int ii = d - begin;
			if (ii >= int(m_equations.m_ADE[ʎ0] - 1) && ii <= int(m_equations.m_ADE[ʎ1] - 1))
				m_diapause_end_NCDD += NDD;

			stat[S_DIAPAUSE_END_NCDD] = m_diapause_end_NCDD;
		}


		begin = m_diapause_end;
		end = CTRef(m_diapause_end.GetYear() + 1, JUNE, DAY_30);
		if (d >= begin && d <= end)
		{
			//adult emergence (growing DD)
			double T = wday[H_TNTX][MEAN];
			double GDD = max(0.0, T - m_equations.m_EAS[Τᴴ]);
			m_adult_emergence_CDD += GDD;
			stat[S_EMERGING_ADULT_CDD] = m_adult_emergence_CDD;
		}
	}


}