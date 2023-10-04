//*****************************************************************************
//*****************************************************************************
// Class: CAprocerosLeucopoda
//          
//
// Description: the CAprocerosLeucopoda represents a group of TZZ insect. scale by m_ScaleFactor
//*****************************************************************************
// 2022-03-20	Rémi Saint-Amant    Bug correction in emergence
// 2020-10-17   Rémi Saint-Amant    Creation
//*****************************************************************************

#include "ALeucopodaEquations.h"
#include "ALeucopoda.h"
#include <boost/math/distributions/weibull.hpp>
#include <boost/math/distributions/logistic.hpp>

using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::TZZ;


namespace WBSF
{

	class COverheat2: public COverheat
	{
	public:

		COverheat2(double overheat = 1)
		{
			m_overheat = overheat;
		}
		
		virtual double GetTmax(const CWeatherDay& weather)const;

	protected:

		double m_overheat;
	};

	double COverheat2::GetTmax(const CWeatherDay& weather)const
	{
		if (m_overheat == 1)//by optimisation, avoid compute statistic
			return weather[H_TMAX][MEAN];

		return weather[H_TNTX][MEAN] + weather[H_TRNG2][MEAN]/2 * m_overheat;
	}
	//*********************************************************************************
	//CAprocerosLeucopoda class


	//*****************************************************************************
	// Object creator
	//
	// Input: See CIndividual creator
	//
	// Note: m_RDR (relative Development Rate)  member is init with random values.
	//*****************************************************************************
	CAprocerosLeucopoda::CAprocerosLeucopoda(CHost* pHost, CTRef creationDate, double age, TSex sex, bool bFertil, size_t generation, double scaleFactor) :
		CIndividual(pHost, creationDate, age, sex, bFertil, generation, scaleFactor)
	{
		//reset creation date
		int year = creationDate.GetYear();
		//m_creationDate = GetCreationDate(year);
		m_adult_emergence = GetAdultEmergence(year);
		m_bDiapause = false;// age == PUPA;

		for (size_t s = 0; s < NB_STAGES; s++)
			m_RDR[s] = Equations().GetRelativeDevRate(s);

		//m_adult_longevity = Equations().GetAdultLongevity(m_sex);
		m_Fi = Equations().GetFecondity();
		m_bDeadByAttrition = false;
	}

	CTRef CAprocerosLeucopoda::GetCreationDate(int year)const
	{
		CTRef creationDate = CTRef(year, JANUARY, DAY_01);
		return creationDate;


		//double creationCDD = Equations().GetCreationCDD();

		//const CWeatherStation& weather_station = GetStand()->GetModel()->m_weather;
		/*CTRef begin = CTRef(year, JANUARY, DAY_01);





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

		return creationDate;*/
	}

	CTRef CAprocerosLeucopoda::GetAdultEmergence(int year)const
	{
		const CWeatherStation& weather_station = GetStand()->GetModel()->m_weather;
		CTPeriod p = weather_station[year].GetEntireTPeriod(CTM::DAILY);

		CTRef adult_emergence;
		double adult_emerging_CDD = Equations().GetAdultEmergingCDD();

		CTRef begin = p.Begin();
		//CTRef begin = GetStand()->m_diapause_end;
		CTRef end = p.End();
		//if (weather_station[year].HaveNext())
			//end = min(p.End(), CTRef(begin.GetYear() + 1, JUNE, DAY_30));

		//double CDD = 0;
		
		static const CDegreeDays::TDailyMethod DD_METHOD = CDegreeDays::ALLEN_WAVE;
		CDegreeDays DDmodel(DD_METHOD, GetStand()->m_equations.m_EAS[Τᴴ¹], GetStand()->m_equations.m_EAS[Τᴴ²]);
		
		CModelStatVector GDD;
		DDmodel.Execute(weather_station[year], GDD);

		double CDD = 0;
		for (CTRef TRef = begin; TRef <= end && !adult_emergence.IsInit(); TRef++)
		{
			if (TRef.GetJDay() >= GetStand()->m_equations.m_EAS[Tᴼ])//0 base
			{
				double DD = GDD[TRef][CDegreeDays::S_DD];
				CDD += DD;
			}

			if (CDD >= adult_emerging_CDD)
			{
				adult_emergence = TRef;
			}
		}

		return adult_emergence;
	}

