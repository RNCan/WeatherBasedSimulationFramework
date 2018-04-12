#include "StdAfx.h"
#include "UINAM.h"
#include "Basic/FileStamp.h"
#include "Basic/CSV.h"
#include "Basic/WeatherStation.h"
#include "UI/Common/SYShowMessage.h"


#include "TaskFactory.h"
#include "WeatherBasedSimulationString.h"
#include "../Resource.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;


namespace WBSF
{
	//NAM analysis
	//ftp://nomads.ncdc.noaa.gov/NAM/Grid218/

	//*********************************************************************
	const char* CUINAM::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "Begin", "End", "Show WinSCP"};
	const size_t CUINAM::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_DATE, T_DATE, T_BOOL};
	const UINT CUINAM::ATTRIBUTE_TITLE_ID = IDS_UPDATER_NAM_P;
	const UINT CUINAM::DESCRIPTION_TITLE_ID = ID_TASK_NAM;

	const char* CUINAM::CLASS_NAME(){ static const char* THE_CLASS_NAME = "NorthAmericanMesoscaleForecastSystem";  return THE_CLASS_NAME; }
	CTaskBase::TType CUINAM::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUINAM::CLASS_NAME(), (createF)CUINAM::create);


	
	const char* CUINAM::SERVER_NAME = "nomads.ncdc.noaa.gov";
	const char* CUINAM::NAM_FORMAT = "/data/namanl/%4d%02d/%4d%02d%02d/namanl_218_%4d%02d%02d_%02d00_%03d%s";
	const char* CUINAM::FTP_SERVER_NAME[NB_SOURCES] = { "nomads.ncdc.noaa.gov", "www.ftp.ncep.noaa.gov" };
	

	CUINAM::CUINAM(void)
	{}
	
	CUINAM::~CUINAM(void)
	{}

	
	std::string CUINAM::Option(size_t i)const
	{
		string str;
		//switch (i)
		//{
		//};
		return str;
	}

	std::string CUINAM::Default(size_t i)const
	{
		std::string str;
		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "NAM\\"; break;
		case FIRST_DATE:
		case LAST_DATE:   str = CTRef::GetCurrentTRef().GetFormatedString("%Y-%m-%d"); break;
		case SHOW_WINSCP: str = "0"; break;
		};

		return str;
	}
		
	

	//************************************************************************************************************
	//Load station definition list section

	string CUINAM::GetInputFilePath(CTRef TRef, bool bGrib)const
	{
		int y = TRef.GetYear();
		int m = int(TRef.GetMonth() + 1);
		int d = int(TRef.GetDay() + 1);
		int h = int(TRef.GetHour());

		string filePath;
		int forecastH = h % 6;
		h = int(h / 5) * 6;
		filePath = FormatA(NAM_FORMAT, y, m, y, m, d, y, m, d, h, forecastH, bGrib ? ".grb" : ".inv");

		return filePath;
	}

	string CUINAM::GetOutputFilePath(CTRef TRef, bool bGrib)const
	{
		static const char* OUTPUT_FORMAT = "%s%4d\\%02d\\%02d\\%s%4d%02d%02d_%02d00_%03d%s";
		string workingDir = GetDir(WORKING_DIR);

		int y = TRef.GetYear();
		int m = int(TRef.GetMonth() + 1);
		int d = int(TRef.GetDay() + 1);
		int h = int(TRef.GetHour());
		int forecastH = h % 6;
		h = int(h / 6) * 6;
		
		return FormatA(OUTPUT_FORMAT, workingDir.c_str(), y, m, d, "nam_218_", y, m, d, h, forecastH, bGrib ? ".grb2" : ".inv");
	}

	ERMsg CUINAM::DownloadGrib(CHttpConnectionPtr& pConnection, CTRef TRef, bool bGrib, CCallback& callback)const
	{
		ERMsg msg;

		string inputPath = GetInputFilePath(TRef, bGrib);
		string outputPath = GetOutputFilePath(TRef, bGrib);
		CreateMultipleDir(GetPath(outputPath));

		msg += CopyFile(pConnection, inputPath, outputPath, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE);
		if (msg)
		{
			CFileInfo info = GetFileInfo(outputPath);
			if (info.m_size < 10*1024*1024)//Strange thing on server smaller than 10 Mo
			{
				//remove file
				msg += RemoveFile(outputPath);
			}
		}
		return msg;
	}


	bool CUINAM::NeedDownload(const string& filePath)const
	{
		bool bDownload = true;

		if (!filePath.empty())
		{
			CFileStamp fileStamp(filePath);
			CTime lastUpdate = fileStamp.m_time;
			if (lastUpdate.GetTime() > 0)
			{
				bDownload = false;
			}
		}

		return bDownload;
	}



	//*************************************************************************************************

	CTPeriod CUINAM::GetPeriod()const
	{
		CTPeriod p;

		StringVector t1(Get(FIRST_DATE), "-/");
		StringVector t2(Get(LAST_DATE), "-/");
		if (t1.size() == 3 && t2.size() == 3)
			p = CTPeriod(CTRef(ToInt(t1[0]), ToSizeT(t1[1]) - 1, ToSizeT(t1[2]) - 1, FIRST_HOUR), CTRef(ToInt(t2[0]), ToSizeT(t2[1]) - 1, ToSizeT(t2[2]) - 1, LAST_HOUR));

		return p;
	}


	ERMsg CUINAM::Execute(CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		CreateMultipleDir(workingDir);

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(SERVER_NAME, 1);
		callback.AddMessage("");

		CTRef today = CTRef::GetCurrentTRef(CTM::HOURLY);

		CTPeriod period = GetPeriod();
		if (period.IsInit() && period.Begin() <= period.End() && period.Begin() <= today)
		{
			if (today - period.Begin() > 360*24)
			{
				msg = ExecuteHTTP(period, callback);
			}
			else
			{
				msg = ExecuteFTP(period, callback);
			}
		}
		else
		{
			msg.ajoute("Invalid period");
			return msg;
		}


		return msg;
	}

	ERMsg CUINAM::ExecuteHTTP(CTPeriod period, CCallback& callback)
	{
		ERMsg msg;
		int nbFilesToDownload = 0;
		int nbDownloaded = 0;

		CArray<bool> bGrbNeedDownload;
		bGrbNeedDownload.SetSize(period.size());

		for (CTRef h = period.Begin(); h <= period.End(); h++)
		{
			size_t hh = (h - period.Begin());

			bGrbNeedDownload[hh] = NeedDownload(GetOutputFilePath(h, true));
			nbFilesToDownload += bGrbNeedDownload[hh] ? 1 : 0;

			msg += callback.StepIt(0);
		}


		callback.PushTask("Download NAM gribs for period: " + period.GetFormatedString() + " (" + ToString(nbFilesToDownload) + " gribs)", nbFilesToDownload);

		int nbRun = 0;
		CTRef curH = period.Begin();

		while (curH < period.End() && msg)
		{
			nbRun++;

			CInternetSessionPtr pSession;
			CHttpConnectionPtr pConnection;

			msg = GetHttpConnection(SERVER_NAME, pConnection, pSession);

			if (msg)
			{
				TRY
				{
					for (CTRef h = curH; h <= period.End() && msg; h++, curH++)
					{
						size_t hh = (h - period.Begin());
						if (bGrbNeedDownload[hh])
						{
							//now try with NAM product
							msg = DownloadGrib(pConnection, h, true, callback);
							//if (msg && FileExists(GetOutputFilePath(h, true)))
								//msg = DownloadGrib(pConnection, h, false, callback);
						}


						if (msg)
						{
							curH = h;
							nbRun = 0;
							nbDownloaded++;
							msg += callback.StepIt();
						}
					}
				}
				CATCH_ALL(e)
				{
					msg = UtilWin::SYGetMessage(*e);
				}
				END_CATCH_ALL

				//if an error occur: try again
				if (!msg && !callback.GetUserCancel())
				{
					if (nbRun < 5)
					{
						callback.AddMessage(msg);
						msg = ERMsg();
						Sleep(1000);//wait 1 sec
					}
				}

				//clean connection
				pConnection->Close();
				pSession->Close();
			}
		}


		callback.AddMessage(FormatMsg(IDS_UPDATE_END, ToString(nbDownloaded), ToString(nbFilesToDownload)));
		callback.PopTask();

		return msg;
	}

	ERMsg CUINAM::ExecuteFTP(CTPeriod period, CCallback& callback)
	{
		ERMsg msg;
		
		int nbDownloaded = 0;
	
		callback.PushTask("Download NAM gribs for period: " + period.GetFormatedString(), 2);
		for (size_t s = 0; s < 2&&msg; s++)
		{
			CFileInfoVector fileList;
			msg = GetFilesToDownload(s, period, fileList, callback);
			CleanList(s, fileList);

			size_t nbFileToDownload = fileList.size();
			static const char* NAME_NET[2] = { "NOMADS", "NCEP" };
			
			callback.PushTask(string("Download NAM gribs from ") + NAME_NET[s] + ": " + ToString(nbFileToDownload) + " files", nbFileToDownload);
			

			string workingDir = GetDir(WORKING_DIR);
			string scriptFilePath = workingDir + "script.txt";
			WBSF::RemoveFile(scriptFilePath + ".log");


			for (size_t i = 0; i < fileList.size() && msg; i++)
			{
				ofStream stript;
				msg = stript.open(scriptFilePath);
				if (msg)
				{
					CTRef TRef = GetTRef(s, fileList[i].m_filePath);
					string outputFilePaht = GetOutputFilePath(TRef, true);
					string tmpFilePaht = GetPath(outputFilePaht) + GetFileName(fileList[i].m_filePath);
					CreateMultipleDir(GetPath(outputFilePaht));

					stript << "open ftp://anonymous:anonymous%40example.com@" << FTP_SERVER_NAME[s] << endl;

					stript << "cd " << GetPath(fileList[i].m_filePath) << endl;
					stript << "lcd " << GetPath(tmpFilePaht) << endl;
					stript << "get " << GetFileName(tmpFilePaht) << endl;
					stript << "exit" << endl;
					stript.close();

					bool bShow = as<bool>(SHOW_WINSCP);
					//# Execute the script using a command like:
					string command = "\"" + GetApplicationPath() + "External\\WinSCP.exe\" " + string(bShow?"/console ": "") +  "-timeout=300 -passive=on /log=\"" + scriptFilePath + ".log\" /ini=nul /script=\"" + scriptFilePath;
					DWORD exit_code;
					msg = WBSF::WinExecWait(command, "", SW_SHOW, &exit_code);
					if (msg)
					{
						//verify if the file finish with 7777
						
						if (exit_code == 0 && FileExists(tmpFilePaht))
						{
							ifStream stream;
							if (stream.open(tmpFilePaht))
							{
								char test[5] = { 0 };
								stream.seekg(-4, ifstream::end);
								stream.read(&(test[0]), 4);
								stream.close();

								if (string(test) == "7777")
								{
									nbDownloaded++;
									msg = RenameFile(tmpFilePaht, outputFilePaht);
								}
								else
								{
									callback.AddMessage("corrupt file, remove: " + tmpFilePaht);
									msg = WBSF::RemoveFile(tmpFilePaht);
								}
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
			}//for all files
			
			callback.PopTask();
			msg += callback.StepIt();
		}//for all sources 

		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbDownloaded), 2);
		callback.PopTask();


		return msg;
	}

	CTRef CUINAM::GetTRef(size_t s, const string& remote)
	{
		CTRef TRef;

		if (s == S_NOMADS)
		{
			//nam_218_20170701_0000_001.grb2
			string name = GetFileTitle(remote);
			int year = WBSF::as<int>(name.substr(8, 4));
			size_t m = WBSF::as<size_t >(name.substr(12,2))-1;
			size_t d = WBSF::as<size_t >(name.substr(14,2))-1;
			size_t h = WBSF::as<size_t >(name.substr(17,2));
			size_t hh = WBSF::as<size_t >(name.substr(23, 2));
			
			TRef = CTRef(year, m, d, h+hh);
		}
		else if (s == S_NCEP)
		{
			//nam.t00z.awphys00.tm00.grib2
			string name1 = WBSF::GetLastDirName(GetPath(remote));
			string name2 = GetFileTitle(remote);
			int year = WBSF::as<int>(name1.substr(4, 4));
			size_t m = WBSF::as<size_t >(name1.substr(8, 2))-1;
			size_t d = WBSF::as<size_t >(name1.substr(10, 2))-1;
			size_t h = WBSF::as<size_t >(name2.substr(5, 2));
			size_t hh = WBSF::as<size_t >(name2.substr(15, 2));

			TRef = CTRef(year, m, d, h+hh);
		}

		return TRef;
	}

	void CUINAM::CleanList(size_t s, CFileInfoVector& fileList1)
	{

		CFileInfoVector fileList2;
		fileList2.reserve(fileList1.size());
		for (size_t i = 0; i < fileList1.size(); i++)
		{
			CTRef TRef = GetTRef(s, fileList1[i].m_filePath);
			
			string filePath = GetOutputFilePath(TRef, true);
			//CFileStamp fileStamp(filePath);
			
			ifStream stream;
			if (stream.open(filePath))
			{
				char test[5] = { 0 };
				stream.seekg(-4, ifstream::end);
				stream.read(&(test[0]), 4);
				stream.close();

				if (string(test) != "7777")
					fileList2.push_back(fileList1[i]);
			}
			else
			{
				fileList2.push_back(fileList1[i]);
			}
		}

		fileList1 = fileList2;
	}

	ERMsg CUINAM::GetFilesToDownload(size_t s, CTPeriod period, CFileInfoVector& fileList, CCallback& callback)
	{
		ERMsg msg;
		
		CTRef now = CTRef::GetCurrentTRef();
		
		CInternetSessionPtr pSession;
		CFtpConnectionPtr pConnection;

		msg = GetFtpConnection(FTP_SERVER_NAME[s], pConnection, pSession);
		if (msg)
		{
			//CTRef TRef = GetLatestTRef(source, pConnection);
			//if (TRef.IsInit())
			//{
			if (s == S_NOMADS)
			{
				CFileInfoVector dir1;
				msg = FindDirectories(pConnection, "/33/nam/*", dir1);
				if (msg)
				{
					StringVector paths1;
					for (size_t d1 = 0; d1 != dir1.size() && msg; d1++)
					{
						string name = WBSF::GetLastDirName(dir1[d1].m_filePath);
						int year = WBSF::as<int>(name.substr(0, 4));
						size_t m = WBSF::as<size_t>(name.substr(4, 2)) - 1;
						if (name.length() == 6 && year >= 2000 && year <= 2100 && m < 12)
						{
							CTPeriod p2(CTRef(year, m, FIRST_DAY, FIRST_HOUR), CTRef(year, m, LAST_DAY, LAST_HOUR));

							if (period.IsIntersect(p2))
								paths1.push_back(dir1[d1].m_filePath);
						}
					}

					if (paths1.size()>1)
						callback.PushTask(string("Get files list from: ") + FTP_SERVER_NAME[s] + " (" + ToString(paths1.size()) + " directories)", paths1.size());

					for (size_t d1 = 0; d1 != paths1.size() && msg; d1++)
					{
						

						for (size_t d1 = 0; d1 != paths1.size() && msg; d1++)
						{
							string name = WBSF::GetLastDirName(paths1[d1]);

							CFileInfoVector dir2;
							msg = FindDirectories(pConnection, paths1[d1] + "*", dir2);
							if (msg)
							{
								vector<pair<string, CTRef>> paths2;
								for (size_t d2 = 0; d2 != dir2.size() && msg; d2++)
								{
									string name = WBSF::GetLastDirName(dir2[d2].m_filePath);
									int year = WBSF::as<int>(name.substr(0, 4));
									size_t m = WBSF::as<size_t>(name.substr(4, 2)) - 1;
									size_t d = WBSF::as<int>(name.substr(6, 2)) - 1;
									CTRef TRef(year, m, d, FIRST_HOUR);

									if (period.IsInside(TRef))
										paths2.push_back(make_pair(dir2[d2].m_filePath, TRef));
								}

								callback.PushTask(string("Get files list from: ") + name + " (" + ToString(paths2.size()) + " directories)", paths2.size()*6);
								for (size_t d2 = 0; d2 != paths2.size() && msg; d2++)
								{
									//string name = WBSF::GetLastDirName(paths2[d2].first);
									CTRef TRef = paths2[d2].second;
										
									for (size_t h = 0; h < 6; h++)
									{
										string URL = paths2[d2].first + "/";
										URL += FormatA("nam_218_%d%02d%02d_??00_0%02d.grb2", TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, h);
										CFileInfoVector fileListTmp;
										msg = FindFiles(pConnection, URL, fileListTmp);
										fileList.insert(fileList.end(), fileListTmp.begin(), fileListTmp.end());
										msg += callback.StepIt();
									}
								
								}//if msg

								callback.PopTask();
							}//if is inside
						}

						if (paths1.size()>1)
							msg += callback.StepIt();
					}//for all dir1

					if (paths1.size()>1)
						callback.PopTask();
				}//if msg
			}
			else
			{
				CTRef now = CTRef::GetCurrentTRef(CTM::HOURLY);

				if (period.End() >= now - 24)
				{
					CFileInfoVector dir1;
					msg = FindDirectories(pConnection, "/pub/data/nccf/com/nam/prod/nam.*", dir1);
					if (msg)
					{
						StringVector paths;
						for (size_t d1 = 0; d1 != dir1.size() && msg; d1++)
						{
							string name1 = WBSF::GetLastDirName(GetPath(dir1[d1].m_filePath));
							int year = WBSF::as<int>(name1.substr(4, 4));
							size_t m = WBSF::as<size_t >(name1.substr(8, 2)) - 1;
							size_t d = WBSF::as<size_t >(name1.substr(10, 2)) - 1;
							CTRef TRef(year, m, d, FIRST_HOUR);
							if (period.IsInside(TRef))
							{
								for (size_t h = 0; h < 6; h++)
								{
									string URL = dir1[d1].m_filePath + FormatA("nam.t??z.awphys%02d.tm00.grib2", h);
									paths.push_back(URL);
								}
							}
						}

						callback.PushTask(string("Get files list from: ") + FTP_SERVER_NAME[s] + " (" + ToString(paths.size()) + " files)", paths.size());
						for (size_t d1 = 0; d1 != paths.size() && msg; d1++)
						{
							CFileInfoVector fileListTmp;
							msg = FindFiles(pConnection, paths[d1], fileListTmp);
							fileList.insert(fileList.end(), fileListTmp.begin(), fileListTmp.end());
							msg += callback.StepIt();
							
						}//for all dir1

						callback.PopTask();
					}//if msg
				}
			}
			pConnection->Close();
			pSession->Close();
		}//if msg
		

		return msg;

	}



	ERMsg CUINAM::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		return msg;
	}


	ERMsg CUINAM::GetGribsList(CTPeriod p, std::map<CTRef, std::string>& gribsList, CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);

		int firstYear = p.Begin().GetYear();
		int lastYear = p.End().GetYear();
		size_t nbYears = lastYear - firstYear + 1;
		
		for (size_t y = 0; y < nbYears; y++)
		{
			int year = firstYear + int(y);

			StringVector fileList = GetFilesList(workingDir + ToString(year) + "\\*.grb2", FILE_PATH, true);
			for (size_t i = 0; i < fileList.size(); i++)
			{
				CTRef TRef = GetTRef(fileList[i]);
				if (p.IsInside(TRef))
					gribsList[TRef] = fileList[i];
			}
		}


		return msg;
	}

	ERMsg CUINAM::GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		return msg;
	}

	CTRef CUINAM::GetTRef(string filePath)
	{
		string name = GetFileTitle(filePath);
		int year = WBSF::as<int>(name.substr(8, 4));
		size_t m = WBSF::as<int>(name.substr(12, 2))-1;
		size_t d = WBSF::as<int>(name.substr(14, 2))-1;
		size_t h = WBSF::as<int>(name.substr(17, 2));
		size_t hh = WBSF::as<int>(name.substr(22, 3));


		return CTRef(year, m, d, h) + hh;//h+hh can be greather than 23
	}
}