//*****************************************************************************
//*****************************************************************************
// Class: CPopilliaJaponica
//          
//
// Description: the CPopilliaJaponica represents a group of PJN insect. scale by m_ScaleFactor
//*****************************************************************************
// 20/09/2023   Rémi Saint-Amant    Creation
//*****************************************************************************

#include "PopilliaJaponicaEquations.h"
#include "PopilliaJaponica.h"
#include <boost/math/distributions/weibull.hpp>
#include <boost/math/distributions/logistic.hpp>
#include <valarray>
#include "Basic/DegreeDays.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::PJN;


namespace WBSF
{

	//*********************************************************************************
	//CPopilliaJaponica class


	//*****************************************************************************
	// Object creator
	//
	// Input: See CIndividual creator
	//
	// Note: m_RDR (relative Development Rate)  member is init with random values.
	//*****************************************************************************
	CPopilliaJaponica::CPopilliaJaponica(CHost* pHost, CTRef creationDate, double age, TSex sex, bool bFertil, size_t generation, double scaleFactor) :
		CIndividual(pHost, creationDate, age, sex, bFertil, generation, scaleFactor)
	{
		//reset creation date
		for (size_t s = 0; s < NB_STAGES; s++)
			m_RDR[s] = Equations().GetRelativeDevRate(s);


		m_bDeadByAttrition = false;



		m_diapause_date = creationDate;
		if (m_diapause_date.GetJDay() == 0)
			m_diapause_date -= 180; //last summer

		double a1 = Equations().m_other[SNDH_MU];
		double a2 = Equations().m_other[SNDH_S];
		double F = Equations().m_other[SNDH_F];
		//double sndh = weather[H_SNDH][MEAN];
		double TsoilAMJ = GetStand()->m_TsoilAMJ;
		double L = 0.5 - CModelDistribution::get_cdf(TsoilAMJ, CModelDistribution::LOGISTIC, a1, a2);
		//m_diapauseCDD = m_r_snow * Equations().GetEndOfDiapauseCDD();
		m_r_snow = exp(F * L);


		//double snow = GetStand()->m_snow;  
		//double L = CModelDistribution::get_cdf(snow, CModelDistribution::LOGISTIC, a1, a2)-0.5;
		//m_r_snow = exp(F * L);
		


		//double L = 0.5 - CModelDistribution::get_cdf(meanTsoil, CModelDistribution::GOMPERTZ1, a1, a2);
		//m_r_snow = exp(F * L); //1/exp(-F * L);



		m_diapause_status = 0;
		m_diapause_RDR = Equations().GetRelativeDevRate(Equations().m_EOD[6]);

		if (m_sex == FEMALE)
			m_Fi = 50;


		m_cooling_point = Equations().GetCoolingPoint();
		//		m_Fi = (m_sex == FEMALE) ? Equations().GetFecondity() : 0;

	}

	CPopilliaJaponica& CPopilliaJaponica::operator=(const CPopilliaJaponica& in)
	{
		if (&in != this)
		{
			CIndividual::operator=(in);

			//new relative development rate
			for (size_t s = 0; s < NB_STAGES; s++)
				m_RDR[s] = Equations().GetRelativeDevRate(s);

			//m_adult_emergence = in.m_adult_emergence;

			m_EOD = in.m_EOD;
			//m_ACC = in.m_ACC;
			m_diapause_date = in.m_diapause_date;
			m_diapause_status = in.m_diapause_status;
			//m_diapauseCDD = in.m_diapauseCDD;
			m_r_snow = in.m_r_snow;

			m_reachDate = in.m_reachDate;
		}

		return *this;
	}

	//destructor
	CPopilliaJaponica::~CPopilliaJaponica(void)
	{}




	void CPopilliaJaponica::OnNewDay(const CWeatherDay& weather)
	{
		CIndividual::OnNewDay(weather);
	}

