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
// 15-11-2013  Rémi Saint-Amant	Created from old file
//****************************************************************************
#include "stdafx.h"
#pragma warning( disable : 4244 )
#include <boost\archive\binary_oarchive.hpp>
#include <boost\archive\binary_iarchive.hpp>
#include <boost\dynamic_bitset.hpp>
#include <boost\serialization\map.hpp>
#include <boost\serialization\array.hpp>
#include <boost\serialization\vector.hpp>
#include <boost\serialization\set.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <set>

#include "Basic/WeatherStation.h"
#include "Basic/WeatherDatabaseOptimisation.h"
#include "Basic/WeatherDatabaseCreator.h"
#include "ANN/ANN.h"

#include "WeatherBasedSimulationString.h"


//#include "Common\MultiAppSync.h"
//static const char mutexName[] = "CloseSearchNormalMutex";
//void DoNothing(){}


using namespace WBSF::HOURLY_DATA;
using namespace std;

namespace WBSF
{

	CSearchOptimisation::CSearchOptimisation()
	{

	}

	CSearchOptimisation::~CSearchOptimisation()
	{
		ASSERT(!IsOpen());

		if (IsOpen())
			Close();
	}

	void CSearchOptimisation::clear()
	{

		m_ANNs.clear();
		m_canalPosition.clear();
		m_filePathIndex.clear();
		m_filePathData.clear();
		m_fileIndex.clear();
		m_fileData.clear();

	}


	//static CMultiAppSync appSync;
	//bool bAlreadyOpen = false;

	ERMsg CSearchOptimisation::Open(const std::string& filePathIndex, const std::string& filePathData)
	{
		ASSERT(!IsOpen());

		ERMsg msg;

		clear();

		//CMultiAppSync appSyncOpenClose;
		//appSyncOpenClose.Do("OpenCloseNormal", DoNothing);

		//	bAlreadyOpen = false;
		//if (!appSync.Enter(mutexName))
		//bAlreadyOpen = true;


		//if (!FileExists(filePathIndex))
		//{
		//	//ASSERT(!bAlreadyOpen);
		//	ASSERT(m_canalPosition.empty() && m_ANNs.empty());

		//	//create empty data file
		//	msg += m_fileIndex.open(filePathIndex, ios::in | ios::out | ios::binary | ios::app);
		//	if (msg)
		//	{
		//		boost::archive::binary_oarchive ar(m_fileIndex, boost::archive::no_header);

		//		int version = 0;
		//		ar&VERSION;
		//		ar&m_canalPosition;
		//		m_fileIndex.close();

		//		//create empty file
		//		msg += m_fileData.open(filePathData, ios::in | ios::out | ios::binary | ios::trunc);
		//		m_fileData.close();
		//	}
		//}

		//if (msg)
		//{
		msg += m_fileIndex.open(filePathIndex, ios::in | ios::out | ios::binary | ios::app);//, SH_DENYNO
		if (msg)
			msg += m_fileData.open(filePathData, ios::in | ios::out | ios::binary | ios::app);//, SH_DENYWR

		if (msg)
		{
			//
			//if (!msg)//already open
			//{
			//	
			//	msg = m_fileData.open(filePathData, ios::in | ios::binary, SH_DENYNO);
			//	if (msg)
			//	{
			//		std::ios_base::openmode _Mode = ios::in | ios::binary;
			//		ASSERT(m_fileData.flags() == _Mode);
			//		ASSERT( (m_fileData.flags()&ios::out)  == false);
			//	}
			//}


			//				if (msg)
			//			{
			try
			{
				boost::archive::binary_iarchive ar(m_fileIndex, boost::archive::no_header);

				int version = 0;
				ar&version;
				if (version == VERSION)
				{
					ar&m_canalPosition;
					m_ANNs.resize(m_canalPosition.size());
					m_filePathIndex = filePathIndex;
					m_filePathData = filePathData;

					//open now for writing only to avoid exception throw in boost
					m_fileIndex.close();
					msg = m_fileIndex.open(filePathIndex, ios::out | ios::binary);
				}
			}
			catch (...)
			{
				//if (m_fileData.flags()&ios::out)//onpen in ouput
				//{
				//the file is corrupted, the format have probably change. erase 
				m_fileData.close();
				msg += m_fileData.open(filePathData, ios::in | ios::out | ios::binary | ios::trunc);

				m_fileIndex.close();
				msg += m_fileIndex.open(filePathIndex, ios::out | ios::binary | ios::trunc);
				//}
				if (msg)
				{
					m_filePathIndex = filePathIndex;
					m_filePathData = filePathData;
				}
				//}
				//else
				//{
				//	//fail even in read only mode... return error
				//	msg.ajoute("fail to open search optimization in read only mode. Probably the file was saved at the same time");
				//	msg.ajoute("\tfile:" + filePathData);
				//
				//}

			}
		}
		else
		{
			//close both file and rebuild search in memory
			m_fileData.close();
			m_fileIndex.close();
		}
		//}

		//appSyncOpenClose.Leave();


		return ERMsg();//never return error because, able to work without opt search files
	}


