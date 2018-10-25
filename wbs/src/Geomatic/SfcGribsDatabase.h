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
#include "Basic/FileStamp.h"
//#include "Basic/ModelStat.h"
//#include "Basic/WaterTemperature.h"
//#include "Geomatic/IWD.h"
#include "Geomatic/GDALBasic.h"
#include "Geomatic/ProjectionTransformation.h"
#include "Geomatic/TimeZones.h"

extern const char* WBSF_ATM_VERSION;


namespace WBSF
{

	//inline size_t GetHourlySeconds(size_t time)
	//{
	//	return fmod(time, 3600);
	//}

	//*****************************************************************************************************************
	class CGribsDB : public std::map<CTRef, std::string>
	{
	public:

		ERMsg load(const std::string& file_path);
		ERMsg save(const std::string& file_path)const;

		CTPeriod GetEntireTPeriod()const;
		std::set<CTRef> GetAllTRef()const;
	};

	//*****************************************************************************************************************
	class CIncementalDB : public std::map<CTRef, CFileStamp>
	{
	public:
		ERMsg load(const std::string& file_path);
		ERMsg save(const std::string& file_path)const;

		//ERMsg GetInvalidPeriod(const std::string& gribs_file_path, CTPeriod& p_invalid)const;
		ERMsg GetInvalidPeriod(const CGribsDB& gribs, CTPeriod& p_invalid)const;
		ERMsg GetInvalidTRef(const CGribsDB& gribs, std::set<CTRef>& invalid)const;
		ERMsg Update(const CGribsDB& gribs);
	};



	//*****************************************************************************************************************
	class CSfcVariableLine
	{
	public:

		CSfcVariableLine(size_t nXBlockSize, int dataType);

		~CSfcVariableLine()
		{
			delete[] m_ptr;
		}


		//double GetValue(int x)const;
		//void SetValue(int x, double value);

		double get_value(size_t x)const;
		void set_value(size_t x, double value);

		void* m_ptr;
		size_t m_xBlockSize;
		int m_dataType;
	};

	
	typedef std::shared_ptr<CSfcVariableLine> CSfcVariableLinePtr;
	typedef std::array < CSfcVariableLinePtr, HOURLY_DATA::NB_VAR_EX> CSfcWeatherLine;
	typedef std::shared_ptr<CSfcWeatherLine> CSfcWeatherLinePtr;
	typedef std::array<std::array< CHourlyData, 2>, 2> CHourlyData4;

	//typedef boost::multi_array <CSfcDataLinePtr, 2> CSfcDataCache;
	//all gribs seem to have one line block
	//so the number of blocq equl the number of line (y)
	
	typedef std::deque < CSfcWeatherLinePtr> CSfcWeatherData; //
	class CSfcDatasetCached : public CGDALDatasetEx
	{
	public:

		static size_t get_var(const std::string& strVar);
		static bool is_sfc(const std::string& strLevel);

		CSfcDatasetCached();
		ERMsg open(const std::string& filePath, bool bOpenInv = false);
		void close();

		void get_weather(const CGeoPointIndex& index, CHourlyData& data)const;
		float get_variable(const CGeoPointIndex& index, size_t v)const;
		bool is_cached(size_t y)const { assert(is_block_inside(y));  return m_lines[y] != NULL; }
		size_t get_band(size_t v)const { return m_bands[v]; }
		void set_variables(std::bitset< HOURLY_DATA::NB_VAR_EX> in) { m_variables = in; }


		void get_weather(const CGeoPoint& pt, CHourlyData& data)const;
		CGeoPointIndex get_ul(const CGeoPoint& ptIn)const;
		void get_nearest(const CGeoPoint& pt, CHourlyData& data)const;
		void get_4nearest(const CGeoPoint& pt, CHourlyData4& data4)const;

	protected:


		void init_cache()const;
		bool is_cache_init()const { return !m_lines.empty(); }
		//void LoadBlock(size_t b, const CGeoBlockIndex& xy){ LoadBlock(CGeoBlockIndex(xy.m_x, xy.m_y, (int)b)); }
		void load_block(size_t y);
		bool is_block_inside(size_t y)const { return y < m_lines.size(); }

		std::bitset< HOURLY_DATA::NB_VAR_EX> m_variables;
		std::array<size_t, HOURLY_DATA::NB_VAR_EX> m_bands;
		CSfcWeatherData m_lines;
		//static std::mutex m_mutex;
	};

	//*****************************************************************************************************************
	//typedef std::map <CTRef, std::string > TTRefFilePathMap;
	//typedef std::shared_ptr<CSfcDatasetCached >CSfcDatasetCachedPtr;
	//typedef std::map <CTRef, CSfcDatasetCachedPtr> CSfcDatasetMapBase;

	//class CSfcDatasetMap : public CSfcDatasetCached
	//{
	//public:

	//	CSfcDatasetMap();

	//	//ERMsg load(CTRef TRef, const std::string& filePath, CCallback& callback)const;
	//	void get_weather(CTRef TRef, const CGeoPointIndex& xy, CHourlyData& data)const;
	//	const CGeoExtents& get_extents(CTRef TRef)const;
	//	//bool is_loaded(CTRef TRef)const;
	//	//ERMsg discard(CCallback& callback);
	//	size_t get_band(CTRef TRef, size_t v)const;
	//	//size_t get_prj_ID(CTRef TRef)const;
	//};


//

//	class CSfcWeather
//	{
//	public:
//
//		CSfcWeather()
//		{
//			//m_bHgtOverSea = false;
//			//m_bHgtOverSeaTested = false;
//		}
//
//		ERMsg load(const std::string& filepath, CCallback& callback);
//		ERMsg discard(CCallback& callback);
//
//		ERMsg load_weather(CTRef TRef, CCallback& callback);
//		
//		//void get_weather(const CGeoPoint& pt, CTRef TRef, CHourlyData4& data)const;
//		void get_weather(const CGeoPoint& pt, CTRef TRef, CHourlyData& data, bool bAtLocation)const;
//		std::string get_image_filepath(CTRef TRef)const;
//
//		CGeoPointIndex get_ul(const CGeoPoint& pt, CTRef UTCWeatherTime)const;
//		//double GetFirstAltitude(const CGeoPointIndex& xy, CTRef TRef)const;
//
//		bool is_open()const { return !m_file_path_gribs.empty(); }
//		size_t get_prj_ID()const;
//
//
////		CSfcDatasetCachedPtr& at(CTRef TRef) { return m_p_weather_DS.at(TRef); }
//		//bool is_loaded(CTRef TRef)const;
//
//		CTPeriod GetEntireTPeriod()const;
//
//		void set_variables(std::bitset< HOURLY_DATA::NB_VAR_EX> in) { m_variables = in; }
//
//	protected:
//
//		std::string m_file_path_gribs;
//		
//
//		//TTRefFilePathMap m_filepath_map;
//		CSfcDatasetMap m_p_weather_DS;
//		std::bitset< HOURLY_DATA::NB_VAR_EX> m_variables;
//
//		//bool m_bHgtOverSea;
//		//bool m_bHgtOverSeaTested;
//	};
//

	/*

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
		*/
}
