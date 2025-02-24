﻿//*****************************************************************************
// Class: CSpruceBudworm
//          
//
// Description: the CSpruceBudworm represents a group of SBW insect. scale by m_ScaleFactor
//*****************************************************************************
// 12-07-2019	Rémi Saint-Amant	Add sex as enum and not size_t
// 01-02-2019	Rémi Saint-Amant	Bug correction when sunset is before noon 
// 19-12-2018	Rémi Saint-Amant	Add option of adult attrition.
// 03-08-2018	Rémi Saint-Amant	Remove the reduction factor for defoliation. 
// 13/03/2017   Jacques Régnière    Reduced OVERHEATING_FACTOR to 0.04 from 0.11
// 13/03/2017   Jacques Régnière    Reduced EXODUS_AGE to {0.15, 0}  from { 0.5, 0}
// 08/01/2017	Rémi Saint-Amant	Add hourly live
// 22/12/2016   Rémi Saint-Amant	Change flight activity by exodus flight
// 10/05/2016	Rémi Saint-Amant	Elimination of th optimization under -10 
// 05/03/2015	Rémi Saint-Amant	Update for BioSIM11
// 27/06/2013	Rémi Saint-Amant	New framework, Bug correction in fix AI
// 27/09/2011	Rémi Saint-Amant	Add precipitation in live
// 13/06/2010	Rémi Saint-Amant	inherit from CIndividual
// 23/03/2010   Rémi Saint-Amant    Creation from old code
//*****************************************************************************

#include "SpruceBudwormEquations.h"
#include "SpruceBudworm.h"

namespace WBSF
{


	using namespace std;
	using namespace WBSF::HOURLY_DATA;
	using namespace SBW;


	//*********************************************************************************
	//CSpruceBudworm class

	const double CSpruceBudworm::WHITE_SPRUCE_FACTOR[NB_STAGES] =
	{ 1.0, 1.0, 1.0, 1.12, 1.15, 1.1, 1.1, 1.1, 1.0, 1.0 };

	const double CSpruceBudworm::SURVIVAL_RATE[NB_STAGES] =
	{
		1.0, 1.0, 1.0, .79, .73, .62, 0.4, .66, 1.0, .39
	};

	static const double OVIPOSITING_STAGE_AGE = 0.1; //change by RSA 16-05-2018, was 0.05
	static const double MINIMUM_AGE_LIFTOFF[2] = { 0.15, 0 };
	
	const double CSpruceBudworm::POTENTIAL_FECUNDITY = 200;
	//const bool CSpruceBudworm::ALWAYSE_APPLY_ADULT_ATTRITION = true;

	//*****************************************************************************
	// Object creator
	//
	// Input: See CIndividual creator
	//
	// Note: m_relativeDevRate member is init ewith random values.
	//*****************************************************************************
	CSpruceBudworm::CSpruceBudworm(CHost* pHost, CTRef creationDate, double age, TSex sex, bool bFertil, size_t generation, double scaleFactor) :
		CIndividual(pHost, creationDate, age, sex, bFertil, generation, scaleFactor)
	{
		//Individual's "relative" development rate for each life stage
		//These are independent in successive life stages
		for (size_t s = 0; s < NB_STAGES; s++)
			m_relativeDevRate[s] = Equations().RelativeDevRate(s);

		//Compute defoliation at shoot level from defoliation at stand level
		m_D = Equations().get_defoliation(GetStand()->m_defoliation);

		m_Fº = CBioSIMModelBase::VMISS;
		m_Fᴰ = CBioSIMModelBase::VMISS;
		m_F = CBioSIMModelBase::VMISS;
		m_A = Equations().get_A(m_sex);					//Generale forewing area [cm²]
		
		if (m_sex == FEMALE)
		{
			m_Fº = Round(Equations().get_Fº(m_A));		//generate fecundity without defoliation
			m_Fᴰ = Round((1.0 - 0.0054*m_D)*m_Fº, 0);	//compute fecundity with defoliation
			m_F = m_Fᴰ;	//set current fecundity
		} 

		m_ξ = Equations().get_ξ(m_sex, m_A);			//generater weight term error
		m_M = Equations().get_M(m_sex, m_A, GetG())*m_ξ;//compute weight
		
		m_p_exodus = Equations().get_p_exodus();		//generate exodus liftoff position
		m_bExodus = false;
		m_bAlreadyExodus = false;


		// Each individual created gets the following attributes
		// Initial energy Level, the same for everyone
		static const double ALPHA0 = 2.1571;
		m_OWEnergy = ALPHA0;
		m_bMissingEnergyAlreadyApplied = false;
		m_bKillByAttrition = false;

		ASSERT(m_sex==MALE || m_Fº >= 0);
		ASSERT(m_sex == MALE || m_Fᴰ >= 0);
		ASSERT(m_M > 0);
	}



