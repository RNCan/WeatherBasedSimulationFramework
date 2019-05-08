//*****************************************************************************
// Class: CWhitemarkedTussockMoth
//          
//
// Description: the CWhitemarkedTussockMoth represents a group of WTM insect. scale by m_ScaleFactor
//*****************************************************************************
// 22/04/2019   Rémi Saint-Amant    Creation
//*****************************************************************************

#include "WhitemarkedTussockMothEquations.h"
#include "WhitemarkedTussockMoth.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::WTM;


namespace WBSF
{

	//*********************************************************************************
	//CWhitemarkedTussockMoth class


	//*****************************************************************************
	// Object creator
	//
	// Input: See CIndividual creator
	//
	// Note: m_RDR (relative Development Rate)  member is init with random values.
	//*****************************************************************************
	CWhitemarkedTussockMoth::CWhitemarkedTussockMoth(CHost* pHost, CTRef creationDate, double age, TSex sex, bool bFertil, size_t generation, double scaleFactor) :
		CIndividual(pHost, creationDate, age, sex, bFertil, generation, scaleFactor)
	{
		//Individual's "relative" development rate for each life stage
		//These are independent in successive life stages
		for (size_t s = 0; s < NB_STAGES; s++)
			m_RDR[s] = Equations().GetRelativeDevRate(s);
		
		m_GDD = 0;
		m_F = Equations().GetFecondity();
		m_transit[int(age)] = creationDate.as(CTM::DAILY);

		m_diapauseGDD = 0;
		if (generation == 0)
			m_diapauseGDD = Equations().GetHatchCDD();//DD after brood to hatch

	}



	CWhitemarkedTussockMoth& CWhitemarkedTussockMoth::operator=(const CWhitemarkedTussockMoth& in)
	{
		if (&in != this)
		{
			CIndividual::operator=(in);

			//regenerate relative development rate
			for (size_t s = 0; s < NB_STAGES; s++)
				m_RDR[s] = Equations().GetRelativeDevRate(s);

		}

		return *this;
	}
	// Object destructor
	CWhitemarkedTussockMoth::~CWhitemarkedTussockMoth(void)
	{}

	/*static double Zalom83(double Tmin, double Tmax, double Tresh)
	{
		double Q = asin((Tresh - (Tmax + Tmin) / 2.0) / ((Tmax - Tmin) / 2.0));

		double DD = 0;
		if (Tmax > Tresh)
		{
			if (Tmin >= Tresh)
			{
				DD = (Tmax + Tmin) / 2.0 - Tresh;
			}
			else
			{
				DD = 1.0 / PI * (((Tmax + Tmin) / 2.0 - Tresh)*(PI / 2.0 - Q) + (Tmax - Tmin) / 2.0 * cos(Q));
			}
		}

		return DD;
	}

	static double Zalom83(double Tmin, double Tmax, double Tlo, double Thi)
	{
		double DDlo = Zalom83(Tmin, Tmax, Tlo);
		double DDhi = Zalom83(Tmin, Tmax, Thi);
		return DDlo - DDhi;
	}*/

	void CWhitemarkedTussockMoth::OnNewDay(const CWeatherDay& weather)
	{
		CIndividual::OnNewDay(weather);
		//double Tmin = weather[H_TMIN][MEAN];
		//double Tmax = weather[H_TMAX][MEAN];
		//double DDAY = Zalom83(Tmin, Tmax, Equations().m_H[Τᴴ], 999);
		//m_GDD += DDAY;
		if(weather.GetTRef().GetJDay()>=Equations().m_H[startᴴ])
			m_GDD += GetStand()->m_DD.GetDD(weather);
	}

	//*****************************************************************************
	// Develops all stages for one time step
	// Input:	weather: weather of the hour
	//			timeStep: timeStep [h]
	//*****************************************************************************
	void CWhitemarkedTussockMoth::Live(const CHourlyData& weather, size_t timeStep)
	{
		assert(IsAlive());
		assert(m_status == HEALTHY);

		CWTMHost* pHost = GetHost();
		CWTMStand* pStand = GetStand();

		size_t h = weather.GetTRef().GetHour();
		size_t g = GetGeneration();
		size_t s = GetStage();

	/*	if (GetGeneration() == 1 && s == EGG)
		{
			if (m_GDD >= m_hatchGDD)
				m_age = LARVAE;
		}
		else*/
		
		{
			//begin development after output diapause
			//if (weather.GetTRef().GetJDay() >= Equations().m_H[startᴴ])
			if (m_GDD > m_diapauseGDD)
			{
				double T = weather[H_TAIR];

				//Time step development rate
				double r = Equations().GetRate(s, T) / (24.0 / timeStep);
				if (s == EGG)
					r *= GetStand()->m_egg_factor[g];
				//Relative development rate for this individual
				double rr = m_RDR[s];

				if(IsChangingStage(rr))
					m_transit[s+1] = weather.GetTRef().as(CTM::DAILY);
				
				//Time step development rate for this individual
				r *= rr;

				
				//Adjust age
				m_age = min(double(DEAD_ADULT), m_age + r);
			}
		}

	}



