#include <gsl/gsl_randist.h>


#include "Inputs.h"



//JL: Best parameters of the full model, from Kyle et al. 2020
const std::array<double, EMParameters::NUM_PARS> EMParameters::BEST_PARAMS = 
{
	1.0, 1.0, 0.0, 0.000241071699421562, 0, 0, 0.00962435749864498, 10, 5, 50, 
	0, 0.37161719994828, 0.913699399999732, 2.2223804999527, 0.945435549999967, 0, 525.015699999847, 8.32036899999904, 0.119701349994476, 267.034499999981,
	7.88482749903281, 3.80285399989692, 0.070488499999861, 0.233982799999915, 7.05116449999956, 6.38002749970359, 3.54725448752468, 100.157149999888, 291.2745, 0.166585199947054 
};

//Initial host densities at each location (larvae/m^2)
const double EMParameters::INITIAL_S = 25.0;


//Initial resting spore densities (resting spores/m^2), taken from Kyle et al. 2020. The 1st to 3rd values represent low to high fungus densities.
const std::array<double, EMParameters::NB_FUNGUS_DESITIES> EMParameters::INITIAL_R = { 0.000691100449204947, 0.00571302599998285, 0.018277349998676 };

//sites:6,2,1,6,2,1,6,2,1// 0.000691100449204947, 0.00571302599998285, 0.018277349998676, 0.000691100449204947, 0.00571302599998285, 0.018277349998676, 0.000691100449204947, 0.00571302599998285, 0.018277349998676

//const double EMParameters::TIME_STEP = 0.01;		        // time step

EMParameters::EMParameters()
{

	PARS = BEST_PARAMS;
	initS = INITIAL_S;
	initR = INITIAL_R[FD_MEDIUM];

	global_fixed_parms();
}


// ------------------------------------------------------------------------------------------- //
void EMParameters::global_fixed_parms()
{
	PARS[0] = 1.0;
	PARS[1] = 1;			// dummy parm to differentiate between profile hood and maxhood
	PARS[8] = 5;
	PARS[9] = 5;			// gamma steps
	PARS[10] = 0.002;		//ck// ratio for virus infection.  Trying to set it and see what happens
}