	CSpruceBudworm& CSpruceBudworm::operator=(const CSpruceBudworm& in)
	{
		if (&in != this)
		{
			CIndividual::operator=(in);
			m_OWEnergy = in.m_OWEnergy;
			m_bMissingEnergyAlreadyApplied = in.m_bMissingEnergyAlreadyApplied;
			m_bKillByAttrition = in.m_bMissingEnergyAlreadyApplied;


			m_overwinteringDate = in.m_overwinteringDate;
			m_emergingL2oDate = in.m_emergingL2oDate;
			m_emergingPupaeDate = in.m_emergingPupaeDate;

			m_eatenFoliage = in.m_eatenFoliage;
			m_bExodus = in.m_bExodus;
			m_bAlreadyExodus = in.m_bAlreadyExodus;


			//regenerate relative development rate
			for (size_t s = 0; s < NB_STAGES; s++)
				m_relativeDevRate[s] = Equations().RelativeDevRate(s);

			m_bKillByAttrition = in.m_bKillByAttrition;
			m_A = in.m_A;
			m_M = in.m_M;
			m_p_exodus = in.m_p_exodus;
			m_Fº = in.m_Fº;
			m_Fᴰ = in.m_Fᴰ;
			m_F = in.m_F; 
			m_bExodus = in.m_bExodus;
			m_bAlreadyExodus = in.m_bAlreadyExodus;
			m_D = in.m_D;


			// Each individual created gets the following attributes
			// Initial energy Level, the same for everyone
			//static const double ALPHA0 = 2.1571;
			//m_OWEnergy = ALPHA0;
			//m_bMissingEnergyAlreadyApplied = false;
			//m_bKillByAttrition = false;
		}

		return *this;
	}
	// Object destructor
	CSpruceBudworm::~CSpruceBudworm(void)
	{}


	void CSpruceBudworm::ResetRelativeDevRate()
	{
		for (int i = 0; i < NB_STAGES; i++)
			m_relativeDevRate[i] = 1;
	}

