﻿//*****************************************************************************
//*****************************************************************************
// Class: CLeucopis
//          
//
// Description: the CLeucopis represents a group of LNF insect. scale by m_ScaleFactor
//*****************************************************************************
// 17/10/2020   Rémi Saint-Amant    Creation
//*****************************************************************************

#include "LeucopisEquations.h"
#include "Leucopis.h"
#include <boost/math/distributions/weibull.hpp>
#include <boost/math/distributions/logistic.hpp>

using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::LOF;


namespace WBSF
{

	//*********************************************************************************
	//CLeucopis class


	//*****************************************************************************
	// Object creator
	//
	// Input: See CIndividual creator
	//
	// Note: m_RDR (relative Development Rate)  member is init with random values.
	//*****************************************************************************
	CLeucopis::CLeucopis(CHost* pHost, CTRef creationDate, double age, TSex sex, bool bFertil, size_t generation, double scaleFactor) :
		CIndividual(pHost, creationDate, age, sex, bFertil, generation, scaleFactor)
	{
		//reset creation date
		int year = creationDate.GetYear();
		m_creationDate = GetCreationDate(year);
		m_adult_emergence = GetAdultEmergence(year);

		for (size_t s = 0; s < NB_STAGES; s++)
			m_RDR[s] = Equations().GetRelativeDevRate(s);

		m_adult_longevity = Equations().GetAdultLongevity(m_sex);
		m_F = (m_sex == FEMALE) ? Equations().GetFecondity(m_adult_longevity) : 0;
		m_bDeadByAttrition = false;
	}

	CTRef CLeucopis::GetCreationDate(int year)const
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

		ASSERT(creationDate.IsInit());