	void CSearchOptimisation::Close()
	{
		if (IsOpen())
		{
			//CMultiAppSync appSyncOpenClose;
			//appSyncOpenClose.Do("OpenCloseNormal", DoNothing);


			//if (!bAlreadyOpen)
			//{
			ASSERT(std::distance(m_canalPosition.begin(), m_canalPosition.end()) == m_ANNs.size());

			boost::archive::binary_oarchive ar(m_fileIndex, boost::archive::no_header);
			ar&VERSION;
			ar&m_canalPosition;

			m_fileIndex.close();
			m_fileData.close();
			clear();


			//bAlreadyOpen = false;
			//appSync.Leave();
			//}

			//appSyncOpenClose.Leave();
		}




	}


	bool CSearchOptimisation::CanalExists(__int64 canal)const
	{
		
		CCanalPositionMap::const_iterator it = m_canalPosition.find(canal);
		
		//bool bCanalExist = 
		//if (bCanalExist && m_fileData.is_open())//enty must also exist in the data file
		//{
		//	bCanalExist = false;

		//	const std::pair<__int64, ULONGLONG>& info = it->second;
		//	if (info.first >= 0 && info.first < __int64(m_ANNs.size()))
		//	{
		//		ULONGLONG curPos = 0;
		//		ULONGLONG length = 0;

		//		CSearchOptimisation& me = const_cast<CSearchOptimisation&>(*this);
		//		me.m_fileData.seekg(info.second, ios::beg);
		//		me.m_fileData.read((char*)(&curPos), sizeof(curPos));
		//		me.m_fileData.read((char*)(&length), sizeof(length));
		//		bCanalExist = curPos == info.second && length > 0;
		//	}
		//}

		return it != m_canalPosition.end();
	}

	std::pair<__int64, ULONGLONG> CSearchOptimisation::AddCanal(__int64 canal, CApproximateNearestNeighborPtr pANN)
	{
		//CMultiAppSync appSync;
		//appSync.Do(mutexName, DoNothing);

		//init wiht a default value of zero when the file is not open
		std::pair<__int64, ULONGLONG> info((__int64)m_ANNs.size(), 0);

		if ( m_fileData.is_open())//open in output
		{
			if (CanalExists(canal))
			{
				//if the canal exist, therefor, the search data and index file is corrupted.
				//the need to erase and begin again
				m_ANNs.clear();
				m_canalPosition.clear();
				m_fileData.seekp(0, ios::beg);

				m_fileData.close();
				m_fileData.open(m_filePathData, ios::in | ios::out | ios::binary | ios::trunc);

				m_fileIndex.close();
				m_fileIndex.open(m_filePathIndex, ios::out | ios::binary | ios::trunc);

			}

			
			m_fileData.seekp(0, ios::end);
			ULONGLONG curPos = m_fileData.tellp().seekpos();
			info.second = curPos;

			std::stringstream ss;
			boost::archive::binary_oarchive ar(ss, boost::archive::no_header);
			ar << *pANN;

			ULONGLONG length = ss.str().size();
			ASSERT(length > 0);

			m_fileData.write((char*)(&curPos), sizeof(curPos));
			m_fileData.write((char*)(&length), sizeof(length));
			m_fileData << ss.rdbuf();
			m_fileData.flush();

			fpos_t test = m_fileData.tellp().seekpos();
			ASSERT(test == curPos + length + sizeof(curPos) + sizeof(length));
		}

		m_ANNs.push_back(pANN);//canal only add in memory
		m_canalPosition[canal] = info;

		//appSync.Leave();

		return info;
	}


