#include "StdAfx.h"
#include "UIRapidUpdateCycle.h"
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

	static const CGeoRect DEFAULT_BOUDINGBOX(-180, -90, 180, 90, PRJ_WGS_84);
	static const char* NAME_NET[2] = { "NOMADS", "NCEP" };

	//rap en format grib2 (13km) horaire en temps réel
	//http://nomads.ncdc.noaa.gov/data/rap130
	//http://www.ftp.ncep.noaa.gov/data/nccf/com/rap/prod/rap.20150212/
	//http://www.ftp.ncep.noaa.gov/data/nccf/com/rap/prod/rap.20150212/

	//rap 0.18 deg hourly
	//http://nomads.ncep.noaa.gov:9090/dods/rap/rap20150212


	//RUC 13 km hourly historical depuis 2005
	//http://nomads.ncdc.noaa.gov/data/rucanl/
	//RUC 252 jusqu'en 2002
	//http://nomads.ncdc.noaa.gov/data/ruc/

	//au 4 heur mais tout l'amérique du nord au 12 km
	//http://nomads.ncdc.noaa.gov/data/namanl/
	//au 4 heures mais tout le globe au 0.5 deg
	//http://nomads.ncdc.noaa.gov/data/gfsanl/

	//http://www.nco.ncep.noaa.gov/pmb/products/rap/rap.t00z.awp200f00.grib2.shtml
	//http://www.nco.ncep.noaa.gov/pmb/products/gfs/gfs.t00z.pgrb2f00.shtml

	//chaque variable est séparer (40km) en format bufr
	//ftp://tgftp.nws.noaa.gov/SL.us008001/ST.opnl/MT.rap_CY.00/RD.20150213/PT.grid_DF.bb/

	//hrrr 3km horaire bufr ou grib2 
	//http://www.nco.ncep.noaa.gov/pmb/products/hrrr/
	//http://www.ftp.ncep.noaa.gov/data/nccf/nonoperational/com/hrrr/prod/
	//http://nomads.ncep.noaa.gov/pub/data/nccf/nonoperational/com/hrrr/prod/hrrr.20150212/

	//canada
	//http://www.nco.ncep.noaa.gov/pmb/products/cmcens/cmc_gep01.t00z.pgrb2af00.shtml

	//NAM analysis
	//ftp://nomads.ncdc.noaa.gov/NAM/Grid218/


	//HTTPS server
	//https://www.ncei.noaa.gov/thredds/catalog/rap130/catalog.html
	//forecast (3 jours de lag)
	//https://www.ncei.noaa.gov/thredds/fileServer/rap130/201806/20180612/rap_130_20180612_0200_021.grb2
	//analyse (3 jours de lag)
	//https://www.ncei.noaa.gov/thredds/fileServer/rap130anl/201806/20180612/rap_130_20180612_0200_000.grb2
	//forecast 
	//http://nomads.ncep.noaa.gov/pub/data/nccf/com/rap/prod/
	//http://soostrc.comet.ucar.edu/data/grib/rap/20180614/hybrid/

	//*********************************************************************
	const char* CUIRapidUpdateCycle::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "Begin", "End", "Product", "ServerType", "ShowWinSCP" };
	const size_t CUIRapidUpdateCycle::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_DATE, T_DATE, T_COMBO_INDEX, T_COMBO_INDEX, T_BOOL };
	const UINT CUIRapidUpdateCycle::ATTRIBUTE_TITLE_ID = IDS_UPDATER_RAP_P;
	const UINT CUIRapidUpdateCycle::DESCRIPTION_TITLE_ID = ID_TASK_RAP;

	const char* CUIRapidUpdateCycle::CLASS_NAME() { static const char* THE_CLASS_NAME = "RapidUpdateCycle";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIRapidUpdateCycle::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIRapidUpdateCycle::CLASS_NAME(), (createF)CUIRapidUpdateCycle::create);




	const char* CUIRapidUpdateCycle::INPUT_FORMAT1 = "/data/rucanl/%4d%02d/%4d%02d%02d/rap_252_%4d%02d%02d_%02d00_%03d%s";
	const char* CUIRapidUpdateCycle::INPUT_FORMAT2 = "/data/rucanl/%4d%02d/%4d%02d%02d/ruc2anl_130_%4d%02d%02d_%02d00_%03d%s";
	const char* CUIRapidUpdateCycle::INPUT_FORMAT3 = "/data/rucanl/%4d%02d/%4d%02d%02d/rap_130_%4d%02d%02d_%02d00_%03d%s";
	const char* CUIRapidUpdateCycle::INPUT_FORMAT4 = "/data/rap130/%4d%02d/%4d%02d%02d/rap_130_%4d%02d%02d_%02d00_%03d%s";
	const char* CUIRapidUpdateCycle::FTP_SERVER_NAME[NB_SOURCES] = { "nomads.ncdc.noaa.gov", "www.ftp.ncep.noaa.gov" };
	const char* CUIRapidUpdateCycle::HTTP_SERVER_NAME[NB_SOURCES] = { "www.ncei.noaa.gov", "nomads.ncep.noaa.gov" };
	static char* PRODUCT_NAME[2] = { "pgrb", "bgrb" };

	CUIRapidUpdateCycle::CUIRapidUpdateCycle(void)
	{}

	CUIRapidUpdateCycle::~CUIRapidUpdateCycle(void)
	{}


	std::string CUIRapidUpdateCycle::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case PRODUCT:	str = "pgrb|bgrb"; break;
		case SERVER_TYPE: str = "HTTP|FTP"; break;
		};
		return str;
	}

	std::string CUIRapidUpdateCycle::Default(size_t i)const
	{
		std::string str;
		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "RAP\\"; break;
		case FIRST_DATE:
		case LAST_DATE:   str = CTRef::GetCurrentTRef().GetFormatedString("%Y-%m-%d"); break;
		case PRODUCT:	str = "1"; break;
		case SERVER_TYPE: str = "1"; break;
		case SHOW_WINSCP: str = "0"; break;
		};

		return str;
	}



	//************************************************************************************************************
	//Load station definition list section

	string CUIRapidUpdateCycle::GetInputFilePath(CTRef TRef, bool bGrib, bool bForecast)const
	{

		if (bForecast)
			TRef--;

		int y = TRef.GetYear();
		int m = int(TRef.GetMonth() + 1);
		int d = int(TRef.GetDay() + 1);
		int h = int(TRef.GetHour());

		string filePath;

		if (TRef < CTRef(2012, MAY, 8, 0))
		{
			if (TRef >= CTRef(2008, JANUARY, FIRST_DAY, 0) && TRef <= CTRef(2008, OCTOBER, 28, 0))
				filePath = FormatA(INPUT_FORMAT1, y, m, y, m, d, y, m, d, h, bForecast ? 1 : 0, bGrib ? ".grb" : ".inv");
			else
				filePath = FormatA(INPUT_FORMAT2, y, m, y, m, d, y, m, d, h, bForecast ? 1 : 0, bGrib ? ".grb2" : ".inv");
		}
		else
		{



			CTRef now = CTRef::GetCurrentTRef(CTM(CTM::HOURLY));
			if (now - TRef >= 24)
				filePath = FormatA(INPUT_FORMAT3, y, m, y, m, d, y, m, d, h, bForecast ? 1 : 0, bGrib ? ".grb2" : ".inv");
			else
				filePath = FormatA(INPUT_FORMAT4, y, m, y, m, d, y, m, d, h, bForecast ? 1 : 0, bGrib ? ".grb2" : ".inv");
		}

		return filePath;
	}

	string CUIRapidUpdateCycle::GetOutputFilePath(CTRef TRef, bool bGrib, bool bForecast)const
	{
		static const char* OUTPUT_FORMAT = "%s%s\\%4d\\%02d\\%02d\\%s%4d%02d%02d_%02d00_%03d%s";


		string workingDir = GetDir(WORKING_DIR);
		size_t prod = WBSF::as<size_t>(Get(PRODUCT));


		int y = TRef.GetYear();
		int m = int(TRef.GetMonth() + 1);
		int d = int(TRef.GetDay() + 1);
		int h = int(TRef.GetHour());
		int forecastH = 0;

		return FormatA(OUTPUT_FORMAT, workingDir.c_str(), PRODUCT_NAME[prod], y, m, d, "rap_130_", y, m, d, h, forecastH, bGrib ? ".grb2" : ".inv");
	}

	ERMsg CUIRapidUpdateCycle::DownloadGrib(CHttpConnectionPtr& pConnection, CTRef TRef, bool bGrib, bool bForecast, CCallback& callback)const
	{
		ERMsg msg;

		string inputPath = GetInputFilePath(TRef, bGrib, bForecast);
		string outputPath = GetOutputFilePath(TRef, bGrib, bForecast);
		CreateMultipleDir(GetPath(outputPath));

		msg += CopyFile(pConnection, inputPath, outputPath, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE);
		if (msg)
		{
			CFileInfo info = GetFileInfo(outputPath);
			if (info.m_size < 1000)//in bites?
			{
				//remove file
				msg += RemoveFile(outputPath);
			}
		}
		return msg;
	}


	bool CUIRapidUpdateCycle::GoodGrib(const string& filePath)const
	{
		bool bGoodGrib = false;

		if (!filePath.empty())
		{
			ifStream stream;
			if (stream.open(filePath))
			{
				char test[5] = { 0 };
				stream.seekg(-4, ifstream::end);
				stream.read(&(test[0]), 4);
				stream.close();
				if (string(test) == "7777")
					bGoodGrib = true;

				stream.close();
			}
		}

		return bGoodGrib;
	}



	//*************************************************************************************************

	CTPeriod CUIRapidUpdateCycle::GetPeriod()const
	{
		CTPeriod p;

		StringVector t1(Get(FIRST_DATE), "-/");
		StringVector t2(Get(LAST_DATE), "-/");
		if (t1.size() == 3 && t2.size() == 3)
			p = CTPeriod(CTRef(ToInt(t1[0]), ToSizeT(t1[1]) - 1, ToSizeT(t1[2]) - 1), CTRef(ToInt(t2[0]), ToSizeT(t2[1]) - 1, ToSizeT(t2[2]) - 1));

		return p;
	}


	ERMsg CUIRapidUpdateCycle::Execute(CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		size_t serverType = as<size_t>(SERVER_TYPE);
		CreateMultipleDir(workingDir);

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);

		CTRef today = CTRef::GetCurrentTRef();

		CTPeriod period = GetPeriod();
		if (period.IsInit() && period.Begin() <= period.End() && period.Begin() <= today)
		{
			if (serverType == HTTP_SERVER)
				msg = ExecuteHTTP(callback);
			else
				msg = ExecuteFTP(callback);
		}
		else
		{
			msg.ajoute("Invalid period");
			return msg;
		}


		return msg;
	}

	ERMsg CUIRapidUpdateCycle::ExecuteHTTP(CCallback& callback)
	{
		ERMsg msg;

		size_t prod = WBSF::as<size_t>(Get(PRODUCT));

		callback.PushTask("Download RAP gribs from HTTP servers", 2);
		for (size_t s = 0; s < 2 && msg; s++)
		{
			CTRef today = CTRef::GetCurrentTRef(CTM::DAILY);
			CTPeriod period = GetPeriod();

			if (s == S_NOMADS)
			{
				if (period.End() >= today - 2)
					period.End() = today - 2;
			}
			else if (s == S_NCEP)
			{
				if (period.End() >= today - 1)
					period = CTPeriod(today - 1, today);
				else
					period.Reset();
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

					bGrbNeedDownload[hh] = NeedDownload(GetOutputFilePath(h, true, false));
					nbFilesToDownload += bGrbNeedDownload[hh] ? 1 : 0;

					msg += callback.StepIt(0);
				}

				callback.PushTask(string("Download RAP gribs from \"") + HTTP_SERVER_NAME[s] + "\" (" + HTTP_SERVER_NAME[s] + ") for period " + period.GetFormatedString("%1 to %2") + ": " + to_string(nbFilesToDownload) + " files", nbFilesToDownload);
				callback.AddMessage(string("Download RAP gribs from \"") + HTTP_SERVER_NAME[s] + "\" (" + HTTP_SERVER_NAME[s] + ") for period " + period.GetFormatedString("%1 to %2") + ": " + to_string(nbFilesToDownload) + " files");

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
										//string inputPath = GetInputFilePath(curH, true, false);
										static const char* HTTP_FORMAT[2] =
										{
											"/thredds/fileServer/rap130/%4d%02d/%4d%02d%02d/rap_130_%4d%02d%02d_%02d00_%03d.grb2",
											"/pub/data/nccf/com/rap/prod/rap.%4d%02d%02d/rap.t%02dz.awp130%sf%02d.grib2"
										};


										int y = curH.GetYear();
										int m = int(curH.GetMonth() + 1);
										int d = int(curH.GetDay() + 1);
										int hs = int(curH.GetHour());

										string inputPath;
										if (s == 0)
											inputPath = FormatA(HTTP_FORMAT[s], y, m, y, m, d, y, m, d, hs, 0);
										else
											inputPath = FormatA(HTTP_FORMAT[s], y, m, d, hs, PRODUCT_NAME[prod], 0);

										string outputPath = GetOutputFilePath(curH, true, false);
										CreateMultipleDir(GetPath(outputPath));

										msg += CopyFile(pConnection, inputPath, outputPath, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE);
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

		callback.PopTask();

		return msg;
	}


	ERMsg CUIRapidUpdateCycle::ExecuteFTP(CCallback& callback)
	{
		ERMsg msg;

		int nbDownloaded = 0;

		CTPeriod period = GetPeriod();
		period.Transform(CTM::HOURLY);

		callback.PushTask("Download RAP gribs from FTP servers (NOMADS, NCEP) for period: " + period.GetFormatedString(), 2);
		callback.AddMessage("Download NAM gribs from FTP servers (NOMADS, NCEP) for period: " + period.GetFormatedString("%1 ---- %2"));

		for (size_t s = 0; s < 2 && msg; s++)
		{
			callback.AddMessage(string("Try to connect to: ") + FTP_SERVER_NAME[s]);

			if (server_available(s))
			{
				CFileInfoVector fileList;
				msg = GetFilesToDownload(s, period, fileList, callback);
				if (msg)
				{
					CTPeriod p_avail = CleanList(s, fileList);
					size_t nbFileToDownload = fileList.size();

					callback.PushTask(string("Download RAP gribs from \"") + NAME_NET[s] + "\": " + to_string(nbFileToDownload) + " files " + p_avail.GetFormatedString("(%1 ---- %2)"), nbFileToDownload);
					callback.AddMessage(string("Download RAP gribs from \"") + NAME_NET[s] + "\": " + to_string(nbFileToDownload) + " files " + p_avail.GetFormatedString("(%1 ---- %2)"));


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
							string outputFilePaht = GetOutputFilePath(TRef, true, false);
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
								//verify if the file finish with 7777
								if (exit_code == 0)
								{
									if (GoodGrib(tmpFilePaht))
									{
										nbDownloaded++;
										msg = RenameFile(tmpFilePaht, outputFilePaht);
									}
									else
									{
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

					callback.PopTask();
				}


			}//server available
			else
			{
				callback.AddMessage(string("Unable to connect to: ") + FTP_SERVER_NAME[s] + ". Server skipped.");
			}


			msg += callback.StepIt();
		}//for 2 sources

		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbDownloaded));
		callback.PopTask();

		return msg;
	}



	CTRef CUIRapidUpdateCycle::GetTRef(size_t s, const string& remote)
	{
		CTRef TRef;

		if (s == S_NOMADS)
		{
			string name = GetFileTitle(remote);
			int year = WBSF::as<int>(name.substr(8, 4));
			size_t m = WBSF::as<size_t >(name.substr(12, 2)) - 1;
			size_t d = WBSF::as<size_t >(name.substr(14, 2)) - 1;
			size_t h = WBSF::as<size_t >(name.substr(17, 2));

			TRef = CTRef(year, m, d, h);
		}
		else if (s == S_NCEP)
		{
			string name1 = WBSF::GetLastDirName(GetPath(remote));
			string name2 = GetFileTitle(remote);
			int year = WBSF::as<int>(name1.substr(4, 4));
			size_t m = WBSF::as<size_t >(name1.substr(8, 2)) - 1;
			size_t d = WBSF::as<size_t >(name1.substr(10, 2)) - 1;
			size_t h = WBSF::as<size_t >(name2.substr(5, 2));

			TRef = CTRef(year, m, d, h);
		}

		return TRef;
	}


	CTPeriod CUIRapidUpdateCycle::CleanList(size_t s, CFileInfoVector& fileList1)
	{
		CTPeriod p_avail;
		CFileInfoVector fileList2;
		fileList2.reserve(fileList1.size());
		for (size_t i = 0; i < fileList1.size(); i++)
		{
			CTRef TRef = GetTRef(s, fileList1[i].m_filePath);
			p_avail += TRef;

			string filePath = GetOutputFilePath(TRef, true, false);
			if (NeedDownload(filePath))
			{
				fileList2.push_back(fileList1[i]);
			}

		}

		fileList1 = fileList2;

		return p_avail;
	}

	bool CUIRapidUpdateCycle::server_available(size_t s)const
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

	ERMsg CUIRapidUpdateCycle::GetFilesToDownload(size_t s, CTPeriod period, CFileInfoVector& fileList, CCallback& callback)
	{
		ERMsg msg;

		CTRef now = CTRef::GetCurrentTRef();

		CInternetSessionPtr pSession;
		CFtpConnectionPtr pConnection;

		msg = GetFtpConnection(FTP_SERVER_NAME[s], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", true, 5, callback);
		if (msg)
		{
			if (s == S_NOMADS)
			{
				CFileInfoVector dir1;
				msg = FindDirectories(pConnection, "/33/ruc13/*", dir1);
				if (msg)
				{
					StringVector paths1;
					for (size_t d1 = 0; d1 != dir1.size() && msg; d1++)
					{
						string name = WBSF::GetLastDirName(dir1[d1].m_filePath);
						int year = WBSF::as<int>(name.substr(0, 4));
						size_t m = WBSF::as<size_t>(name.substr(4, 2)) - 1;
						CTPeriod p2(CTRef(year, m, FIRST_DAY, FIRST_HOUR), CTRef(year, m, LAST_DAY, LAST_HOUR));
						if (period.IsIntersect(p2))
							paths1.push_back(dir1[d1].m_filePath);
					}

					if (paths1.size() > 1)
						callback.PushTask(string("Get files list from ") + NAME_NET[s] + ": " + ToString(paths1.size()) + " directories", paths1.size());

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

							callback.PushTask(string("Get files list from ") + NAME_NET[s] + ": "+  name + " (" + ToString(paths2.size()) + " directories)", paths2.size());
							for (size_t d2 = 0; d2 != paths2.size() && msg; d2++)
							{
								string URL = paths2[d2].first + "/";
								CTRef TRef = paths2[d2].second;
								URL += FormatA("rap_130_%d%02d%02d_??00_000.grb2", TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1);
								CFileInfoVector fileListTmp;
								msg = FindFiles(pConnection, URL, fileListTmp);
								fileList.insert(fileList.end(), fileListTmp.begin(), fileListTmp.end());

								msg += callback.StepIt();
							}//for all dir2

							callback.PopTask();
						}//if msg

						if (paths1.size() > 1)
							msg += callback.StepIt();

					}//for all dir1

					if (paths1.size() > 1)
						callback.PopTask();
				}//if msg
			}
			else
			{
				CTRef now = CTRef::GetCurrentTRef(CTM::HOURLY);

				if (period.End() >= now - 24)
				{
					CFileInfoVector dir1;
					msg = FindDirectories(pConnection, "/pub/data/nccf/com/rap/prod/rap.*", dir1);
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
								size_t prod = WBSF::as<size_t>(Get(PRODUCT));
								string URL = dir1[d1].m_filePath + "rap.t??z.awp130" + PRODUCT_NAME[prod] + "f00.grib2";
								paths.push_back(URL);
							}
							msg += callback.StepIt();
						}//for all dir1

						callback.PushTask(string("Get files list from ") + NAME_NET[s] + ": " + ToString(paths.size()) + " directories", paths.size());
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



	ERMsg CUIRapidUpdateCycle::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		return msg;
	}


	ERMsg CUIRapidUpdateCycle::GetGribsList(CTPeriod p, std::map<CTRef, std::string>& gribsList, CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		size_t prod = WBSF::as<size_t>(Get(PRODUCT));

		int firstYear = p.Begin().GetYear();
		int lastYear = p.End().GetYear();
		size_t nbYears = lastYear - firstYear + 1;

		for (size_t y = 0; y < nbYears; y++)
		{
			int year = firstYear + int(y);

			StringVector fileList = GetFilesList(workingDir + PRODUCT_NAME[prod] + "\\" + ToString(year) + "\\*.grb2", FILE_PATH, true);
			for (size_t i = 0; i < fileList.size(); i++)
			{
				CTRef TRef = GetTRef(fileList[i]);
				if (p.IsInside(TRef))
					gribsList[TRef] = fileList[i];
			}
		}


		return msg;
	}

	ERMsg CUIRapidUpdateCycle::GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		return msg;
	}

	CTRef CUIRapidUpdateCycle::GetTRef(string filePath)
	{
		string name = GetFileTitle(filePath);
		int year = WBSF::as<int>(name.substr(8, 4));
		size_t m = WBSF::as<int>(name.substr(12, 2)) - 1;
		size_t d = WBSF::as<int>(name.substr(14, 2)) - 1;
		size_t h = WBSF::as<int>(name.substr(17, 2));
		size_t hh = WBSF::as<int>(name.substr(22, 3));


		return CTRef(year, m, d, h + hh);
	}
}