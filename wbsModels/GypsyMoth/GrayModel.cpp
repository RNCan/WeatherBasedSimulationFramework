// Gray's egg development model.     
// RECODED WITH SOME CHANGES BY JACQUES   
// Prediapause rates and variability from Gray et al. 1991 (Envir. Ent. 20(6)) 
// Diapause rates and variability from Experiment 9                   
// Postdiapause rates and variability from Experiment 10              
//  VERSION DATE:16-11/2008    
//***************************  Modifications  *************************
// JR 25/02/1999: corrected as per David's recommendation the date referencing
//              in routine sinewave().
// JR 25/02/1999: in module gray(), hatch is output (to array) as daily hatch rate
//              rather than cumulative hatch rate.
// JR 24/03/1999: Introduced GrayReset() to re-initialize (0.0) globals
// JR 25/03/1999: Introduced free_Gray_arrays() to free allocated memory before exit.
//				Also freed allocated arrays in module calc_rates()
//RSA 22/05/2005: Incorporate to BioSIM model Base
// JR 31/10/2005: Gross error in calculations of variability corrected
// JR 16/11/2008: Adaptation to Gray 2009 Environmental Entomology (all changes commented with "Gray 2009" for searching)
// JR 18/02/2009: Because of unresolved issues with Gray 2009, a switch is added to use or not the Gray 2009 changes   
//RSA 08/03/2011: Really add the switch
//RSA 25/01/2012: Compute over many years
//RSA 16/11/2016: Compute with BioSIM 11
//*********************************************************************
#include <stdio.h>
#include <conio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "GrayModel.h"
#include "Basic/WeatherStation.h"

using namespace std;

namespace WBSF
{

	static const size_t GRAY_TIME_STEP = 4; //[h]

	const int CGrayModel::number_classes[3] = { N_CLASS1, N_CLASS2, N_CLASS3 };    //number of variability classes in 3 phases

	const double CGrayModel::class_size[3] =
	{
		(double)MAXEGGS / number_classes[0],
		(double)MAXEGGS / (number_classes[0] * number_classes[1]),
		(double)MAXEGGS / (number_classes[0] * number_classes[1] * number_classes[2])
	};

	const double CGrayModel::lower_thresh[3] = { 4.0, -5.0, 5.0 };
	const double CGrayModel::upper_thresh[3] = { 38.0, 25.0, 35.0 };
	const double CGrayModel::psiI = 0.019;
	const double CGrayModel::rhoI = 0.1455;
	const double CGrayModel::TmI = 33.993;
	const double CGrayModel::deltaTI = 6.35;
	const double CGrayModel::start_inhibitor = 1.188;
	const double CGrayModel::RS_c = 763.315*pow(10.0, -3.0);
	const double CGrayModel::RS_rp = -640.447*pow(10.0, -3.0);
	const double CGrayModel::RP_c = 0.42178*pow(10.0, -3.0);
	const double CGrayModel::eff_res_1 = 1564.41*pow(10.0, -3.0);
	const double CGrayModel::eff_res_2 = 463.55*pow(10.0, -3.0);
	const double CGrayModel::PDR_c = -5627.11*pow(10.0, -3.0);
	const double CGrayModel::PDR_t = 59.9694*pow(10.0, -3.0);
	const double CGrayModel::PDR_t2 = 10.3904*pow(10.0, -3.0);
	const double CGrayModel::PDR_t4 = -0.007987*pow(10.0, -3.0);

	//#ifdef GRAY_2009
	////New parameters from Gray 2009 Environ. Entomol. Table 1
	//const double CGrayModel::tauIII   = 3.338182*pow(10.0,-7.0);
	//const double CGrayModel::deltaIII = 0.390727;
	//const double CGrayModel::omegaIII =-1.821620;
	//const double CGrayModel::kappaIII = 0.373854;
	//const double CGrayModel::psiIII   =-0.148244286*pow(10.0,-1.0);
	//const double CGrayModel::thetaIII = 1.561466667*pow(10.0,-4.0);
	////New parameters from Gray 2009 Environ. Entomol. Table 1
	//const double CGrayModel::alphaIII = 2.9140;
	//const double CGrayModel::betaIII  = 0.3924;
	//const double CGrayModel::gammaIII = 0.6015;
	//
	//#else
	// Old parameters (2005)
	const double CGrayModel::tauIII = -7.58*pow(10.0, -3.0);
	const double CGrayModel::deltaIII = 2.34*pow(10.0, -3.0);
	const double CGrayModel::omegaIII = -172.4*pow(10.0, -3.0);
	const double CGrayModel::kappaIII = 27.82*pow(10.0, -3.0);
	const double CGrayModel::psiIII = -0.77*pow(10.0, -3.0);
	const double CGrayModel::thetaIII = 0;
	//Old parameters (2005)
	const double CGrayModel::alphaIII = 3.003;
	const double CGrayModel::betaIII = 0.523;
	const double CGrayModel::gammaIII = 0.499;


