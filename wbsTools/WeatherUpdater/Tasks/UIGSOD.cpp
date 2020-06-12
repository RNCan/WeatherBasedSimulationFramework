#include "StdAfx.h"
#include "UIGSOD.h"

#include <boost\dynamic_bitset.hpp>
//#include <boost\filesystem.hpp>
//#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/gzip.hpp>



#include "Basic/DailyDatabase.h"
#include "Basic/FileStamp.h"
#include "UI/Common/SYShowMessage.h"
#include "TaskFactory.h"
#include "../Resource.h"

#include "CountrySelection.h"
#include "StateSelection.h"


using namespace std; 
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;
using namespace boost;


namespace WBSF
{



	const char* CUIGSOD::SERVER_NAME = "ftp.ncdc.noaa.gov";
	const char* CUIGSOD::SERVER_PATH = "pub/data/gsod/";
	const char* CUIGSOD::LIST_PATH = "pub/data/noaa/";



	//*********************************************************************
	const char* CUIGSOD::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "FirstYear", "LastYear", "Countries", "States", "ShowProgress" };
	const size_t CUIGSOD::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_STRING, T_STRING, T_STRING_SELECT, T_STRING_SELECT, T_BOOL };
	const UINT CUIGSOD::ATTRIBUTE_TITLE_ID = IDS_UPDATER_NOAA_GSOD_P;
	const UINT CUIGSOD::DESCRIPTION_TITLE_ID = ID_TASK_NOAA_GSOD;

	const char* CUIGSOD::CLASS_NAME(){ static const char* THE_CLASS_NAME = "GSOD";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIGSOD::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIGSOD::CLASS_NAME(), (createF)CUIGSOD::create);



	CUIGSOD::CUIGSOD(void)
	{}

	CUIGSOD::~CUIGSOD(void)
	{}



	std::string CUIGSOD::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case COUNTRIES:	str = CCountrySelection::GetAllPossibleValue(); break;
		case STATES:	str = CStateSelection::GetAllPossibleValue(); break;
		};
		return str;
	}

	std::string CUIGSOD::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "GSOD\\"; break;
		case FIRST_YEAR:
		case LAST_YEAR:	str = ToString(CTRef::GetCurrentTRef().GetYear()); break;
		};

		return str;
	}

	//****************************************************


	ERMsg CUIGSOD::UpdateStationHistory(CCallback& callback)
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

				string workingDir = GetDir(WORKING_DIR);
				string outputFilePath = workingDir + GetFileName(fileList[0].m_filePath);
				
				if (!IsFileUpToDate(fileList[0], outputFilePath))
					msg = CopyFile(pConnection, fileList[0].m_filePath, outputFilePath);
			}

			pConnection->Close();
			pSession->Close();
		}

		return msg;
	}

	ERMsg CUIGSOD::GetFileList(CFileInfoVector& fileList, CCallback& callback)const
	{
		ERMsg msg;


		fileList.clear();

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;

		callback.PushTask(GetString(IDS_LOAD_FILE_LIST), nbYears);
//		callback.SetNbStep(nbYears);



		dynamic_bitset<size_t> toDo(nbYears + 1);
		toDo.set();
		//toDo.InsertAt(0, true, nbYears+1);


		size_t nbRun = 0;

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
				pSession->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 55000);
				pSession->SetOption(INTERNET_OPTION_DATA_RECEIVE_TIMEOUT, 55000);
				

				if (toDo[0])
				{
					callback.PushTask(GetString(IDS_LOAD_FILE_LIST), NOT_INIT);
					msgTmp = FindDirectories(pConnection, SERVER_PATH, dirList);
					callback.PopTask();

					if (msgTmp)
						toDo[0] = false;
				}

				if (msgTmp)
				{
					for (size_t y = 0; y < dirList.size() && msg&&msgTmp; y++)
					{
						msg += callback.StepIt(0);

						const CFileInfo& info = dirList[y];
						string path = info.m_filePath;
						int year = ToInt(path.substr(14, 4));
						if (year >= firstYear && year <= lastYear)
						{
							int index = year - firstYear + 1;
							if (toDo[index])
							{
								//msgTmp = FindFiles(pConnection, string(info.m_filePath) + "*.op.gz", fileList, callback);
								msgTmp = FindFiles(pConnection, string(info.m_filePath) + "gsod_" + ToString(year) + ".tar", fileList, false, callback);
								if (msgTmp)
								{
									toDo[index] = false;
									msg += callback.StepIt();

									nbRun = 0;
								}
							}
						}
					}
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

			callback.AddMessage(GetString(IDS_NB_FILES_FOUND) + ToString(fileList.size()), 1);

			msg = CleanList(fileList, callback);
		}

		return msg;
	}


	ERMsg CUIGSOD::FTPDownload(const string& server, const string& inputFilePath, const string& outputFilePath, CCallback& callback)
	{
		ERMsg msg;

		callback.PushTask("FTPTransfer...", NOT_INIT);

		string command = "\"" + GetApplicationPath() + "External\\FTPTransfer.exe\" -Server \"" + server + "\" -Remote \"" + inputFilePath + "\" -Local \"" + outputFilePath + "\" -Passive -Download";

		UINT show = APP_VISIBLE && as<bool>(SHOW_PROGRESS) ? SW_SHOW : SW_HIDE;

		DWORD exitCode = 0;
		msg = WinExecWait(command, GetTempPath(), show, &exitCode);
		if (msg && exitCode != 0)
			msg.ajoute("FTPTransfer as exit with error code " + ToString(int(exitCode)));

		callback.PopTask();

		return msg;
	}
	
	ERMsg CUIGSOD::Uncompress(const string& filePathZip, const string& outputPath, CCallback& callback)
	{
		ERMsg msg;

		callback.PushTask(GetString(IDS_UNZIP_FILE), NOT_INIT);

		
	
		DWORD exitCode = 0;
		string command = GetApplicationPath() + "External\\7za.exe e \"" + filePathZip + "\" -y -o\"" + outputPath + "\"";
		msg = WinExecWait(command, outputPath, SW_HIDE, &exitCode);

		if (msg && exitCode != 0)
			msg.ajoute("7za.exe as exit with error code " + ToString(int(exitCode)));


		callback.PopTask();


		return msg;
	}

	ERMsg CUIGSOD::Execute(CCallback& callback)
	{
		ERMsg msg;

		//problème : certain nom dans le fichier history n'est pas le même que dans le non de fichier
		//		callback.SetNbStep(3);

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
			msg = UpdateOptimisationStationFile(GetDir(WORKING_DIR), callback);

		if (msg)
			msg = LoadOptimisation();

		if (msg)
			msg = GetFileList(fileList, callback);

		if (!msg)
			return msg;


		callback.PushTask(GetString(IDS_UPDATE_FILE) + " GSOD (" + ToString(fileList.size() ) + " files)", fileList.size());

		size_t curI = 0;

		for (size_t i = 0; i < fileList.size() && msg; i++)
		{
			msg += callback.StepIt(0);

			string outputFilePath = GetOutputFilePath(fileList[i]);

			string outputPath = GetPath(outputFilePath);
			CreateMultipleDir(outputPath);
			msg = FTPDownload(SERVER_NAME, fileList[i].m_filePath, outputFilePath, callback);

			//unzip 
			if (msg)
			{
				ASSERT(FileExists(outputFilePath));
				msg = Uncompress(outputFilePath, outputPath, callback);

				msg += callback.StepIt();
				curI++;
			}
		}

		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(curI), 1);
		callback.PopTask();

		return msg;
	}


	string CUIGSOD::GetOutputFilePath(const string& stationName, int year)const
	{
		return GetDir(WORKING_DIR) + ToString(year) + "\\" + stationName + "-" + ToString(year) + ".op.gz";
	}

	string CUIGSOD::GetOutputFilePath(const CFileInfo& info)const
	{
		//int pos = int(strlen(SERVER_PATH));
		string title = GetFileTitle(info.m_filePath);

		return GetDir(WORKING_DIR) + title.substr(title.length() - 4) + "\\" + GetFileName(info.m_filePath);
	}


	bool CUIGSOD::IsFileInclude(const string& fileTitle)const
	{
		bool bRep = false;

		if (StationExist(fileTitle))
		{
			//PAS VRAIMENT OPTIMISER
			CCountrySelection countries(Get(COUNTRIES));
			CStateSelection states(Get(STATES));
			CGeoRect boundingBox;

			CGSODStation station;
			GetStationInformation(fileTitle, station);
			station.GetFromSSI();

			size_t country = CCountrySelection::GetCountry(station.m_country.c_str());

			if (country == NOT_INIT || countries.none() || countries[country])
			{
				if (boundingBox.IsRectEmpty() || boundingBox.PtInRect(station))
				{
					if (IsEqual(station.m_country, "US"))
					{
						//size_t state = CStateSelection::GetState(station.m_state);
						if (states.at(station.m_state))
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

	ERMsg CUIGSOD::CleanList(StringVector& fileList, CCallback& callback)const
	{
		ERMsg msg;

		callback.PushTask(GetString(IDS_CLEAN_LIST), fileList.size());
		//callback.SetNbStep(fileList.size());


		for (StringVector::iterator it = fileList.begin(); it != fileList.end() && msg;)
		{
			string fileTitle = GetFileTitle(*it);

			if (!IsFileInclude(fileTitle))
				it = fileList.erase(it);
			else
				it++;

			msg += callback.StepIt();
		}

		callback.PopTask();

		return msg;
	}

	ERMsg CUIGSOD::CleanList(CFileInfoVector& fileList, CCallback& callback)const
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);

		callback.PushTask(GetString(IDS_CLEAN_LIST), fileList.size());
		//callback.SetNbStep(fileList.size());
		

		for (CFileInfoVector::const_iterator it = fileList.begin(); it != fileList.end() && msg;)
		{
			string fileTitle = GetFileTitle(it->m_filePath);
			string outputFilePath = GetOutputFilePath(*it);

			//!IsFileInclude(fileTitle) ||
			if ( IsFileUpToDate(*it, outputFilePath, false))
				it = fileList.erase(it);
			else
				it++;

			msg += callback.StepIt();
		}

		//CStdioFile file("d:\\travail\\anomalie.txt", CFile::modeWrite|CFile::modeCreate);
		//for( int i=0; i<missStationArray.size(); i++)
		//{
		//	file.WriteString(missStationArray[i]);
		//	file.WriteString("\n");
		//}
		//file.Close();

		callback.AddMessage(GetString(IDS_NB_FILES_CLEARED) + ToString(fileList.size()), 1);
		callback.AddMessage("");

		callback.PopTask();

		return msg;
	}

	ERMsg CUIGSOD::LoadOptimisation()
	{
		//load station list in memory for optimization
		ERMsg msg;
		string filePath = GetHistoryFilePath();

		msg = m_optFile.Load(GetOptFilePath(filePath));
		return msg;
	}


	string CUIGSOD::GetOptFilePath(const string& filePath)const
	{
		string optFilePath = filePath;
		SetFileExtension(optFilePath, ".GSODopt");

		return optFilePath;
	}


	bool CUIGSOD::StationExist(const string& fileTitle)const
	{
		string ID = fileTitle.substr(0, 12);
		return m_optFile.KeyExists(ID);
	}

	void CUIGSOD::GetStationInformation(const string& fileTitle, CLocation& station)const
	{
		ASSERT(StationExist(fileTitle));

		
		string ID = fileTitle.substr(0, 12);
		if (m_optFile.KeyExists(ID))
	
			station = m_optFile.at(ID);
	}

	ERMsg CUIGSOD::UpdateOptimisationStationFile(const string& workingDir, CCallback& callback)const
	{
		ERMsg msg;

		string refFilePath = GetHistoryFilePath();
		string optFilePath = GetOptFilePath(refFilePath.c_str());

		if (CGSODStationOptimisation::NeedUpdate(refFilePath, optFilePath))
		{
			CGSODStationOptimisation optFile;
			msg = optFile.Update(refFilePath, callback);
			if (msg)
				msg = optFile.Save(optFilePath);
		}


		return msg;
	}

	ERMsg CUIGSOD::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);

		msg = UpdateOptimisationStationFile(workingDir, callback);
		if (!msg)
			return msg;


		msg = LoadOptimisation();
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

			string strSearch = workingDir + ToString(year) + "\\*.op.gz";

			StringVector fileListTmp = GetFilesList(strSearch);
			fileList.insert(fileList.begin(), fileListTmp.begin(), fileListTmp.end());

			msg += callback.StepIt();
		}

		if (msg)
		{
			//remove file 
			msg = CleanList(fileList, callback);

			for (int i = 0; i < fileList.size(); i++)
			{
				string fileTitle = GetFileTitle(fileList[i]);
				string stationID = fileTitle.substr(0, 12);
				if (std::find(stationList.begin(), stationList.end(), stationID) == stationList.end())
					stationList.push_back(stationID);
			}
		}

		callback.PopTask();
		return msg;
	}

	ERMsg CUIGSOD::GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		//Get station information
		GetStationInformation(stationName, station);
		station.m_name = PurgeFileName(station.m_name);

		vector<int> stationIDList;
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYear = lastYear - firstYear + 1;
		station.CreateYears(firstYear, nbYear);

		//now extract data 
		for (size_t y = 0; y < nbYear&&msg; y++)
		{
			int year = firstYear + int(y);

			string filePath = GetOutputFilePath(stationName, year);
			if (FileExists(filePath))
				msg = ReadData(filePath, station[year]);

			msg += callback.StepIt(0);
		}

		//verify station is valid
		if (msg && station.HaveData())
		{
			//verify station is valid
			msg = station.IsValid();
		}

		return msg;
	}

	ERMsg CUIGSOD::ReadData(const string& filePath, CYear& data)const
	{
		ERMsg msg;

		ifStream file;
		msg = file.open(filePath, ios_base::in | ios_base::binary);
		if (msg)
		{
			boost::iostreams::filtering_istreambuf in;
			in.push(boost::iostreams::gzip_decompressor());
			in.push(file);
			std::istream incoming(&in);

			string line;

			std::getline(incoming, line);
			while (std::getline(incoming, line) && msg)
			{
				ASSERT(line.length() == 138);
				
				int year = ToInt(line.substr(14, 4));
				int month = ToInt(line.substr(18, 2)) - 1;
				int day = ToInt(line.substr(20, 2)) - 1;


				ASSERT(month >= 0 && month < 12);
				ASSERT(day >= 0 && day < GetNbDayPerMonth(year, month));

				CTRef TRef(year, month, day);
				//int d = GetJDay(day, month, year)-1;

				double Tmin = -999;
				double Tmean = -999;
				double Tmax = -999;
				double ppt = -999;
				double Tdew = -999;
				double windSpeed = -999;
				double snowDept = -999;
				double press = -999;

				if (line.substr(24, 6) != "9999.9")
					Tmean = ToDouble(line.substr(24, 6));

				if (line.substr(110, 6) != "9999.9")
					Tmin = ToDouble(line.substr(110, 6));

				if (line.substr(102, 6) != "9999.9")
					Tmax = ToDouble(line.substr(102, 6));

				if (line.substr(57, 6) != "9999.9")
					press = ToDouble(line.substr(57, 6));

				// A = 1 report of 6-hour precipitation 
				//     amount.
				// B = Summation of 2 reports of 6-hour 
				//     precipitation amount.
				// C = Summation of 3 reports of 6-hour 
				//     precipitation amount.
				// D = Summation of 4 reports of 6-hour 
				//     precipitation amount.
				// E = 1 report of 12-hour precipitation
				//     amount.
				// F = Summation of 2 reports of 12-hour
				//     precipitation amount.
				// G = 1 report of 24-hour precipitation
				//     amount.
				// H = Station reported '0' as the amount
				//     for the day (eg, from 6-hour reports),
				//     but also reported at least one
				//     occurrence of precipitation in hourly
				//     observations--this could indicate a
				//     trace occurred, but should be considered
				//     as incomplete data for the day.
				// I = Station did not report any precip data
				//     for the day and did not report any
				//     occurrences of precipitation in its hourly
				//     observations--it's still possible that
				//     precip occurred but was not reported.


				char pptFlag = line[123];
				if (pptFlag != 'I' && pptFlag != 'H' &&line.substr(118, 5) != "99.99")
					ppt = ToDouble(line.substr(118, 5));

				if (line.substr(35, 6) != "9999.9")
					Tdew = ToDouble(line.substr(35, 6));

				if (line.substr(78, 5) != "999.9")
					windSpeed = ToDouble(line.substr(78, 5));

				if (line.substr(125, 5) != "999.9")
					snowDept = ToDouble(line.substr(125, 5));


				if (Tmean > -76 && Tmean <  122)
				{
					double TmeanC = ((Tmean - 32.0)*5.0 / 9.0);
					data[TRef][H_TAIR] = TmeanC;
				}

				if (Tmin > -76 && Tmin < 122 &&
					Tmax > -76 && Tmax < 122)
				{
					assert(Tmin<Tmax);
					if (Tmin > Tmax)
						Switch(Tmin, Tmax);

					double TminC = ((Tmin - 32.0)*5.0 / 9.0);
					double TmaxC = ((Tmax - 32.0)*5.0 / 9.0);
					data[TRef][H_TMIN] = TminC;
					data[TRef][H_TMAX] = TmaxC;
				}

				if (ppt >= 0 && ppt<12)
				{
					data[TRef][H_PRCP] = (ppt*25.40);
				}


				if (Tdew > -76 && Tdew < 122)
				{
					data[TRef][H_TDEW] = ((Tdew - 32.0)*5.0 / 9.0);

					//here relative humidity is compute from DewPoint and Tmean (important to take Tmean)
					if (Tmean != -999)
						data[TRef][H_RELH] = Td2Hr((Tmean - 32.0)*5.0 / 9.0, (Tdew - 32.0)*5.0 / 9.0);//grossiere aproximation?????
				}


				if (windSpeed > 0 && windSpeed < (120.0 / 1.852184256))//does we acceot zero wind speed?
				{
					//knot
					//1 Knot = 1 Nautical Mile per hour
					//1 Nautical mile = 6076.12 ft. = 1852.184256 (m) = 1.852184256 (km)
					data[TRef][H_WNDS] = (windSpeed*1.852184256);
				}

				if (press > 800 && press < 1100)
				{
					data[TRef][H_PRES] = (press);
				}

				if (snowDept >= 0)
				{
					data[TRef][H_SNDH] = snowDept*2.540;//inch --> cm
				}
			}
		}

		return msg;
	}

}