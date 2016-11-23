//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
#pragma once

#include <deque>
#include <memory>

#include "Basic/LRUCache.h"
#include "Basic/WeatherStation.h"
#include "Basic/Callback.h"
#include "Basic/WeatherDatabaseOptimisation.h"

namespace WBSF
{


	class CHourlyDataVector;
	class CHourlyWeight;
	class CHourlyData;
	class CHourlyYearArray;
	class CHourlyYear;
	class CWeatherGradient;
	class CWeatherStation;
	class CGeoRect;

	typedef cache::lru_cache< size_t, CWeatherStation > CWeatherStationCache;

	class CWeatherDatabase
	{
	public:

		enum TOpenMode { modeNotOpen = -1, modeRead, modeWrite, modeEdit = modeWrite };
		enum TWeatherGenerationMethod{ ALL_STATIONS, MOST_COMPLETE_STATIONS, WELL_DISTRIBUTED_STATIONS, COMPLETE_AND_DISTRIBUTED_STATIONS, NB_WEATHER_METHOD };


		bool m_bUseCache;
		CWeatherDatabase(int cacheSize = 200);
		virtual ~CWeatherDatabase();


		virtual ERMsg VerifyVersion(const std::string& filePath)const = 0;
		virtual __time64_t GetLastUpdate(const std::string& filePath, bool bVerifyAllFiles = true)const = 0;
		virtual ERMsg VerifyDB(CCallback& callBack = CCallback::DEFAULT_CALLBACK)const = 0;
		virtual ERMsg CreateFromMerge(const std::string& filePath1, const std::string& filePath2, double distance, double deltaElev, size_t mergeType, size_t priorityRules, std::string& log, CCallback& callback = CCallback::DEFAULT_CALLBACK) = 0;
		virtual ERMsg Search(CSearchResultVector& searchResultArray, const CLocation& station, size_t nbStation, double searchRadius=-1, CWVariables filter = CWVariables(), int year = YEAR_NOT_INIT, bool bExcludeUnused = true, bool bUseElevation = true)const = 0;
		virtual int GetVersion()const = 0;
		virtual const char* GetXMLFlag()const = 0;
		virtual const char* GetDatabaseExtension()const = 0;
		virtual const char* GetOptimizationExtension()const = 0;
		virtual const char* GetDataExtension()const = 0;
		virtual const char* GetHeaderExtension()const = 0;
		virtual const CTM	GetDataTM()const = 0;
		virtual const char	GetDBType()const = 0;

