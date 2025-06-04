#pragma once

#include "Inputs.h"



class ODESOlverParam
{
public:

    ODESOlverParam()
    {
        nuF = 0;
        nuR = 0;
        muF = 0;
        size_C = 0;

        initS=0;
        initR=0;
    }

    std::array<double, EMParameters::NUM_PARS> PARS;


    double nuF;
    double nuR;
    double muF;
    double size_C;

    double initS;
    double initR;

};

// ------------------------------------------  ODE Solver  ----------------------------------------------- //
void ODE_Solver(double t_ode, double t_end, ODESOlverParam* Params, double* y_ode);
