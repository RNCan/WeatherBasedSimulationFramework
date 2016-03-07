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
#include <boost\serialization\access.hpp>
#include <boost\array.hpp>
#include "basic/zenXml.h"

#include "Basic/WeatherDefine.h"
#include "Basic/Location.h"
#include "Basic/Callback.h"
#include "Basic/ApproximateNearestNeighbor.h"
#include "Basic/WeatherDataSection.h"


namespace WBSF
{
	static const size_t UNSET_INDEX = (size_t)-1;

	typedef std::map<__int64, std::pair<__int64, ULONGLONG> > CCanalPositionMap;
	typedef std::deque<CApproximateNearestNeighborPtr> CApproximateNearestNeighborPtrVector;

	class CSearchOptimisation
	{
	public:

		CSearchOptimisation();
		~CSearchOptimisation();

		ERMsg Open(const std::string& filePathIndex, const std::string& filePathData);
		void Close();
		bool IsOpen()const{ return m_fileData.is_open() && m_fileIndex.is_open(); }
		void Reset(){ clear(); }
		ERMsg DeleteOptimization(const std::string& filePath);

		void clear();
		bool CanalExists(__int64 canal)const;
		std::pair<__int64, ULONGLONG> AddCanal(__int64 canal, CApproximateNearestNeighborPtr pANN);
		CApproximateNearestNeighborPtr GetCanal(__int64 canal)const;

	protected:

		std::string m_filePathIndex;
		std::string m_filePathData;
		fStream m_fileIndex;
		fStream m_fileData;

		//search section
		CCanalPositionMap m_canalPosition;
		CApproximateNearestNeighborPtrVector m_ANNs;


		static const int VERSION = 1;
	};

	typedef std::map<std::string, __time64_t> CFileStampMap;

	class CWeatherDatabaseOptimization : public CLocationVector
	{
	public:

		CWeatherDatabaseOptimization();
		~CWeatherDatabaseOptimization()
		{
			CloseSearch();
		}

		void Reset(){ clear(); }

		ERMsg Load(const std::string& filePath);
		ERMsg Save(const std::string& filePath);
		ERMsg LoadFromXML(const std::string& filePath, const std::string& rooName);
		ERMsg SaveAsXML(const std::string& filePath, const std::string& subDir, const std::string& rootName, short version)const;
		ERMsg LoadData(const std::string& filePath){ return m_filesSection.Load(filePath); }
		ERMsg SaveData(const std::string& filePath){ return m_filesSection.Save(filePath); }



		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & m_time & m_filesStamps & m_years;
			ar & boost::serialization::base_object<CLocationVector>(*this);
		}


		void clear();
		size_t GetNbStation()const{ return size(); }
		void push_back(const CLocation& location);
		void set(size_t i, const CLocation& in);
		void erase(size_t i);

		const CLocation& at(size_t i)const{ ASSERT(i >= 0 && i < size()); return CLocationVector::at(i); }
		const CLocation& operator[](size_t i)const{ return at(i); }
		const CLocation& GetStation(size_t stationIndex)const{ return at(stationIndex); }
		CWVariables GetWVariables(size_t i, const std::set<int>& year = std::set<int>(), bool bForAllYears = false)const;
		CWVariablesCounter GetWVariablesCounter(size_t i, const std::set<int>& years)const;

		CLocationVector GetStations(const CSearchResultVector& searchResultArray)const;

		StringVector GetDataFiles()const;
		ERMsg GetDataFiles(CFileInfoVector& fileInfo, bool bToUpdateOnly = true, CCallback& callback = CCallback::DEFAULT_CALLBACK)const;

		bool IsStationDefinitionUpToDate(const std::string& filePath)const{ return m_time != -1 && m_time == GetFileStamp(filePath); }
		bool IsDataFileUpToDate(const CFileInfo& fileInfo)const
		{
			CFileStampMap::const_iterator it = m_filesStamps.find(GetFileName(fileInfo.m_filePath));
			return it != m_filesStamps.end() ? fileInfo.m_time == it->second : false;
		}

		ERMsg UpdateDataFilesYearsIndex(const std::string& filePath, const CFileInfoVector& list, CCallback& callback);
		ERMsg UpdateDataFiles(const std::string& filePath);

		const CWeatherFileSectionIndex& GetDataSection()const{ return m_filesSection; }

		//search section
		bool SearchIsOpen()const{ return m_ANNs.IsOpen(); }
		bool CanalExists(__int64 canal)const;
		void AddCanal(__int64 canal, CApproximateNearestNeighborPtr pANN)const;
		ERMsg Search(const CLocation& station, size_t nbPoint, CSearchResultVector& searchResultArray, __int64 canal)const;
		ERMsg OpenSearch(const std::string& filePath1, std::string& filePath2)const;
		void CloseSearch()const;

		void SetYears(const std::set<int>& in){ m_years = in; }
		const std::set<int>& GetYears()const{ return m_years; }
		const std::set<int> GetYears(size_t index)const;

		CGeoRect GetBoundingBox()const{ return m_boundingBox; }
		std::string GetDataPath()const{ return m_filePath + (m_bSubDir ? GetFileTitle(m_filePath) + "\\" : ""); }

	protected:

		std::string m_filePath;
		__time64_t m_time;

		//std::string m_subDir;
		bool m_bSubDir;
		std::set<int> m_years;
		CGeoRect m_boundingBox;

		//data index section
		CFileStampMap m_filesStamps;
		CWeatherFileSectionIndex m_filesSection;

		//search
		CSearchOptimisation m_ANNs;



		CCriticalSection m_CS;
		static const int VERSION = 1;
	};

}

namespace zen
{
	template <> inline
		void writeStruc(const WBSF::CWeatherDatabaseOptimization& in, XmlElement& output)
	{
		writeStruc(((const WBSF::CLocationVector&)in), output);
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CWeatherDatabaseOptimization& out)
	{
		readStruc(input, ((WBSF::CLocationVector&)out));
		return true;
	}
}