		return creationDate;
	}

	CTRef CLeucopis::GetAdultEmergence(int year)const
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
			//T = CLeucopis::AdjustTLab(wday.GetWeatherStation()->m_name, NOT_INIT, wday.GetTRef(), T);

			double DD = max(0.0, T - Equations().m_EAS[Τᴴ]);
			CDD += DD;
			if (CDD >= adult_emerging_CDD)
			{
				adult_emergence = wday.GetTRef();
			}
		}

		return adult_emergence;
	}

	CLeucopis& CLeucopis::operator=(const CLeucopis& in)
	{
		if (&in != this)
		{
			CIndividual::operator=(in);

			m_adult_emergence = in.m_adult_emergence;
			m_adult_longevity = in.m_adult_longevity;// Equations().GetAdultLongevity(m_sex) / 2;
			m_F = in.m_F;
		}

		return *this;
	}

	//destructor
	CLeucopis::~CLeucopis(void)
	{}




	void CLeucopis::OnNewDay(const CWeatherDay& weather)
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
	void CLeucopis::Live(const CHourlyData& weather, size_t timeStep)
	{
		assert(IsAlive());
		assert(m_status == HEALTHY);

		CLNFHost* pHost = GetHost();
		CLNFStand* pStand = GetStand();

		double nb_steps = (24.0 / timeStep);
		size_t h = weather.GetTRef().GetHour();
		size_t s = GetStage();

		double T = weather[H_TAIR];
		//T = AdjustTLab(weather.GetWeatherStation()->m_name, s, weather.GetTRef(), T);

		double day_length = weather.GetLocation().GetDayLength(weather.GetTRef()) / 3600.0;//[h]

		if (s < AESTIVAL_DIAPAUSE_ADULT)
		{
			//Time step development rate
			double r = Equations().GetRate(s, T) / nb_steps;

			//double corr_r = (s == EGG || s == LARVAE) ? : 1;

			//Relative development rate for this individual
			double rr = m_RDR[s];

			//Time step development rate for this individual
			r *= rr;
			ASSERT(r >= 0 && r < 1);

			//Adjust age
			m_age += r;

			if (!m_dropToGroundDate.IsInit() && m_age > LARVAE4 + 0.9)//drop to the soil when 90% competed (guess)
				m_dropToGroundDate = weather.GetTRef().as(CTM::DAILY);

			//evaluate attrition once a day
			if (GetStand()->m_bApplyAttrition)
			{
				if(IsDeadByAttrition(s, T, r))
					m_bDeadByAttrition = true;
			}

		}
		else if (s == AESTIVAL_DIAPAUSE_ADULT)
		{
			CTRef TRef = weather.GetTRef().as(CTM::DAILY);
			if (TRef == m_adult_emergence)
				m_age = ACTIVE_ADULT;
		}
		else//ACTIVE_ADULT
		{
			double r = (1.0 / m_adult_longevity) / nb_steps;
			ASSERT(r >= 0 && r < 1);

			m_age += r;
		}
	}




	//*****************************************************************************
	// Develops all stages, including adults
	// Input:	weather: the weather of the day
	//*****************************************************************************
	void CLeucopis::Live(const CWeatherDay& weather)
	{
		CIndividual::Live(weather);

		ASSERT(IsCreated(weather.GetTRef()));

		if (!IsCreated(weather.GetTRef()))
			return;

		size_t nbSteps = GetTimeStep().NbSteps();
		for (size_t step = 0; step < nbSteps&&IsAlive(); step++)
		{
			size_t h = step * GetTimeStep();
			Live(weather[h], GetTimeStep());
		}

		if (weather.GetTRef() == m_creationDate || HasChangedStage())
			m_reachDate[GetStage()] = weather.GetTRef();


	}


	void CLeucopis::Brood(const CWeatherDay& weather)
	{
		assert(/*IsAlive() &&*/ m_sex == FEMALE);


		if (GetStage() == ACTIVE_ADULT)
		{
			//no brood process done

			//brooding
			//m_broods = m_F;
			//m_totalBroods = m_F;
			//m_F = 0;
		}
	}

	// kills by old age and frost
	// Output:  Individual's state is updated to follow update
	void CLeucopis::Die(const CWeatherDay& weather)
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
			size_t s = GetStage();

			//Preliminary assessment of the cold tolerance of Laricobius Osakensis, a winter - active predator of the hemlock woolly adelgid from western canada
			//Leland M.Humble
			static const double COLD_TOLERENCE_T[NB_STAGES] = { -27.5,-22.1, -99.0,-99.0,-19.0,-19.0 };
			//Toland:L. Osakensis was -13.6 oC (± 0.5) with temperatures that ranged from -6 oC to -21 oC.
			if (weather[H_TMIN][MEAN] < COLD_TOLERENCE_T[s])
			{
				m_status = DEAD;
				m_death = FROZEN;
			}
		}
	}


	//s: stage
	//T: temperature for this time step
	//r: devlopement rate for this time step
	bool CLeucopis::IsDeadByAttrition(size_t s, double T, double r)const
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
	void CLeucopis::GetStat(CTRef d, CModelStat& stat)
	{
		if (IsCreated(d))
		{
			size_t s = GetStage();
			ASSERT(s <= DEAD_ADULT);

			if (IsAlive() || (s == DEAD_ADULT))
				stat[S_EGG + s] += m_scaleFactor;


			if (m_status == DEAD && m_death == ATTRITION)
				stat[S_DEAD_ATTRTION] += m_scaleFactor;

			if (HasChangedStage())
				stat[S_M_EGG + s] += m_scaleFactor;

			//if (s == ACTIVE_ADULT)
			//{
			//	stat[S_ADULT_ABUNDANCE] += m_scaleFactor * m_adult_abundance;
			//}
		}
	}


	void CLeucopis::Pack(const CIndividualPtr& pBug)
	{
		assert(m_sex == pBug->GetSex());

		CLeucopis* in = (CLeucopis*)(pBug.get());
		CIndividual::Pack(pBug);
	}

	double CLeucopis::GetInstar(bool includeLast)const
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

		for (size_t ii = (172 - 1); ii <= (m_equations.m_ADE[ʎ0] - 1); ii++)
		{
			CTRef TRef = p.Begin() + ii;
			const CWeatherDay& wday = weather.GetDay(TRef);
			double T = wday[H_TNTX][MEAN];
			T = max(m_equations.m_ADE[ʎa], T);

			double DD = min(0.0, T - m_equations.m_ADE[ʎb]);//DD is negative
			sumDD += DD;
		}

		boost::math::logistic_distribution<double> begin_dist(m_equations.m_ADE[ʎ2], m_equations.m_ADE[ʎ3]);
		int begin = (int)Round((m_equations.m_ADE[ʎ0] - 1) + m_equations.m_ADE[ʎ1] * cdf(begin_dist, sumDD), 0);


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
			//Egg creation DD (allen 1976)
			m_egg_creation_CDD += m_DD.GetDD(wday);
			stat[S_EGG_CREATION_CDD] = m_egg_creation_CDD;

			//diapause end negative DD
			double T = wday[H_TNTX][MEAN];
			T = max(m_equations.m_ADE[ʎa], T);
			double NDD = min(0.0, T - m_equations.m_ADE[ʎb]);//DD is negative

			int ii = d - begin;
			if (ii >= (172 - 1) && ii <= int(m_equations.m_ADE[ʎ0] - 1))
				m_diapause_end_NCDD += NDD;

			stat[S_DIAPAUSE_END_NCDD] = m_diapause_end_NCDD;
		}


		begin = m_diapause_end;
		end = CTRef(m_diapause_end.GetYear() + 1, MARCH, DAY_01);
		if (d >= begin && d <= end)
		{
			//adult emergence (growing DD)
			double T = wday[H_TNTX][MEAN];
			double GDD = max(0.0, T - m_equations.m_EAS[Τᴴ]);
			m_adult_emergence_CDD += GDD;
			stat[S_ADULT_EMERGENCE_CDD] = m_adult_emergence_CDD;
		}
	}


}