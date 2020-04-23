//*****************************************************************************
// Class: CMeteorusTrachynotus
//          
//
// Description: Biology of Meteorus trachynotus
//*****************************************************************************
#include "MeteorusTrachynotusEquations.h"
#include "MeteorusTrachynotus.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::MeteorusTrachynotus;

namespace WBSF
{

	//*********************************************************************************
	//CMeteorusTrachynotus class



	//*********************************************************************************
	// Object creator
	//
	// Input: See CIndividual creator
	//
	// Note: m_relativeDevRate member is initialized with random values.
	//*****************************************************************************
	CMeteorusTrachynotus::CMeteorusTrachynotus(CHost* pHost, CTRef creationDate, double age, WBSF::TSex sex, bool bFertil, size_t generation, double scaleFactor) :
		CIndividual(pHost, creationDate, age, sex, bFertil, generation, scaleFactor)
	{
		//host is actually unknowns, will be set later
		for (size_t s = 0; s < NB_STAGES; s++)
		{
			//will be int later when host type will be known
			m_δ[s] = Equations().Getδ(s);
		}

		//oviposition
		m_Pmax = Equations().GetPmax();

		//Individuals are created as non-diapause individuals
		m_Nh = 0;
	}



	CMeteorusTrachynotus& CMeteorusTrachynotus::operator=(const CMeteorusTrachynotus& in)
	{
		if (&in != this)
		{
			CIndividual::operator=(in);

			//m_OBLPostDiapause = in.m_OBLPostDiapause;
			//m_OBLPostDiapause_δ = in.m_OBLPostDiapause_δ;

			m_δ = in.m_δ;
			m_Pmax = in.m_Pmax;
			m_diapauseTRef = in.m_diapauseTRef;
		}

		return *this;
	}

	// Object destructor
	CMeteorusTrachynotus::~CMeteorusTrachynotus(void)
	{}


	//*****************************************************************************
	// Develops all stages, including adults
	// Input:	weather: the weather of the day
	//*****************************************************************************
	void CMeteorusTrachynotus::Live(const CWeatherDay& weather)
	{
		assert(IsAlive());
		assert(m_status == HEALTHY);
		assert(m_totalBroods + m_broods < m_Pmax);
		assert(m_broods == 0);//daily brood must be rest
		assert(m_creationDate.GetJDay() == 0 || GetHost()->m_diapause_age >= 1);
		
		CIndividual::Live(weather);

		//wait the end of host diapause before begginning Meteorus development
		if (m_creationDate.GetJDay() > 0 || GetHost()->m_diapause_age>=1)
		{
			double dayLength = weather.GetDayLength() / 3600.; //in hours
			CTRef TRef = weather.GetTRef();
			size_t JDay = TRef.GetJDay();
			size_t nbSteps = GetTimeStep().NbSteps();

			for (size_t step = 0; step < nbSteps&&m_age < DEAD_ADULT; step++)
			{
				size_t h = step * GetTimeStep();
				size_t s = GetStage();
				double T = weather[h][H_TAIR];

				//Relative development rate for time step

				double r = m_δ[s] * Equations().GetRate(s, T) / nbSteps;

				//if (s == ADULT) //Set maximum longevity to 100 days
					//r = max(1.0 / (100.0*nbSteps), r);

				//Adjust age
				if (weather.GetTRef().GetYear() != m_diapauseTRef.GetYear())
					m_age += r;


				if (!m_adultDate.IsInit() && m_age >= ADULT)
					m_adultDate = TRef;

				//compute brooding
				static const double pre_ovip_age = GetStand()->m_preOvip;


				if (s == ADULT && m_sex == FEMALE && GetStageAge() >= pre_ovip_age)
				{
					ASSERT(m_age >= ADULT);

					double Emax = 4.97;
					double wmax = 13.81 / nbSteps;
					double dtOnd20 = Equations().GetRate(ADULT, T) / Equations().GetRate(ADULT, 20.);
					double as = 0.047;
					double th = 0.01;

					double w = Emax * as*m_Nh / (1 + as * th*m_Nh) * (1 - m_broods / m_Pmax) * dtOnd20 / nbSteps; //Number of attacks per time step

					//eggs laid with successful attack is, at most, host find
					double broods = max(0.0, min(wmax, w));
					m_broods += broods;

					ASSERT(m_totalBroods + m_broods < m_Pmax);
				}
			}//for all time steps


			//file.close();
			m_age = min(m_age, (double)DEAD_ADULT);
		}
	}