		virtual ERMsg Open(const std::string& filePath, UINT flag = modeRead, CCallback& callback = DEFAULT_CALLBACK, bool bSkipVerify=false);
		virtual ERMsg OpenOptimizationFile(const std::string& referencedFilePath, CCallback& callback = DEFAULT_CALLBACK, bool bSkipVerify=false);
		virtual ERMsg Save();
		virtual ERMsg Close(bool bSave = true, CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg Add(const CLocation& station);
		virtual ERMsg Get(CLocation& station, size_t index, const std::set<int>& years = std::set<int>())const;
		virtual ERMsg Set(size_t index, const CLocation& station);
		virtual ERMsg Remove(size_t index);
		virtual CWVariables GetWVariables(size_t i, const std::set<int>& year, bool bForAllYears = false)const;
		virtual CWVariablesCounter GetWVariablesCounter(size_t i, const std::set<int>& years)const;
		virtual ERMsg GetStationList(CSearchResultVector& searchResultArray, CWVariables filter, int year, bool bExcludeUnused = true, const CGeoRect& boundingBoxIn = CGeoRect())const;
		virtual ERMsg GetStationList(CSearchResultVector& searchResultArray, CWVariables filter = CWVariables(), const std::set<int>& years = std::set<int>(), bool bForAllYears = false, bool bExcludeUnused = true, const CGeoRect& boundingBoxIn = CGeoRect())const;

		virtual std::string GetSubDir(const std::string& filePath)const{ return ""; }
		virtual std::string GetDataPath(const std::string& filePath)const{ return WBSF::GetPath(filePath); }
		std::string GetDataFilePath(const std::string& filePath, const std::string& fileName)const{ return GetDataPath(filePath) + fileName; }
		std::string GetOptimisationFilePath(const std::string& referencedFilePath)const{ return WBSF::GetPath(referencedFilePath) + WBSF::GetFileTitle(referencedFilePath) + GetOptimizationExtension(); }
		std::string GetHeaderFilePath(const std::string& referencedFilePath)const{ return WBSF::GetPath(referencedFilePath) + WBSF::GetFileTitle(referencedFilePath) + GetHeaderExtension(); }
		
		
		std::string GetOptimisationDataFilePath(const std::string& referencedFilePath)const{ return  GetOptimisationFilePath(referencedFilePath) + "Data"; }
		std::string GetOptimisationSearchFilePath1(const std::string& referencedFilePath)const{ return  GetOptimisationFilePath(referencedFilePath) + "SearchIndex"; }
		std::string GetOptimisationSearchFilePath2(const std::string& referencedFilePath)const{ return  GetOptimisationFilePath(referencedFilePath) + "SearchData"; }

		std::string GetHeaderFilePath()const{ return GetHeaderFilePath(m_filePath); }
		std::string GetOptimisationFilePath()const{ return GetOptimisationFilePath(m_filePath); }
		std::string GetOptimisationDataFilePath()const{ return  GetOptimisationDataFilePath(m_filePath); }
		std::string GetOptimisationSearchFilePath1()const{ return  GetOptimisationSearchFilePath1(m_filePath); }
		std::string GetOptimisationSearchFilePath2()const{ return  GetOptimisationSearchFilePath2(m_filePath); }

		std::string GetPath()const{ return WBSF::GetPath(m_filePath); }
		std::string GetDataPath()const{ ASSERT(!m_filePath.empty());	return GetDataPath(m_filePath); }
		std::string GetDataFilePath(const std::string& fileName)const{ return GetDataPath() + fileName; }

		
		CWVariables GetWVariables(size_t i, int year = YEAR_NOT_INIT)const;
		CWVariablesCounter GetWVariablesCounter(size_t i, int years = YEAR_NOT_INIT)const;
		ERMsg Get(CLocation& stationIn, size_t index, int year)const;

		bool IsOpen()const{ return m_openMode != modeNotOpen; }
		bool empty()const { return m_zop.size() == 0; }
		size_t size()const{ return m_zop.size(); }
		ERMsg push_back(const CLocation& station){ return Add(station); }
		const CLocation& at(size_t index)const{ ASSERT(IsOpen()); return m_zop[index]; }
		const CLocation& operator[](size_t index)const{ ASSERT(IsOpen()); return m_zop[index]; }
		const CLocation& GetLocation(size_t index)const{ ASSERT(IsOpen()); return m_zop[index]; }
		CLocationVector GetLocations(const CSearchResultVector& results)const{ ASSERT(IsOpen()); return m_zop.GetStations(results); }

		bool StationExist(const std::string& name, bool bByName = true)const;
		int GetStationIndex(const std::string& nameIn, bool bByName = true)const;

		StringVector GetStationNameList(const CSearchResultVector& searchResultArray)const;
		std::string GetUniqueName(std::string const& name)const;
		void SearchD(CSearchResultVector& searchResultArray, const CLocation& station, double radius, CWVariables filter = CWVariables(), int year = YEAR_NOT_INIT, bool bExcludeUnused = true, bool bUseElevation = true)const;

		//tools
		ERMsg GenerateLOC(CSearchResultVector& searchResult, size_t method = ALL_STATIONS, size_t nbStation = NOT_INIT, CWVariables filter = CWVariables(), int year = YEAR_NOT_INIT, bool bExcludeUnused = true, bool bUseElevation = true, const CGeoRect& boundingBox = CGeoRect(), CCallback& callBack = DEFAULT_CALLBACK)const;
		ERMsg GetStationOrder(std::vector<size_t>& DBOrder, CWVariables filter = CWAllVariables())const;

		ERMsg ClearSearchOpt(const std::string& filePath);
		ERMsg ClearSearchOpt(){ return ClearSearchOpt(m_filePath); }

		//simple accessor
		bool GetModified()const { return m_bModified; }
		void SetModified(bool in){ m_bModified = in; }
		const std::string& GetFilePath()const{ return m_filePath; }
		//std::string GetDataPath()const{ return m_filePath + (m_bSubDir?GetFileTitle(m_filePath)+"\\":""); }

		const std::set<int>& GetYears()const{ ASSERT(IsOpen()); return m_zop.GetYears(); }
		std::set<int> GetYears(size_t index)const;
		CGeoRect GetBoundingBox()const{ ASSERT(IsOpen());	return m_zop.GetBoundingBox(); }
		CWeatherDatabaseOptimization const& GetOptimization()const{ return m_zop; }

		ERMsg OpenSearchOptimization(CCallback& callback);
		void CloseSearchOptimization();
		ERMsg GetPriority(std::vector<size_t>& priority, CWVariables filter, int year)const;

	protected:

		ERMsg GenerateWellDistributedStation(size_t nbStations, CSearchResultVector& searchResult, std::vector<size_t> priority, bool bUseElevation, CCallback& callback)const;

		std::string m_filePath;
		short m_openMode;
		bool m_bModified;

		// for optimization
		CWeatherDatabaseOptimization m_zop;
		CWeatherStationCache m_cache;

		CCriticalSection m_CS;

	private:

		CWeatherDatabase(const CWeatherDatabase& in) :m_cache(0){}
		CWeatherDatabase& operator=(const CWeatherDatabase& in){ return *this; }

	};