	//#endif

	const double CGrayModel::alphaI = 10.033;
	const double CGrayModel::betaI = 0.523;
	const double CGrayModel::gammaI = 0.499;
	const double CGrayModel::alphaII = 2.00;
	const double CGrayModel::betaII = 0.62062;
	const double CGrayModel::gammaII = 0.56;

	CGrayModel::CGrayModel(const CGMEggParam& param) : CEggModel(param)
	{

		//******************************************
		prediapause_table = NULL;   //Lookup table of prediapause rate for each temperature
		d_inhibitor_table = NULL;  //Lookup table of inhibitor depletion rate for each temperature
		diapause_table = NULL;     //Lookup table of diapause rate for each temperature & inhibitor titre combination
		postdiapause_table = NULL; //Lookup table of postdiapause rate for each temperature & inhibitor titre combination

		memset(variability_table, 0, 4 * MAXCLASSES*sizeof(double));    //10 rate classes for each phase plus hatch
		memset(prediapause_age, 0, N_CLASS1*sizeof(double));         // age of each class in prediapause
		memset(inhibitor_titre, 0, N_CLASS1*N_CLASS2*sizeof(double)); //inhibitor level of each class in diapause
		memset(diapause_age, 0, N_CLASS1*N_CLASS2*sizeof(double));         //  age of each class in diapause
		memset(postdiapause_age, 0, N_CLASS1*N_CLASS2*N_CLASS3*sizeof(double));  //age of each class in postdiapause
		memset(diapause_eggs, 0, N_CLASS1*sizeof(double));
		memset(postdiapause_eggs, 0, N_CLASS1*N_CLASS2*sizeof(double));

		age_range_postdiapause = AGERANGEPOSTDIAPAUSE;       //re-initialized in GrayReset

	}


	ERMsg CGrayModel::ComputeHatch(const CWeatherStation& weather, const CTPeriod& p)
	{
		ERMsg  message;

		//resize output array
		//Resize( weather.GetNbDay() );
		m_eggState.Init(p.GetLength(), p.Begin());

		//alocate internal array
		allocate_arrays(p.GetNbDay());
		GrayReset(p.GetNbDay());

		calc_rates();			//construct rate tables for each phase
		//construct depletion table for diapause
		calc_variability();		//construct variability factor table for each class of each phase
		initialize_ages();		//make all ages=0 and inhibitor titres=1


		//daily loop from m_ovipDate-on 
		CEggState tot_eggs(NB_EGG_OUTPUT);      //used for accounting 
		tot_eggs[PREDIAPAUSE] = MAXEGGS;

		for (CTRef day = p.Begin(); day < m_param.m_ovipDate; day++)
			m_eggState[day][PREDIAPAUSE] = MAXEGGS;

		//int last_day = weather.GetNbDay();
		CTRef lastDay = p.End();
		//for (int day=param.GetOvipDate(); day<weather.GetNbDay(); day++)
		for (CTRef day = m_param.m_ovipDate; day <= p.End(); day++)
		{
			//compute total Eggs
			ComputeTotalEgg(day, weather, tot_eggs);
			m_eggState[day] = tot_eggs;
			if (m_eggState[day][HATCH] >= MAXEGGS)
			{
				lastDay = day;
				break;
			}
		} //day loop






		//fill egg array up to the end
		for (CTRef day = lastDay + 1; day <= p.End(); day++)
			m_eggState[day][HATCH] = MAXEGGS;


		//compute haching 
		ComputeHatching();

		free_Gray_arrays();

		return message;
	}


