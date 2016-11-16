#pragma once
//Southern Hemisphere conditions
//#define MAXDAYS 1095 // 3 times 365
//Oviposition date later than 280 or less than 85: Southern hemisphere
//#define SH_BEGIN 280
//#define SH_END 140


#include <vector>

//double ddays_sine_method(double tmin1,double tmax,double tmin2,double threshlow,double threshhigh);
//int allen_sin_wave(double,double,double,double,double *,int); // this routines generates temperature by using allen sin wave method 

double develop(int sex, int stage, std::vector<double>& hourly);
double rat(int sex, int stage, double t);
double cdfy(int, double);
double xlin(double , double, double, double);
double exprat(double , double, double, double);
double exptb(double , double, double);
double blogan(double , double, double, double, double);
double stnrat( double , double, double, double, double);
double alogan( double , double, double, double, double, double);
double clogan( double, double, double, double, double, double, double);
double typiii( double, double, double, double, double, double);
double sigmoid( double, double, double, double);
double stinnr(double,double,double,double,double);
double xlogst( double, double, double);
double ybulcd( double, double, double, double);


int round_off(double);