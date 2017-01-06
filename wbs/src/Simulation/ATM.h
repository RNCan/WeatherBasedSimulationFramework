//******************************************************************************
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

namespace WBSF
{

	class CATMWorld;

	extern const char ATM_HEADER[];//ATM_W_ASCENT
	enum TATMOuput{ ATM_SEX, ATM_G, ATM_STATE, ATM_X, ATM_Y, ATM_LAT, ATM_LON, ATM_T, ATM_P, ATM_U, ATM_V, ATM_W, ATM_MEAN_HEIGHT, ATM_CURRENT_HEIGHT, ATM_DELTA_HEIGHT, ATM_SCALE, ATM_W_HORIZONTAL, ATM_W_VERTICAL, ATM_DIRECTION, ATM_DISTANCE, ATM_DISTANCE_FROM_OIRIGINE, ATM_FLIGHT_TIME, LIFTOFF_TIME, LANDING_TIME, NB_ATM_OUTPUT };
	typedef CModelStatVectorTemplate<NB_ATM_OUTPUT, ATM_HEADER> ATMOutput;
	typedef std::vector<std::vector<std::vector<ATMOutput>>> CATMOutputMatrix;

	//                     °C        hPa      kg/m²     m/s        m/s      m/s        °C
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
	class CATMParameters
	{
	public:

		
		enum TSex{ MALE, FEMALE, NB_SEX};
		

		enum TType{ WING_BEAT, MAX_SPEED, MAX_TEMPERATURE };
		enum TMember{ T_MIN, T_MAX, P_MAX, W_MIN, LIFTOFF_CORRECTION, LIFTOFF_SIGMA, DURATION_MIN, DURATION_MAX, DURATION_ALPHA, DURATION_BETA, CRUISE_DURATION, CRUISE_HEIGHT, HEIGHT_TYPE, WING_BEAT_K, WING_BEAT_VMAX, WING_BEAT_EX, WING_BEAT_ALPHA, W_HORZ, W_HORZ_SD, W_DESCENT, W_DESCENT_SD, WIND_STABILITY, NB_WEATHER_STATIONS, NB_MEMBERS };
		static const char* GetMemberName(int i){ ASSERT(i >= 0 && i < NB_MEMBERS); return MEMBERS_NAME[i]; }


		double m_Tmin;				//minimum temperature for flight [°C]
		double m_Tmax;				//maximum temperature for flight [°C]
		double m_Pmax;				//maximum precipitation for flight [mm/h]
		double m_Wmin;				//minimum wind speed for flight [km/h]
		//size_t m_t_liftoff_type;	//
		//double m_t_liftoff_begin;	//Earliest observed lift-off relatif to suset
		//double m_t_liftoff_end;		//latest observed lift-off relatif to suset
		double m_t_liftoff_correction;	//Extra offset correction of liftoff
		double m_t_liftoff_σ_correction;		//liftoff stadard deviation
		//size_t m_duration_type;		//
		double m_duration_min;		//min flight duration [h]
		double m_duration_max;		//max flight duration [h]
		double m_duration_α;		//max flight duration alpha
		double m_duration_β;		//max flight duration beta
		double m_cruise_duration;	//cruise duration [h]
		double m_cruise_height;		//cruise height [m]

		size_t m_height_type;		//
		//double m_height_lo;			//lower flith height
		//double m_height;			//mean flight height [m]
		//double m_height_σ;			//standard deviation of flight height [m]
		//double m_height_hi;			//highest flith height
//		double m_w_ascent;			//ascent vertical velocity [km/h]
	//	double m_w_ascent_σ;		//ascent vertical velocity standard deviation [km/h]
		double m_K;
		double m_Vmax;
		double m_w_Ex;
		double m_w_α;				//km/h
		double m_w_horizontal;		//horizontal velocity [km/h]
		double m_w_horizontal_σ;	//horizontal velocity standard deviation [km/h]
		double m_w_descent;			//terminal velocity [km/h]
		double m_w_descent_σ;		//terminal velocity standard deviation [km/h]

		int m_windS_stability_type;	//not used for the moment
		int m_nb_weather_stations;	//number of weather station to looking for
		int m_max_hour_load;		//maximum number or weaher image load at the same time

		CATMParameters()
		{
			clear();
		}