	void CGrayModel::ComputeTotalEgg(CTRef day, const CWeatherStation& weather, CEggState& tot_eggs)
	{
		//CDailyWaveVector hour_temp;                 //Array of hourly temperatures
		//weather.GetDay(day).GetAllenWave(hour_temp, 12, gTimeStep);

		const CWeatherDay& wDay = weather.GetDay(day);

		for (size_t h = 0; h < 24; h += GRAY_TIME_STEP)
		{
			double T = wDay[h][HOURLY_DATA::H_TAIR];

			//prediapause rates are age-independent 
			double prediap_rate = prediapause_rate(T);
			//prediapause development
			for (int prediapause_class = 0; prediapause_class < number_classes[0]; prediapause_class++)
			{
				if (prediapause_age[prediapause_class] < 1)  //temperature ranges checked elsewhere
				{
					double rate = prediap_rate*variability_table[0][prediapause_class];
					prediapause_age[prediapause_class] += rate;
					if (prediapause_age[prediapause_class] >= 1)
					{
						tot_eggs[0] -= class_size[0];
						tot_eggs[1] += class_size[0];
						diapause_eggs[prediapause_class] += class_size[0];
					}
				}
				else //eggs of this classs are PAST the prediapause phase 
				{
					if (diapause_eggs[prediapause_class] > 0)
					{
						for (int diapause_class = 0; diapause_class < number_classes[1]; diapause_class++)
						{
							if (diapause_age[prediapause_class][diapause_class] < 1) //diapause development
							{
								double rate = diapause_rate(T, inhibitor_titre[prediapause_class][diapause_class])*variability_table[1][diapause_class];
								diapause_age[prediapause_class][diapause_class] += rate;

								double depletion = 0.0;
								if (inhibitor_titre[prediapause_class][diapause_class] > 0)
									depletion = depletion_rate(T, inhibitor_titre[prediapause_class][diapause_class]);

								inhibitor_titre[prediapause_class][diapause_class] += max(-inhibitor_titre[prediapause_class][diapause_class], depletion);
								if (diapause_age[prediapause_class][diapause_class] >= 1)
								{
									tot_eggs[1] -= class_size[1];
									tot_eggs[2] += class_size[1];
									postdiapause_eggs[prediapause_class][diapause_class] += class_size[1];
								}
							}
							else //these eggs are in postdiapause
							{
								if (postdiapause_eggs[prediapause_class][diapause_class] > 0)  /*postdiapause development */
								{
									for (int postdiapause_class = 0; postdiapause_class < number_classes[2]; postdiapause_class++)
									{
										if (postdiapause_age[prediapause_class][diapause_class][postdiapause_class] < 1)
										{
											double rate = postdiapause_rate(T, postdiapause_age[prediapause_class][diapause_class][postdiapause_class])*variability_table[2][postdiapause_class];
											postdiapause_age[prediapause_class][diapause_class][postdiapause_class] += rate;
											if (postdiapause_age[prediapause_class][diapause_class][postdiapause_class] >= 1)
											{
												tot_eggs[2] -= class_size[2];
												tot_eggs[3] += class_size[2];
												postdiapause_eggs[prediapause_class][diapause_class] -= class_size[2];
											}
										}
									} //postdiapause classes 
								}
							}
						} //diapause classes
					}
				}
			} //prediapause classes
		} //m_timeStep

	}

	void CGrayModel::ComputeHatching()
	{
		double eggpr = 0;
		for (CTRef i = m_eggState.GetFirstTRef(); i <= m_eggState.GetLastTRef(); i++)
		{
			double haching = GetEggsPourcent(i, HATCH);
			m_eggState[i][HATCHING] = haching - eggpr;
			eggpr = haching;
		}
	}


