//mtclim43.cpp : code take from :
//MTCLIM
//VERSION 4.3  
//
//Peter Thornton
//NTSG, School of Forestry
//University of Montana
//1/20/00
//
//***************************************
//** Questions or comments? Contact... **
//** Dr. Peter E. Thornton             **
//** NTSG, School of Forestry          **
//** University of Montana             **
//** Missoula, MT 59812                **
//** email: peter@ntsg.umt.edu         **
//** phone: 406-243-4326               **
//***************************************
//
//--------------------------------------------------------------------------
//Code History
//------------
//Original code written by R.R. Nemani
//Updated   4/1/1989 by J.C. Coughlan
//Updated 12/23/1989 by Joe Glassy
//Updated   1/4/1993 by Raymond Hunt (version 2.1)
//Updated  3/26/1997 by Peter Thornton (version 3.0)
//Updated  7/28/1997 by Peter Thornton (version 3.1)
//Updated   5/7/1998 by Peter Thornton (version 4.0)
//Updated   8/1/1998 by Peter Thornton (version 4.1) 
//Updated  4/20/1999 by Peter Thornton (version 4.2) 
//Updated  1/20/2000 by Peter Thornton (version 4.3)
//--------------------------------------------------------------------------
//UNITS
//-----
//Temperatures           degrees C
//Temp. lapse rates      degrees C / 1000 m
//Precipitation          cm / day
//Vapor pressure         Pa
//Vapor pressure deficit Pa
//Radiation              W/m2, average over daylight period, and output as J/m² for sum of the day length
//Daylength              s (sunrise to sunset, flat horizons) 
//Elevation              m
//Latitude               decimal degrees
//Aspect                 decimal degrees
//Slope                  decimal degrees
//E/W horizons           decimal degrees

/*********************
**                  **
**  START OF CODE   **
**                  **
*********************/
#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <crtdbg.h>

#include "mtclim43.h"
using namespace MTClim43;

const double CMTClim43::SECPERRAD =13750.9871;     // seconds per radian of hour angle 
const double CMTClim43::RADPERDAY =0.017214;       // radians of Earth orbit per julian day 
const double CMTClim43::RADPERDEG =0.01745329;     // radians per degree 
const double CMTClim43::MINDECL =-0.4092797;       // minimum declination (radians) 
const double CMTClim43::DAYSOFF =11.25;            // julian day offset of winter solstice 
const double CMTClim43::SRADDT =600.0;             // timestep for radiation routine (seconds) 

const double CMTClim43::MA       =28.9644e-3;      // (kg mol-1) molecular weight of air 
const double CMTClim43::MW       =18.0148e-3;      // (kg mol-1) molecular weight of water 
const double CMTClim43::R        =8.3143;          // (m3 Pa mol-1 K-1) gas law constant 
const double CMTClim43::G_STD    =9.80665;         // (m s-2) standard gravitational accel.  
const double CMTClim43::P_STD    =101325.0;        // (Pa) standard pressure at 0.0 m elevation 
const double CMTClim43::T_STD    =288.15;          // (K) standard temp at 0.0 m elevation   
const double CMTClim43::CP       =1010.0;          // (J kg-1 K-1) specific heat of air 
const double CMTClim43::LR_STD   =0.0065;          // (-K m-1) standard temperature lapse rate 
const double CMTClim43::EPS      =0.62196351;      // (MW/MA) unitless ratio of molec weights 
const double CMTClim43::PI       =3.14159265;      // pi 


// parameters for the radiation algorithm 
const double  CMTClim43::TBASE       =0.870;  // (dim) max inst. trans., 0m, nadir, dry atm 
const double  CMTClim43::ABASE     =-6.1e-5;  // (1/Pa) vapor pressure effect on transmittance 
const double  CMTClim43::C          =   1.5;  // (dim) radiation parameter 
const double  CMTClim43::B0          =0.013;  // (dim) radiation parameter 
const double  CMTClim43::B1          =0.201;  // (dim) radiation parameter 
const double  CMTClim43::B2          =0.185;  // (dim) radiation parameter 
const double  CMTClim43::RAIN_SCALAR  =0.75;  // (dim) correction to trans. for rain day 
const double  CMTClim43::DIF_ALB       =0.6;  // (dim) diffuse albedo for horizon correction 
const double  CMTClim43::SC_INT       =1.32;  // (MJ/m2/day) snow correction intercept 
const double  CMTClim43::SC_SLOPE    =0.096;  // (MJ/m2/day/cm) snow correction slope 

CMTClim43::CMTClim43(void)
{
}

CMTClim43::~CMTClim43(void)
{
}
	
/********************************
**                             **
**      START OF MAIN()        **
**                             **
********************************/
//bool CMTClim43::Generate(int nbDay, double site_lat, double site_lon, int site_elevation, double slope, double orientation,
//						 float*simmin, float* simmax, float* simprec, double* modulateRad)
//{
//	_ASSERTE(site_lat>= -90 && site_lat<=90);
//	_ASSERTE(site_lon>= -180 && site_lat<=180);
//	_ASSERTE(site_elevation>= -110 && site_elevation<=5000);
//	_ASSERTE(slope>= 0 && slope<=90);
//	_ASSERTE(orientation>= 0 && orientation<=360);
//	// variable declarations 
//	
//	
//	CParameter p;  // site and base parameters 
//	CData data;    // site and base meteorological data arrays 
//	
//	// read initialization file, open input files, do basic error checking
//	//on input parameters, and open output file 
//	ctrl.ndays = nbDay;
//
//	p.site_lat = site_lat;
//	p.site_elev = site_elevation;
//	p.site_slp = slope;
//	p.site_asp = orientation;
//	p.site_ehoriz = 0;
//	p.site_whoriz = 0;
//
//
//	// allocate space in the data arrays for input and output data 
//	if (data.data_alloc(ctrl.ndays))
//	{
//		//printf("Error in data_alloc()... exiting\n");
//		return false;
//	}
//	data.SetData(simmin, simmax, simprec);
//	//printf("Completed data_alloc()\n");
//	
//	
//	/* test for the presence of Tdew observations, and branch to the
//	appropriate srad and humidity algorithms */
//	//if (ctrl.indewpt)
//	//{
//	//	/* estimate srad and humidity using real Tdew data */
//	//	if (calc_srad_humidity(&ctrl, &p, &data))
//	//	{
//	//		printf("Error in calc_srad_humidity()... exiting\n");
//	//		exit(1);
//	//	}
//	//	printf("Completed calc_srad_humidity()\n");
//	//	
//	//}
//	//else /* no dewpoint temperature data */
//	//{	
//		/* estimate srad and humidity with iterative algorithm */
//		if (calc_srad_humidity_iterative(&ctrl, &p, &data))
//		{
//			//printf("Error in calc_srad_humidity_iterative()... exiting\n");
//			//exit(1);
//			return false;
//		}
//		//printf("Completed calc_srad_humidity_iterative()\n");
////	}
//	
//	
//	for(int i=0; i<ctrl.ndays; i++)
//	{
//		modulateRad[i] = data.s_srad[i]*data.s_dayl[i]/1000000;
//		//Es[i] = data.s_Es[i]/1000;
//		//Ea[i] = data.s_Ea[i]/1000;
//		//if (VPD[i] < 0.0) VPD[i] = 0.0;
//
//		//snowpack[i] = data.s_swe[i]*10;
//	}
//
//	return true;
//}