	CApproximateNearestNeighborPtr CSearchOptimisation::GetCanal(__int64 canal)const
	{
		CSearchOptimisation& me = const_cast<CSearchOptimisation&>(*this);

		CCanalPositionMap::iterator it = me.m_canalPosition.find(canal);
		ASSERT(it != m_canalPosition.end());

		if (it == m_canalPosition.end())
			return CApproximateNearestNeighborPtr();


		const std::pair<__int64, ULONGLONG>& info = it->second;
		ASSERT(info.first >= 0 && info.first < __int64(m_ANNs.size()));
		if (m_ANNs[info.first].get() == NULL)
		{
			ASSERT(m_fileData.is_open());//if the file is not open, the canal must be loaded in memory

			me.m_ANNs[info.first].reset(new CApproximateNearestNeighbor);

			ULONGLONG curPos = 0;
			ULONGLONG length = 0;

			me.m_fileData.seekg(info.second, ios::beg);
			me.m_fileData.read((char*)(&curPos), sizeof(curPos));
			me.m_fileData.read((char*)(&length), sizeof(length));
			ASSERT(length > 0);
			if (curPos == info.second && length > 0)
			{
				string buffer;
				buffer.resize(length);
				me.m_fileData.read((char*)(&buffer[0]), length);


				// * Create your string stream.
				// * Get the stringbuffer from the stream and set the vector as it source.

				std::stringstream       localStream(buffer);

				try
				{
					//load this canal
					boost::archive::binary_iarchive ar(localStream, boost::archive::no_header);
					ar >> *me.m_ANNs[info.first];
				}
				catch (...)
				{
					me.m_ANNs[info.first].reset();
					me.m_canalPosition.erase(it);
				}
			}
			else
			{
				me.m_ANNs[info.first].reset();//
				me.m_canalPosition.erase(it);
			}
		}


		return m_ANNs[info.first];
	}


	ERMsg CSearchOptimisation::DeleteOptimization(const std::string& filePath)
	{
		ERMsg msg;

		if (IsOpen())
			Close();

		clear();

		string indexFilePath = filePath + "Index";
		if (FileExists(indexFilePath))
			msg += RemoveFile(indexFilePath);

		if (FileExists(filePath))
			msg += RemoveFile(filePath);


		return msg;
	}


	//**************************************************************************************************************************
	CWeatherDatabaseOptimization::CWeatherDatabaseOptimization()
	{
		m_time = -1;
	}

	void CWeatherDatabaseOptimization::clear()
	{
		CLocationVector::clear();

		m_filePath.clear();
		m_time = -1;

		m_bSubDir = false;
		m_years.clear();
		m_filesStamps.clear();
		m_filesSection.clear();

		m_ANNs.Close();
	}

	void CWeatherDatabaseOptimization::push_back(const CLocation& in)
	{
		CLocationVector::push_back(in);
	}

	void CWeatherDatabaseOptimization::set(size_t i, const CLocation& in)
	{
		if (in != CLocationVector::at(i))
		{
			string oldFileName = at(i).GetDataFileName();
			string newFileName = in.GetDataFileName();
			if (newFileName != oldFileName)
			{
				CWeatherFileSectionIndex::const_iterator it1 = m_filesSection.find(oldFileName);
				if (it1 != m_filesSection.end())
				{
					CWeatherYearSectionMap tmp = it1->second;
					m_filesSection.erase(it1);
					m_filesSection[newFileName] = tmp;
				}

				CFileStampMap::const_iterator it2 = m_filesStamps.find(oldFileName);
				if (it2 != m_filesStamps.end())
				{
					__time64_t tmp = it2->second;
					m_filesStamps.erase(it2);
					m_filesStamps[newFileName] = tmp;

				}
			}

			CLocationVector::at(i) = in;
		}
	}

