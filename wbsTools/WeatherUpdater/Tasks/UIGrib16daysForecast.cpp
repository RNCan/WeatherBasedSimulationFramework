#include "StdAfx.h"
#include "HRDPS.h"
#include "UIGrib16daysForecast.h"
#include "Basic/FileStamp.h"

#include "UI/Common/SYShowMessage.h"
#include "Geomatic/TimeZones.h"
#include "TaskFactory.h"
#include "WeatherBasedSimulationString.h"
#include "../Resource.h"



#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"



using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;


namespace WBSF
{

	//ftp
	//ftp://ftp.ncep.noaa.gov/pub/data/nccf/com/gfs/prod/gfs.20200526/06/
	//ftp://ftpprd.ncep.noaa.gov/pub/data/nccf/com/gfs/prod

	//HTTP
	//https://nomads.ncep.noaa.gov/pub/data/nccf/com/gfs/prod/gfs.20200526/12/



	//*********************************************************************
	const char* CUIGrib16daysForecast::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "Sources", "Server", "FirstHour", "LastHour", "DeleteAfter", "ShowWinSCP" };
	const size_t CUIGrib16daysForecast::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_COMBO_INDEX, T_COMBO_INDEX, T_STRING, T_STRING, T_STRING, T_BOOL };
	const UINT CUIGrib16daysForecast::ATTRIBUTE_TITLE_ID = IDS_UPDATER_LONG_FORECAST_P;
	const UINT CUIGrib16daysForecast::DESCRIPTION_TITLE_ID = ID_TASK_LONG_FORECAST;

	const char* CUIGrib16daysForecast::CLASS_NAME() { static const char* THE_CLASS_NAME = "LongForecast";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIGrib16daysForecast::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIGrib16daysForecast::CLASS_NAME(), (createF)CUIGrib16daysForecast::create);

	const char* CUIGrib16daysForecast::SOURCES_NAME[NB_SOURCES] = { "GFS" };
	const char* CUIGrib16daysForecast::HTTP_SERVER_NAME[NB_SOURCES] = { "nomads.ncep.noaa.gov" };
	const char* CUIGrib16daysForecast::FTP_SERVER_NAME[NB_SOURCES] = { "ftpprd.ncep.noaa.gov" };
	const char* CUIGrib16daysForecast::SERVER_PATH[NB_SOURCES] = { "/pub/data/nccf/com/gfs/prod/" };


	CUIGrib16daysForecast::CUIGrib16daysForecast(void)
	{}

	CUIGrib16daysForecast::~CUIGrib16daysForecast(void)
	{}

	std::string CUIGrib16daysForecast::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case SOURCES:	str = "GFS (World)"; break;
		case SERVER_TYPE: str = "HTTP|FTP"; break;
		};
		return str;
	}

	std::string CUIGrib16daysForecast::Default(size_t i)const
	{
		std::string str;
		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "Forecast\\"; break;
		case SOURCES: str = "0"; break;
		case SERVER_TYPE: str = "0"; break;
		case FIRST_HOUR: str = "1"; break;
		case LAST_HOUR: str = "384"; break;
		case DELETE_AFTER: str = "96"; break;
		};

		return str;
	}



	//************************************************************************************************************
	//Load station definition list section
	ERMsg CUIGrib16daysForecast::Execute(CCallback& callback)
	{
		ERMsg msg;

		size_t serverType = as<size_t>(SERVER_TYPE);
		if (serverType == HTTP_SERVER)
		{
			msg = ExecuteHTTP(callback);
		}
		else
		{
			msg = ExecuteFTP(callback);
		}





		return msg;
	}

	ERMsg CUIGrib16daysForecast::ExecuteHTTP(CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		size_t nbDownload = 0;
		size_t source = as<size_t>(SOURCES);

		if (source == N_GFS)
		{

			CFileInfoVector fileList;
			msg = GetFilesToDownload(source, HTTP_SERVER, fileList, callback);
			CTPeriod p = CleanList(source, fileList);

			callback.PushTask(string("Download ") + SOURCES_NAME[source] + " forecast gribs: " + to_string(fileList.size()) + " files " + p.GetFormatedString("(%1 -- %2)"), fileList.size());
			callback.AddMessage(string("Download ") + SOURCES_NAME[source] + " forecast gribs : " + to_string(fileList.size()) + " files " + p.GetFormatedString("(%1 -- %2)"));

			size_t nbTry = 0;
			size_t curI = 0;

			while (curI < fileList.size() && msg)
			{
				nbTry++;

				CInternetSessionPtr pSession;
				CHttpConnectionPtr pConnection;
				msg = GetHttpConnection(HTTP_SERVER_NAME[source], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);
				if (msg)
				{
					try
					{
						for (size_t i = curI; i < fileList.size() && msg; i++)
						{
							string outputPath = GetLocaleFilePath(source, fileList[i].m_filePath);
							CreateMultipleDir(GetPath(outputPath));

							string file_name = WBSF::GetFileName(fileList[i].m_filePath);
							//CTRef TRef = GetTRef(source, outputPath);
							//string dir_name = FormatA("/gfs.%4d%02d%02d/%02d", TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, TRef.GetHour());
							string dir_name = fileList[i].m_filePath.substr(27,16);

							//msg += CopyFile(pConnection, inputPath, outputPath, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE, true, callback);

							//URL=https://nomads.ncep.noaa.gov/cgi-bin/filter_gfs_sflux.pl?file=gfs.t06z.sfluxgrbf384.grib2&lev_10_m_above_ground=on&lev_2_m_above_ground=on&lev_surface=on&var_DSWRF=on&var_HGT=on&var_PRATE=on&var_PRES=on&var_SNOD=on&var_SPFH=on&var_TMAX=on&var_TMIN=on&var_TMP=on&var_UGRD=on&var_VGRD=on&var_WEASD=on&leftlon=0&rightlon=360&toplat=90&bottomlat=-90&dir=%2Fgfs.20200611%2F06
							string inputPath = "/cgi-bin/filter_gfs_sflux.pl?lev_10_m_above_ground=on&lev_2_m_above_ground=on&lev_surface=on&var_DSWRF=on&var_HGT=on&var_PRATE=on&var_PRES=on&var_SNOD=on&var_SPFH=on&var_TMAX=on&var_TMIN=on&var_TMP=on&var_UGRD=on&var_VGRD=on&var_WEASD=on";
							
							inputPath += "&dir=" + dir_name;
							inputPath += "&file=" + file_name;
							msg += CopyFile(pConnection, inputPath, outputPath, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE, true, callback);

							if (msg)
							{
								if (GoodGrib(outputPath))
								{
									nbDownload++;
									msg += CreateHourlyGeotiff(outputPath, callback);
								}
								else
								{
									//remove file
									msg += RemoveFile(outputPath);
								}

								curI++;
								msg += callback.StepIt();
							}
						}
					}
					catch (CException* e)
					{
						msg = UtilWin::SYGetMessage(*e);
						if (nbTry < 5)
						{
							callback.AddMessage(UtilWin::SYGetMessage(*e));
							msg += WaitServer(30, callback);
						}
						else
						{
							msg = UtilWin::SYGetMessage(*e);
						}
					}

					//clean connection
					pConnection->Close();
					pSession->Close();
				}
			}

			callback.AddMessage(string("Number of ") + SOURCES_NAME[source] + " forecast gribs downloaded: " + ToString(nbDownload));
			callback.PopTask();




			//now, create daily GeoTiff
			//if (msg && m_bCreateDailyGeotiff)
				//msg = CreateDailyGeotiff(fileList, callback);


			//delete old files
			Clean(source, callback);

		}

		return msg;
	}

	ERMsg CUIGrib16daysForecast::ExecuteFTP(CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);

		size_t nbDownload = 0;
		size_t source = as<size_t>(SOURCES);

		if (source == N_GFS)
		{
			string scriptFilePath = workingDir + "script.txt";
			WBSF::RemoveFile(scriptFilePath + ".log");

			CFileInfoVector fileList;
			msg = GetFilesToDownload(source, FTP_SERVER, fileList, callback);
			CTPeriod p = CleanList(source, fileList);

			callback.PushTask(string("Download ") + SOURCES_NAME[source] + " forecast gribs: " + to_string(fileList.size()) + " files " + p.GetFormatedString("(%1 -- %2)"), fileList.size());
			callback.AddMessage(string("Download ") + SOURCES_NAME[source] + " forecast gribs : " + to_string(fileList.size()) + " files " + p.GetFormatedString("(%1 -- %2)"));

			for (size_t i = 0; i < fileList.size() && msg; i++)
			{
				ofStream stript;
				msg = stript.open(scriptFilePath);
				if (msg)
				{
					//CTRef TRef = GetTRef(s, fileList[i].m_filePath);
					string outputFilePath = GetLocaleFilePath(source, fileList[i].m_filePath);
					string tmpFilePaht = GetPath(outputFilePath) + GetFileName(fileList[i].m_filePath);
					CreateMultipleDir(GetPath(outputFilePath));

					stript << "open ftp://anonymous:anonymous%40example.com@" << FTP_SERVER_NAME[source] << endl;

					stript << "cd " << GetPath(fileList[i].m_filePath) << endl;
					stript << "lcd \"" << GetPath(tmpFilePaht) << "\"" << endl;
					stript << "get " << GetFileName(tmpFilePaht) << endl;
					stript << "exit" << endl;
					stript.close();

					bool bShow = as<bool>(SHOW_WINSCP);
					//# Execute the script using a command like:
					string command = "\"" + GetApplicationPath() + "External\\WinSCP.exe\" " + string(bShow ? "/console " : "") + "-timeout=300 -passive=on /log=\"" + scriptFilePath + ".log\" /ini=nul /script=\"" + scriptFilePath;
					DWORD exit_code;
					msg = WBSF::WinExecWait(command, "", SW_SHOW, &exit_code);
					if (msg)
					{
						if (exit_code == 0 && FileExists(tmpFilePaht))
						{
							ifStream stream;
							if (GoodGrib(tmpFilePaht))
							{
								nbDownload++;
								msg = RenameFile(tmpFilePaht, outputFilePath);
							}
							else
							{
								callback.AddMessage("corrupt file, remove: " + tmpFilePaht);
								msg = WBSF::RemoveFile(tmpFilePaht);
							}

						}
						else
						{
							//msg.ajoute("Error in WinCSV");
							callback.AddMessage("Error in WinCSV");
						}
					}

				}

				msg += callback.StepIt();
			}

			callback.AddMessage(string("Number of ") + SOURCES_NAME[source] + " forecast gribs downloaded: " + ToString(nbDownload));
			callback.PopTask();

		}

		//delete old files
		Clean(source, callback);

		return msg;
	}


	ERMsg CUIGrib16daysForecast::Clean(size_t source, CCallback& callback)
	{
		ERMsg msg;

		int delete_after = as<int>(DELETE_AFTER);
		string workingDir = GetDir(WORKING_DIR) + SOURCES_NAME[source] + "\\";

		if (source == N_GFS)
		{
			CTRef nowUTC = CTRef::GetCurrentTRef(CTM::HOURLY, true);

			StringVector filesList;
			StringVector dirList;

			StringVector dates = WBSF::GetDirectoriesList(workingDir + "20??????");//for security, need years
			for (size_t i = 0; i < dates.size(); i++)
			{
				int year = WBSF::as<int>(dates[i].substr(0, 4));
				size_t m = WBSF::as<size_t>(dates[i].substr(4, 2)) - 1;
				size_t d = WBSF::as<size_t>(dates[i].substr(6, 2)) - 1;


				if (year >= 2000 && year <= 2099 && m < 12 && d < 31)
				{
					bool bRemoveDir = true;

					StringVector filesListTmp = GetFilesList(workingDir + dates[i] + "/*.tif");
					for (size_t f = 0; f < filesListTmp.size(); f++)
					{
						CTRef TRefUTC = GetTRef(source, filesListTmp[f]);
						int passHours = nowUTC - TRefUTC;

						if (passHours > delete_after)
						{
							filesList.push_back(filesListTmp[f]);
						}
						else
						{
							bRemoveDir = false;
						}
					}

					if (bRemoveDir)
						dirList.push_back(workingDir + dates[i]);
				}//if valid date
			}//for all dates

			string comment = string("Delete old ") + SOURCES_NAME[source] + " forecast (" + to_string(filesList.size()) + " files)";
			callback.PushTask(comment, filesList.size());
			callback.AddMessage(comment);

			for (size_t i = 0; i != filesList.size() && msg; i++)
			{
				msg += RemoveFile(filesList[i]);
				msg += callback.StepIt();
			}

			//remove directory
			for (size_t i = 0; i != dirList.size() && msg; i++)
			{
				WBSF::RemoveDirectory(dirList[i]);
			}

			callback.PopTask();
		}

		return msg;
	}

	CTPeriod CUIGrib16daysForecast::CleanList(size_t s, CFileInfoVector& fileList1)
	{
		CTPeriod p;
		CTRef nowUTC = CTRef::GetCurrentTRef(CTM::HOURLY, true);

		__int32 first_hours = as<__int32>(FIRST_HOUR);
		__int32 last_hours = as<__int32>(LAST_HOUR);



		std::sort(fileList1.begin(), fileList1.end(), [](const CFileInfo& s1, const CFileInfo& s2) { return std::strcmp(s1.m_filePath.c_str(), s2.m_filePath.c_str()) < 0; });
		//select the latest file for a particular forecast hour
		map<CTRef, CFileInfo>fileList2;
		for (size_t i = 0; i < fileList1.size(); i++)
		{
			string filePath = GetLocaleFilePath(s, fileList1[i].m_filePath);

			size_t HH = GetHH(s, filePath);

			//int hours = TRefUTC - nowUTC;
			if (HH >= first_hours && HH <= last_hours)
			{
				CTRef TRefUTC = GetTRef(s, filePath);
				fileList2[TRefUTC] = fileList1[i];
			}
		}

		fileList1.clear();
		for (auto it = fileList2.begin(); it != fileList2.end(); it++)
		{
			string filePath = GetLocaleFilePath(s, it->second.m_filePath);
			ReplaceString(filePath, ".grib2", ".tif");

			//if (NeedDownload(filePath))
			if (!FileExists(filePath))
			{
				CTRef TRefUTC = GetTRef(s, filePath);
				p += TRefUTC;
				fileList1.push_back(it->second);
			}
		}

		return p;
	}




	ERMsg CUIGrib16daysForecast::GetFilesToDownload(size_t source, size_t server, CFileInfoVector& fileList, CCallback& callback)
	{
		ERMsg msg;

		if (server == HTTP_SERVER)
		{
			callback.PushTask(string("Get files list from: ") + SERVER_PATH[source], 2);

			CInternetSessionPtr pSession;
			CHttpConnectionPtr pConnection;

			msg = GetHttpConnection(HTTP_SERVER_NAME[source], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);
			msg += callback.StepIt(0);
			if (msg)
			{
				CTRef TRef = GetLatestTRef(source, pConnection);
				if (TRef.IsInit())
				{
					string URL = SERVER_PATH[source];
					switch (source)
					{
						//					ftp://ftpprd.ncep.noaa.gov/pub/data/nccf/com/gfs/prod/gfs.20200528/06/gfs.t06z.sfluxgrbf312.grib2
					case N_GFS:URL += FormatA("gfs.%4d%02d%02d/%02d/gfs.t%02dz.sfluxgrbf???.grib2", TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, TRef.GetHour(), TRef.GetHour()); break;
					default: ASSERT(false);
					}

					msg = FindFiles(pConnection, URL, fileList);
				}
			}

			pConnection->Close();
			pSession->Close();

			callback.PopTask();
		}
		else
		{
			callback.PushTask(string("Get files list from: ") + SERVER_PATH[source], 2);

			CInternetSessionPtr pSession;
			CFtpConnectionPtr pConnection;

			msg = GetFtpConnection(FTP_SERVER_NAME[source], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", true, 5, callback);
			msg += callback.StepIt(0);
			if (msg)
			{
				CTRef TRef = GetLatestTRef(source, pConnection);
				if (TRef.IsInit())
				{
					string URL = SERVER_PATH[source];
					switch (source)
					{
					case N_GFS:URL += FormatA("gfs.%4d%02d%02d/%02d/gfs.t%02dz.sfluxgrbf???.grib2", TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, TRef.GetHour(), TRef.GetHour()); break;
					default: ASSERT(false);
					}

					msg = FindFiles(pConnection, URL, fileList);
				}
			}

			pConnection->Close();
			pSession->Close();


			callback.PopTask();
		}

		return msg;
	}

	std::string CUIGrib16daysForecast::GetRemoteFilePath(size_t source, CTRef TRef, size_t HH, size_t hhh)
	{
		ASSERT(TRef.GetType() == CTM::HOURLY);
		string filePath = SERVER_PATH[source];

		switch (source)
		{
		case N_GFS:filePath += FormatA("gfs.%4d%02d%02d/%02d/gfs.t%02dz.sfluxgrbf??.grib2", TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, HH, HH, hhh); break;
		}

		return  filePath;
	}



	string CUIGrib16daysForecast::GetLocaleFilePath(size_t s, const string& remote)const
	{
		string workingDir = GetDir(WORKING_DIR);


		//CTRef TRef = GetTRef(source, remote);
		//string filePath = FormatA("%s%d%02d%02d\\%s", workingDir, TRef.GetYear(), TRef.GetMonth(), TRef.GetDay(), GetFileName(remote).c_str());
		string filePath = workingDir + SOURCES_NAME[s] + "\\";
		switch (s)
		{
		case N_GFS:
		{
			string dir = GetPath(remote);
			//string dir = WBSF::GetLastDirName(GetPath(remote));
			//std::replace(dir.begin(), dir.end(), '.', '\\');
			size_t pos = dir.find(".");//find the date after the product name
			string date = dir.substr(pos + 1, 8);
			string fileName = GetFileName(remote);
			filePath += date + "\\" + fileName;
		}

		}

		return  filePath;
	}


	CTRef CUIGrib16daysForecast::GetLatestTRef(size_t source, CFtpConnectionPtr& pConnection)const
	{
		CTRef TRef;

		ERMsg msg;

		string URL = SERVER_PATH[source];
		switch (source)
		{
		case N_GFS:	URL += "gfs.*"; break;
		default: ASSERT(false);
		}


		CFileInfoVector fileListTmp;
		msg = UtilWWW::FindDirectories(pConnection, URL, fileListTmp);

		//CTRef TRef;
		if (msg)
		{
			set<CTRef> latest1;
			for (size_t i = 0; i < fileListTmp.size(); i++)
			{
				string name = WBSF::GetLastDirName(fileListTmp[i].m_filePath);
				size_t pos = name.find('.');
				if (pos != NOT_INIT)
				{
					name = name.substr(pos + 1);
					int year = WBSF::as<int>(name.substr(0, 4));
					size_t m = WBSF::as<size_t>(name.substr(4, 2)) - 1;
					size_t d = WBSF::as<size_t>(name.substr(6, 2)) - 1;

					CTRef TRef(year, m, d, 0);
					latest1.insert(TRef);
				}
			}

			set<CTRef> latest2;
			if (!latest1.empty())
			{
				for (auto it = latest1.rbegin(); it != latest1.rend() && latest2.empty(); it++)
				{
					CTRef TRef = *it;

					string URL = SERVER_PATH[source];
					switch (source)
					{
					case N_GFS:URL += FormatA("gfs.%4d%02d%02d/*", TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1); break;
					default: ASSERT(false);
					}

					CFileInfoVector dirList2;
					if (UtilWWW::FindDirectories(pConnection, URL, dirList2))
					{
						for (auto it2 = dirList2.rbegin(); it2 != dirList2.rend() && latest2.empty(); it2++)
						{
							string name = WBSF::GetLastDirName(it2->m_filePath);
							size_t HH = WBSF::as<size_t>(name);

							string URL = SERVER_PATH[source];
							switch (source)
							{
							case N_GFS:URL += FormatA("gfs.%4d%02d%02d/%02d/gfs.t%02dz.sfluxgrbf???.grib2", TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, HH, HH); break;
							default: ASSERT(false);
							}

							CFileInfoVector fileList;
							msg = FindFiles(pConnection, URL, fileList);
							if (fileList.size() == 209)
							{
								latest2.insert(TRef + HH);
							}
						}
					}
				}
			}



			if (!latest2.empty())
				TRef = *latest2.rbegin();
		}


		return TRef;
	}


	CTRef CUIGrib16daysForecast::GetLatestTRef(size_t source, CHttpConnectionPtr& pConnection)const
	{
		CTRef TRef;

		ERMsg msg;

		string URL = SERVER_PATH[source];

		CFileInfoVector dirList;
		msg = UtilWWW::FindDirectories(pConnection, URL, dirList);

		//CTRef TRef;
		if (msg)
		{
			set<CTRef> latest1;
			for (size_t i = 0; i < dirList.size(); i++)
			{
				string name = WBSF::GetLastDirName(dirList[i].m_filePath);
				size_t pos = name.find('.');
				if (pos != NOT_INIT && (pos == 3 || pos == 4))
				{
					name = name.substr(pos + 1);
					int year = WBSF::as<int>(name.substr(0, 4));
					size_t m = WBSF::as<size_t>(name.substr(4, 2)) - 1;
					size_t d = WBSF::as<size_t>(name.substr(6, 2)) - 1;

					CTRef TRef(year, m, d, 0);
					latest1.insert(TRef);
				}
			}

			set<CTRef> latest2;
			if (!latest1.empty())
			{
				for (auto it = latest1.rbegin(); it != latest1.rend() && latest2.empty(); it++)
				{
					CTRef TRef = *it;

					string URL = SERVER_PATH[source];
					switch (source)
					{
					case N_GFS:URL += FormatA("gfs.%4d%02d%02d/", TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1); break;
					default: ASSERT(false);
					}

					CFileInfoVector dirList2;
					if (UtilWWW::FindDirectories(pConnection, URL, dirList2))
					{
						for (auto it2 = dirList2.rbegin(); it2 != dirList2.rend() && latest2.empty(); it2++)
						{
							string name = WBSF::GetLastDirName(it2->m_filePath);
							size_t HH = WBSF::as<size_t>(name);

							string URL = SERVER_PATH[source];
							switch (source)
							{
							case N_GFS:URL += FormatA("gfs.%4d%02d%02d/%02d/gfs.t%02dz.sfluxgrbf???.grib2", TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, HH, HH); break;
							default: ASSERT(false);
							}

							CFileInfoVector fileList;
							msg = FindFiles(pConnection, URL, fileList);
							if (fileList.size() == 209)
							{
								latest2.insert(TRef + HH);
							}
						}
					}
				}
			}

			if (!latest2.empty())
				TRef = *latest2.rbegin();
		}


		return TRef;
	}


	ERMsg CUIGrib16daysForecast::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;



		return msg;
	}

	ERMsg CUIGrib16daysForecast::Initialize(TType type, CCallback& callback)
	{
		ERMsg msg;

		GDALSetCacheMax64(128 * 1024 * 1024);

		CTRef nowUTC = CTRef::GetCurrentTRef(CTM::HOURLY, true);

		CTPeriod p(nowUTC - 480, nowUTC + 480);//Get all file available

		CGribsMap gribsList;
		msg = GetGribsList(p, gribsList, callback);

		callback.PushTask("Open forecast images (" + to_string(gribsList.size()) + ")", gribsList.size());

		vector<CTRef> tmp;
		for (auto it = gribsList.begin(); it != gribsList.end() && msg; it++)
		{
			m_psfcDS[it->first].reset(new CSfcDatasetCached);
			tmp.push_back(it->first);
		}

		//#pragma omp parallel for shared(msg)
		for (__int64 i = 0; i < (__int64)tmp.size(); i++)
		{
#pragma omp flush(msg)
			if (msg)
			{
				//		for (auto it = gribsList.begin(); it != gribsList.end() && msg; it++)
				CTRef TRef = tmp[i];
				//m_psfcDS[TRef]->m_variables_to_load = m_variables;
				msg = m_psfcDS[TRef]->open(gribsList[TRef], true);
				GribVariables var = m_psfcDS[TRef]->get_variables();
				msg += callback.StepIt();
			}
		}


		callback.PopTask();

		return msg;
	}

	ERMsg CUIGrib16daysForecast::GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;


		if (!m_psfcDS.empty())
		{
			CTRef current = CTRef::GetCurrentTRef(TM);
			station.GetStat(H_TAIR);//force to compute stat before call GetVariablesCount
			CWVariablesCounter counter = station.GetVariablesCount();
			//CTRef TRefEnd = counter.GetTPeriod().End();
			//ASSERT(TRefEnd.as(CTM::DAILY) <= current.as(CTM::DAILY));


			//station must have data in the last week
			//clean up varaibles that are not up to date
			static const int NB_MISS_DAY_TO_IGNORE_FORECAST = 7;

			CWVariables vars = station.GetVariables();
			for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)
			{
				if (current.as(CTM::DAILY) - counter[v].second.End().as(CTM::DAILY) >= NB_MISS_DAY_TO_IGNORE_FORECAST)
					vars.reset(v);
			}

			if (vars.any())
			{

				//create all data for multi-thread
				CTPeriod p(m_psfcDS.begin()->first, m_psfcDS.rbegin()->first);

				CWeatherStation forecast_station(true);//always extract forecast as hourly
				((CLocation&)forecast_station) = station;
				forecast_station.CreateYears(p);

				size_t nbStationAdded = 0;
				string feed = "Update GFS forecast for \"" + forecast_station.m_name + "\" (extracting " + to_string(m_psfcDS.size()) + " hours)";
				//callback.PushTask(feed, m_psfcDS.size());
				//callback.AddMessage(feed);

				//convert set into vector for multi-thread
				vector<CTRef> tmp;
				for (auto it = m_psfcDS.begin(); it != m_psfcDS.end() && msg; it++)
					tmp.push_back(it->first);



				//				#pragma omp parallel for shared(msg)
				for (__int64 i = 0; i < (__int64)tmp.size(); i++)
				{
#pragma omp flush(msg)
					if (msg)
					{
						CTRef TRef = tmp[i];
						msg += ExtractStation(TRef, forecast_station, callback);
						msg += callback.StepIt(0);
#pragma omp flush(msg)

					}
				}

				CompleteVariables(forecast_station);

				p.Transform(station.GetTM());
				//add valid forecast to the station
				for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
				{
					for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)
					{
						if (vars[v] && !station[TRef][v].IsInit() && forecast_station[TRef][v].IsInit())
							station[TRef].SetStat(v, forecast_station[TRef][v]);
					}

				}

				//callback.PopTask();
			}
		}


		return msg;
	}

	ERMsg CUIGrib16daysForecast::ExtractStation(CTRef TRef, CWeatherStation& station, CCallback& callback)
	{
		ASSERT(station.GetTM() == CTM::HOURLY);

		ERMsg msg;

		CGeoPoint pt = station;

		//pt.Reproject(GEO_2_WEA);
		if (m_psfcDS[TRef]->GetExtents().IsInside(pt))
		{
			CTRef localTRef = CTimeZones::UTCTRef2LocalTRef(TRef, station);
			CHourlyData& data = station.GetHour(localTRef);
			m_psfcDS[TRef]->get_weather(pt, data);//estimate weather at location
			msg += callback.StepIt(0);
		}

		return msg;
	}

	//fill missing value between 3 hours 
	void CUIGrib16daysForecast::CompleteVariables(CWeatherStation& weather)
	{
		ASSERT(weather.IsHourly());


		//compute direct hourly value. For example RH from Tdew and TAir or Ea from Tdew or Es from Tair
		//CTPeriod period = weather.GetEntireTPeriod();
		weather.GetStat(H_TAIR);//force to compute stat before call GetVariablesCount
		CWVariablesCounter counter = weather.GetVariablesCount();
		for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)
		{
			CTPeriod period = counter[v].second;
			if (/*v != H_PRCP && */period.IsInit())
			{
				for (CTRef TRef = period.Begin(); TRef <= period.End() - 3; TRef ++)
				{
					CHourlyData& data0 = weather.GetHour(TRef);
					CHourlyData& data1 = weather.GetHour(TRef + 1);
					CHourlyData& data2 = weather.GetHour(TRef + 2);
					CHourlyData& data3 = weather.GetHour(TRef + 3);

					if (WEATHER::HaveValue(data0[v]) && WEATHER::HaveValue(data3[v]) && 
						WEATHER::IsMissing(data1[v]) && WEATHER::IsMissing(data2[v]))
					{
						data1.SetStat(v, (2 * data0[v] + 1 * data3[v]) / 3.0);
						data2.SetStat(v, (1 * data0[v] + 2 * data3[v]) / 3.0);
					}

				}
			}
		}
	}

	ERMsg CUIGrib16daysForecast::Finalize(TType type, CCallback& callback)
	{
		for (auto it = m_psfcDS.begin(); it != m_psfcDS.end(); it++)
			it->second->Close();

		m_psfcDS.clear();

		return ERMsg();
	}

	ERMsg CUIGrib16daysForecast::GetGribsList(CTPeriod p, CGribsMap& gribsList, CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		size_t source = as<size_t>(SOURCES);


		int firstYear = p.Begin().GetYear();
		int lastYear = p.End().GetYear();
		size_t nbYears = lastYear - firstYear + 1;

		for (size_t y = 0; y < nbYears; y++)
		{
			int year = firstYear + int(y);


			string path = workingDir + string(SOURCES_NAME[source]) + "\\";
			StringVector list = WBSF::GetDirectoriesList(path + "*");
			sort(list.begin(), list.end());

			for (size_t i = 0; i < list.size(); i++)
			{
				if (list[i].length() == 8)
				{
					string filter = path + list[i] + "\\*.tif";
					StringVector fileList = GetFilesList(filter, FILE_PATH, true);

					for (size_t i = 0; i < fileList.size(); i++)
					{
						CTRef TRef = GetTRef(source, fileList[i]);
						if (p.IsInside(TRef))
							gribsList[TRef] = fileList[i];
					}
				}
			}
		}


		return msg;
	}

	CTRef CUIGrib16daysForecast::GetTRef(size_t source, string filePath)
	{
		CTRef TRef;


		static const size_t POS_HOUR1[NB_SOURCES] = { 5 };
		static const size_t POS_HOUR2[NB_SOURCES] = { 18 };

		string name = GetFileTitle(filePath);
		string dir1 = WBSF::GetLastDirName(GetPath(filePath));

		int year = WBSF::as<int>(dir1.substr(0, 4));
		size_t m = WBSF::as<int>(dir1.substr(4, 2)) - 1;
		size_t d = WBSF::as<int>(dir1.substr(6, 2)) - 1;
		size_t h = WBSF::as<int>(name.substr(POS_HOUR1[source], 2));
		size_t hh = WBSF::as<int>(name.substr(POS_HOUR2[source], 3));

		TRef = CTRef(year, m, d, h) + hh;

		return TRef;
	}

	size_t CUIGrib16daysForecast::GetHH(size_t source, string filePath)
	{
		static const size_t HH_POS[NB_SOURCES] = { 18 };
		string name = GetFileTitle(filePath);
		ASSERT(name.size() >= HH_POS[source] + 2);

		string HHstr = name.substr(HH_POS[source], 3);
		ASSERT(HHstr.size() == 3);
		size_t HH = WBSF::as<size_t>(HHstr);

		return HH;
	}

	
	ERMsg CUIGrib16daysForecast::CreateHourlyGeotiff(const string& inputFilePath, CCallback& callback)const
	{
		ERMsg msg;

		string outputFilePath = inputFilePath;
		ReplaceString(outputFilePath, ".grib2", ".tif");

		ASSERT(GoodGrib(inputFilePath));

		CSfcDatasetCached DSin;
		msg += DSin.open(inputFilePath, true);
		if (msg)
		{
			int nbBands = (int)DSin.get_variables().count();
			callback.PushTask(string("Convert to GeoTiff ") + GetFileTitle(inputFilePath), nbBands);


			float no_data = 9999;// GetDefaultNoData(GDT_Int16);
			CBaseOptions options;
			DSin.UpdateOption(options);
			options.m_nbBands = nbBands;
			options.m_outputType = GDT_Float32;
			options.m_dstNodata = no_data;
			options.m_bOverwrite = true;
			options.m_extents.m_xMin -= 180;
			options.m_extents.m_xMax -= 180;
			options.m_prj = PRJ_WGS_84_WKT;
			options.m_createOptions.push_back("COMPRESS=LZW");
			options.m_createOptions.push_back("PREDICTOR=3");
			options.m_createOptions.push_back("TILED=YES");
			options.m_createOptions.push_back("BLOCKXSIZE=256");
			options.m_createOptions.push_back("BLOCKYSIZE=256");

			CGDALDatasetEx DSout;
			msg += DSout.CreateImage(outputFilePath + "2", options);
			if (msg)
			{
				for (size_t v = 0, bb = 0; v < DSin.get_variables().size() && msg; v++)
				{
					size_t b = DSin.get_band(v);

					if (b != NOT_INIT)
					{
						GDALRasterBand* pBandin = DSin.GetRasterBand(b);
						GDALRasterBand* pBandout = DSout.GetRasterBand(bb);


						ASSERT(DSin.GetRasterXSize() == DSout.GetRasterXSize());
						ASSERT(DSin.GetRasterYSize() == DSout.GetRasterYSize());

						float no_data_in = DSin.GetNoData(b);
						vector<float> data(DSin.GetRasterXSize()*DSin.GetRasterYSize());
						pBandin->RasterIO(GF_Read, 0, 0, DSin.GetRasterXSize(), DSin.GetRasterYSize(), &(data[0]), DSin.GetRasterXSize(), DSin.GetRasterYSize(), GDT_Float32, 0, 0);
						for (size_t y = 0; y < DSin.GetRasterYSize(); y++)
						{
							for (size_t x = 0; x < DSin.GetRasterXSize() / 2; x++)
							{
								size_t xy1 = y * DSin.GetRasterXSize() + x;
								size_t xy2 = y * DSin.GetRasterXSize() + DSin.GetRasterXSize() / 2 + x;
								float tmp = data[xy1];
								data[xy1] = data[xy2];
								data[xy2] = tmp;

								if (fabs(data[xy1] - no_data_in) < 0.1)
									data[xy1] = no_data;

								if (fabs(data[xy2] - no_data_in) < 0.1)
									data[xy2] = no_data;
							}
						}
						pBandout->RasterIO(GF_Write, 0, 0, DSin.GetRasterXSize(), DSin.GetRasterYSize(), &(data[0]), DSin.GetRasterXSize(), DSin.GetRasterYSize(), GDT_Float32, 0, 0);


						if (pBandin->GetDescription())
							pBandout->SetDescription(pBandin->GetDescription());

						if (pBandin->GetMetadata())
							pBandout->SetMetadata(pBandin->GetMetadata());

						bb++;

						msg += callback.StepIt();
					}

					DSout->FlushCache();
				}

				DSout.Close();

				callback.PopTask();
			}

			DSin.close();

		}//if dataset open



		if (msg)
		{
			//copy the file to fully use compression with GDAL_translate
			string argument = "-ot Float32 -stats -co COMPRESS=LZW -co PREDICTOR=3 -co TILED=YES -co BLOCKXSIZE=256 -co BLOCKYSIZE=256 \"" + outputFilePath + "2" + "\" \"" + outputFilePath + "\"";
			string command = "\"" + GetApplicationPath() + "External\\gdal_translate.exe\" " + argument;
			msg += WinExecWait(command);
			msg += RemoveFile(outputFilePath + "2");
		}

		msg += RemoveFile(inputFilePath);


		return msg;
	}
}