		void clear()
		{
			//default parameters for spuce budworm
			m_Tmin = 15.0;				//[°C]
			m_Tmax = 29.5;				//[°C]
			m_Pmax = 2.5;				//[mm/h]
			m_Wmin = 0.7 * 3600 / 1000;	//[km/h]

		//	m_t_liftoff_type = NEW_TYPE;
			//m_t_liftoff_begin = -1.75;				//19.5;	//attention dans la publication c'est marquer 18.5
			//m_t_liftoff_end = 2.25;					//23.5;	
			m_t_liftoff_correction = 0;				//0
			m_t_liftoff_σ_correction = 1;
			//m_duration_type = NEW_TYPE;
			m_duration_min = 0.5;
			m_duration_max = 6;						//[h]
			m_duration_α = 0.9;						//alpha
			m_duration_β = 3;						//beta
			m_cruise_duration = 0.0;				//[h]
			m_cruise_height = 50;					//[m]
			m_height_type = WING_BEAT;
			//m_height_lo = 20;						//[m]
			//m_height = 600;							//mean of the height 
			//m_height_σ = 150;						//stadard deviation of the log of the height (0.53)
			//m_height_hi = 1200;						//[m]
			m_K = 166;
			m_Vmax = 65;							//Male Vmax
			m_w_Ex = 1.2;							//Female Vmac multiplication factor
			m_w_α = 0.6 * 3600 / 1000;				//km/(h Hz)
			//m_w_ascent = 0.6 * 3600 / 1000;		//[km/h]
			//m_w_ascent_σ = 0.2 * 3600 / 1000;		//[km/h] attention ici c'est 0.1 dans l'article
			m_w_horizontal = 2.0 * 3600 / 1000;		//[km/h]
			m_w_horizontal_σ = 0.5 * 3600 / 1000;	//[km/h] 
			m_w_descent = -2.0 * 3600 / 1000;		//[km/h]
			m_w_descent_σ = 1.0 * 3600 / 1000;		//[km/h]

			m_windS_stability_type = CWindStability::AUTO_DETECT;
			m_nb_weather_stations = 3;
			m_max_hour_load = 22;
			//m_nb_nearest_soundings = 4;
		}

		CATMParameters& operator =(const CATMParameters& in)
		{
			if (&in != this)
			{
				m_Tmin = in.m_Tmin;
				m_Tmax = in.m_Tmax;
				m_Pmax = in.m_Pmax;
				m_Wmin = in.m_Wmin;
				//m_t_liftoff_type = in.m_t_liftoff_type;
				//m_t_liftoff_begin = in.m_t_liftoff_begin;
				//m_t_liftoff_end = in.m_t_liftoff_end;
				m_t_liftoff_correction = in.m_t_liftoff_correction;
				m_t_liftoff_σ_correction = in.m_t_liftoff_σ_correction;
				//m_duration_type = in.m_duration_type;
				m_duration_min = in.m_duration_min;
				m_duration_max = in.m_duration_max;
				m_duration_α = in.m_duration_α;
				m_duration_β = in.m_duration_β; 
				m_cruise_duration = in.m_cruise_duration;
				m_cruise_height = in.m_cruise_height;
				//m_duration = in.m_duration;
				//m_duration_σ = in.m_duration_σ;
				m_height_type = in.m_height_type;
				//m_height_lo = in.m_height_lo;
				//m_height = in.m_height;
				//m_height_σ = in.m_height_σ;
				//m_height_hi = in.m_height_hi;
				//m_w_ascent = in.m_w_ascent;
				//m_w_ascent_σ = in.m_w_ascent_σ;
				m_K = in.m_K;
				m_Vmax = in.m_Vmax;
				m_w_Ex = in.m_w_Ex;
				m_w_α = in.m_w_α;
				m_w_horizontal = in.m_w_horizontal;
				m_w_horizontal_σ = in.m_w_horizontal_σ;
				m_w_descent = in.m_w_descent;
				m_w_descent_σ = in.m_w_descent_σ;

				m_windS_stability_type = in.m_windS_stability_type;
				m_nb_weather_stations = in.m_nb_weather_stations;
				m_max_hour_load = in.m_max_hour_load;
			}

			return *this;
		}

		bool operator ==(const CATMParameters& in)const
		{
			bool bEqual = true;

			if (m_Tmin != in.m_Tmin)bEqual = false;
			if (m_Tmax != in.m_Tmax)bEqual = false;
			if (m_Pmax != in.m_Pmax)bEqual = false;
			if (m_Wmin != in.m_Wmin)bEqual = false;
			//if (m_t_liftoff_type != in.m_t_liftoff_type)bEqual = false;
			//if (m_t_liftoff_begin != in.m_t_liftoff_begin)bEqual = false;
			//if (m_t_liftoff_end != in.m_t_liftoff_end)bEqual = false;
			if (m_t_liftoff_correction != in.m_t_liftoff_correction)bEqual = false;
			if (m_t_liftoff_σ_correction != in.m_t_liftoff_σ_correction)bEqual = false;
			//if (m_duration_type != in.m_duration_type)bEqual = false;
			//if (m_duration != in.m_duration)bEqual = false;
			//if (m_duration_σ != in.m_duration_σ)bEqual = false;
			
			if (m_duration_min != in.m_duration_min)bEqual = false;
			if (m_duration_max != in.m_duration_max)bEqual = false;
			if (m_duration_α != in.m_duration_α)bEqual = false;
			if (m_duration_β != in.m_duration_β)bEqual = false;
			if (m_cruise_duration != in.m_cruise_duration)bEqual = false;
			if (m_cruise_height != in.m_cruise_height)bEqual = false;
			if (m_height_type != in.m_height_type)bEqual = false;
			//if (m_height_lo != in.m_height_lo)bEqual = false;
			//if (m_height != in.m_height)bEqual = false;
			//if (m_height_σ != in.m_height_σ)bEqual = false;
			//if (m_height_hi != in.m_height_hi)bEqual = false;
			//if (m_w_ascent != in.m_w_ascent)bEqual = false;
			//if (m_w_ascent_σ != in.m_w_ascent_σ)bEqual = false;
			if (m_K != in.m_K)bEqual = false;
			if (m_Vmax != in.m_Vmax)bEqual = false;
			if (m_w_Ex != in.m_w_Ex)bEqual = false;
			if (m_w_α != in.m_w_α)bEqual = false;
			if (m_w_horizontal != in.m_w_horizontal)bEqual = false;
			if (m_w_horizontal_σ != in.m_w_horizontal_σ)bEqual = false;
			if (m_w_descent != in.m_w_descent)bEqual = false;
			if (m_w_descent_σ != in.m_w_descent_σ)bEqual = false;
			if (m_windS_stability_type != in.m_windS_stability_type)bEqual = false;
			if (m_nb_weather_stations != in.m_nb_weather_stations)bEqual = false;
			if (m_max_hour_load != in.m_max_hour_load)bEqual = false;
			
			return bEqual;
		}

