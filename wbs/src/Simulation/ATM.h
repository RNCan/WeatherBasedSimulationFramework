﻿//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include <string>
#include <array>
#include <boost\multi_array.hpp>
#include <deque>
#include <mutex>

#include "Basic/Statistic.h"
#include "Basic/GeoBasic.h"
#include "Basic/UtilTime.h"
#include "Basic/UtilMath.h"
#include "Basic/Location.h"
#include "Basic/WeatherStation.h"
#include "Basic/HourlyDatabase.h"
#include "Basic/ModelStat.h"
#include "Basic/WaterTemperature.h"
#include "Geomatic/IWD.h"
#include "Geomatic/GDALBasic.h"
#include "Geomatic/ProjectionTransformation.h"
#include "Geomatic/TimeZones.h"

extern const char* WBSF_ATM_VERSION;


namespace WBSF
{

	

	class CATMWorld;

	typedef std::pair<__int64, __int64> CTimePeriod;

	extern const char ATM_HEADER[];//ATM_W_ASCENT
	enum TATMOuput{ ATM_FLIGHT, ATM_AGE, ATM_SEX, ATM_A, ATM_M, ATM_EGGS, ATM_G, ATM_F, ATM_STATE, ATM_FLAG, ATM_X, ATM_Y, ATM_LAT, ATM_LON, ATM_T, ATM_P, ATM_U, ATM_V, ATM_W, ATM_GROUND_ALT, ATM_MEAN_HEIGHT, ATM_CURRENT_HEIGHT, ATM_DELTA_HEIGHT, ATM_MOTH_SPEED, ATM_W_HORIZONTAL, ATM_W_VERTICAL, ATM_DIRECTION, ATM_DISTANCE, ATM_DISTANCE_FROM_OIRIGINE, ATM_FLIGHT_TIME, ATM_LIFTOFF_TIME, ATM_LANDING_TIME, ATM_LIFTOFF_T, ATM_LANDING_T, ATM_DEFOLIATION, NB_ATM_OUTPUT };
	typedef CModelStatVectorTemplate<NB_ATM_OUTPUT, ATM_HEADER> ATMOutput;
	//typedef std::vector<std::vector<std::vector<ATMOutput>>> CATMOutputMatrix;
	typedef std::vector<ATMOutput> CATMOutputMatrix;

	//                     ᵒC        hPa      kg/m²     m/s        m/s      m/s        ᵒC
	enum TATMVariable { ATM_TAIR, ATM_PRES, ATM_PRCP, ATM_WNDU, ATM_WNDV, ATM_WNDW, ATM_WATER, NB_ATM_VARIABLES };
	//extra gribs variable             Pa/s               m        %
	enum TExtraVariable { ATM_VVEL = NB_ATM_VARIABLES, ATM_HGT, ATM_RH, NB_ATM_VARIABLES_EX };

	size_t Hourly2ATM(size_t vv);
	


	inline size_t GetHourlySeconds(size_t time)
	{
		return fmod(time, 3600);
	}




	//*******************************************************************************
	//
	class CWindStability
	{
	public:

		//http ://en.wikipedia.org/wiki/Air_pollution_dispersion_terminology#Characterization_of_atmospheric_turbulence
		//Table 1 : The Pasquill stability classes
		//Stability class	Definition	 	Stability class	Definition
		//A:	very unstable	 	
		//B:	unstable	 	
		//C:	slightly unstable
		//D:	neutral
		//E:	slightly stable
		//F:	stable

		//Table 2: Meteorological conditions that define the Pasquill stability classes
		//	Surface windspeed	Daytime incoming solar radiation	Nighttime cloud cover
		//	m/s					Strong		Moderate	Slight		> 50 %			< 50 %
		//	< 2					A			A – B		B			E				F
		//	2 – 3				A – B		B			C			E				F
		//	3 – 5				B			B – C		C			D				E
		//	5 – 6				C			C – D		D			D				D
		//	> 6					C			D			D			D				D
		//Note : Class D applies to heavily overcast skies, at any windspeed day or night


		//from: http://en.wikipedia.org/wiki/Wind_gradient
		//Unstable air above open water surface : 0.06
		//Neutral air above open water surface : 0.10
		//Stable air above open water surface : 0.27

		//Unstable air above flat open coast : 0.11
		//Neutral air above flat open coast : 0.16
		//Stable air above flat open coast : 0.40

		//Unstable air above human inhabited areas : 0.27
		//Neutral air above human inhabited areas : 0.34
		//Stable air above human inhabited areas : 0.60
		enum TStability{ UNSTABLE, NEUTRAL, STABLE, AUTO_DETECT, NB_STABILITY };
		//static const char* STABILITY_NAME[NB_STABILITY] =  { "Unstable", "Neutral", "Stable" };
		static std::string GetWindStabilityName(int stabType);

		//Hellmann exponent
		static double GetAlpha(int stabType, bool bOverWater)
		{
			double α = 0;

			switch (stabType)
			{
			case UNSTABLE: α = bOverWater ? 0.11 : 0.27; break;
			case NEUTRAL:  α = bOverWater ? 0.16 : 0.34; break;
			case STABLE:   α = bOverWater ? 0.40 : 0.60; break;
			default: ASSERT(false);
			}

			return α;
		}
	};

	//*******************************************************************************
	//
	class CSBWMothParameters
	{
	public:
		
		enum TSex{ MALE, FEMALE, NB_SEX};

		enum TMember{ P_MAX, W_MIN, WING_BEAT_SCALE, REDUCTION_FACTORE, REDUCTION_HEIGHT, W_HORZ, W_HORZ_SD, W_DESCENT, W_DESCENT_SD, FLIGHT_AFTER_SUNRISE, MAXIMUM_FLIGHTS, APPPLY_ATTRITION, MAX_ADULT_LONGEVITY, READY_SHIFT0,  READY_SHIFT1, NB_ATM_MEMBERS };
		static const char* GetMemberName(int i){ ASSERT(i >= 0 && i < NB_ATM_MEMBERS); return MEMBERS_NAME[i]; }

		double m_Pmax;				//maximum precipitation for flight [mm/h]
		double m_Wmin;				//minimum wind speed for flight [km/h]
		double m_w_α;				//vertical factor [km/(h•Hz)]
		double m_Δv;				//proportional reduction from physiological maximum wingbeat frequency
		double m_Hv;				//flight altitude to apply Δv [m]
		double m_w_horizontal;		//horizontal velocity [km/h]
		double m_w_horizontal_σ;	//horizontal velocity standard deviation [km/h]
		double m_w_descent;			//terminal velocity [km/h]
		double m_w_descent_σ;		//terminal velocity standard deviation [km/h]
		
