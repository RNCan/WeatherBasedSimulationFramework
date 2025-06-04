#include "eco-climate.h"

#include "random_setup.h"
#include "DDEVF.h"




int get_DD_DOY(int start, double lim1, double lim2, double DDsum, const EMWeatherYear& CCDATA)
{
	double DD10 = 0.0;
	int DOY = start;

	while (DD10 <= DDsum && DOY < 365)   //Getting the starting day of epizootic (larvae hatch)
	{
		//CK// summing degree days over time
		DD10 += std::max(0.0, std::min(lim2, double(CCDATA[DOY][EM_TAIR])) - lim1);  //CK// begin calculation of accumulated Degree Days
		DOY++;
	}

	return DOY;
}


//JL: The general structure of code is similar to the code structure in Kyle et al. 2020.
//See Supplemental data at https://www.journals.uchicago.edu/doi/suppl/10.1086/707138 for the previous version of code in model fitting and parameterization.
std::vector<std::array<double, 365>> execute_ecoclimate_model(const EMParameters& Params, const EMWeatherYears& CCDATA)
{
	std::vector<std::array<double, 365>> infected(CCDATA.size());//init output
	//std::vector<double> infected(CCDATA.size());//init output

	// ---------------------------------------- Random Number Stuff ------------------------------------------------------ //
	gsl_rng* r_seed;
	r_seed = random_setup();

	// ------------------------------------ Declare Likelidhood Quanitites ----------------------------------------------- //
	int reps = 100;  //was 500

	for (size_t y = 0; y < CCDATA.size(); y++)   	//begin loop to go through 10 years of CC data FOR EACH GRID CELL!!!  QUICK AND DIRTY AVERAGES ACROSS YEARS
	{
		//Degree day needed for larvae hatch, from Russo et al. 1993
		int S_start = get_DD_DOY(0, 3.0, 40.0, 317.0, CCDATA[y]);

		//Degree day needed for pupation, from Carter et al. 1992
		int S_end = get_DD_DOY(S_start, 7.65, 41.0, 586.5, CCDATA[y]);

		int MAXT3 = S_end - S_start;	//number of days the bugs are active

		//double survivors = 0.0;
		std::array<double, 365> survivors = {0};
		for (size_t j = 0; j < reps; j++)
		{
			std::array<double, 365> s = DDEVF(S_start, MAXT3, Params, CCDATA[y], r_seed);   //NEED TO ADD q TO THE DDEVF FUNCTION.  UPDATE ALL WEATHER READING LINES
			//survivors += DDEVF(S_start, MAXT3, Params, CCDATA[y], r_seed);   //NEED TO ADD q TO THE DDEVF FUNCTION.  UPDATE ALL WEATHER READING LINES
			for (size_t d = 0; d < survivors.size(); d++)
				survivors[d] += s[d];
		}

		double total = reps * Params.initS;
		//infected[y] = 1.0 - (survivors / total);  //Average fraction infected across realizations

		for (size_t d = 0; d < survivors.size(); d++)
			infected[y][d] = 1.0 - (survivors[d] / total);  //Average fraction infected across realizations
	}//for all years


	return infected;
}