		bool operator !=(const CATMParameters& in)const{ return !operator ==(in); }



	protected:

		static const char* MEMBERS_NAME[NB_MEMBERS];
	};



	//*****************************************************************************************************************
	class CATMVariables : public std::array < double, NB_ATM_VARIABLES >
	{
	public:

		CATMVariables()
		{
			fill(0);
		}

		CATMVariables& operator += (CATMVariables& in)
		{
			for (size_t i = 0; i < size(); i++)
				at(i) += in.at(i);

			return *this;
		}

		CATMVariables& operator *= (double in)
		{
			for (size_t i = 0; i < size(); i++)
				at(i) *= in;

			return *this;
		}

		CATMVariables& operator /= (double in)
		{
			for (size_t i = 0; i < size(); i++)
				at(i) /= in;

			return *this;
		}

		friend CATMVariables operator + (const CATMVariables& in1, const CATMVariables& in2)
		{
			CATMVariables out;
			for (size_t i = 0; i < in1.size(); i++)
				out[i] = in1[i] + in2[i];

			return out;
		}

		friend CATMVariables operator * (const CATMVariables& in1, double f)
		{
			CATMVariables out;
			for (size_t i = 0; i < in1.size(); i++)
				out[i] = in1[i] * f;

			return out;
		}

		friend CATMVariables operator / (const CATMVariables& in1, double f)
		{
			CATMVariables out;
			for (size_t i = 0; i < in1.size(); i++)
				out[i] = in1[i] / f;

			return out;
		}


		static double get_Uw(double p, double t, double ω);
		double get_wind_speed(bool b_used_z = false)const{ return   sqrt(at(ATM_WNDU) * at(ATM_WNDU) + at(ATM_WNDV) * at(ATM_WNDV) + (b_used_z ? (at(ATM_WNDW)*at(ATM_WNDW)) : 0)); }
		double get_wind_direction()const{ return fmod(5 * WBSF::PI / 2 - atan2(at(ATM_WNDV), at(ATM_WNDU)), 2 * WBSF::PI) / WBSF::PI * 180; }//a vérifier
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
		CATMVariables get_weather(const CGeoPoint3D& pt, __int64 time)const;

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


		double GetValue(int x, int y);

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
		ERMsg OpenInputImage(const std::string& filePath, bool bOpenInv = false);

		double GetPixel(const CGeoPoint3DIndex& index)const;
		double GetPixel(size_t b, const CGeoPointIndex& xy)const{ return GetPixel(CGeoPoint3DIndex(xy.m_x, xy.m_y, (int)b)); }
		bool IsCached(const CGeoBlock3DIndex& ijk)const{ assert(IsBlockInside(ijk));  return m_data[ijk.m_z][ijk.m_y][ijk.m_x] != NULL; }
		bool IsCached(size_t b, const CGeoBlockIndex& xy)const{ return IsCached(CGeoPoint3DIndex(xy.m_x, xy.m_y, (int)b)); }

		size_t get_band(size_t v, size_t level)const;
		//bool CGDALDatasetCached::convert_VVEL()const;
		//RasterIO
	protected:


		void InitCache()const
		{
			assert(IsOpen());
			if (!IsCacheInit())
			{
				const_cast<CGDALDatasetCached*>(this)->m_data.resize(boost::extents[GetRasterCount()][GetRasterYSize()][GetRasterXSize()]);
			}
		}

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

		//bool m_bConverVVEL_into_WNDW;

		std::array<std::array<size_t, NB_LEVELS>, NB_ATM_VARIABLES_EX> m_bands;
		//std::array<double, NB_LEVELS> m_pressure;
	};