		double m_flight_after_sunrise;	//let moths to flight after sunrise [h]
		size_t m_maxFlights;			//macimum number of flight for moths
		bool   m_bAppplyAttrition;  //apply attrition on adult
		size_t m_max_adult_longevity; //maximum adult longivity [days]

		std::array<double, NB_SEX> m_ready_to_fly;	//minimum age to fly. Usefull to delay male
		
		CSBWMothParameters()
		{
			clear();
		}

		void clear()
		{
			//default parameters for spruce budworm
			m_Pmax = 2.5;				//[mm/h]
			m_Wmin = 0.7 * 3600 / 1000;	//[km/h]
			m_w_α = 0.4;							//scale factor [km/(h•Hz)]
			m_Δv = 1.0;								//proportional reduction from physiological maximum wingbeat frequency
			m_Hv = 60;								//flight height to apply Δv [m]
			m_w_horizontal = 2.0 * 3600 / 1000;		//horizontal mean speed [km/h]
			m_w_horizontal_σ = 0.5 * 3600 / 1000;	//horizontal speed standard deviation [km/h] 
			m_w_descent = -2.0 * 3600 / 1000;		//descent mean speed [km/h]
			m_w_descent_σ = 1.0 * 3600 / 1000;		//descent speed standard deviation [km/h]

			m_flight_after_sunrise = 0;		//let moths to flight after sunrize [h]
			m_maxFlights = 1;
			m_bAppplyAttrition = true;  //apply attrition on adult
			m_max_adult_longevity = 12; //maximum adult longivity [days]

			m_ready_to_fly = { {0.15,0} };

		}

		CSBWMothParameters& operator =(const CSBWMothParameters& in)
		{
			if (&in != this)
			{
				m_Pmax = in.m_Pmax;
				m_Wmin = in.m_Wmin;
				m_w_α = in.m_w_α;
				m_Δv = in.m_Δv;
				m_Hv = in.m_Hv;
				m_w_horizontal = in.m_w_horizontal;
				m_w_horizontal_σ = in.m_w_horizontal_σ;
				m_w_descent = in.m_w_descent;
				m_w_descent_σ = in.m_w_descent_σ;

				m_flight_after_sunrise = in.m_flight_after_sunrise;
				m_maxFlights = in.m_maxFlights;
				m_bAppplyAttrition = in.m_bAppplyAttrition;  
				m_max_adult_longevity = in.m_max_adult_longevity;

				m_ready_to_fly = in.m_ready_to_fly;
								
			}

			return *this;
		}

		bool operator ==(const CSBWMothParameters& in)const
		{
			bool bEqual = true;

			if (m_Pmax != in.m_Pmax)bEqual = false;
			if (m_Wmin != in.m_Wmin)bEqual = false;
			if (m_w_α != in.m_w_α)bEqual = false;
			if (m_w_horizontal != in.m_w_horizontal)bEqual = false;
			if (m_w_horizontal_σ != in.m_w_horizontal_σ)bEqual = false;
			if (m_Δv != in.m_Δv)bEqual = false;
			if(m_Hv != in.m_Hv)bEqual = false;
			if (m_w_descent != in.m_w_descent)bEqual = false;
			if (m_w_descent_σ != in.m_w_descent_σ)bEqual = false;
			if(m_flight_after_sunrise != in.m_flight_after_sunrise)bEqual = false;
			if (m_maxFlights != in.m_maxFlights)bEqual = false;
			if (m_ready_to_fly != in.m_ready_to_fly)bEqual = false;
			if (m_bAppplyAttrition != in.m_bAppplyAttrition)bEqual = false;
			if (m_max_adult_longevity != in.m_max_adult_longevity)bEqual = false;

			
			
			
			return bEqual;
		}

		bool operator !=(const CSBWMothParameters& in)const{ return !operator ==(in); }


	protected:

		static const char* MEMBERS_NAME[NB_ATM_MEMBERS];
	};


	//*****************************************************************************************************************
	class CATMWorldParamters
	{
	public:

		enum Tprcp { DONT_USE_PRCP, PRCP_SAME_AS_INPUT, PRCP_WEATHER_STATION };
		enum TBroodT { BROOD_T_17, BROOD_AT_SUNSET };

		//static public member 
		enum TweatherType { FROM_GRIBS, FROM_STATIONS, FROM_BOTH, NB_WEATHER_TYPE };
		enum TMember { WEATHER_TYPE, SIMUL_PERIOD, TIME_STEP, SEED, USE_SPACE_INTERPOL, USE_TIME_INTERPOL, USE_PREDICTOR_CORRECTOR_METHOD, USE_VERTICAL_VELOCITY, MAXIMUM_FLYERS, MAX_MISS_HOURS, FORCE_FIRST_FLIGHT, BROOD_T_SOURCE, PRCP_SOURCE, DEM, WATER, GRIBS, HOURLY_DB, DEFOLIATION, OUTPUT_SUB_HOURLY, OUTPUT_FILE_TITLE, OUTPUT_FREQUENCY, CREATE_EGG_MAPS, EGG_MAP_TITLE, EGG_MAP_RES, WIND_STABILITY, NB_WEATHER_STATIONS, NB_MEMBERS };
		static const char* GetMemberName(int i) { ASSERT(i >= 0 && i < NB_MEMBERS); return MEMBERS_NAME[i]; }

		//public member
		int m_weather_type;	//weather type: stations hourly databse or multi-layer (3D) grid
		CTPeriod m_simulationPeriod;	//simulation period
		size_t m_time_step;				//time step in [s]
		size_t m_seed;					//seed for random number
		bool m_bUseSpaceInterpolation;
		bool m_bUseTimeInterpolation;
		bool m_bUsePredictorCorrectorMethod;
		bool m_bUseVerticalVelocity;
		size_t m_maxFlyers;				//maximum number of flyer per night
		double m_max_missing_weather;	//maximum of missing weather to skip day (h)
		bool m_bForceFirstFlight;		//always force the first flight even if there is no defoliation or over water
		size_t m_broodTSource;			//source temperature for broods
		size_t m_PSource;				//precipitation source


		std::string m_gribs_name;		//filepath on the grib file list
		std::string m_defoliation_name;
		std::string m_hourly_DB_name;
		std::string m_DEM_name;
		std::string m_water_name;

