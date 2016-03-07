//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
// 15-01-2014	Rémi Saint-Amant  Initial version 
//******************************************************************************

#include "stdafx.h"
#include "WeatherDataSection.h"

#pragma warning( disable : 4244 )
#include <boost\archive\binary_oarchive.hpp>
#include <boost\archive\binary_iarchive.hpp>
#include <boost\dynamic_bitset.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost\serialization\map.hpp>
#include <boost\serialization\array.hpp>
#include <boost\serialization\vector.hpp>
#include <boost\crc.hpp>
#include <boost\container\flat_set.hpp>


using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::WEATHER;

namespace WBSF
{

	CWeatherFileSectionIndex::CWeatherFileSectionIndex()
	{
		Reset();
	}


	CWeatherFileSectionIndex::CWeatherFileSectionIndex(const CWeatherFileSectionIndex& in)
	{
		operator=(in);
	}


	CWeatherFileSectionIndex& CWeatherFileSectionIndex::operator=(const CWeatherFileSectionIndex& in)
	{
		if (&in != this)
		{
			CDataFilesMap::operator=(in);
			m_filePath = in.m_filePath;
		}

		return *this;
	}

	void CWeatherFileSectionIndex::Reset()
	{
		clear();
	}

	//bool CWeatherFileSectionIndex::Update(const CFileInfoVector& list)
	//{
	//}
	/*CArchive& CWeatherFileSectionIndex::operator >> ( CArchive& ar)
	{
	ar << (short)1;

	ar << m_fileTime;
	ar << m_fileSize;

	m_sections.Serialize(ar);

	return ar;
	}

	CArchive& CWeatherFileSectionIndex::operator << ( CArchive& ar)
	{
	short version=0;
	ar >> version;
	ar >> m_fileTime;
	ar >> m_fileSize;

	m_sections.Serialize(ar);

	return ar;
	}

	CArchive& operator << ( CArchive& ar, CWeatherFileSectionIndex& weaOpt)
	{
	weaOpt >> ar;
	return ar;
	}
	CArchive& operator >> ( CArchive& ar, CWeatherFileSectionIndex& weaOpt)
	{
	weaOpt << ar;
	return ar;
	}


	template <> void AFXAPI SerializeElements<CWeatherFileSectionIndex> (CArchive& ar, CWeatherFileSectionIndex* pElements, INT_PTR nCount)
	{
	for ( INT_PTR i = 0; i < nCount; i++, pElements++ )
	{
	if( ar.IsStoring() )
	ar << *pElements;
	else
	ar >> (*pElements);
	}

	}
	*/


	ERMsg CWeatherFileSectionIndex::Load(const std::string& filePath)
	{
		ERMsg msg;


		ifStream file;
		msg = file.open(filePath, ios::in | ios::binary);
		if (msg)
		{
			m_filePath = filePath;
			try
			{

				boost::archive::binary_iarchive ar(file, boost::archive::no_header);

				// read map instance from archive
				int version = 0;
				ar >> version;
				if (version == VERSION)
					ar >> (*this);
			}
			catch (...)
			{
			}
		}

		return msg;
	}



	ERMsg CWeatherFileSectionIndex::Save(const std::string& filePath)
	{
		ERMsg msg;

		ofStream file;
		msg = file.open(filePath, ios::out | ios::binary);
		if (msg)
		{
			m_filePath = filePath;
			try
			{

				// write map instance to archive
				boost::archive::binary_oarchive ar(file, boost::archive::no_header);
				ar << VERSION;
				ar << (*this);
			}
			catch (...)
			{

			}
		}

		return msg;
	}

	CWeatherYearSectionMap CWeatherFileSectionIndex::GetYearsSection(const std::string& dataFilePath, const set<int>& years)const
	{
		ASSERT(!m_filePath.empty());

		CWeatherYearSectionMap map;
		string fileName = GetFileName(dataFilePath);
		ASSERT(!fileName.empty());

		CWeatherFileSectionIndex::const_iterator it1 = find(fileName);
		ASSERT(it1 != end());

		if (!years.empty())
		{
			//Get years positions
			for (set<int>::const_iterator it2 = years.begin(); it2 != years.end(); it2++)
			{
				CWeatherYearSectionMap::const_iterator it = it1->second.find(*it2);
				if (it != it1->second.end())
					map[*it2] = it->second;
			}
		}

		return map;
	}

	void CWeatherFileSectionIndex::CompileLine(const StringVector& data, const CWeatherFormat& format, CCWeatherYearSection& section)
	{
		bool bTmin = false;
		bool bTmax = false;

		CTRef TRef = format.GetTRef(data);
		for (size_t i = 0; i < format.size(); i++)
		{
			if (IsVariable(format[i].m_var) && !data[i].empty())
			{
				double value = ToDouble(data[i]);
				if (!WEATHER::IsMissing(value))
				{
					if (format[i].m_var == H_TAIR && format[i].m_stat == LOWEST)
						bTmin = true;
					else if (format[i].m_var == H_TAIR && format[i].m_stat == HIGHEST)
						bTmax = true;
					else
						section.m_nbRecords[format[i].m_var] += CCountPeriod(1, CTPeriod(TRef, TRef));
				}

			}
		}//for all variables

		if (bTmin && bTmax)
		{
			section.m_nbRecords[H_TAIR] += CCountPeriod(1, CTPeriod(TRef, TRef));
			section.m_nbRecords[H_TRNG] += CCountPeriod(1, CTPeriod(TRef, TRef));

		}
	}