	void CWeatherDatabaseOptimization::erase(size_t i)
	{
		string fileName = at(i).GetDataFileName();
		CWeatherFileSectionIndex::const_iterator it1 = m_filesSection.find(fileName);
		if (it1 != m_filesSection.end())
			m_filesSection.erase(it1);

		CFileStampMap::const_iterator it2 = m_filesStamps.find(fileName);
		if (it2 != m_filesStamps.end())
			m_filesStamps.erase(it2);


		CLocationVector::erase(begin() + i);
	}

	ERMsg CWeatherDatabaseOptimization::Load(const std::string& filePath)
	{
		ERMsg msg;


		ifStream file;
		msg = file.open(filePath, ios::in | ios::binary);
		if (msg)
		{
			try
			{
				boost::archive::binary_iarchive ar(file, boost::archive::no_header);

				int version = 0;
				ar >> version;
				if (version == VERSION)
				{
					string DBExtension;

					ar >> DBExtension;
					ar >> m_bSubDir;
					ar >> (*this);

					m_filePath = filePath;
					SetFileExtension(m_filePath, DBExtension);
				}
			}
			catch (...)
			{
				//do nothing and recreate optimization
			}

			file.close();


		}

		return msg;
	}

	ERMsg CWeatherDatabaseOptimization::Save(const std::string& filePath)
	{
		ERMsg msg;

		ofStream file;
		msg = file.open(filePath, ios::out | ios::binary);
		if (msg)
		{
			ASSERT(!m_filePath.empty());
			// write map instance to archive
			boost::archive::binary_oarchive ar(file, boost::archive::no_header);
			string DBExtension = GetFileExtension(m_filePath);

			ar << VERSION;
			ar << DBExtension;
			ar << m_bSubDir;
			ar << (*this);
		}

		return msg;
	}


	ERMsg CWeatherDatabaseOptimization::LoadFromXML(const std::string& filePath, const string& rooName, const std::string& hdrExt)
	{
		ASSERT(FileExists(filePath));//save file before first open

		ERMsg msg;

		clear();

		zen::XmlDoc doc(rooName);
		msg = load(filePath, doc);
		if (msg)
		{
			
 			if (IsNormalsDB(filePath))
			{
				string str;
				if (doc.root().getAttribute("start", str) && !str.empty())
				{
					int year = stoi(str);
					if (year > INVLID_YEAR)
						m_years.insert(year);
				}
				if (doc.root().getAttribute("end", str) && !str.empty())
				{
					int year = stoi(str);
					if (year > INVLID_YEAR)
						m_years.insert(year);
				}
			}

			string subDir;
			doc.root().getAttribute("subdir", subDir);

			if (readStruc(doc.root(), *this))
			{
				//don't update it now. Update when open next time
				m_filePath = filePath;
				m_bSubDir = !subDir.empty() ? stoi(subDir) : false;

				//load csv format... remove xml later
				string CSVFilePath = filePath;
				SetFileExtension(CSVFilePath, hdrExt);
				msg = CLocationVector::Load(CSVFilePath);

				if (msg)
					m_time = GetFileStamp(CSVFilePath);
			}
			else
			{
				msg.ajoute("Error reading database: " + filePath);
			}

		}


		return msg;
	}


	ERMsg CWeatherDatabaseOptimization::SaveAsXML(const std::string& filePath, const std::string& subDir, const string& rootName, short version, const std::string& hdrExt)const
	{
		ERMsg msg;



		zen::XmlDoc doc(rootName);
		doc.setEncoding("Windows-1252");

		zen::writeStruc(*this, doc.root());
		doc.root().setAttribute("version", version);
		if (!m_years.empty())
		{
			doc.root().setAttribute("begin", to_string(*m_years.begin()));
			doc.root().setAttribute("end", to_string(*m_years.rbegin()));
		}

		doc.root().setAttribute("subdir", subDir.empty() ? "0" : "1");
		msg = save(doc, filePath);
		if (msg)
		{
			CWeatherDatabaseOptimization& me = const_cast<CWeatherDatabaseOptimization&>(*this);
			me.m_filePath = filePath;
			me.m_bSubDir = !subDir.empty();

			//save also in csv format... remove xml later
			string CSVFilePath = filePath;
			SetFileExtension(CSVFilePath, hdrExt);
			msg = CLocationVector::Save(CSVFilePath, ',');
		}


		return msg;
	}


