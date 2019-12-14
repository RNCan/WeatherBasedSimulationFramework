#include "GypsyMoth.h"
#include "GrayModel.h"
#include "SawyerModel.h"
#include "LyonsModel.h"
#include "JohnsonModel.h"
#include "GypsyMothCommon.h"
#include "Basic/TimeStep.h"

namespace WBSF
{

	const double CGypsyMoth::TOTAL_POP_THRESHOLD = 0.0001;
	double CGypsyMoth::m_SSSurvivalRate[2][8] = { { 0.65, 0.81, 0.91, 0.6, 0.09, 0, 0.36, 0.5 }, { 0.65, 0.81, 0.91, 0.6, 0.36, 0.26, 0.36, 0.5 } };
	bool CGypsyMoth::m_bApplyMortality = false;
	int CGypsyMoth::DEFAULT_FLAGS = 0;


	CGypsyMoth::CGypsyMoth(int hatchModelType, const CGMEggParam& eggParam)
	{
		_ASSERTE(hatchModelType >= JOHNSON_MODEL && hatchModelType < NB_HATCH_MODEL);

		m_pHatch = NULL;
		CreateHatchObject(hatchModelType, eggParam);

		Reset();
	}


	CGypsyMoth::~CGypsyMoth()
	{
		delete m_pHatch;
		m_pHatch = NULL;
	}

	void CGypsyMoth::Reset()
	{
		ASSERT(m_pHatch);
		m_pHatch->Reset();
		m_stageFreq.clear();
	}

	void CGypsyMoth::CreateHatchObject(int hatchModel, const CGMEggParam& eggParam)
	{
		delete m_pHatch;
		m_pHatch = NULL;

		switch (hatchModel)
		{
		case JOHNSON_MODEL: m_pHatch = new CJohnsonModel(eggParam); break;
		case LYONS_MODEL: m_pHatch = new CLyonsModel(eggParam); break;
		case SAWYER_MODEL:m_pHatch = new CSawyerModel(eggParam); break;
		case GRAY_MODEL:m_pHatch = new CGrayModel(eggParam); break;
		}

		_ASSERTE(m_pHatch);
	}

	void CGypsyMoth::SimulateDeveloppement(const CWeatherStation& weather, const CTPeriod& p)
	{
		Reset();

		m_stageFreq.Init(p.GetLength(), p.Begin());
		//if( param.m_hatchModelType != m_hatchModel)
		//CreateHatchObject( param.m_hatchModelType );

		//develop eggs
		m_pHatch->ComputeHatch(weather, p);

		for (CTRef d = m_stageFreq.GetFirstTRef(); d <= m_stageFreq.GetLastTRef(); d++)
			m_stageFreq[d][EGG] = 100 - m_pHatch->GetEggsPourcent(d, HATCH);


		if (m_pHatch->GetFirstHatch().IsInit())
			ComputeNonDiapause(weather, p);

	}