	//*****************************************************************************
	// Develops all stages for one time step
	// Input:	weather: weather of the hour
	//			timeStep: timeStep [h]
	//*****************************************************************************
	void CPopilliaJaponica::Live(CTRef d, double Tsoil, double Tair, size_t timeStep)
	{
		assert(IsAlive());
		assert(m_status == HEALTHY);


		double nb_steps = (24.0 / timeStep);
		//size_t h = weather.GetTRef().GetHour();
		size_t s = GetStage();

		//double T = weather[H_TAIR];

		bool bInDiapause = (s == L3) && m_diapause_status < 1.0;

		//special case for diapause calculation
		if (bInDiapause)
		{
			if (d.GetYear() > m_diapause_date.GetYear())
			{
				//compute end of diapause
				double r = Equations().GetRateDiapause(Tsoil) / nb_steps;
				r *= m_diapause_RDR;
				r *= m_r_snow;
				m_diapause_status += r;



				//if ( m_diapauseCDD >= GetStand()->m_CDD[d][0])
				//{
				//m_diapause_status = GetStand()->m_CDD[d][0] / m_diapauseCDD;
				//	}
			}
		}
		else
		{

			//Time step development rate
			double r = Equations().GetRate(s, Tsoil) / nb_steps;

			if(s==L3)
				r *= m_r_snow;


			if (s == ADULT)
			{

				r = Equations().GetRate(s, Tair) / nb_steps;
				//double a1 = Equations().m_other[SNDH_MU];
				//double a2 = Equations().m_other[SNDH_S];
				//double F = Equations().m_other[SNDH_F];
				//double L = CModelDistribution::get_cdf(Tair, CModelDistribution::LOGISTIC, a1, a2) - 0.5;
				//m_diapauseCDD = m_r_snow * Equations().GetEndOfDiapauseCDD();
				//m_r_snow = exp(F * L);

				//r *= m_r_snow;
			}

			//Relative development rate for this individual
			double rr = m_RDR[s];
			r *= rr;

			

		


			//correction factor
			//double rrr = Equations().m_psy[s];
			//r *= rrr;
			ASSERT(floor(m_age + 1) - m_age >= 0 && floor(m_age + 1) - m_age <= 1);
			r = min(r, floor(m_age + 1) - m_age);
			ASSERT(r >= 0 && r <= 1);

			//Adjust age
			m_age += r;


			/*if (s == ADULT)
			{
				if (IsDeadByAttrition(s, Tsoil, r))
					m_bDeadByAttrition = true;
			}*/


			//if (m_sex == FEMALE && GetStage() >= ACTIVE_ADULT)
			//{
			//	double to = 0;
			//	double t = timeStep / 24.0;
			//	double λ = Equations().GetFecondityRate(GetAge(), weather[H_TAIR]);
			//	double brood = m_Fi * (exp(-λ * (m_t - to)) - exp(-λ * (m_t + t - to)));

			//	m_broods += brood;
			//	m_totalBroods += brood;

			//	m_t += t;
			//}
		}
	}




