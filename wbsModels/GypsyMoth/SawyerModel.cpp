#include <stdio.h>
#include <conio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "SawyerModel.h"
#include "GypsyMothCommon.h"
#include "Basic/WeatherStation.h"


using namespace std;


namespace WBSF
{

	static const size_t SAWYER_TIME_STEP = 4;

	//Sawyer's model
	static const size_t SUBSTAGES = 200;


	const double CSawyerModel::lower_thresh[3] = { 4.0, -5.0, 5.0 };
	const double CSawyerModel::upper_thresh[3] = { 38.0, 25.0, 35.0 };
	const double CSawyerModel::psiI = 0.019;
	const double CSawyerModel::rhoI = 0.1455;
	const double CSawyerModel::TmI = 33.993;
	const double CSawyerModel::deltaTI = 6.35;
	const double CSawyerModel::alphaI = 10.033;
	const double CSawyerModel::betaI = 0.523;
	const double CSawyerModel::gammaI = 0.499;

	//Model 1
	const double CSawyerModel::Tl0[2] = { -10., -15. };
	const double CSawyerModel::Th0[2] = { 20., 20. };
	const double CSawyerModel::Topt0[2] = { 5., 10. };
	const double CSawyerModel::Ropt0[2] = { 0.00017, 0.0003 };

	const double CSawyerModel::Tl1[2] = { 8.6, 8.8 };
	const double CSawyerModel::Th1[2] = { 47.4, 47.0 };
	const double CSawyerModel::Topt1[2] = { 37.8, 37.0 };
	const double CSawyerModel::Ropt1[2] = { 0.054, 0.052 };

	const double CSawyerModel::g[2][5] =
	{
		{ 0., 3.7381, -10.606, 13.132, -5.2639 },
		{ 0., 1., 0., 0., 0. }
	};



	CSawyerModel::CSawyerModel(const CGMEggParam& param) : CEggModel(param)
	{
		prediap_table = NULL;
	}

	CSawyerModel::~CSawyerModel()
	{
		if (prediap_table)
		{
			free(prediap_table);
			prediap_table = NULL;
		}
	}

	//int m_timeStep;
	ERMsg CSawyerModel::ComputeHatch(const CWeatherStation& weather, const CTPeriod& p)
	{
		ERMsg message;

		//resize output array
		m_eggState.Init(p.GetLength(), p.Begin());

		double r[SUBSTAGES] = { 0 };
		double Tl[SUBSTAGES] = { 0 };
		double Th[SUBSTAGES] = { 0 };
		double Topt[SUBSTAGES] = { 0 };
		double Ropt[SUBSTAGES] = { 0 };
		double delp[SUBSTAGES] = { 0 };

		bool diapause_started = false;
		//	int first_hatch = 730;


		//double dt = param.m_timeStep/24.;

		//	initialize pre-diapause model
		init_prediapause();

		int m = m_param.m_sawyerModel;
		//	initialize the delays, and compute the development parameters for
		//  substages
		for (int i = 0; i < SUBSTAGES; ++i)
		{
			r[i] = 0.;
			double age = (i + 0.5) / SUBSTAGES;
			double gi = g[m][0] + g[m][1] * age + g[m][2] * pow(age, 2.0) + g[m][3] * pow(age, 3) + g[m][4] * pow(age, 4.0);
			Tl[i] = Tl0[m] + gi*(Tl1[m] - Tl0[m]);
			Th[i] = Th0[m] + gi*(Th1[m] - Th0[m]);
			Topt[i] = Topt0[m] + gi*(Topt1[m] - Topt0[m]);
			Ropt[i] = Ropt0[m] + gi*(Ropt1[m] - Ropt0[m]);
			delp[i] = 10000.;
		}

		double class_size = (double)100 / NCLASS1;
		//int day=m_param.m_ovipDate - p.Begin();

		for (CTRef i = m_eggState.GetFirstTRef(); i < m_param.m_ovipDate; i++)
			m_eggState[i][DIAPAUSE] = MAXEGGS;

		CTRef day = m_param.m_ovipDate;
		double tout = 0;
		do
		{
			//here, each day, get hourly temperatures
			//CDailyWaveVector Htemp;
			//weather.GetDay(day).GetAllenWave(Htemp, 12, gTimeStep);

			double dayout = 0.;
			//loop over time steps 
			const CWeatherDay& wDay = weather.GetDay(day);
			for (size_t h = 0; h < 24; h += SAWYER_TIME_STEP)
			{
				double T = wDay[h][HOURLY_DATA::H_TAIR];
				double rin = 0.;
				if (T > Tl0[m] && T < Th1[m])
				{
					//are the slowest developers out yet?
					if (prediap_age[0] < 1.)
					{
						double prediap_rate = calc_prediap_rate(T);
						//prediapause development
						for (int prediapause_class = 0; prediapause_class < NCLASS1; prediapause_class++)
						{
							if (prediap_age[prediapause_class] < 1)  //temperature ranges checked elsewhere
							{
								prediap_age[prediapause_class] += prediap_rate*prediap_variability[prediapause_class];
								if (prediap_age[prediapause_class] >= 1)
								{
									rin += class_size;
								}
							}
						} //prediapause classes
					} //slowest out yet test

					//diapause distributed-delay cascade
					if (diapause_started || rin > 0.)
					{
						diapause_started = true;

						//compute the delay
						double Rt[SUBSTAGES] = { 0 };
						getRates(T, Tl, Th, Topt, Ropt, Rt);
						double dt = SAWYER_TIME_STEP / 24.;
						double rout = dellvf(rin, r, Rt, delp, dt, SUBSTAGES);
						dayout += rout;
					}
				}
			}
			tout += dayout;

			//if(first_hatch == 730 && tout >= 0.1) 
			//{
			//	first_hatch =  day;
			//	dayout=tout;
			//}

			//m_hatching[day]=(float)dayout; 
			if (tout >= 0.1)
			{
				m_eggState[day][HATCHING] = dayout;
				m_eggState[day][HATCH] = (tout / 100)*MAXEGGS;
				m_eggState[day][DIAPAUSE] = MAXEGGS - m_eggState[day][HATCH];
			}

			day++;
		} while (tout < 0.995 * 100 && day < m_param.m_ovipDate + 365);

		for (CTRef i = day; i <= m_eggState.GetLastTRef(); i++)
			m_eggState[i][HATCH] = MAXEGGS;

		return message;
	}

