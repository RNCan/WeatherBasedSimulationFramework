//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     R�mi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
// 12-11-2019	R�mi Saint-Amant	Bug correction in MostComplete
// 01-01-2016	R�mi Saint-Amant	Include into Weather-based simulation framework
//****************************************************************************
#include "stdafx.h"
#include <direct.h>

#pragma warning( disable : 4244 )
#include <boost\archive\binary_oarchive.hpp>
#include <boost\archive\binary_iarchive.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost\dynamic_bitset.hpp>

#include "Basic/WeatherDatabase.h"
#include "Basic/WeatherDatabaseCreator.h"
#include "Basic/SearchResult.h"
#include "Basic/UtilStd.h"
#include "Basic/GeoBasic.h"
#include "Basic/OpenMP.h"


#include "WeatherBasedSimulationString.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::WEATHER;

//***********************************************************************

namespace WBSF
{

	//global variable to find azure_dll
	std::string CWeatherDatabase::m_azure_dll_filepath;


	ERMsg CWeatherDatabase::ClearSearchOpt(const std::string& filePath)
	{
		ERMsg msg;
		string filePath1 = GetOptimisationSearchFilePath1(filePath);
		string filePath2 = GetOptimisationSearchFilePath2(filePath);

		__time64_t time = GetFileStamp(filePath);
		__time64_t time1 = GetFileStamp(filePath1);
		__time64_t time2 = GetFileStamp(filePath2);
		bool bRemove = time > time1 || time > time2;

		//Remove Search
		if (bRemove && FileExists(filePath1))
			msg += RemoveFile(filePath1);

		if (bRemove && FileExists(filePath2))
			msg += RemoveFile(filePath2);

		return msg;
	}



	std::set<int> CWeatherDatabase::GetYears(size_t index)const
	{
		ASSERT(IsOpen());

		if (m_openMode != modeBinary && m_zop.GetDataSection().GetFilePath().empty())
		{
			CWeatherDatabaseOptimization& zop = const_cast<CWeatherDatabaseOptimization&>(m_zop);
			zop.LoadData(GetOptimisationDataFilePath(m_filePath));
		}

		return m_zop.GetYears(index);
	}



	//****************************************************************************
	// Sommaire:    Constructeur par d�faut.
	//
	// Description: Cr�e la classe et assigne le path par default
	//
	// Entr�e:      const std::string& sPath: le path ou se situe la BD stations.rep
	//
	// Sortie:
	//
	// Note:        On doit initialiser cette classe avec un path
	//****************************************************************************
	CWeatherDatabase::CWeatherDatabase(int cacheSize) :
		m_hDll(nullptr),
		load_azure_weather_years(nullptr),
		m_cache(cacheSize)
	{
		m_bUseCache = cacheSize > 0;
		m_openMode = modeNotOpen;
		m_bModified = false;
	}

	//****************************************************************************
	// Sommaire:     Destructeur de la classe
	//
	// Description:  Lib�res la m�moire si la BD est encore ouverte
	//
	// Entr�e:
	//
	// Sortie:
	//
	// Note:
	//****************************************************************************
	CWeatherDatabase::~CWeatherDatabase()
	{
		if (IsOpen())
			Close();

		UnloadAzureDLL();
	}


	//****************************************************************************
	// Sommaire:    Ouvrir la Base de donn�es
	//
	// Description: La methode Open(const std::string, UINT) 
	//              permet d'obtenir la liste(nom) des stations qui sont dans la BD, bWithPrecipitation 
	//              force les stations a avoir de la pr�cipitation
	//
	// Entr�e:      Filter: possibiliter de filter les station qui sont incompl�te 
	//
	// Sortie:      StringVector& stationListName: la liste des station normals
	//              int: le nombre de stations trouver
	//
	// Note:        Si la BD n'est pas charg�e en m�moire, on la charge avec 
	//              CheckIfDBReady().
	//****************************************************************************
	ERMsg CWeatherDatabase::Open(const std::string& filePath, UINT flag, CCallback& callback, bool bSkipVerify)
	{
		ASSERT(!filePath.empty());
		ASSERT(m_openMode == modeNotOpen);

		ERMsg msg;

		if (flag == modeRead && !FileExists(filePath))
		{
			msg.ajoute("Unable to open: " + filePath);
			return msg;
		}

		if (IsOpen())
			Close();

		if (msg && flag == modeWrite)
		{
			string dataPath = GetDataPath(filePath);
			if (!DirectoryExists(dataPath))
			{
				msg = CreateMultipleDir(dataPath);
				if (!msg)
					return msg;
			}

			//create a new file
			if (!FileExists(filePath))
				msg = CWeatherDatabaseOptimization().SaveAsXML(filePath, GetSubDir(filePath), GetXMLFlag(), GetVersion(), GetHeaderExtension());

		}

		msg = OpenOptimizationFile(filePath, callback, flag == modeRead && bSkipVerify);

		//create data sub directory
		if (msg && flag == modeWrite)
		{
			if (!FileExists(filePath))//create a new database
				m_bModified = true;
		}

		if (msg)
		{
			m_openMode = flag;
			m_filePath = filePath;
		}

		return msg;
	}

	ERMsg CWeatherDatabase::Save()
	{
		ERMsg msg;

		if (m_bModified)
		{
			msg = m_zop.SaveAsXML(m_filePath, GetSubDir(m_filePath), GetXMLFlag(), GetVersion(), GetHeaderExtension());

			if (msg)
			{
				//save station coordinate
				msg += m_zop.Save(GetOptimisationFilePath());
				//if data section loaded then save it
				if (!m_zop.GetDataSection().GetFilePath().empty())
					msg += m_zop.SaveData(GetOptimisationDataFilePath());
			}


			if (msg)
			{
				ClearSearchOpt();
				m_bModified = false;
			}

		}


		m_cache.clear();


		return msg;
	}

	ERMsg CWeatherDatabase::Close(bool bSave, CCallback& callback)
	{
		ERMsg msg;
		if (m_openMode == modeWrite && bSave)
		{
			msg = Save();
		}

		m_openMode = modeNotOpen;
		m_filePath.clear();
		CloseSearchOptimization();

		return msg;
	}


	ERMsg CWeatherDatabase::OpenOptimizationFile(const std::string& referencedFilePath, CCallback& callback, bool bSkipVerify)
	{
		ERMsg msg;



		if (FileExists(referencedFilePath))
		{
			string headerFilePath = GetHeaderFilePath(referencedFilePath);
			if (FileExists(headerFilePath))
			{
				bool bStationsAsChange = true;
				bool bDataAsChange = true;

				std::string optFilePath = GetOptimisationFilePath(referencedFilePath);
				if (FileExists(optFilePath))
				{
					callback.AddMessage(FormatMsg(IDS_BSC_OPEN_FILE, GetFileName(optFilePath)));
					msg = m_zop.Load(optFilePath);
					if (msg)
					{
						if (m_zop.IsStationDefinitionUpToDate(headerFilePath))
						{
							bStationsAsChange = false;
						}
					}
				}


				if (msg && bStationsAsChange)
				{
					//if (!FileExists(referencedFilePath) && m_openMode==modeWrite)//save reference file from this optimization ?????????????hummm
					//msg = CWeatherDatabaseOptimization().SaveAsXML(referencedFilePath, GetSubDir(referencedFilePath), GetXMLFlag(), GetVersion());

					if (msg)
						msg = VerifyVersion(referencedFilePath);

					if (msg)
					{
						callback.AddMessage(FormatMsg(IDS_BSC_OPEN_FILE, GetFileName(referencedFilePath)));
						msg = m_zop.LoadFromXML(referencedFilePath, GetXMLFlag(), GetHeaderExtension());
					}
				}

				if (msg)
				{

					//Get data file to be updated
					CFileInfoVector fileInfo;

					if (bStationsAsChange || !bSkipVerify)
						msg = m_zop.GetDataFiles(fileInfo, true, callback);

					bDataAsChange = !fileInfo.empty();

					if (bDataAsChange)
					{
						callback.AddMessage(FormatMsg(IDS_BSC_UPDATE_FILE, GetFileName(optFilePath)));
						string dataOptFilePath = GetOptimisationDataFilePath(referencedFilePath);
						msg = m_zop.UpdateDataFilesYearsIndex(dataOptFilePath, fileInfo, callback);
					}
				}

				if (msg && (bStationsAsChange || bDataAsChange))
				{
					callback.AddMessage(FormatMsg(IDS_BSC_SAVE_FILE, GetFileName(optFilePath)));

					msg += m_zop.Save(optFilePath);
				}
			}
			else
			{
				msg.ajoute(FormatMsg(IDS_WG_DB_NOTEXIST, headerFilePath));
			}
		}
		else
		{
			msg.ajoute(FormatMsg(IDS_WG_DB_NOTEXIST, referencedFilePath));
		}

		msg += ClearSearchOpt(referencedFilePath);


		return msg;
	}


	ERMsg CWeatherDatabase::OpenSearchOptimization(CCallback& callback)
	{
		ASSERT(m_openMode == modeRead || m_openMode == modeBinary);

		ERMsg msg;


		if (m_openMode != modeBinary && m_zop.GetDataSection().GetFilePath().empty())
		{
			string filePath = GetOptimisationDataFilePath(m_filePath);
			callback.PushTask(FormatMsg(IDS_MSG_LOAD_OP, GetFileName(filePath)), NOT_INIT);
			CWeatherDatabaseOptimization& zop = const_cast<CWeatherDatabaseOptimization&>(m_zop);
			msg = zop.LoadData(filePath);
			callback.PopTask();
		}

		if (m_openMode != modeBinary && !m_zop.SearchIsOpen())
		{
			callback.PushTask(FormatMsg(IDS_MSG_LOAD_OP, GetFileName(GetOptimisationSearchFilePath1())), NOT_INIT);
			//run even of they are not able to open search optimization
			msg = m_zop.OpenSearch(GetOptimisationSearchFilePath1(), GetOptimisationSearchFilePath2());
			callback.PopTask();
		}



		return msg;
	}

