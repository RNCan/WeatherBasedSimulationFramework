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
	enum TMetaField { M_DESC, M_COMMENT, M_ELEMENT, M_SHORT_NAME, M_UNIT, NB_META };
	enum TGribsVariables { H_GHGT = HOURLY_DATA::NB_VAR_H, H_UWND, H_VWND, H_DSWR, H_DLWR, H_SPFH, H_PRATE, NB_VAR_GRIBS };
	typedef std::array<std::array< CHourlyData, 2>, 2> CHourlyData4;
	typedef std::array<std::array< CWeatherDay, 2>, 2> CDailyData4;
	
	typedef std::bitset< NB_VAR_GRIBS> GribVariables;



	//*****************************************************************************************************************
	class CGribsMap : public std::map<CTRef, std::string>
	{
	public:

		ERMsg load(const std::string& file_path);
		ERMsg save(const std::string& file_path)const;

		CTPeriod GetEntireTPeriod()const;
		std::set<CTRef> GetAllTRef()const;

		std::string get_file_path()const { return m_file_path; }

	protected:
		std::string m_file_path;
	};

	//*****************************************************************************************************************
	class CIncementalDB : public std::map<CTRef, CFileStamp>
	{
	public:

		void clear()
		{
			std::map<CTRef, CFileStamp>::clear();
			m_grib_file_path.clear();
			m_loc_file_path.clear();
			m_nb_points=0;
			m_variables.reset();
			m_period.clear();
			m_locations.clear();
		}

		ERMsg load(const std::string& file_path);
		ERMsg save(const std::string& file_path)const;

		
		ERMsg GetInvalidPeriod(const CGribsMap& gribs, const CTPeriod& period, CTPeriod& p_invalid)const;
		ERMsg GetInvalidTRef(const CGribsMap& gribs, const CTPeriod& period, std::set<CTRef>& invalid)const;
		ERMsg Update(const CGribsMap& gribs);
		static ERMsg Delete(const std::string& file_path);

		std::string m_grib_file_path;
		std::string m_loc_file_path;
		size_t m_nb_points;
		GribVariables m_variables;
		CTPeriod m_period;
		CLocationVector m_locations;

	};



	//*****************************************************************************************************************

	class CSfcBlock
	{
	public:

		CSfcBlock(size_t xBlockSize = 0, size_t yBlockSize = 0)
		{
			m_ptr = nullptr;
			m_xBlockSize = 0;
			m_yBlockSize = 0;
			resize(xBlockSize, yBlockSize);
		}

		~CSfcBlock()
		{
			clean();
		}

		void clean()
		{
			m_xBlockSize = 0;
			m_yBlockSize = 0;
			delete[] m_ptr;
			m_ptr = nullptr;
		}

		//double GetValue(int x)const;
		//void SetValue(int x, double value);
		void resize(size_t xBlockSize, size_t yBlockSize)
		{
			clean();

			m_xBlockSize = xBlockSize;
			m_yBlockSize = yBlockSize;
			if (m_xBlockSize > 0 && m_yBlockSize > 0)
				m_ptr = new float[m_xBlockSize*m_yBlockSize];
		}

		float get_value(size_t x, size_t y)const
		{
			ASSERT(x < m_xBlockSize);
			ASSERT(y < m_yBlockSize);

			return m_ptr[y*m_yBlockSize + x];
		}


		void set_value(size_t x, size_t y, float value)
		{
			ASSERT(x < m_xBlockSize);
			ASSERT(y < m_yBlockSize);

			m_ptr[y*m_yBlockSize + x] = value;
		}
		
		float* data() { return m_ptr; }
		const float* data()const { return m_ptr; }

	protected:

		float* m_ptr;
		size_t m_xBlockSize;
		size_t m_yBlockSize;
		//int m_dataType;
	};


	typedef std::shared_ptr<CSfcBlock> CSfcBlockPtr;
	typedef std::array < CSfcBlockPtr, NB_VAR_GRIBS> CSfcWeatherBlock;
	typedef std::shared_ptr<CSfcWeatherBlock> CSfcWeatherBlockPtr;

	//typedef std::deque < CSfcWeatherBlockPtr> CSfcWeatherData; //
	//typedef boost::multi_array <CSfcWeatherBlockPtr, 2> CSfcWeatherData;
	typedef std::vector<std::vector<CSfcWeatherBlockPtr>> CSfcWeatherData;

	class CSfcDatasetCached : public CGDALDatasetEx
	{
	public:

		static size_t get_var(const std::string& strVar);
		static bool is_sfc(const std::string& strLevel);

		CSfcDatasetCached();
		ERMsg open(const std::string& filePath, bool bOpenInv = false);
		void close();

		void get_weather(const CGeoPointIndex& index, CHourlyData& data)const;
		void get_weather(const CGeoPointIndex& index, CWeatherDay& data)const;
		
		float get_variable(const CGeoPointIndex& index, size_t v)const;
		bool is_cached(size_t i, size_t j)const { assert(is_block_inside(i, j));  return block(i,j) != nullptr; }
		size_t get_band(size_t v)const { return m_bands[v]; }
		const GribVariables& get_variables()const { return m_variables; }

		void get_weather(const CGeoPoint& pt, CHourlyData& data)const;
		void get_weather(const CGeoPoint& pt, CWeatherDay& data)const;
		CGeoPointIndex get_ul(const CGeoPoint& ptIn)const;
		void get_nearest(const CGeoPoint& pt, CHourlyData& data)const;
		void get_nearest(const CGeoPoint& pt, CWeatherDay& data)const;
		void get_4nearest(const CGeoPoint& pt, CHourlyData4& data4)const;
		void get_4nearest(const CGeoPoint& pt, CDailyData4& data4)const;
		

		CLocationVector get_nearest(const CLocationVector& location, size_t nb_points)const;

		GribVariables m_variables_to_load;

	protected:


		void init_cache()const;
		bool is_cache_init()const { return !m_blocks.empty(); }
		void load_block(size_t i, size_t j);
		bool is_block_inside(size_t i, size_t j)const {return	j < m_blocks.size() && i < m_blocks[j].size();}
		CSfcWeatherBlockPtr& block(size_t i, size_t j){ return	m_blocks[j][i]; }
		const CSfcWeatherBlockPtr& block(size_t i, size_t j)const { return	m_blocks[j][i]; }

	//	const block(size_t i, size_t j);

		GribVariables m_variables;
		std::array<size_t, NB_VAR_GRIBS> m_bands;
		std::array<std::string, NB_VAR_GRIBS> m_units;
		std::array<float, NB_VAR_GRIBS> m_noData;

		//CSfcWeatherData m_lines;
		CSfcWeatherData m_blocks;
	};

	typedef std::shared_ptr<CSfcDatasetCached> CSfcDatasetCachedPtr;

	class CSfcGribDatabase 
	{
	public:
		
		static const char* META_DATA[NB_VAR_GRIBS][NB_META];



		size_t m_nb_points;
		bool m_bIncremental;
		GribVariables m_variables;
		int m_nbMaxThreads;
		CTPeriod m_period;

		CSfcGribDatabase() //: CDHDatabaseBase(cacheSize)
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

		ERMsg Open(const std::string& filePath, UINT flag, CCallback& callback= DEFAULT_CALLBACK, bool bSkipVerify=false);
		ERMsg Update(const CGribsMap& gribs, const CLocationVector& locations, CCallback& callback);
		ERMsg ExtractStation(CTRef TRef, const std::string& file_path, CWeatherStationVector& stations, CCallback& callback);
		ERMsg Search(CSearchResultVector& searchResultArray, const CLocation& station, size_t nbStation, double searchRadius = -1, CWVariables filter = CWVariables(), int year = YEAR_NOT_INIT, bool bExcludeUnused = true, bool bUseElevation = true, bool bUseShoreDistance = true)const;
		ERMsg GetStations(CWeatherStationVector& stationArray, const CSearchResultVector& results, int year)const;
		ERMsg Close(bool bSave = true, CCallback& callback = DEFAULT_CALLBACK);

		const CDHDatabaseBase* GetDB()const { return m_pDB.get(); }

	protected:

		static std::shared_ptr < CDHDatabaseBase> GetDatabase(const std::string& filePath);

		bool m_bIsHourly;
		std::shared_ptr < CDHDatabaseBase> m_pDB;
	};

	typedef std::shared_ptr<CSfcGribDatabase> CSfcGribDatabasePtr;

}

