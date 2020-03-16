//*****************************************************************************
// Class: CActiaInterrupta
//          
//
// Description: Biology of Actia interrupta
//*****************************************************************************
#include "ActiaInterruptaEquations.h"
#include "ActiaInterrupta.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::ActiaInterrupta;

namespace WBSF
{

	//*********************************************************************************
	//CActiaInterrupta class



	//*********************************************************************************
	// Object creator
	//
	// Input: See CIndividual creator
	//
	// Note: m_relativeDevRate member is initialized with random values.
	//*****************************************************************************
	CActiaInterrupta::CActiaInterrupta(CHost* pHost, CTRef creationDate, double age, WBSF::TSex sex, bool bFertil, size_t generation, double scaleFactor) :
		CIndividual(pHost, creationDate, age, sex, bFertil, generation, scaleFactor)
	{

		m_OBLPostDiapause = 0; //actual state of overwintering post diapause host
		m_OBLPostDiapause_δ = Equations().Getδ(EQ_OBL_POST_DIAPAUSE);//Individual's relative overwintering post diapause host

		//host is actually unknowns, will be set later
		for (size_t s = 0; s < NB_STAGES; s++)
		{
			//will be int later when host type will be known
			m_δ[s] = 0;// Equations().Getδ(s, GetHost()->m_hostType);
			//Stage-specific survival random draws
			m_luck[s] = Equations().GetLuck(s);
		}

		//oviposition
		//Random values of Pmax and Eº
		m_Pmax = Equations().GetPmax();
		double Eº = Equations().GetEº();
		//Initial values
		m_Pᵗ = Eº;
		m_Eᵗ = Eº;

		//Individuals are created as non-diapause individuals
		m_badluck = false;
		m_Nh = 0;
	}



	CActiaInterrupta& CActiaInterrupta::operator=(const CActiaInterrupta& in)
	{
		if (&in != this)
		{
			CIndividual::operator=(in);

			m_OBLPostDiapause = in.m_OBLPostDiapause;
			m_OBLPostDiapause_δ = in.m_OBLPostDiapause_δ;

			m_δ = in.m_δ;
			m_Pmax = in.m_Pmax;
			m_Pᵗ = in.m_Pᵗ;
			m_Eᵗ = in.m_Eᵗ;
			m_diapauseTRef = in.m_diapauseTRef;

			m_luck = in.m_luck;
			m_badluck = in.m_badluck;
		}

		return *this;
	}

	// Object destructor
	CActiaInterrupta::~CActiaInterrupta(void)
	{}


	//*****************************************************************************
	// Develops all stages, including adults
	// Input:	weather: the weather of the day
	//*****************************************************************************
	void CActiaInterrupta::Live(const CWeatherDay& weather)
	{
		assert(IsAlive());
		assert(m_status == HEALTHY);

		//assert(get_property("HostType") == "0" || get_property("HostType") == "1");
		//size_t hostType = stoi(get_property("HostType"));

		size_t first_generation = m_generation == 0 ? 0 : 1;
		if (m_δ[0] == 0)//not init yet
		{
			for (size_t s = 0; s < NB_STAGES; s++)
				m_δ[s] = Equations().Getδ(s, first_generation);
		}


		CIndividual::Live(weather);
		double dayLength = weather.GetDayLength() / 3600.; //in hours
		CTRef TRef = weather.GetTRef();
		size_t JDay = TRef.GetJDay();
		size_t nbSteps = GetTimeStep().NbSteps();


		//if (m_creationDate.GetJDay() == 0 && m_OBLPostDiapause < 1)//for insect create the first of January
		//{
		//	//wait for end of host(OBL) post-diapause
		//	for (size_t step = 0; step < nbSteps; step++)
		//	{
		//		size_t h = step * GetTimeStep();
		//		double T = weather[h][H_TAIR];

		//		//Relative development rate for time step
		//		double r = m_OBLPostDiapause_δ * Equations().GetRate(EQ_OBL_POST_DIAPAUSE, T) / nbSteps;
		//		m_OBLPostDiapause += r;
		//	}
		//}
		//else
		{

			//WBSF::ofStream file;
			//if (JDay == 0)
			//{
			//	file.open("g:/Actia.csv");
			//	file.imbue(std::locale(std::locale::classic(), new std::codecvt_utf8<size_t>()));
			//	file << u8"Year,Month,Day,Hour,Pmax,broods,Total,Oᵗ,Rᵗ,Nh,Na,Pᵗ,Eᵗ" << endl;
			//	file.close();
			//}

			//if (GetStand()->m_bAutoComputeDiapause && TRef.GetJDay() == 0)
				//m_bDiapause = false;

			for (size_t step = 0; step < nbSteps&&m_age < DEAD_ADULT; step++)
			{
				size_t h = step * GetTimeStep();
				size_t s = GetStage();
				double T = weather[h][H_TAIR];

				//Relative development rate for time step

				double r = m_δ[s] * Equations().GetRate(s, first_generation, T) / nbSteps;

				if (s == ADULT) //Set maximum longevity to 100 days
					r = max(1.0 / (100.0*nbSteps), r);

				if (GetStand()->m_bApplyAttrition)
				{
					if (IsChangingStage(r))
						m_badluck = RandomGenerator().Randu() > m_luck[s];
					else
						m_badluck = IsDeadByAttrition(s, T);
				}


				//Adjust age
				if (weather.GetTRef().GetYear() != m_diapauseTRef.GetYear())
					m_age += r;


				if (!m_adultDate.IsInit() && m_age >= ADULT)
					m_adultDate = TRef;

				//compute brooding
				static const double pre_ovip_age = GetStand()->m_preOvip;
				

				if (s == ADULT && m_sex == FEMALE && GetStageAge() >= pre_ovip_age)//TRef >= m_adultDate + GetStand()->m_preOvip
				{
					ASSERT(m_age >= ADULT);
					double fec = m_Pmax * r / (1.0 - pre_ovip_age);

					//This is Holling's disk equation, with parameters as=0.05 and th=0.8, values guessed at 
					//in Régnière et al Tranosema model (submitted)
					double as = 0.05;
					double th = 0.8;
					double Na = as * m_Nh / (1 + as * th*m_Nh) / nbSteps; //Number of attacks per time step

					//eggs laid with successful attack is, at most, host find
					double broods = max(0.0, min(fec, Na));

					m_broods += broods;
					ASSERT(m_totalBroods + m_broods < m_Pmax);
				}
			}//for all time steps


			//file.close();
			m_age = min(m_age, (double)DEAD_ADULT);
		}
	}