	//*****************************************************************************
	// Develops all stages, including adults
	// Input:	weather: the weather of the day
	//*****************************************************************************
	void CPopilliaJaponica::Live(const CWeatherDay& weather)
	{
		CIndividual::Live(weather);

		//if (weather.GetTRef() < GetStand()->m_diapause_end || m_bInDiapause)
		//	return;

		//double a1 = Equations().m_other[SNDH_MU];
		//double a2 = Equations().m_other[SNDH_S];
		//double F = Equations().m_other[SNDH_F];
		////double sndh = weather[H_SNDH][MEAN];
		//double L = 0.5 - CModelDistribution::get_cdf(GetStand()->m_snow, CModelDistribution::LOGISTIC, a1, a2);
		//double r_snow = exp(F * L); //1/exp(-F * L);


		//double r_snow = 1/exp(-pow(sndh,a2)/a1);
		//double r_snow = 1;

		size_t DOY = weather.GetTRef().GetJDay();
		size_t nbSteps = GetTimeStep().NbSteps();
		for (size_t step = 0; step < nbSteps && IsAlive(); step++)
		{

			size_t h = step * GetTimeStep();
			//Use soil temperature
			//CHourlyData weather_soil;
			//weather_soil[H_TAIR] = 
			//double Tsoil = GetStand()->m_soil_temperature[DOY*24+h][0];
			double Tsoil = GetStand()->m_soil_temperature[weather[h].GetTRef()][0];
			Live(weather.GetTRef(), Tsoil, weather[h][H_TNTX], GetTimeStep());


			//Live(weather[h], GetTimeStep());
		}

		if (weather.GetTRef() == m_creationDate || HasChangedStage())
			m_reachDate[GetStage()] = weather.GetTRef();

		if (HasChangedStage() && GetStage() == L3)
			m_diapause_date = weather.GetTRef();//no more development up to the end of the year.


	}


	void CPopilliaJaponica::Brood(const CWeatherDay& weather)
	{
		assert(m_sex == FEMALE);


		if (GetStage() == ADULT)
		{
			//Brood 10 bouts of 5 eggs during 30 days


			int nbDays = weather.GetTRef().as(CTM::DAILY) - m_reachDate[ADULT].as(CTM::DAILY);
			if (m_Fi > 0)
			{
				double broods = Round(12.0 / (nbDays + 1));
				broods = max(0.0, m_Fi - broods);
				m_Fi -= broods;
				m_broods += broods;
				m_totalBroods += broods;

				if (m_bFertil && m_broods > 0)
				{
					ASSERT(m_age >= ADULT);
					CPJNStand* pStand = GetStand(); ASSERT(pStand);

					double gSurvival = 0.1;// pStand->m_generationSurvival;
					double scaleFactor = m_broods * m_scaleFactor * gSurvival;
					CIndividualPtr object = make_shared<CPopilliaJaponica>(m_pHost, weather.GetTRef(), EGG, RANDOM_SEX, true, m_generation + 1, scaleFactor);
					m_pHost->push_front(object);
				}
			}

			//brooding
			//m_broods = m_F;
			//m_totalBroods = m_F;
			//m_F = 0;
			//m_F -= m_F * r;
		}
	}

	// kills by old age and frost
	// Output:  Individual's state is updated to follow update
	void CPopilliaJaponica::Die(const CWeatherDay& weather)
	{
		//attrition mortality. Killed at the end of time step 


		if (GetStage() == ADULT && IsDeadByAttrition())
		{
			m_status = DEAD;
			m_death = ATTRITION;
		}

		if (GetStage() == DEAD_ADULT)
		{
			//Old age
			m_status = DEAD;
			m_death = OLD_AGE;
		}
		else
		{

			size_t s = GetStage();

			double Tsoil = GetStand()->m_soil_temperature[weather.GetTRef().as(CTM::HOURLY)][0];


			//if (GetStand()->m_bApplyFrost)
			{
				if (s == L2 || s == L3)
				{
					if (Tsoil < m_cooling_point)
					{
						m_status = DEAD;
						m_death = FROZEN;
					}
				}
				else
				{

					if (Tsoil <= 0)
					{
						m_status = DEAD;
						m_death = FROZEN;
					}
				}
			}
		}
	}


	//s: stage
	//T: temperature for this time step
	//r: development rate for this time step
	bool CPopilliaJaponica::IsDeadByAttrition()const
	{
		bool bDeath = false;

		//if (T >= 0)//under zero is manage by the frost mortality
		//{

			//daily survival
			//double ds = GetStand()->m_equations.GetDailySurvivalRate(s, T);
		double SS = GetStand()->m_equations.m_other[ADULT_SURVIVAL];
		//overall survival
		//double Time = 1 / Equations().GetRate(s, T);//time(days)
		//double S = pow(ds, Time);

		//time step survival
		//double SS = pow(S, r);

		//Computes attrition (probability of survival in a given time step, based on development rate)
		if (RandomGenerator().RandUniform() > SS)
			bDeath = true;
		//}



		return bDeath;
	}