		bool m_bOutputSubHourly;
		std::string m_outputFileTitle;
		__int64 m_outputFrequency;

		bool m_bCreateEggMaps;
		std::string m_eggMapsTitle;
		double m_eggMapsResolution;
		int m_windS_stability_type;	//not used for the moment
		int m_nb_weather_stations;	//number of weather station to looking for
		int m_max_hour_load;		//maximum number or weaher image load at the same time


		size_t get_time_step()const { return m_time_step; }//[s]
		size_t get_nb_time_step()const { assert(3600 % m_time_step == 0); return size_t(3600 / m_time_step); }	//number of timeStep per hour

		bool UseGribs()const { return m_weather_type == FROM_GRIBS || m_weather_type == FROM_BOTH; }
		bool UseHourlyDB()const { return m_weather_type == FROM_STATIONS || m_weather_type == FROM_BOTH; }



		CATMWorldParamters()
		{
			clear();
		}

		void clear()
		{
			m_weather_type = FROM_GRIBS;
			m_simulationPeriod = CTPeriod(CTRef::GetCurrentTRef(), CTRef::GetCurrentTRef());
			m_time_step = 10; //10 seconds by default
			m_seed = 0;
			m_bUseSpaceInterpolation = true;
			m_bUseTimeInterpolation = true;
			m_bUsePredictorCorrectorMethod = true;
			m_bUseVerticalVelocity = true;
			m_windS_stability_type = CWindStability::AUTO_DETECT;
			m_nb_weather_stations = 3;
			m_max_hour_load = 24;
			m_max_missing_weather = 3;		//maximum of missing hour to skip day
			m_maxFlyers = 0;
			m_bForceFirstFlight = false;
			m_broodTSource = BROOD_T_17;
			m_PSource = DONT_USE_PRCP;

			m_gribs_name.clear();
			m_defoliation_name.clear();
			m_hourly_DB_name.clear();
			m_DEM_name.clear();
			m_water_name.clear();

			m_bOutputSubHourly = false;
			m_outputFileTitle = "06MinutesOutput.csv";
			m_outputFrequency = 360;

			m_bCreateEggMaps = false;
			m_eggMapsTitle = "EggDensity.tif";
			m_eggMapsResolution = 4000;
		}


		CATMWorldParamters& operator =(const CATMWorldParamters& in)
		{
			if (&in != this)
			{
				m_weather_type = in.m_weather_type;
				m_simulationPeriod = in.m_simulationPeriod;
				m_time_step = in.m_time_step;
				m_seed = in.m_seed;
				m_bUseSpaceInterpolation = in.m_bUseSpaceInterpolation;
				m_bUseTimeInterpolation = in.m_bUseTimeInterpolation;
				m_bUsePredictorCorrectorMethod = in.m_bUsePredictorCorrectorMethod;
				m_bUseVerticalVelocity = in.m_bUseVerticalVelocity;
				m_windS_stability_type = in.m_windS_stability_type;
				m_nb_weather_stations = in.m_nb_weather_stations;
				m_max_hour_load = in.m_max_hour_load;
				m_max_missing_weather = in.m_max_missing_weather;
				m_maxFlyers = in.m_maxFlyers;
				m_bForceFirstFlight = in.m_bForceFirstFlight;
				m_broodTSource = in.m_broodTSource;
				m_PSource = in.m_PSource;



				m_gribs_name = in.m_gribs_name;
				m_defoliation_name = in.m_defoliation_name;
				m_hourly_DB_name = in.m_hourly_DB_name;
				m_DEM_name = in.m_DEM_name;
				m_water_name = in.m_water_name;

				m_bOutputSubHourly = in.m_bOutputSubHourly;
				m_outputFileTitle = in.m_outputFileTitle;
				m_outputFrequency = in.m_outputFrequency;

				m_bCreateEggMaps = in.m_bCreateEggMaps;
				m_eggMapsTitle = in.m_eggMapsTitle;
				m_eggMapsResolution = in.m_eggMapsResolution;

			}

			return *this;
		}

		bool operator ==(const CATMWorldParamters& in)const
		{
			bool bEqual = true;

			if (m_weather_type != in.m_weather_type)bEqual = false;
			if (m_simulationPeriod != in.m_simulationPeriod)bEqual = false;
			if (m_time_step != in.m_time_step)bEqual = false;
			if (m_seed != in.m_seed)bEqual = false;
			if (m_bUseSpaceInterpolation != in.m_bUseSpaceInterpolation)bEqual = false;
			if (m_bUseTimeInterpolation != in.m_bUseTimeInterpolation)bEqual = false;
			if (m_bUsePredictorCorrectorMethod != in.m_bUsePredictorCorrectorMethod)bEqual = false;
			if (m_bUseVerticalVelocity != in.m_bUseVerticalVelocity)bEqual = false;
			if (m_windS_stability_type != in.m_windS_stability_type)bEqual = false;
			if (m_nb_weather_stations != in.m_nb_weather_stations)bEqual = false;
			if (m_max_hour_load != in.m_max_hour_load)bEqual = false;
			if (m_max_missing_weather != in.m_max_missing_weather)bEqual = false;
			if (m_maxFlyers != in.m_maxFlyers)bEqual = false;
			if (m_bForceFirstFlight != in.m_bForceFirstFlight)bEqual = false;
			if (m_broodTSource != in.m_broodTSource)bEqual = false;
			if (m_PSource != in.m_PSource)bEqual = false;


			if (m_gribs_name != in.m_gribs_name)bEqual = false;
			if (m_defoliation_name != in.m_defoliation_name)bEqual = false;
			if (m_hourly_DB_name != in.m_hourly_DB_name)bEqual = false;
			if (m_DEM_name != in.m_DEM_name)bEqual = false;
			if (m_water_name != in.m_water_name)bEqual = false;

			if (m_bOutputSubHourly != in.m_bOutputSubHourly)bEqual = false;
			if (m_outputFileTitle != in.m_outputFileTitle)bEqual = false;
			if (m_outputFrequency != in.m_outputFrequency)bEqual = false;

			if (m_bCreateEggMaps != in.m_bCreateEggMaps)bEqual = false;
			if (m_eggMapsTitle != in.m_eggMapsTitle)bEqual = false;
			if (m_eggMapsResolution != in.m_eggMapsResolution)bEqual = false;


			return bEqual;
		}

		bool operator !=(const CATMWorldParamters& in)const { return !operator ==(in); }


	protected:


		static const char* MEMBERS_NAME[NB_MEMBERS];
	};

