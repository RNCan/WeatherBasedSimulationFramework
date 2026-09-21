//*****************************************************************************
// Class: CGypsyMothEggHatch
//          
//
// Description: the CGypsyMothEggHatch represents a group of LDD insect. scale by m_ScaleFactor
//*****************************************************************************
// 2026-09-08   Rémi Saint-Amant    Creation
//*****************************************************************************

#include <boost/math/distributions/logistic.hpp>

#include "GypsyMothEggHatchEquations.h"
#include "GypsyMothEggHatch.h"
#include "ModelBase/DevRateEquation.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::LDD;


namespace WBSF
{

	//*********************************************************************************
	//CGypsyMothEggHatch class


	//*****************************************************************************
	// Object creator
	//
	// Input: See CIndividual creator
	//
	// Note: m_RDR (relative Development Rate)  member is init with random values.
	//*****************************************************************************
	CGypsyMothEggHatch::CGypsyMothEggHatch(CHost* pHost, CTRef creationDate, double age, TSex sex, bool bFertil, size_t generation, double scaleFactor) :
		CIndividual(pHost, creationDate, age, sex, bFertil, generation, scaleFactor)
	{
#if RSA_MODEL==1
		m_end_of_diapause = GetStand()->m_EOD_mu;
		m_RDR[DIAPAUSE_EGG] = CDevRateEquation::GetRelativeDevlopmentRate(Equations().RG(), GetStand()->m_EOD_sigma);
		m_RDR[EGG] = CDevRateEquation::GetRelativeDevlopmentRate(Equations().RG(), Equations().m_RDR[EGG]);
		m_RDR[LARVAE] = 0;
#else
		//reset creation date
		for (size_t s = 0; s < NB_STAGES; s++)
			m_RDR[s] = Equations().GetRelativeDevlopmentRate(s);
#endif

		m_cold_tolerence = Equations().GetColdTolerence();
	}

	CGypsyMothEggHatch& CGypsyMothEggHatch::operator=(const CGypsyMothEggHatch& in)
	{
		if (&in != this)
		{
			CIndividual::operator=(in);

			m_end_of_diapause = in.m_end_of_diapause;
			m_deadByAttrition = in.m_deadByAttrition;

		}

		return *this;
	}

	//destructor
	CGypsyMothEggHatch::~CGypsyMothEggHatch(void)
	{
	}

	void CGypsyMothEggHatch::OnNewDay(const CWeatherDay& weather)
	{
		CIndividual::OnNewDay(weather);
	}

	//*****************************************************************************
	// Develops all stages for one time step
	// Input:	weather: weather of the hour
	//			timeStep: timeStep [h]
	//*****************************************************************************
	void CGypsyMothEggHatch::Live(const CHourlyData& weather, size_t time_step)
	{
		assert(IsAlive());
		assert(m_status == HEALTHY);

		CLDDHost* pHost = GetHost();
		CLDDStand* pStand = GetStand();

		double nb_steps = (24.0 / time_step);
		size_t h = weather.GetTRef().GetHour();
		size_t s = GetStage();

		double T = weather[H_TAIR];


		//daily development rate
		double d_r = Equations().GetDailyDevlopmentRate(s, T);
		ASSERT(d_r >= 0.0 && d_r <= 1.0);

//#if RSA_MODEL==1
		if (s == DIAPAUSE_EGG)
		{
			//d_r = 1.0 / (m_end_of_diapause - CTRef(weather.GetTRef().GetYear(), JANUARY, DAY_01));
			d_r = 1.0 / (m_end_of_diapause.GetJDay() + 1);
		} 
		//else
		//{
		//	//std::vector<double> P = { Equations().m_EDP.begin(),Equations().m_EDP.end() };
		//	d_r = max(0.0, CDevRateEquation::GetRate(CDevRateEquation::Régnière_2012, { Equations().m_EDP.begin(),Equations().m_EDP.end() }, T));
		//}

//#else
//		if (s == DIAPAUSE_EGG)
//		{
//			std::vector<double> P = { Equations().m_EOD.begin(),Equations().m_EOD.end() };
//			//double Tb = P[2];
//			//double Tm = P[3];
//			//P[2] = -Tm;
//			//P[3] = -Tb;
////			d_r = max(0.0, min(0.25,CDevRateEquation::GetRate(CDevRateEquation::Régnière_2012, P, Tb-T)));
//			d_r = max(0.0, min(0.25, CDevRateEquation::GetRate(CDevRateEquation::Régnière_2012, P, -T)));
//		}
//		else
//		{
//			std::vector<double> P = { Equations().m_EDP.begin(),Equations().m_EDP.end() };
//			d_r = max(0.0, min(1.0, CDevRateEquation::GetRate(CDevRateEquation::Régnière_2012, P, T)));
//		}
//
//#endif

		//vector<double> P = { Equations().m_EDP[0], Equations().m_EDP[2], Equations().m_EDP[3] };
		//d_r = max(0.0, min(1.0, CDevRateEquation::GetRate(CDevRateEquation::Briere1_1999, P, T)));



	//Time step development rate
		double ts_r = d_r / nb_steps;

		//Time step development rate for this individual
		double i_r = min(1.0, ts_r * m_RDR[s]);
		ASSERT(i_r >= 0.0 && i_r < 1.0);

		//Adjust age
		m_age += i_r;

		//apply attrition
		//if (GetStand()->m_bApplyAttrition)
		//{
		//	if (IsDeadByAttrition(s, T, i_r))
		//		m_deadByAttrition = weather.GetTRef().as(CTM::DAILY);
		//}
	}