	void CGypsyMoth::ComputeNonDiapause(const CWeatherStation& weather, const CTPeriod& p)
	{
		//m_stageFreq.clear();

		//m_stageFreq.resize(  );
		//m_stageFreq.SetFirstTRef();


		int n_cohorts[2][8] = { 0 };                 //number of cohorts in each sex and stage
		int first_cohort[2][8] = { 0 };              //first unempty cohort in each sex and stage
		double recruits[2][9] = { 0 };                //stage-specific daily recruitment ([9] is dead moths)

#define MAXCOHORTS  200
		double den_cohort[2][8][MAXCOHORTS] = { 0 };  //density of all cohorts
		double age_cohort[2][8][MAXCOHORTS] = { 0 };  //cohort physiological age
		double eme_cohort[2][8][MAXCOHORTS] = { 0 };  //proportion of cohort molted to next stages

		static const double sex_ratio = .5;

		bool started = false;	// flag simulation in progress 

		//loop over days, from first hatch-on. 
		//Compute hourly temperatures, calculate development
		//on an hourly basis BUT update on a daily basis 
		//	int nbDay = weather.GetNbDay();
		//int firstHatch = m_pHatch->GetFirstHatch();
		CTRef firstHatch = m_pHatch->GetFirstHatch();
		for (CTRef day = firstHatch; day <= p.End(); day++)
		{
			//compute day's hourly temperatures
			//CDailyWaveVector hourly;// hourly temperature array 
			//weather.GetDay(day).GetAllenWave(hourly, 12.0, gTimeStep);
			std::vector<double> hourly(6);
			for (size_t hh = 0; hh < 6; hh++)
				hourly[hh] = weather.GetDay(day)[hh * 4][HOURLY_DATA::H_TAIR];

			//for all stages, both sexes, if a cohort exists
			for (int stage = 0; stage<8; ++stage)
			{
				for (int sex = 0; sex < 2; ++sex)
				{
					if (n_cohorts[sex][stage] > 0)
					{
						for (int cohort = first_cohort[sex][stage]; cohort < n_cohorts[sex][stage]; ++cohort)
						{
							double devel_sum = develop(sex, stage, hourly);//day's total development
							age_cohort[sex][stage][cohort] += devel_sum;

							// attrition based on stage-specific m_survival rates 
							if (devel_sum > 0)
								if (m_bApplyMortality)
									den_cohort[sex][stage][cohort] *= (double)(pow(m_SSSurvivalRate[sex][stage], devel_sum));

							// compute recruitment to next stage 
							double prop_eme_p = eme_cohort[sex][stage][cohort];// previous-day emergence 
							eme_cohort[sex][stage][cohort] = cdfy(stage, age_cohort[sex][stage][cohort]);
							if (eme_cohort[sex][stage][cohort] > 0.995)
								eme_cohort[sex][stage][cohort] = 1;

							recruits[sex][stage + 1] += (eme_cohort[sex][stage][cohort] - prop_eme_p)*den_cohort[sex][stage][cohort];


							if (eme_cohort[sex][stage][cohort] > 0.995)
								//if (eme_cohort[sex][stage][cohort] > 0.999999999999)
								first_cohort[sex][stage] = cohort + 1;
						}
					}
				}
			}
			//recruitment into 1st instar
			// start a new cohort only if a significant amount of hatch occurred (>1/1000th) 

			if (m_pHatch->GetEggs()[day][HATCHING] > 0.00001 && n_cohorts[0][0] < MAXCOHORTS - 1)
			{
				den_cohort[0][0][n_cohorts[0][0]] = m_pHatch->GetEggs()[day][HATCHING] * (1 - sex_ratio);  //male 1st instar
				den_cohort[1][0][n_cohorts[1][0]] = m_pHatch->GetEggs()[day][HATCHING] * sex_ratio;      //female 1st instar
				n_cohorts[0][0]++;
				n_cohorts[1][0]++;
			}

			//Male and female emergence
			m_stageFreq[day][MALE_EMERGED] = recruits[0][7];
			m_stageFreq[day][FEMALE_EMERGED] = recruits[1][7];

			//recruitment into other stages
			for (int stage = 1; stage < 8; stage++)
			{
				for (int sex = 0; sex < 2; sex++)
				{
					if (stage == 5 && sex == 0)
					{
						recruits[sex][stage + 1] = recruits[sex][stage];
						recruits[sex][stage] = 0;
					}
					if (recruits[sex][stage] > 0)
					{
						int n_temp = n_cohorts[sex][stage];
						den_cohort[sex][stage][n_temp] = recruits[sex][stage];
						n_cohorts[sex][stage]++;
						recruits[sex][stage] = 0;

					}
				}
			}

			// compile stage frequencies 
			double tot_pop = 0;	// total population alive today 
			double ai = 0;			// accumulator for average instar 
			for (int stage = 0; stage < 8; stage++)
			{
				m_stageFreq[day][L1 + stage] = 0;
				for (int sex = 0; sex < 2; ++sex)
				{
					for (int cohort = first_cohort[sex][stage]; cohort < n_cohorts[sex][stage]; ++cohort)
					{
						m_stageFreq[day][L1 + stage] += den_cohort[sex][stage][cohort]
							* (1 - eme_cohort[sex][stage][cohort]);
					}
				}

				tot_pop += m_stageFreq[day][L1 + stage];
				ai += m_stageFreq[day][L1 + stage] * stage;
			}
			_ASSERTE(m_stageFreq[day][MALE_ADULT] == 0);
			_ASSERTE(m_stageFreq[day][FEMALE_ADULT] == 0);

			m_stageFreq[day][DEAD_ADULT] = recruits[0][8] + recruits[1][8];


			//recruits[0][8] = 0;
			//recruits[1][8] = 0;
			// Male and female moths separately 
			for (int cohort = first_cohort[0][7]; cohort < n_cohorts[0][7]; ++cohort)
			{
				m_stageFreq[day][MALE_ADULT] += den_cohort[0][7][cohort] *
					(1 - eme_cohort[0][7][cohort]);
			}


			for (int cohort = first_cohort[1][7]; cohort < n_cohorts[1][7]; ++cohort)
			{
				m_stageFreq[day][FEMALE_ADULT] += den_cohort[1][7][cohort] *
					(1 - eme_cohort[1][7][cohort]);
			}


			m_stageFreq[day][TOT_POP] = tot_pop;
			if (tot_pop > TOTAL_POP_THRESHOLD)
			{
				started = true;
				m_stageFreq[day][AVR_INS] = ai / tot_pop;
			}
			else
			{
				_ASSERTE(m_stageFreq[day][AVR_INS] == 0);
				if (started)
				{
					if (m_stageFreq[day][DEAD_ADULT] > 99.99)
						m_stageFreq[day][DEAD_ADULT] = 100;

					day++;
					for (; day <= p.End(); day++)
						m_stageFreq[day][DEAD_ADULT] = m_stageFreq[day - 1][DEAD_ADULT];
					return;
				}
			}
		}

		return;
	}

