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

#include "boost\array.hpp"
#include "boost\serialization\access.hpp"

#include "Basic/WeatherDefine.h"
#include "Basic/Callback.h"

namespace WBSF
{
	class CCWeatherYearSection
	{
	public:

		CCWeatherYearSection()
		{
			Reset();
		}

		void Reset()
		{
			m_begin = 0;
			m_end = 0;
			m_lineNo = 0;
			m_nbLines = 0;

			m_nbRecords.fill(CCountPeriod());
		}


		ULONGLONG m_begin;
		ULONGLONG m_end;

		int m_lineNo;
		int m_nbLines;
		CWVariablesCounter m_nbRecords;



		friend boost::serialization::access;
		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & m_begin & m_end & m_lineNo & m_nbLines & m_nbRecords;
		}
	};



	typedef std::map<int, CCWeatherYearSection> CWeatherYearSectionMapBase;
	class CWeatherYearSectionMap : public CWeatherYearSectionMapBase
	{
	public:

		CWeatherYearSectionMap()
		{
			Reset();
		}
		CWeatherYearSectionMap(const CWeatherYearSectionMap& in)
		{
			operator=(in);
		}

		CWeatherYearSectionMap& operator=(const CWeatherYearSectionMap& in)
		{
			if (&in != this)
			{
				CWeatherYearSectionMapBase::operator=(in);
				m_checkSum = in.m_checkSum;
			}

			return *this;
		}


		void Reset()
		{
			clear();
			m_checkSum = 0;
		}



		std::set<int> GetYears()const;

		int m_checkSum;


		friend boost::serialization::access;
		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & m_checkSum & boost::serialization::base_object<CWeatherYearSectionMapBase>(*this);
		}
	};

	typedef std::map<std::string, CWeatherYearSectionMap> CDataFilesMap;
	class CWeatherFileSectionIndex : public CDataFilesMap
	{
	public:

		CWeatherFileSectionIndex();
		CWeatherFileSectionIndex(const CWeatherFileSectionIndex& in);
		CWeatherFileSectionIndex& operator=(const CWeatherFileSectionIndex& in);

		void Reset();

		ERMsg Load(const std::string& filePath);
		ERMsg Save(const std::string& filePath);

		CWeatherYearSectionMap GetYearsSection(const std::string& dataFilePath, const std::set<int>& years)const;

		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & boost::serialization::base_object<CDataFilesMap>(*this);
		}

		void CompileLine(const StringVector& data, const CWeatherFormat& format, CCWeatherYearSection& section);

		ERMsg Compile(const std::string& str, ULONGLONG begin, int lineNo, const CWeatherFormat& format, CWeatherYearSectionMap& map);
		ERMsg Update(const std::string& dataFilePath, CCallback& callback);
		

		std::string GetFilePath()const{ return m_filePath; }
		void CleanUnusedDataFiles(const StringVector& fileName);

		std::set<int> GetYears()const;

	protected:

		std::string m_filePath;


		static const int VERSION = 1;
	};
}//namepsace WBSF
