//*****************************************************************************
// Class: CBagworm
//          
//
// Descrition: the CBagworm represents one SBW insect. 
//*****************************************************************************
//*****************************************************************************

#include "Bagworm.h"
#include "Basic/WeatherStation.h"
#include "Basic/UtilMath.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;

namespace WBSF
{


	//*********************************************************************************
	//CBagworm class



	//*****************************************************************************
	// Object creator
	//
	// Input: CTRef creationDate: the day of the creation
	//		  double age: age of the insect when the object is created
	//        
	//
	// Output: 
	//
	// Note: m_relativeDevRate member is modified.
	//*****************************************************************************
	CBagworm::CBagworm(WBSF::CHost* pHost, CTRef creationDate, double age, size_t sex, bool bFertil, size_t generation, double scaleFactor) :
		CIndividual(pHost, creationDate, age, sex, bFertil, generation, scaleFactor)
	{
		//Individual's "relative" development rate for each life stage
		//These are independent in successive life stages
		
		m_relativeDevRate[EGG] = RandomGenerator().RandLogNormal(5.123, .318);
		m_relativeDevRate[LARVAL] = RandomGenerator().RandNormal(2901.1, 191.1);
		m_relativeDevRate[ADULT] = 1;

//		m_fec = 1000;
		m_clusterWeight = 0;
	}

	// Object destructor
	CBagworm::~CBagworm(void)
	{}

	CBagworm& CBagworm::operator=(const CBagworm& in)
	{
		CIndividual::operator=(in);
		m_clusterWeight = in.m_clusterWeight;
		
		return *this;
	}


	//*****************************************************************************
	// Live is for one day development
	//
	// Input:	T: the temperature for all time steps
	//			tree: The tree of the insecs
	// Output:  Individual's state is updated to follow update
	//*****************************************************************************
	void CBagworm::Live(const CWeatherDay& weather)
	{
		//_ASSERTE(T.size() <= 24);//only one day at a time
		//_ASSERTE(T.GetFirstTRef() >= m_creationDate);

		//if (!IsAlive())
			//return;

		CIndividual::Live(weather);

		CTRef TRef = weather.GetTRef();

		if (GetStage() == EGG_DIAPAUSE &&
			TRef.GetMonth() == JANUARY && TRef.GetDay() == DAY_01)
		{
			m_age = EGG;
		}


		//approximation of daily minimum and maximum
		CStatistic Tday;
		for (size_t h = 0; h < weather.size(); h++)
			Tday += weather[h][H_TNTX];


		if (m_Tday[NB_VALUE] == 0 || Tday[LOWEST] < m_Tday[LOWEST])
		{
			m_dayWhenMinimum = TRef;
			m_Tday = Tday;
		}

		size_t nbSteps = GetTimeStep().NbSteps();
		for (size_t step = 0; step < nbSteps&&IsAlive(); step++)
		{
			size_t h = step*GetTimeStep();
			
			//Development, reproduction and attrition
			Develop(weather[h].GetTRef(), weather[h][H_TNTX], nbSteps);
		}

		

	}


	void CBagworm::Brood(const CWeatherDay& weather)
	{
		ASSERT(IsAlive());
		ASSERT(m_sex == FEMALE);

		CIndividual::Brood(weather);

		//CBWHost* pTree = (CBWHost *)GetHost();

//		m_broods = 0; //Individual number of Egg for this day
	//	m_totalBroods = 0;//total number of eggs

		//Oviposition module after Regniere 1983
		if (m_bFertil && m_broods > 0)
		{

			if (m_sex == FEMALE && m_totalBroods == 0 && m_age >= ADULT)
			{
				short Tf = weather.GetTRef().GetJDay();
				double clusterWeight = max(0.0, 1351 - 3.95*Tf);

				//adjust fecondity in function of clusterWeight
				m_broods = clusterWeight*2.45; //2.45 eggs/mg
				m_totalBroods = m_broods;

				//add 2 bugs 
				CBagworm* pBug = new CBagworm(m_pHost, weather.GetTRef(), EGG_DIAPAUSE, RANDOM_SEX, true, m_generation + 1, m_scaleFactor*2);
				pBug->SetClusterWeight(clusterWeight);
				m_pHost->push_front(CIndividualPtr (pBug) );
				//pBug = new CBagworm(T.GetFirstTRef(), EGG_DIAPAUSE, true, m_generation + 1, 1/*m_scaleFactor*m_fec/2*/, m_pHost);
				//pBug->SetClusterWeight(clusterWeight);
				//pTree->AddBug(pBug);


				//m_brood = m_fec; //Weight of egg for this day
				//m_nbEgg = m_fec;//total number of eggs
				//m_fec = 0;
			}
		}
	}