//
//bool CMTClim43::Generate(int nbDay, double site_lat, double site_lon, int site_elevation, double slope, double orientation,
//						 float*Tdew, float*simmin, float* simmax, float* simprec, double* modulateRad)//, double* Es, double* Ea, double* snowpack)
//{
//	_ASSERTE(site_lat>= -90 && site_lat<=90);
//	_ASSERTE(site_lon>= -180 && site_lat<=180);
//	_ASSERTE(site_elevation>= -110 && site_elevation<=5000);
//	_ASSERTE(slope>= 0 && slope<=90);
//	_ASSERTE(orientation>= 0 && orientation<=360);
//	// variable declarations 
//	
//	
//	CParameter p;  // site and base parameters 
//	CData data;    // site and base meteorological data arrays 
//	
//	// read initialization file, open input files, do basic error checking
//	//on input parameters, and open output file 
//	ctrl.ndays = nbDay;
//
//	p.site_lat = site_lat;
//	p.site_elev = site_elevation;
//	p.site_slp = slope;
//	p.site_asp = orientation;
//	p.site_ehoriz = 0;
//	p.site_whoriz = 0;
//
//
//	// allocate space in the data arrays for input and output data 
//	if (data.data_alloc(ctrl.ndays))
//	{
//		//printf("Error in data_alloc()... exiting\n");
//		return false;
//	}
//	data.SetData(Tdew, simmin, simmax, simprec);
//	//printf("Completed data_alloc()\n");
//	
//	
//	// test for the presence of Tdew observations, and branch to the
//	//appropriate srad and humidity algorithms 
//	//estimate srad and humidity using real Tdew data */
//	if (calc_srad_humidity(&ctrl, &p, &data))
//	{
//		//printf("Error in calc_srad_humidity()... exiting\n");
//		return false;
//	}
////	printf("Completed calc_srad_humidity()\n");
////	
//	//}
//	//else /* no dewpoint temperature data */
//	//{	
//		/* estimate srad and humidity with iterative algorithm */
//		//if (calc_srad_humidity_iterative(&ctrl, &p, &data))
//		//{
//		//	//printf("Error in calc_srad_humidity_iterative()... exiting\n");
//		//	//exit(1);
//		//	return false;
//		//}
//		//printf("Completed calc_srad_humidity_iterative()\n");
////	}
//	
//	
//	for(int i=0; i<ctrl.ndays; i++)
//	{
//		modulateRad[i] = data.s_srad[i]*data.s_dayl[i]/1000000;
//		//Es[i] = data.s_Es[i]/1000;
//		//Ea[i] = data.s_Ea[i]/1000;
//		//if (VPD[i] < 0.0) VPD[i] = 0.0;
//
//		//snowpack[i] = data.s_swe[i]*10;
//	}
//
//	return true;
//}

/* end of main */





/* end of snowpack() */