	ERMsg CWeatherFileSectionIndex::Compile(const std::string& str, ULONGLONG begin, int lineNo, const CWeatherFormat& format, CWeatherYearSectionMap& map)
	{
		ERMsg  msg;
		if (!str.empty())
		{
			int lastYear = YEAR_NOT_INIT;
			CCWeatherYearSection section;
			section.m_begin = begin;
			section.m_lineNo = lineNo;


			while (msg && begin < str.size())
			{
				string::size_type end = GetNextLinePos(str, begin);
				ASSERT(end >= begin);

				StringVector data;
				data.reserve(format.size());
				//data.Tokenize(str, ",\r\n", false, begin, end);
				data.Tokenize(str.substr(begin, end - begin - 2), ",", false);
				if (data.size() == 1)//try load old version
					data.Tokenize(str.substr(begin, end - begin - 2), " \t", true);

				if (!data.empty())
				{
					CTRef TRef = format.GetTRef(data);
					if (TRef.IsInit())
					{
						if (lastYear != YEAR_NOT_INIT && TRef.GetYear() != lastYear)
						{
							if (map.find(lastYear) == map.end())
							{
								map[lastYear] = section;

								section.Reset();
								section.m_begin = begin;
								section.m_lineNo = lineNo;

								lastYear = YEAR_NOT_INIT;
							}
							else
							{
								msg.ajoute("Error reading data file at position " + ToString(begin));
								msg.ajoute("Line = " + str.substr(begin, end - begin));
								msg.ajoute("Annual data must not be separated");
							}
						}

						if (lastYear == YEAR_NOT_INIT)
							lastYear = TRef.GetYear();

						CompileLine(data, format, section);

						section.m_end = end;
						section.m_nbLines++;
						lastYear = TRef.GetYear();
					}
					else
					{
						msg.ajoute("Error reading data file at position " + ToString(begin));
						msg.ajoute("Line = " + str.substr(begin, end - begin));
						msg.ajoute("Invalid temporal reference");
					}
				}

				begin = end;
				lineNo++;
			}

			//ad last section
			if (lastYear != YEAR_NOT_INIT)
			{
				if (map.find(lastYear) == map.end())
				{
					map[lastYear] = section;
				}
				else
				{
					msg.ajoute("Error reading data file at position " + ToString(begin));
					msg.ajoute("Annual data must not be separated");
				}
			}
		}

		return msg;
	}

	std::set<int> CWeatherYearSectionMap::GetYears()const
	{
		std::set<int> tmp;
		for (CWeatherYearSectionMap::const_iterator it = begin(); it != end(); it++)
			tmp.insert(it->first);

		return tmp;
	}

	bool VerifyOrder(CWeatherFileSectionIndex::const_iterator it1)
	{
		bool bRep = true;
		//for debug purpose only
		for (CWeatherYearSectionMap::const_iterator it2 = it1->second.begin(); it2 != it1->second.end(); it2++)
		{
			if (it2 != it1->second.begin())
			{
				CWeatherYearSectionMap::const_iterator it3 = it2; it3--;
				if (it2->first == it3->first)
					bRep = false;

				if (it2->second.m_begin != it3->second.m_end)
					bRep = false;
			}
		}

		return bRep;
	}

	int GetCheckSum(ifStream& file, int nbLines)
	{
		boost::crc_32_type result;
		int nbLine = 0;
		string line;
		while (nbLine++ < nbLines && file.good() && file >> line)
			result.process_bytes(&(line[0]), line.length());

		return result.checksum();
	}

	ERMsg CWeatherFileSectionIndex::Update(const std::string& dataFilePath, CCallback& callback)
	{
		ERMsg msg;

		string fileName = GetFileName(dataFilePath);
		ASSERT(!fileName.empty());

		CWeatherFileSectionIndex::iterator it1 = find(fileName);
		if (it1 == end())
		{
			(*this)[fileName] = CWeatherYearSectionMap();
			it1 = find(fileName);
		}

		//load file
		ifStream file;
		msg = file.open(dataFilePath, ios::in | ios::binary);//open file in binary to avoid problem with \r\n
		if (msg)
		{
			string str = file.GetText();
			file.close();

			int checkSum = GetCrc32(str);
			bool bCompile = it1->second.m_checkSum != checkSum;

			if (bCompile)
			{
				//compile only if the file have not the same checkSum
				//When compile; compile all the file
				it1->second.Reset();

				ULONGLONG begin = GetNextLinePos(str, 0);
				string header = str.substr(0, begin);
				CWeatherFormat format(header.c_str());

				int lineNo = 1;//header is not include

				msg = Compile(str, begin, lineNo, format, it1->second);
				if (msg)
					it1->second.m_checkSum = checkSum;
				else
					msg.ajoute("Error reading file" + dataFilePath);
			}
		}

		return msg;
	}
	
	void CWeatherFileSectionIndex::CleanUnusedDataFiles(const StringVector& fileName)
	{
		for (CWeatherFileSectionIndex::iterator it1 = begin(); it1 != end(); it1++)
		{
			StringVector::const_iterator it2 = FindStringExact(fileName, it1->first, false);
			if (it2 == fileName.end())//this file name is not longer used
				it1 = erase(it1);
		}
	}

	std::set<int> CWeatherFileSectionIndex::GetYears()const
	{
		std::set<int> tmp;

		for (CWeatherFileSectionIndex::const_iterator it1 = begin(); it1 != end(); it1++)
		{
			for (CWeatherYearSectionMap::const_iterator it2 = it1->second.begin(); it2 != it1->second.end(); it2++)
				tmp.insert(it2->first);
		}

		return tmp;
	}

}//namespace WBSF