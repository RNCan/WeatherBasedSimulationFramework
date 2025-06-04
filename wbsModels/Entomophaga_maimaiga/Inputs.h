#pragma once


#include <array>
#include <vector>
#include <gsl/gsl_rng.h>


//Input data and parameters
enum TEMWeatherDay { EM_PRCP, EM_RH_MIN, EM_TMAX, EM_TAIR, NB_EM_WEATHER_VAR };

typedef std::array<float, NB_EM_WEATHER_VAR> EMWeatherDay;
typedef std::array<EMWeatherDay, 365> EMWeatherYear;
typedef std::vector<EMWeatherYear> EMWeatherYears;



class EMParameters
{
public:

	enum TFungusDensities { FD_LOW, FD_MEDIUM, FD_HIGH, NB_FUNGUS_DESITIES };


	

	enum TParamewters {
		PROFILE = 1,			//dummy parm to differentiate between profile hood and maxhood
		SPECIFIC_NUF = 3,		//Site-specific infection rate for conidia
		SPECIFIC_MUF = 6,		//general intercept for MAX TEMP decay function for conidia
		BETA = 7,				//used to dictate how many days to go back when accumulating rain
		GSTEPSV = 8,
		N_ODE_EQUATIONS = 9,	//number of ODE equations, 
		RATIO = 10,				//ck// ratio for virus infection.  Trying to set it and see what happens
		RAND_NUR = 11,			//transmission rates of conidia 
		RAND_NUF = 12,			//noise variance
		DELTA_FIXED = 13,
		C_END = 16,				//CK// fit param that turns off new conidia production once a specific size has been reached
		COVER_C = 17,			//CK// effect of cage.  Testing to see if EXP bugs had higher or lower infection than ferals
		DDSTOP = 19,			//CK// param used for stopping date
		COVER_R = 20,			//CK// effect of cage.  Testing to see if EXP bugs had higher or lower infection than ferals
		RAIN_P1 = 21,			//fit param used to scale accumulating rain.
		RH_P = 22,				//CK//  Parameter for RH data
		TEMP_P1 = 23,			//CK//  Parameter for temperature data
		OPEN_C = 24,			//CK// effect of cage.  Testing to see if EXP bugs had higher or lower infection than ferals
		OPEN_R = 25,			//CK// effect of cage.  Testing to see if EXP bugs had higher or lower infection than ferals
		RAIN_P2 = 26,			//fit param used to scale accumulating rain.
		DDSTART = 27,			//CK// param used for starting date
		FOURTH_SIZE = 28,		//CK// degree day when the bugs reach 4th instar
		RAIN_P3 = 29,			//fit param used to scale accumulating rain.
		NUM_PARS
	};

	
	

	
	static const double INITIAL_S;
	static const std::array<double, NB_FUNGUS_DESITIES> INITIAL_R;
	static const std::array<double, NUM_PARS> BEST_PARAMS;
    //static const double TIME_STEP;		        // time step
	


    EMParameters();

    std::array<double, NUM_PARS> PARS;
    double initS;
    double initR;
    
    void global_fixed_parms();
  
};
