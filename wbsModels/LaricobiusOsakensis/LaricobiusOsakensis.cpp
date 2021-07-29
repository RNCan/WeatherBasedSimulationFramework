//*****************************************************************************
//*****************************************************************************
// Class: CLaricobiusOsakensis
//          
//
// Description: the CLaricobiusOsakensis represents a group of LNF insect. scale by m_ScaleFactor
//*****************************************************************************
// 07/07/2021   Rémi Saint-Amant    Creation
//*****************************************************************************

#include "LaricobiusOsakensisEquations.h"
#include "LaricobiusOsakensis.h"
#include <boost/math/distributions/weibull.hpp>
#include <boost/math/distributions/logistic.hpp>

using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::LOF;


namespace WBSF
{

	//*********************************************************************************
	//CLaricobiusOsakensis class


	//*****************************************************************************
	// Object creator
	//
	// Input: See CIndividual creator
	//
	// Note: m_RDR (relative Development Rate)  member is init with random values.
	//*****************************************************************************
	CLaricobiusOsakensis::CLaricobiusOsakensis(CHost* pHost, CTRef creationDate, double age, TSex sex, bool bFertil, size_t generation, double scaleFactor) :
		CIndividual(pHost, creationDate, age, sex, bFertil, generation, scaleFactor)
	{
		//reset creation date
		int year = creationDate.GetYear();
		m_creationDate = GetCreationDate(year);
		m_adult_emergence = GetAdultEmergence(year);

		for (size_t s = 0; s < NB_STAGES; s++)
			m_RDR[s] = Equations().GetRelativeDevRate(s);

		//m_adult_longevity = Equations().GetAdultLongevity();
		m_t = 0;
		m_Fi = (m_sex == FEMALE) ? Equations().GetFecondity() : 0;
		m_bDeadByAttrition = false;
	}

	CTRef CLaricobiusOsakensis::GetCreationDate(int year)const
	{
		CTRef creationDate;
		double creationCDD = Equations().GetCreationCDD();

		const CWeatherStation& weather_station = GetStand()->GetModel()->m_weather;
		//CTRef begin = CTRef(year, JANUARY, DAY_01);
		//CTRef end = CTRef(year, JUNE, DAY_30);

		CTRef begin = CTRef(year, DECEMBER, DAY_01);
		CTRef end = CTRef(year+1, JULY, DAY_31);

		double CDD = 0;
		for (CTRef TRef = begin; TRef <= end && !creationDate.IsInit(); TRef++)
		{
			const CWeatherDay& wDay = weather_station.GetDay(TRef);
			double DD = GetStand()->m_DD.GetDD(wDay);
			CDD += DD;

			if (CDD  >= creationCDD)
			{
				creationDate = wDay.GetTRef();
			}
		}
		if (!creationDate.IsInit())
			creationDate = end;//for text only

		ASSERT(creationDate.IsInit());

		return creationDate;
	}

	CTRef CLaricobiusOsakensis::GetAdultEmergence(int year)const
	{
		const CWeatherStation& weather_station = GetStand()->GetModel()->m_weather;
		CTPeriod p = weather_station.GetEntireTPeriod(CTM::DAILY);

		CTRef adult_emergence;
		double adult_emerging_CDD = Equations().GetAdultEmergingCDD();

		CTRef begin = GetStand()->m_diapause_end;
		CTRef end = p.End();
		if (weather_station[year].HaveNext())
			end = min(p.End(), CTRef(begin.GetYear() + 1, JANUARY, DAY_31));

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

		//if (!adult_emergence.IsInit())
			//adult_emergence = CTRef(year, DECEMBER, DAY_31);//pour test seulement a revoir...

		ASSERT(adult_emergence.IsInit());
		return adult_emergence;
	}

	CLaricobiusOsakensis& CLaricobiusOsakensis::operator=(const CLaricobiusOsakensis& in)
	{
		if (&in != this)
		{
			CIndividual::operator=(in);

			//new relative developement rate
			for (size_t s = 0; s < NB_STAGES; s++)
				m_RDR[s] = Equations().GetRelativeDevRate(s);


			m_dropToGroundDate = in.m_dropToGroundDate;
			m_adult_emergence = in.m_adult_emergence;
			m_reachDate = in.m_reachDate;
			m_t = in.m_t;
			m_Fi = in.m_Fi;
			m_bDeadByAttrition = in.m_bDeadByAttrition;
		}

		return *this;
	}

	//destructor
	CLaricobiusOsakensis::~CLaricobiusOsakensis(void)
	{}




	void CLaricobiusOsakensis::OnNewDay(const CWeatherDay& weather)
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
	void CLaricobiusOsakensis::Live(const CHourlyData& weather, size_t timeStep)
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

