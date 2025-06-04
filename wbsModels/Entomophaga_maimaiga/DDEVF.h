#pragma once

#include <gsl/gsl_rng.h>

#include "Inputs.h"


class EMParameters;

std::array<double, 365> DDEVF(int hatch, int MAXT3, const EMParameters& Params, const EMWeatherYear& CCDATA, gsl_rng* RandNumsPass);