	CAprocerosLeucopoda& CAprocerosLeucopoda::operator=(const CAprocerosLeucopoda& in)
	{
		if (&in != this)
		{
			CIndividual::operator=(in);

			m_adult_emergence = in.m_adult_emergence;
			m_bDiapause = in.m_bDiapause;
			m_bDeadByAttrition = in.m_bDeadByAttrition;

			for (size_t s = 0; s < NB_STAGES; s++)
				m_RDR[s] = Equations().GetRelativeDevRate(s);
			
			m_Fi = Equations().GetFecondity();;
		}

		return *this;
	}

	//destructor
	CAprocerosLeucopoda::~CAprocerosLeucopoda(void)
	{}




	void CAprocerosLeucopoda::OnNewDay(const CWeatherDay& weather)
	{
		CIndividual::OnNewDay(weather);

		/*if (weather.GetTRef() == m_creationDate)
		{
			m_age = EGG;
		}*/
	}

	//*****************************************************************************
	// Develops all stages for one time step
	// Input:	weather: weather of the hour
	//			timeStep: timeStep [h]
	//*****************************************************************************
	void CAprocerosLeucopoda::Live(const CWeatherDay& weatherD, size_t h, size_t timeStep)
	{
		assert(IsAlive());
		assert(m_status == HEALTHY);

		//COverheat2 overheat(2.0);


		CTZZHost* pHost = GetHost();
		CTZZStand* pStand = GetStand();

		double nb_steps = (24.0 / timeStep);
		//size_t h = weather.GetTRef().GetHour();
		size_t s = GetStage();

		//double T = overheat.GetT(weatherD, h);
		double T = weatherD[h][H_TAIR];
		
		//T = AdjustTLab(weather.GetWeatherStation()->m_name, s, weather.GetTRef(), T);

		//double day_length = weatherD.GetLocation().GetDayLength(weather.GetTRef()) / 3600.0;//[h]


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


		//if (!m_dropToGroundDate.IsInit() && m_age > LARVAE4 + 0.9)//drop to the soil when 90% competed (guess)
			//m_dropToGroundDate = weather.GetTRef().as(CTM::DAILY);

		//evaluate attrition once a day
		if (GetStand()->m_bApplyAttrition&&m_generation>0)
		{
			if (IsDeadByAttrition(s, T, r))
				m_bDeadByAttrition = true;
		}

		

		if (!m_adult_emergence.IsInit() && m_age >= ADULT)
			m_adult_emergence = weatherD.GetTRef().as(CTM::DAILY);

		/*else if (s == AESTIVAL_DIAPAUSE_ADULT)
		{
			CTRef TRef = weather.GetTRef().as(CTM::DAILY);
			if (TRef == m_adult_emergence)
				m_age = ACTIVE_ADULT;
		}*/
		//else//ACTIVE_ADULT
		//{
			//double r = (1.0 / m_adult_longevity) / nb_steps;
			//ASSERT(r >= 0 && r < 1);

		//	m_age += r;
		//}/
	}




	//*****************************************************************************
	// Develops all stages, including adults
	// Input:	weather: the weather of the day
	//*****************************************************************************
	void CAprocerosLeucopoda::Live(const CWeatherDay& weather)
	{
		CIndividual::Live(weather);

		ASSERT(IsCreated(weather.GetTRef()));

		if (weather.GetTRef() < m_adult_emergence)
			return;


		//if (!IsCreated(weather.GetTRef()))
			//return;

		/*if (m_bDiapause && !m_dropToGroundDate.IsInit() && weather.GetTRef() >= m_adult_emergence)
		{
			m_bDiapause = false;
			m_age = ADULT;
		}*/

		size_t nbSteps = GetTimeStep().NbSteps();
		for (size_t step = 0; step < nbSteps&&IsAlive() && m_age < DEAD_ADULT && !m_bDiapause; step++)
		{
			size_t h = step * GetTimeStep();
			//Live(weather[h], GetTimeStep());
			Live(weather, h, GetTimeStep());

			if (GetStage() == PUPA && HasChangedStage() && weather.GetTRef().GetJDay() >= 260)
			{
				m_dropToGroundDate = weather.GetTRef();
				m_bDiapause = true;
			}
		}

		if (HasChangedStage())
			m_reachDate[GetStage()] = weather.GetTRef();

	}