	//*****************************************************************************
	// Develops all stages for one time step
	// Input:	weather: weather of the hour
	//			timeStep: timeStep [h]
	//*****************************************************************************
	void CSpruceBudworm::Live(const CHourlyData& weather, size_t timeStep)
	{
		assert(IsAlive());
		assert(m_status == HEALTHY);

		CSBWTree* pTree = GetTree();
		CSBWStand* pStand = GetStand();

		static const double OVERHEAT_FACTOR = 0.04; //was 0.11, but on comparison with earlier (published) model, was reduced JR 2017/03/13
		COverheat overheat(OVERHEAT_FACTOR);

		size_t h = weather.GetTRef().GetHour();
		size_t s = GetStage();
		double T = weather[H_TAIR];
		if (NeedOverheating())
		{
			double OH1 = overheat.GetOverheat(((const CWeatherDay&)*weather.GetParent()), h, 16);
			T += OH1;


			{
				//const CWeatherDay& day = *((const CWeatherDay*)weather.GetParent());
				//const CWeatherDay& dp = day.GetPrevious();
				//const CWeatherDay& dn = day.GetNext();
				//
				//double Tmin[3] = { dp[H_TMIN][MEAN], day[H_TMIN][MEAN], dn[H_TMIN][MEAN] };
				//double Tmax[3] = { dp[H_TMAX][MEAN], day[H_TMAX][MEAN], dn[H_TMAX][MEAN] };
				//
				//double Fo = 0.5*(1 + cos((double(16) - h) / 12.0*PI));
				//
				//double TRange = max(0.0, (h <= 4) ? dp[H_TMAX][MEAN] - day[H_TMIN][MEAN] : (h <= 16) ? day[H_TMAX][MEAN] - day[H_TMIN][MEAN] : day[H_TMAX][MEAN] - dn[H_TMIN][MEAN]);
				//double maxOverheat = TRange * OVERHEAT_FACTOR;
				//double OH2 = maxOverheat * Fo;
				//T += OH2;

				
			}
		}


		//Time step development rate
		double r = Equations().GetRate(s, m_sex, T) / (24.0 / timeStep);
		//Relative development rate
		double RR = GetRelativeDevRate(weather[H_TAIR], r);

		//development rate for white spruce is accelerated by a factor
		if (pTree->m_kind == CSBWTree::WHITE_SPRUCE)
			RR *= WHITE_SPRUCE_FACTOR[s];


		//If we became L2o this year, then we stop
		//development until the next year  (diapause)
		if ((s == L2o && m_overwinteringDate.GetYear() == weather.GetTRef().GetYear()))
			RR = 0;

		//this line avoid to develop L2 of the generation 1
		if (GetStand()->m_bStopL22 && s == L2 && m_generation == 1)
			RR = 0;

		//If we became a new L2o, then we note the date(year)
		if (s == L1 && IsChangingStage(RR))
			m_overwinteringDate = weather.GetTRef();

		//Emerging from overwintering
		if (s == L2o && IsChangingStage(RR))
			m_emergingL2oDate = weather.GetTRef();

		//Emerging from pupae
		if (s == PUPAE && IsChangingStage(RR))
			m_emergingPupaeDate = weather.GetTRef().as(CTM::DAILY);

		//Adjust age
		m_age += RR;

		//adjust overwintering energy
		if (s == L2o)
			m_OWEnergy -= GetEnergyLost(weather[H_TAIR]) / (24.0 / timeStep);

		//verify adult longevity
		if (GetStage() == ADULT && GetStand()->m_adult_longivity_max > NO_MAX_ADULT_LONGEVITY)
		{
			ASSERT(m_emergingPupaeDate.IsInit());
			if (weather.GetTRef().as(CTM::DAILY) - m_emergingPupaeDate > GetStand()->m_adult_longivity_max)
			{
				//adult reach maximum longevity. Kill it.
				m_age = DEAD_ADULT;
			}
		}
		//Compute defoliation on tree
		m_eatenFoliage += GetEatenFoliage(RR);

		if (IsDeadByAttrition(RR))
			m_bKillByAttrition = true;


		m_age = min(m_age, double(DEAD_ADULT));
	}




	//*****************************************************************************
	// Develops all stages, including adults
	// Input:	weather: the weather iof the day
	//*****************************************************************************
	void CSpruceBudworm::Live(const CWeatherDay& weather)
	{
		//For optimization, nothing happens when temperature is under -10
		if (weather[H_TMIN][MEAN] < -10)
			return;

		size_t nbSteps = GetTimeStep().NbSteps();
		for (size_t step = 0; step < nbSteps&&m_age < DEAD_ADULT; step++)
		{
			size_t h = step * GetTimeStep();
			Live(weather[h], GetTimeStep());
		}
		//Live(weather[0], 24);
		m_bExodus = false;

		//flight activity, only in live adults 
		if (GetStage() == ADULT && !m_bAlreadyExodus)//
			m_bExodus = ComputeExodus(weather);

		if (m_bExodus)
			m_bAlreadyExodus = true;
	}


	void CSpruceBudworm::Brood(const CWeatherDay& weather)
	{
		assert(IsAlive() && m_sex == FEMALE);


		if (GetStage() == ADULT && 
			GetStageAge() > OVIPOSITING_STAGE_AGE && 
			weather[H_TNTX][MEAN] >= 10 &&
			m_F > 0)
		{
			assert(m_Fᴰ >= 0);
			assert(m_totalBroods >= 0 && m_totalBroods <= m_Fᴰ);
			ASSERT(Round(m_F + m_totalBroods, 0) == m_Fᴰ);

			//brooding
			double P = Equations().get_P(weather[H_TNTX][MEAN]);
			ASSERT(P >= 0 && P < 1);

			double broods = m_F * P;
			ASSERT(broods < m_F);
			ASSERT((m_totalBroods + broods) <= m_Fᴰ);

			//after regniere 1983 Equation [12] at x = 0 :  (29.8 *(1 - exp(-0.214))) = 5.74
			if (m_F - broods < 5.74)//avoid very small egg deposition
				broods = m_F;

			//Don't apply survival here. Survival must be apply in brooding
			m_broods = broods;
			m_totalBroods += broods;

			m_F = m_Fᴰ - m_totalBroods;
			ASSERT(m_totalBroods <= m_Fᴰ);

			//Oviposition module after Régniere 1983
			if (m_bFertil && m_broods > 0)
			{
				CSBWTree* pTree = GetTree();
				CSBWStand* pStand = GetStand(); ASSERT(pStand);

				double scaleFactor = m_broods * m_scaleFactor;
				CIndividualPtr object = make_shared<CSpruceBudworm>(GetHost(), weather.GetTRef(), EGG, RANDOM_SEX, pStand->m_bFertilEgg, m_generation + 1, scaleFactor);
				pTree->push_front(object);
			}


			//compute weight from forewing area and female gravidity
			m_M = Equations().get_M(m_sex, m_A, GetG())*m_ξ;
		}
	}