// when dewpoint temperature observations are available, radiation and
//humidity can be estimated directly 
int CMTClim43::calc_srad_humidity(const CControl *ctrl, const CParameter *p, CData *data)
{
	int ok=1;
	int i,ndays;
//	double pva,pvs,vpd;
	int ami,yday;
	double ttmax0[366];
	double flat_potrad[366];
	double slope_potrad[366];
	double daylength[366];
	double *dtr, *sm_dtr;
//	double tmax,tmin;
	double t1,t2;
	double pratio;
	double lat,coslat,sinlat,dt,dh,h;
	double cosslp,sinslp,cosasp,sinasp;
	double bsg1,bsg2,bsg3;
	double decl,cosdecl,sindecl,cosegeom,sinegeom,coshss,hss;
	double sc,dir_beam_topa;
	double sum_flat_potrad, sum_slope_potrad, sum_trans;
	double cosh,sinh;
	double cza,cbsa,coszeh,coszwh;
	double dir_flat_topa,am;
	double trans1,trans2;
	double t_tmax,b,t_fmax;
	double t_final,pdif,pdir,srad1,srad2; 
	double sky_prop;
	double avg_horizon, slope_excess;
	double horizon_scalar, slope_scalar;
	
	// optical airmass by degrees 
	double optam[21] = {2.90,3.05,3.21,3.39,3.69,3.82,4.07,4.37,4.72,5.12,5.60,
	6.18,6.88,7.77,8.90,10.39,12.44,15.36,19.79,26.96,30.00};
	
	// number of simulation days 
	ndays = ctrl->ndays;

	// calculate humidity from Tdew observations 
	//for (i=0 ; i<ndays ; i++)
	//{
		// convert dewpoint to vapor pressure 
		//data->s_Ea[i] = 610.7 * exp(17.38 * data->s_tdew[i] / (239.0 + data->s_tdew[i]));
		//if (ctrl->outhum)
		//{
			// output humidity as vapor pressure 
		//	data->s_hum[i] = pva;
		//}
		//else
		//{
			// output humidity as vapor pressure deficit 
			// calculate saturation vapor pressure at tday 
			//data->s_Es[i] = 610.7 * exp(17.38 * data->s_tday[i] / (239.0 + data->s_tday[i]));
			// calculate vpd 
			/*vpd = pvs-pva;
			if (vpd < 0.0) vpd = 0.0;
			data->s_hum[i] = vpd;*/
		//}
	//}
	
	// estimate radiation using Tdew observations 
	// allocate space for DTR and smoothed DTR arrays 
	if (!(dtr = (double*) malloc(ndays * sizeof(double))))
	{
		printf("Error allocating for DTR array\n");
		ok=0;
	}
	if (!(sm_dtr = (double*) malloc(ndays * sizeof(double))))
	{
		printf("Error allocating for smoothed DTR array\n");
		ok=0;
	}

	// calculate diurnal temperature range for transmittance calculations 
	//for (i=0 ; i<ndays ; i++)
	//{
	//	tmax = data->tmax[i];
	//	tmin = data->tmin[i];
	//	if (tmax < tmin) tmax = tmin;
	//	dtr[i] = tmax-tmin;
	//}
	for (i=0 ; i<ndays ; i++)
	{
		//double tmax = data->tmax[i];//old code use base temperature???
		//double tmin = data->tmin[i];
		double tmax = data->s_tmax[i];
		double tmin = data->s_tmin[i];
		if (tmax < tmin) tmax = tmin;
		dtr[i] = tmax-tmin;
	}
	
	// smooth dtr array using a 30-day antecedent smoothing window 
	if (ndays >= 30)
	{
		if (pulled_boxcar(dtr, sm_dtr, ndays, 30, 0))
		{
			printf("Error in boxcar smoothing, calc_srad_humidity()\n");
			ok=0;
		}
	}
	else // smoothing window width = ndays 
	{
		if (pulled_boxcar(dtr, sm_dtr, ndays, ndays, 0))
		{
			printf("Error in boxcar smoothing, calc_srad_humidity()\n");
			ok=0;
		}
	}

//*****************************************
//*                                       *
//* start of the main radiation algorithm *
//*                                       *
//*****************************************
	
	// STEP (1) calculate pressure ratio (site/reference) = f(elevation) 
	t1 = 1.0 - (LR_STD * p->site_elev)/T_STD;
	t2 = G_STD / (LR_STD * (R/MA));
	pratio = pow(t1,t2);
	
	// STEP (2) correct initial transmittance for elevation  
	trans1 = pow(TBASE,pratio);
	
	// STEP (3) build 366-day array of ttmax0, potential rad, and daylength 

	// precalculate the transcendentals 
	lat = p->site_lat;
	// check for (+/-) 90 degrees latitude, throws off daylength calc 
	lat *= RADPERDEG;
	if (lat > 1.5707) lat = 1.5707;
	if (lat < -1.5707) lat = -1.5707;
	coslat = cos(lat);
	sinlat = sin(lat);
	cosslp = cos(p->site_slp * RADPERDEG);
	sinslp = sin(p->site_slp * RADPERDEG);
	cosasp = cos(p->site_asp * RADPERDEG);
	sinasp = sin(p->site_asp * RADPERDEG);
	// cosine of zenith angle for east and west horizons 
	coszeh = cos(1.570796 - (p->site_ehoriz * RADPERDEG));
	coszwh = cos(1.570796 - (p->site_whoriz * RADPERDEG));
	
	// sub-daily time and angular increment information 
	dt = SRADDT;                // set timestep  
	dh = dt / SECPERRAD;        // calculate hour-angle step 
	
	// begin loop through yeardays 
	for (i=0 ; i<365 ; i++)
	{
		// calculate cos and sin of declination 
		decl = MINDECL * cos(((double)i + DAYSOFF) * RADPERDAY);
		cosdecl = cos(decl);
		sindecl = sin(decl);
		
		// do some precalculations for beam-slope geometry (bsg) 
		bsg1 = -sinslp * sinasp * cosdecl;
		bsg2 = (-cosasp * sinslp * sinlat + cosslp * coslat) * cosdecl;
		bsg3 = (cosasp * sinslp * coslat + cosslp * sinlat) * sindecl;
		
		// calculate daylength as a function of lat and decl 
		cosegeom = coslat * cosdecl;
		sinegeom = sinlat * sindecl;
		coshss = -(sinegeom) / cosegeom;
		if (coshss < -1.0) coshss = -1.0;  // 24-hr daylight 
		if (coshss > 1.0) coshss = 1.0;    // 0-hr daylight 
		hss = acos(coshss);                // hour angle at sunset (radians) 
		// daylength (seconds) 
		daylength[i] = 2.0 * hss * SECPERRAD;

		// solar constant as a function of yearday (W/m^2) 
		sc = 1368.0 + 45.5*sin((2.0*PI*(double)i/365.25) + 1.7);
		// extraterrestrial radiation perpendicular to beam, total over
		//the timestep (J) 
		dir_beam_topa = sc * dt;
		
		sum_trans = 0.0;
		sum_flat_potrad = 0.0;
		sum_slope_potrad = 0.0;

		// begin sub-daily hour-angle loop, from -hss to hss 
		for (h=-hss ; h<hss ; h+=dh)
		{
			// precalculate cos and sin of hour angle 
			cosh = cos(h);
			sinh = sin(h);
			
			// calculate cosine of solar zenith angle 
			cza = cosegeom * cosh + sinegeom;
			
			// calculate cosine of beam-slope angle 
			cbsa = sinh * bsg1 + cosh * bsg2 + bsg3;
			
			// check if sun is above a flat horizon 
			if (cza > 0.0) 
			{
				// when sun is above the ideal (flat) horizon, do all the
				//flat-surface calculations to determine daily total
				//transmittance, and save flat-surface potential radiation
				//for later calculations of diffuse radiation 
				
				// potential radiation for this time period, flat surface,
				//top of atmosphere 
				dir_flat_topa = dir_beam_topa * cza;
				
				// determine optical air mass 
				am = 1.0/(cza + 0.0000001);
				if (am > 2.9)
				{
					ami = (int)(acos(cza)/RADPERDEG) - 69;
					if (ami < 0) ami = 0;
					if (ami > 20) ami = 20;
					am = optam[ami];
				}
				
				//correct instantaneous transmittance for this optical
				//air mass 
				trans2 = pow(trans1,am);
				
				//instantaneous transmittance is weighted by potential
				//radiation for flat surface at top of atmosphere to get
				//daily total transmittance 
				sum_trans += trans2 * dir_flat_topa;
				
				//keep track of total potential radiation on a flat
				//surface for ideal horizons 
				sum_flat_potrad += dir_flat_topa;
				
				//keep track of whether this time step contributes to
				//component 1 (direct on slope) 
				if ((h<0.0 && cza>coszeh && cbsa>0.0) ||
					(h>=0.0 && cza>coszwh && cbsa>0.0))
				{
					// sun between east and west horizons, and direct on
					//slope. this period contributes to component 1 
					sum_slope_potrad += dir_beam_topa * cbsa;
				}

			} // end if sun above ideal horizon 
			
		} // end of sub-daily hour-angle loop 
		

		//calculate maximum daily total transmittance and daylight average
		//flux density for a flat surface and the slope 
		if (daylength[i])
		{
			ttmax0[i] = sum_trans / sum_flat_potrad;
			flat_potrad[i] = sum_flat_potrad / daylength[i];
			slope_potrad[i] = sum_slope_potrad / daylength[i];
		}
		else
		{
			ttmax0[i] = 0.0;
			flat_potrad[i] = 0.0;
			slope_potrad[i] = 0.0;
		}
	} // end of i=365 days loop 
	
	// force yearday 366 = yearday 365 
	ttmax0[365] = ttmax0[364];
	flat_potrad[365] = flat_potrad[364];
	slope_potrad[365] = slope_potrad[364];
	daylength[365] = daylength[364];

	// STEP (4)  calculate the sky proportion for diffuse radiation 
	//// uses the product of spherical cap defined by average horizon angle
	//and the great-circle truncation of a hemisphere. this factor does not
	//vary by yearday. 
	avg_horizon = (p->site_ehoriz + p->site_whoriz)/2.0;
	horizon_scalar = 1.0 - sin(avg_horizon * RADPERDEG);
	if (p->site_slp > avg_horizon) slope_excess = p->site_slp - avg_horizon;
	else slope_excess = 0.0;
	if (2.0*avg_horizon > 180.0) slope_scalar = 0.0;
	else
	{
		slope_scalar = 1.0 - (slope_excess/(180.0 - 2.0*avg_horizon));
		if (slope_scalar < 0.0) slope_scalar = 0.0;
	}
	sky_prop = horizon_scalar * slope_scalar;
	
	// STEP (5)  final calculation of daily total radiation 
	for (i=0 ; i<ndays ; i++)
	{
		// correct this day's maximum transmittance for vapor pressure 
		yday = data->yday[i]-1;
		double pva = 610.7 * exp(17.38 * data->s_tdew[i] / (239.0 + data->s_tdew[i]));
		t_tmax = ttmax0[yday] + ABASE * pva;
		
		// b parameter from 30-day average of DTR 
		b = B0 + B1 * exp(-B2 * sm_dtr[i]);
		
		// proportion of daily maximum transmittance 
		t_fmax = 1.0 - 0.9 * exp(-b * pow(dtr[i],C));
		
		// correct for precipitation if this is a rain day 
		if (data->s_prcp[i]) t_fmax *= RAIN_SCALAR;
		
		// final daily total transmittance 
		t_final = t_tmax * t_fmax;
		
		// estimate fraction of radiation that is diffuse, on an
		//instantaneous basis, from relationship with daily total
		//transmittance in Jones (Plants and Microclimate, 1992)
		//Fig 2.8, p. 25, and Gates (Biophysical Ecology, 1980)
		//Fig 6.14, p. 122. 
		pdif = -1.25*t_final + 1.25;
		if (pdif > 1.0) pdif = 1.0;
		if (pdif < 0.0) pdif = 0.0;
		
		// estimate fraction of radiation that is direct, on an
		//instantaneous basis 
		pdir = 1.0 - pdif;
		
		//the daily total radiation is estimated as the sum of the
		//following two components:
		//1. The direct radiation arriving during the part of
		//   the day when there is direct beam on the slope.
		//2. The diffuse radiation arriving over the entire daylength
		//   (when sun is above ideal horizon).
		//
		
		// component 1 (direct) 
		srad1 = slope_potrad[yday] * t_final * pdir;
		
		// component 2 (diffuse) 
		// includes the effect of surface albedo in raising the diffuse
		//radiation for obstructed horizons 
		srad2 = flat_potrad[yday] * t_final * pdif * 
			(sky_prop + DIF_ALB*(1.0-sky_prop)); 
		
		// snow pack influence on radiation 	
		if (data->s_swe[i] > 0.0)
		{
			// snow correction in J/m2/day 
			sc = (1.32 + 0.096 * data->s_swe[i]) * 1e6;
			// convert to W/m2 and check for zero daylength 
			if (daylength[yday] > 0.0) sc /= daylength[yday];
			else sc = 0.0;
			// set a maximum correction of 100 W/m2 
			if (sc > 100.0) sc = 100.0;
		}
		else sc = 0.0;
		
		// save mean daily radiation [W/m²] and daylength [s]
		data->s_srad[i] = srad1 + srad2 + sc;
		data->s_dayl[i] = daylength[yday];
	}

	// free local array memory 
	free(dtr);
	free(sm_dtr);
	
	return (!ok);
} // end of calc_srad_humidity() 