	//*****************************************************************************************************************
	typedef std::map <CTRef, std::string > TRefFilePathMap;
	typedef std::shared_ptr<CGDALDatasetCached >CGDALDatasetCachedPtr;
	typedef std::map <CTRef, CGDALDatasetCachedPtr> TRefDatasetMapBase;
	class CTRefDatasetMap : protected TRefDatasetMapBase
	{
	public:

		int m_max_hour_load;

		CTRefDatasetMap();

		ERMsg load(CTRef TRef, const std::string& filePath, CCallback& callback)const;
		double GetPixel(CTRef TRef, const CGeoPoint3DIndex& index)const;
		double GetPixel(CTRef TRef, size_t b, const CGeoPointIndex& xy)const{ return GetPixel(TRef, CGeoPoint3DIndex(xy.m_x, xy.m_y, (int)b)); }
		const CGeoExtents& GetExtents(CTRef TRef)const;
		CGDALDatasetCachedPtr& Get(CTRef TRef);
		bool IsLoaded(CTRef TRef)const;
		ERMsg Discard(CCallback& callback);
		size_t get_band(CTRef TRef, size_t v, size_t level)const;
		bool convert_VVEL(CTRef TRef)const;
	};




	class CATMWeather
	{
	public:

		enum TGribType{ RUC_TYPE, WRF_TYPE };

		CATMWeather(CATMWorld& world) :
			m_world(world)
		{
			m_bSkipDay = false;
		}

		ERMsg Load(const std::string& gribsFilepath, const std::string& hourlyDBFilepath, CCallback& callback);
		ERMsg load_gribs(const std::string& filepath, CCallback& callback);
		ERMsg load_hourly(const std::string& filepath, CCallback& callback);
		ERMsg Discard(CCallback& callback);


		ERMsg LoadWeather(CTRef TRef, CCallback& callback);
		CATMWeatherCuboidsPtr get_cuboids(const CGeoPoint3D& pt, CTRef UTCTRef, __int64 UTCTime)const;
		CATMVariables get_station_weather(const CGeoPoint3D& pt, CTRef UTCTRef, __int64 UTCTime)const;
		CATMVariables get_station_weather(const CGeoPoint3D& pt, CTRef UTCTRef)const;
		CATMVariables get_weather(const CGeoPoint3D& pt, CTRef UTCTRef, __int64 UTCTime)const;
		std::string get_image_filepath(CTRef TRef)const;

		CGeoPoint3DIndex get_xyz(const CGeoPoint3D& pt, CTRef UTCTRef)const;
		
		CGeoPointIndex get_xy(const CGeoPoint& pt, CTRef UTCTRef)const;
		int get_level(const CGeoPointIndex& xy, double alt, CTRef UTCTRef, bool bLow)const;
		int get_level(const CGeoPointIndex& xy, double alt, CTRef UTCTRef)const;
		double GetGroundAltitude(const CGeoPointIndex& xy, CTRef UTCTRef)const;

		bool is_init()const{ return !m_filepath_map.empty() || m_p_hourly_DB != NULL; }

		size_t GetPrjID()const{ return m_extents.GetPrjID(); }
		CGDALDatasetCachedPtr& Get(CTRef TRef) { return m_p_weather_DS.Get(TRef); }
		bool IsLoaded(CTRef TRef)const;

		const CGeoExtents& GetExtents()const{ return  m_extents; }
		static double LandWaterWindFactor(double Ul, double ΔT);
		static void GetWindProfileRelationship(double& Ur, double& Vr, double z, int stabType, bool bOverWather, double ΔT);

		void ResetSkipDay(){ m_bSkipDay = false; }
		bool SkipDay()const{ return  m_bSkipDay; }

		bool HaveGribsWeather()const{ return !m_filePathGribs.empty(); }
		bool HaveStationWeather()const{ return !m_filePathHDB.empty(); }

	protected:

		std::string m_filePathGribs;
		std::string m_filePathHDB;

		TRefFilePathMap m_filepath_map;
		CHourlyDatabasePtr m_p_hourly_DB;
		CTRefDatasetMap m_p_weather_DS;
		CGeoExtents m_extents;


		std::map<size_t, CWeatherStation> m_stations;
		std::map<size_t, CWaterTemperature> m_Twater;
		std::map<CTRef, std::array<CIWD, NB_ATM_VARIABLES>> m_iwd;
		CATMWorld& m_world;
		bool m_bSkipDay;
	};


	//target parameters
	class CFlightParameters
	{
	public:

		double m_M;				//dry weight [g]
		double m_A;				//Forewing area [cm²]
		double m_p_exodus;		//exodus probability

		double m_w_horizontal;	//horizontal flight speed [m/s]
		double m_w_descent;		//descent flight speed [m/s]
		__int64 m_duration;		//total flight duration [s]
		__int64 m_cruise_duration;//cruise duration[s]
		__int64 m_t_liftoff;	//launch time in evening
		

		CFlightParameters()
		{
			m_A = 0;
			m_M = 0;
			m_p_exodus = 0;
			
			m_w_horizontal = 0;
			m_w_descent = 0;
			m_t_liftoff = 0;
			m_duration = 0;
			m_cruise_duration = 0;
		}

	};