	// kills by attrition, old age, frost and overwintering
	// Output:  Individual's state is updated to follow update
	void CSpruceBudworm::Die(const CWeatherDay& weather)
	{
		//attrition mortality. Killed at the end of time step 
		if (GetStage() == DEAD_ADULT)
		{
			//Old age
			m_status = DEAD;
			m_death = OLD_AGE;
		}
		else if (m_bKillByAttrition)
		{
			//kill by attrition
			m_status = DEAD;
			m_death = ATTRITION;
		}
		else if (m_overwinteringDate.IsInit() && m_emergingL2oDate.as(CTM::DAILY) == weather.GetTRef())
		{
			//second generation L2o emerging:compute mortality
			if (IsDeadByMissingEnergy())
			{
				m_status = DEAD;
				m_death = MISSING_ENERGY;
			}
		}
		else if (GetStage() == PUPAE && weather[H_TMIN][MEAN] < 0)
		{
			//all pupae are killed by frost under 0ºC
			m_status = DEAD;
			m_death = FROZEN;
		}
		else if (GetStage() != L2o && weather[H_TMIN][MEAN] < -9)
		{
			//all non l2o are kill by frost under -10ºC
			m_status = DEAD;
			m_death = FROZEN;
		}
		

	}

	//*****************************************************************************
	// GetStat gather information of this object
	//
	// Input: stat: the statistic object
	// Output: The stat is modified
	//*****************************************************************************
	void CSpruceBudworm::GetStat(CTRef d, CModelStat& stat)
	{
		size_t stage = GetStage();

		if (m_generation == 0)
		{

			stat[S_BROOD] += m_broods * m_scaleFactor;

			if (IsAlive() || stage == DEAD_ADULT)
			{
				if (stage >= L2o && stage <= DEAD_ADULT)
					stat[S_L2o + stage - L2o] += m_scaleFactor;

				if (stage == ADULT && m_sex == FEMALE && GetStageAge() > OVIPOSITING_STAGE_AGE)
					stat[S_OVIPOSITING_ADULT] += m_scaleFactor;

				static const double SEX_RATIO[2] = { 1.0, 1.0 };//humm????

				if (m_bExodus)
					stat[S_MALE_FLIGHT + m_sex] += m_scaleFactor * SEX_RATIO[m_sex];

				if (stage == PUPAE)
					stat[S_PUPA_MALE + m_sex] += m_scaleFactor;
				if (stage == ADULT)
					stat[S_ADULT_MALE + m_sex] += m_scaleFactor;
				if (stage == ADULT && HasChangedStage())
					stat[S_MALE_EMERGENCE + m_sex] += m_scaleFactor;
			}
		}
		else if (m_generation == 1)
		{
			if (IsAlive())
			{
				if (stage >= EGG && stage <= L2)
					stat[S_EGG + stage - EGG] += m_scaleFactor;
			}
		}

	}

	//Get relative dev rate as function of stage and rate
	//NOTE: for L2o, special computation is done
	double CSpruceBudworm::GetRelativeDevRate(double T, double r)const
	{
		size_t s = GetStage();
		double RR = m_relativeDevRate[s] * r;
		if (s == L2o && r > 0)
		{
			//Equation [5] in Régniere 1990
			//Relative dev rate of L2o depend of the age of L2o
			//Adjust Relative dev rate
			double dprime = min(1.0, max(0.25, m_age - L2o));
			double tairp = max(T, 5.0);
			double fat = .091*tairp*pow(dprime, (1.0 - 1.0 / (.091*tairp)));
			RR *= fat;
		}

		return RR;
	}

