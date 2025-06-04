#pragma once

#include <gsl/gsl_odeiv.h>	// ODE solver

#include "ODE_Solver.h"
#include "Inputs.h"


// --------------------------------Begin ODE system of White model --------------------------------------------//
int fast_odes(double t, const double y[], double dydt[], void* Paramstuff)
{
	ODESOlverParam* Params = (ODESOlverParam*)Paramstuff;

	int m = int(Params->PARS[9]);  //CK//  the number of exposed classes.  adjustable and fit.  yay!

	//Need to fix because it is a constant right now	(set to Params->PARS[0]).  Going to try y[7] bc that should be C density...
	double C = y[m + 1];
	double nuV = 0.0;		//CK//  FOR FUNGUS ONLY MODEL. NO VIRUS INFECTION
	double nuF = Params->nuF;		//Params->PARS[3];
	double nuR = Params->nuR;	//param for nuR after it includes rainfall
	double muV = 0.0;		//CK// FUNGUS ONLY MODEL.  MAKE SURE DECAY IS ZERO SO NEGATIVE VIRUS DOESN'T HAPPEN!!
	double lambdaV = Params->PARS[17];
	double lambdaF = Params->PARS[18];		// decay rate of E_V and E_F

	double size_C = Params->size_C;  //Scaling effect of size on susceptibility over time.
	double muF = Params->muF;  //CK//  Using the site-specific muF that was loaded up in DDEVF14
	double S0 = Params->initS;
	double R = Params->initR;


	double Finf = y[0] * nuF * C;
	double Rinf = y[0] * nuF * R;

	// ------------------------------------------ ODEs -------------------------------------------- //
	if (y[0] < .000001)	
		dydt[0] = 0;
	else
		dydt[0] = -y[0] * (nuF * y[m + 1] + nuR * R);

	dydt[1] = nuF * y[m + 1] * y[0] + nuR * R * y[0] - m * lambdaF * y[1];

	for (int i = 2; i <= m; i++)
	{
		dydt[i] = m * lambdaF * (y[i - 1] - y[i]);
	}

	dydt[m + 1] = m * lambdaF * y[m] * size_C - muF * y[m + 1];  //Conidia class!  Transission from final exposed class (m) to conidia class (m+1)
	dydt[m + 2] = m * lambdaF * y[m];

	return GSL_SUCCESS;
}