	//Calculates 24 temperatures from the daily maximum
	//and minimum temperatures.
	/*void CGrayModel::sinwave(int iday)
	{
	int hour=0;
	int peak=12;
	int tday=0;         //a variable to let iday exceed 365
	double time_factor=6;  //"rotates" the radian clock to put the peak at the top
	double r_hour=0;     //hours converted to radians:  24=2*pi
	double mean1=0;      //mean temperature for day1
	double range1=0;     //half the difference of min and max temps of day1
	double mean2=0;      //mean of max temp (day1) and min temp (day2)
	double range2=0;     //half the difference of max (day1) and min (day2)
	double mean=0;       //one or the other of the means
	double range=0;      //one or the other of the ranges
	double theta=0;

	tday=iday;

	mean1=(T_max[tday]+T_min[tday])/2;
	range1=(T_max[tday]-T_min[tday])/2;
	mean2=(T_max[tday]+T_min[tday+1])/2;
	range2=(T_max[tday]-T_min[tday+1])/2;
	r_hour=3.14159/12;

	for(hour=0; hour<24; hour++)
	{
	if(hour<peak)mean=mean1;
	else mean=mean2;
	if(hour<peak)range=range1;
	else range=range2;
	theta=((double)hour-time_factor)*r_hour;
	hour_temp[hour]=mean+range*sin(theta);
	}
	}
	*/
	//Calculates the developmental rates and constructs the rate table for each phase.
	//Each table will have an extra entry in each dimension because it's needed when
	//the table is called by diapause_rate() for example.
	void CGrayModel::calc_rates()
	{
		double rate_zero = 0;
		double a_t = 0;
		double age = 0;
		double inhibitor_titre = 0;
		double hundreds = 100.0;
		double T = 0;      //temperature transforms for the PDR function
		double Z = 0;      //temperature transforms for the RP & effective resistance functions
		double x = 0;      //a temporary variable
		double RSK = 0;
		double ln_RP = 0;
		double rate = 0;

		//calculate prediapause rates for a lookup table  
		int i = 0;
		for (int temp = round_off(lower_thresh[0]); temp <= round_off(upper_thresh[0]); temp++)
		{
			T = (double)temp - lower_thresh[0];
			rate = psiI*(exp(rhoI*T) - exp(rhoI*TmI - (TmI - T) / deltaTI));
			if (rate > 0)
				prediapause_table[i] = rate*(double)(GRAY_TIME_STEP / 24.0);
			else 
				prediapause_table[i] = 0;
			i++;
		}

		prediapause_table[i] = prediapause_table[i - 1];

		//calculate PDR (rate), RP, RS, and effective resistance in diapause  
		int temp_range_diapause = (round_off)(upper_thresh[1] - lower_thresh[1] + 1);
		std::vector<double> RS(temp_range_diapause);
		std::vector<double> RP(temp_range_diapause);
		std::vector<double> PDR(temp_range_diapause);    //potential developmental rate in diapause
		std::vector<double> eff_res(temp_range_diapause);

		for (int temp = round_off(lower_thresh[1]), i = 0; temp <= round_off(upper_thresh[1]); temp++, i++)
		{
			//Z is a temperature transform for effective resistance and RP.
			Z = (upper_thresh[1] - (double)temp) / (upper_thresh[1] - lower_thresh[1]);
			x = eff_res_1*pow(Z, eff_res_2);
			eff_res[i] = 0.3 + 0.7*pow((1 - Z), x);
			RP[i] = 1.0 + RP_c*pow(exp(Z), 6.0);
			rate = max(0.0, exp(PDR_c + PDR_t*(double)temp + PDR_t2*pow((double)temp, 2.0) + PDR_t4*pow((double)temp, 4.0)));
			RS[i] = RS_c + RS_rp*RP[i];
			
			if (rate > 0)
				PDR[i] = rate;
			else 
				PDR[i] = 0;
		}

		//calculate inhib. depletion rates and actual develop. rates (in diapause) for lookup tables
		int inhib_titre_range = (round_off)(start_inhibitor*hundreds);
		i = 0;
		for (int temp = round_off(lower_thresh[1]); temp <= round_off(upper_thresh[1]); temp++)
		{
			RSK = start_inhibitor + RS[i];
			ln_RP = log(RP[i]);
			for (int j = inhib_titre_range - 1; j >= 0; j--)
			{
				inhibitor_titre = (double)j / hundreds;
				double pctDay = GRAY_TIME_STEP / 24.0;
				d_inhibitor_table[i][j] = max(-inhibitor_titre*pctDay, (inhibitor_titre - RSK)*ln_RP*pctDay);
				if (d_inhibitor_table[i][j] > 0)
				{
					printf("Inhibitor titre will INCREASE. You must change parameter values.\n");
					exit(0);
				}
				rate = max(0.0, (1 - inhibitor_titre*eff_res[i]))*PDR[i];
				if (rate <= 0)
					diapause_table[i][j] = 0;
				else 
					diapause_table[i][j] = rate*pctDay;
			}
			d_inhibitor_table[i][inhib_titre_range] = d_inhibitor_table[i][inhib_titre_range - 1];
			diapause_table[i][inhib_titre_range] = diapause_table[i][inhib_titre_range - 1];
			i++;
		}

		for (int j = inhib_titre_range; j >= 0; j--)
		{
			d_inhibitor_table[i][j] = d_inhibitor_table[i - 1][j];
			diapause_table[i][j] = diapause_table[i - 1][j];
		}


		//calculate postdiapause rates for a lookup table
		i = 0;
		for (int temp = round_off(lower_thresh[2]); temp <= round_off(upper_thresh[2]); temp++)
		{
			int j = 0;
			for (; j < age_range_postdiapause; j++)
			{
				age = (float)j / (age_range_postdiapause - 1);

				//#ifdef GRAY_2009
				//New equation (6) in Gray 2009. Environ. Entomol.
				//			rate_zero=tauIII*exp(deltaIII*(double)temp);
				//#else
				// 2005 code: 
				rate_zero = tauIII + deltaIII*(double)temp;
				//#endif
				a_t = omegaIII + kappaIII*(double)temp + psiIII*pow((double)temp, 2.0) + thetaIII*pow((double)temp, 3.0);
				rate = a_t*age + rate_zero;
				double pctDay = GRAY_TIME_STEP / 24.0;
				if (rate <= 0)
					postdiapause_table[i][j] = 0;
				else if (rate >= 1)
					postdiapause_table[i][j] = 1 * pctDay;
				else 
					postdiapause_table[i][j] = rate*pctDay;
			}

			postdiapause_table[i][j] = postdiapause_table[i][j - 1];
			i++;
		}

		for (int j = 0; j < age_range_postdiapause + 1; j++)
		{
			postdiapause_table[i][j] = postdiapause_table[i - 1][j];
		}
	}


