//*****************************************************************************
//*****************************************************************************
// Class: CSpottedLanternfly
//          
//
// Description: the CSpottedLanternfly represents a group of LDW insect. scale by m_ScaleFactor
//*****************************************************************************
// 30/10/2022   Rémi Saint-Amant    Creation
//*****************************************************************************

#include "SpottedLanternflyEquations.h"
#include "SpottedLanternfly.h"
#include <boost/math/distributions/weibull.hpp>
#include <boost/math/distributions/logistic.hpp>

using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::LDW;


namespace WBSF
{

	//*********************************************************************************
	//CSpottedLanternfly class


	//*****************************************************************************
	// Object creator
	//
	// Input: See CIndividual creator
	//
	// Note: m_RDR (relative Development Rate)  member is init with random values.
	//*****************************************************************************
	CSpottedLanternfly::CSpottedLanternfly(CHost* pHost, CTRef creationDate, double age, TSex sex, bool bFertil, size_t generation, double scaleFactor) :
		CIndividual(pHost, creationDate, age, sex, bFertil, generation, scaleFactor)
	{
		//reset creation date
		for (size_t s = 0; s < NB_STAGES; s++)
			m_RDR[s] = Equations().GetRelativeDevRate(s);

		//m_EOD = Equations().GetEndOfDiapauseCDD();
		//m_EODDate = GetStand()->m_EOD_CDD.GetFirstTRef(0, ">", m_EOD,0);
		//if (!m_EODDate.IsInit() ||m_EODDate.GetJDay() > 151)
			//m_EODDate = CTRef(creationDate.GetYear(), JUNE, DAY_01);


		m_bDeadByAttrition = false;
		//if (GetStand()->m_sigma > 0)
			//m_RDR[ADULT] = Equations().GetRelativeDevRate(GetStand()->m_sigma);
		//		m_Fi = (m_sex == FEMALE) ? Equations().GetFecondity() : 0;

	}

	CSpottedLanternfly& CSpottedLanternfly::operator=(const CSpottedLanternfly& in)
	{
		if (&in != this)
		{
			CIndividual::operator=(in);

			//new relative development rate
			for (size_t s = 0; s < NB_STAGES; s++)
				m_RDR[s] = Equations().GetRelativeDevRate(s);

			//m_adult_emergence = in.m_adult_emergence;

			m_EOD = m_EOD;
			m_EODDate = m_EODDate;

			m_reachDate = in.m_reachDate;
		}

		return *this;
	}

	//destructor
	CSpottedLanternfly::~CSpottedLanternfly(void)
	{}




	void CSpottedLanternfly::OnNewDay(const CWeatherDay& weather)
	{
		CIndividual::OnNewDay(weather);
	}

	//*****************************************************************************
	// Develops all stages for one time step
	// Input:	weather: weather of the hour
	//			timeStep: timeStep [h]
	//*****************************************************************************
	void CSpottedLanternfly::Live(const CHourlyData& weather, size_t timeStep)
	{
		assert(IsAlive());
		assert(m_status == HEALTHY);


		double nb_steps = (24.0 / timeStep);
		size_t h = weather.GetTRef().GetHour();
		size_t s = GetStage();

		double T = weather[H_TAIR];

		//Time step development rate
		double r = Equations().GetRate(s, T) / nb_steps;

		//Relative development rate for this individual
		double rr = m_RDR[s];
		r *= rr;

		//correction factor
		double rrr = Equations().m_psy[s];
		r *= rrr;

		ASSERT(r >= 0 && r < 1);

		//Adjust age
		m_age += r;


		if (GetStand()->m_bApplyAttrition)
		{
			if (IsDeadByAttrition(s, T, r))
				m_bDeadByAttrition = true;
		}


		//if (m_sex == FEMALE && GetStage() >= ACTIVE_ADULT)
		//{
		//	double to = 0;
		//	double t = timeStep / 24.0;
		//	double λ = Equations().GetFecondityRate(GetAge(), weather[H_TAIR]);
		//	double brood = m_Fi * (exp(-λ * (m_t - to)) - exp(-λ * (m_t + t - to)));

		//	m_broods += brood;
		//	m_totalBroods += brood;

		//	m_t += t;
		//}

	}




	//*****************************************************************************
	// Develops all stages, including adults
	// Input:	weather: the weather of the day
	//*****************************************************************************
	void CSpottedLanternfly::Live(const CWeatherDay& weather)
	{
		CIndividual::Live(weather);

		ASSERT(IsCreated(weather.GetTRef()));

		if (!IsCreated(weather.GetTRef()))
			return;

		//if (weather.GetTRef() < m_EODDate)
		//	return;

		//if (weather.GetTRef() == m_EODDate)//no development before end of diapause
		//{
		//	m_age = N1;
		//}

		size_t nbSteps = GetTimeStep().NbSteps();
		for (size_t step = 0; step < nbSteps && IsAlive(); step++)
		{
			size_t h = step * GetTimeStep();
			Live(weather[h], GetTimeStep());
		}

		if (weather.GetTRef() == m_creationDate || HasChangedStage())
			m_reachDate[GetStage()] = weather.GetTRef();


	}