	//void CMeteorusTrachynotus::Brood(const CWeatherDay& weather)
	//{
	//	ASSERT(IsAlive() && m_sex == FEMALE);
	//	ASSERT(m_totalBroods <= m_Pmax + 1);

	//	m_totalBroods += m_broods;

	//	//Oviposition module after Régniere 1983
	//	if (m_bFertil && m_broods > 0)
	//	{
	//		ASSERT(m_age >= ADULT);

	//		double attRate = GetStand()->m_generationAttrition;
	//		double scaleFactor = m_broods * m_scaleFactor*attRate;
	//		CIndividualPtr object = make_shared<CMeteorusTrachynotus>(m_pHost, weather.GetTRef(), EGG, FEMALE, true, m_generation + 1, scaleFactor);
	//		m_pHost->push_front(object);
	//	}
	//}

	// kills by attrition, old age and end of season
	// Output:  Individual's state is updated to follow update
	void CMeteorusTrachynotus::Die(const CWeatherDay& weather)
	{
		//ASSERT(!m_diapauseTRef.IsInit() || fabs(m_age - GetStand()->m_diapauseAge)<0.0001);

		//attrition mortality. Killed at the end of time step 
		if (GetStage() == DEAD_ADULT)
		{
			//Old age
			m_status = DEAD;
			m_death = OLD_AGE;
		}
		else if (m_generation > 0 && weather[H_TMIN][MEAN] < GetStand()->m_lethalTemp && !m_diapauseTRef.IsInit())
		{
			m_status = DEAD;
			m_death = FROZEN;
		}
		else if (!m_diapauseTRef.IsInit() && weather.GetTRef().GetMonth() == DECEMBER && weather.GetTRef().GetDay() == DAY_31)
		{
			//all individual not in diapause are kill at the end of the season
			m_status = DEAD;
			m_death = OTHERS;
		}
	}

	//*****************************************************************************
	// GetStat gather information of this object
	//
	// Input: stat: the statistic object
	// Output: The stat is modified
	//*****************************************************************************
	void CMeteorusTrachynotus::GetStat(CTRef d, CModelStat& stat)
	{
		if (IsCreated(d))
		{
			size_t s = GetStage();

			//assert(get_property("HostType") == "0" || get_property("HostType") == "1");
			//size_t hostType = stoi(get_property("HostType"));

			//stat[S_BROOD_OBL+hostType] += m_broods*m_scaleFactor;
			//stat[E_BROOD] += m_broods*m_scaleFactor; //E_BROOD is the same as S_BROOD




			if (s >= ADULT)//individuals that reach adult stage (alive or dead)
				stat[S_CUMUL_REATCH_ADULT] += m_scaleFactor;

			if (IsAlive())
			{
				if (s < DEAD_ADULT)
					stat[S_EGG + s] += m_scaleFactor;


				if (s == ADULT && m_sex == FEMALE && GetStageAge() >= GetStand()->m_preOvip)
				{
					stat[S_OVIPOSITING_ADULT] += m_scaleFactor;
				}

				if (m_diapauseTRef.IsInit())
					stat[S_DIAPAUSE] += m_scaleFactor;

				//because attrition is affected when the object change stage,
				//we need to take only insect alive
				if (GetStage() != GetLastStage())
				{
					stat[M_EGG + s] += m_scaleFactor;
				}

				if (s == ADULT && m_sex == FEMALE && GetStageAge() >= GetStand()->m_preOvip)
					stat[M_OVIPOSITING_ADULT] += m_scaleFactor;
			}
			else
			{
				if (m_death == OLD_AGE)
				{
					stat[S_DEAD_ADULT] += m_scaleFactor;
					if (GetStage() != GetLastStage())
						stat[M_DEAD_ADULT] += m_scaleFactor;
				}


				if (m_death == ATTRITION)
					stat[S_ATTRITION] += m_scaleFactor;

				if (m_lastStatus == HEALTHY && m_status == DEAD && m_death == ATTRITION)
					stat[M_ATTRITION] += m_scaleFactor;

				if (m_lastStatus == HEALTHY && m_status == DEAD && m_death == FROZEN)
					stat[M_FROZEN] += m_scaleFactor;

				if (m_lastStatus == HEALTHY && m_status == DEAD && m_death == OTHERS)
					stat[M_OTHERS] += m_scaleFactor;
			}

			//if (m_lastAge < GetStand()->m_diapauseAge && m_age >= GetStand()->m_diapauseAge)
			if (d == m_diapauseTRef)
			{
				stat[M_DIAPAUSE] += m_scaleFactor;
				stat[M_DIAPAUSE_AGE] += m_scaleFactor * m_age;
			}
		}
	}

