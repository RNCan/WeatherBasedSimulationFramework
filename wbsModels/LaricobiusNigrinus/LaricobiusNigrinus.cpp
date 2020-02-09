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
#include <boost/math/distributions/weibull.hpp>
//#include <boost/math/distributions/beta.hpp>
//#include <boost/math/distributions/Rayleigh.hpp>
#include <boost/math/distributions/logistic.hpp>
//#include <boost/math/distributions/exponential.hpp>

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
	CLaricobiusNigrinus::CLaricobiusNigrinus(CHost* pHost, CTRef cd, double age, TSex sex, bool bFertil, size_t generation, double scaleFactor) :
		CIndividual(pHost, cd, age, sex, bFertil, generation, scaleFactor)
	{
		//Individual's "relative" development rate for each life stage
		//These are independent in successive life stages
		for (size_t s = 0; s < NB_STAGES; s++)
			m_RDR[s] = Equations().GetRelativeDevRate(s);

		//reset creation date
		//m_creationDate.clear();
		//m_creationCDD = Equations().GetCreationCDD();
		//m_ii = 0;
		//m_CDD = 0;

		m_adultEmergingCDD = Equations().GetAdultEmergingCDD();

		m_parentAdultEmergence = GetParentAdultEmergence();
		m_creationDate = GetCreationDate(m_parentAdultEmergence);

		m_adult_longevity = Equations().GetAdultLongevity(m_sex);
		m_F = (m_sex == FEMALE) ? Equations().GetFecondity(m_adult_longevity) : 0;
	}

	CTRef CLaricobiusNigrinus::GetAdultEmergenceBegin(size_t y)const
	{
		return GetStand()->m_adultEmergenceBegin[y];
	}
	
	CTRef CLaricobiusNigrinus::GetParentAdultEmergence()const
	{
		const CWeatherStation& weather_station = GetStand()->GetModel()->m_weather;
		ASSERT(weather_station.GetEntireTPeriod().GetNbYears() >= 2);
		ASSERT(GetStand()->m_adultEmergenceBegin.size() == 2);


		CTRef parentAdultEmergence;
		double adultEmergingCDD = Equations().GetAdultEmergingCDD();

		CTRef begin = GetStand()->m_adultEmergenceBegin[0];
		CTRef end = CTRef(begin.GetYear() + 1, JUNE, DAY_30);

		double CDD = 0;
		for (CTRef TRef = begin; TRef <= end && !parentAdultEmergence.IsInit(); TRef++)
		{
			const CWeatherDay& wDay = weather_station.GetDay(TRef);
			double DD4 = GetStand()->m_DD4.GetDD(wDay);//use 4° as threshold
			CDD += DD4;
			if (CDD >= adultEmergingCDD)
			{
				parentAdultEmergence = wDay.GetTRef();
			}
		}

		return parentAdultEmergence;
	}

	CTRef CLaricobiusNigrinus::GetCreationDate(CTRef parentAdultEmergence)const
	{
		ASSERT(!m_creationDate.IsInit());

		CTRef creationDate;
		double creationCDD = Equations().GetCreationCDD();

		const CWeatherStation& weather_station = GetStand()->GetModel()->m_weather;
		ASSERT(weather_station.GetEntireTPeriod().GetNbYears() >= 2);
		ASSERT(GetStand()->m_adultEmergenceBegin.size() == 2);
		
		int year = GetStand()->m_adultEmergenceBegin[1].GetYear();
		//CTRef begin = parentAdultEmergence;
		//CTRef begin = GetStand()->m_adultEmergenceBegin[0];
		CTRef begin = CTRef(year, JANUARY, DAY_01);
		CTRef end = CTRef(year, JUNE, DAY_30);


		double CDD = 0;
		for (CTRef TRef = begin; TRef <= end && !creationDate.IsInit(); TRef++)
		{
			const CWeatherDay& wDay = weather_station.GetDay(TRef);
			double DD = GetStand()->m_DD.GetDD(wDay);
			CDD += DD;
			if (CDD >= creationCDD)
			{
				creationDate = wDay.GetTRef();
			}
		}

		ASSERT(creationDate.IsInit());

		return creationDate;
	}

	CLaricobiusNigrinus& CLaricobiusNigrinus::operator=(const CLaricobiusNigrinus& in)
	{
		if (&in != this)
		{
			CIndividual::operator=(in);

			//regenerate relative development rate
			for (size_t s = 0; s < NB_STAGES; s++)
				m_RDR[s] = Equations().GetRelativeDevRate(s);

			//			m_CDD = in.m_CDD;
					//	m_creationCDD = in.m_creationCDD;
						//			m_CDD_ADE = in.m_CDD_ADE;
									//m_aestivalDiapauseEndCDD = in.m_aestivalDiapauseEndCDD;
			m_adultEmergingCDD = in.m_adultEmergingCDD;
			m_adult_longevity = in.m_adult_longevity;// Equations().GetAdultLongevity(m_sex) / 2;
			m_F = in.m_F;
		}

		return *this;
	}
	// Object destructor
	CLaricobiusNigrinus::~CLaricobiusNigrinus(void)
	{}


	double CLaricobiusNigrinus::AdjustTLab(const string& name, size_t s, CTRef TRef, double T)
	{
		//	size_t s = GetStage();


			/*if (name == "VictoriaLab")
			{
				if (s == -1)
				{

				}

				if (s >= LARVAE)
					T = 13;
			}
			else */
		if (name == "BlacksburgLab")
		{
			if (s == -1)
			{
				if (TRef.GetJDay() < CTRef(0, MARCH, DAY_25).GetJDay())
					s = EGG;
				else if (TRef.GetJDay() < CTRef(0, APRIL, DAY_15).GetJDay())
					s = LARVAE;
				else if (TRef.GetJDay() < CTRef(0, MAY, DAY_25).GetJDay())
					s = PUPAE;
				else
					s = AESTIVAL_DIAPAUSE_ADULT;
			}

			//if we are in Blacksburg lab situation, take temperature depend of year
			//lab rearing: change temperature Lamb(2005) thesis
			if (s == LARVAE)
				T = 13;
			else if (s == PREPUPAE || s == PUPAE)
				T = 15;
			else if (s == AESTIVAL_DIAPAUSE_ADULT && TRef.GetYear() == 2003)
				T = 15;
			else if (s == AESTIVAL_DIAPAUSE_ADULT && TRef.GetYear() == 2004)
			{
				if (TRef < CTRef(2004, SEPTEMBER, DAY_27))
					T = 19;
				else
					T = 13;
			}
		}


		return T;
	}

	double CLaricobiusNigrinus::AdjustDLLab(const string& name, size_t s, CTRef TRef, double day_length)
	{
		/*if (name == "VictoriaLab")
		{
			day_length = 12;
		}
		else */
		if (name == "BlacksburgLab")
		{
			//if we are in Blacksburg lab situation, take 12 hours
			if (TRef.GetYear() == 2003)
				day_length = 14;
			else if (TRef < CTRef(2004, SEPTEMBER, DAY_27))
				day_length = 16;
			else
				day_length = 10;
		}

		return day_length;
	}


	//sans lab adjust
	//NbVal = 47	Bias = -2.15890	MAE = 6.48341	RMSE = 9.40934	CD = 0.94188	R² = 0.94691
	//	a5 = 920.13275 { 919.62096, 920.69584}	VM = { 0.05738,   0.18442 }
	//	b5 = 58.03780 {  57.86170, 58.24332}	VM = { 0.06575,   0.17611 }
	//	a6 = 153.32574 { 151.44158, 155.37724}	VM = { 0.63950,   1.79858 }
	//	b6 = 6.06014 {   6.05634, 6.06402}	VM = { 0.00035,   0.00117 }


	void CLaricobiusNigrinus::OnNewDay(const CWeatherDay& weather)
	{
		CIndividual::OnNewDay(weather);

		//if (!m_creationDate.IsInit())
		//{
		//	//CTRef TRef = weather.GetTRef();

		//	//compute creation date
		//		//at the first day, 
		//	CTRef year = weather.GetTRef().as(CTM::ANNUAL);
		//	//CTRef TRefBegin = CJDayRef(year.GetYear() - 1, GetStand()->m_adultEmegenceBegin[year - 1][0]);
		//	CTRef TRefBegin = CTRef(year.GetYear(), JANUARY, DAY_01);
		//	CTRef TRefEnd = CTRef(year.GetYear(), JUNE, DAY_30);

		//	double CDD = 0;
		//	const CWeatherStation& weather_station = GetStand()->GetModel()->m_weather;
		//	for (CTRef TRef = TRefBegin; TRef < TRefEnd && !m_creationDate.IsInit(); TRef++)
		//	{
		//		const CWeatherDay& wDay = weather_station.GetDay(TRef);
		//		double DD = GetStand()->m_DD.GetDD(wDay);
		//		CDD += DD;
		//		if (CDD >= m_creationCDD)
		//		{
		//			m_creationDate = weather.GetTRef();
		//		}
		//	}
		//}



	//if (!IsCreated(weather.GetTRef()))
	//{
	//	m_CDD += GetStand()->m_DD.GetDD(weather);
	//	//GetStand()->m_cumDD[m_ii];
	//	if (m_CDD >= m_creationCDD)
	//	{
	//		m_creationDate = weather.GetTRef();
	//		m_reachDate[EGG] = weather.GetTRef();
	//	}
	//}

	//const CLaricobiusNigrinusEquations& e = GetStand()->m_equations;


	//compute emergence begin
	//if (weather.GetTRef().GetJDay())
	/*
			CTRef TRef = weather.GetTRef();
			if(m_ii >= GetStand()->m_adultEmegenceBegin[TRef.GetYear()][0])
			{
				double T = weather[H_TNTX][MEAN];
				T = AdjustTLab(weather.GetWeatherStation()->m_name, TRef, T);
				double DD = max(0.0, T - e.m_OVP[Τᴴ]);

				m_CDD_AE += DD;
			}
	*/



	//double T = weather[H_TNTX][MEAN];
	//T = AdjustTLab(weather.GetTRef(), T);

	//double day_length = weather.GetDayLength() / 3600.0;
	//day_length = AdjustDLLab(weather.GetTRef(), day_length);
	//double threshold = Round(e.m_ADE[ʎ0] + e.m_ADE[ʎ1] * 1 / (1 + exp(-(day_length - e.m_ADE[ʎ2]) / e.m_ADE[ʎ3])), 1);

	//double DD = max(0.0, threshold - T);//DD can be negative
	//ASSERT(DD >= 0);
	//if (m_ii >= e.m_ADE[ʎa])
	//	m_CDD_ADE += Round(e.m_ADE[ʎb] + DD,1);



	//if (!IsCreated(weather.GetTRef()))
	//{
	//	double day_length = weather.GetDayLength() / 3600.0;
	//	double threshold = e.m_ADE[ʎ0] + e.m_ADE[ʎ1] * 1 / (1 + exp(-(day_length - e.m_ADE[ʎ2]) / e.m_ADE[ʎ3]));
	//	double T = weather[H_TNTX][MEAN];
	//	double DD = max(0.0, threshold - T);//DD can be negative
	//	ASSERT(DD >= 0);

	//	if (m_ii >= e.m_ADE[ʎa])
	//		m_CDD_ADE += e.m_ADE[ʎb] + DD;

	//	m_CDD_ADE += GetStand()->m_DD.GetDD(weather);
	//	if (m_CDD_ADE >= m_aestivalDiapauseEndCDD)
	//	{
	//		m_creationDate = weather.GetTRef();
	//		m_reachDate[ACTIVE_ADULT] = weather.GetTRef();
	//	}
	//}


	/*
	for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
	{

		if (m_ii >= e.m_ADE[ʎa])
		{
			const CWeatherStation* pWeather = weather.GetWeatherStation();


			CStatistic Tmean;
			CTRef TRef = weather.GetTRef();

			CWeatherDay const & wday = weather;
			for (CTRef TRef2 = TRef - int(m_ADE[ʎb]); TRef2 <= TRef; TRef2++)
			{
				double T = wday[H_TNTX][MEAN];
				Tmean += T;

				wday = wday.GetPrevious();
			}

			if (m_ii == e.m_ADE[ʎa])
				CDD[m_ii] = Tmean[MEAN];
			else
				CDD[m_ii] = min(CDD[m_ii - 1], Tmean[MEAN]);

		}
	}*/


	//	m_ii++;
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

		double nb_steps = (24.0 / timeStep);
		size_t h = weather.GetTRef().GetHour();
		size_t s = GetStage();

		double T = weather[H_TAIR];
		//if (weather.GetTRef() >= m_dropToGroundDate && s < ACTIVE_ADULT)
			//T = 4.27 + 0.825 *T; //Data from Mausel 2007, correction air/soil temperature from April to September

		T = AdjustTLab(weather.GetWeatherStation()->m_name, s, weather.GetTRef(), T);

		double day_length = weather.GetLocation().GetDayLength(weather.GetTRef()) / 3600.0;//[h]
		day_length = AdjustDLLab(weather.GetWeatherStation()->m_name, s, weather.GetTRef(), day_length);
		
		if (s < AESTIVAL_DIAPAUSE_ADULT)
		{
			//Time step development rate
			double r = Equations().GetRate(s, T) / nb_steps;
			
			double corr_r = (s == EGG || s == LARVAE) ? Equations().m_RDR[5][s] : 1;

			//Relative development rate for this individual
			double rr = (s == EGG || s == LARVAE) ? m_RDR[s] : 1;

			//Time step development rate for this individual
			r *= (corr_r*rr);
			ASSERT(r >= 0 && r < 1);
			//if (s == EGG || s == LARVAE)
				//r *= Equations().m_RDR[PREPUPAE][s];


			//Adjust age
			//m_age = min(double(ACTIVE_ADULT), m_age + r);
			m_age += r;

			if (!m_dropToGroundDate.IsInit() && m_age > LARVAE + 0.9)//drop to the soil when 90% competed (guess)
				m_dropToGroundDate = weather.GetTRef().as(CTM::DAILY);
		}
		else if (s == AESTIVAL_DIAPAUSE_ADULT)
		{
			if (!m_adultDate.IsInit())
				m_adultDate = weather.GetTRef().as(CTM::DAILY);


			//double pupationTime = m_adultDate - m_dropToGroundDate;//pupation time [days]
			//double r = Equations().GetAdultAestivalDiapauseRate(T, day_length, m_creationDate.GetJDay(), pupationTime) / nb_steps;

			//Relative development rate for this individual
			//double rr = m_RDR[s];
			//Time step development rate for this individual
			//r *= rr;
			//ASSERT(r >= 0 && r < 1);


			//start accumulating rate after a fixed date
			//if(weather.GetTRef() >= m_dropToGroundDate)
			//m_age += r;

			CTRef TRef = weather.GetTRef().as(CTM::DAILY);
			if (TRef >= GetAdultEmergenceBegin())
			{
				double DD = max(0.0, T - 4.0);//we used same temperature for emerging adult and individual creation
				m_age += min(1.0 / 6.0, (DD / m_adultEmergingCDD) / nb_steps);
			}
			//if (m_CDD_AE >= m_adultEmergingCDD)
				//m_age = ACTIVE_ADULT;


			//if (m_ii >= Equations().m_ADE[ʎa])
			//{
			//	CTRef TRef = weather.GetTRef().as(CTM::DAILY);
			//	double day_length = GetDayLength(pStand->GetModel()->GetInfo().m_loc.m_lat, TRef.GetJDay()) / 3600.0;
			//	//double f = exp(-1 + 2 * 1.0 / (1.0 + exp(-(day_length - Equations().m_ADE[ʎ2]) / Equations().m_ADE[ʎ3])));//day length factor
			//	//double f = exp(Equations().m_ADE[ʎ0] + Equations().m_ADE[ʎ1] * 1.0 / (1.0 + exp(-(m_ii - Equations().m_ADE[ʎ2]) / Equations().m_ADE[ʎ3])));//day length factor
			//	//double f = day_length * (1 - 2 * 1.0 / (1.0 + exp(-(m_ii - Equations().m_ADE[ʎ2]) / Equations().m_ADE[ʎ3])));//day length factor
			//	double f = day_length * (1 - 2 * 1.0 / (1.0 + exp(-(m_ii - Equations().m_ADE[ʎ2]) / Equations().m_ADE[ʎ3])));//day length factor


			//	//double f = exp((-1.0 + 2.0 / (1.0 + exp(-(day_length - Equations().m_ADE[ʎ0]) / Equations().m_ADE[ʎ1])))) * exp((1.0 - 2.0 / (1.0 + exp(-(m_ii - Equations().m_ADE[ʎ2]) / Equations().m_ADE[ʎ3]))));//day factor
			//	double Tavg30 = f + pStand->m_Tavg30[TRef][0];


			//	if (Tavg30 < m_aestivalDiapauseEndTavg30)
			//		m_age = ACTIVE_ADULT;
			//}
		}
		else//ACTIVE_ADULT
		{
			double r = (1.0 / m_adult_longevity) / nb_steps;
			ASSERT(r >= 0 && r < 1);

			m_age += r;

		}
	}




	//*****************************************************************************
	// Develops all stages, including adults
	// Input:	weather: the weather of the day
	//*****************************************************************************
	void CLaricobiusNigrinus::Live(const CWeatherDay& weather)
	{
		CIndividual::Live(weather);

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
		for (size_t step = 0; step < nbSteps&&m_age < DEAD_ADULT; step++)
		{
			size_t h = step * GetTimeStep();
			Live(weather[h], GetTimeStep());
		}

		if (weather.GetTRef() == m_creationDate || HasChangedStage())
			m_reachDate[GetStage()] = weather.GetTRef();


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

	// kills by old age and frost
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
			//Toland:L. nigrinus was -13.6 oC (± 0.5) with temperatures that ranged from -6 oC to -21 oC.
			if (weather[H_TMIN][MEAN] < COLD_TOLERENCE_T[s])
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


		if (IsCreated(d))
		{
			if (IsAlive() || (s == DEAD_ADULT))
				stat[S_EGG + s] += m_scaleFactor;


			if (m_status == DEAD && m_death == FROZEN)
				stat[S_DEAD_FROST] += m_scaleFactor;

			if (s == ACTIVE_ADULT && HasChangedStage())
				stat[S_ADULT_EMERGENCE] += m_scaleFactor;

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
	//CLNFStand

	void CLNFStand::init(int year, const CWeatherYears& weather)
	{
		CTPeriod p = weather[year].GetEntireTPeriod(CTM::DAILY);
		p += weather[year + 1].GetEntireTPeriod(CTM::DAILY);


		//m_cumDD.Init(p, 1);
		//m_adultEmegenceBegin.Init(p.as(CTM::ANNUAL), 1);
		//m_adultEmegenceBegin.res


		for (size_t y = 0; y < 2; y++, year++)
		{
			//CTPeriod p = weather[year].GetEntireTPeriod(CTM::DAILY);

		/*	double sumDD = 0;
			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			{
				const CWeatherDay& wday = weather[year].GetDay(TRef);
				sumDD += m_DD.GetDD(wday);
				m_cumDD[TRef][0] = sumDD;
			}*/

			m_adultEmergenceBegin[y] = ComputeAdultEmergenceBegin(weather[year]);
		}

	}
	//return ordinal date
	CTRef CLNFStand::ComputeAdultEmergenceBegin(const CWeatherYear& weather)const
	{
		CTPeriod p = weather.GetEntireTPeriod(CTM::DAILY);

		double sumDD = 0;
		for (size_t ii = 0; ii < m_equations.m_ADE[ʎ0]; ii++)
		{
			CTRef TRef = p.Begin() + ii;
			const CWeatherDay& wday = weather.GetDay(TRef);
			double T = wday[H_TNTX][MEAN];

			T = CLaricobiusNigrinus::AdjustTLab(wday.GetWeatherStation()->m_name, NOT_INIT, wday.GetTRef(), T);
			T = max(m_equations.m_ADE[ʎa], T);

			double DD = min(0.0, T - m_equations.m_ADE[ʎb]);//DD is negative
			sumDD += DD;
		}

		boost::math::logistic_distribution<double> begin_dist(m_equations.m_ADE[ʎ2], m_equations.m_ADE[ʎ3]);
		int begin = (int)Round(m_equations.m_ADE[ʎ0] + m_equations.m_ADE[ʎ1] * cdf(begin_dist, sumDD), 0);


		return p.Begin() + begin;
	}

	void CLNFStand::ComputeTavg30(int year, const CWeatherYears& weather)
	{
		CTPeriod p = weather[year].GetEntireTPeriod(CTM::DAILY);
		if (weather[year].HaveNext())
			p.End() = CTRef(year + 1, JULY, DAY_01);



		m_Tavg30.Init(p, 1);

		int lam_b = int(m_equations.m_ADE[ʎb]);
		for (CTRef TRef = p.Begin() + lam_b; TRef <= p.End(); TRef++)
		{
			CStatistic Tavg30;
			for (CTRef TRef2 = TRef - lam_b; TRef2 <= TRef; TRef2++)
			{
				const CWeatherDay & wday = weather.GetDay(TRef2);

				double T = wday[H_TMIN][MEAN];
				Tavg30 += T;
			}

			size_t ii = TRef - p.Begin();

			if (ii == lam_b)
			{
				for (size_t iii = 0; iii <= size_t(lam_b); iii++)//fill all value before July first
					m_Tavg30[iii][0] = Tavg30[MEAN];
			}
			else
			{
				if (ii < 182)
					m_Tavg30[ii][0] = max(m_Tavg30[ii - 1][0], Tavg30[MEAN]);
				else
					m_Tavg30[ii][0] = min(m_Tavg30[ii - 1][0], Tavg30[MEAN]);
			}

		}
	}

	void CLNFStand::GetStat(CTRef d, CModelStat& stat, size_t generation)
	{
		CStand::GetStat(d, stat, generation);

		int year = m_adultEmergenceBegin[1].GetYear();
		//CTRef begin = m_adultEmergenceBegin[0];
		CTRef begin = CTRef(year, JANUARY, DAY_01);
		CTRef end = CTRef(year, JUNE, DAY_30);
		if (d >= begin && d<= end)
		{
			const CWeatherStation& weather_station = GetModel()->m_weather;
			const CWeatherDay& wday = weather_station.GetDay(d);
			m_egg_creation_CDD += m_DD.GetDD(wday);
			stat[S_EGG_CREATION_CDD] = m_egg_creation_CDD;
		}
	}


}