	class CFlyer
	{
	public:

		//T_HUNTING, 
		enum TLog{ T_CREATION, T_LIFTOFF, T_CRUISE, T_LANDING, T_IDLE_END, T_DESTROY, NB_FLYER_LOG };
		enum TStat{ S_TAIR, S_PRCP, S_U, S_V, S_W, S_D_X, S_D_Y, S_D_Z, S_DISTANCE, S_HEIGHT, S_W_HORIZONTAL, S_W_VERTICAL, NB_FLYER_STAT };//S_W_ASCENT, 

		enum TStates{ NOT_CREATED, IDLE_BEGIN, LIFTOFF, FLIGHT, CRUISE, LANDING, IDLE_END, DESTROYED, NB_STATES };
		enum TEnd{ NO_END_DEFINE, NO_LIFTOFF, END_BY_RAIN, END_BY_TAIR, END_BY_WNDS, END_OF_TIME_FLIGHT, FIND_HOST, FIND_DISTRACTION, OUTSIDE_MAP, OUTSIDE_TIME_WINDOW, NB_END_TYPE };
		// 
		size_t m_rep;
		size_t m_loc;
		size_t m_var;
		double m_scale;
		size_t m_sex;			//sex (MALE=0, FEMALE=1)
		double m_G;				//gravid=1, spent=0, male=0
		CTRef m_localTRef;		//Creation date in local time
		CLocation m_location;	//initial position
		CLocation m_newLocation;//actual position, z is elevation over sea level
		CGeoPoint3D m_pt;		//same as m_newLocation but with flight height instead of elevation over sea level

		CFlyer(CATMWorld& world);

		void init();
		void live();
		void create(CTRef UTCTRef, __int64 UTCTime);
		void idle_begin(CTRef UTCTRef, __int64 time);
		void liftoff(CTRef UTCTRef, __int64 UTCTime);
		void flight(CTRef UTCTRef, __int64 UTCTime);
		void cruise(CTRef UTCTRef, __int64 UTCTime);
		void landing(CTRef UTCTRef, __int64 UTCTime);
		void idle_end(CTRef UTCTRef, __int64 UTCTime);
		void destroy(CTRef UTCTRef, __int64 UTCTime);

		const CGeoPoint3D& get_pt()const{ return m_pt; }

		CATMVariables get_weather(CTRef UTCTRef, __int64 UTCTime)const;
		CGeoDistance3D get_U(__int64 UTCTime, const CATMVariables& w)const;
		CGeoDistance3D get_U(__int64 UTCTime)const;
		double get_Uz(__int64 UTCTime, const CATMVariables& w)const;
		
		//double get_Uz(double T)const; 
		
		double get_Vᵀ(double T)const;
		double get_Tᴸ()const;

		static double get_Vᵀ(size_t sex, double Vmax, double T);
		static double get_Tᴸ(size_t sex, double K, double Vmax, double A, double M);

		double GetLog(size_t i)const{ return m_log[i]; }
		double GetStat(size_t i, size_t s=MEAN)const{ return m_stat[i].IsInit() ? m_stat[i][s] : 0; }
		const CStatistic& operator[](size_t i)const{ return m_stat[i]; }
		void ResetStat(){ m_stat.fill(CStatistic()); }
		int GetState()const{ return m_state; }
		int GetEnd()const{ return m_end_type; }
		const CFlightParameters& P()const{ return m_parameters; }
		int GetUTCShift()const{ return int(m_UTCShift / 3600); }//in [h]
		double get_MRatio()const;

	protected:

		void AddStat(const CATMVariables& w, const CGeoDistance3D& U, const CGeoDistance3D& d);


		__int64 m_creation_time;//creation time in second
		__int64 m_UTCShift;//shift between local time and UTC [s]

		CFlightParameters m_parameters;

		int m_state;		//state of the flyer
		int m_end_type;		//Terminason of the flyer
		CATMWorld& m_world;

		//statistic of flight
		std::array<double, NB_FLYER_LOG> m_log;
		std::array<CStatistic, NB_FLYER_STAT> m_stat;


		
		static const double b[2];
		static const double c[2];

	};


	typedef std::deque <CFlyer> CFlyers;
	typedef CFlyers::iterator CFlyersIt;
	typedef CFlyers::const_iterator CFlyersCIt;


	class CATMWorldParamters
	{
	public:

		//static public member 
		enum TweatherType{ FROM_GRIBS, FROM_STATIONS, FROM_BOTH, NB_WEATHER_TYPE };
		enum TMember{ WEATHER_TYPE, PERIOD, TIME_STEP, SEED, REVERSED, USE_SPACE_INTERPOL, USE_TIME_INTERPOL, USE_PREDICTOR_CORRECTOR_METHOD, USE_TURBULANCE, USE_VERTICAL_VELOCITY, EVENT_THRESHOLD, DEFOL_THRESHOLD, DISTRACT_THRESHOLD, HOST_THRESHOLD, DEM, WATER, GRIBS, HOURLY_DB, DEFOLIATION, DISTRACTION, HOST, NB_MEMBERS };
		static const char* GetMemberName(int i){ ASSERT(i >= 0 && i < NB_MEMBERS); return MEMBERS_NAME[i]; }

