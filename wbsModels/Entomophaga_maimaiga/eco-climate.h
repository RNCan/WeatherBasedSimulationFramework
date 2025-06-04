#pragma once

#include "Inputs.h"


typedef std::vector<std::array<double, 365>> CEMResultVector;

//JL: The general structure of code is similar to the code structure in Kyle et al. 2020.
CEMResultVector execute_ecoclimate_model(const EMParameters& Params, const EMWeatherYears& CCDATA);

