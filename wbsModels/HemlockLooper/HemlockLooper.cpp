//*****************************************************************************
// File: HLRates.h
//
// Class: CHemlockLooper
//          
//
// Descrition: the CHemlockLooper represents one hemlock looper. 
//*****************************************************************************
// 25/02/2015	Remi Saint-Amant	Update with new BioSIMModelBase
// 17/04/2014   Jacques Régnière    Creation from wsb model
//*****************************************************************************
#include <math.h>
#include "basic/UtilMath.h"
#include "basic/WeatherStation.h"
#include "HemlockLooper.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::HemlockLooper;

namespace WBSF
{
	//******************************************************************
	//CHemlockLooper class


	const double CHemlockLooper::FREEZING_POINT = -10;
	const double CHemlockLooper::PRE_OVIPOSITION = 1;

	//*****************************************************************************
	// Object creator
	//
	// Input: int creationDay: the day of the creation
	//		  int stage: the stage of the insect when the object is created
	//        
	//
	// Output: 
	//
	// Note: m_relativeDevRate member is modified.
	//*****************************************************************************
	CHemlockLooper::CHemlockLooper(CHost* pHost, CTRef creationDate, double age, size_t sex, bool bFertil, size_t generation, double scaleFactor) :
		CIndividual(pHost, creationDate, age, sex, bFertil, generation, scaleFactor)
	{
		m_adultAge = 0;
		m_Sh = 1;
		m_ʃT = 0;
		m_hatchSurvival = 1;
		m_Tmin = 999;

		//no pre-diapause for insects created on January first
		m_preDiapause = creationDate.GetJDay() == 0 ? 1 : 0;
		//first year insect are set at September 15
		//m_preDiapause = 0;

		//A creation date is assigned to each individual
		//Individual's "relative" development rate for each life stage
		//These are independent in successive life stages
		AssignRelativeDevRate();

	}



	//*****************************************************************************
	// AssignRelativeDevRate sets the relative development rate for all stages
	//
	// Input: 
	//
	// Output: 
	//
	// Note: m_relativeDevRate member is modified.
	//*****************************************************************************
	void CHemlockLooper::AssignRelativeDevRate()
	{
		ASSERT(GetStand());
		const CHLStand& stand = *GetStand();
		double lat = stand.GetModel()->m_weather.m_lat;

		for (int s = 0; s < NB_STAGES; s++)
			m_relativeDevRate[s] = stand.m_development.GetRelativeRate(s, m_sex);


		m_overwinterLuck = stand.m_survival.GetWinterSurvival();
		m_potentialFecundity = stand.m_oviposition.GetFecundity(lat);
		m_preDiapauseRelativeDevRate = stand.m_development.GetPreDiapauseRelativeRate();
	}




	//*****************************************************************************
	// Live is for one day development
	//
	// Input: CDailyWaveVector T: the temperature for all time steps
	//
	// Output: return true if the bug continues to be in the population, false otherwise
	//*****************************************************************************

	void CHemlockLooper::Live(const CWeatherDay& weather)
	{
		ASSERT(GetStand());

		const CHLStand& stand = *GetStand();
		double lat = stand.GetModel()->m_weather.m_lat;
		size_t nbSteps = GetTimeStep().NbSteps();


		CIndividual::Live(weather);

		for (size_t step = 0; step < nbSteps && GetStage() < DEAD_ADULTS; step++)
		{
			size_t h = step*GetTimeStep();
			double T = weather[h][H_TAIR];

			size_t s = GetStage();


			double r = m_relativeDevRate[s] * stand.m_development.GetRate(s, lat, T) / nbSteps;
			_ASSERTE(r >= 0);

			if (s == EGGS)
			{
				// Question pour Remi: Est-ce que ceci signifie que les oeufs initiaux sont créés le 1 janvier de la première année et sont soumis à la perte d'énergie à partir du 1 janvier? Je ne comprends pas comment la simulation commence...
				// Réponse : Ça dépend si on est la première année ou non. voir ligne 92. On ne s'attend pas a avoir de la mortalité la première année. Seulement les années subséquantes.

				if (m_preDiapause<1)
					m_preDiapause += stand.m_development.GetPreDiapauseRate(T)*m_preDiapauseRelativeDevRate / nbSteps;
				else
					m_ʃT += stand.m_survival.SenergyʃT(T) / nbSteps;

				m_Tmin = min(m_Tmin, T);
				if (weather.GetTRef().GetYear()>m_creationDate.GetYear())//hatch survival starts January first of the next year
					m_Sh *= stand.m_survival.Shatch(T, GetTimeStep() / 24.0);

				if (IsChangingStage(r))
				{
					//compute hatch survival
					m_hatchSurvival = stand.m_survival.GetEggSurvival(lat, m_Sh, m_ʃT, m_Tmin);
					m_hatchTRef = weather.GetTRef();
				}
			}

			if (m_creationDate.GetJDay() == 0 ||
				weather.GetTRef().GetYear() > m_creationDate.GetYear())//aging starts January first of the next year
				m_age += r;
		}

	}

