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
#include "Geomatic/GDALBasic.h"
#include "Geomatic/ProjectionTransformation.h"


namespace WBSF
{

	enum TGribsVariables { H_GHGT = HOURLY_DATA::NB_VAR_H, H_UWND, H_VWND, H_DSWR, H_DLWR, NB_VAR_GRIBS };
	typedef std::array<std::array< CHourlyData, 2>, 2> CHourlyData4;
	typedef std::bitset< NB_VAR_GRIBS> GribVariables;



	//*****************************************************************************************************************
	class CGribsMap : public std::map<CTRef, std::vector<std::string>>
	{
	public:

		ERMsg load(const std::string& file_path);
		ERMsg save(const std::string& file_path)const;

		CTPeriod GetEntireTPeriod()const;
		std::set<CTRef> GetAllTRef()const;

		std::string get_file_path()const {	return m_file_path;}

	protected:
		std::string m_file_path;
	};

	//*****************************************************************************************************************
	class CIncementalDB : public std::map<CTRef, std::vector<CFileStamp>>
	{
	public:
		ERMsg load(const std::string& file_path);
		ERMsg save(const std::string& file_path)const;

		//ERMsg GetInvalidPeriod(const std::string& gribs_file_path, CTPeriod& p_invalid)const;
		ERMsg GetInvalidPeriod(const CGribsMap& gribs, CTPeriod& p_invalid)const;
		ERMsg GetInvalidTRef(const CGribsMap& gribs, std::set<CTRef>& invalid)const;
		ERMsg Update(const CGribsMap& gribs);
		static ERMsg Delete(const std::string& file_path);

		std::string m_grib_file_path;
		std::string m_loc_file_path;
		size_t m_nb_points;
		GribVariables m_variables;
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
	typedef std::array < CSfcVariableLinePtr, NB_VAR_GRIBS> CSfcWeatherLine;
	typedef std::shared_ptr<CSfcWeatherLine> CSfcWeatherLinePtr;
	
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
		const GribVariables& get_variables()const { return m_variables; }
		void set_variables(GribVariables& in) 
		{ 
			if (in != m_variables)
			{
				m_variables = in;
				//if open, reset band that is not selected
				if (IsOpen())
				{
					ASSERT(m_bands.size() == m_variables.size());
					for (size_t v = 0; v < m_bands.size(); v++)
					{
						if (!m_variables.test(v))
						{
							m_bands[v] = NOT_INIT;
						}
					}
				}
			}
		}
		


		void get_weather(const CGeoPoint& pt, CHourlyData& data)const;
		CGeoPointIndex get_ul(const CGeoPoint& ptIn)const;
		void get_nearest(const CGeoPoint& pt, CHourlyData& data)const;
		void get_4nearest(const CGeoPoint& pt, CHourlyData4& data4)const;

		CLocationVector get_nearest(const CLocationVector& location, size_t nb_points)const;
			protected:


		void init_cache()const;
		bool is_cache_init()const { return !m_lines.empty(); }
		void load_block(size_t y);
		bool is_block_inside(size_t y)const { return y < m_lines.size(); }

		GribVariables m_variables;
		std::array<size_t, NB_VAR_GRIBS> m_bands;
		std::array<std::string, NB_VAR_GRIBS> m_units;
		std::array<float, NB_VAR_GRIBS> m_noData;
		 
		CSfcWeatherData m_lines;
		//static std::mutex m_mutex;
	};

	class CSfcGribDatabase : public CDHDatabaseBase
	{
	public:

		static const int   VERSION;
		static const char* XML_FLAG;
		static const char* DATABASE_EXT;
		static const char* OPT_EXT;
		static const char* DATA_EXT;
		static const char* META_EXT;
		static const CTM   DATA_TM;
		virtual int GetVersion()const { return VERSION; }
		virtual const char* GetXMLFlag()const { return XML_FLAG; }
		virtual const char* GetDatabaseExtension()const { return DATABASE_EXT; }
		virtual const char* GetOptimizationExtension()const { return OPT_EXT; }
		virtual const char* GetDataExtension()const { return DATA_EXT; }
		virtual const char* GetHeaderExtension()const { return META_EXT; }
		virtual const CTM	GetDataTM()const { return DATA_TM; }
		virtual const char	GetDBType()const { return 'H'; }

		size_t m_nb_points;
		bool m_bIncremental;
		GribVariables m_variables;
		int m_nbMaxThreads;

		CSfcGribDatabase(int cacheSize = 200) : CDHDatabaseBase(cacheSize)
		{
			m_nb_points = 0;
			m_bIncremental = true;
			m_variables.set();
			m_nbMaxThreads = 1;
		}

		static int GetVersion(const std::string& filePath);
		static ERMsg CreateDatabase(const std::string& filePath);
		static ERMsg DeleteDatabase(const std::string& filePath, CCallback& callback = DEFAULT_CALLBACK);
		static ERMsg RenameDatabase(const std::string& inputFilePath, const std::string& outputFilePath, CCallback& callback = DEFAULT_CALLBACK);
		static GribVariables get_var(CWVariables m_variables);

		ERMsg Update(const CGribsMap& gribs, const CLocationVector& locations, CCallback& callback);
		ERMsg ExtractStation(CTRef TRef, const std::string& file_path, CWeatherStationVector& stations, CCallback& callback);
	};

	typedef std::shared_ptr<CSfcGribDatabase> CSfcGribDatabasePtr;
	
}


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