	bool CSpruceBudworm::ComputeExodus(const CWeatherDay& wº)
	{
		bool bExodus = false;

		if (GetStageAge() >= MINIMUM_AGE_LIFTOFF[m_sex])
		{
			__int64 tº = 0;
			__int64 tᶜ = 0;
			__int64 tᴹ = 0;
			if (get_t(wº, tº, tᶜ, tᴹ))
			{
				//now compute tau, p and flight
				static const __int64 Δt = 60;
				for (__int64 t = tº; t <= tᴹ && !bExodus; t += Δt)
				{
					double tau = double(t - tᶜ) / (tᴹ - tᶜ);
					double h = t / 3600.0;

					const CWeatherDay& w¹ = wº.GetNext();
					const CWeatherDay& w = h < 24 ? wº : w¹;

					double T = get_Tair(w, h < 24 ? h : h - 24.0);
					double P = get_Prcp(w, h < 24 ? h : h - 24.0);
					double W = get_WndS(w, h < 24 ? h : h - 24.0);

					bExodus = ComputeExodus(T, P, W, tau);
				}
			}
		}

		return bExodus;
	}



	bool CSpruceBudworm::ComputeExodus(double T, double P, double W, double tau)
	{
		static const double C = 1.0 - 2.0 / 3.0 + 1.0 / 5.0;

		static const double K = 167.5;//proportionality constant [Hz·cm²/√g]
		static const double a = 23.0;//°C
		static const double b = 8.6957;//°C
		static const double Vmax = 72.5;//Hz
		static const double Δv = 1;


		bool bExodus = false;

		if (P == -999)
			P = 0;

		if (W == -999)
			W = 10;

		if (T > 0 && P < 2.5 && W > 2.5)//No lift-off if hourly precipitation greater than 2.5 mm
		{
			double p = (C + tau - (2 * pow(tau, 3) / 3) + (pow(tau, 5) / 5)) / (2 * C);
			ASSERT(p >= 0 && p <= 1);

			//Compute wing-beat
			double Vᴸ = K * sqrt(m_M) / m_A;//compute liftoff wing-beat to fly with actual weight (Vᴸ)
			double Vᵀ = Vmax / (1 + exp(-(T - a) / b));//compute potential wing-beat for the current temperature (Vᵀ)

			//potential wing-beat is greater than liftoff wing-beat, then exodus 
			if (Vᵀ > Vᴸ && p > m_p_exodus)
				bExodus = true;		//this insect is exodus
		}

		return bExodus;
	}


	double CSpruceBudworm::get_Tair(const CWeatherDay& weather, double h)
	{
		ASSERT(h >= 0 && h < 24);

		size_t hº = size_t(h);
		size_t h¹ = hº + 1;
		ASSERT(h >= hº && h <= h¹);
		ASSERT((h - hº) >= 0 && (h - hº) <= 1);
		ASSERT((h¹ - h) >= 0 && (h¹ - h) <= 1);

		//temperature interpolation between 2 hours
		const CHourlyData& wº = weather[hº];
		const CHourlyData& w¹ = wº.GetNext();
		double Tair = (h - hº)*w¹[H_TAIR] + (h¹ - h)*wº[H_TAIR];

		ASSERT(!WEATHER::IsMissing(Tair));
		return Tair;
	}

	double CSpruceBudworm::get_Prcp(const CWeatherDay& weather, double h)
	{
		ASSERT(h >= 0 && h < 24);

		double prcp = -999;

		size_t hº = size_t(h);
		if (!WEATHER::IsMissing(weather[hº][H_PRCP]))
			prcp = weather[hº][H_PRCP];

		return prcp;
	}

	double CSpruceBudworm::get_WndS(const CWeatherDay& weather, double h)
	{
		ASSERT(h >= 0 && h < 24);

		double wind = -999;

		size_t hº = size_t(h);
		size_t h¹ = hº + 1;
		ASSERT(h >= hº && h <= h¹);
		ASSERT((h - hº) >= 0 && (h - hº) <= 1);
		ASSERT((h¹ - h) >= 0 && (h¹ - h) <= 1);

		if (!WEATHER::IsMissing(weather[hº][H_WNDS]) && !WEATHER::IsMissing(weather[hº][H_WNDS]))
		{
			//temperature interpolation between 2 hours
			const CHourlyData& wº = weather[hº];
			const CHourlyData& w¹ = wº.GetNext();
			wind = (h - hº)*w¹[H_WNDS] + (h¹ - h)*wº[H_WNDS];
		}

		return wind;
	}