	void CActiaInterrupta::Brood(const CWeatherDay& weather)
	{
		ASSERT(IsAlive() && m_sex == FEMALE);
		ASSERT(m_totalBroods <= m_Pmax + 1);

		m_totalBroods += m_broods;

		//Oviposition module after Régniere 1983
		if (m_bFertil && m_broods > 0)
		{
			ASSERT(m_age >= ADULT);

			double attRate = GetStand()->m_generationAttrition;
			double scaleFactor = m_broods * m_scaleFactor*attRate;
			CIndividualPtr object = make_shared<CActiaInterrupta>(m_pHost, weather.GetTRef(), MAGGOT, FEMALE, true, m_generation + 1, scaleFactor);
			m_pHost->push_front(object);
		}
	}

	// kills by attrition, old age and end of season
	// Output:  Individual's state is updated to follow update
	void CActiaInterrupta::Die(const CWeatherDay& weather)
	{
		//ASSERT(!m_diapauseTRef.IsInit() || fabs(m_age - GetStand()->m_diapauseAge)<0.0001);

		//attrition mortality. Killed at the end of time step 
		if (GetStage() == DEAD_ADULT)
		{
			//Old age
			m_status = DEAD;
			m_death = OLD_AGE;
		}
		else if (m_badluck)
		{
			//kill by attrition
			m_status = DEAD;
			m_death = ATTRITION;
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
	void CActiaInterrupta::GetStat(CTRef d, CModelStat& stat)
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
				if (s >= MAGGOT && s < DEAD_ADULT)
					stat[S_EGG + s] += m_scaleFactor;


				if (s == ADULT)
				{
					if (m_sex == FEMALE && d >= m_adultDate + 5)
					{
						stat[S_OVIPOSITING_ADULT] += m_scaleFactor;
					}
				}

				if (m_diapauseTRef.IsInit())
					stat[S_DIAPAUSE] += m_scaleFactor;

				//because attrition is affected when the object change stage,
				//we need to take only insect alive
				if (GetStage() != GetLastStage())
				{
					stat[M_EGG + s] += m_scaleFactor;
				}

				if (s == ADULT && m_sex == FEMALE && d == m_adultDate + 5)
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
	bool CActiaInterrupta::IsDeadByAttrition(size_t s, double T)
	{
		bool bDeath = false;


		//Computes attrition (probability of survival in a given time step, based on daily rate)
		double survival = pow(Equations().GetSurvivalRate(s, T), 1.0 / GetTimeStep().NbSteps());
		if (RandomGenerator().Randu() > survival)
			bDeath = true;

		return bDeath;
	}

	bool CActiaInterrupta::CanPack(const CIndividualPtr& in)const
	{
		CActiaInterrupta* pIn = static_cast<CActiaInterrupta*>(in.get());
		return CIndividual::CanPack(in) && (GetStage() != ADULT || GetSex() != FEMALE) && pIn->m_diapauseTRef.IsInit() == m_diapauseTRef.IsInit();
	}

	void CActiaInterrupta::Pack(const CIndividualPtr& pBug)
	{
		CActiaInterrupta* in = (CActiaInterrupta*)pBug.get();

		//m_Pmax = (m_Pmax*m_scaleFactor + in->m_Pmax*in->m_scaleFactor) / (m_scaleFactor + in->m_scaleFactor);
	//	m_Pᵗ = (m_Pᵗ*m_scaleFactor + in->m_Pᵗ*in->m_scaleFactor) / (m_scaleFactor + in->m_scaleFactor);
	//	m_Eᵗ = (m_Eᵗ*m_scaleFactor + in->m_Eᵗ*in->m_scaleFactor) / (m_scaleFactor + in->m_scaleFactor);

		CIndividual::Pack(pBug);
	}



	//*********************************************************************************************************************



	CActiaInterruptaHost::CActiaInterruptaHost(WBSF::CStand* pStand/*, size_t hostType*/) :WBSF::CHost(pStand)
	{
		//	m_hostType = hostType;
	}



	//std::string CActiaInterruptaHost::get_property(const std::string& name)
	//{
	//	std::string prop;
	//	//if (name == "HostType")
	//		//prop = to_string(m_hostType);
	//	
	//	return prop;
	//}

	//*********************************************************************************************************************

}