	void CWeatherDatabase::CloseSearchOptimization()
	{
		m_zop.CloseSearch();
	}

	//****************************************************************************
	// Sommaire:    Obtenir une stations Quotidienne
	//
	// Description: Permet d'obtenir toutes les informations (incluant les donn�es
	//              de temp�rature) d'une station quotidienne pour une ann�e donn�e.
	//
	// Entr�e:  int index: le no de la station
	//          int year: l'ann�e 
	//
	// Sortie:  CWeatherStation& station: la station
	//          bool: true: la station a �t� trouv�e, faux autrement
	// 
	// Note:    par default on retourne toutes les ann�es d'une stations
	//**************************************************************************** 
	ERMsg CWeatherDatabase::Get(CLocation& station, size_t index, const std::set<int>& years)const
	{
		ERMsg msg;

		if (m_openMode != modeBinary && m_zop.GetDataSection().GetFilePath().empty())
		{
			m_CS.Enter();
			CWeatherDatabaseOptimization& zop = const_cast<CWeatherDatabaseOptimization&>(m_zop);
			msg = zop.LoadData(GetOptimisationDataFilePath(m_filePath));
			m_CS.Leave();
		}

		station = m_zop[index];

		return msg;
	}

	//****************************************************************************
	// Sommaire:    Obtenir une stations Quotidienne
	//
	// Description: Permet d'obtenir toutes les informations (incluant les donn�es
	//              de temp�rature) d'une station quotidienne pour une ann�e donn�e.
	//
	// Entr�e:  int index: le no de la station
	//          int year: l'ann�e 
	//
	// Sortie:  CWeatherStation& station: la station
	//          bool: true: la station a �t� trouv�e, faux autrement
	// 
	// Note:    par default on retourne toutes les ann�es d'une stations
	//**************************************************************************** 
	ERMsg CWeatherDatabase::Get(CLocation& station, size_t index, int year)const
	{
		std::set<int> years;

		if (year > YEAR_NOT_INIT)
		{
			ASSERT(year > 1700 && year <= 2100);
			years.insert(year);
		}


		return Get(station, index, years);
	}

	//****************************************************************************
	// Sommaire:     Ajouter une station Real Time � la BD
	//
	// Description: La methode push_back permet d'ajouter une station a la DB si cette 
	//              station n'y est pas.
	//
	// Entr�e:      CWeatherStation& station: la station � ajouter.
	//
	// Sortie:      bool: true si succes, false autrement
	//
	// Note:        Si la BD est charg�e en m�moire, on la decharge 
	//****************************************************************************
	ERMsg CWeatherDatabase::Add(const CLocation& location)
	{
		ASSERT(IsOpen());
		ASSERT(m_openMode == modeWrite);

		ERMsg msg;

		if (m_openMode != modeWrite)
		{
			msg.ajoute("Database is not open in write mode");
			return msg;
		}


		m_zop.push_back(location);
		m_bModified = true;

		return msg;
	}