	//tᶳ [in]: sunset [s] (since the begginning of the day)
	//tº [out]: start of liftoff [s] (since the begginning of the day)
	//tᴹ [out]: end of liftoff [s] (since the begginning of the day)
	//Base on: Modeling the circadian rhythm of migratory flight in spruce budworm
	//Jacques Régnière, Matthew Garcia and Rémi St-Amant
	bool CSpruceBudworm::get_t(const CWeatherDay& wº, __int64 &tº, __int64 &tᶜ, __int64 &tᴹ)
	{
		bool bRep = false;
		
		CSun sun(wº.GetLocation().m_lat, wº.GetLocation().m_lon, wº.GetLocation().GetTimeZone());
		__int64 tᶳ = sun.GetSunset(wº.GetTRef())*3600;
		if (tᶳ > 12*3600)//if sunset is after noon (avoid problem in north)
		{
			static const __int64 H19h30 = 18.5;//s at 18:30 normal time (=19:30 Daylight Saving Time)

			//sunset hour shifted by t
			//temperature interpolation between 2 hours
			double T19h30 = get_Tair(wº, H19h30);
			if (T19h30 > 15)
			{
				static const double p1 = -3.8;//h
				static const double p2 = 0.145;//h/°C
				static const double p3 = -1.267;//h
				static const double p4 = -0.397;
				static const double p5 = -2.465;
				static const double Kf = 1.35;//°C

				double Δs = p1 + p2 * T19h30;//h
				double Δo = p3 + p4 * Δs;//h
				double Δf = Kf * p5 * Δo;//h

				//now calculate the real tº, tᶬ 
				tᶜ = tᶳ + (Δs * 3600);
				tº = tᶜ + (Δo * 3600);
				tᴹ = tº + (Δf * 3600);
				ASSERT(tᶜ >= tº && tᶜ <= tᴹ);
				ASSERT(tᶜ >= 0 && tᶜ < 24 * 3600);//tᶳ is sunset [s] since the beginning of the day

				bRep = true;
			}
		}

		return bRep;
	}




	//Get the eaten foliage 
	double CSpruceBudworm::GetEatenFoliage(double RR)const
	{
		static const double AVERAGE_WEIGHT[NB_STAGES] = { 0, 0, 0, 0.06, 0.24, 0.96, 3.80, 15.02, 0, 0 };
		static const double FEDINS[NB_STAGES][2] =
		{
			{ 0.0000, 0.0000 },
			{ 0.0000, 0.0000 },
			{ 0.0000, 0.0000 },
			{ 1.7100, 1.5300 },
			{ 6.8500, 6.1100 },
			{ 11.020, 17.220 },
			{ 27.680, 31.840 },
			{ 150.42, 271.83 },
			{ 0.0000, 0.0000 },
			{ 0.0000, 0.0000 }
		};

		double eatenFoliage = 0;
		size_t s = GetStage();
		if (s >= L2 && s <= L6)
		{
			//Compute relative weight for a specific stage
			double relativWeight = pow(10.0, (-2.436 + 0.597*(m_age - 0.5))) / AVERAGE_WEIGHT[s];
			//Compute eaten foliage: a function of relative weight
			eatenFoliage = FEDINS[s][m_sex] * RR*relativWeight;
		}

		return eatenFoliage;
	}

	//*****************************************************************************
	// IsDeadByAttrition is for one time step development
	//
	// Input: double RR: development (0..1) for this stage
	//
	// Output: TRUE if the insect dies, FALSE otherwise
	//*****************************************************************************
	bool CSpruceBudworm::IsDeadByAttrition(double RR)const
	{
		bool bDeath = false;

		bool bAdultAttrition = GetStage() == ADULT && GetStand()->m_bApplyAdultAttrition;
		if (GetStand()->m_bApplyAttrition || bAdultAttrition)
		{
			if (SURVIVAL_RATE[GetStage()] < 1)
			{
				//Computes attrition (probability of survival in a given time step, based on development rate)
				double probabSurvival = pow(SURVIVAL_RATE[GetStage()], RR);
				if (RandomGenerator().Randu() > probabSurvival)
					bDeath = true;
			}
		}

		return bDeath;
	}

