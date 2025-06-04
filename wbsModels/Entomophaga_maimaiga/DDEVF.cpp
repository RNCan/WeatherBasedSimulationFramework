
#include <vector>
#include <assert.h>
#include <gsl/gsl_randist.h>


#include "ODE_Solver.h"
#include "DDEVF.h"
#include "Inputs.h"



int get_DD10_DOY(int hatch, double DDsum, const EMWeatherYear& CCDATA)
{
	double DD10 = 0.0;
	int DOY = hatch;


	while (DD10 <= DDsum && DOY < 365)
	{
		double DDtemp_now = std::max(0.0, CCDATA[DOY][EM_TAIR] - 10.0);  //CK// begin calculation of accumulated Degree Days
		DD10 = DD10 + DDtemp_now;			//CK// summing degree days over time
		DOY++;
	}

	assert(DOY - hatch >= 1);
	return DOY - hatch;
}

double get_total_rain(int begin, int end, const EMWeatherYear& CCDATA)
{
	assert(begin <= end);
	assert(begin < CCDATA.size());
	assert(end < CCDATA.size());

	double total_rainfall = 0.0;
	for (int d = begin; d < end; d++)
	{
		total_rainfall += CCDATA[d][EM_PRCP];
	}

	return total_rainfall;
}

// DDEVF sets up model and calls ODE_SOLVER, returns Infection for this year
std::array<double, 365> DDEVF(int hatch, int MAXT3, const EMParameters& Params, const EMWeatherYear& CCDATA, gsl_rng* RandNumsPass)
{
	std::array<double, 365> result = { 0 };

	//init parameters
	double specific_muF = Params.PARS[6];   //general intercept for MAX TEMP decay function for conidia
	double specific_nuF = Params.PARS[3];   //Site-specific infection rate for conidia
	double rain_P1 = Params.PARS[21];  //fit param used to scale accumulating rain.
	double rain_P2 = Params.PARS[26];  //fit param used to scale accumulating rain.
	double rain_P3 = Params.PARS[29];  //fit param used to scale accumulating rain.
	double RH_P = Params.PARS[22];   //CK//  Parameter for RH data
	double temp_P = Params.PARS[23];   //CK//  Parameter for temperature data
	double fourth_size = Params.PARS[28];	//CK// degree day when the bugs reach 4th instar
	double C_end = Params.PARS[16];	  //CK// fit param that turns off new conidia production once a specific size has been reached
	int beta = int(Params.PARS[7]);//used to dictate how many days to go back when accumulating rain
	int theta = 1;					//used to determine lag period before calculating accumulated rainfall


	
	ODESOlverParam SolverParam;
	
	//init solver
	SolverParam.PARS = Params.PARS;
	SolverParam.size_C = 1.0;
	SolverParam.initS = Params.initS;			// initS
	SolverParam.initR = 0.0;



	int DIM = int(Params.PARS[9] + 3);   //number of ODE equations

	//init y for ODE
	std::vector<double> y_ode(DIM + 1, 0);
	y_ode[0] = Params.initS;

	
	


	
	//init random number
	std::vector<double> rand_nuR(MAXT3 + 1, 0);
	std::vector<double> rand_nuF(MAXT3 + 1, 0);

	// ----------------------------------- Generate Random Numbers -------------------------------------------- //
	for (int i = 0; i <= MAXT3; i++)
	{
		//JL: The stochasticity to change the transmission rates of conidia and resting spores vary every day.
		rand_nuR[i] = gsl_ran_gaussian(RandNumsPass, Params.PARS[11]); //2nd entry is stdev
		rand_nuF[i] = gsl_ran_gaussian(RandNumsPass, Params.PARS[12]);
	}
	

	// ------------------------------------- initialize model parameters --------------------------------------- //

	int R_start = get_DD10_DOY(hatch, Params.PARS[27], CCDATA);
	int R_end = get_DD10_DOY(hatch, Params.PARS[19], CCDATA);
	assert(R_start < R_end);

	// ---------------------------- Calculated Parameters  and Population Sizes ------------------------------- //
	
	for (int DOY = 0; DOY < hatch; DOY++)
		result[DOY] = y_ode[0];
	
	double DD10 = 0;    //accumulated degree days about 10 degrees C
	// -------------------- MAIN LOOP!! (calculate populations as time is increased) -------------------------- //
	for (int t = 0; t <= MAXT3; t++)
	{
		int DOY = hatch - 1 + t;//RSA: it's strange to begin one day before hatch???

		//init solver for this day
		
		// --------------------- resting spores bloom  -------------------- //
		if (t == R_start)
		{
			SolverParam.initR = Params.initR;
		}
		// --------------------- resting spores done -------------------- //
		else if (t == R_end)
		{
			SolverParam.initR = 0;
		}

		// -------------------------- integrate until next day ---------------------------------- //
		
		DD10 += std::max(0.0, CCDATA[DOY][EM_TAIR] - 10.0);  //CK// begin calculation of accumulated Degree Days
		if (DD10 >= C_end)
		{
			SolverParam.size_C = 0.0;   //CK// stops new conidia production once a fit size has been reached.
		}


		SolverParam.nuF = (DD10 / fourth_size) * specific_nuF * exp(RH_P * CCDATA[DOY][EM_RH_MIN]) * exp(rand_nuF[t]);
		SolverParam.muF = specific_muF * exp(temp_P * CCDATA[DOY][EM_TMAX]);	//CK// Conidia Decay Response #2.2  BEST SO FAR!!

		

		if (SolverParam.initR > 0.0)
		{
			double total_rainfall = get_total_rain(DOY - beta - theta, DOY - theta, CCDATA);
			double nuR2 = (rain_P1 / (1 + rain_P2 * exp(-rain_P3 * total_rainfall)) - rain_P1 / (rain_P2 + 1)) * exp(rand_nuR[t]);
			SolverParam.nuR = (DD10 / fourth_size) * nuR2;
		}
		else
		{
			SolverParam.nuR = 0.0;
		}


		ODE_Solver(t, t+1, &SolverParam, &(y_ode[0])); //Numerical integration from t to t+1, y_ode holds group densities
		
		// ---------------------------- end of the day---------------------------------- //
		result[DOY] = y_ode[0];
	}

	for (int DOY = MAXT3; DOY <365; DOY++)
		result[DOY] = y_ode[0];

	return result;
}
