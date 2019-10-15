//*****************************************************************************
// Class: CLaricobiusNigrinus
//          
//
// Description: the CLaricobiusNigrinus represents a group of LNF insect. scale by m_ScaleFactor
//*****************************************************************************
// 15/10/2019   Rémi Saint-Amant    Creation
//*****************************************************************************

#include "LaricobiusNigrinusEquations.h"
#include "LaricobiusNigrinus.h"

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
		//Individual's "relative" development rate for each life stage
		//These are independent in successive life stages
		for (size_t s = 0; s < NB_STAGES; s++)
			m_RDR[s] = Equations().GetRelativeDevRate(s);

		m_CDD = 0;
		m_creationCDD = Equations().GetOvipositionDD();
		m_adult_longevity = Equations().GetAdultLongevity(m_sex);
		m_F = (m_sex == FEMALE)?Equations().GetFecondity(m_adult_longevity) :0;
	}



	CLaricobiusNigrinus& CLaricobiusNigrinus::operator=(const CLaricobiusNigrinus& in)
	{
		if (&in != this)
		{
			CIndividual::operator=(in);
			
			//regenerate relative development rate
			for (size_t s = 0; s < NB_STAGES; s++)
				m_RDR[s] = Equations().GetRelativeDevRate(s);

			m_CDD = in.m_CDD;
			m_creationCDD = in.m_creationCDD;
			m_adult_longevity = in.m_adult_longevity;// Equations().GetAdultLongevity(m_sex) / 2;
			m_F = in.m_F;
		}

		return *this;
	}
	// Object destructor
	CLaricobiusNigrinus::~CLaricobiusNigrinus(void)
	{}

	
	void CLaricobiusNigrinus::OnNewDay(const CWeatherDay& weather)
	{
		if (!IsCreated(weather.GetTRef()))
		{
			m_CDD += GetStand()->m_DD.GetDD(weather);
			if (m_CDD >= m_creationCDD)
			{
				m_creationDate = weather.GetTRef();
				ASSERT(IsCreated(weather.GetTRef()));
			}
		}
	}
	
	//*****************************************************************************
	// Develops all stages for one time step
	// Input:	weather: weather of the hour
	//			timeStep: timeStep [h]
	//*****************************************************************************
	void CLaricobiusNigrinus::Live(const CHourlyData& weather, size_t timeStep)
	{
		assert(IsAlive()); 
		assert(m_status == HEALTHY);

		CLNFHost* pHost = GetHost();
		CLNFStand* pStand = GetStand();
		
		size_t h = weather.GetTRef().GetHour();
		size_t s = GetStage();
		double T = weather[H_TAIR];
		double nb_steps = (24.0 / timeStep);

		if (s >= PREPUPAE && s<= AESTIVAL_DIAPAUSE_ADULT)
		{
			//take air temperature
			//estimate of soil temperature : need a real model
			//T = m_Tsoil;

			if (pStand->GetModel()->m_info.m_loc.m_name == "BlacksburgLab")
			{
				//lab rearing: change temperature
				if (s == PREPUPAE)
					T = 13;
				else if (s == PUPAE)
					T = 15;
				else if (s == AESTIVAL_DIAPAUSE_ADULT && weather.GetTRef().GetYear() == 2003)
					T = 15;
				else if (s == AESTIVAL_DIAPAUSE_ADULT && weather.GetTRef().GetYear() == 2004)
					T = 19;
			}


		}
		
		if (s < AESTIVAL_DIAPAUSE_ADULT)
		{
			//Time step development rate
			double r = Equations().GetRate(s, T) / nb_steps;

			//Relative development rate for this individual
			double rr = m_RDR[s];

			//Time step development rate for this individual
			r *= rr;
			ASSERT(r >= 0 && r < 1);

			//Adjust age
			//m_age = min(double(ACTIVE_ADULT), m_age + r);
			m_age += r;
		}
		else if(s==AESTIVAL_DIAPAUSE_ADULT)
		{

			if( !m_adultDate.IsInit())
				m_adultDate = weather.GetTRef().as(CTM::DAILY);

			double day_length = weather.GetLocation().GetDayLength(weather.GetTRef())/3600.0;//[h]
			if (pStand->GetModel()->m_info.m_loc.m_name == "BlacksburgLab")
				day_length = 12;


			double r = Equations().GetAdultAestivalDiapauseRate(T, day_length) / nb_steps;
			//Relative development rate for this individual
			double rr = m_RDR[s];

			//Time step development rate for this individual
			r *= rr;
			ASSERT(r >= 0 && r < 1);
			
			m_age += r;
		}
		else//ACTIVE_ADULT
		{
			//double a = Equations().GetAdultAbundance(weather[H_TAIR], weather.GetTRef().as(CTM::DAILY) - m_creationDate) / nb_steps;
			double r = (1.0 / m_adult_longevity) / nb_steps;
			ASSERT(r >= 0 && r < 1);

			//m_age = min( (double)DEAD_ADULT, m_age + r);
			m_age += r;
			//m_adult_abundance += a;
		}

		if (m_age >= DEAD_ADULT)
		{
			int g;
			g = 0;
		}
		//adjust overwintering energy
		//if (s == EGG)
			//m_OWEnergy -= GetEnergyLost(weather[H_TAIR]) / (24.0 / timeStep);
	}




	//*****************************************************************************
	// Develops all stages, including adults
	// Input:	weather: the weather of the day
	//*****************************************************************************
	void CLaricobiusNigrinus::Live(const CWeatherDay& weather)
	{
		ASSERT(IsCreated(weather.GetTRef()));
		//For optimization, nothing happens when temperature is under 0
		if (!IsCreated(weather.GetTRef()))
			return;

		//after Chapter 13: Defining pc/qc standards for mass - rearing HWA predators
		//Allen C.Cohen(2011)
		//double soil_depth = 4.5; //[cm] Equations().m_D[PREPUPAE][0];
		//double quatile = Equations().m_D[PREPUPAE][1];
		//double LAI = 2.5;// CLeafAreaIndex::ComputeLAI(weather.GetTRef().GetJDay(), LeafAreaIndex::WHITE_SPRUCE, quatile);
		//double LAI = 2.5;// Equations().m_D[PREPUPAE][1];
		//m_Tsoil = CSoilTemperatureModel::GetSoilTemperature(4.5, weather[H_TAIR][MEAN], m_Tsoil, 2.5, 0);

		size_t nbSteps = GetTimeStep().NbSteps();
		for (size_t step = 0; step < nbSteps&&m_age< DEAD_ADULT; step++)
		{
			size_t h = step * GetTimeStep();
			Live(weather[h], GetTimeStep());
		}
		
	}


	void CLaricobiusNigrinus::Brood(const CWeatherDay& weather)
	{
		assert(IsAlive() && m_sex == FEMALE);


		if (GetStage() == ACTIVE_ADULT)
		{
			//assert(m_F > 0);

			//brooding
			//m_broods = m_F;
			//m_totalBroods = m_F;
			//m_F = 0;
		}
	}

	// kills by attrition, old age, frost and overwintering
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
		else
		{
			size_t s = GetStage();
			
			//Preliminary assessment of the cold tolerance of Laricobius nigrinus, a winter - active predator of the hemlock woolly adelgid from western canada
			//Leland M.Humble
			static const double COLD_TOLERENCE_T[NB_STAGES] = { -27.5,-22.1, -99.0,-99.0,-19.0,-19.0 };
			
			if(weather[H_TMIN][MEAN] < COLD_TOLERENCE_T[s])
			{
				m_status = DEAD;
				m_death = FROZEN;
			}
		}
		

	}

	//*****************************************************************************
	// GetStat gather information of this object
	//
	// Input: stat: the statistic object
	// Output: The stat is modified
	//*****************************************************************************
	void CLaricobiusNigrinus::GetStat(CTRef d, CModelStat& stat)
	{
		size_t s = GetStage();
		ASSERT(s <= DEAD_ADULT);
//		stat[S_BROOD] += m_broods * m_scaleFactor;

		if (IsCreated(d) )
		{
			if( IsAlive() || (s == DEAD_ADULT))
				stat[S_EGG + s] += m_scaleFactor;

			
			if(m_status == DEAD && m_death == FROZEN)
				stat[S_DEAD_FROST] += m_scaleFactor;
			
			//if (s == ACTIVE_ADULT)
			//{
			//	stat[S_ADULT_ABUNDANCE] += m_scaleFactor * m_adult_abundance;
			//}
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
	


}