	//Calculates the variability factor for each class in each phase 
	void CGrayModel::calc_variability(void)
	{
		int nClass = 0;
		int phase = 0;
		double x = 0;
		//calculate prediapause variability
		phase = 0;
		for (nClass = 0; nClass < number_classes[phase]; nClass++)
		{
			x = (double)nClass / number_classes[phase] + 0.5 / number_classes[phase];
			//		variability_table[phase][nClass]=pow(gammaI+betaI*(-log(1-x)),(1/alphaI)); //This was in error. JR 2005/10/31
			variability_table[phase][nClass] = gammaI + betaI*pow((-log(1 - x)), (1 / alphaI));
		}

		//calculate diapause variability
		phase = 1;
		for (nClass = 0; nClass < number_classes[phase]; nClass++)
		{
			x = (double)nClass / number_classes[phase] + 0.5 / number_classes[phase];
			//		variability_table[phase][nClass]=pow(gammaII+betaII*(-log(1-x)),(1/alphaII)); //This was in error. JR 2005/10/31
			variability_table[phase][nClass] = gammaII + betaII*pow((-log(1 - x)), (1 / alphaII));
		}

		//calculate postdiapause variability
		phase = 2;
		for (nClass = 0; nClass < number_classes[phase]; nClass++)
		{
			x = (double)nClass / number_classes[phase] + 0.5 / number_classes[phase];
			//		variability_table[phase][nClass]=pow(gammaIII+betaIII*(-log(1-x)),(1/alphaIII)); //This was in error. JR 2005/10/31
			variability_table[phase][nClass] = gammaIII + betaIII*pow((-log(1 - x)), (1 / alphaIII));
		}
	}


	//Determines the age increment in prediapause from the lookup table
	double CGrayModel::prediapause_rate(double T)
	{
		int TP = 0;
		double fraction = 0;
		double dev_rate = 0;

		if (T<lower_thresh[0] || T>upper_thresh[0])
		{
			if (T < lower_thresh[0])
				TP = 0;
			else 
				TP = round_off(upper_thresh[0]) - round_off(lower_thresh[0]);
		}
		else
		{
			TP = (round_off)(T - lower_thresh[0]);
		}

		fraction = T - (lower_thresh[0] + (double)TP);
		dev_rate = prediapause_table[TP] + fraction*(prediapause_table[TP + 1] - prediapause_table[TP]);

		return(dev_rate);
	}