	//ERMsg CWeatherDatabaseOptimization::SaveAsCSV(const std::string& filePath, const std::string& subDir, const string& rootName, short version)const
	//{
	//	ERMsg msg = Save(filePath);
	//
	//	//doc.root().setAttribute("version", version);
	//	//doc.root().setAttribute("begin", to_string(*m_years.begin()));
	//	//doc.root().setAttribute("end", to_string(*m_years.rbegin()));
	//	//doc.root().setAttribute("subdir", subDir.empty() ? "0" : "1");
	//
	//	if (msg)
	//	{
	//		CWeatherDatabaseOptimization& me = const_cast<CWeatherDatabaseOptimization&>(*this);
	//		me.m_filePath = filePath;
	//		me.m_bSubDir = !subDir.empty();
	//	}
	//
	//
	//	return msg;
	//}




	CLocationVector CWeatherDatabaseOptimization::GetStations(const CSearchResultVector& results)const
	{
		CLocationVector stations(results.size());

		for (size_t i = 0; i < results.size(); i++)
		{
			ASSERT(results[i].m_index >= 0 && results[i].m_index < size());
			stations[i] = at(results[i].m_index);
		}

		return stations;
	}


	StringVector CWeatherDatabaseOptimization::GetDataFiles()const
	{
		return GetWeatherDBDataFileList(m_filePath, *this);
	}

	ERMsg CWeatherDatabaseOptimization::GetDataFiles(CFileInfoVector& filesInfo, bool bToUpdateOnly, CCallback& callback)const
	{
		ERMsg msg;

		StringVector filesList = GetDataFiles();

		callback.PushTask(GetString(IDS_WG_DAILY_UPTODATE), filesList.size());

		filesInfo.reserve(filesList.size());
		for (size_t i = 0; i < filesList.size() && msg; i++)
		{
			CFileInfo info;
			msg = GetFileInfo(filesList[i], info);

			if (msg && (!bToUpdateOnly || !IsDataFileUpToDate(info)))
				filesInfo.push_back(info);

			msg += callback.StepIt();
		}

		callback.PopTask();
		return msg;
	}

	ERMsg CWeatherDatabaseOptimization::UpdateDataFiles(const std::string& filePath)
	{
		ERMsg msg;
		if (FileExists(filePath))
		{
			m_filesStamps[GetFileName(filePath)] = GetFileStamp(filePath);
		}

		return msg;
	}

	ERMsg CWeatherDatabaseOptimization::UpdateDataFilesYearsIndex(const std::string& filePath, const CFileInfoVector& list, CCallback& callback)
	{
		ERMsg msg;

		callback.PushTask(GetString(IDS_WG_DAILY_CREATE_OPT), list.size());

		if (FileExists(filePath))
			msg = m_filesSection.Load(filePath);

		for (CFileInfoVector::const_iterator it = list.begin(); it != list.end() && msg; it++)
		{
			msg = m_filesSection.Update(it->m_filePath, callback);
			if (msg)
				m_filesStamps[GetFileName(it->m_filePath)] = it->m_time;

			msg += callback.StepIt();
		}


		if (msg)
			msg = m_filesSection.Save(filePath);

		if (msg)
		{
			//update years
			std::set<int> tmp = m_filesSection.GetYears();
			m_years.insert(tmp.begin(), tmp.end());
		}

		callback.PopTask();
		return msg;
	}

	CWVariables CWeatherDatabaseOptimization::GetWVariables(size_t i, const std::set<int>& years, bool bForAllYears)const
	{
		ASSERT(i < size());
		CWVariables WVars;



		CWeatherFileSectionIndex::const_iterator it1 = m_filesSection.find(at(i).GetDataFileName());
		ASSERT(it1 != m_filesSection.end());
		if (it1 != m_filesSection.end() && !it1->second.empty())
		{
			if (bForAllYears)
				WVars.set();

			if (years.empty())
			{
				for (CWeatherYearSectionMap::const_iterator it2 = it1->second.begin(); it2 != it1->second.end(); it2++)
				{
					if (bForAllYears)
						WVars &= it2->second.m_nbRecords.GetVariables();
					else
						WVars |= it2->second.m_nbRecords.GetVariables();

				}
			}
			else
			{
				for (std::set<int>::const_iterator itY = years.begin(); itY != years.end(); itY++)
				{
					CWeatherYearSectionMap::const_iterator it2 = it1->second.find(*itY);
					if (it2 != it1->second.end())
					{
						if (bForAllYears)
							WVars &= it2->second.m_nbRecords.GetVariables();
						else
							WVars |= it2->second.m_nbRecords.GetVariables();

					}
				}
			}
		}

		return WVars;
	}

