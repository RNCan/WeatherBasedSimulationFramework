//*****************************************************************************
// Class: CSpruceBudworm
//          
//
// Description: the CSpruceBudworm represents a group of SBW insect. scale by m_ScaleFactor
//*****************************************************************************
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


		// Each individual created gets the following attributes
		// Initial energy Level, the same for everyone
		static const double ALPHA0 = 2.1571;
		m_OWEnergy = ALPHA0;
		m_bMissingEnergyAlreadyApplied = false;
		m_bKillByAttrition = false;
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
			m_flightActivity = in.m_flightActivity;

			//regenerate relative development rate
			for (size_t s = 0; s < NB_STAGES; s++)
				m_relativeDevRate[s] = Equations().RelativeDevRate(s);

			m_bKillByAttrition = in.m_bKillByAttrition;
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
	// Develops all stages, including adults
	// Input:	weather: the weather iof the day
	//*****************************************************************************
	void CSpruceBudworm::Live(const CWeatherDay& weather)
	{
		assert(IsAlive());
		assert(m_status == HEALTHY);

		//For optimization, nothing happens when temperature is under -10
		if (weather[H_TMIN][MEAN] < -10)
			return;


		CIndividual::Live(weather);

		CSBWTree* pTree = GetTree();
		CSBWStand* pStand = GetStand();

		static const double OVERHEAT_FACTOR = 0.11;
		COverheat overheat(OVERHEAT_FACTOR);

		size_t nbSteps = GetTimeStep().NbSteps();
		for (size_t step = 0; step < nbSteps&&m_age < DEAD_ADULT; step++)
		{
			size_t h = step*GetTimeStep();
			size_t s = GetStage();
			double T = weather[h][H_TAIR];
			if (NeedOverheating())  
				T += overheat.GetOverheat(weather, h);

			//Time step development rate
			double r = Equations().GetRate(s, m_sex, T) / nbSteps;
			//Relative development rate
			double RR = GetRelativeDevRate(weather[h][H_TAIR], r);

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
				m_OWEnergy -= GetEnergyLost(weather[h][H_TAIR]) / nbSteps;

			//Compute defoliation on tree
			m_eatenFoliage += GetEatenFoliage(RR);

			if (IsDeadByAttrition(RR))
				m_bKillByAttrition = true;
		}

		//flight activity, only in live adults 
		if (GetStage() == ADULT)
		{
			m_flightActivity = GetFlightActivity(weather);
		}

		m_age = min(m_age, double(DEAD_ADULT));
	}


	void CSpruceBudworm::Brood(const CWeatherDay& weather)
	{
		assert(IsAlive() && m_sex == FEMALE);

		if (m_age >= ADULT + 0.0666)
		{
			//brooding
			static const double POTENTIAL_FECONDITY = 200;
			double eggLeft = POTENTIAL_FECONDITY - m_totalBroods;
			double Tmax = weather[H_TAIR][HIGHEST];
			double brood = eggLeft*max(0.0, min(0.5, (0.035*Tmax - 0.32)));
			if (m_totalBroods + brood > POTENTIAL_FECONDITY)
				brood = POTENTIAL_FECONDITY - m_totalBroods;

			//Don't apply survival here. Survival must be apply in brooding
			m_broods = brood;
			m_totalBroods += brood;

			ASSERT(m_totalBroods <= POTENTIAL_FECONDITY);

			//Oviposition module after Régniere 1983
			if (m_bFertil && m_broods > 0)
			{
				CSBWTree* pTree = GetTree();
				CSBWStand* pStand = GetStand(); ASSERT(pStand);

				double scaleFactor = m_broods*m_scaleFactor;
				CIndividualPtr object = make_shared<CSpruceBudworm>(GetHost(), weather.GetTRef(), EGG, NOT_INIT, pStand->m_bFertilEgg, m_generation + 1, scaleFactor);
				pTree->push_front(object);
			}
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
		else if (GetStage() != L2o && weather[H_TMIN][LOWEST] < -9)
		{
			//all non l2o are kill by frost under -10°C
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
		stat[S_BROOD] += m_broods*m_scaleFactor;

		if (m_generation == 0)
		{
			if (IsAlive())
			{

				if (stage >= L2o && stage < DEAD_ADULT)
					stat[S_L2o + stage - L2o] += m_scaleFactor;

				if (stage == ADULT)
				{
					if (m_sex == FEMALE)
					{
						stat[S_FEMALE_FLIGHT_ACTIVITY] += m_flightActivity*m_scaleFactor;
						stat[S_OVIPOSITING_ADULT] += m_scaleFactor;
					}
					else
					{
						stat[S_MALE_FLIGHT_ACTIVITY] += m_flightActivity*m_scaleFactor;
					}
				}
			}
			else
			{
				if (stage == DEAD_ADULT)
					stat[S_DEAD_ADULT] += m_scaleFactor;
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


	double CSpruceBudworm::GetFlightActivity(const CWeatherDay& weather)
	{
		double prcp = -1;
		double sumF = 0;
		double k0 = 10.;
		double k1 = -8.25;
		double twoPi = 2 * 3.14159 / 24.;
		double fourPi = 4 * 3.14159 / 24.;

		size_t nbSteps = GetTimeStep().NbSteps();
		for (size_t step = 0; step < nbSteps; step++)
		{
			size_t h = step*GetTimeStep();

			//effect of time of day
			//double time = nbSteps / 2. + 24 * h / nbSteps;


			//TRES TRES ETRANGE....
			double time = (double)h + GetTimeStep() / 2.0;
			double F = .373 - 0.339*cos(twoPi*(time + k1)) - 0.183*sin(twoPi*(time + k1)) + 0.157*cos(fourPi*(time + k1)) + 0.184*sin(fourPi*(time + k1)); //Simmons and Chen (1975)

			//effect of temperature. The amplitude of sumF is independent of size of time step.
			//Equation [4] in Regniere unpublished (from CJ Sanders buzzing data)
			if (prcp >= 0)
				F = F*0.91*pow(max(0.0, (31. - weather[h][H_TAIR])), 0.3)*exp(-pow(max(0.0, (31. - weather[h][H_TAIR]) / 9.52), 1.3));

			sumF += F / nbSteps;
		}

		double f_ppt = max(0.0, 1.0 - pow(prcp / k0, 2));
		return sumF*f_ppt;
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
		CSpruceBudworm* in = (CSpruceBudworm*)(pBug.get());
		m_OWEnergy = (m_OWEnergy*m_scaleFactor + in->m_OWEnergy*in->m_scaleFactor) / (m_scaleFactor + in->m_scaleFactor);
		m_eatenFoliage = (m_eatenFoliage*m_scaleFactor + in->m_eatenFoliage*in->m_scaleFactor) / (m_scaleFactor + in->m_scaleFactor);
		m_flightActivity = (m_flightActivity*m_scaleFactor + in->m_flightActivity*in->m_scaleFactor) / (m_scaleFactor + in->m_scaleFactor);

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
		if (weather[H_TMIN][LOWEST] < -10)
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
			if ((*it)->GetGeneration() == 0 || (*it)->GetStage()!=L2o)
				it = erase(it);
			else
				it++;
		}
		
		m_bAutumnCleaned = false;
	}



}