	//Determines the inhibitor depletion from the lookup table
	double CGrayModel::depletion_rate(double T, double inhib_titre)
	{
		int TP = 0;
		int I = 0;
		double T_fraction = 0;
		double I_fraction = 0;
		double deplete_rate = 0;

		if (T<lower_thresh[1] || T>upper_thresh[1])
		{
			if (T < lower_thresh[1])TP = 0;
			else TP = round_off(upper_thresh[1]) - round_off(lower_thresh[1]);
			T_fraction = 0.0;
		}
		else
		{
			TP = (round_off)(T - lower_thresh[1]);
			T_fraction = T - (lower_thresh[1] + TP);
		}

		I = (round_off)(inhib_titre * 100);
		I_fraction = inhib_titre*100.0 - I;
		deplete_rate = d_inhibitor_table[TP][I] + T_fraction*(d_inhibitor_table[TP + 1][I] - d_inhibitor_table[TP][I]) + I_fraction*(d_inhibitor_table[TP][I + 1] - d_inhibitor_table[TP][I]);
		return(deplete_rate);
	}

	//Determines the age increment in diapause from the lookup table
	double CGrayModel::diapause_rate(double T, double inhib_titre)
	{
		int TP = 0;
		int I = 0;
		double T_fraction = 0;
		double I_fraction = 0;
		double dev_rate = 0;

		if (T<lower_thresh[1] || T>upper_thresh[1])
		{
			if (T < lower_thresh[1])TP = 0;
			else TP = round_off(upper_thresh[1]) - round_off(lower_thresh[1]);
			T_fraction = 0.0;
		}
		else
		{
			TP = (round_off)(T - lower_thresh[1]);
			T_fraction = T - (lower_thresh[1] + (double)TP);
		}

		I = (round_off)(inhib_titre * 100);
		I_fraction = inhib_titre * 100 - (double)I;

		dev_rate = diapause_table[TP][I] + T_fraction*(diapause_table[TP + 1][I] - diapause_table[TP][I]) + I_fraction*(diapause_table[TP][I + 1] - diapause_table[TP][I]);
		return(max(0.0, dev_rate));
	}

	//Determines the age increment in postdiapause from the lookup table
	double CGrayModel::postdiapause_rate(double T, double age)
	{
		int TP = 0;
		int Iage = 0;
		double T_fraction = 0;
		double age_intpart = 0;
		double age_fraction = 0;
		double dev_rate = 0;

		if (T<lower_thresh[2] || T>upper_thresh[2])
		{
			if (T < lower_thresh[2])
				TP = 0;
			else 
				TP = round_off(upper_thresh[2]) - round_off(lower_thresh[2]);

			T_fraction = 0.0;
		}
		else
		{
			TP = (round_off)(T - lower_thresh[2]);
			T_fraction = T - (lower_thresh[2] + TP);
		}

		age_fraction = modf(((double)age / .05), &age_intpart);
		Iage = (round_off)(age * 20);
		dev_rate = postdiapause_table[TP][Iage] + T_fraction*(postdiapause_table[TP + 1][Iage] - postdiapause_table[TP][Iage]) + age_fraction*(postdiapause_table[TP][Iage + 1] - postdiapause_table[TP][Iage]);
		return(dev_rate);
	}

	//Setup reads necessary parameters from setup file      
	//Remove all the spaces, newlines, and null at the end of a string
	void strip(char *line)
	{
		register int i;
		i = int(strlen(line)) - 1;
		while ((line[i] == '\0' || line[i] == '\n' || line[i] == ' ') && (i > -1))
			--i;
		line[i + 1] = '\0';
		return;
	}

	//Allocates arrays space.
	void CGrayModel::allocate_arrays(int days)
	{
		//	int days=366;
		int phases = 4;                        //3 development phases plus hatched eggs
		int hours = 24;

		int temp_range_prediapause = round_off(upper_thresh[0]) - round_off(lower_thresh[0]) + 1;
		int temp_range_diapause = round_off(upper_thresh[1]) - round_off(lower_thresh[1]) + 1;
		int temp_range_postdiapause = round_off(upper_thresh[2]) - round_off(lower_thresh[2]) + 1;
		int inhib_titre_range = round_off(start_inhibitor * 100) + 1;

		//The tables have 1 extra entry in each dimension. The last entry will be made
		//equal to the penultimtate entry in calc_rates(). This ultimate entry in needed
		//in diapause_rate() [and others] where a reference is made to
		//"diapause_table[TP+1][I]" or "diapause_table[TP][I+1]".
		prediapause_table = (double *)malloc((temp_range_prediapause + 1)*sizeof(double));

		diapause_table = (double **)malloc((temp_range_diapause + 1)*sizeof(double *));
		for (int i = 0; i < temp_range_diapause + 1; i++)
		{
			if ((diapause_table[i] = (double *)malloc((inhib_titre_range + 1)*sizeof(double))) == NULL)
			{
				printf("\nNot enough memory.");
				exit(0);
			}
		}

		d_inhibitor_table = (double **)malloc((temp_range_diapause + 1)*sizeof(double *));
		for (int i = 0; i < temp_range_diapause + 1; i++)
		{
			if ((d_inhibitor_table[i] = (double *)malloc((inhib_titre_range + 1)*sizeof(double))) == NULL)
			{
				printf("\nNot enough memory.");
				exit(0);
			}
		}

		postdiapause_table = (double **)malloc((temp_range_postdiapause + 1)*sizeof(double *));
		for (int i = 0; i < temp_range_postdiapause + 1; i++)
		{
			if ((postdiapause_table[i] = (double *)malloc((age_range_postdiapause + 1)*sizeof(double))) == NULL)
			{
				printf("\nNot enough memory.");
				exit(0);
			}
		}

	}