// without Tdew input data, an iterative estimation of shortwave radiation
// and humidity is required 
int CMTClim43::calc_srad_humidity_iterative(const CControl *ctrl,
	const CParameter *p, CData *data)
{
	int ok=1;
	int i,j,ndays;
	int start_yday,end_yday,isloop;
	int ami,yday;
	double ttmax0[366];
	double flat_potrad[366];
	double slope_potrad[366];
	double daylength[366];
	double *dtr, *sm_dtr;
	double *parray, *window, *t_fmax;//, *tdew;
	double *save_pet;
	double sum_prcp,ann_prcp,effann_prcp;
	double sum_pet,ann_pet;
	//double tmax,tmin;
	double t1,t2;
	double pratio;
	double lat,coslat,sinlat,dt,h,dh;
	double cosslp,sinslp,cosasp,sinasp;
	double bsg1,bsg2,bsg3;
	double decl,cosdecl,sindecl,cosegeom,sinegeom,coshss,hss;
	double sc,dir_beam_topa;
	double sum_flat_potrad,sum_slope_potrad,sum_trans;
	double cosh,sinh;
	double cza,cbsa,coszeh,coszwh;
	double dir_flat_topa,am;
	double pva,t_tmax,b;
	double tmink,pet,ratio,ratio2,ratio3,tdewk;
//	double pvs,vpd;
	double trans1,trans2;
	double t_final,pdif,pdir,srad1,srad2; 
	double pa;
	double sky_prop;
	double avg_horizon, slope_excess;
	double horizon_scalar, slope_scalar;

	// optical airmass by degrees 
	double optam[21] = {2.90,3.05,3.21,3.39,3.69,3.82,4.07,4.37,4.72,5.12,5.60,
	6.18,6.88,7.77,8.90,10.39,12.44,15.36,19.79,26.96,30.00};

	// number of simulation days 
	ndays = ctrl->ndays;
	
	// local array memory allocation 
	// allocate space for DTR and smoothed DTR arrays 
	if (!(dtr = (double*) malloc(ndays * sizeof(double))))
	{
		printf("Error allocating for DTR array\n");
		ok=0;
	}
	if (!(sm_dtr = (double*) malloc(ndays * sizeof(double))))
	{
		printf("Error allocating for smoothed DTR array\n");
		ok=0;
	}
	/* allocate space for effective annual precip array */
	if (!(parray = (double*) malloc(ndays * sizeof(double))))
	{
		printf("Error allocating for effective annual precip array\n");
		ok=0;
	}
	/* allocate space for the prcp totaling array */
	if (!(window = (double*) malloc((ndays+90)*sizeof(double))))
	{
		printf("Error allocating for prcp totaling array\n");
		ok = 0;
	}
	/* allocate space for t_fmax */
	if (!(t_fmax = (double*) malloc(ndays * sizeof(double))))
	{
		printf("Error allocating for p_tt_max array\n");
		ok=0;
	}
	/* allocate space for Tdew array */
	//if (!(tdew = (double*) malloc(ndays * sizeof(double))))
	//{
	//	printf("Error allocating for Tdew array\n");
	//	ok=0;
	//}
	/* allocate space for save_pet array */
	if (!(save_pet = (double*) malloc(ndays * sizeof(double))))
	{
		printf("Error allocating for save_pet array\n");
		ok=0;
	}
	
	// calculate diurnal temperature range for transmittance calculations 
	for (i=0 ; i<ndays ; i++)
	{
		//double tmax = data->tmax[i];//old code use base temperature???
		//double tmin = data->tmin[i];
		double tmax = data->s_tmax[i];
		double tmin = data->s_tmin[i];
		if (tmax < tmin) tmax = tmin;
		dtr[i] = tmax-tmin;
	}
	
	/* smooth dtr array: After Bristow and Campbell, 1984 */
	if (ndays >= 30) /* use 30-day antecedent smoothing window */
	{
		if (pulled_boxcar(dtr, sm_dtr, ndays, 30, 0))
		{
			printf("Error in boxcar smoothing, calc_srad_humidity()\n");
			ok=0;
		}
	}
	else /* smoothing window width = ndays */
	{
		if (pulled_boxcar(dtr, sm_dtr, ndays, ndays, 0))
		{
			printf("Error in boxcar smoothing, calc_srad_humidity()\n");
			ok=0;
		}
	}
	
	/* calculate the annual total precip for decision between
	simple and arid-corrected humidity algorithm */
	sum_prcp = 0.0;
	for (i=0 ; i<ndays ; i++)
	{
		sum_prcp += data->s_prcp[i];
	}
	ann_prcp = (sum_prcp/(double)ndays) * 365.25;
	if (ann_prcp == 0.0) ann_prcp = 1.0;
	
	/* Generate the effective annual precip, based on a 3-month
	moving-window. Requires some special case handling for the
	beginning of the record and for short records. */
	/* check if there are at least 90 days in this input file, if not,
	use a simple total scaled to effective annual precip */
	if (ndays < 90)
	{
		sum_prcp = 0.0;
		for (i=0 ; i<ndays ; i++)
		{
			sum_prcp += data->s_prcp[i];
		}
		effann_prcp = (sum_prcp/(double)ndays) * 365.25;
		/* if the effective annual precip for this period
		is less than 8 cm, set the effective annual precip to 8 cm
		to reflect an arid condition, while avoiding possible
		division-by-zero errors and very large ratios (PET/Pann) */
		if (effann_prcp < 8.0) effann_prcp = 8.0;
		for (i=0 ; i<ndays ; i++)
		{
			parray[i] = effann_prcp;
		}
	}
	else
	{
		/* Check if the yeardays at beginning and the end of this input file
		match up. If so, use parts of the three months at the end
		of the input file to generate effective annual precip for
		the first 3-months. Otherwise, duplicate the first 90 days
		of the record. */
		start_yday = data->yday[0];
		end_yday = data->yday[ndays-1];
		if (start_yday != 1)
		{
			isloop = (end_yday == start_yday-1) ? 1 : 0;
		}
		else
		{
			isloop = (end_yday == 365 || end_yday == 366) ? 1 : 0;
		}

		/* fill the first 90 days of window */
		for (i=0 ; i<90 ; i++)
		{
			if (isloop) window[i] = data->s_prcp[ndays-90+i];
			else window[i] = data->s_prcp[i];
		}
		/* fill the rest of the window array */
		for (i=0 ; i<ndays ; i++)
		{
			window[i+90] = data->s_prcp[i];
		}

		/* for each day, calculate the effective annual precip from 
		scaled 90-day total */
		for (i=0 ; i<ndays ; i++)
		{
			sum_prcp = 0.0;
			for (j=0 ; j<90 ; j++)
			{
				sum_prcp += window[i+j];
			}
			sum_prcp = (sum_prcp/90.0) * 365.25;
			/* if the effective annual precip for this 90-day period
			is less than 8 cm, set the effective annual precip to 8 cm
			to reflect an arid condition, while avoiding possible
			division-by-zero errors and very large ratios (PET/Pann) */
			parray[i] = (sum_prcp < 8.0) ? 8.0 : sum_prcp;
		}
	} /* end if ndays >= 90 */	
	
	/*****************************************
	 *                                       *
	 * start of the main radiation algorithm *
	 *                                       *
	 *****************************************/
	 
	/* before starting the iterative algorithm between humidity and 
	radiation, calculate all the variables that don't depend on 
	humidity so they only get done once. */
	
	// STEP (1) calculate pressure ratio (site/reference) = f(elevation) 
	t1 = 1.0 - (LR_STD * p->site_elev)/T_STD;
	t2 = G_STD / (LR_STD * (R/MA));
	pratio = pow(t1,t2);
	
	// STEP (2) correct initial transmittance for elevation 
	trans1 = pow(TBASE,pratio);
	
	// STEP (3) build 366-day array of ttmax0, potential rad, and daylength 

	// precalculate the transcendentals 
	lat = p->site_lat;
	// check for (+/-) 90 degrees latitude, throws off daylength calc 
	lat *= RADPERDEG;
	if (lat > 1.5707) lat = 1.5707;
	if (lat < -1.5707) lat = -1.5707;
	coslat = cos(lat);
	sinlat = sin(lat);
	cosslp = cos(p->site_slp * RADPERDEG);
	sinslp = sin(p->site_slp * RADPERDEG);
	cosasp = cos(p->site_asp * RADPERDEG);
	sinasp = sin(p->site_asp * RADPERDEG);
	// cosine of zenith angle for east and west horizons 
	coszeh = cos(1.570796 - (p->site_ehoriz * RADPERDEG));
	coszwh = cos(1.570796 - (p->site_whoriz * RADPERDEG));
	
	/* sub-daily time and angular increment information */
	dt = SRADDT;                /* set timestep */ 
	dh = dt / SECPERRAD;        /* calculate hour-angle step */
	
	/* begin loop through yeardays */
	for (i=0 ; i<365 ; i++)
	{
		/* calculate cos and sin of declination */
		decl = MINDECL * cos(((double)i + DAYSOFF) * RADPERDAY);
		cosdecl = cos(decl);
		sindecl = sin(decl);
		
		/* do some precalculations for beam-slope geometry (bsg) */
		bsg1 = -sinslp * sinasp * cosdecl;
		bsg2 = (-cosasp * sinslp * sinlat + cosslp * coslat) * cosdecl;
		bsg3 = (cosasp * sinslp * coslat + cosslp * sinlat) * sindecl;
		
		/* calculate daylength as a function of lat and decl */
		cosegeom = coslat * cosdecl;
		sinegeom = sinlat * sindecl;
		coshss = -(sinegeom) / cosegeom;
		if (coshss < -1.0) coshss = -1.0;  /* 24-hr daylight */
		if (coshss > 1.0) coshss = 1.0;    /* 0-hr daylight */
		hss = acos(coshss);                /* hour angle at sunset (radians) */
		/* daylength (seconds) */
		daylength[i] = 2.0 * hss * SECPERRAD;

		/* solar constant as a function of yearday (W/m^2) */
		sc = 1368.0 + 45.5*sin((2.0*PI*(double)i/365.25) + 1.7);
		/* extraterrestrial radiation perpendicular to beam, total over
		the timestep (J) */
		dir_beam_topa = sc * dt;
		
		sum_trans = 0.0;
		sum_flat_potrad = 0.0;
		sum_slope_potrad = 0.0;

		/* begin sub-daily hour-angle loop, from -hss to hss */
		for (h=-hss ; h<hss ; h+=dh)
		{
			/* precalculate cos and sin of hour angle */
			cosh = cos(h);
			sinh = sin(h);
			
			/* calculate cosine of solar zenith angle */
			cza = cosegeom * cosh + sinegeom;
			
			/* calculate cosine of beam-slope angle */
			cbsa = sinh * bsg1 + cosh * bsg2 + bsg3;
			
			/* check if sun is above a flat horizon */
			if (cza > 0.0) 
			{
				/* when sun is above the ideal (flat) horizon, do all the
				flat-surface calculations to determine daily total
				transmittance, and save flat-surface potential radiation
				for later calculaxtions of diffuse radiation */
				
				/* potential radiation for this time period, flat surface,
				top of atmosphere */
				dir_flat_topa = dir_beam_topa * cza;
				
				/* determine optical air mass */
				am = 1.0/(cza + 0.0000001);
				if (am > 2.9)
				{
					ami = (int)(acos(cza)/RADPERDEG) - 69;
					if (ami < 0) ami = 0;
					if (ami > 20) ami = 20;
					am = optam[ami];
				}
				
				/* correct instantaneous transmittance for this optical
				air mass */
				trans2 = pow(trans1,am);
				
				/* instantaneous transmittance is weighted by potential
				radiation for flat surface at top of atmosphere to get
				daily total transmittance */
				sum_trans += trans2 * dir_flat_topa;
				
				/* keep track of total potential radiation on a flat
				surface for ideal horizons */
				sum_flat_potrad += dir_flat_topa;
				
				/* keep track of whether this time step contributes to
				component 1 (direct on slope) */
				if ((h<0.0 && cza>coszeh && cbsa>0.0) ||
					(h>=0.0 && cza>coszwh && cbsa>0.0))
				{
					/* sun between east and west horizons, and direct on
					slope. this period contributes to component 1 */
					sum_slope_potrad += dir_beam_topa * cbsa;
				}

			} /* end if sun above ideal horizon */
			
		} /* end of sub-daily hour-angle loop */
		
		/* calculate maximum daily total transmittance and daylight average
		flux density for a flat surface and the slope */
		//if (daylength[i]) by RSA 10/02/2004
		if (daylength[i] && sum_flat_potrad)
		{
			ttmax0[i] = sum_trans / sum_flat_potrad;
			flat_potrad[i] = sum_flat_potrad / daylength[i];
			slope_potrad[i] = sum_slope_potrad / daylength[i];
		}
		else
		{
			ttmax0[i] = 0.0;
			flat_potrad[i] = 0.0;
			slope_potrad[i] = 0.0;
		}
	} /* end of i=365 days loop */
	
	/* force yearday 366 = yearday 365 */
	ttmax0[365] = ttmax0[364];
	flat_potrad[365] = flat_potrad[364];
	slope_potrad[365] = slope_potrad[364];
	daylength[365] = daylength[364];

	/* STEP (4)  calculate the sky proportion for diffuse radiation */
	/* uses the product of spherical cap defined by average horizon angle
	and the great-circle truncation of a hemisphere. this factor does not
	vary by yearday. */
	avg_horizon = (p->site_ehoriz + p->site_whoriz)/2.0;
	horizon_scalar = 1.0 - sin(avg_horizon * RADPERDEG);
	if (p->site_slp > avg_horizon) slope_excess = p->site_slp - avg_horizon;
	else slope_excess = 0.0;
	if (2.0*avg_horizon > 180.0) slope_scalar = 0.0;
	else
	{
		slope_scalar = 1.0 - (slope_excess/(180.0 - 2.0*avg_horizon));
		if (slope_scalar < 0.0) slope_scalar = 0.0;
	}
	sky_prop = horizon_scalar * slope_scalar;
	
	/* b parameter, and t_fmax not varying with Tdew, so these can be
	calculated once, outside the iteration between radiation and humidity
	estimates. Requires storing t_fmax in an array. */
	for (i=0 ; i<ndays ; i++)
	{	
		/* b parameter from 30-day average of DTR */
		b = B0 + B1 * exp(-B2 * sm_dtr[i]);
		
		/* proportion of daily maximum transmittance */
		t_fmax[i] = 1.0 - 0.9 * exp(-b * pow(dtr[i],C));

		// correct for precipitation if this is a rain day 
		//if (data->prcp[i]) t_fmax[i] *= RAIN_SCALAR;//old code use prcp
		if (data->s_prcp[i]) t_fmax[i] *= RAIN_SCALAR;
		
	}
	
	/* As a first approximation, calculate radiation assuming
	that Tdew = Tmin */
	for (i=0 ; i<ndays ; i++)
	{
		yday = data->yday[i]-1;
		data->s_tdew[i] = data->s_tmin[i];
		pva = 610.7 * exp(17.38 * data->s_tdew[i] / (239.0 + data->s_tdew[i]));
		t_tmax = ttmax0[yday] + ABASE * pva;
		
		/* final daily total transmittance */
		t_final = t_tmax * t_fmax[i];
		
		/* estimate fraction of radiation that is diffuse, on an
		instantaneous basis, from relationship with daily total
		transmittance in Jones (Plants and Microclimate, 1992)
		Fig 2.8, p. 25, and Gates (Biophysical Ecology, 1980)
		Fig 6.14, p. 122. */
		pdif = -1.25*t_final + 1.25;
		if (pdif > 1.0) pdif = 1.0;
		if (pdif < 0.0) pdif = 0.0;
		
		/* estimate fraction of radiation that is direct, on an
		instantaneous basis */
		pdir = 1.0 - pdif;
		
		/* the daily total radiation is estimated as the sum of the
		following two components:
		1. The direct radiation arriving during the part of
		   the day when there is direct beam on the slope.
		2. The diffuse radiation arriving over the entire daylength
		   (when sun is above ideal horizon).
		*/
		
		/* component 1 */
		srad1 = slope_potrad[yday] * t_final * pdir;
		
		/* component 2 (diffuse) */
		/* includes the effect of surface albedo in raising the diffuse
		radiation for obstructed horizons */
		srad2 = flat_potrad[yday] * t_final * pdif * 
			(sky_prop + DIF_ALB*(1.0-sky_prop)); 
		
		/* snow pack influence on radiation */	
		if (data->s_swe[i] > 0.0)
		{
			/* snow correction in J/m2/day */
			sc = (1.32 + 0.096 * data->s_swe[i]) * 1e6;
			/* convert to W/m2 and check for zero daylength */
			if (daylength[yday] > 0.0) sc /= daylength[yday];
			else sc = 0.0;
			/* set a maximum correction of 100 W/m2 */
			if (sc > 100.0) sc = 100.0;
		}
		else sc = 0.0;
		
		// save mean daily radiation [W/m²] and daylength [s]
		data->s_srad[i] = srad1 + srad2 + sc;
		data->s_dayl[i] = daylength[yday];
	}
	
	/* estimate annual PET first, to decide which humidity algorithm 
	should be used */
	/* estimate air pressure at site */
	pa = atm_pres(p->site_elev);
	sum_pet = 0.0;
	for (i=0 ; i<ndays ; i++)
	{
		int yday = data->yday[i] - 1;
		double dayl = daylength[yday];
		save_pet[i] = calc_pet(data->s_srad[i], data->s_tday[i], pa, dayl);// data->s_dayl[i]);
		sum_pet += save_pet[i];
	}
	ann_pet = (sum_pet/(double)ndays) * 365.25;
	
	/* humidity algorithm decision: 
	PET/prcp >= 2.5 -> arid correction
	PET/prcp <  2.5 -> use tdew-tmin, which is already finished */
	printf("PET/PRCP = %.4lf\n",ann_pet/ann_prcp);
	
	if (ann_pet/ann_prcp >= 2.5) 
	{
		printf("Using arid-climate humidity algorithm\n");
		/* Estimate Tdew using the initial estimate of radiation for PET */
		for (i=0 ; i<ndays ; i++)
		{
			tmink = data->s_tmin[i] + 273.15;
			pet = save_pet[i];

			// calculate ratio (PET/effann_prcp) and correct the dewpoint 
			ratio = pet/parray[i];
			ratio2 = ratio*ratio;
			ratio3 = ratio2*ratio;
			tdewk = tmink*(-0.127 + 1.121*(1.003 - 1.444*ratio + 12.312*ratio2 
				- 32.766*ratio3) + 0.0006*(dtr[i]));
			data->s_tdew[i] = tdewk - 273.15;
		}

		/* Revise estimate of radiation using new Tdew */
		for (i=0 ; i<ndays ; i++)
		{
			yday = data->yday[i]-1;
			pva = 610.7 * exp(17.38 * data->s_tdew[i] / (239.0 + data->s_tdew[i]));
			t_tmax = ttmax0[yday] + ABASE * pva;

			/* final daily total transmittance */
			t_final = t_tmax * t_fmax[i];

			/* estimate fraction of radiation that is diffuse, on an
			instantaneous basis, from relationship with daily total
			transmittance in Jones (Plants and Microclimate, 1992)
			Fig 2.8, p. 25, and Gates (Biophysical Ecology, 1980)
			Fig 6.14, p. 122. */
			pdif = -1.25*t_final + 1.25;
			if (pdif > 1.0) pdif = 1.0;
			if (pdif < 0.0) pdif = 0.0;

			/* estimate fraction of radiation that is direct, on an
			instantaneous basis */
			pdir = 1.0 - pdif;

			/* the daily total radiation is estimated as the sum of the
			following two components:
			1. The direct radiation arriving during the part of
			   the day when there is direct beam on the slope.
			2. The diffuse radiation arriving over the entire daylength
			   (when sun is above ideal horizon).
			*/

			/* component 1 */
			srad1 = slope_potrad[yday] * t_final * pdir;

			/* component 2 (diffuse) */
			/* includes the effect of surface albedo in raising the diffuse
			radiation for obstructed horizons */
			srad2 = flat_potrad[yday] * t_final * pdif * 
				(sky_prop + DIF_ALB*(1.0-sky_prop)); 

			/* snow pack influence on radiation */	
			if (data->s_swe[i] > 0.0)
			{
				/* snow correction in J/m2/day */
				sc = (1.32 + 0.096 * data->s_swe[i]) * 1e6;
				/* convert to W/m2 and check for zero daylength */
				if (daylength[yday] > 0.0) sc /= daylength[yday];
				else sc = 0.0;
				/* set a maximum correction of 100 W/m2 */
				if (sc > 100.0) sc = 100.0;
			}
			else sc = 0.0;

			/* save daily radiation */
			data->s_srad[i] = srad1 + srad2 + sc;
		}

		/* Revise estimate of Tdew using new radiation */
		for (i=0 ; i<ndays ; i++)
		{
			int yday = data->yday[i] - 1;
			double dayl = daylength[yday];
			tmink = data->s_tmin[i] + 273.15;
			pet = calc_pet(data->s_srad[i], data->s_tday[i], pa, dayl);// data->s_dayl[i]);

			/* calculate ratio (PET/effann_prcp) and correct the dewpoint */
			ratio = pet/parray[i];
			ratio2 = ratio*ratio;
			ratio3 = ratio2*ratio;
			tdewk = tmink*(-0.127 + 1.121*(1.003 - 1.444*ratio + 12.312*ratio2 
				- 32.766*ratio3) + 0.0006*(dtr[i]));
			data->s_tdew[i] = tdewk - 273.15;
		}
	} /* end of arid-correction humidity and radiation estimation */
	else
	{
		printf("Using Tdew=Tmin humidity algorithm\n");
	}
	
	/* now calculate vapor pressure from tdew */
	//for (i=0 ; i<ndays ; i++)
	//{
		//data->s_Ea[i] = 610.7 * exp(17.38 * data->s_tdew[i] / (239.0 + data->s_tdew[i]));
		//if (ctrl->outhum)
		//{
			/* output humidity as vapor pressure (Pa) */
		//	 = pva;
		//}
		//else
		//{
			/* output humidity as vapor pressure deficit (Pa) */
			/* calculate saturated VP at tday */
			//data->s_Es[i] = 610.7 * exp(17.38 * data->s_tday[i]/(239.0+data->s_tday[i]));
			//vpd = pvs - pva;
			//if (vpd < 0.0) vpd = 0.0;
			//data->s_hum[i] = vpd;
		//}
	//} /* end for i = ndays loop */
	
	/* free local array memory */
	free(dtr);
	free(sm_dtr);
	free(parray);
	free(window);
	free(t_fmax);
//	free(tdew);
	free(save_pet);
	
	return (!ok);
} /* end of calc_srad_humidity_iterative() */
	
			
/* calc_pet() calculates the potential evapotranspiration for aridity 
corrections in calc_vpd(), according to Kimball et al., 1997 */
double CMTClim43::calc_pet(double rad, double ta, double pa, double dayl)
{
	/* input parameters and units :
	double rad      (W/m2)  daylight average incident shortwave radiation
	double ta       (deg C) daylight average air temperature
	double pa       (Pa)    air pressure
	double dayl     (s)     daylength 
	*/
	
	double rnet;       /* (W m-2) absorbed shortwave radiation avail. for ET */
	double lhvap;      /* (J kg-1) latent heat of vaporization of water */ 
	double gamma;      /* (Pa K-1) psychrometer parameter */
	double dt = 0.2;   /* offset for saturation vapor pressure calculation */
	double t1, t2;     /* (deg C) air temperatures */
	double pvs1, pvs2; /* (Pa)   saturated vapor pressures */
	double pet;        /* (kg m-2 day-1) potential evapotranspiration */
	double s;          /* (Pa K-1) slope of saturated vapor pressure curve */

	/* calculate absorbed radiation, assuming albedo = 0.2  and ground
	heat flux = 10% of absorbed radiation during daylight */
	rnet = rad * 0.72;
		
    /* calculate latent heat of vaporization as a function of ta */
    lhvap = 2.5023e6 - 2430.54 * ta;
    
    /* calculate the psychrometer parameter: gamma = (cp pa)/(lhvap epsilon)
    where:
    cp       (J/kg K)   specific heat of air
    epsilon  (unitless) ratio of molecular weights of water and air
    */
    gamma = CP * pa / (lhvap * EPS);
    
    /* estimate the slope of the saturation vapor pressure curve at ta */
    /* temperature offsets for slope estimate */
    t1 = ta+dt;
    t2 = ta-dt;
    
    /* calculate saturation vapor pressures at t1 and t2, using formula from 
	Abbott, P.F., and R.C. Tabony, 1985. The estimation of humidity parameters.
	Meteorol. Mag., 114:49-56.
	*/
    pvs1 = 610.7 * exp(17.38 * t1 / (239.0 + t1));
    pvs2 = 610.7 * exp(17.38 * t2 / (239.0 + t2));

    /* calculate slope of pvs vs. T curve near ta */
    s = (pvs1-pvs2) / (t1-t2);
    
    /* calculate PET using Priestly-Taylor approximation, with coefficient
    set at 1.26. Units of result are kg/m^2/day, equivalent to mm water/day */
	pet = (1.26 * (s/(s+gamma)) * rnet * dayl)/lhvap;
	
	/* return a value in centimeters/day, because this value is used in a ratio
	to annual total precip, and precip units are centimeters */
	return (pet/10.0);
}    
		