	double CSpruceBudworm::GetEnergyLost(double T)
	{
		static const double ALPHA1 = 8.6623e-11;
		static const double ALPHA2 = 6.5241;

		double e = 0;
		if (T >= 0)
			e = ALPHA1 * pow(T, ALPHA2);

		return e;
	}

	bool CSpruceBudworm::IsDeadByMissingEnergy()
	{
		ASSERT(m_overwinteringDate.IsInit());

		//L2o Survival module
		bool bDead = false;

		//Don't apply MissingEnergie twice when fix is used
		if (!m_bMissingEnergyAlreadyApplied)
		{
			double ex = exp(m_OWEnergy);
			double pSurr = ex / (1 + ex);
			if (RandomGenerator().Randu() > pSurr)
				bDead = true;

			m_bMissingEnergyAlreadyApplied = true;
			if (!bDead)
			{
				int g;
				g = 0;
			}
		}

		return bDead;
	}


	void CSpruceBudworm::Pack(const CIndividualPtr& pBug)
	{
		assert(m_sex == pBug->GetSex());

		CSpruceBudworm* in = (CSpruceBudworm*)(pBug.get());
		m_OWEnergy = (m_OWEnergy*m_scaleFactor + in->m_OWEnergy*in->m_scaleFactor) / (m_scaleFactor + in->m_scaleFactor);
		m_eatenFoliage = (m_eatenFoliage*m_scaleFactor + in->m_eatenFoliage*in->m_scaleFactor) / (m_scaleFactor + in->m_scaleFactor);

		m_D = (m_D*m_scaleFactor + in->m_D*in->m_scaleFactor) / (m_scaleFactor + in->m_scaleFactor);

		CIndividual::Pack(pBug);
	}

	double CSpruceBudworm::GetInstar(bool includeLast)const
	{
		return (IsAlive() || m_death == OLD_AGE) ? std::min(GetStage() <= SBW::L2o ? GetStage() : std::max(size_t(SBW::L2o), GetStage() - 1), size_t(SBW::NB_STAGES) - (includeLast ? 0 : 1)) : CBioSIMModelBase::VMISS;
	}

	//*********************************************************************************************************************

	//*********************************************************************************
	//CSBWTree

	CSBWTree::CSBWTree(CStand* pStand) : CHost(pStand)
	{
		m_bAutumnCleaned = false;
	}


	void CSBWTree::Live(const CWeatherDay& weather)
	{
		if (weather[H_TMIN][MEAN] < -10)
		{
			if (weather.GetTRef().GetJDay() > 180 && !m_bAutumnCleaned)
			{
				m_bAutumnCleaned = true;

				for (iterator it = begin(); it != end(); it++)
					(*it)->Die(weather);//let the insect to die
			}

			return;
		}

		CHost::Live(weather);

		//	uncomment to get defoliation
		//	m_hu += GetHU(T);
		//	UpdateDefoliation();

	}

	void CSBWTree::GetStat(CTRef d, CModelStat& stat, size_t generation)
	{
		CHost::GetStat(d, stat, generation);

		stat[S_AVERAGE_INSTAR] = GetAI(true);
	}


	bool CSBWTree::GetHatchDate(CTRef& hatchDate, double& d)const
	{
		CStatistic stat;


		for (const_iterator it = begin(); it != end(); it++)
		{
			ASSERT((*it)->GetGeneration() == 1);

			CTRef ref = (*it)->GetCreationDate();
			stat += ref.GetRef();
		}

		if (stat[NB_VALUE] > 0)
		{
			hatchDate.SetRef(int(Round(stat[MEAN])), CTM(CTRef::DAILY));
			d = stat[VARIANCE];
		}


		return stat[NB_VALUE] > 0;
	}
	

	//remove all bugs of generation 0 and all non L2o
	void CSBWTree::CleanUp()
	{
		for (iterator it = begin(); it != end();)
		{
			if ((*it)->GetGeneration() == 0 || (*it)->GetStage() != L2o)
				it = erase(it);
			else
				it++;
		}

		m_bAutumnCleaned = false;
	}



}