	//*****************************************************************************************************************
	class CATMVariables : public std::array < double, NB_ATM_VARIABLES >
	{
	public:

		CATMVariables()
		{
			fill(-999);
		}

		CATMVariables& operator += (CATMVariables& in)
		{
			ASSERT(is_init());
			ASSERT(in.is_init());
			for (size_t i = 0; i < size(); i++)
			{
				if (at(i) != -999 && in.at(i) != -999)
					at(i) += in.at(i);
			}

			return *this;
		}

		CATMVariables& operator *= (double in)
		{
			ASSERT(is_init());
			for (size_t i = 0; i < size(); i++)
			{
				if (at(i) != -999)
					at(i) *= in;
			}


			return *this;
		}

		CATMVariables& operator /= (double in)
		{
			ASSERT(is_init());
			for (size_t i = 0; i < size(); i++)
			{
				if (at(i) != -999)
					at(i) /= in;
			}

			return *this;
		}

		friend CATMVariables operator + (const CATMVariables& in1, const CATMVariables& in2)
		{
			ASSERT(in1.is_init());
			ASSERT(in2.is_init());

			CATMVariables out;
			for (size_t i = 0; i < in1.size(); i++)
			{
				if (in1[i] != -999 && in2[i] != -999)
					out[i] = in1[i] + in2[i];
			}

			return out;
		}

		friend CATMVariables operator * (const CATMVariables& in1, double f)
		{
			ASSERT(in1.is_init());
			CATMVariables out;
			for (size_t i = 0; i < in1.size(); i++)
			{
				if (in1[i] != -999)
					out[i] = in1[i] * f;
			}

			return out;
		}

		friend CATMVariables operator / (const CATMVariables& in1, double f)
		{
			CATMVariables out;
			for (size_t i = 0; i < in1.size(); i++)
			{
				if(in1[i] != -999)
				out[i] = in1[i] / f;
			}

			return out;
		}


		static double get_Uw(double p, double t, double ω);
		double get_wind_speed(bool b_used_z = false)const{ return   sqrt(at(ATM_WNDU) * at(ATM_WNDU) + at(ATM_WNDV) * at(ATM_WNDV) + (b_used_z ? (at(ATM_WNDW)*at(ATM_WNDW)) : 0)); }
		//return wind direction clockwise from up [°]
		//in the middle of the grid, it can be considerate to be the North
		//double get_wind_direction()const { return Rad2Deg(atan2(at(ATM_WNDU), at(ATM_WNDU))) ; }
		bool is_init()const
		{ 
			bool bInit = false;
			for (size_t i = 0; i < size() && !bInit; i++)
				bInit = at(i) != -999;

			return bInit;
		}
	};

	//************************************************************************************************************

	static const int NB_POINTS_X = 2;
	static const int NB_POINTS_Y = 2;
	static const int NB_POINTS_Z = 2;
	static const int TIME_SIZE = 2;
	static const int NB_LEVELS = 38;
	class CATMWeatherCuboid : public std::array < std::array < std::array < CATMVariables, NB_POINTS_X >, NB_POINTS_Y >, NB_POINTS_Z >
	{

	public:

		CATMWeatherCuboid()
		{
			m_time = 0;
		}

		//m_pt have elevation above ground
		std::array < std::array < std::array < CGeoPoint3D, NB_POINTS_X >, NB_POINTS_Y >, NB_POINTS_Z > m_pt;
		__int64 m_time;

		CATMVariables get_weather(const CGeoPoint3D& pt, bool bSpaceInterpol)const;
	};


	class CATMWeatherCuboids : public std::array < CATMWeatherCuboid, TIME_SIZE >
	{
	public:

		CATMWeatherCuboids()
		{
			m_bUseSpaceInterpolation = true;
			m_bUseTimeInterpolation = true;
		}

		bool m_bUseSpaceInterpolation;
		bool m_bUseTimeInterpolation;
		CATMVariables get_weather(const CGeoPoint3D& pt, __int64 UTCCurrentTime)const;
		
	};

	typedef std::shared_ptr<CATMWeatherCuboids> CATMWeatherCuboidsPtr;



	//*****************************************************************************************************************
	class CBlockData
	{
	public:

		CBlockData(int nXBlockSize, int nYBlockSize, int dataType);

		~CBlockData()
		{
			delete[] m_ptr;
		}


		double GetValue(int x, int y)const;
		void SetValue(int x, int y, double value);

		double GetValue(size_t pos)const;
		void SetValue(size_t pos, double value);

		void* m_ptr;
		int m_xBlockSize;
		int m_yBlockSize;
		int m_dataType;
	};


	typedef std::shared_ptr<CBlockData> CBlockDataPtr;
	typedef boost::multi_array <CBlockDataPtr, 3> CDataCache;
	class CGDALDatasetCached : public CGDALDatasetEx
	{
	public:

		CGDALDatasetCached();
		ERMsg OpenInputImage(const std::string& filePath, bool bOpenInv = false, bool bSurfaceOnly = false);
		void Close();


//		CGeoRect m_clipRect;

		double GetPixel(const CGeoPoint3DIndex& index)const;
		double GetPixel(size_t b, const CGeoPointIndex& xy)const{ return GetPixel(CGeoPoint3DIndex(xy.m_x, xy.m_y, (int)b)); }
		bool IsCached(const CGeoBlock3DIndex& ijk)const{ assert(IsBlockInside(ijk));  return m_data[ijk.m_z][ijk.m_y][ijk.m_x] != NULL; }
		bool IsCached(size_t b, const CGeoBlockIndex& xy)const{ return IsCached(CGeoPoint3DIndex(xy.m_x, xy.m_y, (int)b)); }

		size_t get_band(size_t v, size_t level)const;
		bool get_fixed_elevation_level(size_t l, double& elev)const;
		
		//RasterIO
	protected:


		void InitCache()const;
		bool IsCacheInit()const{ return !m_data.empty(); }
		void LoadBlock(size_t b, const CGeoBlockIndex& xy){ LoadBlock(CGeoBlock3DIndex(xy.m_x, xy.m_y, (int)b)); }
		void LoadBlock(const CGeoBlock3DIndex& index);
		bool IsBlockInside(const CGeoBlock3DIndex& i)const
		{
			return
				i.m_z >= 0 && i.m_z < m_data.size() &&
				i.m_y >= 0 && i.m_y < m_data[i.m_z].size() &&
				i.m_x >= 0 && i.m_x < m_data[i.m_z][i.m_y].size();
		}