	CWVariablesCounter CWeatherDatabaseOptimization::GetWVariablesCounter(size_t i, const std::set<int>& years)const
	{
		ASSERT(i < size());
		CWVariablesCounter WCounter;



		CWeatherFileSectionIndex::const_iterator it1 = m_filesSection.find(at(i).GetDataFileName());
		ASSERT(it1 != m_filesSection.end());
		if (it1 != m_filesSection.end() && !it1->second.empty())
		{
			if (years.empty())
			{
				for (CWeatherYearSectionMap::const_iterator it2 = it1->second.begin(); it2 != it1->second.end(); it2++)
				{
					WCounter += it2->second.m_nbRecords;
				}
			}
			else
			{
				for (std::set<int>::const_iterator itY = years.begin(); itY != years.end(); itY++)
				{
					CWeatherYearSectionMap::const_iterator it2 = it1->second.find(*itY);
					if (it2 != it1->second.end())
					{
						WCounter += it2->second.m_nbRecords;
					}
				}
			}
		}

		return WCounter;
	}
	bool CWeatherDatabaseOptimization::CanalExists(__int64 canal)const
	{
		return m_ANNs.CanalExists(canal);
	}

	void CWeatherDatabaseOptimization::AddCanal(__int64 canal, CApproximateNearestNeighborPtr pANN)const
	{
		//ASSERT(!CanalExists(canal));

		CWeatherDatabaseOptimization& me = const_cast<CWeatherDatabaseOptimization&>(*this);
		me.m_ANNs.AddCanal(canal, pANN);
	}

	ERMsg CWeatherDatabaseOptimization::Search(const CLocation& pt, size_t nbPoint, CSearchResultVector& result, __int64 canal)const
	{
		ERMsg msg;

		if (m_ANNs.CanalExists(canal))
		{
			m_CS.Enter();
			CApproximateNearestNeighborPtr pANN = m_ANNs.GetCanal(canal);
			m_CS.Leave();

			if (pANN.get() != NULL)
			{
				msg = pANN->search(pt, nbPoint, result);
				for (size_t i = 0; i < result.size(); i++)
				{
					result[i].m_location = at(result[i].m_index);
					//compute distance without elevation
					result[i].m_distance = pt.GetDistance(result[i].m_location, false);
					result[i].m_deltaElev = result[i].m_location.m_elev - pt.m_elev;
					//result[i].m_deltaElev = pt.GetDistance(result[i].m_location, false);
				}
			}
			else
			{
				msg.ajoute("Invalid optimization files for search. Delete optimizations files and retry.");
			}
		}


		return msg;
	}

	ERMsg CWeatherDatabaseOptimization::OpenSearch(const std::string& filePath1, std::string& filePath2)const
	{
		ERMsg msg;
		m_CS.Enter();
		msg = const_cast<CWeatherDatabaseOptimization*>(this)->m_ANNs.Open(filePath1, filePath2);
		m_CS.Leave();
		return msg;
	}

	void CWeatherDatabaseOptimization::CloseSearch()const
	{
		m_CS.Enter();
		CWeatherDatabaseOptimization& me = const_cast<CWeatherDatabaseOptimization&>(*this);
		me.m_ANNs.Close();
		m_CS.Leave();
	}

	const std::set<int> CWeatherDatabaseOptimization::GetYears(size_t index)const
	{
		string fileName = at(index).GetDataFileName();
		CWeatherFileSectionIndex::const_iterator it = m_filesSection.find(fileName);
		ASSERT(it != m_filesSection.end());
		if (it != m_filesSection.end())
			return it->second.GetYears();

		return std::set<int>();

	}

}