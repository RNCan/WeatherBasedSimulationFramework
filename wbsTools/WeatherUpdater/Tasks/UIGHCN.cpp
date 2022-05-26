#include "StdAfx.h"

#include "UIGHCN.h"
#include <boost\filesystem.hpp>
#include "Basic/DailyDatabase.h"
#include "Basic/FileStamp.h"
#include "Basic/CSV.h"
#include "Basic/ExtractLocationInfo.h"
#include "Basic/Registry.h"
#include "UI/Common/SYShowMessage.h"
#include "Geomatic/ShapeFileBase.h"
#include "TaskFactory.h"
#include "../Resource.h"

#include "CountrySelection.h"
#include "StateSelection.h"

#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/gzip.hpp>


using namespace WBSF::HOURLY_DATA;
using namespace std;
using namespace UtilWWW;

//HTTPS
//https://www.ncei.noaa.gov/data/global-historical-climatology-network-daily/access/

namespace WBSF
{


	using namespace boost;


	const int CUIGHCND::GHCN_VARIABLES[NB_VARIABLES] = { TMIN, TMAX, PRCP, AWND, AWDR, WESF, SNWD, WESD, TAVG, WDF1 };


	//********************************************************************************************
	const char* CUIGHCND::ELEM_CODE[NB_ELEMENT] =
	{
		"ASMM", "ASSS", "AWND", "AWDR", "CLDG", "DPNT", "DPTP", "DYSW", "DYVC",
		"F2MN", "F5SC", "FMTM", "FRGB", "FRGT", "FRTH", "FSIN", "FSMI",
		"FSMN", "GAHT", "HTDG", "MNRH", "MNTP", "MXRH", "PGTM", "PKGS",
		"PRCP", "PRES", "PSUN", "RDIR", "RWND", "SAMM", "SASS", "SCMM",
		"SCSS", "SGMM", "SGSS", "SLVP", "SMMM", "SMSS", "SNOW", "SNWD",
		"WESF", "WESD",
		"STMM", "STSS", "TAVG", "THIC", "TMAX", "TMIN", "TMPW", "TSUN", "WTEQ",
		"EVAP", "MNPN", "MXPN", "TOBS", "WDMV", "WDF1"
	};

	//MNPN = Daily minimum temperature of water in an evaporation pan (tenths of degrees C)
	//MXPN = Daily maximum temperature of water in an evaporation pan (tenths of degrees C)

	size_t CUIGHCND::GetElementType(const char* str)
	{
		//if (var == TMIN || var == TMAX || var == PRCP || var == AWND || var == WESF || var == SNWD || var == WESD)
		size_t pos = NOT_INIT;
		for (int i = 0; i < NB_ELEMENT&&pos == NOT_INIT; i++)
			if (IsEqualNoCase(str, ELEM_CODE[i]))
				pos = i;

		return pos;
	}


	//Global Climate Observing System (GCOS) Surface Network (GSN)
	const char* CUIGHCND::SERVER_NAME = "ftp.ncdc.noaa.gov";
	const char* CUIGHCND::SERVER_PATH = "pub/data/ghcn/daily/";

	std::string CUIGHCND::GetStationFilePath(size_t type)const
	{
		string file_path;
		switch (type)
		{
		case DISTANCE_TXT: file_path = std::string(SERVER_PATH) + "ghcnd-stations.txt"; break;
		case LOCAL_TXT:file_path = GetDir(WORKING_DIR) + "ghcnd-stations.txt"; break;
		case LOCAL_CSV:file_path = GetDir(WORKING_DIR) + "ghcnd-stations.csv"; break;
		default: ASSERT(false);
		}

		return file_path;
	}