		CDataCache m_data;
		static std::mutex m_mutex;

		//statistic of simulation
		std::array<CStatistic, 1 > m_stats;
		std::array<std::array<size_t, NB_LEVELS>, NB_ATM_VARIABLES_EX> m_bands;
		std::vector<std::string> m_units;
		std::map < size_t, double > m_fixedElevationLevel;
	};

	//*****************************************************************************************************************
	typedef std::map <__int64, std::string > TTimeFilePathMap;
	typedef std::shared_ptr<CGDALDatasetCached >CGDALDatasetCachedPtr;
	typedef std::map <__int64, CGDALDatasetCachedPtr> TTimeDatasetMapBase;

	class CTimeDatasetMap : protected TTimeDatasetMapBase
	{
	public:

		int m_max_hour_load;
		CGeoRect m_clipRect;

		CTimeDatasetMap();
		
		ERMsg load(__int64 UTCWeatherTime, const std::string& filePath, CCallback& callback)const;
		double GetPixel(__int64 UTCWeatherTime, const CGeoPoint3DIndex& index)const;
		double GetPixel(__int64 UTCWeatherTime, size_t b, const CGeoPointIndex& xy)const{ return GetPixel(UTCWeatherTime, CGeoPoint3DIndex(xy.m_x, xy.m_y, (int)b)); }
		const CGeoExtents& GetExtents(__int64 UTCWeatherTime)const;
		bool IsLoaded(__int64 UTCWeatherTime)const;
		ERMsg Discard(CCallback& callback);
		size_t get_band(__int64 UTCWeatherTime, size_t v, size_t level)const;
		size_t GetPrjID(__int64 UTCWeatherTime)const{ return at(UTCWeatherTime)->GetPrjID(); }
		CGDALDatasetCachedPtr& at(__int64 UTCWeatherTime) { return TTimeDatasetMapBase::at(GetNearestFloorTime(UTCWeatherTime)); }
		const CGDALDatasetCachedPtr& at(__int64 UTCWeatherTime)const { return TTimeDatasetMapBase::at(GetNearestFloorTime(UTCWeatherTime)); }
		size_t size()const{ return TTimeDatasetMapBase::size(); }
		
		bool get_fixed_elevation_level(__int64 UTCWeatherTime, size_t l, double& elev)const;
		
		__int64 GetNearestFloorTime(__int64 UTCWeatherTime)const;
		__int64 GetNextTime(__int64 UTCWeatherTime)const;
		

	};




	class CATMWeather
	{
	public:



		CATMWeather(CATMWorld& world) :
			m_world(world)
		{
			m_bHgtOverSea = false;
			m_bHgtOverSeaTested = false;
		}
		
		
		ERMsg Load(const std::string& gribsFilepath, const std::string& hourlyDBFilepath, CCallback& callback);
		ERMsg load_gribs(const std::string& filepath, CCallback& callback);
		ERMsg load_hourly(const std::string& filepath, CCallback& callback);
		ERMsg Discard(CCallback& callback);


		ERMsg LoadWeather(__int64 UTCWeatherTime, CCallback& callback);
		CATMWeatherCuboidsPtr get_cuboids(const CGeoPoint3D& pt, __int64 UTCWeatherTime)const;
		CATMVariables get_station_weather(const CGeoPoint3D& pt, __int64 UTCWeatherTime)const;
		CATMVariables get_station_weather(const CGeoPoint3D& pt, __int64 UTCWeatherTime, bool m_bUseTimeInterpolation)const;
		double get_air_temperature(const CGeoPoint3D& pt, __int64 UTCWeatherTime, __int64 UTCCurrentTime);

		CATMVariables get_weather(const CGeoPoint3D& pt, __int64 UTCWeatherTime, __int64 UTCCurrentTime)const;
		std::string get_image_filepath(__int64 UTCWeatherTime)const;

		CGeoPoint3DIndex get_ulz(const CGeoPoint3D& pt, __int64 UTCWeatherTime)const;
		CGeoPointIndex get_ul(const CGeoPoint& pt, __int64 UTCWeatherTime)const;
		size_t get_level(const CGeoPointIndex& xy, const CGeoPoint3D& pt, __int64 UTCWeatherTime, bool bLow)const;
		double GetFirstAltitude(const CGeoPointIndex& xy, __int64 UTCWeatherTime)const;

		bool is_init()const{ return !m_filepath_map.empty() || m_p_hourly_DB != NULL; }

		size_t GetGribsPrjID(__int64 UTCWeatherTime)const;
		
		CGDALDatasetCachedPtr& at(__int64 UTCWeatherTime) { return m_p_weather_DS.at(UTCWeatherTime); }
		bool IsLoaded(__int64 UTCWeatherTime)const;

		static double LandWaterWindFactor(double Ul, double ΔT);
		static void GetWindProfileRelationship(double& Ur, double& Vr, double z, int stabType, bool bOverWather, double ΔT);

		bool HaveGribsWeather()const{ return !m_filePathGribs.empty(); }
		bool HaveStationWeather()const{ return !m_filePathHDB.empty(); }

		__int64 GetNearestFloorTime(__int64 UTCTime)const;
		__int64 GetNextTime(__int64 UTCTime)const;
		__int64 GetLastTime()const;
		CTPeriod GetEntireTPeriod()const;

	protected:

		std::string m_filePathGribs;
		std::string m_filePathHDB;

		TTimeFilePathMap m_filepath_map;
		CHourlyDatabasePtr m_p_hourly_DB;
		CTimeDatasetMap m_p_weather_DS;
		

		std::map<size_t, CWeatherStation> m_stations;
		std::map<size_t, CWaterTemperature> m_Twater;
		std::map<CTRef, std::array<CIWD, NB_ATM_VARIABLES>> m_iwd;
		CATMWorld& m_world;
		bool m_bHgtOverSea;
		bool m_bHgtOverSeaTested;
	};



	class CSBWMoth
	{
	public:


		enum TStat{ HOURLY_STAT, SUB_HOURLY_STAT, NB_STATS};
		enum TLog{ T_LIFTOFF, T_LANDING_BEGIN, T_LANDING_END, NB_FLYER_LOG };
		enum TFlyerStat{ S_TAIR, S_PRCP, S_U, S_V, S_W, S_D_X, S_D_Y, S_D_Z, S_DISTANCE, S_HEIGHT, S_W_HORIZONTAL, S_W_VERTICAL, S_MOTH_WH, S_MOTH_WV, NB_FLYER_STAT };
		enum TStates { NOT_EMERGED, LIVE, FLY, FINISHED, NB_STATES };
		