		//public member
		int m_weather_type;	//weather type: stations hourly databse or gridded NCEP multi-layer grid
		CTPeriod m_simulationPeriod;
		size_t m_time_step;	//time step in [s]
		size_t m_seed;
		bool m_bReversed;
		bool m_bUseSpaceInterpolation;
		bool m_bUseTimeInterpolation;
		bool m_bUsePredictorCorrectorMethod;
		bool m_bUseTurbulance;
		bool m_bUseVerticalVelocity;


		double m_eventThreshold;
		double m_defoliationThreshold;
		double m_distractionThreshold;
		double m_hostThreshold;


		std::string m_gribs_name; //filepath on the grib file list
		std::string m_defoliation_name;
		std::string m_host_name;
		std::string m_distraction_name;
		std::string m_hourly_DB_name;
		std::string m_DEM_name;
		std::string m_water_name;
		

		size_t get_time_step()const{ return m_time_step; }//[s]
		size_t get_nb_time_step()const{ assert(3600 % m_time_step == 0); return size_t(3600 / m_time_step); }	//number of timeStep per hour

		bool UseGribs()const{ return m_weather_type == FROM_GRIBS || m_weather_type == FROM_BOTH; }
		bool UseHourlyDB()const{ return m_weather_type == FROM_STATIONS || m_weather_type == FROM_BOTH; }

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
			m_bReversed = false;
			m_bUseSpaceInterpolation = true;
			m_bUseTimeInterpolation = true;
			m_bUsePredictorCorrectorMethod = true;
			m_bUseTurbulance = false;
			m_bUseVerticalVelocity = true;


			m_eventThreshold = 0;
			m_defoliationThreshold = 20;
			m_distractionThreshold = 90;
			m_hostThreshold = 40;

			m_gribs_name.clear();
			m_defoliation_name.clear();
			m_host_name.clear();
			m_distraction_name.clear();
			m_hourly_DB_name.clear();
			m_DEM_name.clear();
			m_water_name.clear();

		}


		CATMWorldParamters& operator =(const CATMWorldParamters& in)
		{
			if (&in != this)
			{
				m_weather_type = in.m_weather_type;
				m_simulationPeriod = in.m_simulationPeriod;
				m_time_step = in.m_time_step;
				m_seed = in.m_seed;
				m_bReversed = in.m_bReversed;
				m_bUseSpaceInterpolation = in.m_bUseSpaceInterpolation;
				m_bUseTimeInterpolation = in.m_bUseTimeInterpolation;
				m_bUsePredictorCorrectorMethod = in.m_bUsePredictorCorrectorMethod;
				m_bUseTurbulance = in.m_bUseTurbulance;
				m_bUseVerticalVelocity = in.m_bUseVerticalVelocity;

				m_eventThreshold = in.m_eventThreshold;
				m_defoliationThreshold = in.m_defoliationThreshold;
				m_distractionThreshold = in.m_distractionThreshold;
				m_hostThreshold = in.m_hostThreshold;

				m_gribs_name = in.m_gribs_name;
				m_defoliation_name = in.m_defoliation_name;
				m_host_name = in.m_host_name;
				m_distraction_name = in.m_distraction_name;
				m_hourly_DB_name = in.m_hourly_DB_name;
				m_DEM_name = in.m_DEM_name;
				m_water_name = in.m_water_name;
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
			if (m_distractionThreshold != in.m_distractionThreshold)bEqual = false;
			if (m_bUseSpaceInterpolation != in.m_bUseSpaceInterpolation)bEqual = false;
			if (m_bUseTimeInterpolation != in.m_bUseTimeInterpolation)bEqual = false;
			if (m_bUsePredictorCorrectorMethod != in.m_bUsePredictorCorrectorMethod)bEqual = false;
			if (m_bUseTurbulance != in.m_bUseTurbulance)bEqual = false;
			if (m_bUseVerticalVelocity != in.m_bUseVerticalVelocity)bEqual = false;
			if (m_eventThreshold != in.m_eventThreshold)bEqual = false;
			if (m_defoliationThreshold != in.m_defoliationThreshold)bEqual = false;
			if (m_distractionThreshold != in.m_distractionThreshold)bEqual = false;
			if (m_hostThreshold != in.m_hostThreshold)bEqual = false;

			if (m_gribs_name != in.m_gribs_name)bEqual = false;
			if (m_defoliation_name != in.m_defoliation_name)bEqual = false;
			if (m_host_name != in.m_host_name)bEqual = false;
			if (m_distraction_name != in.m_distraction_name)bEqual = false;
			if (m_hourly_DB_name != in.m_hourly_DB_name)bEqual = false;
			if (m_DEM_name != in.m_DEM_name)bEqual = false;
			if (m_water_name != in.m_water_name)bEqual = false;


			return bEqual;
		}

		bool operator !=(const CATMWorldParamters& in)const{ return !operator ==(in); }

	protected:


		static const char* MEMBERS_NAME[NB_MEMBERS];
	};


