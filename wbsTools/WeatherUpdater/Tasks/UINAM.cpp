#include "StdAfx.h"
#include "UINAM.h"
#include "Basic/FileStamp.h"
#include "Basic/CSV.h"
#include "Basic/WeatherStation.h"
#include "Geomatic/SfcGribsDatabase.h"
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

	//https://nomads.ncdc.noaa.gov/data/
	//*********************************************************************
	const char* CUINAM::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "Sources", "ServerType", "Begin", "End", "ShowWinSCP" };
	const size_t CUINAM::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_STRING_SELECT, T_COMBO_INDEX, T_DATE, T_DATE, T_BOOL };
	const UINT CUINAM::ATTRIBUTE_TITLE_ID = IDS_UPDATER_NAM_P;
	const UINT CUINAM::DESCRIPTION_TITLE_ID = ID_TASK_NAM;

	const char* CUINAM::CLASS_NAME() { static const char* THE_CLASS_NAME = "NorthAmericanMesoscaleForecastSystem";  return THE_CLASS_NAME; }
	CTaskBase::TType CUINAM::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUINAM::CLASS_NAME(), (createF)CUINAM::create);
	
	
	const char* CUINAM::HTTP_SERVER_NAME[NB_SOURCES] = { "nomads.ncdc.noaa.gov", "nomads.ncep.noaa.gov" };
	const char* CUINAM::FTP_SERVER_NAME[NB_SOURCES] = { "nomads.ncdc.noaa.gov", "www.ftp.ncep.noaa.gov" };
	const char* CUINAM::NAME_NET[NB_SOURCES] = { "NOMADS", "NCEP" };


	CUINAM::CUINAM(void)
	{}

	CUINAM::~CUINAM(void)
	{}


	std::string CUINAM::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case SOURCES: str = "NOMADS=Archived (NOMADS)|NCEP=Current (NCEP)"; break;
		case SERVER_TYPE: str = "HTTP|FTP"; break;
		};
		return str;
	}

	std::string CUINAM::Default(size_t i)const
	{
		std::string str;
		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "NAM\\"; break;
		case SERVER_TYPE: str = "1"; break;
		case FIRST_DATE:
		case LAST_DATE:   str = CTRef::GetCurrentTRef().GetFormatedString("%Y-%m-%d"); break;
		case SHOW_WINSCP: str = "0"; break;
		};

		return str;
	}



	//************************************************************************************************************
	//Load station definition list section

	string CUINAM::GetInputFilePath(CTRef TRef)const
	{
		const char* NAM_FORMAT = "/data/namanl/%4d%02d/%4d%02d%02d/namanl_218_%4d%02d%02d_%02d00_%03d.grb";
		int y = TRef.GetYear();
		int m = int(TRef.GetMonth() + 1);
		int d = int(TRef.GetDay() + 1);
		int h = int(TRef.GetHour());

		string filePath;
		int forecastH = h % 6;
		h = int(h / 5) * 6;
		filePath = FormatA(NAM_FORMAT, y, m, y, m, d, y, m, d, h, forecastH);

		return filePath;
	}

	string CUINAM::GetOutputFilePath(CTRef TRef)const
	{
		static const char* OUTPUT_FORMAT = "%s%4d\\%02d\\%02d\\%s%4d%02d%02d_%02d00_%03d.grb2";
		string workingDir = GetDir(WORKING_DIR);

		int y = TRef.GetYear();
		int m = int(TRef.GetMonth() + 1);
		int d = int(TRef.GetDay() + 1);
		int h = int(TRef.GetHour());
		int forecastH = h % 6;
		h = int(h / 6) * 6;

		return FormatA(OUTPUT_FORMAT, workingDir.c_str(), y, m, d, "nam_218_", y, m, d, h, forecastH);
	}

	ERMsg CUINAM::DownloadGrib(CHttpConnectionPtr& pConnection, CTRef TRef, size_t& nbDownloaded, CCallback& callback)const
	{
		ERMsg msg;

		string inputPath = GetInputFilePath(TRef);
		string outputPath = GetOutputFilePath(TRef);
		CreateMultipleDir(GetPath(outputPath));

		msg += CopyFile(pConnection, inputPath, outputPath, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE, true, callback);
		if (msg)
		{
			//CFileInfo info = GetFileInfo(outputPath);
			//if (info.m_size > 10 * 1024 * 1024)//Strange thing on server smaller than 10 Mo
			if(GoodGrib(outputPath))
			{
				nbDownloaded++;
			}
			else
			{
				//remove file
				msg += RemoveFile(outputPath);
			}
		}

		return msg;
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
		size_t serverType = as<size_t>(SERVER_TYPE);
		CreateMultipleDir(workingDir);
		

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

	ERMsg CUINAM::ExecuteHTTP(CCallback& callback)
	{
		ERMsg msg;
		
		std::bitset<NB_SOURCES> sources = GetSources();


		callback.PushTask("Download NAM gribs from HTTP servers (" + to_string(sources.count()) + " sources)", sources.count());
		callback.AddMessage("Download NAM gribs from HTTP server  servers (" + to_string(sources.count()) + " sources)");


		for (size_t s = 0; s < 2 && msg; s++)
		{
			if (sources[s])
			{
				CTRef now = CTRef::GetCurrentTRef(CTM::HOURLY, true);
				CTPeriod period;

				if (s == S_NOMADS)
				{
					period = GetPeriod();
					if (period.End() >= now - 48)
						period.End() = now - 48;
				}
				else if (s == S_NCEP)
				{
					period = CTPeriod(now - 7*24, now);
				}

				if (period.IsInit())
				{
					period.Transform(CTM::HOURLY);


					size_t nbFilesToDownload = 0;
					size_t nbDownloaded = 0;

					CArray<bool> bGrbNeedDownload;
					bGrbNeedDownload.SetSize(period.size());

					for (CTRef h = period.Begin(); h <= period.End(); h++)
					{
						size_t hh = (h - period.Begin());

						bGrbNeedDownload[hh] = NeedDownload(GetOutputFilePath(h));
						nbFilesToDownload += bGrbNeedDownload[hh] ? 1 : 0;

						msg += callback.StepIt(0);
					}

					callback.PushTask(string("Download NAM gribs from \"") + NAME_NET[s] + "\" (https://" + HTTP_SERVER_NAME[s] + ") for period " + period.GetFormatedString("%1 to %2") + ": " + to_string(nbFilesToDownload) + " files", nbFilesToDownload);
					callback.AddMessage(string("Download NAM gribs from \"") + NAME_NET[s] + "\" (https://" + HTTP_SERVER_NAME[s] + ") for period " + period.GetFormatedString("%1 to %2") + ": " + to_string(nbFilesToDownload) + " files");

					if (nbFilesToDownload > 0)
					{
						size_t nbTry = 0;
						CTRef curH = period.Begin();

						while (curH < period.End() && msg)
						{
							nbTry++;

							CInternetSessionPtr pSession;
							CHttpConnectionPtr pConnection;
							msg = GetHttpConnection(HTTP_SERVER_NAME[s], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);


							if (msg)
							{
								try
								{
									while (curH <= period.End() && msg)
									{
										size_t hh = (curH - period.Begin());
										if (bGrbNeedDownload[hh])
										{
											//download gribs file
											static const char* HTTP_FORMAT[2] =
											{
												"/data/namanl/%4d%02d/%4d%02d%02d/namanl_218_%4d%02d%02d_%02d00_%03d.grb",
												"pub/data/nccf/com/nam/prod/nam.%4d%02d%02d/nam.t%02dz.awphys%02d.tm00.grib2"
											};


											int y = curH.GetYear();
											int m = int(curH.GetMonth() + 1);
											int d = int(curH.GetDay() + 1);
											int h = int(curH.GetHour());
											int hs = h % 6;
											h = int(h / 6) * 6;
											ASSERT(h == 0 || h == 6 || h == 12 || h == 18);

											string inputPath;
											if (s == 0)
											{
												inputPath = FormatA(HTTP_FORMAT[s], y, m, y, m, d, y, m, d, h, hs);
												if(curH >= CTRef(2017,APRIL, DAY_10, 0))//add a 2 to the format
													inputPath += "2";
											}
											else
											{
												inputPath = FormatA(HTTP_FORMAT[s], y, m, d, h, hs);
											}
												

											string outputPath = GetOutputFilePath(curH);
											CreateMultipleDir(GetPath(outputPath));

											msg += CopyFile(pConnection, inputPath, outputPath, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE, true, callback);
											if (msg)
											{
												if (GoodGrib(outputPath))
												{
													nbDownloaded++;
												}
												else
												{
													//remove file
													msg += RemoveFile(outputPath);
												}
											}

											if (msg)
											{
												nbTry = 0;
												msg += callback.StepIt();
											}
										}//need download

										if (msg)
											curH++;
									}
								}
								catch (CException* e)
								{
									msg = UtilWin::SYGetMessage(*e);
									if (nbTry < 5)
									{
										callback.AddMessage(UtilWin::SYGetMessage(*e));
										msg += WaitServer(10, callback);
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
					}

					callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbDownloaded));
					callback.PopTask();
				}

				msg += callback.StepIt();
			}
		}
		
		callback.PopTask();

		return msg;
	}

	std::bitset<CUINAM::NB_SOURCES> CUINAM::GetSources()const
	{
		std::bitset<NB_SOURCES> bSources;
		bSources.set();

		string type = Get(SOURCES);
		if (!type.empty())
		{
			bSources.set(S_NOMADS, type.find("NOMADS") != string::npos);
			bSources.set(S_NCEP, type.find("NCEP") != string::npos);
		};

		return bSources;
	}

	ERMsg CUINAM::ExecuteFTP(CCallback& callback)
	{
		ERMsg msg;

		std::bitset<NB_SOURCES> sources = GetSources();
		
		size_t nbDownloaded = 0;

		callback.PushTask("Download NAM gribs from FTP  servers (" + to_string(sources.count()) + " sources)", sources.count());
		callback.AddMessage("Download NAM gribs from FTP servers (" + to_string(sources.count()) + " sources)");

		for (size_t s = 0; s < 2 && msg; s++)
		{
			if (sources[s])
			{

				callback.AddMessage(string("Try to connect to: ") + FTP_SERVER_NAME[s]);

				if (server_available(s))
				{
					CFileInfoVector fileList;
					msg = GetFilesToDownload(s, fileList, callback);
					if (msg)
					{
						CTPeriod p = CleanList(s, fileList);

						size_t nbFileToDownload = fileList.size();


						callback.PushTask(string("Download NAM gribs from \"") + NAME_NET[s] + "\": " + to_string(nbFileToDownload) + " files " + p.GetFormatedString("(%1 ---- %2)"), nbFileToDownload);
						callback.AddMessage(string("Download NAM gribs from \"") + NAME_NET[s] + "\": " + to_string(nbFileToDownload) + " files " + p.GetFormatedString("(%1 ---- %2)"));


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
								string outputFilePaht = GetOutputFilePath(TRef);
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
								string command = "\"" + GetApplicationPath() + "External\\WinSCP.exe\" " + string(bShow ? "/console " : "") + "-timeout=300 -passive=on /log=\"" + scriptFilePath + ".log\" /ini=nul /script=\"" + scriptFilePath;
								DWORD exit_code;
								msg = WBSF::WinExecWait(command, "", SW_SHOW, &exit_code);
								if (msg)
								{
									if (exit_code == 0 && FileExists(tmpFilePaht))
									{
										//verify if the file finish with 7777
										if (GoodGrib(tmpFilePaht))
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
									else
									{
										callback.AddMessage("Error in WinCSV");
									}

								}
							}

							msg += callback.StepIt();
						}//for all files

						callback.PopTask();
					}//if msg 

				}//server available
				else
				{
					callback.AddMessage(string("Unable to connect to: ") + FTP_SERVER_NAME[s] + ". Server skipped.");
				}


				msg += callback.StepIt();
			}
		}//for all servers 

		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + to_string(nbDownloaded));
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
			size_t m = WBSF::as<size_t >(name.substr(12, 2)) - 1;
			size_t d = WBSF::as<size_t >(name.substr(14, 2)) - 1;
			size_t h = WBSF::as<size_t >(name.substr(17, 2));
			size_t hh = WBSF::as<size_t >(name.substr(23, 2));

			TRef = CTRef(year, m, d, h + hh);
		}
		else if (s == S_NCEP)
		{
			//nam.t00z.awphys00.tm00.grib2
			string name1 = WBSF::GetLastDirName(GetPath(remote));
			string name2 = GetFileTitle(remote);
			int year = WBSF::as<int>(name1.substr(4, 4));
			size_t m = WBSF::as<size_t >(name1.substr(8, 2)) - 1;
			size_t d = WBSF::as<size_t >(name1.substr(10, 2)) - 1;
			size_t h = WBSF::as<size_t >(name2.substr(5, 2));
			size_t hh = WBSF::as<size_t >(name2.substr(15, 2));

			TRef = CTRef(year, m, d, h + hh);
		}

		return TRef;
	}

	CTPeriod CUINAM::CleanList(size_t s, CFileInfoVector& fileList1)
	{
		CTPeriod p;
		CFileInfoVector fileList2;
		fileList2.reserve(fileList1.size());
		for (size_t i = 0; i < fileList1.size(); i++)
		{
			CTRef TRef = GetTRef(s, fileList1[i].m_filePath);
			

			string filePath = GetOutputFilePath(TRef);

			if (NeedDownload(filePath))
			{
				p += TRef;
				fileList2.push_back(fileList1[i]);
			}
		}

		fileList1 = fileList2;

		return p;
	}

	bool CUINAM::server_available(size_t s)const
	{
		ERMsg msg;

		CInternetSessionPtr pSession;
		CFtpConnectionPtr pConnection;

		msg = GetFtpConnection(FTP_SERVER_NAME[s], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", true, 1);
		if (msg)
		{
			pConnection->Close();
			pSession->Close();
		}

		return msg;
	}

	ERMsg CUINAM::GetFilesToDownload(size_t s, CFileInfoVector& fileList, CCallback& callback)
	{
		ERMsg msg;

		CTRef now = CTRef::GetCurrentTRef();

		CInternetSessionPtr pSession;
		CFtpConnectionPtr pConnection;

		msg = GetFtpConnection(FTP_SERVER_NAME[s], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", true, 5, callback);
		if (msg)
		{
			//CTRef TRef = GetLatestTRef(source, pConnection);
			//if (TRef.IsInit())
			//{
			if (s == S_NOMADS)
			{
				CTPeriod period = GetPeriod();

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

					if (paths1.size() > 1)
						callback.PushTask(string("Get files list from ") + NAME_NET[s] + ": " + to_string(paths1.size()) + " directories", paths1.size());

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

								callback.PushTask(string("Get files list from ") + NAME_NET[s] + ": " + name + " " + to_string(paths2.size()) + " directories", paths2.size() * 6);
								for (size_t d2 = 0; d2 != paths2.size() && msg; d2++)
								{
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

						if (paths1.size() > 1)
							msg += callback.StepIt();
					}//for all dir1

					if (paths1.size() > 1)
						callback.PopTask();
				}//if msg
			}
			else
			{
				CTRef now = CTRef::GetCurrentTRef(CTM::HOURLY, true);

				//if (period.End() >= now - 24)
				//{
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

							
							//if (period.IsInside(TRef))
							//{
							for (size_t h = 0; h < 24; h+=6)
							{
								for (size_t hh = 0; hh < 6; hh++)
								{
									CTRef TRef(year, m, d, h+hh);
									if (TRef<now)
									{
										string output_file_path = GetOutputFilePath(TRef);
										if (NeedDownload(output_file_path))
										{
											string URL = dir1[d1].m_filePath + FormatA("nam.t%02dz.awphys%02d.tm00.grib2", h, hh);
											paths.push_back(URL);
										}
									}
								}
							}
							//}
						}

						callback.PushTask(string("Get files info from: ftp://") + FTP_SERVER_NAME[s] + " (" + ToString(paths.size()) + " files)", paths.size());
						for (size_t d1 = 0; d1 != paths.size() && msg; d1++)
						{
							CFileInfoVector fileListTmp;
							msg = FindFiles(pConnection, paths[d1], fileListTmp);
							fileList.insert(fileList.end(), fileListTmp.begin(), fileListTmp.end());
							msg += callback.StepIt();

						}//for all dir1

						callback.PopTask();
					}//if msg
				//}
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


	ERMsg CUINAM::GetGribsList(CTPeriod p, CGribsMap& gribsList, CCallback& callback)
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
		size_t m = WBSF::as<int>(name.substr(12, 2)) - 1;
		size_t d = WBSF::as<int>(name.substr(14, 2)) - 1;
		size_t h = WBSF::as<int>(name.substr(17, 2));
		size_t hh = WBSF::as<int>(name.substr(22, 3));


		return CTRef(year, m, d, h) + hh;//h+hh can be greater than 23
	}
}