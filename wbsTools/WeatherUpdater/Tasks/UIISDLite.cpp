//Integrated Surface Data - “Lite”
#include "stdafx.h"
#include "UIISDLite.h"

#include <boost\filesystem.hpp>
#include "Basic/FileStamp.h"
#include "Basic/DailyDatabase.h"
#include "UI/Common/SYShowMessage.h"
#include "TaskFactory.h"
#include "CountrySelection.h"
#include "StateSelection.h"
#include "../Resource.h"



using namespace std; 
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;

namespace WBSF
{

	const char* CUIISDLite::SERVER_NAME = "ftp.ncdc.noaa.gov";
	const char* CUIISDLite::SERVER_PATH = "pub/data/noaa/";
	const char* CUIISDLite::LIST_PATH = "pub/data/noaa/";

	//*********************************************************************
	const char* CUIISDLite::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "FirstYear", "LastYear", "Countries", "States"};
	const size_t CUIISDLite::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_STRING, T_STRING, T_STRING_BROWSE, T_STRING_BROWSE};
	const UINT CUIISDLite::ATTRIBUTE_TITLE_ID = IDS_UPDATER_NOAA_ISD_LITE_P;
	const UINT CUIISDLite::DESCRIPTION_TITLE_ID = ID_TASK_NOAA_ISD_LITE;

	const char* CUIISDLite::CLASS_NAME(){ static const char* THE_CLASS_NAME = "ISD-Lite";  return THE_CLASS_NAME; }
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
		case COUNTRIES:	str = CCountrySelection::GetAllPossibleValue(); break;
		case STATES:	str = CStateSelection::GetAllPossibleValue(); break;
		};
		return str;
	}

	std::string CUIISDLite::Default(size_t i)const
	{
		string str;

		switch (i)
		{
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


	ERMsg CUIISDLite::UpdateStationHistory()
	{
		ERMsg msg;

		CInternetSessionPtr pSession;
		CFtpConnectionPtr pConnection;

		msg = GetFtpConnection(SERVER_NAME, pConnection, pSession);
		if (msg)
		{
			string path = GetHistoryFilePath(false);

			CFileInfoVector fileList;
			msg = FindFiles(pConnection, path, fileList, CCallback::DEFAULT_CALLBACK);
			if (msg)
			{
				ASSERT(fileList.size() == 1);

				string outputFilePath = GetHistoryFilePath(true);
				if (!IsFileUpToDate(fileList[0], outputFilePath))
					msg = CopyFile(pConnection, fileList[0].m_filePath, outputFilePath);
			}

			pConnection->Close();
			pSession->Close();
		}

		return msg;
	}

	ERMsg CUIISDLite::GetFileList(CFileInfoVector& fileList, CCallback& callback)const
	{
		ERMsg msg;


		fileList.clear();
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;

		callback.PushTask(GetString(IDS_LOAD_FILE_LIST), nbYears);
		//callback.SetNbStep(nbYears);

		vector<bool> toDo(nbYears + 1, true);

		int nbRun = 0;

		CFileInfoVector dirList;
		while (nbRun < 20 && toDo[nbYears] && msg)
		{
			nbRun++;

			//open a connection on the server
			CInternetSessionPtr pSession;
			CFtpConnectionPtr pConnection;

			ERMsg msgTmp = GetFtpConnection(SERVER_NAME, pConnection, pSession);
			if (msgTmp)
			{
				pSession->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 15000);

				if (toDo[0])
				{
					msgTmp = FindDirectories(pConnection, string(SERVER_PATH) + "isd-lite/", dirList);
					if (msgTmp)
						toDo[0] = false;
				}

				if (msgTmp)
				{
					for (int y = 0; y < dirList.size() && msg&&msgTmp; y++)
					{
						msg += callback.StepIt(0);

						const CFileInfo& info = dirList[y];
						string path = info.m_filePath;

						int year = ToInt(GetLastDirName(path));
						if (year >= firstYear && year <= lastYear)
						{
							int index = year - firstYear + 1;
							if (toDo[index])
							{
								msgTmp = FindFiles(pConnection, string(info.m_filePath) + "*.gz", fileList, callback);
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
					callback.PushTask("Waiting 30 seconds for server...", 600);
					for (size_t i = 0; i < 600 && msg; i++)
					{
						Sleep(50);//wait 50 milisec
						msg += callback.StepIt();
					}
					callback.PopTask();
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

		if (StationExist(fileTitle))
		{

			CCountrySelection countries(Get(COUNTRIES));
			
			CGeoRect boundingBox;

			CIDSLiteStation station;
			GetStationInformation(fileTitle, station);
			station.GetFromSSI();

			//CTPeriod p(CTRef(1981,0,0), CTRef(2010,0,0));
			//p = p.Intersect(station.m_period);
			//if( p.GetNbYear() >= 25 )
			//{
				size_t country = CCountrySelection::GetCountry(station.m_country.c_str());

				if (country != -1 && (countries.none() || countries.at(country)))
				{
					if (boundingBox.IsRectEmpty() || boundingBox.PtInRect(station))
					{
						if (IsEqualNoCase(station.m_country, "US"))
						{
							CStateSelection states(Get(STATES));
							size_t state = CStateSelection::GetState(station.m_state.c_str());
							if (states.none() || (state < states.size() && states.at(state)))
								bRep = true;
						}
						else
						{
							bRep = true;
						}
					}
				}
			//}
		}

		return bRep;
	}

	ERMsg CUIISDLite::CleanList(StringVector& fileList, CCallback& callback)const
	{
		ERMsg msg;

		callback.PushTask(GetString(IDS_CLEAN_LIST), fileList.size());
//		callback.SetNbStep(fileList.size());


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
		//callback.SetNbStep(fileList.size());

		for (CFileInfoVector::const_iterator it = fileList.begin(); it != fileList.end() && msg;)
		{
			string fileTitle = GetFileTitle(it->m_filePath);
			string stationID = fileTitle.substr(0, 12);
			int year = ToInt(Right(fileTitle, 4));
			string outputFilePath = GetOutputFilePath(stationID, year);

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


	bool CUIISDLite::StationExist(const string& fileTitle)const
	{
		string ID = fileTitle.substr(0, 12);
		return m_optFile.KeyExists(ID);
	}

	void CUIISDLite::GetStationInformation(const string& fileTitle, CLocation& station)const
	{
		ASSERT(StationExist(fileTitle));

		string ID = fileTitle.substr(0, 12);
		station = m_optFile.at(ID);

	}

	ERMsg CUIISDLite::LoadOptimisation()
	{
		//load station list in memory for optimization
		ERMsg msg;
		string filePath = GetHistoryFilePath();

		msg = m_optFile.Load(GetOptFilePath(filePath));
		return msg;
	}


	string CUIISDLite::GetOptFilePath(const string& filePath)const
	{
		string optFilePath = filePath;
		SetFileExtension(optFilePath, ".ISDopt");


		return optFilePath;
	}


	ERMsg CUIISDLite::UpdateOptimisationStationFile(const string& workingDir, CCallback& callback)const
	{
		ERMsg msg;

		string refFilePath = GetHistoryFilePath();
		string optFilePath = GetOptFilePath(refFilePath);


		CIDSLiteStationOptimisation optFile;

		if (CLocationOptimisation::NeedUpdate(refFilePath, optFilePath))
		{
			msg = optFile.Update(refFilePath, callback);
			if (msg)
				msg = optFile.Save(optFilePath);
		}

		return msg;
	}

	ERMsg CUIISDLite::Execute(CCallback& callback)
	{
		ERMsg msg;

		//callback.PushLevel();
		//callback.SetNbStep(3);

		string workingDir = GetDir(WORKING_DIR);
		msg = CreateMultipleDir(workingDir);


		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(string(SERVER_NAME) + "/" + SERVER_PATH + "isd-lite/", 1);
		callback.AddMessage("");

		//load station list
		CFileInfoVector fileList;
		msg = UpdateStationHistory();

		if (msg)
			msg = UpdateOptimisationStationFile(GetDir(WORKING_DIR), callback);

		if (msg)
			msg = LoadOptimisation();

		if (msg)
			msg = GetFileList(fileList, callback);

		if (!msg)
			return msg;

		callback.PushTask(GetString(IDS_UPDATE_FILE), fileList.size());
		//callback.SetNbStep(fileList.size());

		int nbRun = 0;
		int curI = 0;

		while (curI < fileList.size() && nbRun < 20 && msg)
		{
			nbRun++;

			CInternetSessionPtr pSession;
			CFtpConnectionPtr pConnection;

			ERMsg msgTmp = GetFtpConnection(SERVER_NAME, pConnection, pSession);
			if (msgTmp)
			{
				TRY
				{
					for (int i = curI; i < fileList.size() && msgTmp && msg; i++)
					{
						msg += callback.StepIt(0);

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
							

							
							string command = "External\\7z.exe e \"" + zipFilePath + "\" -y -o\"" + outputPath + "\"";
							msg += WinExecWait(command.c_str());
							RemoveFile(zipFilePath);
							//by default, file don't have extension,
							RenameFile(extractedFilePath, outputFilePath);
							
							//update time to the time of the .gz file
							boost::filesystem::path p(outputFilePath);
							if (boost::filesystem::exists(p))
								boost::filesystem::last_write_time(p, fileList[i].m_time);


							ASSERT(FileExists(outputFilePath));
							nbRun = 0;
							curI++;

							msg += callback.StepIt();
						}
					}
				}
				CATCH_ALL(e)
				{
					msgTmp = UtilWin::SYGetMessage(*e);
				}
				END_CATCH_ALL

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
					callback.PushTask("Waiting 30 seconds for server...", 600);
					for (size_t i = 0; i < 600 && msg; i++)
					{
						Sleep(50);//wait 50 milisec
						msg += callback.StepIt();
					}
					callback.PopTask();
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


		string workingDir = GetDir(WORKING_DIR);
		msg = UpdateOptimisationStationFile(workingDir, callback);

		if (msg)
			msg = LoadOptimisation();

		if (!msg)
			return msg;

		//stationList.push_back("072080-99999");
		//return msg;


		//get all file in the directory
		StringVector fileList;
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;

		callback.PushTask(GetString(IDS_GET_STATION_LIST), nbYears);
		//callback.SetNbStep(nbYears);


		for (size_t y = 0; y < nbYears&&msg; y++)
		{
			int year = firstYear + int(y);

			string strSearch = workingDir + ToString(year) + "\\*.isd";

			StringVector fileListTmp = GetFilesList(strSearch);
			fileList.insert(fileList.begin(), fileListTmp.begin(), fileListTmp.end());

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


	StringVector GetElem(string line)
	{
		static const int SIZE[12] = { 4, 3, 3, 3, 6, 6, 6, 6, 6, 6, 6, 6 };

		StringVector elem(12);

		for (int i = 0, pos = 0; i < 12; i++)
		{
			elem[i] = line.substr(pos, SIZE[i]);
			pos += SIZE[i];
		}

		ASSERT(elem.size() == CUIISDLite::NB_ISD_FIELD);

		return elem;
	}



	ERMsg CUIISDLite::GetWeatherStation(const string& stationName, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		GetStationInformation(stationName, station);
		station.m_name = PurgeFileName(station.m_name);


		int timeZone = (int)Round(station.m_lon / 15);
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);
		size_t nbYears = lastYear - firstYear + 1;
		station.CreateYears(firstYear, nbYears);

		//now extact data
		CWeatherAccumulator accumulator(TM);


		for (size_t y = 0; y < nbYears + 1 && msg; y++)//+1 to get the first hours of the first day of the next year
		{
			int year = firstYear + int(y);

			string filePath = GetOutputFilePath(stationName, year);
			if (FileExists(filePath))
			{
				ERMsg msgTmp = ReadData(filePath, timeZone, station, accumulator, callback);

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

		if (msg && station.HaveData())
			msg = station.IsValid();
		
		return msg;
	}


	ERMsg CUIISDLite::ReadData(const string& filePath, int timeZone, CWeatherYears& data, CWeatherAccumulator& accumulator, CCallback& callback)const
	{
		ASSERT(timeZone >= -12 && timeZone <= 12);

		ERMsg msg;

		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);

		ifStream  file;

		msg = file.open(filePath);
		if (msg)
		{
			CTPeriod period(CTRef(firstYear, 0, 0, 0), CTRef(lastYear, LAST_MONTH, LAST_DAY, LAST_HOUR));

			string line;
			while (std::getline(file, line) && msg)
			{
				Trim(line);
				if (!line.empty())
				{
					StringVector elem = GetElem(line);
					CTRef TRef = GetTRef(elem, timeZone);

					if (period.IsInside(TRef))
					{
						if (accumulator.TRefIsChanging(TRef))
							data[accumulator.GetTRef()].SetData(accumulator);

						if (ToInt(elem[ISD_T]) != -9999)
							accumulator.Add(TRef, H_TAIR, ToDouble(elem[ISD_T]) / 10.0);
						if ( ToInt(elem[ISD_P]) != -9999)
							accumulator.Add(TRef, H_PRES, ToDouble(elem[ISD_P]) / 10);//in hPa
						if (ToInt(elem[ISD_PRCP1]) != -9999 && ToInt(elem[ISD_PRCP1]) != -1)
							accumulator.Add(TRef, H_PRCP, ToDouble(elem[ISD_PRCP1]) / 10);//NOTE: there are a lot of problem before 2006...
						if (ToInt(elem[ISD_TDEW]) != -9999)
							accumulator.Add(TRef, H_TDEW, ToDouble(elem[ISD_TDEW]) / 10);
						if (ToInt(elem[ISD_T]) != -9999 && ToInt(elem[ISD_TDEW]) != -9999)
							accumulator.Add(TRef, H_RELH, Td2Hr(ToDouble(elem[ISD_T]) / 10.0, ToDouble(elem[ISD_TDEW]) / 10.0));
						if (ToInt(elem[ISD_WSPD]) != -9999)
							accumulator.Add(TRef, H_WNDS, (ToDouble(elem[ISD_WSPD]) / 10)*(3600 / 1000));//convert m/s --> km/h
						if ( ToInt(elem[ISD_WDIR]) != -9999)
							accumulator.Add(TRef, H_WNDD, ToDouble(elem[ISD_WDIR]));
					}
				}//not empty

				msg += callback.StepIt(0);
			}//while
		}//if open


		return msg;
	}

	CTRef CUIISDLite::GetTRef(const StringVector& elem, int timeZone)const
	{
		int firstYear = as<int>(FIRST_YEAR);
		int lastYear = as<int>(LAST_YEAR);

		int year = ToInt(elem[ISD_YEAR]);
		size_t m = ToInt(elem[ISD_MONTH]) - 1;
		size_t d = ToInt(elem[ISD_DAY]) - 1;
		size_t h = ToInt(elem[ISD_HOUR]);

		ASSERT(year >= firstYear && year <= lastYear + 1);
		ASSERT(m >= 0 && m < 12);
		ASSERT(d >= 0 && d < GetNbDayPerMonth(year, m));
		ASSERT(h >= 0 && h < 24);


		CTRef TRef(year, m, d, h);
		TRef.Shift(timeZone);//convert UTC time to local time

		return TRef;
	}

	//ERMsg CUIISDLite::GetStation(const string& stationName, CDailyStation& station, CCallback& callback)
	//{
	//	return GetWeatherStation(stationName, CTM(CTM::DAILY), station, callback);
	//}

	//string CUIISDLite::GetStationIDFromName(const string& stationName)
	//{
	//	CLocation station;
	//	GetStationInformation(stationName, station);

	//	return station.m_ID.c_str();
	//}
}