	class CATMWorld
	{
	public:



		//return the number of second since 1 december of year 1
		size_t get_time_step()const{ return m_parameters1.get_time_step(); }//[s]
		size_t get_nb_time_step()const{ return m_parameters1.get_nb_time_step(); }	//number of timeStep per hour
		CTRef GetUTRef()const{ return m_UTCTRef; }
		__int64 get_UTC_time()const { return m_UTCTTime; }
		static __int64 get_local_sunset(CTRef TRef, const CLocation& loc);
		CATMVariables get_weather(const CGeoPoint3D& pt, CTRef UTCTRef, __int64 UTCTime)const{ return m_weather.get_weather(pt, UTCTRef, UTCTime); }
		

		std::vector<CFlyersIt> GetFlyers(CTRef TRef);
		CTPeriod get_UTC_period(const std::vector<CFlyersIt>& fls);

		int m_nb_max_threads;
		CATMWorldParamters m_parameters1;
		CATMParameters m_parameters2;


		CATMWeather m_weather;
		CFlyers m_flyers;
		CGDALDatasetCached m_DEM_DS;
		CGDALDatasetCached m_defoliation_DS;
		CGDALDatasetCached m_host_DS;
		CGDALDatasetCached m_distraction_DS;
		CGDALDatasetCached m_water_DS;


		double GetGroundAltitude(const CGeoPoint3D& pt)const;
		bool is_over_defoliation(const CGeoPoint3D& pt)const;
		bool is_over_distraction(const CGeoPoint3D& pt)const;
		bool is_over_host(const CGeoPoint3D& pt)const;

		ERMsg Execute(CATMOutputMatrix& output, CCallback& callback);

		CATMWorld() :
			m_weather(*this)//pas sure...
		{
			m_nb_max_threads = 1;
		}

		CRandomGenerator& random(){ return m_random; }
		std::set<int> get_years()const;
		CTPeriod get_period(bool bUTC = true, int year = YEAR_NOT_INIT)const;
		std::set<CTRef> get_TRefs(int year)const;

		const CStatistic& GetStats(size_t i)const{ return  m_stats[i]; }


		CProjectionTransformation m_GEO2DEM;
		CProjectionTransformation m_GEO2WEA;
		CProjectionTransformation m_WEA2GEO;


		void get_t(const CLocation& loc, __int64 UTCSunset, __int64 &t°, __int64 &tᴹ)const;
		double get_Tᵀ(const CLocation& loc, __int64 UTCSunset, __int64 t°, __int64 tᴹ)const;
		__int64 get_t_liftoff_offset(__int64 t°, __int64 tᴹ)const;
		__int64 get_duration()const;

		
		size_t get_S()const;
		double get_G(size_t sex)const;
		double get_A(size_t sex)const;
		double get_M(size_t sex, double A)const;
		double get_M(size_t sex, double A, double G)const;
		
		double get_w_horizontal()const;
		double get_w_descent()const;
		bool IsInside(const CGeoPoint& pt)const;

	protected:

		CTRef m_UTCTRef;
		__int64 m_UTCTTime;
		CRandomGenerator m_random;
		static const double Δtᶠ;
		static const double Δtᶳ;
		static const double T°;


		//statistic of simulation
		std::array<CStatistic, 1 > m_stats;
	};

}


namespace zen
{