	void CSpottedLanternfly::Brood(const CWeatherDay& weather)
	{
		assert(/*IsAlive() &&*/ m_sex == FEMALE);


		if (GetStage() == ADULT)
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
	void CSpottedLanternfly::Die(const CWeatherDay& weather)
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

			if (GetStand()->m_bApplyFrost)
			{
				if (s == EGG)
				{
					if (weather.GetTRef().GetMonth() == JANUARY)
					{
						if (GetStand()->RandomGenerator().Randu() > GetStand()->m_overwinter_s)
						{
							m_status = DEAD;
							m_death = FROZEN;
						}
					}
				}
				/*else
				{
					if (weather[H_TMIN][MEAN] < -5)
					{
						m_status = DEAD;
						m_death = FROZEN;
					}
				}*/

			}
		}
	}


	//s: stage
	//T: temperature for this time step
	//r: development rate for this time step
	bool CSpottedLanternfly::IsDeadByAttrition(size_t s, double T, double r)const
	{
		bool bDeath = false;

		if (T >= 0)//under zero is manage by the frost mortality
		{
			//daily survival
			double ds = GetStand()->m_equations.GetDailySurvivalRate(s, T);
			//overall survival
			double Time = 1 / Equations().GetRate(s, T);//time(days)
			double S = pow(ds, Time);

			//time step survival
			double SS = pow(S, r);

			//Computes attrition (probability of survival in a given time step, based on development rate)
			if (RandomGenerator().RandUniform() > SS)
				bDeath = true;
		}



		return bDeath;
	}



	//*****************************************************************************
	// GetStat gather information of this object
	//
	// Input: stat: the statistic object
	// Output: The stat is modified
	//*****************************************************************************
	void CSpottedLanternfly::GetStat(CTRef d, CModelStat& stat)
	{
		if (IsCreated(d))
		{
			size_t s = GetStage();
			ASSERT(s <= DEAD_ADULT);

			if (IsAlive() || (s == DEAD_ADULT))
				stat[S_EGG + s] += m_scaleFactor;



			//if (HasChangedStatus() && m_status == DEAD && m_death == ATTRITION)
				//stat[S_DEAD_ATTRTION] += m_scaleFactor;

			//if(d==m_EODDate)
			//stat[S_EOD] += m_scaleFactor* GetStand()->m_diapause[d][0];


			if (HasChangedStage() && s == N1)
				stat[S_EGG_HATCH] += m_scaleFactor;


			if (HasChangedStage() && s == ADULT)
				stat[S_ADULT_EMERGENCE] += m_scaleFactor;



			//if (HasChangedStatus() && m_status == DEAD && m_death == ATTRITION)
			if (m_status == DEAD && m_death == ATTRITION)
				stat[S_DEAD_ATTRITION] += m_scaleFactor;

			if (m_status == DEAD && m_death == FROZEN)
				stat[S_DEAD_BY_FROST] += m_scaleFactor;

			//if (s == ACTIVE_ADULT)
			//{
			//	stat[S_ADULT_ABUNDANCE] += m_scaleFactor * m_adult_abundance;
			//}
		}
	}


	void CSpottedLanternfly::Pack(const CIndividualPtr& pBug)
	{
		assert(m_sex == pBug->GetSex());

		CSpottedLanternfly* in = (CSpottedLanternfly*)(pBug.get());
		CIndividual::Pack(pBug);
	}

	double CSpottedLanternfly::GetInstar(bool includeLast)const
	{
		return (IsAlive() || m_death == OLD_AGE) ? GetStage() : CBioSIMModelBase::VMISS;
	}

	//*********************************************************************************************************************

	//*********************************************************************************
	//CLDWHost

	CLDWHost::CLDWHost(CStand* pStand) :
		CHost(pStand)
	{
	}


	void CLDWHost::Live(const CWeatherDay& weather)
	{
		CHost::Live(weather);
	}

	void CLDWHost::GetStat(CTRef d, CModelStat& stat, size_t generation)
	{
		CHost::GetStat(d, stat, generation);

		stat[S_AVEARGE_INSTAR] = stat.GetAverageInstar(S_EGG, 0, S_DEAD_ADULT);
		ASSERT(stat[S_AVEARGE_INSTAR] <= DEAD_ADULT);

	}

	//*************************************************
	//CLDWStand

	CLDWStand::CLDWStand(WBSF::CBioSIMModelBase* pModel) :
		WBSF::CStand(pModel),
		m_equations(pModel->RandomGenerator()),
		m_bApplyAttrition(true),
		m_bApplyFrost(true),
		m_overwinter_s(1)
	{
		//m_sigma = 0;
		//m_psy_factor = 1;
	}


	void CLDWStand::InitStand(const array<double, NB_CDD_PARAMS >& P, const CWeatherYear& weather)
	{
		m_equations.m_EOD = P;
		//CModelDistribution::get_CDD(P, weather, m_EOD_CDD);
		//CModelDistribution::get_CDF(P, weather, m_EOD_CDF);

		//winter survival
		double S = CModelDistribution::get_cdf(weather[JANUARY][H_TMIN][MEAN], CModelDistribution::GOMPERTZ2, -3.375384, 0.2685411);
		//double S = CModelDistribution::get_cdf(max(0.0, -weather[JANUARY][H_TMIN][MEAN]), CModelDistribution::WEIBULL, 12.387019, 2.959794);
		


		//daily January survival
		m_overwinter_s = pow(S, 1.0 / 31.0);
	}

}