	//*****************************************************************************
	// IsDeadByAttrition is for one time step development
	// Output: TRUE if the insect dies, FALSE otherwise
	//*****************************************************************************
	//bool CMeteorusTrachynotus::IsDeadByAttrition(size_t s, double T)
	//{
	//	bool bDeath = false;


	//	//Computes attrition (probability of survival in a given time step, based on daily rate)
	//	double survival = pow(Equations().GetSurvivalRate(s, T), 1.0 / GetTimeStep().NbSteps());
	//	if (RandomGenerator().Randu() > survival)
	//		bDeath = true;

	//	return bDeath;
	//}

	bool CMeteorusTrachynotus::CanPack(const CIndividualPtr& in)const
	{
		CMeteorusTrachynotus* pIn = static_cast<CMeteorusTrachynotus*>(in.get());
		return CIndividual::CanPack(in) && (GetStage() != ADULT || GetSex() != FEMALE) && pIn->m_diapauseTRef.IsInit() == m_diapauseTRef.IsInit();
	}

	void CMeteorusTrachynotus::Pack(const CIndividualPtr& pBug)
	{
		CMeteorusTrachynotus* in = (CMeteorusTrachynotus*)pBug.get();

		//m_Pmax = (m_Pmax*m_scaleFactor + in->m_Pmax*in->m_scaleFactor) / (m_scaleFactor + in->m_scaleFactor);
	//	m_Pᵗ = (m_Pᵗ*m_scaleFactor + in->m_Pᵗ*in->m_scaleFactor) / (m_scaleFactor + in->m_scaleFactor);
	//	m_Eᵗ = (m_Eᵗ*m_scaleFactor + in->m_Eᵗ*in->m_scaleFactor) / (m_scaleFactor + in->m_scaleFactor);

		CIndividual::Pack(pBug);
	}



	//*********************************************************************************************************************



	CMeteorusTrachynotusHost::CMeteorusTrachynotusHost(WBSF::CStand* pStand/*, size_t hostType*/) :WBSF::CHost(pStand)
	{
		m_diapause_age = 0; //actual state of overwintering post diapause host
		m_δ = OBL_Equations().Getδ(OBL_POST_DIAPAUSE);//Individual's relative overwintering post diapause host
	}

	void CMeteorusTrachynotusHost::Live(const CWeatherDay& weather)
	{
		if (m_diapause_age < 1)
		{
			CTRef TRef = weather.GetTRef();
			size_t JDay = TRef.GetJDay();
			size_t nbSteps = GetTimeStep().NbSteps();

			for (size_t step = 0; step < nbSteps; step++)
			{

				size_t h = step * GetTimeStep();
				double T = weather[h][H_TAIR];

				//Relative development rate for time step
				double r = OBL_Equations().GetRate(OBL_POST_DIAPAUSE, T) / nbSteps;
				ASSERT(r >= 0);

				m_diapause_age += r * m_δ;
			}//for all time steps
		}

		CHost::Live(weather);
	}

	//std::string CMeteorusTrachynotusHost::get_property(const std::string& name)
	//{
	//	std::string prop;
	//	//if (name == "HostType")
	//		//prop = to_string(m_hostType);
	//	
	//	return prop;
	//}

	//*********************************************************************************************************************

}