/* atm_pres() calculates the atmospheric pressure as a function of elevation */
double CMTClim43::atm_pres(double elev)
{
	/* daily atmospheric pressure (Pa) as a function of elevation (m) */
	/* From the discussion on atmospheric statics in:
	Iribane, J.V., and W.L. Godson, 1981. Atmospheric Thermodynamics, 2nd
		Edition. D. Reidel Publishing Company, Dordrecht, The Netherlands.
		(p. 168)
	*/
	
	int ok=1;
	double t1,t2;
	double pa;
	
	t1 = 1.0 - (LR_STD * elev)/T_STD;
	t2 = G_STD / (LR_STD * (R / MA));
	pa = P_STD * pow(t1,t2);
	
	return(pa);
}

/* pulled_boxcar() calculates a moving average of antecedent values in an
array, using either a ramped (w_flag=1) or a flat (w_flag=0) weighting */	
int CMTClim43::pulled_boxcar(double *input,double *output,int n,int w,int w_flag)
{
	int ok=1;
    int i,j;
    double *wt;
    double total,sum_wt;

    if (w > n) {
        printf("Boxcar longer than array...\n");
        printf("Resize boxcar and try again\n");
        ok=0;
    }
    
    if (ok && !(wt = (double*) malloc(w * sizeof(double))))
    {
    	printf("Allocation error in boxcar()\n");
    	ok=0;
    }
    
    if (ok)
    {
	    /* when w_flag != 0, use linear ramp to weight tails,
	    otherwise use constant weight */
	    sum_wt = 0.0;
	    if (w_flag)
	    {
	        for (i=0 ; i<w ; i++)
	       	{
	            wt[i] = (double)(i+1);
	            sum_wt += wt[i];
	        }
	    }
	    else
	    {
	        for (i=0 ; i<w ; i++)
	        { 	
	            wt[i] = 1.0;
	            sum_wt += wt[i];
	        }
	    }
	    
	    /* fill the output array, starting with the point where a full
	    boxcar can be calculated */
	    for (i=w-1 ; i<n ; i++)
	    {
	        total = 0.0;
	        for (j=0 ; j<w ; j++)
	        {
	            total += input[i-w+j+1] * wt[j];
	        }
	        output[i] = total/sum_wt;
	    }
	    
	    /* fill the first w elements of the output array with the value from
	    the first full boxcar */
	    for (i=0 ; i<w-1 ; i++)
	    {
	    	output[i] = output[w-1];
	    }
	    
	    free(wt);
	    
	} /* end if ok */
	
    return (!ok);
}
/* end of pulled_boxcar() */  