	template <> inline
		void writeStruc(const WBSF::CATMParameters& in, XmlElement& output)
	{
		XmlOut out(output);
		out[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::T_MIN)](in.m_Tmin);
		out[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::T_MAX)](in.m_Tmax);
		out[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::P_MAX)](in.m_Pmax);
		out[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::W_MIN)](in.m_Wmin);
		out[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::LIFTOFF_CORRECTION)](in.m_t_liftoff_correction);
		out[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::LIFTOFF_SIGMA)](in.m_t_liftoff_σ_correction);
		out[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::HEIGHT_TYPE)](in.m_height_type);
		out[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::DURATION_MIN)](in.m_duration_min);
		out[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::DURATION_MAX)](in.m_duration_max);
		out[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::DURATION_ALPHA)](in.m_duration_α);
		out[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::DURATION_BETA)](in.m_duration_β);
		out[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::CRUISE_DURATION)](in.m_cruise_duration);
		out[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::CRUISE_HEIGHT)](in.m_cruise_height);
		out[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::WING_BEAT_K)](in.m_K);
		out[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::WING_BEAT_VMAX)](in.m_Vmax);
		out[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::WING_BEAT_EX)](in.m_w_Ex);
		out[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::WING_BEAT_ALPHA)](in.m_w_α);
		out[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::W_HORZ_SD)](in.m_w_horizontal_σ);
		out[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::W_DESCENT)](in.m_w_descent);
		out[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::W_DESCENT_SD)](in.m_w_descent_σ);
		out[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::WIND_STABILITY)](in.m_windS_stability_type);
		out[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::NB_WEATHER_STATIONS)](in.m_nb_weather_stations);
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CATMParameters& out)
	{
		XmlIn in(input);
		in[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::T_MIN)](out.m_Tmin);
		in[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::T_MAX)](out.m_Tmax);
		in[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::P_MAX)](out.m_Pmax);
		in[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::W_MIN)](out.m_Wmin);
		in[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::LIFTOFF_CORRECTION)](out.m_t_liftoff_correction);
		in[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::LIFTOFF_SIGMA)](out.m_t_liftoff_σ_correction);
		in[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::HEIGHT_TYPE)](out.m_height_type);
		in[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::DURATION_MIN)](out.m_duration_min);
		in[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::DURATION_MAX)](out.m_duration_max);
		in[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::DURATION_ALPHA)](out.m_duration_α);
		in[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::DURATION_BETA)](out.m_duration_β);
		in[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::CRUISE_DURATION)](out.m_cruise_duration);
		in[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::CRUISE_HEIGHT)](out.m_cruise_height);
		in[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::WING_BEAT_K)](out.m_K);
		in[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::WING_BEAT_VMAX)](out.m_Vmax);
		in[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::WING_BEAT_EX)](out.m_w_Ex);
		in[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::WING_BEAT_ALPHA)](out.m_w_α);
		in[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::W_HORZ)](out.m_w_horizontal);
		in[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::W_HORZ_SD)](out.m_w_horizontal_σ);
		in[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::W_DESCENT)](out.m_w_descent);
		in[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::W_DESCENT_SD)](out.m_w_descent_σ);
		in[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::WIND_STABILITY)](out.m_windS_stability_type);
		in[WBSF::CATMParameters::GetMemberName(WBSF::CATMParameters::NB_WEATHER_STATIONS)](out.m_nb_weather_stations);

		return true;
	}




	template <> inline
		void writeStruc(const WBSF::CATMWorldParamters& in, XmlElement& output)
	{
		XmlOut out(output);

		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::WEATHER_TYPE)](in.m_weather_type);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::PERIOD)](in.m_simulationPeriod);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::TIME_STEP)](in.m_time_step);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::SEED)](in.m_seed);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::REVERSED)](in.m_bReversed);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::USE_SPACE_INTERPOL)](in.m_bUseSpaceInterpolation);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::USE_TIME_INTERPOL)](in.m_bUseTimeInterpolation);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::USE_PREDICTOR_CORRECTOR_METHOD)](in.m_bUsePredictorCorrectorMethod);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::USE_TURBULANCE)](in.m_bUseTurbulance);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::USE_VERTICAL_VELOCITY)](in.m_bUseVerticalVelocity);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::EVENT_THRESHOLD)](in.m_eventThreshold);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::DEFOL_THRESHOLD)](in.m_defoliationThreshold);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::DISTRACT_THRESHOLD)](in.m_distractionThreshold);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::HOST_THRESHOLD)](in.m_hostThreshold);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::GRIBS)](in.m_gribs_name);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::DEFOLIATION)](in.m_defoliation_name);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::HOST)](in.m_host_name);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::DISTRACTION)](in.m_distraction_name);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::HOURLY_DB)](in.m_hourly_DB_name);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::DEM)](in.m_DEM_name);
		out[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::WATER)](in.m_water_name);
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CATMWorldParamters& out)
	{
		XmlIn in(input);

		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::WEATHER_TYPE)](out.m_weather_type);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::PERIOD)](out.m_simulationPeriod);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::TIME_STEP)](out.m_time_step);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::SEED)](out.m_seed);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::REVERSED)](out.m_bReversed);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::USE_SPACE_INTERPOL)](out.m_bUseSpaceInterpolation);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::USE_TIME_INTERPOL)](out.m_bUseTimeInterpolation);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::USE_PREDICTOR_CORRECTOR_METHOD)](out.m_bUsePredictorCorrectorMethod);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::USE_TURBULANCE)](out.m_bUseTurbulance);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::USE_VERTICAL_VELOCITY)](out.m_bUseVerticalVelocity);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::EVENT_THRESHOLD)](out.m_eventThreshold);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::DEFOL_THRESHOLD)](out.m_defoliationThreshold);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::DISTRACT_THRESHOLD)](out.m_distractionThreshold);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::HOST_THRESHOLD)](out.m_hostThreshold);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::GRIBS)](out.m_gribs_name);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::DEFOLIATION)](out.m_defoliation_name);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::HOST)](out.m_host_name);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::DISTRACTION)](out.m_distraction_name);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::HOURLY_DB)](out.m_hourly_DB_name);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::DEM)](out.m_DEM_name);
		in[WBSF::CATMWorldParamters::GetMemberName(WBSF::CATMWorldParamters::WATER)](out.m_water_name);

		return true;
	}

}