		enum TNoLiftoff { NO_LIFTOFF_DEFINED, NO_LIFTOFF_EMERGING, NO_LIFTOFF_NOT_READY, NO_LIFTOFF_NO_DEFOLIATION, NO_LIFTOFF_NO_MORE_FLIGHT, NO_LIFTOFF_TAIR, NO_LIFTOFF_PRCP, NO_LIFTOFF_WNDS, NO_LIFTOFF_MISSING_WEATHER, NB_NO_LIFTOFF_TYPE};
		enum TFlight { NO_FLIGHT_DEFINE, WAIT_DEPARTURE, FLIYNG, LANDING, NB_FLIGHT_TYPE };
		enum TFlightEnd{ NO_FLIGHT_END_DEFINE, END_BY_TAIR, END_BY_PRCP, END_BY_SUNRISE, NB_FLIGHT_END_TYPE};
		enum TFinished { NO_END_DEFINE, END_FULLFILLED, END_OVER_WATER_TAIR, END_OVER_WATER_PRCP, END_OVER_WATER_SUNRISE, END_OLD_AGE, END_OUTSIDE_MAP, END_OF_SIMULATION, WEATHER_NOT_INIT, NB_END_TYPE};

		size_t m_ID;
		size_t m_flightNo;
		
		size_t m_sex;			//sex (MALE=0, FEMALE=1)
		double m_A;				//Forewing surface area [cm²]
		double m_M;				//dry weight [g]
		double m_ξ;				//error term on weight (M)
		double m_G;				//gravidity gravid=1, spent=0
		double m_Fᵒ;			//initial fecondity without defoliation
		double m_Fᴰ;			//initial fecondity with defoliation
		double m_F;				//current fecondity
		double m_age;			//current physiological age
		CTRef m_emergingDate;	//Creation date in UTC time

		
		CLocation m_location;	//initial position
		//CLocation m_newLocation;//actual position, z is elevation over sea level
		CGeoPoint3D m_pt;		//same as m_newLocation but with flight height instead of elevation over sea level
		double m_w_horizontal;	//horizontal flight speed [m/s]
		double m_w_descent;		//descent flight speed [m/s]
		__int64 m_liftoff_time;	//UTC liftoff time [s]
		__int64 m_duration;		//maximum flight duration [s]
		double m_p_exodus;		//probability of exedus in function of hour
		__int64 m_UTCShift;		//shift between local time and UTC [s]

		CSBWMoth(CATMWorld& world);

		size_t GetState()const { return m_state; }
		size_t GetFlag()const;

		void live(CTRef TRef);
		bool init_new_night(CTRef TRef);
		
		void fly(__int64 UTCWeatherTime, __int64 UTCCurrentTime);
		void flying(__int64 UTCWeatherTime, __int64 UTCCurrentTime);
		void landing(__int64 UTCWeatherTime, __int64 UTCCurrentTime);
		
		const CGeoPoint3D& get_pt()const{ return m_pt; }
		CATMVariables get_weather(__int64 UTCWeatherTime, __int64 UTCCurrentTime)const;
		CGeoDistance3D get_U(const CATMVariables& w, __int64 UTCWeatherTime)const;
		double get_Uz(const CATMVariables& w)const;
		double get_Tᶠ(double Δv)const;

		static double get_Vᴸ(double A, double M);
		static double get_Vᵀ(double T);
		static double get_Tᴸ(double A, double M);
		static double get_Tᶠ(double A, double M, double Δv);
		static double ComputeRate(double T);
		static double get_Uz(double A, double M, double Tair, double Δv, double α);

		double GetLogTime(size_t i)const{ return m_logTime[i]; }
		double GetLogT(size_t i)const { return m_logT[i]; }
		double GetStat(size_t i, size_t v, double f = 1, size_t s = MEAN)const{ return m_stat[i][v].IsInit() ? m_stat[i][v][s] * f : -999; }
		void ResetStat(size_t i){ m_stat[i].fill(CStatistic()); }


		__int64 GetUTCShift()const{ return m_UTCShift; }//in [s]
		void Brood(double T);
		bool ComputeExodus(double T, double P, double W, double tau);
		bool GetLiftoff(__int64 UTCTimeº, __int64 sunset, __int64& liftoff);
		bool get_t(__int64 UTCTimeº, __int64 tᶳ, __int64 &tº, __int64 &tᶜ, __int64 &tᴹ)const;
		void FillOutput(size_t type, CTRef localTRef, CATMOutputMatrix& output);
		bool IsLanded() const { return m_logTime[T_LANDING_END] > -999; }
		bool CanFly()const;
		bool ForceFirst()const; 
		bool IsOverWater()const;
		bool IsOverDefoliation()const;
		void KillByWater();
		double z_AGL()const;

	protected:

		void AddStat(const CATMVariables& w, const CGeoDistance3D& U, const CGeoDistance3D& d);
		void AddStat(const CATMVariables& w);

		
		

		TStates m_state;				//state of the flyer
		TFlight m_flight_flag;			//flight state
		TFlightEnd m_landing_flag;		//reason of landing
		TNoLiftoff m_no_liftoff_flag;	//reason of no liftoff
		TFinished m_finish_flag;		//reason of simulation end


		
		CATMWorld& m_world;

		//statistic of flight
		std::array<__int64, NB_FLYER_LOG> m_logTime;
		std::array<double, NB_FLYER_LOG> m_logT;

		std::array<std::array<CStatistic, NB_FLYER_STAT>, 2> m_stat;
		std::array<size_t, 3> m_noLiftoff;
		
		
		CTRef m_lastOutput;
		size_t m_lastState;
		double m_lastF;				

		static TNoLiftoff  GetNoLiftoffCode(const std::array<size_t, 3>& noLiftoff);
		//static const double a;
		//static const double b;
		//static const double Vmax;
		//static const double K;

		static const double p1;
		static const double p2;
		static const double p3;
		static const double La;//mm
		static const double To;//°C

	};


	typedef std::deque <CSBWMoth> CSBWMoths;
	typedef CSBWMoths::iterator CSBWMothsIt;
	typedef CSBWMoths::const_iterator CSBWMothsCIt;




	class CATMWorld
	{
	public:

		static const char* ATM_VERTION;

		//return the number of second since 1 december of year 1
		size_t get_time_step()const{ return m_world_param.get_time_step(); }//[s]
		size_t get_nb_time_step()const{ return m_world_param.get_nb_time_step(); }	//number of timeStep per hour
		
