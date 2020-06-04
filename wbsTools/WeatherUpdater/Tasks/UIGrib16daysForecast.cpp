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
	const char* CUIGrib16daysForecast::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "Sources", "Server", "MaxHour", "DeleteAfter", "ShowWinSCP" };
	const size_t CUIGrib16daysForecast::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_COMBO_INDEX, T_COMBO_INDEX, T_STRING, T_STRING, T_BOOL };
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
		case MAX_HOUR: str = "384"; break;
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
		__int32 max_hours = as<__int32>(MAX_HOUR);

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
							string inputPath = fileList[i].m_filePath;
							string outputPath = GetLocaleFilePath(source, fileList[i].m_filePath);
							CreateMultipleDir(GetPath(outputPath));

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
		__int32 max_hours = as<__int32>(MAX_HOUR);

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

					StringVector filesListTmp = GetFilesList(workingDir + dates[i] + "/*.grib2");
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
		__int32 max_hours = as<__int32>(MAX_HOUR);


		std::sort(fileList1.begin(), fileList1.end(), [](const CFileInfo& s1, const CFileInfo& s2) { return std::strcmp(s1.m_filePath.c_str(), s2.m_filePath.c_str()) < 0; });
		//select the latest file for a particular forecast hour
		map<CTRef, CFileInfo>fileList2;
		for (size_t i = 0; i < fileList1.size(); i++)
		{
			string filePath = GetLocaleFilePath(s, fileList1[i].m_filePath);
			CTRef TRefUTC = GetTRef(s, filePath);

			if (TRefUTC >= nowUTC && TRefUTC - nowUTC <= max_hours)
			{
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

		//m_variables = GetVariables(Get(VARIABLES));

			//for optimization, we select only 2 wind variables
	/*	GribVariables var = sfcDS.get_variables();
		if (var.test(H_WNDS) && var.test(H_WNDD))
		{
			var.reset(H_UWND);
			var.reset(H_VWND);
		}
		else if (var.test(H_UWND) && var.test(H_VWND))
		{
			var.reset(H_WNDS);
			var.reset(H_WNDD);
		}
		else
		{
			var.reset(H_UWND);
			var.reset(H_VWND);
			var.reset(H_WNDS);
			var.reset(H_WNDD);
		}

		sfcDS.m_variables_to_load = var;*/


		CTRef nowUTC = CTRef::GetCurrentTRef(CTM::HOURLY, true);
		__int32 max_hours = as<__int32>(MAX_HOUR);

		CTPeriod p(nowUTC - 24, nowUTC + max_hours);//estimate of actual period (only used year)

		CGribsMap gribsList;
		msg = GetGribsList(p, gribsList, callback);
		//m_pGrib.reset(new CSfcGribDatabase);

		//m_pGrib->m_nb_points = 0;
		//m_pGrib->m_bIncremental = false;
		//grib.m_variables = GetVariables(Get(VARIABLES));
		//m_pGrib->m_nbMaxThreads = omp_get_num_procs();


		callback.PushTask("Open forecast images (" + to_string(gribsList.size()) + ")", gribsList.size());

		//		for (auto it = gribsList.begin(); it != gribsList.end() && msg; it++)

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
			CWVariables vars = station.GetVariables();

			//create all data for multi-thread
			CTPeriod p(m_psfcDS.begin()->first, m_psfcDS.rbegin()->first);
			station.CreateYears(p);

			size_t nbStationAdded = 0;
			string feed = "Update GFS forecast for \"" + station.m_name + "\" (extracting " + to_string(m_psfcDS.size()) + " hours)";
			callback.PushTask(feed, m_psfcDS.size());
			callback.AddMessage(feed);

			//convert set into vector for multi-thread
			vector<CTRef> tmp;
			for (auto it = m_psfcDS.begin(); it != m_psfcDS.end() && msg; it++)
				tmp.push_back(it->first);

			//#pragma omp parallel for shared(msg)
			for (__int64 i = 0; i < (__int64)tmp.size(); i++)
			{
#pragma omp flush(msg)
				if (msg)
				{
					CTRef TRef = tmp[i];
					msg += ExtractStation(TRef, station, callback);
					msg += callback.StepIt();
#pragma omp flush(msg)

				}
			}


			station.CleanUnusedVariable(vars);


			callback.PopTask();
		}


		return msg;
	}

	ERMsg CUIGrib16daysForecast::ExtractStation(CTRef TRef, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;


		//if (msg)
		//{


			//CProjectionTransformation GEO_2_WEA(PRJ_WGS_84, m_psfcDS[TRef]->GetPrjID());
		CGeoPoint pt = station; 
		//shift longitude to 0-360 for west 
		//if (pt.m_lon < 0)
		//	pt.m_lon += 360;

		//pt.Reproject(GEO_2_WEA);
		if (m_psfcDS[TRef]->GetExtents().IsInside(pt))
		{
			CTRef localTRef = CTimeZones::UTCTRef2LocalTRef(TRef, station);
			if (station.GetTM() == CTM::HOURLY)
			{
				CHourlyData& data = station.GetHour(localTRef);
				m_psfcDS[TRef]->get_weather(pt, data);//estimate weather at location
			}
			else
			{
				CWeatherDay& data = station.GetDay(localTRef.as(CTM::DAILY));
				m_psfcDS[TRef]->get_weather(pt, data);//estimate weather at location
			}


			msg += callback.StepIt(0);
		}



		//}
		return msg;
	}


	ERMsg CUIGrib16daysForecast::Finalize(TType type, CCallback& callback)
	{
		//m_gribsList.clear();
		//m_pGrib.reset();
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

	CTRef CUIGrib16daysForecast::GetLocalTRef(string filePath)
	{
		CTRef TRef;

		string name = GetFileTitle(filePath);
		filePath = GetPath(filePath);
		string dir1 = WBSF::GetLastDirName(filePath);
		while (WBSF::IsPathEndOk(filePath))
			filePath = filePath.substr(0, filePath.length() - 1);
		filePath = GetPath(filePath);
		string dir2 = WBSF::GetLastDirName(filePath);
		while (WBSF::IsPathEndOk(filePath))
			filePath = filePath.substr(0, filePath.length() - 1);
		filePath = GetPath(filePath);
		string dir3 = WBSF::GetLastDirName(filePath);

		int year = WBSF::as<int>(dir3);
		size_t m = WBSF::as<int>(dir2) - 1;
		size_t d = WBSF::as<int>(dir1) - 1;
		size_t h = WBSF::as<int>(name.substr(6, 2));
		TRef = CTRef(year, m, d, h);

		return TRef;
	}

	ERMsg CUIGrib16daysForecast::CreateHourlyGeotiff(const string& inputFilePath, CCallback& callback)const
	{
		ERMsg msg;

		//CTRef TRef = GetLocalTRef(inputFilePath);
		string outputFilePath = inputFilePath;
		ReplaceString(outputFilePath, ".grib2", ".tif");

		ASSERT(GoodGrib(inputFilePath));

		


		//DSin.m_variables_to_load.reset(H_PRCP);
		//DSin.m_variables_to_load.reset(H_PRATE);
		//DSin.m_variables_to_load.reset(H_WNDS);//use computed wind speed and direction from U and V
		//DSin.m_variables_to_load.reset(H_SNOW);//no data available
		//DSin.m_variables_to_load.reset(H_SWE);//no data available
		
		CSfcDatasetCached DSin;
		msg += DSin.open(inputFilePath, true);
		if (msg)
		{
			int nbBands = (int)DSin.get_variables().count();


			//size_t bS = DSin1.get_band(H_WNDS);
			//size_t bU = DSin.get_band(H_UWND);
			//size_t bV = DSin.get_band(H_VWND);
			//ASSERT(bU && bV);
			////if (bU != NOT_INIT && bV != NOT_INIT)
			//nbBands += 2;

			//size_t bT = DSin.get_band(H_TAIR);
			//size_t bH = DSin.get_band(H_SPFH);
			//ASSERT(bT && bH);

			//if (bH != NOT_INIT )
			//	nbBands += 2;


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
				for (size_t v = 0, bb = 0; v < DSin.get_variables().size()&&msg; v++)
				{
					size_t b = DSin.get_band(v);
					//if (v == H_PRCP)
					//	b = DSin.get_band(H_PRATE);

					//if (v == H_PRATE)//replace PRATE by PRCP
					//	continue;

					if (b != NOT_INIT)
					{
						GDALRasterBand* pBandin = DSin.GetRasterBand(b);
						GDALRasterBand* pBandout = DSout.GetRasterBand(bb);


						ASSERT(DSin.GetRasterXSize() == DSout.GetRasterXSize());
						ASSERT(DSin.GetRasterYSize() == DSout.GetRasterYSize());

						float no_data_in = DSin.GetNoData(b);
						vector<float> data(DSin.GetRasterXSize()*DSin.GetRasterYSize());
						pBandin->RasterIO(GF_Read, 0, 0, DSin.GetRasterXSize(), DSin.GetRasterYSize(), &(data[0]), DSin.GetRasterXSize(), DSin.GetRasterYSize(), GDT_Float32, 0, 0);
						//convert 0-360 to -180-180 replace no data
						//for (size_t xy = 0; xy < data.size()/2; xy++)
						//{
						//	//if (fabs(data[xy] - no_data_in) < 0.1)
						//		//data[xy] = no_data;

						//	size_t xy1 = xy;
						//	size_t xy2 = data.size() / 2 + xy;
						//	float tmp = data[xy1];
						//	data[xy1] = data[xy2];
						//	data[xy2] = tmp;

						//	if (fabs(data[xy1] - no_data_in) < 0.1)
						//		data[xy1] = no_data;

						//	if (fabs(data[xy2] - no_data_in) < 0.1)
						//		data[xy2] = no_data;
						//}
						for (size_t y = 0; y < DSin.GetRasterYSize(); y++)
						{
							for (size_t x = 0; x < DSin.GetRasterXSize() / 2; x++)
							{
								//if (fabs(data[xy] - no_data_in) < 0.1)
									//data[xy] = no_data;

								size_t xy1 = y* DSin.GetRasterXSize() + x;
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

					//if (v == H_TDEW)
					//{
					//	ASSERT(b == NOT_INIT);//Tdew ignored here
					//	//add wind direction from U and V
					//	size_t bT = DSin.get_band(H_TAIR);
					//	size_t bH = DSin.get_band(H_SPFH);


					//	if (bT != NOT_INIT && bH != NOT_INIT)
					//	{
					//		GDALRasterBand* pBandinT = DSin.GetRasterBand(bT);
					//		GDALRasterBand* pBandinH = DSin.GetRasterBand(bH);
					//		GDALRasterBand* pBandoutTd = DSout.GetRasterBand(bb);
					//		GDALRasterBand* pBandoutHr = DSout.GetRasterBand(bb + 1);


					//		ASSERT(DSin.GetRasterXSize() == DSout.GetRasterXSize());
					//		ASSERT(DSin.GetRasterYSize() == DSout.GetRasterYSize());

					//		float no_data_inT = DSin.GetNoData(bT);
					//		float no_data_inH = DSin.GetNoData(bH);
					//		vector<float> dataT(DSin.GetRasterXSize()*DSin.GetRasterYSize());
					//		vector<float> dataH(DSin.GetRasterXSize()*DSin.GetRasterYSize());
					//		pBandinT->RasterIO(GF_Read, 0, 0, DSin.GetRasterXSize(), DSin.GetRasterYSize(), &(dataT[0]), DSin.GetRasterXSize(), DSin.GetRasterYSize(), GDT_Float32, 0, 0);
					//		pBandinH->RasterIO(GF_Read, 0, 0, DSin.GetRasterXSize(), DSin.GetRasterYSize(), &(dataH[0]), DSin.GetRasterXSize(), DSin.GetRasterYSize(), GDT_Float32, 0, 0);

					//		vector<float> dataTd(DSin.GetRasterXSize()*DSin.GetRasterYSize(), no_data);
					//		vector<float> dataHr(DSin.GetRasterXSize()*DSin.GetRasterYSize(), no_data);
					//		//replace no data
					//		for (size_t xy = 0; xy < dataT.size(); xy++)
					//		{
					//			if (fabs(dataT[xy] - no_data_inT) > 0.1 &&
					//				fabs(dataH[xy] - no_data_inH) > 0.1)
					//			{
					//				dataTd[xy] = Hs2Td(dataT[xy], dataH[xy]);
					//				dataHr[xy] = Hs2Hr(dataT[xy], dataH[xy]);
					//			}
					//		}

					//		pBandoutTd->RasterIO(GF_Write, 0, 0, DSin.GetRasterXSize(), DSin.GetRasterYSize(), &(dataTd[0]), DSin.GetRasterXSize(), DSin.GetRasterYSize(), GDT_Float32, 0, 0);
					//		pBandoutTd->SetDescription(CSfcGribDatabase::META_DATA[H_TDEW][M_DESC]);
					//		pBandoutTd->SetMetadataItem("GRIB_COMMENT", CSfcGribDatabase::META_DATA[H_TDEW][M_COMMENT]);
					//		pBandoutTd->SetMetadataItem("GRIB_ELEMENT", CSfcGribDatabase::META_DATA[H_TDEW][M_ELEMENT]);
					//		pBandoutTd->SetMetadataItem("GRIB_SHORT_NAME", CSfcGribDatabase::META_DATA[H_TDEW][M_SHORT_NAME]);
					//		pBandoutTd->SetMetadataItem("GRIB_UNIT", CSfcGribDatabase::META_DATA[H_TDEW][M_UNIT]);

					//		pBandoutHr->RasterIO(GF_Write, 0, 0, DSin.GetRasterXSize(), DSin.GetRasterYSize(), &(dataHr[0]), DSin.GetRasterXSize(), DSin.GetRasterYSize(), GDT_Float32, 0, 0);
					//		pBandoutHr->SetDescription(CSfcGribDatabase::META_DATA[H_RELH][M_DESC]);
					//		pBandoutHr->SetMetadataItem("GRIB_COMMENT", CSfcGribDatabase::META_DATA[H_RELH][M_COMMENT]);
					//		pBandoutHr->SetMetadataItem("GRIB_ELEMENT", CSfcGribDatabase::META_DATA[H_RELH][M_ELEMENT]);
					//		pBandoutHr->SetMetadataItem("GRIB_SHORT_NAME", CSfcGribDatabase::META_DATA[H_RELH][M_SHORT_NAME]);
					//		pBandoutHr->SetMetadataItem("GRIB_UNIT", CSfcGribDatabase::META_DATA[H_RELH][M_UNIT]);

					//		bb += 2;
					//	}
					//}

					//if (v == H_WNDS)
					//{
					//	ASSERT(b == NOT_INIT);//wind speed ignored here
					//	//add wind direction from U and V
					//	size_t bU = DSin.get_band(H_UWND);
					//	size_t bV = DSin.get_band(H_VWND);


					//	if (bU != NOT_INIT && bV != NOT_INIT)
					//	{
					//		GDALRasterBand* pBandinU = DSin.GetRasterBand(bU);
					//		GDALRasterBand* pBandinV = DSin.GetRasterBand(bV);
					//		GDALRasterBand* pBandoutS = DSout.GetRasterBand(bb);
					//		GDALRasterBand* pBandoutD = DSout.GetRasterBand(bb + 1);


					//		ASSERT(DSin.GetRasterXSize() == DSout.GetRasterXSize());
					//		ASSERT(DSin.GetRasterYSize() == DSout.GetRasterYSize());

					//		float no_data_inU = DSin.GetNoData(bU);
					//		float no_data_inV = DSin.GetNoData(bV);
					//		vector<float> dataU(DSin.GetRasterXSize()*DSin.GetRasterYSize());
					//		vector<float> dataV(DSin.GetRasterXSize()*DSin.GetRasterYSize());
					//		pBandinU->RasterIO(GF_Read, 0, 0, DSin.GetRasterXSize(), DSin.GetRasterYSize(), &(dataU[0]), DSin.GetRasterXSize(), DSin.GetRasterYSize(), GDT_Float32, 0, 0);
					//		pBandinV->RasterIO(GF_Read, 0, 0, DSin.GetRasterXSize(), DSin.GetRasterYSize(), &(dataV[0]), DSin.GetRasterXSize(), DSin.GetRasterYSize(), GDT_Float32, 0, 0);

					//		vector<float> dataS(DSin.GetRasterXSize()*DSin.GetRasterYSize(), no_data);
					//		vector<float> dataD(DSin.GetRasterXSize()*DSin.GetRasterYSize(), no_data);
					//		//replace no data
					//		for (size_t xy = 0; xy < dataU.size(); xy++)
					//		{
					//			if (fabs(dataU[xy] - no_data_inU) > 0.1 &&
					//				fabs(dataV[xy] - no_data_inV) > 0.1)
					//			{
					//				dataS[xy] = sqrt(dataU[xy] * dataU[xy] + dataV[xy] * dataV[xy]);
					//				dataD[xy] = (float)GetWindDirection(dataU[xy], dataV[xy], true);//approximation of wind direction
					//			}
					//		}

					//		pBandoutS->RasterIO(GF_Write, 0, 0, DSin.GetRasterXSize(), DSin.GetRasterYSize(), &(dataS[0]), DSin.GetRasterXSize(), DSin.GetRasterYSize(), GDT_Float32, 0, 0);
					//		pBandoutS->SetDescription(CSfcGribDatabase::META_DATA[H_WNDS][M_DESC]);
					//		pBandoutS->SetMetadataItem("GRIB_COMMENT", CSfcGribDatabase::META_DATA[H_WNDS][M_COMMENT]);
					//		pBandoutS->SetMetadataItem("GRIB_ELEMENT", CSfcGribDatabase::META_DATA[H_WNDS][M_ELEMENT]);
					//		pBandoutS->SetMetadataItem("GRIB_SHORT_NAME", CSfcGribDatabase::META_DATA[H_WNDS][M_SHORT_NAME]);
					//		pBandoutS->SetMetadataItem("GRIB_UNIT", CSfcGribDatabase::META_DATA[H_WNDS][M_UNIT]);


					//		pBandoutD->RasterIO(GF_Write, 0, 0, DSin.GetRasterXSize(), DSin.GetRasterYSize(), &(dataD[0]), DSin.GetRasterXSize(), DSin.GetRasterYSize(), GDT_Float32, 0, 0);
					//		pBandoutD->SetDescription(CSfcGribDatabase::META_DATA[H_WNDD][M_DESC]);
					//		pBandoutD->SetMetadataItem("GRIB_COMMENT", CSfcGribDatabase::META_DATA[H_WNDD][M_COMMENT]);
					//		pBandoutD->SetMetadataItem("GRIB_ELEMENT", CSfcGribDatabase::META_DATA[H_WNDD][M_ELEMENT]);
					//		pBandoutD->SetMetadataItem("GRIB_SHORT_NAME", CSfcGribDatabase::META_DATA[H_WNDD][M_SHORT_NAME]);
					//		pBandoutD->SetMetadataItem("GRIB_UNIT", CSfcGribDatabase::META_DATA[H_WNDS][H_WNDD]);


					//		bb += 2;
					//	}
					//}
					
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