//Integrated Surface Data - “Lite”
#include "stdafx.h"
#include "UIISDLite.h"
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include "Basic/FileStamp.h"
#include "Basic/DailyDatabase.h"
#include "Basic/Registry.h"
#include "Basic/CSV.h"
#include "Basic/ExtractLocationInfo.h"
#include "Geomatic/ShapeFileBase.h"
#include "UI/Common/SYShowMessage.h"
#include "TaskFactory.h"
#include "CountrySelection.h"
#include "StateSelection.h"
#include "ProvinceSelection.h"
#include "../Resource.h"
#include "Geomatic/TimeZones.h"
#include "cctz/time_zone.h"

//autre source
//https://www.ncei.noaa.gov/data/global-hourly/access/2020/

//fichier compresser
//https://www.ncei.noaa.gov/data/global-hourly/archive/isd/
//ou en format csv
//https://www.ncei.noaa.gov/data/global-hourly/archive/csv/


using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;

namespace WBSF
{

	const char* CUIISDLite::SERVER_NAME = "ftp.ncdc.noaa.gov";
	const char* CUIISDLite::SERVER_PATH = "pub/data/noaa/isd-lite/";
	const char* CUIISDLite::LIST_PATH = "pub/data/noaa/";

	//*********************************************************************
	const char* CUIISDLite::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "FirstYear", "LastYear", "Countries", "States", "Province" };
	const size_t CUIISDLite::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_STRING, T_STRING, T_STRING_SELECT, T_STRING_SELECT, T_STRING_SELECT };
	const UINT CUIISDLite::ATTRIBUTE_TITLE_ID = IDS_UPDATER_NOAA_ISD_LITE_P;
	const UINT CUIISDLite::DESCRIPTION_TITLE_ID = ID_TASK_NOAA_ISD_LITE;

	const char* CUIISDLite::CLASS_NAME() { static const char* THE_CLASS_NAME = "ISD-Lite";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIISDLite::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIISDLite::CLASS_NAME(), (createF)CUIISDLite::create);



	static const CGeoRect DEFAULT_BOUDINGBOX(-180, -90, 180, 90, PRJ_WGS_84);

	CUIISDLite::CUIISDLite(void)
	{}

	CUIISDLite::~CUIISDLite(void)
	{
	}


	std::string CUIISDLite::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case COUNTRIES:	str = CCountrySelectionGADM::GetAllPossibleValue(); break;
		case STATES:	str = CStateSelection::GetAllPossibleValue(); break;
		case PROVINCES:	str = CProvinceSelection::GetAllPossibleValue(); break;
		};
		return str;
	}

	std::string CUIISDLite::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "ISD-Lite\\"; break;
		case FIRST_YEAR:
		case LAST_YEAR:	str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
		};

		return str;
	}

	//*****************************************************************************

	string CUIISDLite::GetHistoryFilePath(bool bLocal)const
	{
		return (bLocal ? GetDir(WORKING_DIR) : string(LIST_PATH)) + "isd-history.csv";
	}

	string CUIISDLite::GetOutputFilePath(const string& ID, short year, const string& ext)const
	{
		return GetDir(WORKING_DIR) + ToString(year) + "\\" + ID + "-" + ToString(year) + ext;
	}


	ERMsg CUIISDLite::UpdateStationHistory(CCallback& callback)const
	{
		ERMsg msg;

		CInternetSessionPtr pSession;
		CFtpConnectionPtr pConnection;

		msg = GetFtpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", true, 5, callback);
		if (msg)
		{


			string path = GetHistoryFilePath(false);

			CFileInfoVector fileList;
			msg = FindFiles(pConnection, path, fileList, false, CCallback::DEFAULT_CALLBACK);
			if (msg)
			{
				ASSERT(fileList.size() == 1);


				string outputFilePathTmp = GetHistoryFilePath(true);
				SetFileTitle(outputFilePathTmp, GetFileTitle(outputFilePathTmp) + "-tmp");

				if (!IsFileUpToDate(fileList[0], outputFilePathTmp))
				{

					msg = CopyFile(pConnection, fileList[0].m_filePath, outputFilePathTmp);

					string outputFilePath = GetHistoryFilePath(true);
					msg = ExtractCountrySubDivision(outputFilePathTmp, outputFilePath, callback);
				}

			}

			pConnection->Close();
			pSession->Close();
		}

		return msg;
	}


	ERMsg CUIISDLite::ExtractCountrySubDivision(const std::string& inFilePath, const std::string& outFilePath, CCallback& callback)const
	{
		ERMsg msg;

		string path = GetPath(inFilePath);
		std::string correction_file_path = path + "isd-corrections.csv";
		CLocationVector correction;
		correction.Load(correction_file_path);


		CLocationVector old_locations;
		old_locations.Load(outFilePath);

		std::string invalid_coord_file_path = path + "isd-invalid-stations.csv";
		CLocationVector old_invalid_Stations;
		old_invalid_Stations.Load(invalid_coord_file_path);

		ofStream invalid;
		msg += invalid.open(invalid_coord_file_path);

		CRegistry registry;
		std::string GADM_file_path = registry.GetString(CRegistry::GetGeoRegistryKey(L_WORLD_GADM));

		CShapeFileBase shapefile;
		bool bGADM = shapefile.Read(GADM_file_path);
		if (!bGADM)
			callback.AddMessage("WARNING: unable to load correctly GADM file: " + GADM_file_path);


		std::locale utf8_locale = std::locale(std::locale::classic(), new std::codecvt_utf8<char>());

		ifStream file;
		file.imbue(utf8_locale);
		msg += file.open(inFilePath);
		if (msg)
		{

			if (file.length() > 1000)
			{
				callback.PushTask("Extract country and administrative sub-division (isd)", file.length());
				invalid << "ID,Name,Latitude,Longitude,Elevation,Country1,SubDivision1,Country2,SubDivision2,Distance(km),Comment" << endl;


				CLocationVector locations;
				locations.reserve(40000);


				for (CSVIterator loop(file, ",", true, true); loop != CSVIterator() && msg; ++loop)
				{
					CLocation location = LocationFromLine(*loop);
					//make a manual correction if any
					size_t corr_pos = correction.FindByID(location.m_ID);
					bool bManualCorrection = corr_pos != NOT_INIT;
					if (bManualCorrection)
					{
						location = correction[corr_pos];
					}

					bool bExclude = location.GetSSI("Excluded") == "1";
					if (!bExclude)
					{
						if (location.IsValid(true))
						{
							size_t invalid_pos = old_invalid_Stations.FindByID(location.m_ID);
							bool bInvalid = invalid_pos != NOT_INIT;
							bool bNeedExtration = true;


							//find the old station information if any
							size_t oldPos = old_locations.FindByID(location.m_ID);
							if (!bInvalid && !bManualCorrection && oldPos != NOT_INIT)
							{
								bool bDiffLat = fabs(location.m_lat - old_locations[oldPos].m_lat) > 0.001;
								bool bDiffLon = fabs(location.m_lon - old_locations[oldPos].m_lon) > 0.001;
								bool bDiffAlt = old_locations[oldPos].m_alt == -999 || (location.m_alt != -999 && fabs(location.m_alt - old_locations[oldPos].m_alt) > 1);
								bNeedExtration = bDiffLat || bDiffLon || bDiffAlt;

								if (!bNeedExtration)
									location = old_locations[oldPos];
							}


							if (bNeedExtration && bGADM)
							{
								string country = location.GetSSI("Country");
								string subDivision = location.GetSSI("SubDivision");
								string countryII = "--";
								string subDivisionII = "--";
								double d = GetCountrySubDivision(shapefile, location.m_lat, location.m_lon, country, subDivision, countryII, subDivisionII);

								if (countryII == "--")
								{
									invalid << location.m_ID << "," << location.m_name << "," << ToString(location.m_lat, 4) << "," << ToString(location.m_lon, 4) << "," << to_string(location.m_alt) << "," << country << "," << subDivision << "," << countryII << "," << subDivisionII << "," << to_string(Round(d / 1000, 1)) << "," << "UnknownCountry" << endl;
									country = "UNK";//Unknown
								}
								else
								{
									bool bBuoy = location.m_name.find("Buoy")!=string::npos || location.m_name.find("Platform") != string::npos;

									if (!country.empty() && country != countryII && d > 20000 && !bBuoy)
										invalid << location.m_ID << "," << location.m_name << "," << ToString(location.m_lat, 4) << "," << ToString(location.m_lon, 4) << "," << to_string(location.m_alt) << "," << country << "," << subDivision << "," << countryII << "," << subDivisionII << "," << to_string(Round(d / 1000, 1)) << "," << "MissmatchCountry" << endl;

									if (!country.empty() && country == countryII && d > 20000 && !bBuoy)
										invalid << location.m_ID << "," << location.m_name << "," << ToString(location.m_lat, 4) << "," << ToString(location.m_lon, 4) << "," << to_string(location.m_alt) << "," << country << "," << subDivision << "," << countryII << "," << subDivisionII << "," << to_string(Round(d / 1000, 1)) << "," << "BadCountryID" << endl;

									country = countryII;

									if (!subDivision.empty() && subDivisionII != "--" && subDivisionII != subDivision && d > 20000)
										invalid << location.m_ID << "," << location.m_name << "," << ToString(location.m_lat, 4) << "," << ToString(location.m_lon, 4) << "," << to_string(location.m_alt) << "," << country << "," << subDivision << "," << countryII << "," << subDivisionII << "," << to_string(Round(d / 1000, 1)) << "," << "MissmatchSubDivision" << endl;

									subDivision = subDivisionII;
								}

								location.SetSSI("Country", country);
								location.SetSSI("SubDivision", subDivision);
								location.SetSSI("d", to_string(Round(d / 1000, 1)));
								location.SetSSI("ElevType", (location.m_alt == -999) ? "SRTM" : "NOAA");

							}

							locations.push_back(location);

						}
						else//invalid lat/lon
						{
							//callback.AddMessage("BadLocation," + location.m_ID + "," + location.m_name + "," + ToString(location.m_lat, 4) + "," + ToString(location.m_lon, 4) + "," + to_string(location.m_alt) );
							string country = location.GetSSI("Country");
							string subDivision = location.GetSSI("SubDivision");
							//<< "," << (bExclude ? "1" : "0") 
							invalid << location.m_ID << "," << location.m_name << "," << ToString(location.m_lat, 4) << "," << ToString(location.m_lon, 4) << "," << to_string(location.m_alt) + "," << country << "," << subDivision << "," << "," << "," << "," << "BadLocation" << endl;
						}
					}

					msg += callback.StepIt(loop->GetLastLine().length() + 1);
				}//for all lines

				if (bGADM)
					shapefile.Close();

				invalid.close();
				file.close();


				callback.AddMessage("Number of stations: " + to_string(locations.size()));
				callback.PopTask();

				if (msg)
				{
					//extraxt missing elevations
					ASSERT(locations.IsValid(true));

					//extract missing name
					msg = locations.ExtractNominatimName(false, true, false, false, callback);


					//if missing elevation, extract elevation at 30 meters: not support < -60° and > 60°
					if (!locations.IsValid(false))
						msg = locations.ExtractOpenTopoDataElevation(false, COpenTopoDataElevation::NASA_SRTM30M, COpenTopoDataElevation::I_BILINEAR, callback);

					//if still missing elevation, extract elevation at 30 meters ASTER
					if (msg && !locations.IsValid(false))
						msg = locations.ExtractOpenTopoDataElevation(false, COpenTopoDataElevation::NASA_ASTER30M, COpenTopoDataElevation::I_BILINEAR, callback);

					if (/*msg && */!locations.IsValid(false))
					{
						for (CLocationVector::iterator it = locations.begin(); it != locations.end(); )
						{
							if (!it->IsValid(false))
							{
								callback.AddMessage("WARNING: invalid coordinate :" + it->m_name + "(" + it->m_ID + "), " + "lat=" + to_string(it->m_lat) + ", lon=" + to_string(it->m_lon) + ", elev=" + to_string(it->m_alt), 2);
								it = locations.erase(it);
							}
							else
							{
								it++;
							}
						}
					}

					//save event if extract to fail

					msg += locations.Save(outFilePath);

					set<string> unknow_country;
					for (CLocationVector::iterator it = locations.begin(); it != locations.end(); it++)
					{
						size_t country = CCountrySelectionGADM::GetCountry(it->GetSSI("Country"));

						if (country == -1)
							unknow_country.insert(it->GetSSI("Country"));
					}

					for (auto it = unknow_country.begin(); it != unknow_country.end(); it++)
					{
						size_t c = CCountrySelectionGADM::GetCountry(*it);
						string name = c != NOT_INIT ? CCountrySelectionGADM::GetName(c, 1) : "";
						callback.AddMessage("WARNING: unknown country: " + *it + "," + name);
					}
				}
			}
			else
			{
				callback.AddMessage("WARNING: empty stations list file");
				callback.AddMessage("Removing " + inFilePath);
				msg += WBSF::RemoveFile(inFilePath);
			}

		}

		return msg;
	}


	CLocation CUIISDLite::LocationFromLine(const StringVector& line)const
	{
		enum TColumn { C_USAF, C_WBAN, C_STATION_NAME, C_CTRY, C_STATE, C_ICAO, C_LAT, C_LON, C_ELEV, C_BEGIN, C_END, NB_COLUMNS };
		ASSERT(line.size() == NB_COLUMNS);

		CLocation location;

		ASSERT(!line[C_USAF].empty());
		ASSERT(!line[C_WBAN].empty());

		location.SetSSI("USAF_ID", "USAF" + line[C_USAF]);
		location.SetSSI("WBAN_ID", "WBAN" + line[C_WBAN]);
		location.m_ID = line[C_USAF] + "-" + line[C_WBAN];
		ASSERT(!location.m_ID.empty());

		string name = TrimConst(line[C_STATION_NAME]);
		location.m_name = UppercaseFirstLetter(name);


		if (!line[C_LAT].empty() && !line[C_LON].empty() &&
			line[C_LAT] != "+00.000" && line[C_LON] != "+000.000")
		{
			location.m_lat = ToDouble(line[C_LAT]);
			location.m_lon = ToDouble(line[C_LON]);
			ASSERT(location.m_lat >= -90 && location.m_lat < 90);
			ASSERT(location.m_lon >= -180 && location.m_lon < 180);
		}

		if (!line[C_ELEV].empty() && line[C_ELEV] != "-0999.9" && line[C_ELEV] != "-0999.0")
			location.m_alt = ToDouble(line[C_ELEV]);
		else
			location.m_alt = -999;


		string country = CCountrySelectionGADM::NOAA_to_GADM(TrimConst(line[C_CTRY]));
		string subDivisions = TrimConst(line[C_STATE]);
		if (country == "CAN"&&subDivisions == "NF")
			subDivisions = "NL";

		size_t c_ID = CCountrySelectionGADM::GetCountry(country);//by ID
		if (c_ID != NOT_INIT)
			country = CCountrySelectionGADM::m_default_list[c_ID][0];

		location.SetSSI("Country", country);
		location.SetSSI("SubDivision", subDivisions);


		if (!line[C_BEGIN].empty() && !line[C_END].empty())
		{
			CTPeriod period;
			period.Begin() = CTRef(ToInt(line[C_BEGIN].substr(0, 4)), ToInt(line[C_BEGIN].substr(4, 2)) - 1, ToInt(line[C_BEGIN].substr(6, 2)) - 1);
			period.End() = CTRef(ToInt(line[C_END].substr(0, 4)), ToInt(line[C_END].substr(4, 2)) - 1, ToInt(line[C_END].substr(6, 2)) - 1);

			location.SetSSI("Period", period.GetFormatedString("%1|%2", "%Y-%m-%d"));
		}




		return location;
	}


	double CUIISDLite::GetCountrySubDivision(CShapeFileBase& shapefile, double lat, double lon, std::string countryI, std::string subDivisionI, std::string& countryII, std::string& subDivisionII)const
	{
		double d = -1;
		countryII = "--";
		subDivisionII.clear();


		const CDBF3& DBF = shapefile.GetDBF();

		int FindexH = DBF.GetFieldIndex("HASC_1");
		ASSERT(FindexH >= 0 && FindexH < DBF.GetNbField());
		int FindexGID = DBF.GetFieldIndex("GID_0");
		ASSERT(FindexGID >= 0 && FindexGID < DBF.GetNbField());

		CGeoPoint pt(lon, lat, PRJ_WGS_84);
		int shapeNo = -1;

		bool bInShape = shapefile.IsInside(pt, &shapeNo);
		if (bInShape)//inside a shape
		{
			countryII = TrimConst(DBF[shapeNo][FindexGID].GetElement());
			StringVector tmp(DBF[shapeNo][FindexH].GetElement(), ".");

			if (tmp.size() >= 2)
				subDivisionII = TrimConst(tmp[1]);

			d = 0;
		}
		else
		{
			d = shapefile.GetMinimumDistance(pt, &shapeNo);
			ASSERT(shapeNo >= 0 && shapeNo < DBF.GetNbRecord());

			countryII = TrimConst(DBF[shapeNo][FindexGID].GetElement());

			StringVector tmp(DBF[shapeNo][FindexH].GetElement(), ".");

			if (tmp.size() >= 2)
				subDivisionII = TrimConst(tmp[1]);

		}


		if (countryI.length() == 3)
		{
			if (countryII != countryI ||
				(!subDivisionI.empty() && subDivisionII != subDivisionI))
			{

				double min_d = 1e20;
				for (int i = 0; i < DBF.GetNbRecord(); i++)
				{

					StringVector tmp(DBF[i][FindexH].GetElement(), ".");
					string country = TrimConst(DBF[i][FindexGID].GetElement());

					string subDivision;
					if (tmp.size() >= 2)
						subDivision = TrimConst(tmp[1]);

					if (countryI == country &&
						(subDivisionI.empty() || subDivisionI == subDivision))
					{
						double dd = shapefile[i].GetShape().GetMinimumDistance(pt);
						if (dd < min_d)
						{
							d = dd;
							min_d = dd;
							countryII = country;
							subDivisionII = subDivision;
							/*if (min_d < 20000)
							{
								countryII = countryI;
								subDivisionII = subDivisionI;
							}*/
						}
					}
				}
			}
		}

		if (countryII.length() < 2)
			countryII = "--";

		if (subDivisionII.length() != 2)
			subDivisionII.clear();


		return d;
	}

	ERMsg CUIISDLite::GetFileList(CFileInfoVector& fileList, CCallback& callback)const
	{
		ERMsg msg;


		fileList.clear();
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;

		callback.PushTask(GetString(IDS_LOAD_FILE_LIST), nbYears);

		vector<bool> toDo(nbYears + 1, true);

		int nbRun = 0;

		CFileInfoVector dirList;
		while (nbRun < 20 && toDo[nbYears] && msg)
		{
			nbRun++;

			//open a connection on the server
			CInternetSessionPtr pSession;
			CFtpConnectionPtr pConnection;

			ERMsg msgTmp = GetFtpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", true, 5, callback);
			if (msgTmp)
			{


				if (toDo[0])
				{
					callback.PushTask("Load years list", NOT_INIT);
					msgTmp = FindDirectories(pConnection, string(SERVER_PATH), dirList);
					callback.PopTask();
					if (msgTmp)
						toDo[0] = false;
				}

				if (msgTmp)
				{
					for (int y = 0; y < dirList.size() && msg&&msgTmp; y++)
					{
						const CFileInfo& info = dirList[y];
						string path = info.m_filePath;

						int year = ToInt(GetLastDirName(path));
						if (year >= firstYear && year <= lastYear)
						{
							int index = year - firstYear + 1;
							if (toDo[index])
							{
								msgTmp = FindFiles(pConnection, string(info.m_filePath) + "*.gz", fileList, false, callback);
								if (msgTmp)
								{
									toDo[index] = false;
									msg += callback.StepIt();

									nbRun = 0;
								}
							}
						}//
					}//for all dir
				}

				pConnection->Close();
				pSession->Close();


				if (!msgTmp)
					callback.AddMessage(msgTmp);
			}
			else
			{
				if (nbRun > 1 && nbRun < 20)
				{
					msg += WaitServer(10, callback);
				}
			}
		}


		callback.PopTask();

		//remove unwanted file
		if (msg)
		{
			if (toDo[nbYears])
				callback.AddMessage(GetString(IDS_SERVER_BUSY));

			callback.AddMessage(GetString(IDS_NB_FILES_FOUND) + ToString(fileList.size()));
			msg = CleanList(fileList, callback);
		}

		return msg;
	}


	bool CUIISDLite::IsFileInclude(const string& fileTitle)const
	{
		bool bRep = false;

		string ID = fileTitle.substr(0, 12);
		if (m_stations.find(ID) != m_stations.end())
		{
			CCountrySelectionGADM countries(Get(COUNTRIES));
			CGeoRect boundingBox;

			CLocation location = m_stations.at(ID);
			size_t country = CCountrySelectionGADM::GetCountry(location.GetSSI("Country"));


			if (country != -1 && (countries.none() || countries.test(country)))
			{
				if (boundingBox.IsRectEmpty() || boundingBox.PtInRect(location))
				{
					if (IsEqualNoCase(location.GetSSI("Country"), "USA"))
					{
						CStateSelection states(Get(STATES));
						string state = location.GetSSI("SubDivision");
						if (states.at(state))
							bRep = true;
					}
					else if (IsEqualNoCase(location.GetSSI("Country"), "CAN"))
					{
						CProvinceSelection provinces(Get(PROVINCES));
						if (provinces.at(location.GetSSI("SubDivision")))
							bRep = true;
					}
					else
					{
						bRep = true;
					}
				}
			}
		}

		return bRep;
	}

	ERMsg CUIISDLite::CleanList(StringVector& fileList, CCallback& callback)const
	{
		ERMsg msg;

		callback.PushTask(GetString(IDS_CLEAN_LIST), fileList.size());


		for (StringVector::const_iterator it = fileList.begin(); it != fileList.end() && msg;)
		{
			string fileTitle = GetFileTitle(*it);

			if (IsFileInclude(fileTitle))
				it++;
			else
				it = fileList.erase(it);


			msg += callback.StepIt();
		}

		callback.PopTask();
		return msg;
	}

	ERMsg CUIISDLite::CleanList(CFileInfoVector& fileList, CCallback& callback)const
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);

		callback.PushTask(GetString(IDS_CLEAN_LIST), fileList.size());

		for (CFileInfoVector::const_iterator it = fileList.begin(); it != fileList.end() && msg;)
		{
			string fileTitle = GetFileTitle(it->m_filePath);
			string stationID = fileTitle.substr(0, 12);
			int year = ToInt(Right(fileTitle, 4));
			string outputFilePath = GetOutputFilePath(stationID, year);


			//mesure temporaire
			string outputFilePathTmp = GetOutputFilePath(stationID, year, "");
			if (FileExists(outputFilePath) && FileExists(outputFilePathTmp))
				RemoveFile(outputFilePath);

			if (!IsFileInclude(fileTitle) || IsFileUpToDate(*it, outputFilePath, false))
				it = fileList.erase(it);
			else
				it++;


			msg += callback.StepIt();
		}

		callback.AddMessage(GetString(IDS_NB_FILES_CLEARED) + ToString(fileList.size()));
		callback.AddMessage("");

		callback.PopTask();

		return msg;
	}


	ERMsg CUIISDLite::Execute(CCallback& callback)
	{
		ERMsg msg;


		string workingDir = GetDir(WORKING_DIR);
		msg = CreateMultipleDir(workingDir);


		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(string(SERVER_NAME) + "/" + SERVER_PATH, 1);
		callback.AddMessage("");

		//load station list
		CFileInfoVector fileList;
		msg = UpdateStationHistory(callback);

		if (msg)
			msg = m_stations.LoadFromCSV(GetHistoryFilePath(true));


		if (msg)
			msg = GetFileList(fileList, callback);

		if (!msg)
			return msg;

		callback.PushTask(GetString(IDS_UPDATE_FILE) + " ISD-Lite (" + ToString(fileList.size()) + " files)", fileList.size());

		int nbRun = 0;
		int curI = 0;

		while (curI < fileList.size() && nbRun < 20 && msg)
		{
			nbRun++;

			CInternetSessionPtr pSession;
			CFtpConnectionPtr pConnection;

			ERMsg msgTmp = GetFtpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "anonymous", "test@hotmail.com", true, 5, callback);
			if (msgTmp)
			{

				try
				{
					for (int i = curI; i < fileList.size() && msgTmp && msg; i++)
					{
						string fileTitle = GetFileTitle(fileList[i].m_filePath);

						string stationID = fileTitle.substr(0, 12);
						int year = ToInt(Right(fileTitle, 4));

						string zipFilePath = GetOutputFilePath(stationID, year, ".gz");
						string extractedFilePath = GetOutputFilePath(stationID, year, "");
						string outputFilePath = GetOutputFilePath(stationID, year);
						string outputPath = GetPath(outputFilePath);
						CreateMultipleDir(outputPath);
						msgTmp += CopyFile(pConnection, fileList[i].m_filePath, zipFilePath, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE);

						//unzip 
						if (msgTmp)
						{
							ASSERT(FileExists(outputFilePath));
							nbRun = 0;
							curI++;

							msg += callback.StepIt();
						}
					}
				}
				catch (CException* e)
				{
					msgTmp = UtilWin::SYGetMessage(*e);
				}


				//clean connection
				pConnection->Close();
				pSession->Close();

				if (!msgTmp)
				{
					callback.AddMessage(FormatMsg(IDS_UPDATE_END, ToString(curI), ToString(fileList.size())));
				}
			}
			else
			{
				if (nbRun > 1 && nbRun < 20)
				{
					msg += WaitServer(10, callback);
				}
			}
		}

		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(curI));
		callback.PopTask();

		return msg;
	}


	ERMsg CUIISDLite::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;


		stationList.clear();
		string workingDir = GetDir(WORKING_DIR);


		msg = m_stations.LoadFromCSV(GetHistoryFilePath(true));
		if (!msg)
			return msg;

		//get all file in the directory
		StringVector fileList;
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;

		callback.PushTask(GetString(IDS_GET_STATION_LIST), nbYears);



		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			string strSearch = workingDir + ToString(year) + "\\*.gz";

			callback.PushTask(strSearch, NOT_INIT);
			StringVector fileListTmp = GetFilesList(strSearch);
			fileList.insert(fileList.begin(), fileListTmp.begin(), fileListTmp.end());
			callback.PopTask();

			msg += callback.StepIt();
		}

		callback.PopTask();

		if (msg)
		{
			//remove file 
			msg = CleanList(fileList, callback);

			for (size_t i = 0; i < fileList.size(); i++)
			{
				string fileTitle = GetFileTitle(fileList[i]);
				string stationID = fileTitle.substr(0, 12);
				if (std::find(stationList.begin(), stationList.end(), stationID) == stationList.end())
					stationList.push_back(stationID);
			}
		}


		return msg;
	}

	//
	//Format Documentation
	//Field 1: Pos 1-4, Length 4: Observation Year
	//Field 2: Pos 6-7, Length 2: Observation Month
	//Field 3: Pos 9-11, Length 2: Observation Day
	//Field 4: Pos 12-13, Length 2: Observation Hour
	//Field 5: Pos 14-19, Length 6: Air Temperature
	//Field 6: Pos 20-24, Length 6: Dew Point Temperature
	//Field 7: Pos 26-31, Length 6: Sea Level Pressure
	//Field 8: Pos 32-37, Length 6: Wind Direction
	//Field 9: Pos 38-43, Length 6: Wind Speed Rate
	//Field 10: Pos 44-49, Length 6: Sky Condition Total Coverage Code
	//Field 11: Pos 50-55, Length 6: Liquid Precipitation Depth Dimension - One Hour Duration
	//Field 12: Pos 56-61, Length 6: Liquid Precipitation Depth Dimension - Six Hour Duration


	//Sky Condition Total Coverage Code
	//0: None, SKC or CLR
	//1: One okta - 1/10 or less but not zero
	//2: Two oktas - 2/10 - 3/10, or FEW
	//3: Three oktas - 4/10
	//4: Four oktas - 5/10, or SCT
	//5: Five oktas - 6/10
	//6: Six oktas - 7/10 - 8/10
	//7: Seven oktas - 9/10 or more but not 10/10, or BKN
	//8: Eight oktas - 10/10, or OVC
	//9: Sky obscured, or cloud amount cannot be estimated
	//10: Partial obscuration
	//11: Thin scattered
	//12: Scattered
	//13: Dark scattered
	//14: Thin broken
	//15: Broken
	//16: Dark broken
	//17: Thin overcast
	//18: Overcast
	//19: Dark overcast





	ERMsg CUIISDLite::GetWeatherStation(const string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ASSERT(m_stations.find(ID) != m_stations.end());


		ERMsg msg;

		((CLocation&)station) = m_stations.at(ID);
		station.m_name = PurgeFileName(station.m_name);

		
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;
		station.CreateYears(firstYear, nbYears);

		//now extact data
		CWeatherAccumulator accumulator(TM);


		for (size_t y = 0; y < nbYears + 1 && msg; y++)//+1 to get the first hours of the first day of the next year
		{
			int year = firstYear + int(y);

			string filePath = GetOutputFilePath(ID, year);
			if (FileExists(filePath))
			{
				ERMsg msgTmp = ReadData(filePath, station, accumulator, callback);

				if (msg && !msgTmp)
				{
					msg = msgTmp;
				}
			}//if file exist
		}

		if (accumulator.GetTRef().IsInit())
		{
			CTPeriod period(CTRef(firstYear, 0, 0, 0, TM), CTRef(lastYear, LAST_MONTH, LAST_DAY, LAST_HOUR, TM));
			if (period.IsInside(accumulator.GetTRef()))
				station[accumulator.GetTRef()].SetData(accumulator);
		}

		ASSERT(station.GetEntireTPeriod().GetFirstYear() >= firstYear && station.GetEntireTPeriod().GetLastYear() <= lastYear);



		//string country = station.GetSSI("Country");
		//string subDivisions = station.GetSSI("SubDivision");
		//station.m_siteSpeceficInformation.clear();
		station.SetSSI("Provider", "NOAA");
		station.SetSSI("Network", "ISDLite");
		//station.SetSSI("Country", country);
		//station.SetSSI("SubDivision", subDivisions);

		if (msg && station.HaveData())
			msg = station.IsValid();

		return msg;
	}


	ERMsg CUIISDLite::ReadData(const string& filePath, CWeatherStation& station, CWeatherAccumulator& accumulator, CCallback& callback)const
	{
		ERMsg msg;

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);


		ifStream  file;

		msg = file.open(filePath, ios_base::in | ios_base::binary);
		if (msg)
		{
			try
			{
				boost::iostreams::filtering_istreambuf in;
				in.push(boost::iostreams::gzip_decompressor());
				in.push(file);
				std::istream incoming(&in);

				CTPeriod period(CTRef(firstYear, 0, 0, 0), CTRef(lastYear, LAST_MONTH, LAST_DAY, LAST_HOUR));
				array<float, CUIISDLite::NB_ISD_FIELD> e;


				string line;
				while (std::getline(incoming, line) && msg)
				{
					if (LoadFields(line, e))
					{
						CTRef UTCTRef = GetTRef(e);
						CTRef TRef = CTimeZones::UTCTRef2LocalTRef(UTCTRef, station);

						if (period.IsInside(TRef))
						{
							if (accumulator.TRefIsChanging(TRef))
								station[accumulator.GetTRef()].SetData(accumulator);

							if (e[ISD_T] > -9999)
								accumulator.Add(TRef, H_TAIR, e[ISD_T] / 10.0);
							if (e[ISD_P] > -9999)
								accumulator.Add(TRef, H_PRES, e[ISD_P] / 10.0);//in hPa
							if (e[ISD_PRCP1] > -9999 && int(e[ISD_PRCP1]) != -1)
								accumulator.Add(TRef, H_PRCP, e[ISD_PRCP1] / 10.0);//NOTE: there are a lot of problem before 2006...
							if (e[ISD_TDEW] > -9999)
								accumulator.Add(TRef, H_TDEW, e[ISD_TDEW] / 10.0);
							if (e[ISD_T] > -9999 && e[ISD_TDEW] > -9999)
								accumulator.Add(TRef, H_RELH, Td2Hr(e[ISD_T] / 10.0, e[ISD_TDEW] / 10.0));
							if (e[ISD_WSPD] > -9999)
								accumulator.Add(TRef, H_WNDS, (e[ISD_WSPD] / 10.0)*(3600 / 1000));//convert m/s --> km/h
							if (e[ISD_WDIR] > -9999)
								accumulator.Add(TRef, H_WNDD, e[ISD_WDIR]);
						}
					}//not empty

					msg += callback.StepIt(0);
				}//while
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
		}//if open


		return msg;
	}

	CTRef CUIISDLite::GetTRef(const FieldArray& e)
	{
		int year = int(e[ISD_YEAR]);
		size_t m = size_t(e[ISD_MONTH]) - 1;
		size_t d = size_t(e[ISD_DAY]) - 1;
		size_t h = size_t(e[ISD_HOUR]);

		ASSERT(m >= 0 && m < 12);
		ASSERT(d >= 0 && d < GetNbDayPerMonth(year, m));
		ASSERT(h >= 0 && h < 24);


		return CTRef(year, m, d, h);
	}


	bool CUIISDLite::LoadFields(const string& line, FieldArray& e)
	{
		static const int SIZE[CUIISDLite::NB_ISD_FIELD] = { 4, 3, 3, 3, 6, 6, 6, 6, 6, 6, 6, 6 };

		e.fill(-9999);
		return sscanf(line.c_str(), "%f %f %f %f %f %f %f %f %f %f %f %f", &(e[0]), &(e[1]), &(e[2]), &(e[3]), &(e[4]), &(e[5]), &(e[6]), &(e[7]), &(e[8]), &(e[9]), &(e[10]), &(e[11])) == CUIISDLite::NB_ISD_FIELD;
	}

}