	typedef std::shared_ptr<CWeatherDatabase> CWeatherDatabasePtr;


	class CDHDatabaseBase : public CWeatherDatabase
	{
	public:

		CDHDatabaseBase(int cacheSize = 200) : CWeatherDatabase(cacheSize){}


		virtual std::string GetSubDir(const std::string& filePath)const{ return GetFileTitle(filePath); }
		using CWeatherDatabase::GetDataPath;
		virtual std::string GetDataPath(const std::string& filePath)const{ return WBSF::GetPath(filePath) + GetFileTitle(filePath) + GetDBType() + "\\"; }

		virtual ERMsg VerifyVersion(const std::string& filePath)const;
		virtual __time64_t GetLastUpdate(const std::string& filePath, bool bVerifyAllFiles = true)const;
		virtual ERMsg VerifyDB(CCallback& callBack = DEFAULT_CALLBACK)const;
		virtual ERMsg CreateFromMerge(const std::string& filePath1, const std::string& filePath2, double distance, double deltaElev, size_t mergeType, size_t priorityRules, std::string& log, CCallback& callback = DEFAULT_CALLBACK);
		virtual ERMsg Search(CSearchResultVector& searchResultArray, const CLocation& station, size_t nbStation, double radius, CWVariables filter = CWVariables(), int year = YEAR_NOT_INIT, bool bExcludeUnused = true, bool bUseElevation = true)const;

		using CWeatherDatabase::Get;
		virtual ERMsg Get(CLocation& station, size_t index, const std::set<int>& years = std::set<int>())const;

		ERMsg Add(const CLocation& location);
		ERMsg Set(size_t index, const CLocation& location);
		ERMsg Remove(size_t index);
		ERMsg GetStations(const CSearchResultVector& results, CWeatherStationVector& stationArray)const;
		void GetUnlinkedFile(StringVector& fileList);

		ERMsg DeleteDatabase(const std::string& outputFilePath, CCallback& callback = DEFAULT_CALLBACK);
		ERMsg RenameDatabase(const std::string& inputFilePath, const std::string& outputFilePath, CCallback& callback = DEFAULT_CALLBACK);
		ERMsg AppendDatabase(const std::string& inputFilePath1, const std::string& inputFilePath2, CCallback& callback = DEFAULT_CALLBACK);

		using CWeatherDatabase::GetVersion;
		static int GetVersion(const std::string& filePath);
		static ERMsg CreateDatabase(const std::string& filePath);

		static ERMsg MergeStation(CWeatherDatabase& inputDB1, CWeatherDatabase& inputDB2, const CSearchResultVector& results1, const CSearchResultVector& results2, CWeatherStation& station, size_t mergeType, size_t priorityRules, std::string& log);
	};

}