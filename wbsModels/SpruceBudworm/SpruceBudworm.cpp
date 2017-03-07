//*****************************************************************************
// Class: CSpruceBudworm
//          
//
// Description: the CSpruceBudworm represents a group of SBW insect. scale by m_ScaleFactor
//*****************************************************************************
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

	static const double OVIPOSITING_STAGE_AGE = 0.05;
	const double CSpruceBudworm::POTENTIAL_FECONDITY = 200;

	
	//*****************************************************************************
	// Object creator
	//
	// Input: See CIndividual creator
	//
	// Note: m_relativeDevRate member is init ewith random values.
	//*****************************************************************************
	CSpruceBudworm::CSpruceBudworm(CHost* pHost, CTRef creationDate, double age, size_t sex, bool bFertil, size_t generation, double scaleFactor) :
		CIndividual(pHost, creationDate, age, sex, bFertil, generation, scaleFactor)
	{
		//Individual's "relative" development rate for each life stage
		//These are independent in successive life stages
		for (size_t s = 0; s < NB_STAGES; s++)
			m_relativeDevRate[s] = Equations().RelativeDevRate(s);

		//double ξ = RandomGenerator().Rand(-20, 20);
		m_defoliation = Equations().get_defoliation(GetStand()->m_defoliation);
			

		m_A = Equations().get_A(m_sex); 
		m_F° = (m_sex == FEMALE) ? Equations().get_F°(m_A) : 0;
		m_Fᴰ = (1.0 - 0.0054*m_defoliation)*m_F°;
		m_M = Equations().get_M(m_sex, m_A, GetG(), true);//compute weight from forewing area and female gravidity

		m_p_exodus = Equations().get_p_exodus(); 
		m_bExodus = false;
		m_bAlreadyExodus = false;
		
	
		// Each individual created gets the following attributes
		// Initial energy Level, the same for everyone
		static const double ALPHA0 = 2.1571;
		m_OWEnergy = ALPHA0;
		m_bMissingEnergyAlreadyApplied = false;
		m_bKillByAttrition = false;
		
		ASSERT(m_F°>=0);
		ASSERT(m_Fᴰ >= 0);
		ASSERT(m_M > 0);
	}



	CSpruceBudworm& CSpruceBudworm::operator=(const CSpruceBudworm& in)
	{
		if (&in != this)
		{
			CIndividual::operator=(in);
			m_OWEnergy = in.m_OWEnergy;
			m_bMissingEnergyAlreadyApplied = in.m_bMissingEnergyAlreadyApplied;

			m_overwinteringDate = in.m_overwinteringDate;
			m_emergingDate = in.m_emergingDate;

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
			m_F° = in.m_F°;
			m_Fᴰ = in.m_Fᴰ;
			m_bExodus = in.m_bExodus;
			m_bAlreadyExodus = in.m_bAlreadyExodus;
			m_defoliation = in.m_defoliation;


			// Each individual created gets the following attributes
			// Initial energy Level, the same for everyone
			static const double ALPHA0 = 2.1571;
			m_OWEnergy = ALPHA0;
			m_bMissingEnergyAlreadyApplied = false;
			m_bKillByAttrition = false;

			//m_p_mating = in.m_p_mating;
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

		static const double OVERHEAT_FACTOR = 0.11;
		COverheat overheat(OVERHEAT_FACTOR);

		size_t h = weather.GetTRef().GetHour();
		size_t s = GetStage();
		double T = weather[H_TAIR2];
		if (NeedOverheating())
			T += overheat.GetOverheat(((const CWeatherDay&)*weather.GetParent()), h);

		//Time step development rate
		double r = Equations().GetRate(s, m_sex, T) / (24.0 / timeStep);
		//Relative development rate
		double RR = GetRelativeDevRate(weather[H_TAIR2], r);
		//correction for defoliation (de 1 à .75 entre 50 et 100 %)
		if (s >= L3 && s <= L6)//PUPAE
		{
			double defFactor = max(0.75, 1.0 - max(0.0, m_defoliation - 50.0)*0.005);
			RR *= defFactor; 
		}
			

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

		//Emerging 
		if (s == L2o && IsChangingStage(RR))
			m_emergingDate = weather.GetTRef();

		//Adjust age
		m_age += RR;

		//adjust overwintering energy
		if (s == L2o)
			m_OWEnergy -= GetEnergyLost(weather[H_TAIR2]) / (24.0 / timeStep);

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
		if (weather[H_TMIN2][MEAN] < -10)
			return;

		size_t nbSteps = GetTimeStep().NbSteps();
		for (size_t step = 0; step < nbSteps&&m_age < DEAD_ADULT; step++)
		{
			size_t h = step*GetTimeStep();
			Live(weather[h], GetTimeStep());
		}

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
		
		
		if (GetStage() == ADULT && GetStageAge() > OVIPOSITING_STAGE_AGE && weather[H_TNTX][MEAN] >= 10)
		{
			assert(m_Fᴰ>=0);
			assert(m_totalBroods >= 0 && m_totalBroods <= m_Fᴰ);

			//brooding
			double eggLeft = m_Fᴰ - m_totalBroods;
			
			double P = Equations().get_P(weather[H_TNTX][MEAN]);
			double broods = eggLeft*P;

			if ( (m_totalBroods + broods) > m_Fᴰ)
				broods = m_Fᴰ - m_totalBroods;

			//Don't apply survival here. Survival must be apply in brooding
			m_broods = broods;
			m_totalBroods += broods;

			ASSERT(m_totalBroods <= m_Fᴰ);

			//Oviposition module after Régniere 1983
			if (m_bFertil && m_broods > 0)
			{
				CSBWTree* pTree = GetTree();
				CSBWStand* pStand = GetStand(); ASSERT(pStand);

				double scaleFactor = m_broods*m_scaleFactor;
				CIndividualPtr object = make_shared<CSpruceBudworm>(GetHost(), weather.GetTRef(), EGG, NOT_INIT, pStand->m_bFertilEgg, m_generation + 1, scaleFactor);
				pTree->push_front(object);
			}  

			
			m_M = Equations().get_M(m_sex, m_A, (m_Fᴰ - m_totalBroods) / m_F°, true);//compute weight from forewing area and female gravidity
			
			
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
		else if (m_overwinteringDate.IsInit() && m_emergingDate == weather.GetTRef())
		{
			//second generation L2o emerging:compute mortality
			if (IsDeadByMissingEnergy())
			{
				m_status = DEAD;
				m_death = MISSING_ENERGY;
			}
		}
		else if (GetStage() != L2o && weather[H_TMIN2][MEAN] < -9)
		{
			//all non l2o are kill by frost under -10°C
			m_status = DEAD;
			m_death = FROZEN;
		}
		/*else if (m_bRemoveExodus)
		{
			m_status = DEAD;
			m_death = EXODUS;
		}*/

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
			
			stat[S_BROOD] += m_broods*m_scaleFactor;

			if (IsAlive() || stage == DEAD_ADULT)
			{
				if (stage >= L2o && stage <= DEAD_ADULT)
					stat[S_L2o + stage - L2o] += m_scaleFactor;

				if (stage == ADULT && m_sex == FEMALE && GetStageAge() > OVIPOSITING_STAGE_AGE)
					stat[S_OVIPOSITING_ADULT] += m_scaleFactor;
				
				static const double SEX_RATIO[2] = { 1.0, 1.0 };//humm????

				if (m_bExodus) 
					stat[S_MALE_FLIGHT + m_sex] += m_scaleFactor*SEX_RATIO[m_sex];

				if (stage == PUPAE )
					stat[S_PUPA_MALE + m_sex] += m_scaleFactor;
				if (stage == ADULT)
					stat[S_ADULT_MALE + m_sex] += m_scaleFactor;
				if (stage == ADULT && IsChangingStage() )
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

	bool CSpruceBudworm::ComputeExodus(const CWeatherDay& w°)
	{
		bool bExodus = false;

		//double Pmating = GetMatingProbability(GetStageAge());
		if (m_sex==MALE || GetStageAge() > OVIPOSITING_STAGE_AGE)
		{
			__int64 t° = 0;
			__int64 tᴹ = 0;
			if (get_t(w°, t°, tᴹ))
			{
				//calculate tᶜ
				__int64 tᶜ = (t° + tᴹ) / 2;

				//now compute tau, p and flight
				static const __int64 Δt = 60;
				for (__int64 t = t°; t <= tᴹ && !bExodus; t += Δt)
				{
					double tau = double(t - tᶜ) / (tᴹ - tᶜ);
					double h = t / 3600.0;

					const CWeatherDay& w¹ = w°.GetNext();
					const CWeatherDay& w = h < 24 ? w° : w¹;

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
		
		
		//static const double K = 166.0;//all moth flies between 25 à 63 Hz
		//static const double b[2] = { 21.35, 24.08 };
		//static const double c[2] = { 2.97, 6.63 };
		//static const double VmaxMF[2] = { 1.0, 1.0 };
		//static const double VmaxHz = 65;
		

		static const double K = 166.0;//all moth flies between 25 à 63 Hz
		static const double b[2] = { 21.35, 24.08 };
		static const double c[2] = { 2.97, 6.63 };
		static const double VmaxMF[2] = { 1.0, 1.0 };
		static const double VmaxHz = 65;
		static const double deltaT[2] = { 0 , 3.5 };



		//static const double K = 166.0;//95% moths fly between 25 à 42 Hz
		//static const double b[2] = { 21.35, 21.35 };
		//static const double c[2] = { 2.97, 2.97 }; 
		//static const double VmaxMF[2] = { 1.0, 1.0 };
		//static const double VmaxHz = 65;
		//static const double deltaT[2] = { 0 };

		bool bExodus = false;

		if (P == -999)
			P = 0;
		
		if (W == -999)
			W = 10;

		
		//double Pmating = GetMatingProbability(GetStageAge());
		//m_sex == MALE || 
		static const double EXODUS_AGE[2] = { 0.5, 0 };// OVIPOSITING_STAGE_AGE
		if (GetStageAge() > EXODUS_AGE[m_sex] && T > 0 && P < 2.5 && W > 2.5)//No lift-off if hourly precipitation greater than 2.5 mm
		{
			const double Vmax = VmaxHz * VmaxMF[m_sex];
			double p = (C + tau - 2 * pow(tau, 3) / 3 + pow(tau, 5) / 5) / (2 * C);

			//Compute wingbeat
			double Vᴸ = K* sqrt(m_M) / m_A;//compute liftoff wingbeat to fly with actual weight (Vᴸ)
			double Vᵀ = Vmax*(1 - exp(-pow((T + deltaT[m_sex]) / b[m_sex], c[m_sex])));//compute potential wingbeat for the current temperature (Vᵀ)
			
			//potential wingbeat is greather than liftoff wingbeat, then exodus 
			if (Vᵀ > Vᴸ && p > m_p_exodus)
				bExodus = true;		//this insect is exodus

					
		}

		return bExodus;
	}


	double CSpruceBudworm::get_Tair(const CWeatherDay& weather, double h)const
	{
		ASSERT(h >= 0 && h < 24);

		size_t h° = size_t(h);
		size_t h¹ = h° + 1;
		ASSERT(h >= h° && h <= h¹);
		ASSERT((h - h°) >= 0 && (h - h°) <= 1);
		ASSERT((h¹ - h) >= 0 && (h¹ - h) <= 1);

		//temperature interpolation between 2 hours
		const CHourlyData& w° = weather[h°];
		const CHourlyData& w¹ = w°.GetNext();
		double Tair = (h - h°)*w¹[H_TAIR2] + (h¹ - h)*w°[H_TAIR2];

		ASSERT(!WEATHER::IsMissing(Tair));
		return Tair;
	}

	double CSpruceBudworm::get_Prcp(const CWeatherDay& weather, double h)const
	{
		ASSERT(h >= 0 && h < 24);

		double prcp = -999;

		size_t h° = size_t(h);
		if (!WEATHER::IsMissing(weather[h°][H_PRCP]))
			prcp = weather[h°][H_PRCP];

		return prcp;
	}

	double CSpruceBudworm::get_WndS(const CWeatherDay& weather, double h)const
	{
		ASSERT(h >= 0 && h < 24);

		double wind = -999;
		
		size_t h° = size_t(h);
		size_t h¹ = h° + 1;
		ASSERT(h >= h° && h <= h¹);
		ASSERT((h - h°) >= 0 && (h - h°) <= 1);
		ASSERT((h¹ - h) >= 0 && (h¹ - h) <= 1);

		if (!WEATHER::IsMissing(weather[h°][H_WNDS]) && !WEATHER::IsMissing(weather[h°][H_WNDS]))
		{
			//temperature interpolation between 2 hours
			const CHourlyData& w° = weather[h°];
			const CHourlyData& w¹ = w°.GetNext();
			wind = (h - h°)*w¹[H_WNDS] + (h¹ - h)*w°[H_WNDS];
		}

		return wind;
	}



	

	bool CSpruceBudworm::get_t(const CWeatherDay& w°, __int64 &t°, __int64 &tᴹ)const
	{
		static const __int64 Δtᶠ = 3 * 3600;
		static const __int64 Δtᶳ = -3600;
		static const __int64 Δt = 60;
		static const double T° = 24.5;

		CSun sun(w°.GetLocation().m_lat, w°.GetLocation().m_lon, w°.GetLocation().GetTimeZone());
		__int64 sunset = (sun.GetSunset(w°.GetTRef()) + 1.0 ) * 3600;//+1 hour : assume to be in daylight zone  //[s]

		//first estimate of exodus info
		__int64 h4 = 4 * 3600;
		__int64 Δtᵀ = h4;
		t° = sunset - h4;//subtract 4 hours
		tᴹ = sunset + h4;//add 4 hours


		if (t° > 0)
		{
			for (__int64 t = t°; t <= tᴹ && Δtᵀ == h4; t += Δt)
			{
				//sunset hour shifted by t
				double h = t / 3600.0;
				const CWeatherDay& w = h < 24 ? w° : w°.GetNext();
			
				//temperature interpolation between 2 hours
				double Tair = get_Tair(w, h < 24 ? h : h - 24.0);
				if (Tair <= T°)
					Δtᵀ = t - sunset;
			}

			if (Δtᵀ < h4)//if the Δtᵀ is greater than 4, no temperature under T°, then no exodus. probably rare situation
			{
				//now calculate the real t°, tᶬ and tᶜ
				t° = sunset + max((Δtᶳ - Δtᶠ / 2), Δtᵀ); 
				tᴹ = min(sunset + h4, t° + Δtᶠ);
			}
		}


		return Δtᵀ < h4;
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

		if ((GetStand()->m_bApplyAttrition && SURVIVAL_RATE[GetStage()] < 1))
		{
			//Computes attrition (probability of survival in a given time step, based on development rate)
			double probabSurvival = pow(SURVIVAL_RATE[GetStage()], RR);
			if (RandomGenerator().Randu() > probabSurvival)
				bDeath = true;
		}

		return bDeath;
	}

	double CSpruceBudworm::GetEnergyLost(double T)
	{
		static const double ALPHA1 = 8.6623e-11;
		static const double ALPHA2 = 6.5241;

		double e = 0;
		if (T >= 0)
			e = ALPHA1*pow(T, ALPHA2);

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
		}

		return bDead;
	}


	void CSpruceBudworm::Pack(const CIndividualPtr& pBug)
	{
		assert(m_sex == pBug->GetSex());

		CSpruceBudworm* in = (CSpruceBudworm*)(pBug.get());
		m_OWEnergy = (m_OWEnergy*m_scaleFactor + in->m_OWEnergy*in->m_scaleFactor) / (m_scaleFactor + in->m_scaleFactor);
		m_eatenFoliage = (m_eatenFoliage*m_scaleFactor + in->m_eatenFoliage*in->m_scaleFactor) / (m_scaleFactor + in->m_scaleFactor);
		
		m_defoliation = (m_defoliation*m_scaleFactor + in->m_defoliation*in->m_scaleFactor) / (m_scaleFactor + in->m_scaleFactor);
		 
		//m_liftoff_hour = (m_liftoff_hour*m_scaleFactor + in->m_liftoff_hour*in->m_scaleFactor) / (m_scaleFactor + in->m_scaleFactor);
		//m_flightActivity = (m_flightActivity*m_scaleFactor + in->m_flightActivity*in->m_scaleFactor) / (m_scaleFactor + in->m_scaleFactor);

		CIndividual::Pack(pBug);
	}

	double CSpruceBudworm::GetInstar(bool includeLast)const
	{
		return (IsAlive() || m_death == OLD_AGE) ? std::min(GetStage() <= SBW::L2o ? GetStage() : std::max(size_t(SBW::L2o), GetStage() - 1), size_t(SBW::NB_STAGES) - (includeLast ? 0 : 1)) : WBSF::CBioSIMModelBase::VMISS;
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
		if (weather[H_TMIN2][MEAN] < -10)
		{
			if (weather.GetTRef().GetJDay()>180 && !m_bAutumnCleaned)
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

		//CStatistic sum;
		//CStatistic weight;
		//size_t nbStages = (*begin())->GetNbStages();
		//for (const_iterator it = begin(); it != end(); it++)
		//{
		//	if ((*it)->GetStage() == ADULT && (*it)->GetSex() == FEMALE &&
		//		(*it)->IsAlive() )//|| (*it)->GetStage()==DEAD_ADULT)
		//	{
		//		sum += (*it)->GetStageAge()*(*it)->GetScaleFactor();
		//		weight += (*it)->GetScaleFactor();
		//	}
		//}
		
		//stat[S_FEMALE_AGE] = (weight.IsInit() && weight[SUM] > 0) ? sum[SUM] / weight[SUM] : CBioSIMModelBase::VMISS;;
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
	//
	//CTRef CSBWTree::GetFirstHatchDate()const
	//{
	//	_ASSERT( size()>0 );
	//	return size()>0?at(0)->GetCreationDate():CTRef();
	//}

	//double CSBWTree::GetHU(CDailyWaveVector& T)
	//{
	//	static const double TZERO=2.78;
	//	double hu = 0;
	//
	//	for(int h=0; h<(int)T.size(); h++)
	//        hu += Max(0.0, (T[h]-TZERO))/T.size();
	//
	//	return hu;
	//}
	//
	////Update defoliation ratio of the tree
	////Update m_foliageRatio
	//void CSBWTree::UpdateDefoliation()
	//{
	//	static const double UZERO	= 4.80;
	//	static const double UMAX	= 231.30;
	//	static const double WASTE	= 0.4;
	//
	//
	//	if( m_bugsFeeding > 0 )	
	//	{
	//		double totalFoliage=0;
	//		switch(m_kind)
	//		{
	//		case BALSAM_FIR: totalFoliage=m_budDensity*(UZERO+UMAX*(1-exp(-pow((m_hu/689.7),2.651))));break;
	//		case WHITE_SPRUCE: totalFoliage=m_budDensity*(UZERO+UMAX*(1-exp(-pow((m_hu/589.7), 3.051)))); break;
	//		default: _ASSERTE(false);
	//		}
	//			
	//		double availableFoliage = totalFoliage*m_foliageRatio;
	//		double currentFoliage= Max(0.0, availableFoliage-(1.0+WASTE)*m_bugsFeeding);
	//
	//		m_foliageRatio=currentFoliage/totalFoliage;
	//		
	//		m_bugsFeeding=0;
	//	}
	//}

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


	//void CSpruceBudworm::Live(const CWeatherDay& weather)
	//{
	//	assert(IsAlive());
	//	assert(m_status == HEALTHY);

	//	//For optimization, nothing happens when temperature is under -10
	//	if (weather[H_TMIN2][MEAN] < -10)
	//		return;


	//	CIndividual::Live(weather);

	//	CSBWTree* pTree = GetTree();
	//	CSBWStand* pStand = GetStand();

	//	static const double OVERHEAT_FACTOR = 0.11;
	//	COverheat overheat(OVERHEAT_FACTOR);

	//	size_t nbSteps = GetTimeStep().NbSteps();
	//	for (size_t step = 0; step < nbSteps&&m_age < DEAD_ADULT; step++)
	//	{
	//		size_t h = step*GetTimeStep();
	//		size_t s = GetStage();
	//		double T = weather[h][H_TAIR2];
	//		if (NeedOverheating())
	//			T += overheat.GetOverheat(weather, h);

	//		//Time step development rate
	//		double r = Equations().GetRate(s, m_sex, T) / nbSteps;
	//		//Relative development rate
	//		double RR = GetRelativeDevRate(weather[h][H_TAIR2], r);

	//		//development rate for white spruce is accelerated by a factor
	//		if (pTree->m_kind == CSBWTree::WHITE_SPRUCE)
	//			RR *= WHITE_SPRUCE_FACTOR[s];


	//		//If we became L2o this year, then we stop
	//		//development until the next year  (diapause)
	//		if ((s == L2o && m_overwinteringDate.GetYear() == weather.GetTRef().GetYear()))
	//			RR = 0;

	//		//this line avoid to develop L2 of the generation 1
	//		if (GetStand()->m_bStopL22 && s == L2 && m_generation == 1)
	//			RR = 0;

	//		//If we became a new L2o, then we note the date(year)
	//		if (s == L1 && IsChangingStage(RR))
	//			m_overwinteringDate = weather.GetTRef();

	//		//Emerging 
	//		if (s == L2o && IsChangingStage(RR))
	//			m_emergingDate = weather.GetTRef();

	//		//Adjust age
	//		m_age += RR;

	//		//adjust overwintering energy
	//		if (s == L2o)
	//			m_OWEnergy -= GetEnergyLost(weather[h][H_TAIR2]) / nbSteps;

	//		//Compute defoliation on tree
	//		m_eatenFoliage += GetEatenFoliage(RR);

	//		if (IsDeadByAttrition(RR))
	//			m_bKillByAttrition = true;
	//	}

	//	//flight activity, only in live adults 
	//	if (GetStage() == ADULT)
	//	{
	//		m_flightActivity = GetFlightActivity(weather);
	//	}

	//	m_age = min(m_age, double(DEAD_ADULT));
	//}

	//double prcp = -1;
	//double sumF = 0;
	//double k0 = 10.;
	//double k1 = -8.25;
	//double twoPi = 2 * 3.14159 / 24.;
	//double fourPi = 4 * 3.14159 / 24.;

	//size_t nbSteps = GetTimeStep().NbSteps();
	//for (size_t step = 0; step < nbSteps; step++)
	//{
	//	size_t h = step*GetTimeStep();

	//	//effect of time of day
	//	//double time = nbSteps / 2. + 24 * h / nbSteps;


	//	//TRES TRES ETRANGE....
	//	double time = (double)h + GetTimeStep() / 2.0;
	//	double F = .373 - 0.339*cos(twoPi*(time + k1)) - 0.183*sin(twoPi*(time + k1)) + 0.157*cos(fourPi*(time + k1)) + 0.184*sin(fourPi*(time + k1)); //Simmons and Chen (1975)

	//	//effect of temperature. The amplitude of sumF is independent of size of time step.
	//	//Equation [4] in Regniere unpublished (from CJ Sanders buzzing data)
	//	if (prcp >= 0)
	//		F = F*0.91*pow(max(0.0, (31. - weather[h][H_TAIR2])), 0.3)*exp(-pow(max(0.0, (31. - weather[h][H_TAIR2]) / 9.52), 1.3));

	//	sumF += F / nbSteps;
	//}

	//double f_ppt = max(0.0, 1.0 - pow(prcp / k0, 2));
	//return sumF*f_ppt;

	//if (!m_bAlreadyFlow)


	//__int64 h4 = 4;


	//double CSpruceBudworm::GetFlightActivity(const CWeatherDay& weather)
	//{
	//	static const double Δtᶠ = 3;
	//	static const double Δtᶳ = -0.5;//j'ai mis 0.5 ici car j'ai l'impression que mon algo retourne une demi-heure plot tôt : à vérifier
	//	static const double C = 1.0 - 2.0 / 3.0 + 1.0 / 5.0;
	//	static const double K = 166;
	//	static const double b[2] = { 21.35, 24.08 };
	//	static const double c[2] = { 2.97, 6.63 };
	//	static const double T° = 24.5;
	//	static const double Δt = 0.25;
	//	static const size_t hᶬ = 23;//hᶬ is only a practical limit to avoid looking at the next day

	//	const double Vmax = 65 * (m_sex == MALE ? 1 : 1.2);


	//	double flight = 0;

	//	if (m_p_exodus <= 1)
	//	{
	//		CSun sun(weather.GetLocation().m_lat, weather.GetLocation().m_lon);
	//		double sunset = sun.GetSunset(weather.GetTRef());

	//		//first estimate of t° and tᶬ to find Δtᵀ
	//		double t° = -4;//subtract 4 hours
	//		double tᶬ = 4;//add 4 hours
	//		double Δtᵀ = 4;

	//		for (double t = t°; t < tᶬ && Δtᵀ == 4; t += Δt)
	//		{
	//			//sunset hour shifted by t
	//			double h = sunset + t;
	//			size_t h° = size_t(h);
	//			size_t h¹ = h° + 1;


	//			//temperature interpolation between 2 hours
	//			double T = (h - h°)*weather[min(hᶬ, h°)][H_TAIR2] + (h¹ - h)*weather[min(hᶬ, h¹)][H_TAIR2];
	//			if (T <= T°)
	//				Δtᵀ = t;
	//		}


	//		if (Δtᵀ < 4)//if the Δtᵀ is greater than 4, no temperature under T°, then no exodus. probably rare situation
	//		{
	//			//now calculate the real t°, tᶬ and tᶜ
	//			double t° = max(Δtᶳ - 0.5*Δtᶠ, double(Δtᵀ));
	//			double tᶬ = min(4.0, t° + Δtᶠ);
	//			double tᶜ = (t° + tᶬ) / 2;

	//			//
	//			double M° = Equations().get_M(m_A, 1);//initial weight of mean gravid female
	//			double Mᴬ = Equations().get_M(m_A, 1 - m_totalBroods / POTENTIAL_FECONDITY);//actual weight of mean actual female
	//			double RM = Mᴬ / M°; //ratio of actual vs initial weight female
	//			double M = m_M*RM;	//actual weight is initial weight x ratio
	//			double Vᴸ = K* sqrt(M) / m_A;//compute Vᴸ with actual weight

	//			//now compute tau, p and flight
	//			for (double t = t°; t < tᶬ && flight == 0; t += Δt)
	//			{
	//				double tau = (t - tᶜ) / (tᶬ - tᶜ);
	//				double p = (C + tau - 2 * pow(tau, 3) / 3 + pow(tau, 5) / 5) / (2 * C);
	//				if (m_sex == MALE)
	//					p *= 0.3 / 0.7;//sex ratio equilibrium

	//				double h = sunset + t;
	//				size_t h° = size_t(h);
	//				size_t h¹ = h° + 1;


	//				//temperature interpolation between 2 hours
	//				double T = (h - h°)*weather[min(hᶬ, h°)][H_TAIR2] + (h¹ - h)*weather[min(hᶬ, h¹)][H_TAIR2];
	//				if (T > 0)
	//				{
	//					double Vᵀ = Vmax*(1 - exp(-pow(T / b[m_sex], c[m_sex])));
	//					if (Vᵀ > Vᴸ && p > m_p_exodus)
	//					{
	//						flight = 1;		//this insect is exodus
	//						m_p_exodus = 10;//change exodus to ignore this insect for exodus
	//					}
	//				}
	//			}
	//		}
	//	}

	//	return flight;
	//}

}