//**********************************************************
// CData

// parameters for the Tair algorithm 
const double  CData::TDAYCOEF     =0.45;  // (dim) daylight air temperature coefficient (dim) 

// parameters for the snowpack algorithm 
const double  CData::SNOW_TCRIT   =-6.0;  // (deg C) critical temperature for snowmelt    
const double  CData::SNOW_TRATE  =0.042;  // (cm/degC/day) snowmelt rate 



// data_alloc() allocates space for input and output data arrays */
int CData::data_alloc(int ndays)
{
	int ok=1;
	
	m_nbDays = ndays;
	//if (ok && !(year = (int*) malloc(ndays * sizeof(int))))
	//{
	//	printf("Error allocating for year array\n");
	//	ok=0;
	//} 
	if (ok && !(yday = (int*) malloc(ndays * sizeof(int))))
	{
		printf("Error allocating for yearday array\n");
		ok=0;
	} 
	/*if (ok && !(tmax = (double*) malloc(ndays * sizeof(double))))
	{
		printf("Error allocating for tmax array\n");
		ok=0;
	}
	if (ok && !(tmin = (double*) malloc(ndays * sizeof(double))))
	{
		printf("Error allocating for tmin array\n");
		ok=0;
	}
	if (ok && !(prcp = (double*) malloc(ndays * sizeof(double))))
	{
		printf("Error allocating for prcp array\n");
		ok=0;
	}*/
	/*if (ok && ctrl->indewpt && !(tdew = (double*) malloc(ndays *
		sizeof(double))))
	{
		printf("Error allocating for input humidity array\n");
		ok=0;
	}*/

	if (ok && !(s_tmax = (double*) malloc(ndays * sizeof(double))))
	{
		printf("Error allocating for site Tmax array\n");
		ok=0;
	}
	if (ok && !(s_tmin = (double*) malloc(ndays * sizeof(double))))
	{
		printf("Error allocating for site Tmin array\n");
		ok=0;
	}
	if (ok && !(s_tday = (double*) malloc(ndays * sizeof(double))))
	{
		printf("Error allocating for site Tday array\n");
		ok=0;
	}
	if (ok && !(s_prcp = (double*) malloc(ndays * sizeof(double))))
	{
		printf("Error allocating for site prcp array\n");
		ok=0;
	}
	/*if (ok && !(s_Es = (double*) malloc(ndays * sizeof(double))))
	{
		printf("Error allocating for site VPD array\n");
		ok=0;
	}
	if (ok && !(s_Ea = (double*) malloc(ndays * sizeof(double))))
	{
		printf("Error allocating for site VPD array\n");
		ok=0;
	}*/

	if (ok && !(s_srad = (double*) malloc(ndays * sizeof(double))))
	{
		printf("Error allocating for site radiation array\n");
		ok=0;
	}
	if (ok && !(s_dayl = (double*) malloc(ndays * sizeof(double))))
	{
		printf("Error allocating for site daylength array\n");
		ok=0;
	}
	if (ok && !(s_swe = (double*) malloc(ndays * sizeof(double))))
	{
		printf("Error allocating for site snowpack array\n");
		ok=0;
	}
	// allocate space for Tdew array 
	if (ok && !(s_tdew = (double*) malloc(ndays * sizeof(double))))
	{
		printf("Error allocating for Tdew array\n");
		ok=0;
	}

	return (!ok);
}
// end of data_alloc()
// data_free frees the memory previously allocated by data_alloc() 
void CData::data_free()
{
//	if (year) free(year); year=NULL;
	if (yday) free(yday); yday=NULL;
	/*free(tmax);
	free(tmin);
	free(prcp);
	if (ctrl->indewpt) free(tdew);*/

	if (s_tmax) free(s_tmax); s_tmax=NULL;	
	if (s_tmin) free(s_tmin); s_tmin=NULL;	
	if (s_tday) free(s_tday); s_tday=NULL;	
	if (s_prcp) free(s_prcp); s_prcp=NULL;	
	//if (s_Es) free(s_Es); s_Es=NULL;	
	//if (s_Ea) free(s_Ea); s_Ea=NULL;	
	if (s_srad) free(s_srad); s_srad=NULL;	
	if (s_dayl) free(s_dayl); s_dayl=NULL;	
	if (s_swe) free(s_swe); s_swe=NULL;	
	if (s_tdew) free(s_tdew); s_tdew=NULL;	
	
	
}
void CData::SetData(float* Tdew, float* simmin, float* simmax, float* simprec)
{
	for (int i=0 ; i<m_nbDays; i++)
	{
		yday[i] = i+1;
		s_tdew[i] = Tdew[i];
		s_tmax[i] = simmax[i];
		s_tmin[i] = simmin[i];
		s_prcp[i] = simprec[i]/10.0;//ppt in cm
		
		// derived temperatures 
		double tmean = (simmax[i] + simmin[i])/2.0;
		s_tday[i] = ((simmax[i] - tmean)*TDAYCOEF) + tmean;
	}

	//compute snowpack
	snowpack();
}
void CData::SetData(float* simmin, float* simmax, float* simprec)
{
	for (int i=0 ; i<m_nbDays; i++)
	{
		yday[i] = i+1;
		s_tdew[i] = 0;
		s_tmax[i] = simmax[i];
		s_tmin[i] = simmin[i];
		s_prcp[i] = simprec[i]/10.0;//ppt in cm
		
		
		// derived temperatures 
		double tmean = (simmax[i] + simmin[i])/2.0;
		s_tday[i] = ((simmax[i] - tmean)*TDAYCOEF) + tmean;
	}

	//compute snowpack
	snowpack();
}

