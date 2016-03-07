#pragma once

namespace MTClim43
{

class CControl
{
public:
	CControl()
	{
		ndays=365; 
	};
//	file init;             // initialization file 
//	file inmet;            // input meteorological data file 
//	file outmet;           // output meteorological data file 
//	int nhead;             // number of header lines in input met file 
	int ndays;             // number of days of data in input file 
//	int indewpt;           // input dewpoint temperature flag (0=NO, 1=YES) 
//	int outhum;            // output humidity flag            (0=VPD, 1=VP) 
//	int inyear;            // input year flag                 (0=NO, 1=YES) 
//	char systime[100];     // system time at start of simulation 
//    char header[200];      // header string, written to all output files 
//    char outprefix[200];   // output filename prefix 
};

class CParameter
{
public:
	CParameter()
	{
		site_lat=0;   
		site_elev=0;  
		site_slp=0;   
		site_asp=0;   
		site_ehoriz=0;
		site_whoriz=0;
	};

//	double base_elev;      // base elevation, meters 
//	double base_isoh;      // base annual precip isohyet, cm 
	double site_lat;       // site latitude, dec. degrees (- for south) 
	double site_elev;      // site elevation, meters 
	double site_slp;       // site slope, degrees 
	double site_asp;       // site aspect, degrees 
//	double site_isoh;      // site annual precip isohyet, cm 
	double site_ehoriz;    // site east horizon, degrees 
	double site_whoriz;    // site west horizon, degrees 
//	double tmax_lr;        // maximum temperature lapse rate, deg C/1000m 
//	double tmin_lr;        // minimum temperature lapse rate, deg C/1000m 
};

class CData
{
public:
	CData()
	{
		yday=NULL;
		s_tmax=NULL;
		s_tmin=NULL;
		s_tday=NULL;
		s_prcp=NULL;
		//s_Es=NULL;
		//s_Ea=NULL;
		s_srad=NULL;
		s_dayl=NULL;
		s_swe=NULL;
		s_tdew=NULL;

		m_nbDays=0;
	};

	~CData()
	{
		data_free();
	};

	int data_alloc(int ndays);
	void SetData(float* simmin, float* simmax, float* simprec);
	void SetData(float* Tdew, float* simmin, float* simmax, float* simprec);
	void data_free();
	int snowpack();

//input
	int *yday;             // array of yearday values 
	double *s_tmax;        // array of site tmax values 
	double *s_tmin;        // array of site tmin values 
	double *s_tday;        // array of site daylight temperature values 
	double *s_prcp;        // array of site prcp values (cm)
	double *s_swe;         // array of site snow water equivalent values (cm) 

//input/output
	double *s_tdew;        // array of site Tdew values (deg C) 

//output
	//double *s_Es;          // array of site actual vapor pressure values (Pa) 
	//double *s_Ea;          // array of site saturation vapor pressure  values (Pa) 
	double *s_srad;        // array of site shortwave radiation values 
	double *s_dayl;        // array of site daylength values (s)

	int m_nbDays;

private:

	// parameters for the Tair algorithm 
    static const double  TDAYCOEF;      // (dim) daylight air temperature coefficient (dim) 

	// parameters for the snowpack algorithm 
    static const double  SNOW_TCRIT;    // (deg C) critical temperature for snowmelt    
    static const double  SNOW_TRATE;    // (cm/degC/day) snowmelt rate 

};
	



class CMTClim43
{
public:

	CControl ctrl; // iofiles and program control variables 

	CMTClim43();
	~CMTClim43();

	//bool Generate(int nbDay, double site_lat, double site_lon, int site_elevation, double slope, double orientation,
	//	float*simmin, float* simmax, float* simprec, double* modulateRad);// , double* Es, double* Ea, double* snowpack);

	//bool Generate(int nbDay, double site_lat, double site_lon, int site_elevation, double slope, double orientation,
	//	float* Tdew, float*simmin, float* simmax, float* simprec, double* modulateRad);//, double* Es, double* Ea,  double* snowpack);


	int calc_srad_humidity(const CControl *ctrl, const CParameter *p, 
		CData *data);

	int calc_srad_humidity_iterative(const CControl *ctrl,
		const CParameter *p, CData *data);
	
	
	double calc_pet(double rad, double ta, double pa, double dayl);
	double atm_pres(double elev);

private:

//physical constants for MTCLIM 4.3
//
//Peter Thornton
//NTSG, School of Forestry
//University of Montana
//1/20/2000
//
//(dim) stands for dimensionless values
    static const double SECPERRAD;  // seconds per radian of hour angle 
    static const double RADPERDAY;  // radians of Earth orbit per julian day 
    static const double RADPERDEG;  // radians per degree 
    static const double MINDECL;    // minimum declination (radians) 
    static const double DAYSOFF;    // julian day offset of winter solstice 
    static const double SRADDT;     // timestep for radiation routine (seconds) 

    static const double MA;         // (kg mol-1) molecular weight of air 
    static const double MW;         // (kg mol-1) molecular weight of water 
    static const double R;          // (m3 Pa mol-1 K-1) gas law constant 
    static const double G_STD;      // (m s-2) standard gravitational accel.  
    static const double P_STD;      // (Pa) standard pressure at 0.0 m elevation 
    static const double T_STD;      // (K) standard temp at 0.0 m elevation   
    static const double CP;         // (J kg-1 K-1) specific heat of air 
    static const double LR_STD;     // (-K m-1) standard temperature lapse rate 
    static const double EPS;        // (MW/MA) unitless ratio of molec weights 
    static const double PI;         // pi 

//model parameters for MTCLIM 4.3
//
//Some model parameters are set in the *.ini file. Others are set here.
//
//Peter Thornton
//NTSG, School of Forestry
//University of Montana
//1/20/2000
//
//(dim) stands for dimensionless values



// parameters for the radiation algorithm 
    static const double  TBASE;         // (dim) max inst. trans., 0m, nadir, dry atm 
    static const double  ABASE;         // (1/Pa) vapor pressure effect on transmittance 
    static const double  C;             // (dim) radiation parameter 
    static const double  B0;            // (dim) radiation parameter 
    static const double  B1;            // (dim) radiation parameter 
    static const double  B2;            // (dim) radiation parameter 
    static const double  RAIN_SCALAR;   // (dim) correction to trans. for rain day 
    static const double  DIF_ALB;       // (dim) diffuse albedo for horizon correction 
    static const double  SC_INT;        // (MJ/m2/day) snow correction intercept 
    static const double  SC_SLOPE;      // (MJ/m2/day/cm) snow correction slope 

	
//	int calc_srad_humidity(const CControl *ctrl, const CParameter *p, 
//		CData *data);
	int pulled_boxcar(double *input,double *output,int n,int w,int w_flag);

};

};