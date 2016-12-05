//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
// 16-07-2014	Rémi Saint-Amant	version 7 of Normal database. Compile with VC 2013
// 05-02-2010	Rémi Saint-Amant	Add Merge database
// 22-10-2007	Rémi Saint-Amant	Version 6: new variable for wind
// 01-10-1998	Rémi Saint-Amant	Optimisation
// 10-01-1998	Rémi Saint-Amant	Version initiale
//****************************************************************************

#include "stdafx.h"
#pragma warning( disable : 4244 )
#include <boost\archive\binary_oarchive.hpp>
#include <boost\archive\binary_iarchive.hpp>
#include <boost\dynamic_bitset.hpp>
#include <boost\serialization\array.hpp>
#include <boost\serialization\deque.hpp>

#include "Basic/NormalsDatabase.h"
#include "Basic/GeoBasic.h"
#include "Basic/CSV.h"

#include "WeatherBasedSimulationString.h"

using namespace std;
using namespace WBSF::NORMALS_DATA;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::WEATHER;

namespace WBSF
{

const int CNormalsDatabase::VERSION = 7;
const char* CNormalsDatabase::XML_FLAG = "NormalsDatabase";
const char* CNormalsDatabase::DATABASE_EXT = ".NormalsDB";
const char* CNormalsDatabase::OPT_EXT = ".Nzop";
const char* CNormalsDatabase::DATA_EXT = ".NormalsData.csv";
const char* CNormalsDatabase::META_EXT = ".NormalsHdr.csv";

const CTM CNormalsDatabase::DATA_TM = CTM(CTM::MONTHLY, CTM::OVERALL_YEARS);


ERMsg CNormalsDataDeque::Load(const std::string& filePath)
{
	ERMsg msg;

	clear();

	ifStream file;
	msg += file.open(filePath, ios::in | ios::binary);
	if (msg)
	{
		try
		{
			boost::archive::binary_iarchive ar(file, boost::archive::no_header);

			ar >> *this;
			m_filePath = filePath;

			file.close();
		}
		catch (...)
		{
			msg.ajoute("Bad version of optimization file");
		}
	}


	return msg;
}

ERMsg CNormalsDataDeque::Save(const std::string& filePath)
{
	ERMsg msg;

	ofStream file;
	msg += file.open(filePath, ios::out | ios::binary);
	if (msg)
	{
		try
		{
			boost::archive::binary_oarchive ar(file, boost::archive::no_header);

			ar << (*this);
			m_filePath = filePath;

			file.close();
		}
		catch (...)
		{
			msg.ajoute(GetString(IDS_WG_INVALID_OPT));
		}
	}

	return msg;

}

ERMsg CNormalsDataDeque::LoadFromCSV(const std::string& filePath, const CWeatherDatabaseOptimization& zop, CCallback& callback)
{
	ERMsg msg;

	clear();

	ifStream file;
	msg = file.open(filePath);

	if (msg)
	{
		callback.PushTask(FormatMsg(IDS_BSC_OPEN_FILE, filePath), size_t(file.length()));

		size_t i = 0;
		for (CSVIterator loop(file); loop != CSVIterator()&&msg; ++loop, i++)
		{
			if (!(*loop).empty())
			{
				enum TFixedNormalsCols { N_STATION_ID, N_MONTH, NB_FIXED_COLS };
				if ((*loop).size() == NB_FIELDS + NB_FIXED_COLS)
				{
					string stationID = (*loop)[N_STATION_ID];
					size_t pos = size_t(i / 12);
					
					if (pos < zop.size())
					{
						//desactiver temporairement jusqu'à ce que je refasse les BD
						//if (stationID == zop[pos].m_ID)
						if (true)
						{
							if (pos >= size())
								resize(pos + 1);

							int m = stoi((*loop)[N_MONTH]) - 1;
							ASSERT(m >= 0 && m < 12);
							if (m >= 0 && m < 12)
							{
								if (m == i % 12)
								{
									for (size_t j = 0; j < (*loop).size() - NB_FIXED_COLS; j++)
										at(pos).at(m).at(j) = stof((*loop)[j + NB_FIXED_COLS]);
								}
								else
								{
									msg.ajoute("Error reading normal file: " + filePath + " at line : " + to_string(i + 1));
									msg.ajoute("Unexpected month : " + to_string(m + 1) + ", but " + to_string((i % 12) + 1) + " expected");
								}
							}
							else
							{
								msg.ajoute("Error reading normal file: " + filePath + " at line : " + to_string(i + 1));
								msg.ajoute("Invalid month number: " + to_string(m + 1));
							}
						}
						else
						{
							msg.ajoute("Error reading normal file: " + filePath + " at line : " + to_string(i + 1));
							msg.ajoute("Invalid station ID: " + stationID);
							msg.ajoute("CSV data fiel and XML files must have the same size and be in the same order");
						}
					}
					else
					{
						//read all the data field and put error message at the end...
					}
				}
				else
				{
					msg.ajoute("Error reading normal file: " + filePath + " at line : " + to_string(i+1));
					msg.ajoute("Invalid number of columns: " + to_string((*loop).size()) + ", " + to_string(NB_FIELDS + NB_FIXED_COLS) + " expected");
				}

				msg += callback.SetCurrentStepPos((size_t)file.tellg());
			}//if not empty
		}//for all lines

		if (zop.size() != size())
			msg.ajoute("Error: index file (.NormalsStations) have " + to_string(zop.size()) + " stations and data file (.csv) have " + to_string(size()) + " stations");


		callback.PopTask();
	}//if msg

	return msg;

}

ERMsg CNormalsDataDeque::SaveAsCSV(const std::string& filePath, const CWeatherDatabaseOptimization& zop, CCallback& callback)
{
	ASSERT(MISSING == -999);
	ERMsg msg;

	ofStream file;
	msg = file.open(filePath);

	
	if (msg)
	{
		callback.PushTask(FormatMsg(IDS_BSC_SAVE_FILE, filePath), size());
		//callback.SetNbStep(size());

		//write header
		file << "StationID,Month";
		for (size_t f = 0; f != NB_FIELDS && msg; f++)
			file << ',' << GetFieldHeader(f);
		
		file << endl;
		const CNormalsDataDeque& me = *this;

		//size_t i = 0;
		for (size_t i = 0; i != me.size() && msg; i++)
		{
			
			for (size_t m = 0; m != me[i].size() && msg; m++)
			{
				string line = FormatA("%s,%02d", zop[i].m_ID.c_str(), int(m + 1));
				

				for (size_t f = 0; f != me[i][m].size() && msg; f++)
				{
					if (!IsMissing(me[i][m][f]))
					{
						string format = "%7." + to_string(GetNormalDataPrecision((int)f)) + "f";
						string value = FormatA(format.c_str(), me[i][m][f]);
						line += "," + value;
					}
					else
					{
						line += ", -999.0";
					}
					
				}

				//file << endl;
				file.write(line +"\n");
			}

			msg += callback.StepIt();
		}

		file.close();

		callback.PopTask();
	}

	return msg;
}


//****************************************************************************
// Sommaire:    Constructeur par défaut.
//
// Description: Crée la classe et assigne le path par default
//
// Entrée:      
//
// Sortie:
//
// Note:        On doit initialiser cette class avec un path
//****************************************************************************
CNormalsDatabase::CNormalsDatabase() : CWeatherDatabase(0)//no cache is need by normals because normals already loaded in memory
{
	//m_beginYear = beginYear;
	//m_endYear = endYear;
}

//****************************************************************************
// Sommaire:     Destructeur de la classe
//
// Description:  Libères la mémoire si la BD est encore ouverte
//
// Entrée:
//
// Sortie:
//
// Note:
//****************************************************************************
CNormalsDatabase::~CNormalsDatabase()
{
	if( IsOpen() )
		Close();
}


void CNormalsDatabase::SetPeriod(int firstYeat, int lastYear)
{
	set<int> years;
	years.insert(firstYeat);
	years.insert(lastYear);
	m_zop.SetYears(years);
}

ERMsg CNormalsDatabase::Close(bool bSave, CCallback& callback )
{
	ERMsg msg;
	if (m_openMode == modeWrite)
	{
		ASSERT(m_zop.size() == m_data.size());
		if (m_bModified && bSave)
		{
			msg = m_zop.SaveAsXML(m_filePath, "", GetXMLFlag(), GetVersion(), GetHeaderExtension());
			if (msg)
				msg = m_data.SaveAsCSV(GetNormalsDataFilePath(m_filePath), m_zop, callback);

			if (msg)
			{
				msg += m_zop.Save(GetOptimisationFilePath());
				msg += m_data.Save(GetOptimisationDataFilePath());
			}

			ClearSearchOpt();

			m_bModified = false;
		}
	}

	CWeatherDatabase::Close();

	return msg;
}

ERMsg CNormalsDatabase::OpenOptimizationFile(const std::string& referencedFilePath, CCallback& callback, bool /*bSkipVerify*/)
{
	ERMsg msg;
	bool bStationsAsChange = true;
	bool bDataAsChange = true;


	std::string optFilePath = GetOptimisationFilePath(referencedFilePath);
	if (FileExists(referencedFilePath) && FileExists(optFilePath))
	{
		callback.AddMessage(FormatMsg( IDS_MSG_LOAD_OP, GetFileName(optFilePath) ));
		if (m_zop.Load(optFilePath))
		{
			string headerFilePath = GetHeaderFilePath(referencedFilePath);
			if (m_zop.IsStationDefinitionUpToDate(headerFilePath))
			{
				bStationsAsChange = false;
			}
		}
	}

	if (msg && bStationsAsChange)
	{
		if (!FileExists(referencedFilePath))
			msg += CWeatherDatabaseOptimization().SaveAsXML(referencedFilePath, "", GetXMLFlag(), GetVersion(), GetHeaderExtension());
		
		string dataFilePath = CNormalsDatabase().GetNormalsDataFilePath(referencedFilePath);
		if (msg && !FileExists(dataFilePath))
			msg += CNormalsDataDeque().SaveAsCSV(dataFilePath, CWeatherDatabaseOptimization(), DEFAULT_CALLBACK);
		
		
		if (msg)
			msg += VerifyVersion(referencedFilePath);
			
		if (msg)
		{
			
			callback.AddMessage(FormatMsg(IDS_MSG_OPEN, GetFileName(referencedFilePath)) );
			msg = m_zop.LoadFromXML(referencedFilePath, GetXMLFlag(), GetHeaderExtension() );
		}
	}

	if (msg)
	{

		//Get data file to be updated
		CFileInfoVector fileInfo;

		msg = m_zop.GetDataFiles(fileInfo, true, callback);
		bDataAsChange = !fileInfo.empty();

		if (bDataAsChange)
		{
			callback.AddMessage(FormatMsg(IDS_MSG_UPDATE, GetFileName(optFilePath)) );
			string dataOptFilePath = GetOptimisationDataFilePath(referencedFilePath); 
			
			msg = m_data.LoadFromCSV(GetNormalsDataFilePath(referencedFilePath), m_zop, callback);
			
			if (msg)
			{
				m_zop.UpdateDataFiles(GetNormalsDataFilePath(referencedFilePath));
				msg = m_data.Save(GetOptimisationDataFilePath(referencedFilePath));
			}
		}
		else
		{
			msg = m_data.Load(GetOptimisationDataFilePath(referencedFilePath));
		}
	}

	if (msg && (bStationsAsChange || bDataAsChange))
	{
		callback.AddMessage("Save " + GetFileName(optFilePath) + "...");
		msg += m_zop.Save(optFilePath);
		msg += ClearSearchOpt(referencedFilePath);
	}



	return msg;
}

//****************************************************************************
// Sommaire:     Obtenir une station normal
//
// Description: La methode GetNormalStation permet d'obtenir la station a partir
//              de son nom(sStationName).
//
// Entrée:      const string& sStationName: nom de la station désirée.
//
// Sortie:      CNormalsStation& station: la station normals
//              bool: true si succes, false autrement
//
// Note:        Si la BD n'est pas chargée en mémoire, on la charge avec 
//              CheckIfDBReady().
//****************************************************************************
ERMsg CNormalsDatabase::Get(CLocation& stationIn, size_t index, int year)const
{
	ASSERT( IsOpen() );
	ASSERT(m_data.size() == m_zop.size());
    ASSERT( index >= 0 && index < m_zop.size() );
	
	CNormalsStation& station = dynamic_cast<CNormalsStation&>(stationIn);

    ERMsg msg;

	((CLocation&)station) = m_zop[index];
	((CNormalsData&)station) = m_data[index];


    return msg ;
}

const CNormalsData& CNormalsDatabase::GetData(size_t index)const
{
	ASSERT(IsOpen());
	ASSERT(m_data.size() == m_zop.size());
	ASSERT(index >= 0 && index < m_zop.size());
	
	return m_data[index];
}

//****************************************************************************
// Sommaire:     Ajouter une station normal à la BD
//
// Description: La methode Add permet d'ajouter une station a la DB si cette 
//              station n'y est pas.
//
// Entrée:      CNormalsStation& station: la station à ajouter.
//
// Sortie:      bool: true si succes, false autrement
//
// Note:        Si la BD est chargée en mémoire, on la decharge 
//****************************************************************************
ERMsg CNormalsDatabase::Add(const CLocation& stationIn)
{
	ASSERT( IsOpen() );
	ASSERT(m_data.size() == m_zop.size());
	ASSERT( m_openMode==modeEdit );

	ERMsg msg;

	const CNormalsStation& station = static_cast<const CNormalsStation&>(stationIn);

	m_zop.push_back(station);
	m_data.push_back(station);
	m_bModified = true;

    return msg;
}

//****************************************************************************
// Sommaire:     modifie une station normal de la BD
//
// Description: La methode Modify permet de modifier tous les champs d'une station
//              normal, saul le nom(qui identifie la station). Si la station 
//              n'existe pas, on l'ajoute.
//
// Entrée:      CNormalsStation& station: la station à changer.
//
// Sortie:      bool: true si succes, false autrement
//
// Note:        Si la BD est chargée en mémoire, on la decharge 
//****************************************************************************
ERMsg CNormalsDatabase::Set(size_t index, const CLocation& stationIn)
{
	ASSERT(IsOpen());
	ASSERT(m_data.size() == m_zop.size());
	ASSERT(m_openMode == modeEdit);
	//ASSERT( DatabaseExist() );

	ERMsg msg;

	const CNormalsStation& station = static_cast<const CNormalsStation&>(stationIn);

	if ( ((CLocation&)station) != m_zop[index] &&
		((CNormalsData&)station) != m_data[index] )
	{
		m_zop.set(index, station);
		m_data[index] = station;
		m_bModified = true;
	}

	return msg;
}


//****************************************************************************
// Sommaire:     Supprime une station de BD normal
//
// Description: La methode Delete permet de supprimer une station de la DB.
//              Si la station n'existe pas on ASSERT et on ne fait rien
//
// Entrée:      const string& sStationName: le nom de la station à supprimer
//
// Sortie:      bool: true si succes, false autrement
//
// Note:        Si la BD est chargée en mémoire, on la decharge 
//****************************************************************************
ERMsg CNormalsDatabase::Remove(size_t index)
{
	ASSERT( IsOpen() );
	ASSERT(m_data.size() == m_zop.size());
	ASSERT( m_openMode==modeEdit );
    //ASSERT( DatabaseExist() );

    ERMsg msg;

	m_zop.erase(index);
	m_data.erase(m_data.begin() + index);
	m_bModified = true;

    return msg;
}



CWVariables CNormalsDatabase::GetWVariables(size_t i, const set<int>&, bool )const
{
	return m_data[i].GetVariables();
}

CWVariablesCounter CNormalsDatabase::GetWVariablesCounter(size_t i, const set<int>& years)const
{
	CWVariables variables = m_data[i].GetVariables();

	CWVariablesCounter count;

	for (size_t v = 0; v < NB_VAR_H; v++)
		if (variables[v])
			count[v] = CCountPeriod(12, CTPeriod(CTRef(0, FIRST_MONTH, 0, 0, CTM(CTM::MONTHLY, CTM::OVERALL_YEARS)), CTRef(0, LAST_MONTH, 0, 0, CTM(CTM::MONTHLY, CTM::OVERALL_YEARS))));
	
	return count;
}

ERMsg CNormalsDatabase::Search(CSearchResultVector& searchResultArray, const CLocation& station, size_t nbStation, double searchRadius, CWVariables filter, int year, bool bExcludeUnused, bool bUseElevation)const
{
	ASSERT(IsOpen());
	ASSERT(m_openMode == modeRead);
	ASSERT(!m_bModified);//close and open the database again
	ASSERT(m_zop.size() == m_data.size());

	ERMsg msg;


	if (filter == CWVariables(H_WND2))
		filter = H_WNDS;

	if (!m_zop.SearchIsOpen())
	{
		msg = m_zop.OpenSearch(GetOptimisationSearchFilePath1(), GetOptimisationSearchFilePath2());
		if (!msg)
			return msg;
		
	}
	
	year = 0;//always take zero for normals
	__int64 canal = (filter.to_ullong()) * 100000 + year * 10 + (bUseElevation ? 2 : 0) + (bExcludeUnused ? 1 : 0);

	const_cast<CNormalsDatabase*>(this)->m_CS.Enter();
	if (!m_zop.CanalExists(canal))
	{
		CLocationVector locations;
		locations.reserve(m_zop.size());
		std::vector<__int64> positions;
		positions.reserve(m_zop.size());

		
		//build canal
		for (CLocationVector::const_iterator it = m_zop.begin(); it != m_zop.end(); it++)
		{
			bool useIt = it->UseIt();
			if (useIt || !bExcludeUnused)
			{
				size_t index = std::distance(m_zop.begin(), it);
				bool bIncluded = (m_data[index].GetVariables()&filter) == filter;

				if (bIncluded)
				{
					CLocation pt = *it;//removel
					pt.m_siteSpeceficInformation.clear();//remove ssi for ANN
					locations.push_back(pt);
					positions.push_back(index);
				}
			}//use it
		}


		//by optimization, add the canal event if they are empty
		CApproximateNearestNeighborPtr pANN(new CApproximateNearestNeighbor);
		pANN->set(locations, bUseElevation , positions);
		CWeatherDatabaseOptimization& zop = const_cast<CWeatherDatabaseOptimization&>(m_zop);
		zop.AddCanal(canal, pANN);
	}
	const_cast<CNormalsDatabase*>(this)->m_CS.Leave();



	msg = m_zop.Search(station, nbStation, searchResultArray, canal);
	if (searchResultArray.size() == nbStation)
	{
		if (searchRadius >= 0)
		{
			for (CSearchResultVector::iterator it = searchResultArray.begin(); it != searchResultArray.end();)
			{
				if (it->m_distance < searchRadius)//station in the radius not accepted to exclude all station when r=0
					it++; 
				else
					it = searchResultArray.erase(it);
			}

			if (searchResultArray.empty())
			{
				string filterName = filter.GetVariablesName('+');
				string error = FormatMsg(IDS_WG_NOTENOUGH_OBSERVATION2, ToString(searchRadius / 1000, 1), GetFileName(m_filePath), ToString(year), filterName);
				msg.ajoute(error);
			}
		}
	}
	else
	{
		string filterName = filter.GetVariablesName('+');

		assert(!filterName.empty());
		msg = ERMsg();//reset msg and add new message
		string error = FormatMsg(IDS_WG_NOTENOUGH_NORMALSTATION, ToString(searchResultArray.size()), GetFileName(m_filePath), ToString(nbStation), filterName);
		msg.ajoute(error);
	}

	return msg;
}

ERMsg CNormalsDatabase::GetStations( const CSearchResultVector& results, CNormalsStationVector& stations)const
{
	ASSERT( IsOpen() );

	ERMsg msg;

	stations.resize(results.size());

	//Get stations 
	for(size_t i=0; i<results.size(); i++) 
		Get(stations[i], results[i].m_index, results.GetYear());
	

    return msg;
}
//****************************************************************************
// Sommaire:    Permet de savoir la date de la dernière modification
//
// Description: Permet d'avoir la date (CTime) de la dernière modif à la BD
//              en réaliter GetLastModify retourne la date de la DB
//              Si la BD n'exite pas alors GetLastModify retourn false.
//
// Entrée
//
// Sortie:      CTime& fileModify: la date de la dernière modif si succes
//              bool: true si BD exite, false autrement
//
// Note:        
//****************************************************************************
__time64_t CNormalsDatabase::GetLastUpdate(const string& filePath, bool bVerifyAllFiles)const
{
	
	__time64_t lastUpdate = GetFileStamp(filePath);
	if (bVerifyAllFiles)
		lastUpdate = GetFileStamp(GetOptimisationFilePath(filePath));

	return lastUpdate;
}


//****************************************************************************
// fonction protected or private
//****************************************************************************


ERMsg CNormalsDatabase::VerifyVersion(const string& filePath)const
{
    ERMsg msg;
	if( FileExists(filePath) )
    {
		if (GetVersion(filePath) != GetVersion() )
        {
			msg.ajoute(FormatMsg(IDS_WG_BAD_VERSION, ToString(GetVersion(filePath)), ToString(GetVersion())));
        }
    }
    else
    { 
		msg.ajoute(FormatMsg(IDS_WG_DB_NOTEXIST, filePath));
    }

    return msg;
}

int CNormalsDatabase::GetVersion(const string& filePath)
{
    //FILE* pFile;
    int nVersion = -1;
	ifStream file;
	if( file.open(filePath) )
    {
		string line;
		if( std::getline(file, line) )
		{
			if (Find(line, "<?xml"))
			{
				nVersion=7;
				if (std::getline(file, line))
				{
					MakeLower(line);
					string::size_type pos = line.find("version=");
					if (pos != string::npos)
					{
						Tokenize(line, "\" \t'", pos);
						if (pos >= 0)
							nVersion = ToInt(Tokenize(line, "\" \t'", pos));
					}
				}
			}
			else
			{
				int bidon;
				if( sscanf(line.c_str(),"%5d%5d%5d%5d%5d%5d\n",&bidon,&bidon,&bidon,&bidon,&bidon,&nVersion) != 6)
					nVersion = 1;
			}
		}
        //fclose(pFile);    
		file.close();
    }
    
    return nVersion;
}

//void CNormalsDatabase::LoadPeriod(const string& filePath)
//{
///*	FILE* pFile;
//    int nVersion = -1;
//
//    if ((pFile = fopen(filePath,"r")) != NULL)
//    {
//        int bidon;
//        fscanf(pFile,"%5d%5d%5d%5d%5d%5d\n",&bidon,&bidon,&bidon,&m_beginYear,&m_endYear,&bidon);
//        fclose(pFile);    
//    }
//  */  
//}

ERMsg CNormalsDatabase::DeleteDatabase(const string& filePath, CCallback& callback)
{
	ERMsg msg;

	CNormalsDatabase N;//to solve the problem of static function

	if( GetFileTitle( filePath ).empty() )
		return msg;

	if( FileExists(filePath) )
	{
		callback.AddMessage(GetString(IDS_BSC_DELETE_FILE));
		callback.AddMessage(filePath, 1);
			
		callback.PushTask(GetString(IDS_BSC_DELETE_FILE) + filePath, 2);
		//callback.SetNbStep(2);

		msg += RemoveFile(filePath);
		msg += callback.StepIt();

		string dataFilePath = filePath;
		SetFileExtension(dataFilePath, ".csv");
		msg += RemoveFile(dataFilePath);
		msg += callback.StepIt();


		std::string zop = N.GetOptimisationFilePath(filePath);
		if (FileExists(zop))
			msg += RemoveFile(zop);

		std::string zopData = N.GetOptimisationDataFilePath(filePath);
		if (FileExists(zopData))
			msg += RemoveFile(zopData);

		std::string zopSearchIndex = N.GetOptimisationSearchFilePath1(filePath);
		if (FileExists(zopSearchIndex))
			msg += RemoveFile(zopSearchIndex);

		std::string zopSearchData = N.GetOptimisationSearchFilePath2(filePath);
		if (FileExists(zopSearchData))
			msg += RemoveFile(zopSearchData);

		callback.PopTask();
	}

	return msg;
}

//ERMsg CNormalsDatabase::Convert(const string& filePath)
//{
//	ERMsg msg;
//	
//	short version = GetVersion(filePath);
//
//	//switch( version )
//	//{
//	//case 5: msg = Version5ToCurrent(filePath); break;
//	//case 6: msg = v6_to_v7(filePath, filePath); break;
//	//default: msg.ajoute( GetString(IDS_WG_BAD_VERSION) );
//	//}
//
//	return msg;
//}

ERMsg CNormalsDatabase::v6_to_v7(const string& filePath6, const std::string& filePathV7, CCallback& callback )
{
	ERMsg msg;

	

	ifStream stream;
	msg = stream.open(filePath6);

	//open new database
	CNormalsDatabase DB7;
	if (msg)
		msg = DB7.Open(filePathV7, modeWrite, callback);


	if (msg)
	{ 
		callback.PushTask("Convert " + GetFileName(filePath6), (size_t)stream.length());
		//callback.SetNbStep((size_t)stream.length());

		string line;
		std::getline(stream, line);//header1
		std::getline(stream, line);//header2


		while (!stream.eof() && msg)
		{
			CNormalsStation station;
			station.LoadV2(stream);
			if (!stream.eof() && station.IsValid() )
				DB7.Add(station);

			msg += callback.SetCurrentStepPos((size_t)stream.tellg());
		}
		
			
		msg += DB7.Close(true, callback);
		
		callback.PopTask();
	}

	return msg;
}

ERMsg CNormalsDatabase::v7_to_v6(const std::string& filePathV7, const std::string& filePathV6, CCallback& callback )
{
	ERMsg msg;

	CNormalsDatabase db;
	msg = db.Open(filePathV7, modeRead, callback);
	if (msg)
		msg = db.SaveAsV6(filePathV6, callback);

	return msg;
}

ERMsg CNormalsDatabase::SaveAsV6(const string& filePath, CCallback& callback)
{
	ASSERT(IsOpen());

	static const short NB_FIELDS = 16;
	static const short NB_LINE_BY_RECORD = 12;

	ERMsg msg;


	ofStream file;
	msg = file.open(filePath);
	if (msg)
	{ //create successful
		callback.PushTask("Convert " + GetFileName(filePath), m_zop.size());
		//callback.SetNbStep(m_zop.size());

		CTRef time = CTRef::GetCurrentTRef();
		string tmp = FormatA("%5d%5d%5d%5d%5d%5d", time.GetYear(), time.GetMonth(), time.GetDay(), GetFirstYear(), GetLastYear(), 6);

		string format = GetLineFormat();
		string line = FormatA(format.c_str(), tmp.c_str());
		file << line;

		//write header
		string tmp2;
		for (size_t f = 0; f < NB_FIELDS; f++)
		{
			string format = FormatA("%%-%d.%ds", RECORD_LENGTH, RECORD_LENGTH);
			string tmp = FormatA(format.c_str(), GetFieldHeader(f));
			tmp2 += tmp;
		}

		line = FormatA(format.c_str(), tmp2.c_str());
		file << line;
		
		
		//write data
		ASSERT(m_zop.size() == m_data.size());
		for (size_t i = 0; i < m_zop.size()&&msg; i++)
		{
			CWVariables vars = m_data[i].GetVariables();
			
			tmp2 = m_zop[i].m_name;
			line = FormatA(format.c_str(), tmp2.c_str());
			file << line;
			tmp2 = FormatA("%16.10lf %16.10lf %4.0lf %d", m_zop[i].m_lat, m_zop[i].m_lon, m_zop[i].m_elev, m_zop[i].UseIt()?1:0);
			line = FormatA(format.c_str(), tmp2.c_str());
			file << line;
			tmp2 = FormatA("%d %d %d %d 0 0 0", vars[H_TAIR2] ? 1 : 0, vars[H_PRCP] ? 1 : 0, vars[H_RELH] ? 1 : 0, vars[H_WNDS] ? 1 : 0);
			line = FormatA(format.c_str(), tmp2.c_str());
			file << line;
			tmp2 = m_zop[i].m_ID;
			if (!m_zop[i].GetSSI("MergedStationIDs").empty())
				tmp2 += "+" + m_zop[i].GetSSI("MergedStationIDs");
			line = FormatA(format.c_str(), tmp2.c_str());
			file << line;

			for (size_t m = 0; m < 12; m++)
			{
				for (size_t f = 0; f<NB_FIELDS; f++)
				{
					string record;
					if (!IsMissing(m_data[i][m][f]))
						record = FormatA(GetRecordFormat(f).c_str(), m_data[i][m][f]);
					else 
						record = GetEmptyRecord();

					ASSERT(record.length() >= RECORD_LENGTH);
					file << record.substr(0, RECORD_LENGTH);
				}
				file << endl;
			}

			msg += callback.StepIt();
		}
		file.close();

		callback.PopTask();
	}

	return msg;
}

//Get the order of the station by priority. Priority is gived to stations
//with more data. Get 1 point of priority by year and by category
void CNormalsDatabase::GetStationOrder(vector<size_t>& DBOrder)const
{
	DBOrder.clear();
	

	vector<pair<size_t, size_t>> indexedOrder;
	indexedOrder.reserve(m_zop.size());

	
	for (CNormalsDataDeque::const_iterator it = m_data.begin(); it != m_data.end(); it++)
	{
		//this station have data file	
		CWVariables vars = it->GetVariables();
		//CWCategories categories = vars.GetCategories();
		size_t priority = vars.count();

		indexedOrder.push_back(pair<size_t, size_t>(priority, it - m_data.begin()));
	}

	sort(indexedOrder.begin(), indexedOrder.end(), std::greater<pair<size_t, size_t>>());

	DBOrder.resize(indexedOrder.size());
	for (vector<pair<size_t, size_t>>::const_iterator it = indexedOrder.begin(); it != indexedOrder.end(); it++)
		DBOrder[it - indexedOrder.begin()] = it->second;


}


ERMsg CNormalsDatabase::CreateFromMerge(const string& filePath1, const string& filePath2, double distance, double deltaElev, size_t mergeType, CCallback& callback)
{
	ASSERT( m_openMode == modeEdit );

	ERMsg msg;
//	/*
//	string comment;
//	comment.FormatMessage(IDS_CMN_MERGE_DATABASE, m_filePath, filePath1, filePath2);
//	callback.AddMessage( comment );
//	callback.AddMessage("");
//
//	CNormalsDatabase inputDB1;
//	CNormalsDatabase inputDB2;
//
//	msg += inputDB1.Open( filePath1, CNormalsDatabase::modeReadOnly);
//	msg += inputDB2.Open( filePath2, CNormalsDatabase::modeReadOnly);
//	
//	if( !msg)
//		return msg;
//
//	int nbStationAdded=0;
//
//	CIntArray DB1Order;
//	CIntArray DB2Order;
//	inputDB1.GetStationOrder(DB1Order);
//	inputDB2.GetStationOrder(DB2Order);
//
//
//	CSortedArray<int,int> addedIndex1;
//	CSortedArray<int,int> addedIndex2;
//
//	callback.SetCurrentDescription( comment );
//	callback.SetCurrentStepRange(0, inputDB1.size()+inputDB2.size(), 1);
//	callback.SetStartNewStep();
//
//	for(int _i=0; _i<DB1Order.size()&&msg; _i++)
//	{
//		int i = DB1Order[_i];
//		if( addedIndex1.Lookup(i) != -1 )
//			continue;
//		
//
//		CSearchResultVector result1;
//		CSearchResultVector result2;
//		inputDB1.Match(inputDB1.GetStationHead(i), 6, result1);
//		inputDB2.Match(inputDB1.GetStationHead(i), 6, result2);
//		
//		for(int j=result1.size()-1; j>=0; j--)
//		{
//			if( result1[j].m_distance > distance || result1[j].m_deltaElev > deltaElev )
//			{
//				result1.RemoveAt(j);
//			}
//			else
//			{
//				//this station was already add with a another station: By RSA 27/05/2008
//				if( addedIndex1.Lookup(result1[j].m_index) != -1 )
//					result1.RemoveAt(j);
//			}
//		}
//		
//		for(int j=result2.size()-1; j>=0; j--)
//		{
//			if( result2[j].m_distance > distance || result2[j].m_deltaElev > deltaElev )
//			{
//				result2.RemoveAt(j);
//			}
//			else
//			{
//				//this station was already add with a another station
//				if( addedIndex2.Lookup(result2[j].m_index) != -1 )
//					result2.RemoveAt(j);
//			}
//		}
//
//		//there are at least the station itself
//		ASSERT( result1.size() > 0);
//
//		CNormalsStationVector stations1;
//		CNormalsStationVector stations2;
//		msg += inputDB1.GetStations( result1, stations1);
//		msg += inputDB2.GetStations( result2, stations2);
//		
//		//merge station from DB1 and DB2
//		CNormalsStation station;
//		msg = MergeStation(stations1, stations2, station, mergeType);
//
//
//		//add index of station merged from DB1		
//		for(int j=0; j<result1.size(); j++)
//		{
//			addedIndex1.AddSorted(result1[j].m_index);
//		}
//
//		//add index of station merged from DB2
//		for(int j=0; j<result2.size(); j++)
//		{
//			addedIndex2.AddSorted(result2[j].m_index);
//		}
//	
//
//		VERIFY( Add( station) );
//		nbStationAdded++;
//		msg += callback.StepIt();
//	}
//
//	//add station only in the DB2
//	for(int _i=0; _i<DB2Order.size()&&msg; _i++)
//	{
//		int i= DB2Order[_i];
//		if( addedIndex2.Lookup(i) != -1 )
//			continue;
//		
//
//		CSearchResultVector result;
//		inputDB2.Match(inputDB2.GetStationHead(i), 6, result);
//		
//		for(int j=result.size()-1; j>=0; j--)
//		{
//			if( result[j].m_distance > distance || result[j].m_deltaElev > deltaElev )
//				result.RemoveAt(j);
//			else
//			{
//				//this station was already merge
//				if( addedIndex2.Lookup(result[j].m_index) != -1 )
//					result.RemoveAt(j);
//			}
//		}
//		//there are at least the station itself
//		ASSERT( result.size() > 0);
//
//		
//		//msg = MergeStation(inputDB2, inputDB1, result, CSearchResultVector(), station, CDailyStationVector::FROM_MEAN);
//		
//		CNormalsStationVector stations;
//		msg += inputDB2.GetStations( result, stations);
//		
//		//merge station from DB1 and DB2
//		CNormalsStation station;
//		msg = MergeStation(stations, CNormalsStationVector(), station, mergeType);
//
//
//		for(int j=0; j<result.size(); j++)
//		{
//			addedIndex2.AddSorted(result[j].m_index);
//		}
//		
//		
//		VERIFY( Add( station) );
//		nbStationAdded++;
//
//		msg += callback.StepIt();
//	}
//	
//	comment.FormatMessage( IDS_CMN_NB_STATIONS_ADDED, IntToString( nbStationAdded )  );
//	callback.AddMessage( comment, 1);
//	*/
	return msg;
}
//
//ERMsg CNormalsDatabase::MergeStation( const CNormalsStationVector& stations1, const CNormalsStationVector & stations2, CNormalsStation& station, short mergeType)
//{
//	ASSERT( stations1.size() > 0);
//
//	ERMsg msg;
//	/*
//	
//	station.Reset();
//
//	if( mergeType == MERGE_FROM_MEAN)
//	{
//		CNormalsStationVector stations;
//		stations.Append(stations1);
//		stations.Append(stations2);
//		//stations.resize(results1.size()+results2.size());
//
//		//for(int i=0; i<stations.size(); i++)
//		//if( stations[i].GetID().Isclear() )
//		//{
//		//	string name = stations[i].GetName();
//		//	int pos = name.Find('-');
//		//	if( pos >= 0)
//		//		stations[i].SetID(name.Mid(pos+1));
//		//}
//		//int index = 0;
//
//
//		//if( msg )
//		stations.MergeStation( station, mergeType);
//		
//	}
//	else
//	{
//		//CDailyStationVector stations1;
//		//stations1.resize(results1.size());
//		//for(int i=0; i<results1.size(); i++)
//		//	msg += inputDB1.at( results1[i].m_index, stations1[i] );
//
//		//CDailyStationVector stations2;
//		//stations2.resize(results2.size());
//		//for(int i=0; i<results2.size(); i++)
//		//	msg += inputDB2.at( results2[i].m_index, stations2[i] );
//
//		//if( msg )
//		//{
//		CNormalsStationVector stations;
//		stations.resize(2);
//
//		stations1.MergeStation( stations[0], MERGE_FROM_MEAN);
//		stations2.MergeStation( stations[1], MERGE_FROM_MEAN);
//		stations.MergeStation( station, mergeType);
//		//}
//	}
//	*/
//	return msg;
//}

ERMsg CNormalsDatabase::RenameDatabase(const std::string& inputFilePath, const std::string& outputFilePath, CCallback& callback)
{
	ERMsg msg;
	
	if (FileExists(inputFilePath) )
	{
		if (!FileExists(outputFilePath) )
		{
			CNormalsDatabase N;
			msg = RenameFile(inputFilePath, outputFilePath);
			msg +=  RenameFile(GetNormalsDataFileName(inputFilePath), GetNormalsDataFileName(outputFilePath));

			std::string zopIn = N.GetOptimisationFilePath(inputFilePath);
			std::string zopOut = N.GetOptimisationFilePath(outputFilePath);
			if (FileExists(zopIn))
				msg += RenameFile(zopIn, zopOut);

			std::string zopDataIn = N.GetOptimisationDataFilePath(inputFilePath);
			std::string zopDataOut = N.GetOptimisationDataFilePath(outputFilePath);
			if (FileExists(zopDataIn))
				msg += RenameFile(zopDataIn, zopDataOut);

			std::string zopSearch1In = N.GetOptimisationSearchFilePath1(inputFilePath);
			std::string zopSearch1Out = N.GetOptimisationSearchFilePath1(outputFilePath);
			if (FileExists(zopSearch1In))
				msg += RenameFile(zopSearch1In, zopSearch1Out);

			std::string zopSearch2In = N.GetOptimisationSearchFilePath1(inputFilePath);
			std::string zopSearch2Out = N.GetOptimisationSearchFilePath1(outputFilePath);
			if (FileExists(zopSearch2In))
				msg += RenameFile(zopSearch2In, zopSearch2Out);
		}
		else
		{
			msg.ajoute("Unable to rename database: destination database already exist");
		}
	}
	else
	{
		msg.ajoute("Unable to rename database: source database doesn't exist");
	}


	return msg;

}


ERMsg CNormalsDatabase::VerifyDB(CCallback& callback)const 
{
	ERMsg msg;

	return msg;
}

ERMsg CNormalsDatabase::CreateFromMerge(const std::string& filePath1, const std::string& filePath2, double d, double deltaElev, size_t mergeType, size_t priorityRules, std::string& log, CCallback& callback)
{
	ERMsg msg;

	return msg;
}

bool CNormalsDatabase::IsExtendedDatabase(const std::string& filePath)
{
	return false;
}

ERMsg CNormalsDatabase::CreateDatabase(const std::string& filePath)
{
	ERMsg msg;

	CNormalsDatabase DB;
	msg = DB.Open(filePath, modeWrite);
	if (msg)
	{
		DB.Close();
		ASSERT(FileExists(filePath));
	}

	return msg;
}

}