int CData::snowpack()
{
	int ok=1;
	int count;
	int start_yday,prev_yday;
	double snowpack,newsnow,snowmelt,sum;
	
	// first pass to initialize SWE array 
	snowpack = 0.0;
	for (int i=0 ; i<m_nbDays; i++)
	{
		newsnow = 0.0;
		snowmelt = 0.0;
		if (s_tmin[i] <= SNOW_TCRIT) newsnow = s_prcp[i];
		else snowmelt = SNOW_TRATE * (s_tmin[i] - SNOW_TCRIT);
		snowpack += newsnow - snowmelt;
		if (snowpack < 0.0) snowpack = 0.0;
		s_swe[i] = snowpack;
	}
	
	/* use the first pass to set the initial snowpack conditions for the
	first day of data */
	start_yday = yday[0];
	if (start_yday == 1) prev_yday = 365;
	else prev_yday = start_yday-1;
	count = 0;
	sum = 0.0;
	for (int i=1 ; i<m_nbDays; i++)
	{
		if (yday[i] == start_yday || yday[i] == prev_yday)
		{
			count ++;
			sum += s_swe[i];
		}
	}
	/* Proceed with correction if there are valid days to reinitialize
	the snowpack estiamtes. Otherwise use the first-pass estimate. */
	if (count)
	{
		snowpack = sum/(double)count;
		for (int i=0 ; i<m_nbDays; i++)
		{
			newsnow = 0.0;
			snowmelt = 0.0;
			if (s_tmin[i] <= SNOW_TCRIT) newsnow = s_prcp[i];
			else snowmelt = SNOW_TRATE * (s_tmin[i] - SNOW_TCRIT);
			snowpack += newsnow - snowmelt;
			if (snowpack < 0.0) snowpack = 0.0;
			s_swe[i] = snowpack;
		}
	}
		
	return (!ok);
}
// end of snowpack() 