	//****************************************************************************
	// Sommaire:     modifie une station RT de la BD
	//
	// Description: La methode Modify permet de modifier tous les champs d'une station
	//              RT, saul le nom(qui identifie la station). La station doit 
	//              exister dans la BD
	//
	// Entr�e:      CWeatherStation& station: la station � changer.
	//
	// Sortie:      bool: true si succes, false autrement
	//
	// Note:        Si la BD est charg�e en m�moire, on la decharge 
	//****************************************************************************
	ERMsg CWeatherDatabase::Set(size_t index, const CLocation& location)
	{
		ASSERT(IsOpen());
		ASSERT(m_openMode == modeWrite);
		ASSERT(index >= 0 && index < m_zop.size());

		ERMsg msg;
		if (m_openMode != modeWrite)
		{
			msg.ajoute("Database is not open in write mode");
			return msg;
		}


		if (location != m_zop[index])
		{
			m_zop.set(index, location);
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
	// Entr�e:      const std::string& sStationName: le nom de la station � supprimer
	//
	// Sortie:      bool: true si succes, false autrement
	//
	// Note:        Si la BD est charg�e en m�moire, on la decharge 
	//****************************************************************************
	ERMsg CWeatherDatabase::Remove(size_t index)
	{
		ASSERT(IsOpen());
		ASSERT(m_openMode == modeWrite);
		ASSERT(index >= 0 && index < m_zop.size());


		ERMsg msg;

		if (m_openMode != modeWrite)
		{
			msg.ajoute("Database is not open in write mode");
			return msg;
		}

		m_zop.erase(index);
		m_bModified = true;


		return msg;
	}


	//****************************************************************************
	// Sommaire:    Permet de savoir si la station existe ou nom dans la BD
	//
	// Description: Permet de savoir si la station existe ou nom dans la BD. Si la BD
	//              n'exite pas alors la station n'exite pas.
	//
	// Entr�e:      const std::string& sStationName: le nom de la station � trouver
	//
	// Sortie:      bool: true si elle existe, false autrement
	//
	// Note:        Si la BD est charg�e en m�moire, on cherche dans la BD m�moire
	//              sinon on cher dans celle sur le disque
	//****************************************************************************
	bool CWeatherDatabase::StationExist(const std::string& name, bool bByName)const
	{
		return GetStationIndex(name, bByName) >= 0;
	}

	int CWeatherDatabase::GetStationIndex(const std::string& strIn, bool bByName)const
	{
		ASSERT(IsOpen());

		string str = strIn;
		string::size_type pos = 0;
		if (!bByName)
			str = Tokenize(str, "+", pos);//take the first ID

		Trim(str);
		ASSERT(!str.empty());

		CWeatherDatabaseOptimization::const_iterator it = m_zop.end();

		if (bByName)
			it = std::find_if(m_zop.begin(), m_zop.end(), FindLocationByName(str));
		else
			it = std::find_if(m_zop.begin(), m_zop.end(), FindLocationByID(str));

		int index = -1;
		if (it != m_zop.end())
			index = std::distance(m_zop.begin(), it);

		return index;
	}

	//****************************************************************************
	// Sommaire:    Obtenir un LOC � partir de la BD RT
	//
	// Description: Permet de cr�er un LOC � partir des stations RT
	//
	// Entr�e:      float flat_low: latitude en degr�e dec. du premier coin
	//              float flon_low: longitude en degr�e dec. du premier coin
	//              float flat_hi:  latitude en degr�e dec. du deuxi�me coin
	//              float flon_hi:  longitude en degr�e dec. du deuxi�me coin
	//              int year: l'ann�e pour laquelle on fait la recherche
	//
	// Sortie:      LOCArray& locArray: un vecteur de station LOC
	//              bool: true si DB existe, false autrement
	//
	// Note:        Si la BD n'est pas charg�e en m�moire, on la charge avec 
	//              CheckIfDBReady().
	//****************************************************************************

	//1- for all available stations, create a list of pair of stations which are closest to each other.
	//2- from this list, take the pair that distance is less than median. 
	//3- remove station with less data.
	//4- do these step until the number of station is reach.
	//

	ERMsg CWeatherDatabase::GenerateWellDistributedStation(size_t nbStations, CSearchResultVector& searchResult, vector<size_t> priority, bool bUseElevation, CCallback& callback)const
	{
		assert(priority.size() == searchResult.size());

		ERMsg msg;

		//estimate a number of steps to reach objective
		double a = log(searchResult.size()) / log(4);
		double b = log(nbStations) / log(4);
		size_t c = ceil(a - b);

		callback.PushTask(GetString(IDS_MSG_GENERATE_LOC), c);


		//create a status vector
		CSearchResultVector resultNearest;
		resultNearest.resize(searchResult.size());

		boost::dynamic_bitset<size_t> status(searchResult.size());
		status.set();


		size_t step = 0;
		while (status.count() > nbStations && msg)
		{
			step++;
			callback.PushTask(FormatMsg(IDS_MSG_ELIMINATE_POINT, ToString(step)), searchResult.size() * 3);
			//callback.SetNbStep(searchResult.size() * 3);

			callback.AddMessage(FormatMsg(IDS_MSG_STATIONS_LEFT, ToString(status.count())));

			CLocationVector locations(status.count());
			vector<__int64> positions(status.count());
			for (size_t j = 0, jj = 0; j < searchResult.size() && msg; j++)
			{
				if (status[j])
				{
					locations[jj] = GetLocation(searchResult[j].m_index);
					positions[jj] = j;
					jj++;
				}

				msg += callback.StepIt();
			}

			CApproximateNearestNeighbor ann;
			ann.set(locations, bUseElevation, false, positions);

			CStatisticEx stats;
			vector<pair<double, size_t>> sorted_val(status.count());
			for (size_t j = 0, jj = 0; j < searchResult.size() && msg; j++)
			{

				if (status[j])
				{
					CSearchResultVector tmp;
					if (ann.search(locations[jj], 2ull, tmp))
					{
						ASSERT(tmp.size() == 2);
						ASSERT(tmp[0ull].m_index == j);

						resultNearest[j] = tmp[1ull];
						stats += resultNearest[j].m_virtual_distance;
						sorted_val[jj] = make_pair(resultNearest[j].m_virtual_distance, j);
					}

					jj++;
				}

				msg += callback.StepIt();
			}

			double median = stats[MEDIAN];
			//CSearchResultSort sortPred(CSearchResultSort::VIRTUAL_DISTANCE, true);
			std::sort(sorted_val.begin(), sorted_val.end());

			//find pair
			for (size_t i = 0; i < sorted_val.size() && status.count()>nbStations && msg; i++)
			{
				size_t j = sorted_val[i].second;
				if (status[j])
				{
					size_t jj = resultNearest[j].m_index;
					if (resultNearest[jj].m_index == j && status[jj])
					{
						ASSERT(fabs(resultNearest[j].m_virtual_distance - resultNearest[jj].m_virtual_distance) < 0.1);
						if (resultNearest[j].m_virtual_distance < median)
						{
							size_t priority1 = priority[j];
							size_t priority2 = priority[jj];
							if (priority1 < priority2)
								status.reset(j);
							else
								status.reset(jj);
						}
					}
				}

				msg += callback.StepIt();
			}

			callback.PopTask();
		}//while


		if (msg)
		{
			ASSERT(status.count() == nbStations);

			CSearchResultVector result;
			result.reserve(nbStations);
			for (size_t j = 0; j != resultNearest.size(); j++)
			{
				if (status[j])
					result.push_back(searchResult[j]);
			}

			searchResult = result;
		}

		callback.PopTask();

		return msg;
	}

	ERMsg CWeatherDatabase::GenerateLOC(CSearchResultVector& searchResult, size_t method, size_t nbStations, CWVariables filter, int year, bool bExcludeUnused, bool bUseElevation, const CGeoRect& boundingBox, CCallback& callBack)const
	{
		ASSERT(IsOpen());
		ASSERT(m_openMode == modeRead);
		ASSERT(boundingBox.IsRectNormal());

		ERMsg msg;

		searchResult.clear();
		msg = GetStationList(searchResult, filter, year, bExcludeUnused, boundingBox);

		if (msg)
		{
			if (nbStations == NOT_INIT)
				nbStations = searchResult.size();

			if (method == ALL_STATIONS)
			{
				//locations = GetLocations(searchResult);
			}
			else if (method == MOST_COMPLETE_STATIONS)
			{
				if (searchResult.size() > nbStations)
				{
					vector<size_t> order;
					msg = GetStationOrder(order, filter);

					for (vector<size_t>::iterator it = order.begin(); it != order.end();)
					{
						CSearchResultVector::const_iterator itFind = std::find(searchResult.begin(), searchResult.end(), *it);
						if (itFind == searchResult.end())
							it = order.erase(it);
						else
							it++;
					}

					//eliminate unselected stations
					searchResult.resize(nbStations);

					for (size_t i = 0; i != nbStations; i++)
						searchResult[i] = CSearchResult(order[i]);
				}
			}
			else if (method == WELL_DISTRIBUTED_STATIONS || method == COMPLETE_AND_DISTRIBUTED_STATIONS)
			{
				if (searchResult.size() > nbStations)
				{
					vector<size_t> priority(searchResult.size());
					if (method == COMPLETE_AND_DISTRIBUTED_STATIONS)
					{
						for (size_t i = 0; i != searchResult.size(); i++)
						{
							CWVariablesCounter counter = GetWVariablesCounter(searchResult[i].m_index, year);
							if (filter.any())
							{
								for (size_t v = 0; v < NB_VAR_H; v++)
									if (!filter[v])
										counter[v] = CCountPeriod();
							}

							priority[i] = counter.GetSum();
						}
					}

					//�liminate unselected stations
					msg = GenerateWellDistributedStation(nbStations, searchResult, priority, bUseElevation, callBack);
				}
			}
		}//if msg


		return msg;
	}

	CWVariables CWeatherDatabase::GetWVariables(size_t i, int year)const
	{
		std::set<int> years;
		if (year > YEAR_NOT_INIT)
			years.insert(year);

		return GetWVariables(i, years);
	}

	CWVariables CWeatherDatabase::GetWVariables(size_t i, const set<int>& years, bool bForAllYears)const
	{
		return m_zop.GetWVariables(i, years, bForAllYears);
	}

	CWVariablesCounter CWeatherDatabase::GetWVariablesCounter(size_t i, int year)const
	{
		std::set<int> years;
		if (year > YEAR_NOT_INIT)
			years.insert(year);

		return GetWVariablesCounter(i, years);
	}

	CWVariablesCounter CWeatherDatabase::GetWVariablesCounter(size_t i, const set<int>& years)const
	{
		return m_zop.GetWVariablesCounter(i, years);
	}

	ERMsg CWeatherDatabase::GetStationList(CSearchResultVector& results, CWVariables filter, const std::set<int>& years, bool bForAllYears, bool bExcludeUnused, const CGeoRect& boundingBoxIn)const
	{

		if (years.empty())
			return GetStationList(results, filter, YEAR_NOT_INIT, bExcludeUnused, boundingBoxIn);

		ERMsg msg;



		for (std::set<int>::const_iterator it = years.begin(); it != years.end(); it++)
		{
			CSearchResultVector tmp;
			msg += GetStationList(tmp, filter, *it, bExcludeUnused, boundingBoxIn);
			if (msg)
			{
				if (bForAllYears && it != years.begin())
					results &= tmp;
				else
					results |= tmp;
			}
		}

		return msg;
	}

	//mode can be in read or write mode
	ERMsg CWeatherDatabase::GetStationList(CSearchResultVector& searchResultArray, CWVariables filter, int year, bool bExcludeUnused, const CGeoRect& rectIn)const
	{
		ASSERT(IsOpen());

		ERMsg msg;

		if (m_openMode != modeBinary && m_zop.GetDataSection().GetFilePath().empty())
		{
			CWeatherDatabaseOptimization& zop = const_cast<CWeatherDatabaseOptimization&>(m_zop);
			msg = zop.LoadData(GetOptimisationDataFilePath(m_filePath));
		}

		if (year <= 0)
			year = -999;

		CGeoRect rect = rectIn;
		rect.NormalizeRect();


		searchResultArray.Reset();
		searchResultArray.reserve(size());
		//searchResultArray.SetYear(year);
		//searchResultArray.SetFilter(filter);

		for (size_t i = 0; i < size() && msg; i++)
		{
			const CLocation& station = at(i);

			CWVariables WVars = GetWVariables(i, year);
			bool bExclude = (bExcludeUnused && !station.UseIt());
			bool bOutside = (!rect.IsRectEmpty() && !rect.PtInRect(station));
			bool bMissingVariable = ((WVars & filter) != filter);

			if (WVars.any() &&
				!bExclude &&
				!bOutside &&
				!bMissingVariable)
			{
				CSearchResult result(i);
				result.m_location = station;
				searchResultArray.push_back(result);
			}
		}

		return msg;
	}

	//Search by radius instead of number of station
	void CWeatherDatabase::SearchD(CSearchResultVector& searchResultArray, const CLocation& location, double d, CWVariables filter, int year, bool bExcludeUnused, bool bUseElevation)const
	{
		const size_t NB_MATCH_MAX = 3;
		Search(searchResultArray, location, NB_MATCH_MAX, d, filter, year, bExcludeUnused, bUseElevation);

		//if no stations is farther than the distance with try to fin more stations
		for (size_t f = 2; f < 20 && !searchResultArray.empty() && searchResultArray.back().m_distance < d && searchResultArray.size() < size(); f *= 2)
			Search(searchResultArray, location, f * NB_MATCH_MAX, d, filter, year, bExcludeUnused, bUseElevation);

	}


	ERMsg CWeatherDatabase::GetPriority(vector<size_t>& priority, CWVariables filter, int year)const
	{
		ERMsg msg;


		if (m_openMode != modeBinary && m_zop.GetDataSection().GetFilePath().empty())
		{
			ASSERT(FileExists(GetOptimisationDataFilePath(m_filePath)));

			CWeatherDatabaseOptimization& zop = const_cast<CWeatherDatabaseOptimization&>(m_zop);
			msg = zop.LoadData(GetOptimisationDataFilePath(m_filePath));

		}


		priority.resize(m_zop.size());

		const CWeatherFileSectionIndex& sectionsIndex = m_zop.GetDataSection();
		for (CWeatherDatabaseOptimization::const_iterator it = m_zop.begin(); it != m_zop.end(); it++)
		{
			CWeatherFileSectionIndex::const_iterator it2 = sectionsIndex.find(it->GetDataFileName());
			if (it2 != sectionsIndex.end())
			{
				//this station have data file
				size_t p = 0;
				const CWeatherYearSectionMap& section = it2->second;
				//section.find(year);
				for (CWeatherYearSectionMap::const_iterator it3 = section.begin(); it3 != section.end(); it3++)
				{
					if (year == YEAR_NOT_INIT || it3->first == year)
					{
						const CWVariablesCounter& nbRecords = it3->second.m_nbRecords;
						ASSERT(nbRecords.size() == NB_VAR_H);
						for (size_t v = 0; v < NB_VAR_H; v++)
							if (filter[v])
								p += nbRecords[v].first;
					}
				}//year exist

				priority.push_back(p);
			}
		}

		return msg;
	}

	//Get the order of the station by priority. Priority is gived to stations
	//with more data. Get 1 point of priority by observations and by varaibles
	ERMsg CWeatherDatabase::GetStationOrder(vector<size_t>& DBOrder, CWVariables filter)const
	{
		ERMsg msg;

		DBOrder.clear();

		if (m_openMode != modeBinary && m_zop.GetDataSection().GetFilePath().empty())
		{
			ASSERT(FileExists(GetOptimisationDataFilePath(m_filePath)));

			CWeatherDatabaseOptimization& zop = const_cast<CWeatherDatabaseOptimization&>(m_zop);
			msg = zop.LoadData(GetOptimisationDataFilePath(m_filePath));

		}



		vector<pair<size_t, size_t>> indexedOrder;
		indexedOrder.reserve(m_zop.size());
		for (size_t i = 0; i != m_zop.size(); i++)
		{
			CWVariablesCounter counter = GetWVariablesCounter(i);
			CStatistic stat;
			for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)
			{
				if (!filter.any() || filter[v])
					stat += counter[v].first;
			}

			//if (stat[SUM]>0) on doit retourner toute lkes station sm�me si elle soint vide.
			indexedOrder.push_back(make_pair(size_t(stat[SUM]), i));
		}


		sort(indexedOrder.begin(), indexedOrder.end(), std::greater<pair<size_t, size_t>>());

		DBOrder.resize(indexedOrder.size());
		for (vector<pair<size_t, size_t>>::const_iterator it = indexedOrder.begin(); it != indexedOrder.end(); it++)
			DBOrder[it - indexedOrder.begin()] = it->second;


		return msg;
	}



	std::string CWeatherDatabase::GetUniqueName(const std::string& ID, const std::string& name)const
	{
		std::string newName = name;

		int xx = 2;
		while (StationExist(ID, false) && StationExist(newName))
		{
			newName = name + ToString(xx);
			xx++;
		}

		return newName;
	}

	std::ostream& CWeatherDatabase::operator << (std::ostream& stream)const
	{
		stream << m_zop;
		return stream;
	}

	std::istream& CWeatherDatabase::operator >> (std::istream& stream)
	{
		stream >> m_zop;
		m_openMode = modeBinary;
		m_bModified = false;
		m_filePath.clear();

		return stream;
	}

	ERMsg CWeatherDatabase::LoadAzureDLL()
	{
		ERMsg msg;

		if (m_hDll == nullptr)
		{
			string filePath = GetApplicationPath() + "External\\azure_weather.dll";
			if (!m_azure_dll_filepath.empty())
				filePath = m_azure_dll_filepath;

			//load the dll.
			m_hDll = LoadLibrary(convert(filePath).c_str());

			//if (m_hDll == nullptr)
			//{
			//	filePath = "azure_weather.dll";
			//	m_hDll = LoadLibrary(convert(filePath).c_str());
			//}

			////if not working try default path
			//if (m_hDll == nullptr)
			//{
			//	filePath = GetApplicationPath() + "azure_weather.dll";
			//	m_hDll = LoadLibrary(convert(filePath).c_str());
			//}


			if (m_hDll != nullptr)
			{
				load_azure_weather_years = (load_azure_weather_yearsF)GetProcAddress(m_hDll, "load_azure_weather_years");
				if (load_azure_weather_years == NULL)
					msg.ajoute(FormatMsg(IDS_BSC_UNABLE_GETFUNC, "load_azure_weather_years", filePath));
			}
			else
			{
				msg.ajoute(FormatMsg(IDS_BSC_UNABLE_LOADDLL, filePath));
			}

		}

		return msg;
	}

	void CWeatherDatabase::UnloadAzureDLL()
	{
		if (m_hDll)
		{
			//ASSERT(GetModuleHandleW(convert(GetDLLFilePath()).c_str()) != NULL);

			//to prevent a bug in the VCOMP100.dll we must wait 1 sec before closing dll
			//see http://support.microsoft.com/kb/94248
			Sleep(1000);
			bool bFree = FreeLibrary(m_hDll) != 0;
			if (bFree)
			{
				m_hDll = NULL;
				load_azure_weather_years = NULL;
			}

		}
	}
	//*********************************************************************************************************************

	ERMsg CDHDatabaseBase::VerifyDB(CCallback& callback)const
	{
		ASSERT(IsOpen());
		ASSERT(m_openMode == modeRead);

		ERMsg msg;
		//
		//
		//    callback.SetNbTask(3);
		////    callback.SetCurrentStepNo(0);
		//    callback.SetCurrentStepRange(0, m_zop.GetNbStation(), 1);
		//    callback.SetStartNewStep();
		//
		//    // Test pour savoir s'il y a des entr�es dont le fichier
		//    // de donn�es n'existe pas.
		//    std::string lastFileRead;
		//    vector<int> yearArray;
		//
		//    int nSize = m_zop.GetNbStation();
		//    for(int i=0; i<nSize; i++)
		//    {
		//		ASSERT( m_zop.GetStation(i).IsKindOf( RUNTIME_CLASS( CWeatherStation) ));
		//		CWeatherStation station = (const CWeatherStation&) m_zop.GetStation(i);
		//		
		//		msg = station.ReadData( GetDataPath() );
		//		callback.AddMessage(msg);
		//
		//		callback.StepIt();
		//		if( callback.GetUserCancel() )
		//		{
		//
		//			msg.asgType(ERMsg::ERREUR);
		//			msg.ajoute(GetString(IDS_CMN_USER_BREAK));
		//
		//			return msg;
		//		}
		//	}
		//
		//    callback.SetCurrentStepRange(0, nSize, 1);
		//    callback.SetStartNewStep();
		//
		//    //Verifion que tous les wea on une entr�e dans la table
		//	StringVector fileList;
		////	GetUnlinkedFile(fileList);
		//    
		//    nSize = GetFilesList(fileList, GetDataPath() + "*.wea");
		//    for(int i=0; i<fileList.size(); i++)
		//    {
		//		if( !PackageNameUsed(fileList[i]) )
		//        {
		//            msg.asgType(ERMsg::ERREUR);
		//
		//            std::string error;
		//            error.FormatMessage(IDS_FM_WEA_ALONE, (fileList[i]+".wea") );
		//            msg.ajoute(error);
		//            callback.AddMessage(error);
		//        }
		//
		//        callback.StepIt();
		//        if( callback.GetUserCancel() )
		//        {
		//            msg.asgType(ERMsg::ERREUR);
		//            msg.ajoute(GetString(IDS_CMN_USER_BREAK));
		//
		//            return msg;
		//        }
		//    }
		//
		////    callback.SetCurrentStepNo(2);
		//    callback.SetCurrentStepRange(0, m_zop.GetNbStation(), 1);
		//    callback.SetStartNewStep();
		//
		//    std::string lastStationName;
		//    std::string lastFileName;
		//    //verifion que chaque fichier .wea n'est ref�renc� que par une seule station
		//    nSize = m_zop.GetNbStation();
		//    for(int i=0; i<nSize; i++)
		//    {
		//		ASSERT( m_zop.GetStation(i).IsKindOf( RUNTIME_CLASS( CWeatherStation) ));
		//		const CWeatherStation& stationHead = (const CWeatherStation&) m_zop.GetStation(i);
		//    
		//		for(int j=0; j<stationHead.GetNbPackage(); j++)
		//		{
		//			for(int l=j+1; l<stationHead.GetNbPackage(); l++)
		//			{
		//				if( (stationHead[j].GetFileTitle().CompareNoCase(stationHead[l].GetFileTitle()) == 0))
		//				{
		//					msg.asgType(ERMsg::ERREUR);
		//
		//					std::string error;
		//					error.FormatMessage(IDS_FM_WEA_MANY_REFERENCE, stationHead[l].GetFileTitle(), stationHead.GetName(), stationHead.GetName() );
		//					msg.ajoute(error);
		//					callback.AddMessage(error);
		//				}
		//			}
		//
		//            for(int k=i+1; k<nSize; k++)
		//            {
		//		        ASSERT( m_zop.GetStation(k).IsKindOf( RUNTIME_CLASS( CWeatherStation) ));
		//				const CWeatherStation& stationHeadTmp = (const CWeatherStation&) m_zop.GetStation(k);
		//    
		//				for(int l=0; l<stationHeadTmp.GetNbPackage(); l++)
		//				{
		//					if( (stationHead[j].GetFileTitle().CompareNoCase(stationHeadTmp[l].GetFileTitle()) == 0))
		//					{
		//						msg.asgType(ERMsg::ERREUR);
		//
		//						std::string error;
		//						error.FormatMessage(IDS_FM_WEA_MANY_REFERENCE, stationHeadTmp[l].GetFileTitle(), stationHead.GetName(), stationHeadTmp.GetName() );
		//						msg.ajoute(error);
		//						callback.AddMessage(error);
		//					}
		//				}
		//            }
		//        }
		//
		//        callback.StepIt();
		//        if( callback.GetUserCancel() )
		//        {
		//            msg.asgType(ERMsg::ERREUR);
		//            msg.ajoute(GetString(IDS_CMN_USER_BREAK));
		//
		//            return msg;
		//        }
		//    }
		//
		//	callback.AddMessage( GetString( IDS_FM_VERIFYDB_COMPLET) );

		return msg;
	}


	ERMsg CDHDatabaseBase::CreateFromMerge(const std::string& filePath1, const std::string& filePath2, double d, double deltaElev, size_t mergeType, size_t priorityRules, std::string& log, CCallback& callback)
	{
		ASSERT(m_openMode == modeWrite);
		ASSERT(IsDailyDB(filePath1) == IsDailyDB(filePath2));
		ASSERT(IsHourlyDB(filePath1) == IsHourlyDB(filePath2));

		ERMsg msg;


		log = GetString(IDS_STR_MERGE_DB_LOG_HEADER) + "\n";

		string comment = FormatMsg(IDS_CMN_MERGE_DATABASE, m_filePath, filePath1, filePath2);
		callback.AddMessage(comment);
		callback.AddMessage("");

		CWeatherDatabasePtr pDB1 = CreateWeatherDatabase(filePath1);
		CWeatherDatabasePtr pDB2 = CreateWeatherDatabase(filePath2);

		msg += pDB1->Open(filePath1, CWeatherDatabase::modeRead, callback);
		msg += pDB2->Open(filePath2, CWeatherDatabase::modeRead, callback);
		if (!msg)
			return msg;

		vector<size_t> DB1Order;
		vector<size_t> DB2Order;
		msg += pDB1->GetStationOrder(DB1Order);
		msg += pDB2->GetStationOrder(DB2Order);
		if (!msg)
			return msg;


		int nbStationAdded = 0;

		boost::dynamic_bitset<size_t> addedIndex1(DB1Order.size());
		boost::dynamic_bitset<size_t> addedIndex2(DB2Order.size());

		//\n\t%2\n\t%3
		callback.PushTask(FormatMsg(IDS_CMN_MERGE_DATABASE, GetFileName(m_filePath), " \"" + GetFileName(filePath1) + "\", \"", GetFileName(filePath2) + "\""), pDB1->size() + pDB2->size());

		for (size_t _i = 0; _i < DB1Order.size() && msg; _i++)
		{
			size_t i = DB1Order[_i];
			if (!addedIndex1[i])
			{
				const CLocation& location = (*pDB1)[i];

				CSearchResultVector result1;
				CSearchResultVector result2;
				pDB1->SearchD(result1, location, d + 0.1);//+0.1 to accep itself
				pDB2->SearchD(result2, location, d + 0.1);//+0.1 to accep itself

				//keep only nearest stations of DB1
				for (CSearchResultVector::iterator it1 = result1.begin(); it1 != result1.end();)
				{
					if (addedIndex1[it1->m_index])
					{
						//this station was already add with a another station: By RSA 27/05/2008
						it1 = result1.erase(it1);
					}
					else if (it1->m_index == i)
					{
						//It's itself then take it
						it1++;
					}
					else if (it1->m_distance > d || fabs(it1->m_deltaElev) > deltaElev || location.UseIt() != pDB1->GetLocation(it1->m_index).UseIt())
					{
						//this station is too far
						it1 = result1.erase(it1);
					}
					else
					{
						it1++;
					}

				}

				//keep only nearest stations of DB2
				for (CSearchResultVector::iterator it2 = result2.begin(); it2 != result2.end();)
				{
					if (addedIndex2[it2->m_index])
					{
						//this station was already add with a another station: By RSA 27/05/2008
						it2 = result2.erase(it2);
					}
					else if (it2->m_distance > d || fabs(it2->m_deltaElev) > deltaElev || location.UseIt() != pDB2->GetLocation(it2->m_index).UseIt())
					{
						//this station is too far
						it2 = result2.erase(it2);
					}
					else
					{
						it2++;
					}
				}


				//there are at least the station itself
				ASSERT(result1.size() > 0);


				//merge station from DB1 and DB2
				CWeatherStation station(GetDataTM().Type() == CTM::HOURLY);
				msg = MergeStation(*pDB1, *pDB2, result1, result2, station, mergeType, priorityRules, log);

				//add index of station merged from DB1		
				for (CSearchResultVector::iterator it1 = result1.begin(); it1 != result1.end(); it1++)
					addedIndex1.set(it1->m_index);

				//add index of station merged from DB2
				for (CSearchResultVector::iterator it2 = result2.begin(); it2 != result2.end(); it2++)
					addedIndex2.set(it2->m_index);

				Trim(station.m_name);
				ASSERT(!station.m_name.empty());
				//Eliminate duplication in name
				string newName = GetUniqueName(station.m_ID, station.m_name);
				if (newName != station.m_name)
				{
					station.m_name = newName;
					station.SetDataFileName("");
				}

				//Force write file name in the file to convert ot the oold DB
				if (station.IsDaily())
					station.SetDataFileName(station.GetDataFileName());

				//Add station to the output database
				msg += Add(station);
				if (msg)
					nbStationAdded++;
			}

			msg += callback.StepIt();
		}//for DB1

		//add station only in the DB2
		for (size_t _i = 0; _i < DB2Order.size() && msg; _i++)
		{
			size_t i = DB2Order[_i];
			if (!addedIndex2[i])
			{

				const CLocation& location = pDB2->GetLocation(i);

				CSearchResultVector result;
				pDB2->SearchD(result, location, d + 0.1);//+0.1 to accep itself

				////Eliminate duplication in name
				for (CSearchResultVector::iterator it = result.begin(); it != result.end();)
				{
					if (addedIndex2[it->m_index])
					{
						//this station was already add with a another station: By RSA 27/05/2008
						it = result.erase(it);
					}
					else if (it->m_index == i)
					{
						//It's itself then take it
						it++;
					}
					else if (it->m_distance > d || fabs(it->m_deltaElev) > deltaElev || location.UseIt() != pDB2->GetLocation(it->m_index).UseIt())
					{
						//this station is too far
						it = result.erase(it);
					}
					else
					{
						it++;
					}

				}

				//there are at least the station itself
				ASSERT(result.size() > 0);

				//Merge from DB2 only
				CWeatherStation station(GetDataTM().Type() == CTM::HOURLY);
				msg = MergeStation(*pDB1, *pDB2, CSearchResultVector(), result, station, mergeType, priorityRules, log);

				for (CSearchResultVector::iterator it = result.begin(); it != result.end(); it++)
					addedIndex2.set(it->m_index);

				Trim(station.m_name);
				ASSERT(!station.m_name.empty());
				//Eliminate duplication in name
				string newName = GetUniqueName(station.m_ID, station.m_name);
				if (newName != station.m_name)
				{
					station.m_name = newName;
					station.SetDataFileName("");
				}

				//Add station to the output database
				msg += Add(station);
				if (msg)
					nbStationAdded++;
			}

			msg += callback.StepIt();
		}//for DB2


		comment = FormatMsg(IDS_CMN_NB_STATIONS_ADDED, ToString(nbStationAdded));
		callback.AddMessage(comment, 1);

		callback.PopTask();

		return msg;
	}

	ERMsg CDHDatabaseBase::MergeStation(CWeatherDatabase& inputDB1, CWeatherDatabase& inputDB2, const CSearchResultVector& results1, const CSearchResultVector& results2, CWeatherStation& station, size_t mergeType, size_t priorityRules, string& log)
	{
		ASSERT(results1.size() > 0 || results2.size() > 0);
		ASSERT(inputDB1.GetDataTM() == inputDB2.GetDataTM());

		ERMsg msg;
		CTM TM = inputDB1.GetDataTM();

		if (mergeType == MERGE_FROM_MEAN)
		{
			//Get all stations in the same vector
			CWeatherStationVector stations;
			stations.resize(results1.size() + results2.size());

			int index = 0;
			for (int i = 0; i < results1.size(); i++, index++)
			{
				msg += inputDB1.Get(stations[index], results1[i].m_index);
				stations[index].SetSSI("Source", "DB1");
			}
			for (int i = 0; i < results2.size(); i++, index++)
			{
				msg += inputDB2.Get(stations[index], results2[i].m_index);
				stations[index].SetSSI("Source", "DB2");
			}

			if (msg)
				stations.MergeStation(station, TM, mergeType, priorityRules, log);

		}
		else
		{
			//Get stations of DB1 and DB2 in separate vectors
			CWeatherStationVector stations1;
			stations1.resize(results1.size());
			for (int i = 0; i < results1.size(); i++)
			{
				msg += inputDB1.Get(stations1[i], results1[i].m_index);
				stations1[i].SetSSI("Source", "DB1");
				assert(!stations1[i].GetVariables()[H_ADD1]);
			}

			CWeatherStationVector stations2;
			stations2.resize(results2.size());
			for (int i = 0; i < results2.size(); i++)
			{
				msg += inputDB2.Get(stations2[i], results2[i].m_index);
				stations2[i].SetSSI("Source", "DB2");
				assert(!stations2[i].GetVariables()[H_ADD1]);
			}

			if (msg)
			{
				CWeatherStationVector stations;
				//stations.resize(2);


				if (!stations1.empty())
				{
					//Merge stations of DB1
					stations.resize(1);
					stations1.MergeStation(stations[0], TM, MERGE_FROM_MEAN, priorityRules, log);
				}

				if (!stations2.empty())
				{
					//Merge stations of DB2
					stations.resize(stations.size() + 1);
					stations2.MergeStation(stations[stations.size() - 1], TM, MERGE_FROM_MEAN, priorityRules, log);
				}

				//Merge stations of DB1 and DB2
				stations.MergeStation(station, TM, mergeType, priorityRules, log);
				assert(!station.GetVariables()[H_ADD1]);
			}
		}

		station.SetSSI("Source", "");

		return msg;
	}




	ERMsg CDHDatabaseBase::VerifyVersion(const std::string& filePath)const
	{
		ERMsg msg;


		if (FileExists(filePath))
		{
			if (GetVersion(filePath) != GetVersion())
			{
				std::string error = FormatMsg(IDS_WG_BAD_DAILY_VER, std::to_string(GetVersion(filePath)), std::to_string(GetVersion()));
				msg.ajoute(error);
				return msg;
			}
		}
		else
		{
			std::string error = FormatMsg(IDS_WG_DB_NOTEXIST, filePath);
			msg.ajoute(error);
		}

		return msg;
	}

	int CDHDatabaseBase::GetVersion(const std::string& filePath)
	{

		int nVersion = -1;

		ifStream file;
		ERMsg msg = file.open(filePath);
		if (msg)
		{
			std::string line;
			if (std::getline(file, line))
			{
				if (Find(line, "<?xml"))
				{
					nVersion = 3;
					if (std::getline(file, line))
					{
						MakeLower(line);
						std::string::size_type pos = line.find("version=");
						if (pos != std::string::npos)
						{
							Tokenize(line, "\" \t'", pos);
							if (pos >= 0)
								nVersion = std::stoi(Tokenize(line, "\" \t'", pos));
						}
					}
				}
				else
				{
					nVersion = 2;
				}

				file.close();
			}
		}

		return nVersion;
	}





	ERMsg CDHDatabaseBase::AppendDatabase(const std::string& inputFilePath1, const std::string& inputFilePath2, bool bCopy, CCallback& callback)
	{
		ASSERT(IsOpen());

		ERMsg msg;

		std::string inputPath1 = GetDataPath(inputFilePath1);
		std::string inputPath2 = GetDataPath(inputFilePath2);
		std::string outputPath = GetDataPath(m_filePath);

		if (FileExists(inputFilePath1) && FileExists(inputFilePath2) &&
			DirectoryExists(inputPath1) && DirectoryExists(inputPath2))
		{
			std::string comment = FormatMsg(IDS_BSC_COPY_FILE, inputFilePath1 + "\n\t" + inputFilePath2, m_filePath);

			CWeatherDatabaseOptimization zop1;
			CWeatherDatabaseOptimization zop2;
			msg += zop1.LoadFromXML(inputFilePath1, GetXMLFlag(), GetHeaderExtension());
			msg += zop2.LoadFromXML(inputFilePath2, GetXMLFlag(), GetHeaderExtension());

			if (msg)
			{
				callback.AddMessage(comment);
				callback.PushTask(comment, zop1.size() + zop2.size());

				m_zop.reserve(m_zop.size() + zop1.size() + zop2.size());
				for (auto it = zop1.begin(); it != zop1.end() && msg; it++)
				{
					std::string fileName = it->GetDataFileName();
					std::string oldDataFilePath = inputPath1 + fileName;
					std::string newDataFilePath1 = outputPath + fileName;
					std::string newDataFilePath2 = GenerateNewFileName(newDataFilePath1);

					if (bCopy)
						msg += CopyOneFile(oldDataFilePath, newDataFilePath2, false);
					else
						msg += RenameFile(oldDataFilePath, newDataFilePath2);

					if (newDataFilePath1 != newDataFilePath2)
					{
						//this file name already exist in the database rename it
						it->SetDataFileName(GetFileName(newDataFilePath2));
					}

					m_zop.push_back(*it);
					msg += callback.StepIt();
				}

				for (auto it = zop2.begin(); it != zop2.end() && msg; it++)
				{
					std::string fileName = it->GetDataFileName();
					std::string oldDataFilePath = inputPath2 + fileName;
					std::string newDataFilePath1 = outputPath + fileName;
					std::string newDataFilePath2 = GenerateNewFileName(newDataFilePath1);

					//msg += RenameFile(oldDataFilePath, newDataFilePath2);
					if (bCopy)
						msg += CopyOneFile(oldDataFilePath, newDataFilePath2, false);
					else
						msg += RenameFile(oldDataFilePath, newDataFilePath2);

					if (newDataFilePath1 != newDataFilePath2)
					{
						//this file name already exist in the database rename it
						it->SetDataFileName(GetFileName(newDataFilePath2));
					}

					m_zop.push_back(*it);
					msg += callback.StepIt();
				}


				m_bModified = true;
				comment = FormatMsg(IDS_CMN_NB_STATIONS_ADDED, ToString(zop1.size() + zop2.size()));
				callback.AddMessage(comment, 1);


				//m_zop.SaveAsXML(inputFilePath2, GetDataPath(inputFilePath2), GetXMLFlag(), GetVersion() );

				//if (msg)
				//{
					//remove both database
					//msg += DeleteDatabase(inputFilePath1, callback);
					//msg += DeleteDatabase(inputFilePath2, callback);
				//}

				callback.PopTask();
			}//if msg

		}


		return msg;

	}


	ERMsg CDHDatabaseBase::RenameDatabase(const std::string& inputFilePath, const std::string& outputFilePath, CCallback& callback)
	{
		ERMsg msg;

		std::string inputPath = WBSF::GetPath(inputFilePath);
		std::string outputPath = WBSF::GetPath(outputFilePath);

		if (FileExists(inputFilePath) && DirectoryExists(inputPath))
		{
			if (!FileExists(outputFilePath) && !DirectoryExists(outputPath))
			{
				RenameFile(inputFilePath, outputFilePath);//rename
				RenameDir(inputPath, outputPath);

				std::string zopIn = GetOptimisationFilePath(inputFilePath);
				std::string zopOut = GetOptimisationFilePath(outputFilePath);
				if (FileExists(zopIn))
					msg += RenameFile(zopIn, zopOut);

				std::string zopDataIn = GetOptimisationDataFilePath(inputFilePath);
				std::string zopDataOut = GetOptimisationDataFilePath(outputFilePath);
				if (FileExists(zopDataIn))
					msg += RenameFile(zopDataIn, zopDataOut);

				std::string zopSearch1In = GetOptimisationSearchFilePath1(inputFilePath);
				std::string zopSearch1Out = GetOptimisationSearchFilePath1(outputFilePath);
				if (FileExists(zopSearch1In))
					msg += RenameFile(zopSearch1In, zopSearch1Out);

				std::string zopSearch2In = GetOptimisationSearchFilePath1(inputFilePath);
				std::string zopSearch2Out = GetOptimisationSearchFilePath1(outputFilePath);
				if (FileExists(zopSearch2In))
					msg += RenameFile(zopSearch2In, zopSearch2Out);
			}
			else
			{
				callback.AddMessage(FormatMsg(IDS_BSC_UNABLE_RENAME, GetFileName(inputPath), GetFileName(outputPath)));

			}
		}
		else
		{
			callback.AddMessage(FormatMsg(IDS_BSC_UNABLE_RENAME, GetFileName(inputPath), GetFileName(outputPath)));
		}


		return msg;

	}

	ERMsg CDHDatabaseBase::DeleteDatabase(const std::string& filePath, CCallback& callback)
	{
		ERMsg msg;

		if (GetFileTitle(filePath).empty())
			return msg;

		if (FileExists(filePath))
		{
			std::string filter = GetDataPath(filePath) + "*.csv";
			StringVector files = GetFilesList(filter);

			callback.AddMessage(GetString(IDS_BSC_DELETE_FILE));
			callback.AddMessage(filePath, 1);

			callback.PushTask(GetString(IDS_BSC_DELETE_FILE) + filePath, files.size() + 9);

			for (size_t i = 0; i < files.size() && msg; i++)
			{
				if (FileExists(files[i]))
					msg = RemoveFile(files[i]);

				msg += callback.StepIt();
			}

			std::string dataPath = GetDataPath(filePath);
			if (DirectoryExists(dataPath))
			{
				ERMsg msg2 = RemoveDirectory(dataPath);
				if (!msg2)
					callback.AddMessage(msg2);
			}

			std::string zop = GetOptimisationFilePath(filePath);
			if (FileExists(zop))
				msg += RemoveFile(zop);

			std::string zopData = GetOptimisationDataFilePath(filePath);
			if (FileExists(zopData))
				msg += RemoveFile(zopData);

			std::string zopSearchIndex = GetOptimisationSearchFilePath1(filePath);
			if (FileExists(zopSearchIndex))
				msg += RemoveFile(zopSearchIndex);

			std::string zopSearchData = GetOptimisationSearchFilePath2(filePath);
			if (FileExists(zopSearchData))
				msg += RemoveFile(zopSearchData);

			std::string logFilePath = filePath + ".log.csv";
			if (FileExists(logFilePath))
				msg += RemoveFile(logFilePath);

			std::string header = GetHeaderFilePath(filePath);
			if (FileExists(header))
				msg += RemoveFile(header);

			msg += RemoveFile(filePath);


			//wait one second to let the system to clean is memeory
			//this avoid a problem accessing file
			for (size_t i = 0; i < 20 && msg; i++)
			{
				Sleep(50);//wait 50 milisec
				msg += callback.StepIt(0);
			}

			callback.PopTask();
		}


		return msg;
	}

	//****************************************************************************
	// Sommaire:    Permet de savoir la date de la derni�re modification
	//
	// Description: Permet d'avoir la date (CTime) de la derni�re modif � la BD
	//              en r�aliter GetLastModify retourne la date la plus r�cente
	//              entre le fichier stations.rep et tous les fichiers .wea
	//              Si la BD n'exite pas alors GetLastModify retourne false.
	//
	// Entr�e:      bool bVerifyAllFiles: true: v�rifie tous les fichiers .wea
	//                                    false: ne v�rifie que stations.rep
	//
	// Sortie:      CTime& fileModify: la date de la derni�re modif si succes
	//              bool: true si BD exite, false autrement
	//
	// Note:        
	//****************************************************************************
	__time64_t CDHDatabaseBase::GetLastUpdate(const std::string& filePath, bool bVerifyAllFiles)const
	{
		__time64_t lastUpdate = GetFileStamp(filePath);

		if (lastUpdate > 0 && bVerifyAllFiles)
		{
			CFileInfoVector info;
			if (IsNormalsDB(filePath))
				GetFilesInfo(WBSF::GetPath(filePath) + GetFileTitle(filePath) + ".csv", false, info);
			else
				GetFilesInfo(WBSF::GetPath(filePath) + GetFileTitle(filePath) + "\\*.csv", false, info);

			for (int i = 0; i < info.size(); i++)
			{
				if (info[i].m_time > lastUpdate)
					lastUpdate = info[i].m_time;
			}
		}


		return lastUpdate;
	}



	//*************************************************************************************************************************************************

	static std::string get_azure_data_file_name(CWeatherStation& station, int year)
	{
		string name = station.GetDataFileName();
		SetFileExtension(name, ".bin.gz");
		return to_string(year) + "/" + name;
	}


	//****************************************************************************
	// Sommaire:    Obtenir une stations Quotidienne
	//
	// Description: Permet d'obtenir une station quotidienne(incluant les donn�es
	//              de temp�rature) pour plusieur ann�es.
	//
	// Entr�e:  index: le no de la station
	//          years: array d'ann�es
	//
	// Sortie:  station: la station avec les donn�es
	//          ERMsg: message d'erreur
	// 
	// Note:    
	//****************************************************************************
	ERMsg CDHDatabaseBase::Get(CLocation& station, size_t index, const std::set<int>& yearsIn)const
	{
		ASSERT(IsOpen());
		ASSERT(index >= 0 && index < m_zop.size());

		ERMsg msg;



		msg = CWeatherDatabase::Get(station, index);

		//try to get the station from the cache
		if (msg)
		{


			CDHDatabaseBase& me = const_cast<CDHDatabaseBase&>(*this);
			CWeatherStation* pStation = dynamic_cast<CWeatherStation*>(&station);
			assert(pStation);

			std::set<int> years = yearsIn;
			if (years.empty())
				years = GetYears(index);

			//Load DLL if azure storage
			if (!m_account_name.empty() && !m_account_key.empty() && !m_container_name.empty())
				msg = me.LoadAzureDLL();


			if (m_bUseCache)
			{
				m_CS.Enter();
				if (m_cache.exists(index))
				{
					if (!me.m_cache.get(index).IsYearInit(years))
					{
						if (m_openMode == modeBinary)
						{
							msg = LoadBinary(static_cast<CWeatherStation*>(&me.m_cache.get(index)), years);
						}
						else
						{
							std::string dataFilePath = GetDataFilePath(station.GetDataFileName());
							msg = me.m_cache.get(index).LoadData(dataFilePath, MISSING, false, m_zop.GetDataSection().GetYearsSection(dataFilePath, years));
						}

					}


					//station = me.m_cache.get(index);//copy location
					pStation->SetHourly(me.m_cache.get(index).IsHourly());
					pStation->SetFormat(me.m_cache.get(index).GetFormat());
					//copy only wanted years
					for (std::set<int> ::const_iterator it = years.begin(); it != years.end(); it++)
					{
						int year = *it;
						pStation->CreateYear(year);
						CWeatherYear& weatherYear = pStation->at(year);
						weatherYear = me.m_cache.get(index).at(year);
					}
				}
				else
				{
					if (m_openMode == modeBinary)
					{
						msg = LoadBinary(pStation, years);
					}
					else
					{
						std::string dataFilePath = GetDataFilePath(station.GetDataFileName());
						msg = pStation->LoadData(dataFilePath, MISSING, false, m_zop.GetDataSection().GetYearsSection(dataFilePath, yearsIn));
					}

					if (msg)
						me.m_cache.put(index, *pStation);
				}

				m_CS.Leave();
			}
			else
			{
				if (m_openMode == modeBinary)
				{
					msg = LoadBinary(pStation, years);
				}
				else
				{
					std::string dataFilePath = GetDataFilePath(station.GetDataFileName());
					msg = pStation->LoadData(dataFilePath, MISSING, false, m_zop.GetDataSection().GetYearsSection(dataFilePath, yearsIn));
				}
			}
		}



		return msg;
	}



	ERMsg CDHDatabaseBase::LoadBinary(CWeatherStation* pStation, const std::set<int>& years)const
	{
		assert(pStation);

		ERMsg msg;
		if (!m_account_name.empty() && !m_account_key.empty() && !m_container_name.empty())
		{

			if (msg)
			{
				for (auto it = years.begin(); it != years.end(); it++)
				{
					string blob_name = m_DB_blob + "/" + get_azure_data_file_name(*pStation, *it);
					CWeatherYears* pWeather = static_cast<CWeatherYears*>(pStation);
					pWeather->CreateYear(*it);
					if (!load_azure_weather_years(m_account_name.c_str(), m_account_key.c_str(), m_container_name.c_str(), blob_name.c_str(), static_cast<void*>(pWeather)))
					{
						msg.ajoute("Blob not found: " + blob_name);
					}
				}
			}
		}
		else
		{
			//load binary local file
			for (auto it = years.begin(); it != years.end(); it++)
			{
				CWeatherYears* pWeather = static_cast<CWeatherYears*>(pStation);
				pWeather->CreateYear(*it);

				string DBName = GetPath() + GetFileTitle(m_filePath);
				string blob_name = DBName + "/" + get_azure_data_file_name(*pStation, *it);
				assert(!blob_name.empty());

				ifstream local_stream;
				local_stream.open(blob_name, ios::in | ios::binary);
				if (local_stream.is_open())
				{
					try
					{
						boost::iostreams::filtering_istreambuf in;
						in.push(boost::iostreams::gzip_decompressor());
						in.push(local_stream);
						std::istream incoming(&in);

						//CWeatherYears* pWeather = static_cast<CWeatherYears*>(pWeather);

						CTPeriod period;
						CWVariables variable;

						read_value(incoming, period);
						read_value(incoming, variable);
						assert(period.IsInit());

						for (CTRef TRef = period.Begin(); TRef <= period.End(); TRef++)
							pWeather->Get(TRef).ReadStream(incoming, variable, false);


					}
					catch (const boost::iostreams::gzip_error& exception)
					{
						int error = exception.error();
						if (error == boost::iostreams::gzip::zlib_error)
						{
							//check for all error code    
							msg.ajoute(exception.what());
						}
					}
				}
			}
		}
		return msg;
	}


	//****************************************************************************
	// Sommaire:     Ajouter une station Real Time � la BD
	//
	// Description: La methode push_back permet d'ajouter une station a la DB si cette 
	//              station n'y est pas.
	//
	// Entr�e:      CWeatherStation& station: la station � ajouter.
	//
	// Sortie:      bool: true si succes, false autrement
	//
	// Note:        Si la BD est charg�e en m�moire, on la decharge 
	//****************************************************************************
	ERMsg CDHDatabaseBase::Add(const CLocation& location)
	{
		ASSERT(IsOpen());
		ASSERT(m_openMode == modeWrite);

		ERMsg msg;

		msg = CWeatherDatabase::Add(location);
		if (msg)
		{
			const CWeatherStation& station = static_cast<const CWeatherStation&>(location);

			std::string filePath = GetDataFilePath(station.GetDataFileName());
			msg = station.SaveData(filePath, GetDataTM());
		}


		return msg;
	}

	//****************************************************************************
	// Sommaire:     modifie une station RT de la BD
	//
	// Description: La methode Modify permet de modifier tous les champs d'une station
	//              RT, saul le nom(qui identifie la station). La station doit 
	//              exister dans la BD
	//
	// Entr�e:      CWeatherStation& station: la station � changer.
	//
	// Sortie:      bool: true si succes, false autrement
	//
	// Note:        Si la BD est charg�e en m�moire, on la decharge 
	//****************************************************************************
	ERMsg CDHDatabaseBase::Set(size_t index, const CLocation& location)
	{
		ASSERT(IsOpen());
		ASSERT(m_openMode == modeWrite);

		if (index == m_zop.size())
			return Add(location);

		ASSERT(index < m_zop.size());

		ERMsg msg;

		std::string oldFileName = m_zop[index].GetDataFileName();
		std::string newFileName = location.GetDataFileName();
		if (oldFileName != newFileName)
		{
			std::string oldFilePath = GetDataFilePath(oldFileName);
			msg = RenameFile(oldFilePath, newFileName);
		}
		if (msg)
			msg = CWeatherDatabase::Set(index, location);

		if (msg)
		{
			//try to save data if available
			const CWeatherStation* pStation = dynamic_cast<const CWeatherStation*>(&location);
			if (pStation)
			{
				std::string newFilePath = GetDataFilePath(newFileName);
				msg += pStation->SaveData(newFilePath, GetDataTM().Type());
			}
		}

		return msg;
	}



	//****************************************************************************
	// Sommaire:     Supprime une station de BD normal
	//
	// Description: La methode Delete permet de supprimer une station de la DB.
	//              Si la station n'existe pas on ASSERT et on ne fait rien
	//
	// Entr�e:      const std::string& sStationName: le nom de la station � supprimer
	//
	// Sortie:      bool: true si succes, false autrement
	//
	// Note:        Si la BD est charg�e en m�moire, on la decharge 
	//****************************************************************************
	ERMsg CDHDatabaseBase::Remove(size_t index)
	{
		ASSERT(IsOpen());
		ASSERT(m_openMode == modeWrite);
		ASSERT(index >= 0 && index < m_zop.size());

		ERMsg msg;

		std::string filePath = GetDataFilePath(m_zop[index].GetDataFileName());
		msg = CWeatherDatabase::Remove(index);
		if (msg)
			msg = RemoveFile(filePath);

		return msg;
	}


	ERMsg CDHDatabaseBase::GetStations(CWeatherStationVector& stationArray, const CSearchResultVector& results, int year)const
	{
		ASSERT(IsOpen());

		ERMsg msg;

		stationArray.resize(results.size());

		//Get stations 
		for (size_t i = 0; i < results.size() && msg; i++)
			msg = Get(stationArray[i], results[i].m_index, year);

		return msg;
	}


	void CDHDatabaseBase::GetUnlinkedFile(StringVector& fileList)
	{
		ASSERT(IsOpen());
		ASSERT(m_openMode == modeRead);
		if (m_openMode != modeRead)
			return;
	}




	//****************************************************************************
	// Sommaire:    Trouve les stations RT les plus pr�s d'un point qqc.
	//
	// Description: Permet de trouver les n stations les plus pr�s d'un point qqc.
	//              Ne retourne pas les donn�es RT de temp�ratures.
	//              On peut changer la tol�rence � l'altitude, l'utilisation
	//              de zone climatique et le nombre de stations � trouver.
	//
	// Entr�e:      const CLocation& station: le point de recherche.
	//              int year: l'ann�e de recherche
	//              int altTol: la tol�rence en altitude
	//              bool bUseZone:  true: on utilise les zone climatique
	//                              false: on ne les utiliseas pas
	//              int nbStation: le nombre de stations � trouver
	//              
	//
	// Sortie:      CLocationVector& stationArray: un vecteur de stations
	//              int: le nombre de stations trouv�es
	//
	// Note:        Si la BD n'est pas charg�e en m�moire, on la charge avec 
	//              CheckIfDBReady().
	//              Si on ne trouve pas le nombre de stations d�sir�es, on 
	//              relache graduellement les contraintes.
	//****************************************************************************
	ERMsg CDHDatabaseBase::Search(CSearchResultVector& searchResultArray, const CLocation& station, size_t nbStation, double searchRadius, CWVariables filter, int year, bool bExcludeUnused, bool bUseElevation, bool bUseShoreDistance)const
	{
		ASSERT(IsOpen());
		ASSERT(m_openMode == modeRead || m_openMode == modeBinary);
		ASSERT(!m_bModified);//close and open the database again

		ERMsg msg;

		searchResultArray.Reset();


		if (searchRadius != 0)
		{
			if (m_openMode == modeRead && m_zop.GetDataSection().GetFilePath().empty())
			{
				assert(omp_get_num_threads() == 1);
				CWeatherDatabaseOptimization& zop = const_cast<CWeatherDatabaseOptimization&>(m_zop);
				msg = zop.LoadData(GetOptimisationDataFilePath(m_filePath));
			}

			if (m_openMode == modeRead && !m_zop.SearchIsOpen())
			{
				msg = m_zop.OpenSearch(GetOptimisationSearchFilePath1(), GetOptimisationSearchFilePath2());
				if (!msg)
					return msg;
			}


			m_CS.Enter();
			//__int64 canal = (filter.to_ullong()) * 100000 + max(year, 0) * 10 + (bUseShoreDistance ? 4 : 0) + (bUseElevation ? 2 : 0) + (bExcludeUnused ? 1 : 0);
			__int64 canal = m_zop.GetCanal(filter, year, bExcludeUnused, bUseElevation, bUseShoreDistance);
			if (!m_zop.CanalExists(canal))
			{
				const_cast<CDHDatabaseBase&>(*this).CreateCanal(filter, year, bExcludeUnused, bUseElevation, bUseShoreDistance);

				//CLocationVector locations;
				//locations.reserve(m_zop.size());
				//std::vector<__int64> positions;
				//positions.reserve(m_zop.size());

				//const CWeatherFileSectionIndex& index = m_zop.GetDataSection();
				////build canal
				//for (CLocationVector::const_iterator it = m_zop.begin(); it != m_zop.end(); it++)
				//{
				//	bool useIt = it->UseIt();
				//	if (useIt || !bExcludeUnused)
				//	{
				//		CWeatherFileSectionIndex::const_iterator it2 = index.find(it->GetDataFileName());
				//		if (it2 != index.end())
				//		{
				//			bool bIncluded = false;

				//			if (year > 0)
				//			{
				//				const CWeatherYearSectionMap& section = it2->second;
				//				CWeatherYearSectionMap::const_iterator it3 = section.find(year);
				//				if (it3 != section.end())
				//				{
				//					bool bIncluded2 = true;
				//					for (size_t i = 0; i < it3->second.m_nbRecords.size() && bIncluded2; i++)
				//					{
				//						if (filter.test(i))
				//							bIncluded2 = it3->second.m_nbRecords[i].first > 0;
				//					}

				//					bIncluded = bIncluded2;
				//				}//year exist
				//			}
				//			else
				//			{
				//				const CWeatherYearSectionMap& section = it2->second;
				//				for (CWeatherYearSectionMap::const_iterator it3 = section.begin(); it3 != section.end() && !bIncluded; it3++)
				//				{
				//					bool bIncluded2 = true;
				//					for (size_t i = 0; i < it3->second.m_nbRecords.size() && bIncluded2; i++)
				//					{
				//						if (filter.test(i))
				//							bIncluded2 = it3->second.m_nbRecords[i].first > 0;
				//					}

				//					if (bIncluded2)
				//						bIncluded = true;
				//				}//for all years
				//			}

				//			if (bIncluded)
				//			{
				//				CLocation pt = *it;//removel
				//				pt.m_siteSpeceficInformation.clear();//remove ssi for ANN
				//				locations.push_back(pt);
				//				positions.push_back(it - m_zop.begin());
				//			}
				//		}//File exist
				//	}//use it
				//}


				////by optimization, add the canal event if they are empty
				//CApproximateNearestNeighborPtr pANN(new CApproximateNearestNeighbor);
				//pANN->set(locations, bUseElevation, bUseShoreDistance, positions);
				//CWeatherDatabaseOptimization& zop = const_cast<CWeatherDatabaseOptimization&>(m_zop);
				//zop.AddCanal(canal, pANN);
			}
			m_CS.Leave();


			msg = m_zop.Search(station, nbStation, searchResultArray, canal);
		}

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
				string error = FormatMsg(IDS_WG_NOTENOUGH_OBSERVATION2, ToString(searchRadius / 1000, 1), string(m_filePath.empty() ? " " : GetFileName(m_filePath)), ToString(year), filterName);
				msg.ajoute(error);
			}
		}