	//*****************************************************************************
	// Develops all stages, including adults
	// Input:	weather: the weather of the day
	//*****************************************************************************
	void CWhitemarkedTussockMoth::Live(const CWeatherDay& weather)
	{
		//For optimization, nothing happens when temperature is under 0
		if (weather[H_TMAX][MEAN] < 0)
			return;

		size_t nbSteps = GetTimeStep().NbSteps();
		for (size_t step = 0; step < nbSteps&&m_age < DEAD_ADULT; step++)
		{
			size_t h = step * GetTimeStep();
			Live(weather[h], GetTimeStep());
		}

	}


	void CWhitemarkedTussockMoth::Brood(const CWeatherDay& weather)
	{
		assert(IsAlive() && m_sex == FEMALE);


		if (GetStage() == ADULT)
		{
			//brooding
			//if (m_F > 0 && GetStageAge() >= Equations().m_H[μ])//weather.GetTRef() - m_transit[ADULT] >= 2
			if (m_F > 0 && GetStageAge() >= 0.33)
			{
				m_broods = m_F;
				m_totalBroods = m_F;
				m_F = 0;

				//Oviposition 
				if (/*m_bFertil */GetGeneration()==0)
				{
					CWTMHost* pHost = GetHost();
					CWTMStand* pStand = GetStand(); ASSERT(pStand);

					//add 2 individual per female to keep variability
					double scaleFactor = m_broods * m_scaleFactor;
					CIndividualPtr object1 = make_shared<CWhitemarkedTussockMoth>(GetHost(), weather.GetTRef(), EGG, RANDOM_SEX, false, m_generation + 1, scaleFactor);
					pHost->push_front(object1);
					CIndividualPtr object2 = make_shared<CWhitemarkedTussockMoth>(GetHost(), weather.GetTRef(), EGG, RANDOM_SEX, false, m_generation + 1, scaleFactor);
					pHost->push_front(object2);
				}
			}
		}
	}

	// kills by attrition, old age, frost and overwintering
	// Output:  Individual's state is updated to follow update
	void CWhitemarkedTussockMoth::Die(const CWeatherDay& weather)
	{
		//attrition mortality. Killed at the end of time step 
		if (GetStage() == DEAD_ADULT)
		{
			//Old age
			m_status = DEAD;
			m_death = OLD_AGE;


			//if (GetSex() == FEMALE)
			//{
			//	//brooding
			//	m_broods = m_F;
			//	m_totalBroods = m_F;
			//	m_F = 0;
			//}
		}
	}

	//*****************************************************************************
	// GetStat gather information of this object
	//
	// Input: stat: the statistic object
	// Output: The stat is modified
	//*****************************************************************************
	void CWhitemarkedTussockMoth::GetStat(CTRef d, CModelStat& stat)
	{
		ASSERT(IsCreated(d));

		size_t s = GetStage();
		size_t g = GetGeneration();

		//		stat[S_BROOD] += m_broods * m_scaleFactor;
		if (g == 0)
		{
			if (IsAlive() || s == DEAD_ADULT)
			{
				stat[S_EGG0 + s] += m_scaleFactor;
			}

			if (m_broods > 0)
				stat[S_EGG_MASS0] += m_scaleFactor;//we compute mass and not eggs

			if(HasChangedStage() && s == ADULT)
				stat[S_EMERGENCE0] += m_scaleFactor;
		}

		
		if (g == 1)
		{
			if (IsAlive() || s == DEAD_ADULT)
			{
				stat[S_EGG1 + s] += m_scaleFactor;
			}

			if (m_broods > 0)
				stat[S_EGG_MASS1] += m_scaleFactor;//we compute mass and not eggs

			if (HasChangedStage() && s == ADULT)
				stat[S_EMERGENCE1] += m_scaleFactor;
		}


		stat[S_GDD] = m_GDD;
	}

	void CWhitemarkedTussockMoth::Pack(const CIndividualPtr& pBug)
	{
		assert(m_sex == pBug->GetSex());

		CWhitemarkedTussockMoth* in = (CWhitemarkedTussockMoth*)(pBug.get());
		CIndividual::Pack(pBug);
	}

	double CWhitemarkedTussockMoth::GetInstar(bool includeLast)const
	{
		return (IsAlive() || m_death == OLD_AGE) ? GetStage() : CBioSIMModelBase::VMISS;
	}

	//*********************************************************************************************************************

	//*********************************************************************************
	//CWTMHost

	CWTMHost::CWTMHost(CStand* pStand) :
		CHost(pStand)
	{
	}


	void CWTMHost::Live(const CWeatherDay& weather)
	{
		CHost::Live(weather);
	}

	void CWTMHost::GetStat(CTRef d, CModelStat& stat, size_t generation)
	{
		CHost::GetStat(d, stat, generation);

		//stat[S_AVERAGE_INSTAR1] = GetAI(true, 0);
		//stat[S_AVERAGE_INSTAR2] = GetAI(true, 1);
	}

	//*************************************************

}