	//*****************************************************************************
	// GetStat gather information of this object
	//
	// Input: stat: the statistic object
	// Output: The stat is modified
	//*****************************************************************************
	void CPopilliaJaponica::GetStat(CTRef d, CModelStat& stat)
	{
		if (IsCreated(d))
		{
			size_t s = GetStage();
			ASSERT(s <= DEAD_ADULT);

			if (IsAlive() || (s == DEAD_ADULT))
				stat[S_EGG + s] += m_scaleFactor;
			//stat[s < L3 || m_bInDiapause ? S_EGG + s : S_L3 + s - L3] += m_scaleFactor;



		//if (HasChangedStatus() && m_status == DEAD && m_death == ATTRITION)
			//stat[S_DEAD_ATTRTION] += m_scaleFactor;

		//if(d==m_EODDate)
		//stat[S_EOD] += m_scaleFactor* GetStand()->m_diapause[d][0];


			if (HasChangedStage() && s == L1)
				stat[S_EGG_HATCH] += m_scaleFactor;


			if (HasChangedStage() && s == ADULT)
				stat[S_ADULT_EMERGENCE] += m_scaleFactor;


			stat[S_EGG_BROOD] += m_broods;


			//if (HasChangedStatus() && m_status == DEAD && m_death == ATTRITION)
			if (m_status == DEAD && m_death == ATTRITION)
				stat[S_DEAD_ATTRITION] += m_scaleFactor;

			if (m_status == DEAD && m_death == FROZEN)
				stat[S_DEAD_BY_FROST] += m_scaleFactor;

			//if (s == ACTIVE_ADULT)
			//{
			//	stat[S_ADULT_ABUNDANCE] += m_scaleFactor * m_adult_abundance;
			//}
		}
	}


	void CPopilliaJaponica::Pack(const CIndividualPtr& pBug)
	{
		assert(m_sex == pBug->GetSex());

		CPopilliaJaponica* in = (CPopilliaJaponica*)(pBug.get());
		CIndividual::Pack(pBug);
	}

	double CPopilliaJaponica::GetInstar(bool includeLast)const
	{
		return (IsAlive() || m_death == OLD_AGE) ? GetStage() : CBioSIMModelBase::VMISS;
	}

	//*********************************************************************************************************************

	//*********************************************************************************
	//CPJNHost

	CPJNHost::CPJNHost(CStand* pStand) :
		CHost(pStand)
	{
	}


	void CPJNHost::Live(const CWeatherDay& weather)
	{
		CHost::Live(weather);
	}

	void CPJNHost::GetStat(CTRef d, CModelStat& stat, size_t generation)
	{
		CHost::GetStat(d, stat, generation);

		//stat[S_AVEARGE_INSTAR] = stat.GetAverageInstar(S_EGG, 0, S_DEAD_ADULT);
		//ASSERT(stat[S_AVEARGE_INSTAR] <= DEAD_ADULT);

	}

	//*************************************************
	//CPJNStand

	CPJNStand::CPJNStand(WBSF::CBioSIMModelBase* pModel, const CModelStatVector& soil_temperature, const CModelStatVector& PMSI) :
		WBSF::CStand(pModel),
		m_equations(pModel->RandomGenerator()),
		m_soil_temperature(soil_temperature),
		m_PMSI(PMSI)
		//m_bApplyAttrition(true),
		//m_bApplyFrost(true),
		//m_overwinter_s(1)
	{
		//m_sigma = 0;
		//m_psy_factor = 1;
	}