		static __int64 get_sunrise(CTRef TRef, const CGeoPoint& loc, double zone);
		static __int64 get_sunset(CTRef TRef, const CGeoPoint& loc, double zone);
		CATMVariables get_weather(const CGeoPoint3D& pt, __int64 UTCWeatherTime, __int64 UTCCurrentTime)const { return m_weather.get_weather(pt, UTCWeatherTime, UTCCurrentTime); }
		

		
		
		CTimePeriod get_UTC_sunset_period(CTRef TRef, const std::vector<CSBWMothsIt>& fls);
		CTimePeriod get_UTC_flight_period(const std::vector<CSBWMothsIt>& fls);
		std::vector<__int64> GetWeatherTime(CTimePeriod UTC_period, CCallback& callback)const;
		ERMsg LoadWeather(CTRef TRef, const std::vector<__int64>& gribs, CCallback& callback);


		int m_nb_max_threads;
		CATMWorldParamters m_world_param;
		CSBWMothParameters m_moths_param;
		size_t m_seasonalIndividuals;

		CATMWeather m_weather;
		CSBWMoths m_moths;
		CGDALDatasetCached m_DEM_DS;
		CGDALDatasetCached m_defoliation_DS;
		CGDALDatasetCached m_water_DS;


		double GetGroundAltitude(const CGeoPoint3D& pt)const;
		bool is_over_defoliation(const CGeoPoint3D& pt)const;
		double get_defoliation(const CGeoPoint3D& pt)const;
		bool is_over_water(const CGeoPoint3D& pt)const;

		ERMsg Init(CCallback& callback);
		ERMsg Execute(CATMOutputMatrix& output, ofStream& output_file, CCallback& callback);
		ERMsg ExecuteOneNight(CTRef TRef, CATMOutputMatrix& output, CATMOutputMatrix& sub_output, CCallback& callback);
		ERMsg SimulateOneNight(CTRef TRef, std::vector<CSBWMothsIt>& fls, CATMOutputMatrix& output, CATMOutputMatrix& sub_output, CCallback& callback);
		ERMsg CreateEggDepositionMap(const std::string& inputFilePath, const std::string& outputFilePath, CTPeriod savedPeriod, CCallback& callback);
		void FinalizedOutput(CATMOutputMatrix& output);


		CATMWorld() :
			m_weather(*this)
		{
			m_nb_max_threads = 1;
			m_seasonalIndividuals = 0;
		}

		CRandomGenerator& random(){ return m_random; }
		CTPeriod get_moths_period()const;
		
		ERMsg save_output(CTPeriod p, ofStream& output_file, const CATMOutputMatrix& output, CCallback& callback);
		ERMsg save_sub_output(CTRef TRef, ofStream& output_file, const CATMOutputMatrix& sub_output, CCallback& callback);
		//void init_sub_ourly(ofStream& output_file, const CATMOutputMatrix& output, CATMOutputMatrix& sub_output);
		void init_output(CTRef TRef, CATMOutputMatrix& output);
		void init_sub_hourly(CTRef TRef, CATMOutputMatrix& sub_output);
		void write_sub_hourly_header(ofStream& output_file);
		


		std::map<size_t, CProjectionTransformation> m_GEO2;
		std::map<size_t, CProjectionTransformation> m_2GEO;
		const CProjectionTransformation& GetToWeatherTransfo(__int64 UTCWeatherTime)const;
		const CProjectionTransformation& GetFromWeatherTransfo(__int64 UTCWeatherTime)const;
		
		double get_w_horizontal()const;
		double get_w_descent()const;
		bool IsInside(const CGeoPoint& pt)const;
		
		
		double GetPmax()const { return ((m_world_param.m_PSource == CATMWorldParamters::DONT_USE_PRCP) ? 9999 : m_moths_param.m_Pmax); }

	protected:

		CRandomGenerator m_random;

		//last GetGroundAltitudePoint and value
		/*CGeoPoint m_last_GGA_pt;
		double m_last_GGA;*/
	};

	

}


namespace zen
{