	CTRef CGypsyMoth::GetNewOvipDate()const
	{
		//int newOvipDate=-1;
		CTRef newOvipDate;
		double sum_arr = 0.0f;
		double max_arr = 0.0f;

		//Look for mid-point of Female flight period
		for (int i = 366; i < m_stageFreq.size(); i++)
		{
			sum_arr += m_stageFreq[i][FEMALE_ADULT];
		}
		max_arr = sum_arr;
		sum_arr = 0;
		if (max_arr > 0.0f)
		{
			for (int i = 366; i < m_stageFreq.size(); i++)
			{
				sum_arr += m_stageFreq[i][FEMALE_ADULT];
				if (sum_arr >= max_arr / 2.0f)
				{
					newOvipDate = m_stageFreq.GetFirstTRef() + i;
					break;
				}
			}
		}

		return newOvipDate;
	}

	CTRef CGypsyMoth::GetLastDay()const
	{
		return m_stageFreq.GetLastTRef(TOT_POP, 0);
	}

	//See if the generation was viable
	bool CGypsyMoth::GetViabilityFlag(int* flagsIn)const
	{
		CTRef newOvipDate = GetNewOvipDate();

		int flags = 0;

		//****************************************************************************************************************
		//Assess this generation's output for stability and viability, and output
		//Criteria:
		//	1- Some adult emergence occurs during the available weather data
		//	2- Egg hatch starts less than 365 days after oviposition (egg viability)
		//	3- First Midwinter after oviposition is spent in 100% egg stage (diapause in winter)
		//	4- Once hatch starts, peak oviposition is reached prior to mid-winter (host synchrony)
		//
		//Flags is used to identify criterious

		//a NON-NEGATIVE newOvipDate means some oviposition has occurred. Evaluate stability
		if (newOvipDate.IsInit())
		{

			//mid winter is half in the period
			CTRef midWindter = m_stageFreq.GetFirstTRef() + m_stageFreq.GetTPeriod().GetLength() / 2;
			//mid summer is 182 day after mid winter
			CTRef midSummer = midWindter + 182;
			//CTRef firstDay = GetFirstDay(); 
			CTRef firstHatch = GetFirstHatch();
			CTRef medianDiapause = m_pHatch->GetMedian(DIAPAUSE);
			CTRef medianPosDiapause = m_pHatch->GetMedian(POSDIAPAUSE);

			//Criterion 1: Did the egg stage last less than 365 days ?
			//if((firstHatch-firstDay)<=365)
			//flags |= DIAPAUSE_BEFORE_WINTER; 

			//Criterion 1: Did the diapause last less than mid winter?
			if (medianDiapause <= midWindter)
				flags |= DIAPAUSE_BEFORE_WINTER;

			//Criterion 2: Did the posdiapause last less than mid summer?
			if (/*medianPosDiapause>=midWindter &&*/ medianPosDiapause <= midSummer)
				flags |= POSDIAPAUSE_BEFORE_SUMMER;

			//Criterion 3: Are there 100% eggs in mid winter?
			if (firstHatch > midWindter)	//first winter
				flags |= FIRST_WINTER_EGG;

			//Criterion 4: Once hatch starts, peak oviposition is reached prior to mid-winter (host synchrony)
			if (TestSecondWinter()) //second winter
				flags |= ADULT_BEFORE_WINTER;
		}

		if (flagsIn)
			*flagsIn = flags;

		return flags == VIABLE;
	}

	//for the second winter, all states must be 0 except adult
	bool CGypsyMoth::TestSecondWinter()const
	{
		//_ASSERTE(m_stageFreq.IsInside(midWinterDate));

		CTRef lastDate = m_stageFreq.GetLastTRef();

		// 99% of egg are hatched
		bool bGood = m_pHatch->GetEggsPourcent(lastDate, HATCH) > 99;

		if (bGood)
		{
			//all insect are adulds
			for (int i = L1; i < PUPAE; i++)
			{
				if (m_stageFreq[lastDate][i] > 1)
				{
					bGood = false;
					break;
				}
			}
		}

		return bGood;
	}

	void CGypsyMoth::GetOutputStat(CGMOutputVector& stat)const
	{
		//usually stat will be larger thant m_stageFreq. They will countain all years
		ASSERT(stat.size() > 0);
		ASSERT(m_stageFreq.size() == 730 || m_stageFreq.size() == 731);

		CTPeriod p = m_stageFreq.GetTPeriod();

		bool bViable = true;
		CTPeriod p2 = p.GetAnnualPeriodByIndex(1);
		for (CTRef i = p2.Begin(); i <= p2.End(); i++)
			stat[i] = bViable ? m_stageFreq[i] : m_stageFreq[0];


	}

}