		if (searchResultArray.size() != nbStation)
		{
			string filterName = filter.GetVariablesName('+');

			msg = ERMsg();//reset it and add the new message
			string error = FormatMsg(IDS_WG_NOTENOUGH_OBSERVATION, ToString(searchResultArray.size()), string(m_filePath.empty() ? " " : GetFileName(m_filePath)), ToString(year), ToString(nbStation), filterName);
			msg.ajoute(error);
		}

		return msg;
	}

	void CDHDatabaseBase::CreateCanal(CWVariables filter, int year, bool bExcludeUnused, bool bUseElevation, bool bUseShoreDistance)
	{
		__int64 canal = m_zop.GetCanal(filter, year, bExcludeUnused, bUseElevation, bUseShoreDistance);


		CLocationVector locations;
		locations.reserve(m_zop.size());
		std::vector<__int64> positions;
		positions.reserve(m_zop.size());

		const CWeatherFileSectionIndex& index = m_zop.GetDataSection();
		//build canal
		for (CLocationVector::const_iterator it = m_zop.begin(); it != m_zop.end(); it++)
		{
			bool useIt = it->UseIt();
			if (useIt || !bExcludeUnused)
			{
				CWeatherFileSectionIndex::const_iterator it2 = index.find(it->GetDataFileName());
				if (it2 != index.end())
				{
					bool bIncluded = false;

					if (year > 0)
					{
						const CWeatherYearSectionMap& section = it2->second;
						CWeatherYearSectionMap::const_iterator it3 = section.find(year);
						if (it3 != section.end())
						{
							bool bIncluded2 = true;
							for (size_t i = 0; i < it3->second.m_nbRecords.size() && bIncluded2; i++)
							{
								if (filter.test(i))
									bIncluded2 = it3->second.m_nbRecords[i].first > 0;
							}

							bIncluded = bIncluded2;
						}//year exist
					}
					else
					{
						const CWeatherYearSectionMap& section = it2->second;
						for (CWeatherYearSectionMap::const_iterator it3 = section.begin(); it3 != section.end() && !bIncluded; it3++)
						{
							bool bIncluded2 = true;
							for (size_t i = 0; i < it3->second.m_nbRecords.size() && bIncluded2; i++)
							{
								if (filter.test(i))
									bIncluded2 = it3->second.m_nbRecords[i].first > 0;
							}

							if (bIncluded2)
								bIncluded = true;
						}//for all years
					}

					if (bIncluded)
					{
						CLocation pt = *it;//removel
						pt.m_siteSpeceficInformation.clear();//remove ssi for ANN
						locations.push_back(pt);
						positions.push_back(it - m_zop.begin());
					}
				}//File exist
			}//use it
		}


		//by optimization, add the canal event if they are empty
		CApproximateNearestNeighborPtr pANN(new CApproximateNearestNeighbor);
		pANN->set(locations, bUseElevation, bUseShoreDistance, positions);
		CWeatherDatabaseOptimization& zop = const_cast<CWeatherDatabaseOptimization&>(m_zop);
		zop.AddCanal(canal, pANN);
	}

	void CDHDatabaseBase::CreateAllCanals(bool bExcludeUnused, bool bUseElevation, bool bUseShoreDistance)
	{
		set<int> years = GetYears();
		for (auto it = years.begin(); it != years.end(); it++)
		{
			for (TVarH v = H_FIRST_VAR; v < H_SRAD; v++)
				CreateCanal(v, *it, bExcludeUnused, bUseElevation, bUseShoreDistance);
		}
	}


	ERMsg CDHDatabaseBase::SaveAsBinary(const string& file_path)const
	{
		ASSERT(IsOpen());

		ERMsg msg;

		ofStream file;
		msg = file.open(file_path, ios::out | ios::binary);
		if (msg)
		{
			try
			{
				boost::iostreams::filtering_ostreambuf out;
				out.push(boost::iostreams::gzip_compressor());
				out.push(file);
				std::ostream outcoming(&out);

				//save coordinate and search optimization
				size_t version = GetVersion();
				outcoming.write((char*)(&version), sizeof(version));
				outcoming << *this;

				//now save data
				for (size_t i = 0; i < size(); i++)
				{
					CWeatherStation station;
					Get(station, i);

					set<int> years = station.GetYears();
					for (auto it = years.begin(); it != years.end(); it++)
					{
						string data_filepath = WBSF::GetPath(file_path) + GetFileTitle(file_path) + "/" + get_azure_data_file_name(station, *it);
						WBSF::CreateMultipleDir(WBSF::GetPath(data_filepath));

						data_filepath = WBSF::ANSI_2_ASCII(RemoveAccented(data_filepath));//remove all accent caracters;


						ofStream data_file;
						msg = data_file.open(data_filepath, ios::out | ios::binary);
						if (msg)
						{
							try
							{
								boost::iostreams::filtering_ostreambuf data_out;
								data_out.push(boost::iostreams::gzip_compressor());
								data_out.push(data_file);
								std::ostream data_outcoming(&data_out);

								station.GetStat(H_TMIN);//compute stat
								const CWeatherYear& weather = station[*it];
								CWVariablesCounter count = weather.GetVariablesCount();

								CTPeriod period = count.GetTPeriod();
								CWVariables variable = count.GetVariables();

								write_value(data_outcoming, period);
								write_value(data_outcoming, variable);

								for (CTRef TRef = period.Begin(); TRef <= period.End(); TRef++)
								{
									const CDataInterface& data = weather.Get(TRef);
									data.WriteStream(data_outcoming, variable, false);
									/*for (size_t v = 0; v < variable.size(); v++)
									{
										if (variable[v])
										{
											CStatistic stat;
											data.GetStat(TVarH(v), stat);
											float value = stat.IsInit() ? stat[MEAN]:-999;
											write_value(data_outcoming, value);
										}
									}*/
								}
							}
							catch (const boost::iostreams::gzip_error& /*exception*/)
							{
								//int error = exception.error();
								//if (error == boost::iostreams::gzip::zlib_error)
								//{
									//check for all error code    
									//msg.ajoute(exception.what());
								//}
							}
						}

						data_file.close();
					}

				}
			}
			catch (const boost::iostreams::gzip_error& exception)
			{
				int error = exception.error();
				if (error == boost::iostreams::gzip::zlib_error)
				{
					//check for all error code    
					msg.ajoute(exception.what());
				}
			}


			file.close();
		}

		return msg;
	}


	ERMsg CDHDatabaseBase::LoadFromBinary(const string& file_path)
	{
		ERMsg msg;
		ifStream file;
		msg = file.open(file_path, ios::in | ios::binary);
		if (msg)
		{
			try
			{
				boost::iostreams::filtering_istreambuf in;
				in.push(boost::iostreams::gzip_decompressor());
				in.push(file);
				std::istream incoming(&in);

				size_t version = 0;
				incoming.read((char*)(&version), sizeof(version));

				if (version == GetVersion())
				{
					incoming >> *this;
					m_filePath = file_path;
				}
				else
				{
					msg.ajoute("Daily binary database (version = " + to_string(version) + ") was not created with the latest version (" + to_string(GetVersion()) + "). Rebuild new binary.");
				}

			}
			catch (const boost::iostreams::gzip_error& exception)
			{
				int error = exception.error();
				if (error == boost::iostreams::gzip::zlib_error)
				{
					//check for all error code    
					msg.ajoute(exception.what());
				}
			}

			file.close();
		}


		return msg;
	}

}