////////////////////Begin Jacobian of White model///////////////////////////
int jacobian(double t, const double y[], double* dfdy, double dfdt[], void* Paramstuff)
{
	ODESOlverParam* Params = (ODESOlverParam*)Paramstuff;

	int m = int(Params->PARS[9]);  //CK//  the number of exposed classes.  adjustable and fit.  yay!

	//Need to fix because it is a constant right now	(set to Params->PARS[0]).  Going to try y[7] bc that should be C density...
	double C = y[m + 1];
	double nuV = 0.0;		//CK//  FOR FUNGUS ONLY MODEL. NO VIRUS INFECTION
	double nuF = Params->nuF;		//Params->PARS[3];
	double nuR = Params->nuR;	//param for nuR after it includes rainfall
	double muV = 0.0;		//CK// FUNGUS ONLY MODEL.  MAKE SURE DECAY IS ZERO SO NEGATIVE VIRUS DOESN'T HAPPEN!!
	double lambdaV = Params->PARS[17];
	double lambdaF = Params->PARS[18];		// decay rate of E_V and E_F

	double muF = Params->muF;  //CK//  Using the site-specific muF that was loaded up in DDEVF14
	double size_C = Params->size_C;  //Scaling effect of size on susceptibility over time.

	double k = Params->PARS[4];

	int DIM = int(Params->PARS[9] + 3);

	double S0 = Params->initS;
	double R = Params->initR;

	double hetero_pow = pow((y[0] / S0), C);

	*(dfdy + 0 * DIM + 0) = -nuF * y[m + 1] - nuR * R; // \partial (dS/dt) \partial S
	for (int i = 1; i <= m; i++)                //\partial (dS/dt) \partial E_i
	{
		*(dfdy + 0 * DIM + i) = 0;
	}
	*(dfdy + 0 * DIM + m + 1) = -nuF * y[0]; //\partial (dS/dt) \partial C
	*(dfdy + 0 * DIM + m + 2) = 0; //\partial (dS/dt) \partial R


	*(dfdy + 1 * DIM + 0) = nuF * y[m + 1] + nuR * R; // \partial (dE1/dt) \partial S
	*(dfdy + 1 * DIM + 1) = -m * lambdaF;  // \partial (dE1/dt) \partial E1
	for (int i = 2; i <= m; i++)               // \partial (dE1/dt) \partial Ei, i > 2
	{
		*(dfdy + 1 * DIM + i) = 0;
	}
	*(dfdy + 1 * DIM + m + 1) = nuF * y[0]; // \partial (dE1/dt) \partial C
	*(dfdy + 1 * DIM + m + 2) = 0; // \partial (dE1/dt) \partial R


	//This next bit (7 lines) could apparently be merged with the following chunk
	*(dfdy + 2 * DIM + 0) = 0;  // \partial (dE2/dt) \partial S
	*(dfdy + 2 * DIM + 1) = m * lambdaF;  // \partial (dE2/dt) \partial E1
	*(dfdy + 2 * DIM + 2) = -m * lambdaF;  // \partial (dE2/dt) \partial E2
	for (int i = 3; i <= m + 1; i++)
	{
		*(dfdy + 2 * DIM + i) = 0;  // \partial (dE2/dt) \partial Ei, i > 3
	}
	*(dfdy + 2 * DIM + m + 2) = 0; // \partial (dE2/dt) \partial E2


	//Now we do the remaining exposed classes
	for (int a = 3; a <= m; a++)
	{
		for (int b = 0; b <= m + 2; b++)
		{
			if (b == (a - 1))
			{
				*(dfdy + a * DIM + b) = m * lambdaF;   // \partial (dEi/dt) \partial Ei
			}
			else if (b == a)
			{
				*(dfdy + a * DIM + b) = -m * lambdaF;   // \partial (dEi/dt) \partial Ei+1
			}
			else
			{
				*(dfdy + a * DIM + b) = 0;
			}
		}
	}

	//Now for conidia
	for (int i = 0; i <= m - 1; i++)
	{
		*(dfdy + m + 1 * DIM + i) = 0;  // \partial (dC/dt) \partial S is m = 0, and then \partial (dC/dt) \partial Ei,  for i = 1 through m-1
	}
	*(dfdy + m + 1 * DIM + m) = m * lambdaF * size_C; // \partial (dC/dt) \partial Em
	*(dfdy + m + 1 * DIM + m + 1) = -muF;  // \partial (dC/dt) \partial C
	*(dfdy + m + 1 * DIM + m + 2) = 0; // \partial (dC/dt) \partial R


	//Now for R, the cumulative dead
	for (int i = 0; i <= m - 1; i++)
	{
		*(dfdy + m + 2 * DIM + i) = 0;  // \partial (dR/dt) \partial S, \partial Ei, for i = 1 through m - 1;
	}
	*(dfdy + m + 2 * DIM + m) = m * lambdaF;   // \partial (dR/dt)  \partial Em
	*(dfdy + m + 2 * DIM + m + 1) = 0; // \partial (dR/dt)  \partial C
	*(dfdy + m + 2 * DIM + m + 2) = 0; // \partial (dR/dt)  \partial R


	//Nothing is a continuous function of time (note that transmission rates are discrete functions of time)
	for (int i = 0; i <= DIM - 1; i++)
	{
		dfdt[i] = 0;
	}

	return GSL_SUCCESS;
}

// ------------------------------------------  ODE Solver  ----------------------------------------------- //
void ODE_Solver(double t_ode, double t_end, ODESOlverParam* Params, double* y_ode)
{
	int status_ode;
	double h_init = 1.0e-5;

	int DIM = int(Params->PARS[9] + 3);

	const gsl_odeiv_step_type* solver_ode = gsl_odeiv_step_rkf45; // Runge-Kutta Fehlberg (4, 5)


	// returns pointer to a newly allocated instance of a stepping function of type 'solver_ode' for a system of DIM dimensions //
	gsl_odeiv_step* step_ode = gsl_odeiv_step_alloc(solver_ode, DIM);
	gsl_odeiv_control* tol_ode = gsl_odeiv_control_standard_new(1.0e-10, 1.0e-5, 1.0, 0.2);
	gsl_odeiv_evolve* evol_ode = gsl_odeiv_evolve_alloc(DIM);

	gsl_odeiv_system sys_ode;
	sys_ode.function = fast_odes;
	sys_ode.jacobian = jacobian;
	sys_ode.dimension = (size_t)(DIM);
	sys_ode.params = Params;

	// ----------------------------------- Integrate Over Time ------------------------------------ //
	while (t_ode < t_end)
	{
		status_ode = gsl_odeiv_evolve_apply(evol_ode, tol_ode, step_ode, &sys_ode, &t_ode, t_end, &h_init, y_ode);

		for (int i = 0; i <= DIM; i++)
		{
			if (y_ode[i] > 0)
			{
				// keep y_ode as is
			}
			else
			{
				y_ode[i] = 0;
			}
			if (y_ode[i] > Params->initS)
			{
				y_ode[i] = Params->initS;
			}
		}
	}
	// -------------------------------------- Clear Memory ----------------------------------------- //
	gsl_odeiv_evolve_free(evol_ode);
	gsl_odeiv_control_free(tol_ode);
	gsl_odeiv_step_free(step_ode);

	//return (t_end);
}