	void CBagworm::Die(const CWeatherDay& weather)
	{
		CTRef TRef = weather.GetTRef();
		//attrition mortality. Killed at the end of time step 
		if (GetStage() == DEAD_ADULT)
		{
			//Old age
			m_status = DEAD;
			m_death = OLD_AGE;
		}
		else if (m_clusterWeight > 0 && GetStage() == EGG && HasChangedStage())
		{
			//compute winter survival 
			double w = m_clusterWeight / 1000;  //weight in g
			double T = m_Tday[HIGHEST];
			double s = 1 / (1 + exp(-(0.14*T + 65.8*w + 3.29*T*w)));

			if (Randu() > s)
			{
				m_status = DEAD;
				m_death = FROZEN;
			}
		}
		else if (GetStage() > EGG && weather[H_TMIN2][MEAN] < -10)
		{
			m_status = DEAD;
			m_death = FROZEN;
		}
		//all bug dead at the end of the year
		else if (GetStage() != EGG_DIAPAUSE && TRef.GetMonth() == DECEMBER && TRef.GetDay() == DAY_31)
		{
			m_status = DEAD;
			m_death = OTHERS;
		}

	}

	double CBagworm::GetDevRate(size_t s, double T)const
	{
		double DD = 0;

		switch (s)
		{
		case EGG_DIAPAUSE: DD = 0; break;
		case EGG: DD = max(0.0, T - 14.4); break;
		case LARVAL:DD = max(0.0, T - (-4)); break;
		case ADULT: DD = 0.2; break;//how many day live adult????? (I take 5 days by default)
		default: ASSERT(false);
		}

		return DD;
	}

	//*****************************************************************************
	//	Develops all stages, including adults
	//	kills by attrition, old age, frost and overwintering
	//	Input:	date: date of execution
	//			T: is the temperature for one time step
	//			nbStep: is the number of time steps per day (24h/step duration(h) )
	//			tree: the tree on which the insect is, for defoliation
	//*****************************************************************************
	void CBagworm::Develop(CTRef date, double T, short nbStep)
	{
		_ASSERTE(m_status == HEALTHY);

		size_t s = GetStage();

		//Relative developement rate: transform DD into age
		double DD = GetDevRate(s, T) / nbStep;
		double RR = DD / m_relativeDevRate[s];

		
		//Adjust age
		m_age += RR;

		


		_ASSERTE(RR >= 0);
	}

	//*****************************************************************************
	// GetStat gather information of this object
	//
	// Input: stat: the statistic object
	// Output: The stat is modified
	//*****************************************************************************
	void CBagworm::GetStat(CTRef d, CModelStat& stat)
	{
		ASSERT(stat.size() == NB_BAGWORM_STAT);
		short stage = GetStage();

		if (IsAlive())
		{
			stat[stage] += m_scaleFactor;
			stat[STAT_NB_BROOD] += m_broods*m_scaleFactor;
			//stat[STAT_BROOD_WEIGTH] += m_brood;

			if (d == m_creationDate)
			{
				stat[STAT_NB_CLUSTER]++;
				stat[STAT_CLUSTER_WEIGHT] += m_clusterWeight;
			}
		}
		else
		{
			if (stage == DEAD_ADULT)
				stat[stage] += m_scaleFactor;

			if (m_death == FROZEN)
				stat[STAT_DEAD_FROZEN] += m_scaleFactor;

			if (m_death == OTHERS)
				stat[STAT_DEAD_OTHERS] += m_scaleFactor;
		}
	}

}