	void CAprocerosLeucopoda::Brood(const CWeatherDay& weather)
	{
		assert(/*IsAlive() &&*/ m_sex == FEMALE);


		if (GetStage() == ADULT)
		{
			//no brood process done

			size_t nb_days = weather.GetTRef() - m_reachDate[ADULT];
			double brood = Equations().GetBrood(m_Fi, weather[H_TNTX], nb_days, 1);
			//brooding
			m_broods = brood;
			m_totalBroods += brood;
			//m_F = 0;
		//}

			if (m_bFertil && m_broods > 0)
			{
				ASSERT(m_age >= ADULT);
				CTZZStand* pStand = GetStand(); ASSERT(pStand);

				double gSurvival = pStand->m_generationSurvival;
				double scaleFactor = m_broods * m_scaleFactor*gSurvival;
				CIndividualPtr object = make_shared<CAprocerosLeucopoda>(m_pHost, weather.GetTRef(), EGG, FEMALE, true, m_generation + 1, scaleFactor);
				m_pHost->push_front(object);
			}
		}
	}

	// kills by old age and frost
	// Output:  Individual's state is updated to follow update
	void CAprocerosLeucopoda::Die(const CWeatherDay& weather)
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

			static const double COLD_TOLERENCE_T[NB_STAGES] = { 0, 0, 0, 0, 0 };
			//ATTENTION remettre zéro!!
			if (!m_bDiapause && m_generation>1 && weather[H_TMIN][MEAN] < COLD_TOLERENCE_T[s])
			{
				m_status = DEAD;
				m_death = FROZEN;
			}
		}
	}


	//s: stage
	//T: temperature for this time step
	//r: devlopement rate for this time step
	bool CAprocerosLeucopoda::IsDeadByAttrition(size_t s, double T, double r)const
	{
		bool bDeath = false;

		//daily survival
		//double ds = GetStand()->m_equations.GetDailySurvivalRate(s, T);

		////time step survival
		//double S = pow(ds, r);

		////Computes attrition (probability of survival in a given time step, based on development rate)
		//if (RandomGenerator().RandUniform() > S)
		//	bDeath = true;

		//daily survival
		double ds = max(0.15, GetStand()->m_equations.GetDailySurvivalRate(s, T));
		//overall survival
		double Time = 1 / Equations().GetRate(s, T);//time(days)
		double S = pow(ds, Time);

		//time step survival
		double SS = pow(S, r);

		//Computes attrition (probability of survival in a given time step, based on development rate)
		if (RandomGenerator().RandUniform() > SS)
			bDeath = true;

		return bDeath;
	}



	//*****************************************************************************
	// GetStat gather information of this object
	//
	// Input: stat: the statistic object
	// Output: The stat is modified
	//*****************************************************************************
	void CAprocerosLeucopoda::GetStat(CTRef d, CModelStat& stat)
	{
		if (IsCreated(d))
		{
			size_t s = GetStage();
			ASSERT(s <= DEAD_ADULT);

			if ((IsAlive() && !m_bDiapause) || (s == DEAD_ADULT))
				stat[S_EGG + s] += m_scaleFactor;


			if (m_status == DEAD && m_death == ATTRITION)
				stat[S_DEAD_ATTRITION] += m_scaleFactor;

			if (HasChangedStage())
				stat[S_M_EGG + s] += m_scaleFactor;

			//if (s == ADULT)
			//{
			stat[S_BROOD] += m_scaleFactor * m_broods;
			//}

			if(m_bDiapause)
				stat[S_DIAPAUSE] += m_scaleFactor;
			
		}
	}


	void CAprocerosLeucopoda::Pack(const CIndividualPtr& pBug)
	{
		assert(m_sex == pBug->GetSex());

		CAprocerosLeucopoda* in = (CAprocerosLeucopoda*)(pBug.get());
		CIndividual::Pack(pBug);
	}

	double CAprocerosLeucopoda::GetInstar(bool includeLast)const
	{
		return (IsAlive() || m_death == OLD_AGE) ? GetStage() : CBioSIMModelBase::VMISS;
	}

	//*********************************************************************************************************************

	//*********************************************************************************
	//CTZZHost

	CTZZHost::CTZZHost(CStand* pStand) :
		CHost(pStand)
	{
	}


	void CTZZHost::Live(const CWeatherDay& weather)
	{
		CHost::Live(weather);
	}

	void CTZZHost::GetStat(CTRef d, CModelStat& stat, size_t generation)
	{
		CHost::GetStat(d, stat, generation);
	}

	//*************************************************
	//CTZZStand

	void CTZZStand::init(int year, const CWeatherYears& weather)
	{
//		m_diapause_end = ComputeDiapauseEnd(weather[year]);
	}

	CTRef CTZZStand::ComputeDiapauseEnd(const CWeatherYear& weather)const
	{
		CTPeriod p = weather.GetEntireTPeriod(CTM::DAILY);


		double sumDD = 0;

		for (size_t ii = (172 - 1); ii <= (m_equations.m_EWD[ʎ0] - 1); ii++)
		{
			CTRef TRef = p.Begin() + ii;
			const CWeatherDay& wday = weather.GetDay(TRef);
			double T = wday[H_TNTX][MEAN];
			T = max(m_equations.m_EWD[ʎa], T);

			double DD = min(0.0, T - m_equations.m_EWD[ʎb]);//DD is negative
			sumDD += DD;
		}

		boost::math::logistic_distribution<double> begin_dist(m_equations.m_EWD[ʎ2], m_equations.m_EWD[ʎ3]);
		int begin = (int)Round((m_equations.m_EWD[ʎ0] - 1) + m_equations.m_EWD[ʎ1] * cdf(begin_dist, sumDD), 0);


		return p.Begin() + begin;
	}



	void CTZZStand::GetStat(CTRef d, CModelStat& stat, size_t generation)
	{
		CStand::GetStat(d, stat, generation);

//		const CWeatherStation& weather_station = GetModel()->m_weather;
//		const CWeatherDay& wday = weather_station.GetDay(d);
//
//		//use year of diapause to compute correctly the adult emergence cdd
//		int year = m_diapause_end.GetYear();
//		CTRef begin = CTRef(year, JANUARY, DAY_01);
//		CTRef end = CTRef(year, DECEMBER, DAY_31);
//
//		if (d >= begin && d <= end)
//		{
//			//Egg creation DD (allen 1976)
//			//m_egg_creation_CDD += m_DD.GetDD(wday);
////			stat[S_EGG_CREATION_CDD] = m_egg_creation_CDD;
//
//			//diapause end negative DD
//			double T = wday[H_TNTX][MEAN];
//			T = max(m_equations.m_EWD[ʎa], T);
//			double NDD = min(0.0, T - m_equations.m_EWD[ʎb]);//DD is negative
//
//			int ii = d - begin;
//			if (ii >= (172 - 1) && ii <= int(m_equations.m_EWD[ʎ0] - 1))
//				m_diapause_end_NCDD += NDD;
//
//			//stat[S_DIAPAUSE_END_NCDD] = m_diapause_end_NCDD;
//		}
//

		//begin = m_diapause_end;
		//end = CTRef(m_diapause_end.GetYear() + 1, MARCH, DAY_01);
		//if (d >= begin && d <= end)
		//{
		//	//adult emergence (growing DD)
		//	double T = wday[H_TNTX][MEAN];
		//	double GDD = max(0.0, T - m_equations.m_EAS[Τᴴ]);
		//	m_adult_emergence_CDD += GDD;
		//	stat[S_ADULT_EMERGENCE_CDD] = m_adult_emergence_CDD;
		//}
	}


}