	void CPJNStand::GetStat(CTRef d, CModelStat& stat, size_t generation)
	{
		CStand::GetStat(d, stat, generation);


		CStatistic Tsoil;
		for (size_t h = 0; h < 24; h++)
			Tsoil += m_soil_temperature[d.as(CTM::HOURLY) + h][0];
		//Tsoil += m_soil_temperature[d.GetJDay() * 24 + h][0];


		stat[S_TSOIL] = Tsoil[MEAN];


		stat[S_CDD] = m_CDD[d][0];

		stat[S_ADULT_CATCH_CDD] = 100 * m_equations.GetAdultCatchCumul(m_CDD[d][0]);
		if (stat[S_ADULT_CATCH_CDD] < 0.1)
			stat[S_ADULT_CATCH_CDD] = 0;
		if (stat[S_ADULT_CATCH_CDD] > 99.9)
			stat[S_ADULT_CATCH_CDD] = 100;

	}

	//Tair: mean air temperature of actual day [°C]
	//Tsoil: soil temperature of last day [°C]
	//LAI: Leaf area index [m²/m²]
	//Litter: Ground litter as LAI equivalent [m²/m²]
	//z:	soil depth [m]
	// p: time lapse [second]
	//return soil temperature [°C]
	//Note that the initial value of soil temperature is calculated by multiplying DRz by air
	//temperature of the first Julian day and LAI equivalent of above ground litter is assumed as a half of the maximum LAI.
	//the first time, Tsoil must be 0.
	//double GetDeltaSoilTemperature(double z, double F, double litter)
	//{
	//	//LAI ant litter is equal to 0

	//	static const double Ks = 0.005;//soil thermal diffusivity [cm²/s]. possible range  [0.001, 0.01]
	//	static const double p = 24.0 * 60.0 *60.0; //[s]
	//	static const double k = 0.45;//extinction coefficient

	//	return exp(-F * z * sqrt(PI / (Ks * p))) * exp(-k*litter);//range from [0.93, 0.97]
	//}


	//double GetTairAtSurface(const CWeatherDay& weather, size_t h, double Fo)
	//{
	//	static const double Fs = 0.71;//[cm¯¹], 
	//	double Tair = -999;
	//	if (h == NOT_INIT)
	//	{
	//		double overheat = 0;
	//		if (weather[H_TAIR][MEAN] > 10)
	//			overheat = weather[H_TRNG2][MEAN] * Fo;

	//		Tair = weather[H_TAIR][MEAN] + overheat;
	//		Tair *= exp(-Fs * weather[H_SNDH][MEAN]);//take effect of snow
	//	}
	//	else
	//	{
	//		double overheat = 0;
	//		if (weather[h][H_TAIR] > 10)
	//			overheat = weather[H_TRNG2][MEAN] * Fo;

	//		Tair = weather[h][H_TAIR] + overheat;
	//		Tair *= exp(-Fs * weather[H_SNDH][MEAN]);//take effect of snow
	//	}

	//	return Tair;
	//}

	//CModelStatVector GetSoilTemperature(const CWeatherYear& weather, double z, double litter)
	//{
	//	CTPeriod p = weather.GetEntireTPeriod();// (CTM(CTM::DAILY));
	//	CModelStatVector output(p, 1, 0);

	//	
	//	//correction factor
	//	double F = max(0.0, min(8.0, 11.8 * pow(z, -0.740)));

	//	//overheating factor in function of depth
	//	double Fo = max(0.0, min(0.3, 0.212 * pow(z, -0.224)));
	//	double delta = GetDeltaSoilTemperature(z, F, litter);//daily change


	//	double Tsoil = 0;
	//	//pre init simulation with January of the first year
	//	for (size_t d = 0; d < weather[DECEMBER].size(); d++)
	//	{
	//		double Tair = GetTairAtSurface(weather[DECEMBER][d], NOT_INIT, Fo);
	//		Tsoil += (Tair - Tsoil) * delta;
	//	}
	//	

	//	for (size_t m = 0; m < weather.size(); m++)
	//	{
	//		for (size_t d = 0; d < weather[m].size(); d++)
	//		{
	//			for (size_t h = 0; h < weather[m][d].size(); h++)
	//			{
	//				//weather.GetHour();