		if (s < AESTIVAL_DIAPAUSE_ADULT || s == ACTIVE_ADULT)
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
				if (IsDeadByAttrition(s, T, r))
					m_bDeadByAttrition = true;
			}

		}
		else if (s == AESTIVAL_DIAPAUSE_ADULT)
		{
			CTRef TRef = weather.GetTRef().as(CTM::DAILY);
			if (TRef == m_adult_emergence)
				m_age = ACTIVE_ADULT;
		}

		if (m_sex == FEMALE && GetStage() >= ACTIVE_ADULT)
		{
			double to = 0;
			double t = timeStep / 24.0;
			double λ = Equations().GetFecondityRate(GetAge(), weather[H_TAIR]);
			double brood = m_Fi * (exp(-λ * (m_t - to)) - exp(-λ * (m_t + t - to)));

			m_broods += brood;
			m_totalBroods += brood;

			m_t += t;
		}
		//else//ACTIVE_ADULT
		//{
		//	double r = (1.0 / m_adult_longevity) / nb_steps;
		//	ASSERT(r >= 0 && r < 1);

		//	m_age += r;
		//}
	}




	//*****************************************************************************
	// Develops all stages, including adults
	// Input:	weather: the weather of the day
	//*****************************************************************************
	void CLaricobiusOsakensis::Live(const CWeatherDay& weather)
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


	void CLaricobiusOsakensis::Brood(const CWeatherDay& weather)
	{
		assert(/*IsAlive() &&*/ m_sex == FEMALE);


		if (GetStage() == ACTIVE_ADULT)
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
	void CLaricobiusOsakensis::Die(const CWeatherDay& weather)
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
			//size_t s = GetStage();

			////Preliminary assessment of the cold tolerance of Laricobius Osakensis, a winter - active predator of the hemlock woolly adelgid from western canada
			////Leland M.Humble
			//static const double COLD_TOLERENCE_T[NB_STAGES] = { -27.5,-22.1, -99.0,-99.0,-19.0,-19.0 };
			////Toland:L. Osakensis was -13.6 oC (± 0.5) with temperatures that ranged from -6 oC to -21 oC.
			//if (weather[H_TMIN][MEAN] < COLD_TOLERENCE_T[s])
			//{
			//	m_status = DEAD;
			//	m_death = FROZEN;
			//}
		}
	}


	//s: stage
	//T: temperature for this time step
	//r: devlopement rate for this time step
	bool CLaricobiusOsakensis::IsDeadByAttrition(size_t s, double T, double r)const
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
	void CLaricobiusOsakensis::GetStat(CTRef d, CModelStat& stat)
	{
		if (IsCreated(d))
		{
			size_t s = GetStage();
			ASSERT(s <= DEAD_ADULT);

			if (IsAlive() || (s == DEAD_ADULT))
				stat[S_EGG + s] += m_scaleFactor;

			stat[S_LARVAE] = stat[S_L1]+ stat[S_L2]+ stat[S_L3]+ stat[S_L4];

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


	void CLaricobiusOsakensis::Pack(const CIndividualPtr& pBug)
	{
		assert(m_sex == pBug->GetSex());

		CLaricobiusOsakensis* in = (CLaricobiusOsakensis*)(pBug.get());
		CIndividual::Pack(pBug);
	}

	double CLaricobiusOsakensis::GetInstar(bool includeLast)const
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
		//CTPeriod p = weather.GetEntireTPeriod(CTM::DAILY);
		CTPeriod p = weather.GetPrevious().GetEntireTPeriod(CTM::DAILY);

		double sumDD = 0;

		for (size_t ii = (m_equations.m_ADE[ʎ0] - 1); ii <= (m_equations.m_ADE[ʎ1] - 1); ii++)
		{
			CTRef TRef = p.Begin() + ii;
			const CWeatherDay& wday = weather.GetDay(TRef);
			double T = wday[H_TNTX][MEAN];
			double DD = min(0.0, T - m_equations.m_ADE[ʎb]);//DD is negative
			sumDD += DD;
		}

		boost::math::logistic_distribution<double> diapause_end_dist(m_equations.m_ADE[ʎ2], m_equations.m_ADE[ʎ3]);
		int begin = (int)Round((m_equations.m_ADE[ʎ1] - 1) + m_equations.m_ADE[ʎa] * cdf(diapause_end_dist, sumDD), 0);

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
			//m_egg_creation_CDD += m_DD.GetDD(wday);
			//stat[S_EGG_CREATION_CDD] = m_egg_creation_CDD;

			//diapause end negative DD
			double T = wday[H_TNTX][MEAN];
			//T = max(m_equations.m_ADE[ʎa], T);
			double NDD = min(0.0, T - m_equations.m_ADE[ʎb]);//DD is negative

			int ii = d - begin;
			if (ii >= int(m_equations.m_ADE[ʎ0] - 1) && ii <= int(m_equations.m_ADE[ʎ1] - 1))
				m_diapause_end_NCDD += NDD;

			stat[S_DIAPAUSE_END_NCDD] = m_diapause_end_NCDD;
		}


		begin = m_diapause_end;
		end = CTRef(m_diapause_end.GetYear() + 1, JANUARY, DAY_31);
		if (d >= begin && d <= end)
		{
			//adult emergence (growing DD)
			double T = wday[H_TNTX][MEAN];
			double GDD = max(0.0, T - m_equations.m_EAS[Τᴴ]);
			m_adult_emergence_CDD += GDD;
			stat[S_ADULT_EMERGENCE_CDD] = m_adult_emergence_CDD;
		}

		//compute egg creation
		begin = CTRef(year, DECEMBER, DAY_01);
		end = CTRef(year+1, JULY, DAY_31);

		if (d >= begin && d <= end)
		{
			//Egg creation DD (allen 1976)
			m_egg_creation_CDD += m_DD.GetDD(wday);
			stat[S_EGG_CREATION_CDD] = m_egg_creation_CDD;
		}
		

	}


}