	//*********************************************************************
	const char* CUIGHCND::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "FirstYear", "LastYear", "Countries", "States", "ShowProgress" };
	const size_t CUIGHCND::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_STRING, T_STRING, T_STRING_SELECT, T_STRING_SELECT, T_BOOL };
	const UINT CUIGHCND::ATTRIBUTE_TITLE_ID = IDS_UPDATER_NOAA_GHCND_P;
	const UINT CUIGHCND::DESCRIPTION_TITLE_ID = ID_TASK_NOAA_GHCND;

	const char* CUIGHCND::CLASS_NAME() { static const char* THE_CLASS_NAME = "GHCND";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIGHCND::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIGHCND::CLASS_NAME(), (createF)CUIGHCND::create);



	CUIGHCND::CUIGHCND(void)
	{}

	CUIGHCND::~CUIGHCND(void)
	{}


	std::string CUIGHCND::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case COUNTRIES:	str = CCountrySelectionGADM::GetAllPossibleValue(); break;
		case STATES:	str = CStateSelection::GetAllPossibleValue(); break;
		};
		return str;
	}

	std::string CUIGHCND::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "GHCN\\"; break;
		case FIRST_YEAR:
		case LAST_YEAR:	str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
		};

		return str;
	}

	//****************************************************


	ERMsg CUIGHCND::UpdateStationList(CCallback& callback)
	{
		ERMsg msg;



		CInternetSessionPtr pSession;
		CFtpConnectionPtr pConnection;

		msg = GetFtpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", true, 5, callback);
		if (msg)
		{

			string path = GetStationFilePath(DISTANCE_TXT);

			CFileInfoVector fileList;
			msg = FindFiles(pConnection, path, fileList, false, callback);

			pConnection->Close();
			pSession->Close();

			if (msg)
			{
				ASSERT(fileList.size() == 1);


				string txt_filepath = GetStationFilePath(LOCAL_TXT);
				if (!IsFileUpToDate(fileList[0], txt_filepath))
				{
					msg = FTPDownload(SERVER_NAME, path, txt_filepath, callback);
					if (msg)
					{
						string csv_filepath = GetStationFilePath(LOCAL_CSV);

						msg = convert_txt_to_csv(txt_filepath, csv_filepath, callback);
					}
				}
			}
		}

		return msg;
	}

	ERMsg CUIGHCND::convert_txt_to_csv(const std::string& txtFilePath, const std::string& csvFilePath, CCallback& callback)
	{
		ERMsg msg;

		string path = GetPath(txtFilePath);
		std::string correction_file_path = path + "ghcnd-corrections.csv";
		CLocationVector correction;
		correction.Load(correction_file_path);


		CLocationVector old_locations;
		old_locations.Load(csvFilePath);

		std::string invalid_coord_file_path = path + "ghcnd-invalid-stations.csv";
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
		msg += file.open(txtFilePath);
		if (msg)
		{

			if (file.length() > 1000)
			{
				callback.PushTask("Extract country and administrative sub-division (GHCND)", file.length());
				invalid << "ID,Name,Latitude,Longitude,Elevation,Country1,SubDivision1,Country2,SubDivision2,Distance(km),Comment" << endl;


				CLocationVector locations;
				locations.reserve(120000);

				string line;
				while (msg && std::getline(file, line))
				{
					CLocation location = LocationFromLine(line);
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
									bool bBuoy = location.m_name.find("Buoy") != string::npos || location.m_name.find("Platform") != string::npos;

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
							string country = location.GetSSI("Country");
							string subDivision = location.GetSSI("SubDivision");
							invalid << location.m_ID << "," << location.m_name << "," << ToString(location.m_lat, 4) << "," << ToString(location.m_lon, 4) << "," << to_string(location.m_alt) + "," << country << "," << subDivision << "," << "," << "," << "," << "BadLocation" << endl;
						}
					}

					msg += callback.StepIt(line.length() + 1);
				}//for all lines

				if(bGADM)
					shapefile.Close();

				invalid.close();
				file.close();


				callback.AddMessage("Number of stations: " + to_string(locations.size()));
				callback.PopTask();

				if (msg)
				{
					//extraxt missing elevations
					ASSERT(locations.IsValid(true));
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

					msg += locations.Save(csvFilePath);

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
				callback.AddMessage("Removing " + txtFilePath);
				msg += WBSF::RemoveFile(txtFilePath);
			}

		}

		return msg;
	}


	double CUIGHCND::GetCountrySubDivision(CShapeFileBase& shapefile, double lat, double lon, std::string countryI, std::string subDivisionI, std::string& countryII, std::string& subDivisionII)
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
							/*if (min_d < d)
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

	CLocation CUIGHCND::LocationFromLine(std::string line)
	{
		CLocation location;

		Trim(line);
		if (!line.empty())
		{
			string tmp;

			string country = CCountrySelectionGADM::NOAA_to_GADM(TrimConst(line.substr(0, 2)));
			string subDivisions = TrimConst(line.substr(38, 2));
			if (country == "CAN"&&subDivisions == "NF")
				subDivisions = "NL";

			size_t c_ID = CCountrySelectionGADM::GetCountry(country);//by ID
			if (c_ID != NOT_INIT)
				country = CCountrySelectionGADM::m_default_list[c_ID][0];


			location.SetSSI("Country", country);
			location.SetSSI("SubDivision", subDivisions);

			location.m_ID = Trim(line.substr(0, 11));
			location.m_name = WBSF::PurgeFileName(UppercaseFirstLetter(Trim(line.substr(41, 29))));




			tmp = Trim(line.substr(12, 8));
			if (!tmp.empty())
			{
				double lat = ToDouble(tmp);
				if (lat > -90 && lat < 90)
					location.m_lat = lat;
			}

			tmp = Trim(line.substr(21, 9));
			if (!tmp.empty())
			{
				double lon = ToDouble(tmp);
				if (lon > -360 && lon < 360)
					location.m_lon = lon;
			}

			tmp = Trim(line.substr(31, 6));
			if (!tmp.empty())
			{
				double elev = ToDouble(tmp);
				if (elev > -99 && country != "BRA")//most of brazil elevation are at zero
					location.m_elev = elev;
				else
					location.m_elev = -999;
			}

			//if (location.m_lat != -999 && location.m_lon != -999)
				//bRep = true;
		}


		return location;
	}



	ERMsg CUIGHCND::GetFileList(CFileInfoVector& fileList, CCallback& callback)const
	{
		ERMsg msg;

		fileList.clear();

		callback.PushTask(GetString(IDS_LOAD_FILE_LIST), 1);


		//open a connection on the server
		CInternetSessionPtr pSession;
		CFtpConnectionPtr pConnection;

		msg = GetFtpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", true, 5, callback);
		if (msg)
		{
			pSession->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 45000);
			string filter = string(SERVER_PATH) + "by_year/*.gz";
			msg = FindFiles(pConnection, filter, fileList, false, callback);

			pConnection->Close();
			pSession->Close();
		}

		callback.PopTask();


		//remove unwanted file
		if (msg)
		{
			callback.AddMessage(GetString(IDS_NB_FILES_FOUND) + ToString(fileList.size()), 1);
			msg = CleanList(fileList, callback);
		}

		return msg;
	}

	ERMsg CUIGHCND::FTPDownload(const string& server, const string& inputFilePath, const string& outputFilePath, CCallback& callback)
	{
		ERMsg msg;


		callback.PushTask("GHCND FTP Transfer", NOT_INIT);

		string workingDir = GetPath(outputFilePath);
		string scriptFilePath = workingDir + m_name + "_script.txt";
		WBSF::RemoveFile(scriptFilePath + ".log");


		ofStream stript;
		msg = stript.open(scriptFilePath);
		if (msg)
		{
			stript << "open ftp:anonymous:public@" << server << endl;

			stript << "cd \"" << GetPath(inputFilePath) << "\"" << endl;
			stript << "lcd \"" << GetPath(outputFilePath) << "\"" << endl;
			stript << "get" << " \"" << GetFileName(outputFilePath) << "\"" << endl;
			stript << "exit" << endl;
			stript.close();

			UINT show = APP_VISIBLE && as<bool>(SHOW_PROGRESS) ? SW_SHOW : SW_HIDE;
			bool bShow = as<bool>(SHOW_PROGRESS);
			string command = "\"" + GetApplicationPath() + "External\\WinSCP.exe\" " + string(bShow ? "/console " : "") + " /passive=on" + " /log=\"" + scriptFilePath + ".log\" /ini=nul /script=\"" + scriptFilePath + "\"";

			DWORD exitCode = 0;
			msg = WinExecWait(command.c_str(), GetApplicationPath().c_str(), show, &exitCode);
			if (msg && exitCode != 0)
			{
				msg.ajoute("WinSCP as exit with error code " + ToString((int)exitCode));
				msg.ajoute("See log file: " + scriptFilePath + ".log");
			}

		}

		callback.PopTask();
		return msg;
	}


	ERMsg CUIGHCND::Execute(CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		CreateMultipleDir(workingDir);

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(string(SERVER_NAME) + "/" + SERVER_PATH, 1);
		callback.AddMessage("");


		msg = UpdateStationList(callback);
		if (!msg)
			return msg;

		CFileInfoVector fileList;
		msg = GetFileList(fileList, callback);

		if (!msg)
			return msg;

		callback.AddMessage(GetString(IDS_NUMBER_FILES) + ToString(fileList.size()), 1);
		callback.AddMessage("");

		callback.PushTask(GetString(IDS_UPDATE_FILE) + " GHCND (" + ToString(fileList.size()) + " files)", fileList.size());

		int nbRun = 0;
		size_t curI = 0;

		while (curI < fileList.size() && nbRun < 3 && msg)
		{
			nbRun++;

			//ERMsg msgTmp;
			for (size_t i = curI; i < fileList.size() && msg; i++)
			{
				msg += callback.StepIt(0);
				if (msg)
				{
					//string outputFilePath = GetOutputFilePath(fileList[i]) + ".gz";
					string fileName = GetFileName(fileList[i].m_filePath);
					string zipFilePath = GetOutputFilePath(fileName);

					CreateMultipleDir(GetPath(zipFilePath));
					callback.AddMessage("Download " + fileName + " ...");

					msg = FTPDownload(SERVER_NAME, fileList[i].m_filePath, zipFilePath.c_str(), callback);
					if (msg)
					{
						curI++;
						msg += callback.StepIt();
					}
				}
			}
		}

		callback.AddMessage(FormatMsg(IDS_UPDATE_END, ToString(curI), ToString(fileList.size())));
		callback.PopTask();

		return msg;
	}

	int CUIGHCND::GetYear(const string& fileName)
	{
		return ToInt(fileName.substr(0, 4));
	}

	std::string CUIGHCND::GetOutputFilePath(int year)const
	{
		return GetDir(WORKING_DIR) + "by_year\\" + ToString(year) + ".csv.gz";
	}

	string CUIGHCND::GetOutputFilePath(const string& fileName)const
	{
		return GetDir(WORKING_DIR) + "by_year\\" + fileName;
	}

	bool CUIGHCND::IsStationInclude(const string& ID)const
	{
		bool bRep = false;

		CLocationMap::const_iterator it = m_stations.find(ID);
		if (it != m_stations.end())
		{
			const CLocation& location = it->second;
			
			CCountrySelectionGADM countries(Get(COUNTRIES));
			size_t country = CCountrySelectionGADM::GetCountry(location.GetSSI("Country"));
				
			if (country != -1 && (countries.none() || countries.test(country)))
			{
				if (IsEqualNoCase(location.GetSSI("Country"), "USA"))
				{
					CStateSelection states(Get(STATES));
					string state = location.GetSSI("SubDivision");
					if (states.at(state))
						bRep = true;
				}
				else
				{
					bRep = true;
				}//use this state if USA
			}//use this country
			
		}//station exist

		return bRep;
	}
	ERMsg CUIGHCND::CleanList(CFileInfoVector& fileList, CCallback& callback)const
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);

		callback.PushTask(GetString(IDS_CLEAN_LIST), fileList.size());

		for (CFileInfoVector::const_iterator it = fileList.begin(); it != fileList.end() && msg; )
		{
			int year = GetYear(GetFileName(it->m_filePath));
			string outputFilePath = GetOutputFilePath(year);

			if (year<firstYear || year>lastYear || IsFileUpToDate(*it, outputFilePath, false))
				it = fileList.erase(it);
			else
				it++;


			msg += callback.StepIt();
		}

		callback.PopTask();

		return msg;
	}


	ERMsg CUIGHCND::PreProcess(CCallback& callback)
	{
		ERMsg msg;

		m_loadedData.clear();
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);


		string path = GetDir(WORKING_DIR);
		string workingDir = GetDir(WORKING_DIR);

		msg = m_stations.LoadFromCSV(GetStationFilePath(LOCAL_CSV));
		if (msg)
		{
			size_t nbYears = lastYear - firstYear + 1;
			callback.PushTask("Load year in memory (nb years = " + to_string(nbYears) + ")", nbYears);

			for (size_t y = 0; y < nbYears&&msg; y++)
			{
				int year = firstYear + int(y);
				string filePath = GetOutputFilePath(year);
				msg = LoadData(filePath, m_loadedData, callback);
				msg += callback.StepIt();
			}

			callback.PopTask();
		}
		//}

		return msg;
	}

	ERMsg CUIGHCND::GetStationList(StringVector& list, CCallback& callback)
	{
		ERMsg msg;

		msg = PreProcess(callback);
		if (!msg)
			return msg;

		list.clear();

		for (SimpleDataMap::const_iterator it = m_loadedData.begin(); it != m_loadedData.end(); it++)
		{
			list.push_back(it->first);
		}


		return msg;
	}

	ERMsg CUIGHCND::Finalize(TType type, CCallback& callback)
	{
		m_loadedData.clear();

		return ERMsg();
	}


	static std::string TraitFileName(std::string name)
	{
		size_t begin = name.find('(');
		size_t end = name.find(')');
		if (begin != std::string::npos && end != std::string::npos)
			name.erase(begin, end);

		ReplaceString(name, "&amp;", "&");
		std::replace(name.begin(), name.end(), ',', '-');
		std::replace(name.begin(), name.end(), '.', '·');

		//std::replace(name.begin(), name.end(), ',', '-');

		Trim(name);

		return UppercaseFirstLetter(name);
	}

	ERMsg CUIGHCND::GetWeatherStation(const std::string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		ASSERT(m_stations.find(ID) != m_stations.end());
		//GetStationInformation(ID, station);

		((CLocation&)station) = m_stations.at(ID);
		station.m_name = PurgeFileName(station.m_name);

		const SimpleDataYears& data = m_loadedData.at(ID);

		for (SimpleDataYears::const_iterator it = data.begin(); it != data.end() && msg; it++)
		{
			const SimpleDataYear& dataYear = it->second;
			for (size_t jd = 0; jd < dataYear.size() && msg; jd++)
			{
				CJDayRef TRef(it->first, jd);

				CDay& day = (CDay&)station[TRef];

				if (dataYear[jd][V_TAVG] > -999)
					day.SetStat(H_TAIR, (double)dataYear[jd][V_TAVG]);

				if (dataYear[jd][V_TMIN] > -999 && dataYear[jd][V_TMAX] > -999)
				{
					double Tmin = (double)dataYear[jd][V_TMIN];
					double Tmax = (double)dataYear[jd][V_TMAX];
					assert(Tmin <= Tmax);
					if (Tmin > Tmax)
						Switch(Tmin, Tmax);

					day.SetStat(H_TMIN, Tmin);
					day.SetStat(H_TMAX, Tmax);
				}

				if (dataYear[jd][V_TMIN] > -999 && dataYear[jd][V_TMAX] > -999 &&
					dataYear[jd][V_MNPN] > -999 && dataYear[jd][V_MXPN] > -999)
				{

					//from : http://www.fao.org/docrep/X0490E/x0490e07.htm
					double Tmin_d = (double)dataYear[jd][V_TMIN];
					double Tmax_d = (double)dataYear[jd][V_TMAX];
					double Tmin_w = (double)dataYear[jd][V_MNPN];
					double Tmax_w = (double)dataYear[jd][V_MXPN];


					double RHmax = WBSF::Twb2Hr(Tmin_d, Tmin_w);
					double RHmin = WBSF::Twb2Hr(Tmax_d, Tmax_w);

					double Pv = (eᵒ(Tmin_d)*RHmax / 100 + eᵒ(Tmax_d)*RHmin / 100) / 2; //[kPa]
					//double Es = (e°(Tmin_d) + e°(Tmax_d)) / 2;
					double Td = Pv2Td(Pv);

					day.SetStat(H_TDEW, Td);
					day.SetStat(H_RELH, (RHmin + RHmax) / 2);
				}

				if (dataYear[jd][V_PRCP] > -999)
					day.SetStat(H_PRCP, dataYear[jd][V_PRCP]);

				if (dataYear[jd][V_AWND] > -999)
					day.SetStat(H_WNDS, dataYear[jd][V_AWND]);

				if (dataYear[jd][V_AWDR] > -999)
					day.SetStat(H_WNDD, dataYear[jd][V_AWDR]);
				else if (dataYear[jd][V_WDF1] > -999)//estimation of wind direction from the 1 minute wind direction
					day.SetStat(H_WNDD, dataYear[jd][V_WDF1]);

			}
		}

		station.SetSSI("Provider", "NOAA");
		station.SetSSI("Network", "GHCND");

		if (msg && station.HaveData())
		{
			//verify station is valid
			msg = station.IsValid();
		}

		return msg;
	}

	//PRCP = Precipitation (tenths of mm)
	//SNOW = Snowfall (mm)
	//SNWD = Snow depth (mm)
	//TMAX = Maximum temperature (tenths of degrees C)
	//TMIN = Minimum temperature (tenths of degrees C)
	//	   
	//The other elements are:
	//	   
	//ACMC = Average cloudiness midnight to midnight from 30-second ceilometer data (percent)
	//ACMH = Average cloudiness midnight to midnight from manual observations (percent)
	//ACSC = Average cloudiness sunrise to sunset from 30-second ceilometer data (percent)
	//ACSH = Average cloudiness sunrise to sunset from manual observations (percent)
	//AWND = Average daily wind speed (tenths of meters per second)
	//DAEV = Number of days included in the multiday evaporation total (MDEV)
	//DAPR = Number of days included in the multiday precipiation total (MDPR)
	//DASF = Number of days included in the multiday snowfall total (MDSF)		  
	//DATN = Number of days included in the multiday minimum temperature (MDTN)
	//DATX = Number of days included in the multiday maximum temperature (MDTX)
	//DAWM = Number of days included in the multiday wind movement (MDWM)
	//DWPR = Number of days with non-zero precipitation included in multiday precipitation total (MDPR)
	//EVAP = Evaporation of water from evaporation pan (tenths of mm) 
	//FMTM = Time of fastest mile or fastest 1-minute wind (hours and minutes, i.e., HHMM)
	//FRGB = Base of frozen ground layer (cm)
	//FRGT = Top of frozen ground layer (cm)
	//FRTH = Thickness of frozen ground layer (cm)
	//GAHT = Difference between river and gauge height (cm)
	//MDEV = Multiday evaporation total (tenths of mm; use with DAEV)
	//MDPR = Multiday precipitation total (tenths of mm; use with DAPR and DWPR, if available)
	//MDSF = Multiday snowfall total 
	//MDTN = Multiday minimum temperature (tenths of degrees C; use with DATN)
	//MDTX = Multiday maximum temperature (tenths of degress C; use with DATX)
	//MDWM = Multiday wind movement (km)
	//MNPN = Daily minimum temperature of water in an evaporation pan (tenths of degrees C)
	//MXPN = Daily maximum temperature of water in an evaporation pan (tenths of degrees C)
	//PGTM = Peak gust time (hours and minutes, i.e., HHMM)
	//PSUN = Daily percent of possible sunshine (percent)
	//SN*# = Minimum soil temperature (tenths of degrees C) where * corresponds to a code for ground cover and # corresponds to a code for soil depth.  
	//	Ground cover codes include the following:
	//		0 = unknown
	//		1 = grass
	//		2 = fallow
	//		3 = bare ground
	//		4 = brome grass
	//		5 = sod
	//		6 = straw multch
	//		7 = grass muck
	//		8 = bare muck
	//	Depth codes include the following:
	//		1 = 5 cm
	//		2 = 10 cm
	//		3 = 20 cm
	//		4 = 50 cm
	//		5 = 100 cm
	//		6 = 150 cm
	//		7 = 180 cm
	//SX*# = Maximum soil temperature (tenths of degrees C) where * corresponds to a code for ground cover and # corresponds to a code for soil depth. See SN*# for ground cover and depth codes. 
	//THIC = Thickness of ice on water (tenths of mm)	
	//TAVG = Average temperature(tenths of degrees C) [Note that TAVG from source 'S' corresponds to an average for the period ending at 2400 UTC rather than local midnight]
	//TOBS = Temperature at the time of observation (tenths of degrees C)
	//TSUN = Daily total sunshine (minutes)
	//WDF1 = Direction of fastest 1-minute wind (degrees)
	//WDF2 = Direction of fastest 2-minute wind (degrees)
	//WDF5 = Direction of fastest 5-second wind (degrees)
	//WDFG = Direction of peak wind gust (degrees)
	//WDFI = Direction of highest instantaneous wind (degrees)
	//WDFM = Fastest mile wind direction (degrees)
	//WDMV = 24-hour wind movement (km)	   
	//WESD = Water equivalent of snow on the ground (tenths of mm)
	//WESF = Water equivalent of snowfall (tenths of mm)
	//WSF1 = Fastest 1-minute wind speed (tenths of meters per second)
	//WSF2 = Fastest 2-minute wind speed (tenths of meters per second)
	//WSF5 = Fastest 5-second wind speed (tenths of meters per second)
	//WSFG = Peak guest wind speed (tenths of meters per second)
	//WSFI = Highest instantaneous wind speed (tenths of meters per second)
	//WSFM = Fastest mile wind speed (tenths of meters per second)
	//WT** = Weather Type where ** has one of the following values:
	//	01 = Fog, ice fog, or freezing fog (may include heavy fog)
	//	02 = Heavy fog or heaving freezing fog (not always distinguished from fog)
	//	03 = Thunder
	//	04 = Ice pellets, sleet, snow pellets, or small hail 
	//	05 = Hail (may include small hail)
	//	06 = Glaze or rime 
	//	07 = Dust, volcanic ash, blowing dust, blowing sand, or blowing obstruction
	//	08 = Smoke or haze 
	//	09 = Blowing or drifting snow
	//	10 = Tornado, waterspout, or funnel cloud 
	//	11 = High or damaging winds
	//	12 = Blowing spray
	//	13 = Mist
	//	14 = Drizzle
	//	15 = Freezing drizzle 
	//	16 = Rain (may include freezing rain, drizzle, and freezing drizzle) 
	//	17 = Freezing rain 
	//	18 = Snow, snow pellets, snow grains, or ice crystals
	//	19 = Unknown source of precipitation 
	//	21 = Ground fog 
	//	22 = Ice fog or freezing fog
	//WV** = Weather in the Vicinity where ** has one of the following values:
	//	01 = Fog, ice fog, or freezing fog (may include heavy fog)
	//	03 = Thunder
	//	07 = Ash, dust, sand, or other blowing obstruction
	//	18 = Snow or ice crystals
	//	20 = Rain or snow shower

	//MFLAG1     is the measurement flag.  There are ten possible values:
	//	Blank = no measurement information applicable
	//	B     = precipitation total formed from two 12-hour totals
	//	D     = precipitation total formed from four six-hour totals
	//	H     = represents highest or lowest hourly temperature
	//	K     = converted from knots 
	//	L     = temperature appears to be lagged with respect to reported hour of observation
	//	O     = converted from oktas 
	//	P     = identified as "missing presumed zero" in DSI 3200 and 3206
	//	T     = trace of precipitation, snowfall, or snow depth
	//	W     = converted from 16-point WBAN code (for wind direction)
	//
	//QFLAG1     is the quality flag.  There are fourteen possible values:
	//	Blank = did not fail any quality assurance check
	//	D     = failed duplicate check
	//	G     = failed gap check
	//	I     = failed internal consistency check
	//	K     = failed streak/frequent-value check
	//	L     = failed check on length of multiday period 
	//	M     = failed megaconsistency check
	//	N     = failed naught check
	//	O     = failed climatological outlier check
	//	R     = failed lagged range check
	//	S     = failed spatial consistency check
	//	T     = failed temporal consistency check
	//	W     = temperature too warm for snow
	//	X     = failed bounds check
	//	Z     = flagged as a result of an official Datzilla investigation
	//
	//SFLAG1     is the source flag.  There are twenty eight possible values (including blank, upper and lower case letters):
	//	Blank = No source (i.e., data value missing)
	//	0     = U.S. Cooperative Summary of the Day (NCDC DSI-3200)
	//	6     = CDMP Cooperative Summary of the Day (NCDC DSI-3206)
	//	7     = U.S. Cooperative Summary of the Day -- Transmitted via WxCoder3 (NCDC DSI-3207)
	//	A     = U.S. Automated Surface Observing System (ASOS) real-time data (since January 1, 2006)
	//	a     = Australian data from the Australian Bureau of Meteorology
	//	B     = U.S. ASOS data for October 2000-December 2005 (NCDC DSI-3211)
	//	b     = Belarus update
	//	E     = European Climate Assessment and Dataset (Klein Tank et al., 2002)	   
	//	F     = U.S. Fort data 
	//	G     = Official Global Climate Observing System (GCOS) or other government-supplied data
	//	H     = High Plains Regional Climate Center real-time data
	//	I     = International collection (non U.S. data received through  personal contacts)
	//	K     = U.S. Cooperative Summary of the Day data digitized from paper observer forms (from 2011 to present)
	//	M     = Monthly METAR Extract (additional ASOS data)
	//	N     = Community Collaborative Rain, Hail,and Snow (CoCoRaHS)
	//	Q     = Data from several African countries that had been "quarantined", that is, withheld from public release until permission was granted from the respective meteorological services
	//	R     = NCDC Reference Network Database (Climate Reference Network and Historical Climatology Network-Modernized)
	//	r     = All-Russian Research Institute of Hydrometeorological Information-World Data Center
	//	S     = Global Summary of the Day (NCDC DSI-9618)
	//		NOTE: "S" values are derived from hourly synoptic reports exchanged on the Global Telecommunications System (GTS).
	//		Daily values derived in this fashion may differ significantly from "true" daily data, particularly for precipitation (i.e., use with caution).
	//	s     = China Meteorological Administration/National Meteorological Information Center/Climatic Data Center (http://cdc.cma.gov.cn)
	//	T     = SNOwpack TELemtry (SNOTEL) data obtained from the Western  Regional Climate Center
	//	U     = Remote Automatic Weather Station (RAWS) data obtained from the Western Regional Climate Center	   
	//	u     = Ukraine update	   
	//	W     = WBAN/ASOS Summary of the Day from NCDC's Integrated Surface Data (ISD).  
	//	X     = U.S. First-Order Summary of the Day (NCDC DSI-3210)
	//	Z     = Datzilla official additions or replacements 
	//	z     = Uzbekistan update
	//
	//	When data are available for the same time from more than one source, the highest priority source is chosen according to the following
	//	priority order (from highest to lowest):   Z,R,0,6,X,W,K,7,F,B,M,r,E,z,u,b,a,s,G,Q,I,A,N,T,U,H,S





	ERMsg CUIGHCND::LoadData(const string& filePath, SimpleDataMap& data, CCallback& callback)const
	{
		ASSERT(FileExists(filePath));


		//ID = 11 character station identification code
		//YEAR/MONTH/DAY = 8 character date in YYYYMMDD format (e.g. 19860529 = May 29, 1986)
		//ELEMENT = 4 character indicator of element type 
		//DATA VALUE = 5 character data value for ELEMENT 
		//M-FLAG = 1 character Measurement Flag 
		//Q-FLAG = 1 character Quality Flag 
		//S-FLAG = 1 character Source Flag 
		//OBS-TIME = 4-character time of observation in hour-minute format (i.e. 0700 =7:00 am)

		enum TFields { GHCN_ID, GHCN_DATE, GHCN_ELEMENT, GHCN_DATA, GHCN_M, GHCN_Q, GHCN_S };


		ERMsg msg;

		ifStream file;
		msg = file.open(filePath, ios_base::in | ios_base::binary);

		if (msg)
		{
			file.seekg(0, std::istream::end);
			std::istream::pos_type length = file.tellg();
			file.seekg(0);

			boost::iostreams::filtering_istreambuf in;
			in.push(boost::iostreams::gzip_decompressor());
			in.push(file);
			std::istream incoming(&in);


			callback.PushTask("Load in memory " + GetFileName(filePath), length);
			int year = GetYear(GetFileName(filePath));

			bool bInclude = true;
			string lastID;
			for (CSVIterator loop(incoming); loop != CSVIterator() && msg; ++loop)
			{
				string ID = (*loop)[GHCN_ID];

				if (ID != lastID)
				{
					if (m_included.find(ID) != m_included.end())
					{
						bInclude = true;
					}
					else if (m_rejected.find(ID) != m_rejected.end())
					{
						bInclude = false;
					}
					else
					{
						bInclude = IsStationInclude(ID);
						if (bInclude)
							const_cast<CUIGHCND*>(this)->m_included.insert(ID);
						else
							const_cast<CUIGHCND*>(this)->m_rejected.insert(ID);
					}

					lastID = ID;
				}

				if (bInclude)
				{
					const string& date = (*loop)[GHCN_DATE];
					ASSERT(date.length() == 8);

					int year = ToInt(date.substr(0, 4)); ASSERT(year >= 1800 && year < 2100);
					int month = ToInt(date.substr(4, 2)) - 1; ASSERT(month >= 0 && month < 12);
					int day = ToInt(date.substr(6, 2)) - 1; ASSERT(day >= 0 && day < GetNbDayPerMonth(year, month));
					ASSERT(day >= 0 && day < GetNbDayPerMonth(year, month));

					if (day >= 0 && day < GetNbDayPerMonth(year, month))
					{
						CTRef TRef(year, month, day);

						size_t var = GetElementType((*loop)[GHCN_ELEMENT].c_str());
						bool bGoodVar = false;
						for (size_t i = 0; i < NB_VARIABLES && !bGoodVar; i++)
							bGoodVar = (var == GHCN_VARIABLES[i]);

						if (bGoodVar)
						{
							char Qf2 = (*loop)[GHCN_Q].empty() ? ' ' : ToChar((*loop)[GHCN_Q]);

							string strValue = TrimConst((*loop)[GHCN_DATA]);
							if (!strValue.empty() && Qf2 == ' ')//is valid
							{
								float value = ToFloat((*loop)[GHCN_DATA]);
								switch (var)
								{

								case TMIN:
									ASSERT(value > -999 && value < 999 || value == -9999);
									if (value > -999 && value < 999)
									{
										//10e of °C --> °C
										data[ID][year][TRef.GetJDay()][V_TMIN] = value / 10;
									}
									break;

								case TAVG:
									ASSERT(value > -999 && value < 999 || value == -9999);
									if (value > -999 && value < 999)
									{
										//10e of °C --> °C
										data[ID][year][TRef.GetJDay()][V_TAVG] = value / 10;
									}
									break;


								case TMAX:
									ASSERT(value > -999 && value < 999 || value == -9999);
									if (value > -999 && value < 999)
									{
										//10e of °C --> °C
										data[ID][year][TRef.GetJDay()][V_TMAX] = value / 10;
									}
									break;

								case MNPN://minimum temperature of evaporation pan
									ASSERT(value > -999 && value < 999 || value == -9999);
									if (value > -999 && value < 999)
									{
										//10e of °C --> °C
										data[ID][year][TRef.GetJDay()][V_MNPN] = value / 10;
									}
									break;

								case MXPN://maximum temperature of evaporation pan
									ASSERT(value > -999 && value < 999 || value == -9999);
									if (value > -999 && value < 999)
									{
										//10e of °C --> °C
										data[ID][year][TRef.GetJDay()][V_MXPN] = value / 10;
									}
									break;

								case PRCP:
									ASSERT((int)value >= 0 && value < 9999 || value == -9999);
									if ((int)value >= 0 && value < 9999)
									{
										//10e of mm --> mm
										data[ID][year][TRef.GetJDay()][V_PRCP] = value / 10;
									}
									break;

								case AWND://Wind speed
									ASSERT((int)value >= 0 || value <= -9999 || value == 99999);
									if ((int)value >= 0 && value < 9999)
									{
										//10e of m/s --> km/h
										data[ID][year][TRef.GetJDay()][V_AWND] = (value / 10) * 3600 / 1000;
									}
									break;


								case AWDR://Wind direction
									ASSERT((int)value >= 0 || value <= -9999 || value == 99999);
									if ((int)value >= 0 && value <= 360)
									{
										data[ID][year][TRef.GetJDay()][V_AWND] = value;
									}
									break;
								case WDF1://Wind direction for the fatest 1 minutes (approximation)
									ASSERT((int)value >= 0 || value <= -9999 || value == 99999);
									if ((int)value >= 0 && value <= 360)
									{
										data[ID][year][TRef.GetJDay()][V_WDF1] = value;
									}
									break;

								case WESF: //Water equivalent of snowfall
									ASSERT((int)value >= 0 || value <= -9999 || value == 99999);
									if ((int)value >= 0 && value < 9999)
									{
										//10e of mm --> mm
										data[ID][year][TRef.GetJDay()][V_WESF] = (value / 10);
									}
									break;
								case SNWD://snow depth
									ASSERT((int)value >= 0 || value <= -9999 || value == 99999);
									if ((int)value >= 0 && value < 9999)
									{
										//mm --> cm
										data[ID][year][TRef.GetJDay()][V_SNWD] = (value / 10);
									}
									break;

								case WESD://Water equivalent of snow on the ground
									ASSERT((int)value >= 0 || value <= -9999 || value == 99999);
									if ((int)value >= 0 && value < 9999)
									{
										//10e of mm --> mm
										data[ID][year][TRef.GetJDay()][V_WESD] = (value / 10);
									}
									break;



									//case DPTP:
									//	ASSERT( value > -999 && value < 999 || value==99999 );
									//	//ASSERT( unit == "TF");
									//	if( value > -999 && value < 999 )
									//	{
									//		//test
									//		value = (value/10.0f-32)*5.f/9.f;//10e of Fahrenheit - > °C
									//		data(year)[month][day].SetData(H_TDEW, value);
									//	}
									//	break;
									//case MNRH:
									//	ASSERT( (int)value >=0 && (int)value<=100 || value==99999);
									//	//ASSERT( unit == "P ");
									//	if( (int)value>=0 && (int)value<=100)
									//	{
									//		CStatistic stat = data(year)[month][day].GetData(H_RELH);
									//		stat += value;
									//		data(year)[month][day].SetData(H_RELH, stat);
									//	}
									//	break;
									//case MXRH:
									//	ASSERT( (int)value >=0 && (int)value<=100 || value==99999);
									//	//ASSERT( unit == "P ");
									//	if( (int)value>=0 && (int)value<=100)
									//	{
									//		CStatistic stat = data(year)[month][day].GetData(H_RELH);
									//		stat += value;
									//		data(year)[month][day].SetData(H_RELH, stat);
									//	}
									//	break;
									//case -1:
									//case SNOW:
									//case EVAP:
									//case MNPN:
									//case MXPN:
									//case PGTM:
									//case TOBS:
									//case WDMV:
									//case FMTM: break;
									//default: ASSERT(false);
								}
							}//end if flag
							else
							{
								int i;
								i = 0;
								//return msg;
							}//data is valid
						}//it' a good variable
					}//day is valid
				}//valid stations

				msg += callback.SetCurrentStepPos((size_t)file.tellg());
			}// for all lines

			callback.PopTask();
		}

		return msg;
	}


}