	//*****************************************************************************
	// Develops all stages, including adults
	// Input:	weather: the weather of the day
	//*****************************************************************************
	void CGypsyMothEggHatch::Live(const CWeatherDay& weather)
	{
		CIndividual::Live(weather);

		ASSERT(IsCreated(weather.GetTRef()));

		if (!IsCreated(weather.GetTRef()))
			return;

		//Simulate the insect for all time steps
		size_t nbSteps = GetTimeStep().NbSteps();
		for (size_t step = 0; step < nbSteps && m_age < LARVAE; step++)
		{
			size_t h = step * GetTimeStep();
			Live(weather[h], GetTimeStep());
		}

		assert(m_age < NB_STAGES);
		m_age = min(m_age, (double)LARVAE);
		
	}



	// kills by old age and frost
	// Output:  Individual's state is updated to follow update
	void CGypsyMothEggHatch::Die(const CWeatherDay& weather)
	{
		//attrition mortality. Killed at the end of time step 
		if (m_deadByAttrition.IsInit())
		{
			m_status = DEAD;
			m_death = ATTRITION;
		}
		else
		{
			size_t s = GetStage();

			//Preliminary assessment of the cold tolerance of Laricobius nigrinus, 
			//a winter - active predator of the hemlock woolly adelgid from Western Canada
			//Leland M.Humble
			//static const double COLD_TOLERENCE_T[NB_STAGES] = { -27.5,-22.1, -99.0 };
			////Toland:L. nigrinus was -13.6 oC (± 0.5) with temperatures that ranged from -6 oC to -21 oC.
			//if (weather[H_TMIN][MEAN] < COLD_TOLERENCE_T[s])
			//{
			//	m_status = DEAD;
			//	m_death = FROZEN;
			//}
			//
			////From Crandall 2023
			//if (weather[H_TMIN][LOWEST] < m_cold_tolerence)
			//{
			//	m_status = DEAD;
			//	m_death = FROZEN;
			//}

		}
	}


	//stage: stage
	//T: temperature for this time step
	//i_r: Individual time step development rate 
	bool CGypsyMothEggHatch::IsDeadByAttrition(size_t stage, double T, double i_r)const
	{
		bool bDeath = false;

		//Get stage (overall) survival at this temperature
		//double S = Equations().GetStageSurvival(stage, T);
		//
		////Compute time step survival, limit at 1% survival to avoid annihilation
		//double i_s = pow(max(0.01, S), i_r);
		//
		////Computes attrition (probability of survival in a given time step, based on development rate)
		//if (RandomGenerator().RandUniform() > i_s)
		//	bDeath = true;

		return bDeath;
	}



	//*****************************************************************************
	// GetStat gather information of this object
	//
	// Input: stat: the statistic object
	// Output: The stat is modified
	//*****************************************************************************
	void CGypsyMothEggHatch::GetStat(CTRef d, CModelStat& stat)
	{
		if (IsCreated(d))
		{
			size_t s = GetStage();
			ASSERT(s < NB_STAGES);

			if (IsAlive())
				stat[S_DIAPAUSE_EGGS + s] += m_scaleFactor;

			if (m_status == DEAD && m_death == ATTRITION)
				stat[S_DEAD_ATTRITION] += m_scaleFactor;

			if (m_status == DEAD && m_death == FROZEN)
				stat[S_DEAD_FROST] += m_scaleFactor;

			if (HasChangedStage() && s == S_EGGS)
				stat[S_DIAPAUSE_END] += m_scaleFactor;

			if (HasChangedStage() && s == S_LARVAE)
				stat[S_EGGS_HATCH] += m_scaleFactor;
		}
	}


	void CGypsyMothEggHatch::Pack(const CIndividualPtr& pBug)
	{
		assert(m_sex == pBug->GetSex());

		CGypsyMothEggHatch* in = (CGypsyMothEggHatch*)(pBug.get());
		CIndividual::Pack(pBug);
	}

	//*********************************************************************************
	//CLDDHost

	CLDDHost::CLDDHost(CStand* pStand) :
		CHost(pStand)
	{
	}


	void CLDDHost::Live(const CWeatherDay& weather)
	{
		CHost::Live(weather);
	}

	void CLDDHost::GetStat(CTRef d, CModelStat& stat, size_t generation)
	{
		CHost::GetStat(d, stat, generation);
	}

	//*************************************************
	//CLDDStand

	void CLDDStand::init(int year, const CWeatherYears& weather)
	{
		m_equations.ComputeEndOfDiapause(weather[year], m_EOD_mu, m_EOD_sigma, m_diapause_end_NCDD);
	}




	void CLDDStand::GetStat(CTRef d, CModelStat& stat, size_t generation)
	{
		CStand::GetStat(d, stat, generation);

		const CWeatherStation& weather_station = GetModel()->m_weather;
		const CWeatherDay& wday = weather_station.GetDay(d);

		stat[S_DIAPAUSE_END_NCDD1] = m_diapause_end_NCDD[d][0];
		stat[S_DIAPAUSE_END_NCDD2] = m_diapause_end_NCDD[d][1];

	}


}