	double CSawyerModel::dellvf(double rin, double *r, double *rates, double *delp, double dt, int k)
	{
		double vin;
		double del;
		double fk, dr;
		double a, b;
		double fkdt;
		int i;

		fk = (double)k;
		fkdt = fk*dt;
		vin = rin;
		for (i = 0; i < k; ++i) {

			if (rates[i] > 0.) {
				del = 1. / rates[i];
			}
			else del = 10000.;

			a = fkdt / del;
			b = 1. + (del - delp[i]) / fkdt;
			dr = r[i];
			r[i] = __max(0., dr + a*(vin - b*dr));
			delp[i] = del;

			vin = dr;

		}
		return (r[k - 1]);
	}

	void CSawyerModel::getRates(double T, double *Tl, double *Th, double *Topt, double *Ropt, double *Rt)
	{

		int i;
		for (i = 0; i < SUBSTAGES; ++i) {
			if (T < Topt[i]) Rt[i] = max(0., (T - Tl[i]) / (Topt[i] - Tl[i]))*Ropt[i];
			else          Rt[i] = max(0., (Th[i] - T) / (Th[i] - Topt[i]))*Ropt[i];
		}
	}

	/**************Gray's prediapause model*****************/
	void CSawyerModel::init_prediapause()
	{
		allocate_prediapause_array();
		initialize_prediapause_ages();
		calc_prediapause_variability();
		calc_prediapause_rates();
	}


	//Calculates the developmental rates and constructs the rate table for each phase.
	//Each table will have an extra entry in each dimension because it's needed when
	//the table is called by diapause_rate() for example.
	void CSawyerModel::calc_prediapause_rates()
	{
		int temp;
		double T, rate;
		int i;

		double dt = SAWYER_TIME_STEP / 24.;

		//calculate prediapause rates for a lookup table  
		for (temp = round_off(lower_thresh[0]), i = 0; temp <= round_off(upper_thresh[0]); temp++)
		{
			T = (double)temp - lower_thresh[0];
			rate = psiI*(exp(rhoI*T) - exp(rhoI*TmI - (TmI - T) / deltaTI));

			if (rate > 0)
				prediap_table[i] = rate*dt; //(double)timeStep/24.;
			else 
				prediap_table[i] = 0;
			i++;
		}

		prediap_table[i] = prediap_table[i - 1];

	}


	//Calculates the variability factor for each class in each phase 
	void CSawyerModel::calc_prediapause_variability(void)
	{
		int nClass;
		int phase;
		double x;
		//calculate prediapause variability
		phase = 0;
		for (nClass = 0; nClass < NCLASS1; nClass++)
		{
			x = (nClass + 0.5) / NCLASS1;
			prediap_variability[nClass] = pow(gammaI + betaI*(-log(1 - x)), (1 / alphaI));
		}

	}

	void CSawyerModel::allocate_prediapause_array(void)
	{
		if (prediap_table)
		{
			free(prediap_table);
			prediap_table = NULL;
		}

		int temp_range_prediapause = round_off(upper_thresh[0]) - round_off(lower_thresh[0]) + 1;
		prediap_table = (double *)malloc((temp_range_prediapause + 1)*sizeof(double));
	}


	//Make all ages equal to zero and inhibitor titre equal to 1.0
	void CSawyerModel::initialize_prediapause_ages(void)
	{
		int prediapause_class;

		for (prediapause_class = 0; prediapause_class < NCLASS1; prediapause_class++)
		{
			prediap_age[prediapause_class] = 0;
		}
	}

	double CSawyerModel::calc_prediap_rate(double T)
	{
		int TP;
		double fraction;
		double dev_rate = 0;

		if (T < lower_thresh[0])
		{
			TP = 0;
		}
		else
		{
			if (T > upper_thresh[0])
			{
				//TP=TPMAX;
				TP = round_off(upper_thresh[0]) - round_off(lower_thresh[0]);
			}
			else
			{
				TP = round_off(T - lower_thresh[0]);
			}
		}


		fraction = T - (lower_thresh[0] + (double)TP);
		dev_rate = prediap_table[TP] + fraction*(prediap_table[TP + 1] - prediap_table[TP]);
		return(dev_rate);
	}

}