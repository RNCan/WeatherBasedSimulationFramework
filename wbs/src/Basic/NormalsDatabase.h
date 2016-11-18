//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once


#include <memory>
#include <array>
#include <deque>

#include "Basic/NormalsStation.h"
#include "Basic/Location.h"
#include "Basic/SearchResult.h"
#include "Basic/WeatherDatabaseOptimisation.h"
#include "Basic/WeatherDatabase.h"

namespace WBSF
{

	class CGeoRect;

	typedef std::deque<CNormalsData> CNormalDataDequeBase;
	class CNormalsDataDeque : public CNormalDataDequeBase
	{
	public:

		ERMsg Load(const std::string& filePath);
		ERMsg Save(const std::string& filePath);

		ERMsg LoadFromCSV(const std::string& filePath, const CWeatherDatabaseOptimization& zop, CCallback& callback);
		ERMsg SaveAsCSV(const std::string& filePath, const CWeatherDatabaseOptimization& zop, CCallback& callback);


		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & boost::serialization::base_object<CNormalDataDequeBase>(*this);
		}

		std::string m_filePath;
	};

	class CNormalsDatabase : public CWeatherDatabase
	{
	public:

		static const int   VERSION;
		static const char* XML_FLAG;
		static const char* DATABASE_EXT;
		static const char* OPT_EXT;
		static const char* DATA_EXT;
		static const char* META_EXT;
		static const CTM   DATA_TM;
		virtual int GetVersion()const{ return VERSION; }
		virtual const char* GetXMLFlag()const{ return XML_FLAG; }
		virtual const char* GetDatabaseExtension()const{ return DATABASE_EXT; }
		virtual const char* GetOptimizationExtension()const{ return OPT_EXT; }
		virtual const char* GetDataExtension()const{ return DATA_EXT; }
		virtual const char* GetHeaderExtension()const{ return META_EXT; }
		virtual const CTM	GetDataTM()const{ return DATA_TM; }
		virtual const char	GetDBType()const{ return 'N'; }



		CNormalsDatabase();
		~CNormalsDatabase();


		virtual ERMsg OpenOptimizationFile(const std::string& referencedFilePath, CCallback& callBack = DEFAULT_CALLBACK, bool bSkipVerify=false);
		virtual ERMsg Close(bool bSave = true, CCallback& callback = DEFAULT_CALLBACK);

		virtual ERMsg Add(const CLocation& station);
		virtual ERMsg Get(CLocation& station, size_t index, int year = YEAR_NOT_INIT)const;
		virtual ERMsg Set(size_t index, const CLocation& station);
		virtual ERMsg Remove(size_t index);
		virtual ERMsg Search(CSearchResultVector& searchResultArray, const CLocation& station, size_t nbStation, CWVariables filter = CWVariables(), int year = YEAR_NOT_INIT, bool bExcludeUnused = true, bool bUseElevation = true)const;
		virtual CWVariables GetWVariables(size_t i, const std::set<int>& years, bool bForAllYears = false)const;
		virtual CWVariablesCounter GetWVariablesCounter(size_t i, const std::set<int>& years)const;
		virtual ERMsg VerifyVersion(const std::string& filePath)const;
		virtual __time64_t GetLastUpdate(const std::string& filePath, bool bVerifyAllFiles = true)const;
		virtual ERMsg VerifyDB(CCallback& callBack = DEFAULT_CALLBACK)const;
		virtual ERMsg CreateFromMerge(const std::string& filePath1, const std::string& filePath2, double distance, double deltaElev, short mergeType, short priorityRules, std::string& log, CCallback& callback = DEFAULT_CALLBACK);


		ERMsg GetStations(const CSearchResultVector& results, CNormalsStationVector& stationArray)const;
		ERMsg CreateFromMerge(const std::string& filePath1, const std::string& filePath2, double distance, double deltaElev, short mergeType, CCallback& callback);
		ERMsg SaveAsV6(const std::string& filePath, CCallback& callback);


		static int GetVersion(const std::string& filePath);
		static ERMsg CreateDatabase(const std::string& filePath);
		static ERMsg DeleteDatabase(const std::string& filePath, CCallback& callback = DEFAULT_CALLBACK);
		static ERMsg RenameDatabase(const std::string& inputFilePath, const std::string& outputFilePath, CCallback& callback = DEFAULT_CALLBACK);
		//static ERMsg AppendDatabase(const std::string& inputFilePath1, const std::string& inputFilePath2, CCallback& callback = DEFAULT_CALLBACK);
		static bool IsExtendedDatabase(const std::string& filePath);
		static ERMsg v6_to_v7(const std::string& filePathV6, const std::string& filePathV7, CCallback& callback = DEFAULT_CALLBACK);
		static ERMsg v7_to_v6(const std::string& filePathV7, const std::string& filePathV6, CCallback& callback = DEFAULT_CALLBACK);
		//static ERMsg Convert(const std::string& filePath, CCallback& callback);
		static std::string GetNormalsDataFilePath(const std::string& filePath){ return WBSF::GetPath(filePath) + GetNormalsDataFileName(filePath); }
		static std::string GetNormalsDataFileName(const std::string& filePath){ return GetFileTitle(filePath)  + DATA_EXT; }
		static std::string GetNormalsHeaderFileName(const std::string& filePath){ return GetFileTitle(filePath) + META_EXT; }

		void SetPeriod(int firstYeat, int lastYear);
		int GetFirstYear()const{ return (!m_zop.GetYears().empty() ? *m_zop.GetYears().begin() : INVLID_YEAR); }
		int GetLastYear()const{ return (!m_zop.GetYears().empty() ? *m_zop.GetYears().rbegin() : INVLID_YEAR); }
		const CNormalsData& GetData(size_t index)const;
	protected:

		void GetStationOrder(std::vector<size_t>& DBOrder)const;
		CNormalsDataDeque m_data;
		static ERMsg MergeStation(const CNormalsStationVector& stations1, const CNormalsStationVector & stations2, CNormalsStation& station, short mergeType);

	};



	typedef std::shared_ptr<CNormalsDatabase> CNormalsDatabasePtr;

}