	void CHemlockLooper::Brood(const CWeatherDay& weather)
	{
		ASSERT(GetStand());
		ASSERT(m_sex == FEMALE);

		const CHLStand& stand = *GetStand();
		double lat = stand.GetModel()->m_weather.m_lat;
		size_t nbSteps = GetTimeStep().NbSteps();

		CIndividual::Brood(weather);

		if (GetStage() >= ADULTS)
		{
			for (size_t h = 0; h < 24 && IsAlive(); h += GetTimeStep())
			{
				m_adultAge += 1.0 / nbSteps;
				//Each day t, at temperature T
				//Females begin oviposition after 1 days pre-oviposition period
				if (m_adultAge > PRE_OVIPOSITION)
				{
					_ASSERTE(IsAlive());
					double T = weather[h][H_TAIR];
					double Fᵗ = m_potentialFecundity - m_totalBroods;
					double Eᵗ = stand.m_oviposition.GetRate(T, m_potentialFecundity, Fᵗ) / nbSteps;
					m_broods += Eᵗ;
					m_totalBroods += Eᵗ;
				}
			}

			if (m_bFertil&&m_broods > 0)
			{
				//CHLTree* pTree = GetTree(); ASSERT(pTree);
				CHLStand* pStand = GetStand(); ASSERT(pStand);

				double scaleFactor = m_broods*m_scaleFactor;
				ASSERT(scaleFactor > 0);

				m_pHost->push_front(make_shared<CHemlockLooper>(m_pHost, weather.GetTRef(), EGGS, NOT_INIT, pStand->m_bFertilEgg, m_generation + 1, scaleFactor));
			}
		}
	}


	void CHemlockLooper::Die(const CWeatherDay& weather)
	{
		assert(IsAlive());

		CIndividual::Die(weather);

		//Mortality: eggs and adults can be killed by cold. Adults die of old age.
		if (GetStage() == DEAD_ADULTS)
		{
			m_status = DEAD;
			m_death = OLD_AGE;
		}
		else 
		{
			if (GetStand()->m_bApplyMortality)
			{
				if (GetStage() > EGGS)
				{
					if (GetStage() == L1 && HasChangedStage())
					{
						//m_hatchSurvival change only once at hatch, 1 by default
						if (m_hatchSurvival < m_overwinterLuck)
						{
							m_status = DEAD;
							m_death = MISSING_ENERGY;
						}
					}

					double attrition = RandomGenerator().Randu();
					if (attrition>GetStand()->m_survival.GetAttrition())
					{
						m_status = DEAD;
						m_death = ATTRITION;
					}
				
					if( weather[H_TMIN][MEAN] < CHemlockLooper::FREEZING_POINT)
					{
						m_status = DEAD;
						m_death = FROZEN;
					}
				}
			}
		}
	}

	void CHemlockLooper::HappyNewYear()
	{
		if (GetStage() != EGGS)
		{
			m_status = DEAD;
			m_death = FROZEN;
		}
	}

	//*****************************************************************************
	// GetStat: GetStat is called daily to get the state of the object
	//
	// Input: CTRef d: the actual day
	// CHLStat& stat: the statistic object 
	//
	// Output: The stat is modified
	//*****************************************************************************
	void CHemlockLooper::GetStat(CTRef d, CModelStat& stat)
	{
		assert(stat.size() == NB_HL_STAT);

		size_t oldStage = short(m_lastAge);
		size_t stage = GetStage();

		//total and daily brood
		stat[S_BROODS] += m_totalBroods*m_scaleFactor;
		stat[E_BROODS] += m_broods*m_scaleFactor;

		if (IsAlive() || stage == DEAD_ADULTS)
		{
			ASSERT(m_generation == 0);
			stat[S_EGGS + stage] += m_scaleFactor;
		}
		

		if (stage != oldStage)
		{
			stat[E_EGGS + stage] += m_scaleFactor;
			if (stage == ADULTS && m_sex == FEMALE)
				stat[E_FEMALES] += m_scaleFactor;
		}

		if (d == m_hatchTRef)
		{
			if (m_creationDate.GetJDay() == 0)
			{
				const CHLStand& stand = *GetStand();
				double L = stand.GetModel()->m_weather.m_lat;

				stat[E_SWEIGHT] += stand.m_survival.Sweight(L)*m_scaleFactor;
				stat[E_SENERGY] += stand.m_survival.Senergy(m_ʃT)*m_scaleFactor;
				stat[E_SCOLD] += stand.m_survival.Scold(m_Tmin)*m_scaleFactor;
				stat[E_SHATCH] += m_Sh*m_scaleFactor;
				stat[E_NB_HATCH] += m_scaleFactor;
			}
		}

		if (!IsAlive())
		{
			stat[S_DEAD] += m_scaleFactor;

			if (m_death == ATTRITION)
				stat[S_DEAD_ATTRITION] += m_scaleFactor;
			else if (m_death == MISSING_ENERGY)
				stat[S_DEAD_OVERWINTER] += m_scaleFactor;
			else if (m_death == FROZEN)
				stat[S_DEAD_FROZEN] += m_scaleFactor;
			

		}
	}

	//WARNING: cast must be defined here to avoid bug
	CHLTree* CHemlockLooper::GetTree(){ return static_cast<CHLTree*>(m_pHost); }
	const CHLTree* CHemlockLooper::GetTree()const{ return static_cast<const CHLTree*>(m_pHost); }
	CHLStand* CHemlockLooper::GetStand(){ ASSERT(m_pHost); return (CHLStand*)m_pHost->GetStand(); }
	const CHLStand* CHemlockLooper::GetStand()const{ ASSERT(m_pHost); return (const CHLStand*)m_pHost->GetStand(); }



	//******************************************************************
	//CHLTree class

	CHLTree::CHLTree(CStand* pStand) :CHost(pStand)
	{
		clear();
	}


	void CHLTree::GetStat(CTRef d, CModelStat& stat, size_t generation)
	{
		CHost::GetStat(d, stat, generation);
		stat[S_AVERAGE_INSTAR] = GetAI(true);

	}

}