	//				CTRef ref = weather[m][d][h].GetTRef();// .as(CTM(CTM::DAILY));

	//				double Tair = GetTairAtSurface(weather[m][d], h, Fo);
	//				Tsoil += (Tair - Tsoil) * delta/24.0;


	//				output[ref][0] = Tsoil;
	//			}
	//		}
	//	}
	//	

	//	return output;

	//}


	void CPJNStand::InitStand(const vector<double>& EOD, const array<double, NB_CDD_PARAMS >& ACC, std::array<double, NB_OTHER_PARAMS>& other, const CWeatherYear& weather)
	{
		m_equations.m_EOD = EOD;
		m_equations.m_ACC = ACC;
		m_equations.m_other = other;

		//double litter = other[LITTER];
		//m_soil_temperature = GetSoilTemperature(weather, 10, litter);//Get soil temperature at 10cm

		//static const double Tla = 12.8;
		//valarray<double> the15days(0.0, 15);
		////for (CTRef TRef = soil_temperature.GetFirstTRef(); TRef <= soil_temperature.GetLastTRef(); TRef++)
		//for (size_t DOY = 0; DOY < weather.size() && !m_diapause_end.IsInit(); DOY++)
		//{
		//	CStatistic stat;
		//	for (size_t h = 0; h < 24; h++)
		//		stat += m_soil_temperature[DOY * 24 + h][0];

		//	if (DOY < 15)
		//	{
		//		the15days[DOY] = stat[MEAN];
		//	}
		//	else
		//	{
		//		the15days = the15days.shift(1);
		//		the15days[14] = stat[MEAN];
		//		if (the15days.sum() / the15days.size() >= Tla)
		//		{
		//			m_diapause_end = m_soil_temperature.GetFirstTRef().as(CTM::DAILY) + DOY;
		//		}
		//	}
		//}

		//Ebbenga (2022)
		//with a 1 Jan.start date, lower threshold of 15 °C and an upper threshold of 21.7 °C,
		//

		//CDegreeDays DDModel(CDegreeDays::DAILY_AVERAGE, ACC[CDD_Τᴴ¹], ACC[CDD_Τᴴ²]);
		//DDModel.GetCDD(ACC[CDD_DELTA], weather, m_CDD);

		//CModelStatVector CDD_EOD;
		//CModelDistribution::get_CDD({ {0,0,0,0,0,10,50} }, weather.GetPrevious(), CDD_EOD);
		//m_snow = CDD_EOD.GetStat(0)[HIGHEST]/100;


		CTPeriod p = weather.GetEntireTPeriod();
		p.Begin().m_month = APRIL;
		p.Begin().m_day = DAY_30;
		p.End().m_month = JUNE;
		p.End().m_day = DAY_30;
		
		//p.Begin().m_month = MAY;
		//p.Begin().m_day = DAY_31;
		//p.End().m_month = JULY;
		//p.End().m_day = DAY_31;
		//m_snow = weather.GetPrevious().GetStat(H_TAIR, p)[MEAN];
		//m_snow = weather[JULY].GetStat(H_TAIR)[MEAN]- weather[JANUARY].GetStat(H_TAIR)[MEAN];


		CModelDistribution::get_CDD(ACC, weather, m_CDD);
		//m_snow = m_CDD.GetStat(0)[HIGHEST] / 100;

		m_snow = 0;
		for(size_t m=JANUARY; m<MAY; m++)
			m_snow += weather[m].GetStat(H_SNDH)[MEAN]/4.0;


		CStatistic stat;
		p = p.as(m_soil_temperature.GetTM());
		for(CTRef TRef=p.Begin(); TRef <=p.End(); TRef++)
			stat += m_soil_temperature[TRef][0];

		m_TsoilAMJ = stat[MEAN];

		
	}

}