	template <> inline
		void writeStruc(const WBSF::CSBWMothParameters& in, XmlElement& output)
	{
		XmlOut out(output);
		
		
		out[WBSF::CSBWMothParameters::GetMemberName(WBSF::CSBWMothParameters::P_MAX)](in.m_Pmax);
		out[WBSF::CSBWMothParameters::GetMemberName(WBSF::CSBWMothParameters::W_MIN)](in.m_Wmin);
		out[WBSF::CSBWMothParameters::GetMemberName(WBSF::CSBWMothParameters::WING_BEAT_SCALE)](in.m_w_α);
		out[WBSF::CSBWMothParameters::GetMemberName(WBSF::CSBWMothParameters::W_HORZ)](in.m_w_horizontal);
		out[WBSF::CSBWMothParameters::GetMemberName(WBSF::CSBWMothParameters::W_HORZ_SD)](in.m_w_horizontal_σ);
		out[WBSF::CSBWMothParameters::GetMemberName(WBSF::CSBWMothParameters::REDUCTION_FACTORE)](in.m_Δv);
		out[WBSF::CSBWMothParameters::GetMemberName(WBSF::CSBWMothParameters::REDUCTION_HEIGHT)](in.m_Hv);
		out[WBSF::CSBWMothParameters::GetMemberName(WBSF::CSBWMothParameters::W_DESCENT)](in.m_w_descent);
		out[WBSF::CSBWMothParameters::GetMemberName(WBSF::CSBWMothParameters::W_DESCENT_SD)](in.m_w_descent_σ);
		out[WBSF::CSBWMothParameters::GetMemberName(WBSF::CSBWMothParameters::FLIGHT_AFTER_SUNRISE)](in.m_flight_after_sunrise);
		out[WBSF::CSBWMothParameters::GetMemberName(WBSF::CSBWMothParameters::MAXIMUM_FLIGHTS)](in.m_maxFlights);
		out[WBSF::CSBWMothParameters::GetMemberName(WBSF::CSBWMothParameters::APPPLY_ATTRITION)](in.m_bAppplyAttrition);
		out[WBSF::CSBWMothParameters::GetMemberName(WBSF::CSBWMothParameters::MAX_ADULT_LONGEVITY)](in.m_max_adult_longevity);
		out[WBSF::CSBWMothParameters::GetMemberName(WBSF::CSBWMothParameters::READY_SHIFT0)](in.m_ready_to_fly[0]);
		out[WBSF::CSBWMothParameters::GetMemberName(WBSF::CSBWMothParameters::READY_SHIFT1)](in.m_ready_to_fly[1]);
		
		
		
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CSBWMothParameters& out)
	{
		XmlIn in(input);

		in[WBSF::CSBWMothParameters::GetMemberName(WBSF::CSBWMothParameters::P_MAX)](out.m_Pmax);
		in[WBSF::CSBWMothParameters::GetMemberName(WBSF::CSBWMothParameters::W_MIN)](out.m_Wmin);
		in[WBSF::CSBWMothParameters::GetMemberName(WBSF::CSBWMothParameters::WING_BEAT_SCALE)](out.m_w_α);
		in[WBSF::CSBWMothParameters::GetMemberName(WBSF::CSBWMothParameters::W_HORZ)](out.m_w_horizontal);
		in[WBSF::CSBWMothParameters::GetMemberName(WBSF::CSBWMothParameters::W_HORZ_SD)](out.m_w_horizontal_σ);
		in[WBSF::CSBWMothParameters::GetMemberName(WBSF::CSBWMothParameters::REDUCTION_FACTORE)](out.m_Δv);
		in[WBSF::CSBWMothParameters::GetMemberName(WBSF::CSBWMothParameters::REDUCTION_HEIGHT)](out.m_Hv);
		in[WBSF::CSBWMothParameters::GetMemberName(WBSF::CSBWMothParameters::W_DESCENT)](out.m_w_descent);
		in[WBSF::CSBWMothParameters::GetMemberName(WBSF::CSBWMothParameters::W_DESCENT_SD)](out.m_w_descent_σ);
		in[WBSF::CSBWMothParameters::GetMemberName(WBSF::CSBWMothParameters::FLIGHT_AFTER_SUNRISE)](out.m_flight_after_sunrise);
		in[WBSF::CSBWMothParameters::GetMemberName(WBSF::CSBWMothParameters::MAXIMUM_FLIGHTS)](out.m_maxFlights);
		in[WBSF::CSBWMothParameters::GetMemberName(WBSF::CSBWMothParameters::APPPLY_ATTRITION)](out.m_bAppplyAttrition);
		in[WBSF::CSBWMothParameters::GetMemberName(WBSF::CSBWMothParameters::MAX_ADULT_LONGEVITY)](out.m_max_adult_longevity);
		in[WBSF::CSBWMothParameters::GetMemberName(WBSF::CSBWMothParameters::READY_SHIFT0)](out.m_ready_to_fly[0]);
		in[WBSF::CSBWMothParameters::GetMemberName(WBSF::CSBWMothParameters::READY_SHIFT1)](out.m_ready_to_fly[1]);

		return true;
	}




	template <> inline
		void writeStruc(const WBSF::CATMWorldParamters& in, XmlElement& output)
	{
		XmlOut out(output);

		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::WEATHER_TYPE)](in.m_weather_type);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::SIMUL_PERIOD)](in.m_simulationPeriod);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::TIME_STEP)](in.m_time_step);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::SEED)](in.m_seed);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::USE_SPACE_INTERPOL)](in.m_bUseSpaceInterpolation);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::USE_TIME_INTERPOL)](in.m_bUseTimeInterpolation);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::USE_PREDICTOR_CORRECTOR_METHOD)](in.m_bUsePredictorCorrectorMethod);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::USE_VERTICAL_VELOCITY)](in.m_bUseVerticalVelocity);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::MAXIMUM_FLYERS)](in.m_maxFlyers);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::BROOD_T_SOURCE)](in.m_PSource);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::PRCP_SOURCE)](in.m_broodTSource);

		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::GRIBS)](in.m_gribs_name);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::DEFOLIATION)](in.m_defoliation_name);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::HOURLY_DB)](in.m_hourly_DB_name);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::DEM)](in.m_DEM_name);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::WATER)](in.m_water_name);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::OUTPUT_SUB_HOURLY)](in.m_bOutputSubHourly);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::OUTPUT_FILE_TITLE)](in.m_outputFileTitle);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::OUTPUT_FREQUENCY)](in.m_outputFrequency);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::CREATE_EGG_MAPS)](in.m_bCreateEggMaps);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::EGG_MAP_TITLE)](in.m_eggMapsTitle);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::EGG_MAP_RES)](in.m_eggMapsResolution);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::WIND_STABILITY)](in.m_windS_stability_type);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::NB_WEATHER_STATIONS)](in.m_nb_weather_stations);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::MAX_MISS_HOURS)](in.m_max_missing_weather);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::FORCE_FIRST_FLIGHT)](in.m_bForceFirstFlight);


	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CATMWorldParamters& out)
	{
		XmlIn in(input);

		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::WEATHER_TYPE)](out.m_weather_type);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::SIMUL_PERIOD)](out.m_simulationPeriod);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::TIME_STEP)](out.m_time_step);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::SEED)](out.m_seed);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::USE_SPACE_INTERPOL)](out.m_bUseSpaceInterpolation);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::USE_TIME_INTERPOL)](out.m_bUseTimeInterpolation);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::USE_PREDICTOR_CORRECTOR_METHOD)](out.m_bUsePredictorCorrectorMethod);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::USE_VERTICAL_VELOCITY)](out.m_bUseVerticalVelocity);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::MAXIMUM_FLYERS)](out.m_maxFlyers);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::BROOD_T_SOURCE)](out.m_PSource);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::PRCP_SOURCE)](out.m_broodTSource);

		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::GRIBS)](out.m_gribs_name);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::DEFOLIATION)](out.m_defoliation_name);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::HOURLY_DB)](out.m_hourly_DB_name);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::DEM)](out.m_DEM_name);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::WATER)](out.m_water_name);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::OUTPUT_SUB_HOURLY)](out.m_bOutputSubHourly);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::OUTPUT_FILE_TITLE)](out.m_outputFileTitle);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::OUTPUT_FREQUENCY)](out.m_outputFrequency);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::CREATE_EGG_MAPS)](out.m_bCreateEggMaps);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::EGG_MAP_TITLE)](out.m_eggMapsTitle);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::EGG_MAP_RES)](out.m_eggMapsResolution);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::WIND_STABILITY)](out.m_windS_stability_type);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::NB_WEATHER_STATIONS)](out.m_nb_weather_stations);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::MAX_MISS_HOURS)](out.m_max_missing_weather);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::FORCE_FIRST_FLIGHT)](out.m_bForceFirstFlight);

		return true;
	}

}