	//Make all ages equal to zero and inhibitor titre equal to 1.0
	void CGrayModel::initialize_ages(void)
	{
		for (int prediapause_class = 0; prediapause_class < number_classes[0]; prediapause_class++)
		{
			prediapause_age[prediapause_class] = 0;
			for (int diapause_class = 0; diapause_class < number_classes[1]; diapause_class++)
			{
				diapause_age[prediapause_class][diapause_class] = 0;
				inhibitor_titre[prediapause_class][diapause_class] = start_inhibitor;
				for (int postdiapause_class = 0; postdiapause_class < number_classes[2]; postdiapause_class++)
				{
					postdiapause_age[prediapause_class][diapause_class][postdiapause_class] = 0;
				}
			}
		}
	}

	void CGrayModel::GrayReset(int days)
	{
		int phases = 4;                        //3 development phases plus hatched eggs
		int hours = 24;

		int temp_range_prediapause = round_off(upper_thresh[0]) - round_off(lower_thresh[0]) + 1;
		int temp_range_diapause = round_off(upper_thresh[1]) - round_off(lower_thresh[1]) + 1;
		int temp_range_postdiapause = round_off(upper_thresh[2]) - round_off(lower_thresh[2]) + 1;
		int inhib_titre_range = round_off(start_inhibitor * 100) + 1;

		age_range_postdiapause = AGERANGEPOSTDIAPAUSE;

		for (int i = 0; i <= temp_range_prediapause; ++i)
			prediapause_table[i] = 0.;

		for (int i = 0; i <= temp_range_diapause; ++i) {
			for (int j = 0; j <= inhib_titre_range; ++j) {
				d_inhibitor_table[i][j] = 0.;
				diapause_table[i][j] = 0.;
			}
		}

		for (int i = 0; i <= temp_range_postdiapause; ++i)
		{
			for (int j = 0; j <= age_range_postdiapause; ++j)
			{
				postdiapause_table[i][j] = 0.;
			}
		}


		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < MAXCLASSES; ++j)
			{
				variability_table[i][j] = 0.0;
			}
		}

		for (int i = 0; i < N_CLASS1; ++i)
		{
			prediapause_age[i] = 0.0;
			diapause_eggs[i] = 0.0;
			for (int j = 0; j < N_CLASS2; ++j)
			{
				inhibitor_titre[i][j] = 0.0;
				diapause_age[i][j] = 0.0;
				for (int k = 0; k < N_CLASS3; ++k)
				{
					postdiapause_age[i][j][k] = 0.0;
				}
			}
		}
	}

	void CGrayModel::free_Gray_arrays()
	{
		//	int days=366;
		int hours = 24;

		int temp_range_diapause = round_off(upper_thresh[1]) - round_off(lower_thresh[1]) + 1;
		int temp_range_postdiapause = round_off(upper_thresh[2]) - round_off(lower_thresh[2]) + 1;

		free(prediapause_table);

		for (int i = 0; i < temp_range_diapause + 1; i++)
		{
			free(diapause_table[i]);
		}
		free(diapause_table);
		diapause_table = NULL;

		for (int i = 0; i < temp_range_diapause + 1; i++)
		{
			free(d_inhibitor_table[i]);
		}
		free(d_inhibitor_table);
		d_inhibitor_table = NULL;

		for (int i = 0; i < temp_range_postdiapause + 1; i++)
		{
			free(postdiapause_table[i]);
		}
		free(postdiapause_table);
		postdiapause_table = NULL;
	}

}