#pragma once

#include <time.h>
#include <gsl/gsl_rng.h>


inline gsl_rng *random_setup(void)
{
    srand((unsigned) time(NULL));

    long seedy = -rand();

    gsl_rng_env_setup ();
    gsl_rng_default_seed = seedy;
    const gsl_rng_type* TT = gsl_rng_default;

    return gsl_rng_alloc(TT);
}


