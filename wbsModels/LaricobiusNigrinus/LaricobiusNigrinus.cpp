//*****************************************************************************
// Class: CLaricobiusNigrinus
//          
//
// Description: the CLaricobiusNigrinus represents a group of LNF insect. scale by m_ScaleFactor
//*****************************************************************************
// 05/03/2019   Rémi Saint-Amant    Creation
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
	CLaricobiusNigrinus::CLaricobiusNigrinus(CHost* pHost, CTRef creationDate, double age, size_t sex, bool bFertil, size_t generation, double scaleFactor) :
		CIndividual(pHost, creationDate, age, sex, bFertil, generation, scaleFactor)
	{
		//Individual's "relative" development rate for each life stage
		//These are independent in successive life stages
		for (size_t s = 0; s < NB_STAGES; s++)
			m_RDR[s] = Equations().GetRelativeDevRate(s);

		//m_AL = Equations().GetAdultLongevity();
		m_AL = 172;//pour calibration seulement
	}



	CLaricobiusNigrinus& CLaricobiusNigrinus::operator=(const CLaricobiusNigrinus& in)
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
	CLaricobiusNigrinus::~CLaricobiusNigrinus(void)
	{}


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
		if (GetStage() == PREPUPAE || GetStage() == PUPAE)
		{
			T = min(pStand->m_maxTsoil, T);
		}
		/*if (NeedOverheating())
		{
			static const double OVERHEAT_FACTOR = 0.1;
			COverheat overheat(OVERHEAT_FACTOR);
			T += overheat.GetOverheat(((const CWeatherDay&)*weather.GetParent()), h, 16);
		}*/
		
		if (GetStage() == ADULT)
		{
			double r = timeStep / (24.0*m_AL);
			m_age = min(double(DEAD_ADULT), m_age + r );
		}
		else
		{
			//Time step development rate
			double r = Equations().GetRate(s, T) / (24.0 / timeStep);

			//Relative development rate for this individual
			double rr = m_RDR[s];

			//Time step development rate for this individual
			r *= rr;

			//Adjust age
			m_age += r;
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
		//For optimization, nothing happens when temperature is under 0
		if (weather[H_TMIN][MEAN] < 0)
			return;

		size_t nbSteps = GetTimeStep().NbSteps();
		for (size_t step = 0; step < nbSteps&&m_age < DEAD_ADULT; step++)
		{
			size_t h = step * GetTimeStep();
			Live(weather[h], GetTimeStep());
		}
		
	}


	void CLaricobiusNigrinus::Brood(const CWeatherDay& weather)
	{
		assert(IsAlive() && m_sex == FEMALE);


		if (GetStage() == ADULT )
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
		//else if (GetStage() != EGG && weather[H_TMIN][MEAN] < -12)
		//{
		//	//all non EGG are kill by frost under -10ºC
		//	//m_status = DEAD;
		//	//m_death = FROZEN;
		//}
		

	}

	//*****************************************************************************
	// GetStat gather information of this object
	//
	// Input: stat: the statistic object
	// Output: The stat is modified
	//*****************************************************************************
	void CLaricobiusNigrinus::GetStat(CTRef d, CModelStat& stat)
	{
		size_t stage = GetStage();

//		stat[S_BROOD] += m_broods * m_scaleFactor;

		if (IsCreated(d) && IsAlive() || stage == DEAD_ADULT)
		{
			stat[S_EGG + stage] += m_scaleFactor;
//			if(stage>=L1 && stage<=L4)
				//stat[S_LARVAE] += m_scaleFactor;
			
	//		if (stage == ADULT && m_sex == FEMALE && HasChangedStage())
		//		stat[S_EMERGING_FEMALE] += m_scaleFactor;
		}
	}

	//Get relative dev rate as function of stage and rate
	//NOTE: for L2o, special computation is done
	//double CLaricobiusNigrinus::GetRelativeDevRate(double T, double r)const
	//{
	//	size_t s = GetStage();
	//	double RR = m_relativeDevRate[s] * r;
	//	if (s == L2o && r > 0)
	//	{
	//		//Equation [5] in Régniere 1990
	//		//Relative dev rate of L2o depend of the age of L2o
	//		//Adjust Relative dev rate
	//		double dprime = min(1.0, max(0.25, m_age - L2o));
	//		double tairp = max(T, 5.0);
	//		double fat = .091*tairp*pow(dprime, (1.0 - 1.0 / (.091*tairp)));
	//		RR *= fat;
	//	}

	//	return RR;
	//}

	

	//*****************************************************************************
	// IsDeadByAttrition is for one time step development
	//
	// Input: double RR: development (0..1) for this stage
	//
	// Output: TRUE if the insect dies, FALSE otherwise
	//*****************************************************************************
	//bool CLaricobiusNigrinus::IsDeadByAttrition(double RR)const
	//{
	//	bool bDeath = false;

	//	bool bAdultAttrition = GetStage() == ADULT && GetStand()->m_bApplyAdultAttrition;
	//	if (GetStand()->m_bApplyAttrition || bAdultAttrition)
	//	{
	//		if (SURVIVAL_RATE[GetStage()] < 1)
	//		{
	//			//Computes attrition (probability of survival in a given time step, based on development rate)
	//			double probabSurvival = pow(SURVIVAL_RATE[GetStage()], RR);
	//			if (RandomGenerator().Randu() > probabSurvival)
	//				bDeath = true;
	//		}
	//	}

	//	return bDeath;
	//}


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
//		m_DD(CDegreeDays::SINGLE_TRIANGLE, 6.8),
	//	m_sumDD(0)
	{
	}


	void CLNFHost::Live(const CWeatherDay& weather)
	{
		CHost::Live(weather);
	//	m_sumDD += m_DD.GetDD(weather);
	}

	void CLNFHost::GetStat(CTRef d, CModelStat& stat, size_t generation)
	{
		CHost::GetStat(d, stat, generation);

		stat[S_AVERAGE_INSTAR] = GetAI(true);
		//stat[S_DD68] = m_sumDD;
		
	}


	

}