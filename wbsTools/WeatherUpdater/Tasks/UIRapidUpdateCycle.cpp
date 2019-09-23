#include "StdAfx.h"
#include "UIRapidUpdateCycle.h"
#include "Basic/FileStamp.h"
#include "Basic/CSV.h"
#include "Basic/WeatherStation.h"
#include "Geomatic/SfcGribsDatabase.h"
#include "UI/Common/SYShowMessage.h"


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

	static const CGeoRect DEFAULT_BOUDINGBOX(-180, -90, 180, 90, PRJ_WGS_84);


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

	//after 2012 at ucar:
	//http://soostrc.comet.ucar.edu/data/grib/rap/20190620/hybrid/

	//*********************************************************************
	const char* CUIRapidUpdateCycle::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "Sources", "Begin", "End", "Product", "ServerType", "ShowWinSCP", "ComputeHourlyPrcp" };
	const size_t CUIRapidUpdateCycle::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_STRING_SELECT, T_DATE, T_DATE, T_COMBO_INDEX, T_COMBO_INDEX, T_BOOL, T_BOOL };
	const UINT CUIRapidUpdateCycle::ATTRIBUTE_TITLE_ID = IDS_UPDATER_RAP_P;
	const UINT CUIRapidUpdateCycle::DESCRIPTION_TITLE_ID = ID_TASK_RAP;

	const char* CUIRapidUpdateCycle::CLASS_NAME() { static const char* THE_CLASS_NAME = "RapidUpdateCycle";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIRapidUpdateCycle::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIRapidUpdateCycle::CLASS_NAME(), (createF)CUIRapidUpdateCycle::create);




	const char* CUIRapidUpdateCycle::INPUT_FORMAT1 = "/data/rucanl/%4d%02d/%4d%02d%02d/rap_252_%4d%02d%02d_%02d00_%03d.grb";
	const char* CUIRapidUpdateCycle::INPUT_FORMAT2 = "/data/rucanl/%4d%02d/%4d%02d%02d/ruc2anl_130_%4d%02d%02d_%02d00_%03d.grb";
	const char* CUIRapidUpdateCycle::INPUT_FORMAT3 = "/data/rucanl/%4d%02d/%4d%02d%02d/rap_130_%4d%02d%02d_%02d00_%03d.grb";
	const char* CUIRapidUpdateCycle::INPUT_FORMAT4 = "/data/rap130/%4d%02d/%4d%02d%02d/rap_130_%4d%02d%02d_%02d00_%03d.grb";

	const char* CUIRapidUpdateCycle::HTTP_SERVER_NAME[NB_SOURCES] = { /*"www.ncei.noaa.gov"*/"nomads.ncdc.noaa.gov", "nomads.ncep.noaa.gov", "soostrc.comet.ucar.edu" };
	const char* CUIRapidUpdateCycle::FTP_SERVER_NAME[NB_SOURCES] = { "nomads.ncdc.noaa.gov", "www.ftp.ncep.noaa.gov", "" };
	const char* CUIRapidUpdateCycle::PRODUCT_NAME[NB_PRODUCT] = { "pgrb", "bgrb" };
	const char* CUIRapidUpdateCycle::NAME_NET[NB_SOURCES] = { "NOMADS", "NCEP", "UCAR" };

	CUIRapidUpdateCycle::CUIRapidUpdateCycle(void)
	{}

	CUIRapidUpdateCycle::~CUIRapidUpdateCycle(void)
	{}


	std::string CUIRapidUpdateCycle::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case SOURCES: str = "NOMADS=Archived (NOMADS)|NCEP=Current (NCEP)|UCAR=Hybrid (UCAR)"; break;
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
		case COMPUTE_HOURLY_PRCP: str = "1"; break;
		};

		return str;
	}



	//************************************************************************************************************
	//Load station definition list section

	string CUIRapidUpdateCycle::GetInputFilePath(CTRef TRef, size_t HH)const
	{
		int y = TRef.GetYear();
		int m = int(TRef.GetMonth() + 1);
		int d = int(TRef.GetDay() + 1);
		int h = int(TRef.GetHour());

		string filePath;

		if (TRef < CTRef(2012, MAY, 8, 0))
		{
			if (TRef >= CTRef(2008, JANUARY, FIRST_DAY, 0) && TRef <= CTRef(2008, OCTOBER, 28, 0))
				filePath = FormatA(INPUT_FORMAT1, y, m, y, m, d, y, m, d, h, HH);
			else
				filePath = FormatA(INPUT_FORMAT2, y, m, y, m, d, y, m, d, h, HH);
		}
		else
		{
			CTRef now = CTRef::GetCurrentTRef(CTM(CTM::HOURLY));
			if (now - TRef >= 24)
				filePath = FormatA(INPUT_FORMAT3, y, m, y, m, d, y, m, d, h, HH);
			else
				filePath = FormatA(INPUT_FORMAT4, y, m, y, m, d, y, m, d, h, HH);
		}

		return filePath;
	}

	string CUIRapidUpdateCycle::GetOutputFilePath(CTRef TRef, size_t HH)const
	{
		static const char* OUTPUT_FORMAT = "%s%s\\%4d\\%02d\\%02d\\%s%4d%02d%02d_%02d00_%03d.grb2";

		string workingDir = GetDir(WORKING_DIR);
		size_t prod = WBSF::as<size_t>(Get(PRODUCT));

		int y = TRef.GetYear();
		int m = int(TRef.GetMonth() + 1);
		int d = int(TRef.GetDay() + 1);
		int h = int(TRef.GetHour());

		return FormatA(OUTPUT_FORMAT, workingDir.c_str(), PRODUCT_NAME[prod], y, m, d, "rap_130_", y, m, d, h, HH);
	}

	//ERMsg CUIRapidUpdateCycle::DownloadGrib(CHttpConnectionPtr& pConnection, CTRef TRef, CCallback& callback)const
	//{
	//	ERMsg msg;

	//	string inputPath = GetInputFilePath(TRef);
	//	string outputPath = GetOutputFilePath(TRef);
	//	CreateMultipleDir(GetPath(outputPath));

	//	msg += CopyFile(pConnection, inputPath, outputPath, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE, true, callback);
	//	if (msg)
	//	{
	//		CFileInfo info = GetFileInfo(outputPath);
	//		if (info.m_size < 1000)//in bites?
	//		{
	//			//remove file
	//			msg += RemoveFile(outputPath);
	//		}
	//	}
	//	return msg;
	//}


	//*************************************************************************************************

	CTPeriod CUIRapidUpdateCycle::GetPeriod()const
	{
		CTPeriod p;

		StringVector t1(Get(FIRST_DATE), "-/");
		StringVector t2(Get(LAST_DATE), "-/");
		if (t1.size() == 3 && t2.size() == 3)
			p = CTPeriod(CTRef(ToInt(t1[0]), ToSizeT(t1[1]) - 1, ToSizeT(t1[2]) - 1, FIRST_HOUR), CTRef(ToInt(t2[0]), ToSizeT(t2[1]) - 1, ToSizeT(t2[2]) - 1, LAST_HOUR));

		return p;
	}

	CTPeriod CUIRapidUpdateCycle::GetPeriod(size_t s)const
	{
		CTPeriod p;

		CTRef now = CTRef::GetCurrentTRef(CTM::HOURLY, true);
		if (s == S_NOMADS)
		{
			p = GetPeriod();
			if (p.End() >= now - 4 * 24)
				p.End() = now - 4 * 24;
		}
		else if (s == S_NCEP)
		{
			p = CTPeriod(now - 2 * 24, now);
		}
		else if (s == S_UCAR)
		{
			p = GetPeriod();
			if (p.End() >= now)
				p.End() = now;
		}
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


		if (serverType == HTTP_SERVER)
			msg = ExecuteHTTP(callback);
		else
			msg = ExecuteFTP(callback);

		return msg;
	}

	ERMsg CUIRapidUpdateCycle::ExecuteHTTP(CCallback& callback)
	{
		ERMsg msg;

		size_t prod = WBSF::as<size_t>(Get(PRODUCT));
		std::bitset<NB_SOURCES> sources = GetSources();
		bool compute_prcp = as<bool>(COMPUTE_HOURLY_PRCP);

		set<string> date_to_update;


		callback.PushTask("Download RAP gribs from HTTP servers (" + to_string(sources.count()) + " sources)", sources.count());
		for (size_t s = 0; s < NB_SOURCES && msg; s++)
		{
			if (sources[s])
			{
				CTPeriod period = GetPeriod(s);
				if (period.IsInit())
				{
					size_t nbFilesToDownload = 0;
					size_t nbDownloaded = 0;

					vector< array<bool, 2>> bGrbNeedDownload;
					bGrbNeedDownload.resize(period.size());

					size_t nb_pass = compute_prcp ? 2 : 1;

					for (CTRef h = period.Begin(); h <= period.End(); h++)
					{
						size_t hh = (h - period.Begin());
						for (size_t HH = 0; HH < nb_pass; HH++)
						{
							bGrbNeedDownload[hh][HH] = NeedDownload(GetOutputFilePath(h, HH));
							nbFilesToDownload += bGrbNeedDownload[hh][HH] ? 1 : 0;
						}
						msg += callback.StepIt(0);
					}

					callback.PushTask(string("Download RAP gribs from \"") + NAME_NET[s] + " for period " + period.GetFormatedString("%1 ---- %2") + ": " + to_string(nbFilesToDownload) + " files", nbFilesToDownload);
					callback.AddMessage(string("Download RAP gribs from \"") + NAME_NET[s] + " for period " + period.GetFormatedString("%1 ---- %2") + ": " + to_string(nbFilesToDownload) + " files");

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
										for (size_t HH = 0; HH < nb_pass; HH++)
										{
											if (bGrbNeedDownload[hh][HH])
											{
												//download gribs file
												static const char* HTTP_FORMAT[NB_SOURCES] =
												{
													//"/thredds/fileServer/rap130/%4d%02d/%4d%02d%02d/rap_130_%4d%02d%02d_%02d00_%03d.grb2",
													"/data/rucanl/%4d%02d/%4d%02d%02d/rap_130_%4d%02d%02d_%02d00_%03d.grb2",
													"/pub/data/nccf/com/rap/prod/rap.%4d%02d%02d/rap.t%02dz.awp130%sf%02d.grib2",
													"/data/grib/rap/%4d%02d%02d/hybrid/%2d%02d%02d%02d.rap.t%02dz.awp130%sf%02d.grib2"
												};


												int y = curH.GetYear();
												int m = int(curH.GetMonth() + 1);
												int d = int(curH.GetDay() + 1);
												int hs = int(curH.GetHour());

												string inputPath;
												switch (s)
												{
												case S_NOMADS:inputPath = FormatA(HTTP_FORMAT[s], y, m, y, m, d, y, m, d, hs, HH); break;
												case S_NCEP:inputPath = FormatA(HTTP_FORMAT[s], y, m, d, hs, PRODUCT_NAME[prod], HH); break;
												case S_UCAR:inputPath = FormatA(HTTP_FORMAT[s], y, m, d, y - 2000, m, d, hs, hs, PRODUCT_NAME[prod], HH); break;
												default: ASSERT(false);
												}

												string outputPath = GetOutputFilePath(curH, HH);
												CreateMultipleDir(GetPath(outputPath));

												msg += CopyFile(pConnection, inputPath, outputPath, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE, true, callback);
												if (msg)
												{
													if (GoodGrib(outputPath))
													{
														nbDownloaded++;
														date_to_update.insert(curH.GetFormatedString("%Y-%m-%d-%H"));
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
										}//nb pass

										if (msg)
											curH++;
									}//for all hours


								}
								catch (CException* e)
								{
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

				if (compute_prcp)
					ComputePrcp(s, date_to_update, callback);


				msg += callback.StepIt();

			}
		}

		callback.PopTask();


		return msg;
	}

	std::bitset<CUIRapidUpdateCycle::NB_SOURCES> CUIRapidUpdateCycle::GetSources()const
	{
		std::bitset<NB_SOURCES> bSources;
		bSources.set();

		string type = Get(SOURCES);
		if (!type.empty())
		{
			bSources.set(S_NOMADS, type.find("NOMADS") != string::npos);
			bSources.set(S_NCEP, type.find("NCEP") != string::npos);
			bSources.set(S_UCAR, type.find("UCAR") != string::npos);
		};

		return bSources;
	}

	ERMsg CUIRapidUpdateCycle::ExecuteFTP(CCallback& callback)
	{
		ERMsg msg;

		size_t nbDownloaded = 0;

		std::bitset<NB_SOURCES> sources = GetSources();
		bool compute_prcp = as<bool>(COMPUTE_HOURLY_PRCP);

		set<string> date_to_update;


		callback.PushTask("Download RAP gribs from FTP servers (" + to_string(sources.count()) + " sources)", sources.count());
		callback.AddMessage("Download NAM gribs from FTP servers  (" + to_string(sources.count()) + " sources)");

		for (size_t s = 0; s < NB_SOURCES && msg; s++)
		{
			if (sources[s])
			{
				callback.AddMessage(string("Try to connect to: ") + FTP_SERVER_NAME[s]);

				CFileInfoVector fileList;
				msg = GetFilesToDownload(s, fileList, callback);
				if (msg)
				{
					CTPeriod p = CleanList(s, fileList);
					size_t nbFileToDownload = fileList.size();

					callback.PushTask(string("Download RAP gribs from \"") + NAME_NET[s] + "\": " + to_string(nbFileToDownload) + " files " + p.GetFormatedString("(%1 ---- %2)"), nbFileToDownload);
					callback.AddMessage(string("Download RAP gribs from \"") + NAME_NET[s] + "\": " + to_string(nbFileToDownload) + " files " + p.GetFormatedString("(%1 ---- %2)"));


					string workingDir = GetDir(WORKING_DIR);


					size_t nbTry = 0;
					size_t cur_I = 0;
					while (nbTry < 5 && cur_I < fileList.size() && msg)
					{
						nbTry++;

						CInternetSessionPtr pSession;
						CFtpConnectionPtr pConnection;

						msg = GetFtpConnection(FTP_SERVER_NAME[s], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "anonymous", "test@canada.ca", true, 5, callback);
						if (msg)
						{
							for (size_t i = cur_I; i < fileList.size() && msg; i++)
							{
								CTRef TRef = GetRemoteTRef(s, fileList[i].m_filePath);
								size_t HH = GetRemoteHH(s, fileList[i].m_filePath);
								string outputFilePath = GetOutputFilePath(TRef, HH);
								CreateMultipleDir(GetPath(outputFilePath));

								try
								{
									UtilWWW::CopyFile(pConnection, fileList[i].m_filePath, outputFilePath, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE);

									if (GoodGrib(outputFilePath))
									{
										nbDownloaded++;
										date_to_update.insert(TRef.GetFormatedString("%Y-%m-%d-%H"));
										nbTry = 0;
									}
									else
									{
										msg = WBSF::RemoveFile(outputFilePath);
									}

									cur_I++;
									msg += callback.StepIt();

								}
								catch (CException* e)
								{
									if (nbTry < 5)
									{
										callback.AddMessage(UtilWin::SYGetMessage(*e));
										msg = WaitServer(10, callback);
									}
									else
									{
										msg = UtilWin::SYGetMessage(*e);
									}
								}


							}//for all file
							pConnection->Close();
							pSession->Close();
						}//if good connection
					}//try 5 time

					callback.PopTask();



					//	}
						//string scriptFilePath = workingDir + "script.txt";
						//WBSF::RemoveFile(scriptFilePath + ".log");


						//for (size_t i = 0; i < fileList.size() && msg; i++)
						//{
						//	ofStream stript;
						//	msg = stript.open(scriptFilePath);
						//	if (msg)
						//	{
						//		CTRef TRef = GetTRef(s, fileList[i].m_filePath);
						//		string outputFilePaht = GetOutputFilePath(TRef);
						//		string tmpFilePaht = GetPath(outputFilePaht) + GetFileName(fileList[i].m_filePath);
						//		CreateMultipleDir(GetPath(outputFilePaht));

						//		stript << "open ftp://anonymous:anonymous%40example.com@" << FTP_SERVER_NAME[s] << endl;
						//		stript << "cd " << GetPath(fileList[i].m_filePath) << endl;
						//		stript << "lcd " << GetPath(tmpFilePaht) << endl;
						//		stript << "get " << GetFileName(tmpFilePaht) << endl;
						//		stript << "exit" << endl;
						//		stript.close();

						//		bool bShow = as<bool>(SHOW_WINSCP);
						//		//# Execute the script using a command like:
						//		string command = "\"" + GetApplicationPath() + "External\\WinSCP.exe\" " + string(bShow ? "/console " : "") + "-timeout=300 -passive=on /log=\"" + scriptFilePath + ".log\" /ini=nul /script=\"" + scriptFilePath;
						//		DWORD exit_code;
						//		msg = WBSF::WinExecWait(command, "", SW_SHOW, &exit_code);
						//		if (msg)
						//		{
						//			//verify if the file finish with 7777
						//			if (exit_code == 0)
						//			{
						//				if (GoodGrib(tmpFilePaht))
						//				{
						//					nbDownloaded++;
						//					msg = RenameFile(tmpFilePaht, outputFilePaht);
						//				}
						//				else
						//				{
						//					msg = WBSF::RemoveFile(tmpFilePaht);
						//				}

						//			}
						//			else
						//			{
						//				//msg.ajoute("Error in WinCSV");
						//				callback.AddMessage("Error in WinCSV");
						//			}
						//		}
						//	}
						//	msg += callback.StepIt();
						//}


				}//if find file


				//}//server available
				//	else
				//	{
				//	callback.AddMessage(string("Unable to connect to: ") + FTP_SERVER_NAME[s] + ". Server skipped.");
				//	}
				if (compute_prcp)
					ComputePrcp(s, date_to_update, callback);


				msg += callback.StepIt();
			}//is source selected
		}//for 2 sources

		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbDownloaded));
		callback.PopTask();




		return msg;
	}



	CTRef CUIRapidUpdateCycle::GetRemoteTRef(size_t s, const string& remote)
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

	size_t CUIRapidUpdateCycle::GetRemoteHH(size_t s, const string& remote)
	{
		size_t HH = 0;

		if (s == S_NOMADS)
		{
			string name = GetFileTitle(remote);
			HH = WBSF::as<size_t >(name.substr(22, 3));
		}
		else if (s == S_NCEP)
		{
			string name2 = GetFileTitle(remote);
			HH = WBSF::as<size_t >(name2.substr(20, 2));
		}

		return HH;
	}

	CTPeriod CUIRapidUpdateCycle::CleanList(size_t s, CFileInfoVector& fileList1)
	{
		CTPeriod p;

		CFileInfoVector fileList2;
		fileList2.reserve(fileList1.size());

		for (size_t i = 0; i < fileList1.size(); i++)
		{
			CTRef TRef = GetRemoteTRef(s, fileList1[i].m_filePath);
			size_t HH = GetRemoteHH(s, fileList1[i].m_filePath);

			string filePath = GetOutputFilePath(TRef, HH);
			if (NeedDownload(filePath))
			{
				p += TRef;
				fileList2.push_back(fileList1[i]);
			}

		}

		fileList1 = fileList2;

		return p;
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

	ERMsg CUIRapidUpdateCycle::GetFilesToDownload(size_t s, CFileInfoVector& fileList, CCallback& callback)
	{
		ERMsg msg;

		CTRef now = CTRef::GetCurrentTRef();
		bool compute_prcp = as<bool>(COMPUTE_HOURLY_PRCP);
		size_t nb_pass = compute_prcp ? 2 : 1;

		CInternetSessionPtr pSession;
		CFtpConnectionPtr pConnection;

		msg = GetFtpConnection(FTP_SERVER_NAME[s], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "anonymous", "test@canada.ca", true, 5, callback);
		if (msg)
		{
			if (s == S_NOMADS)
			{
				CTPeriod period = GetPeriod();
				ASSERT( period.GetTType() == CTM::HOURLY);

				CFileInfoVector dir1;
				msg = FindDirectories(pConnection, "/RUC/analysis_only/*", dir1);
				if (msg)
				{
					StringVector paths1;
					for (size_t d1 = 0; d1 != dir1.size() && msg; d1++)
					{
						string name = WBSF::GetLastDirName(dir1[d1].m_filePath);
						if (name.length() == 6)
						{
							int year = WBSF::as<int>(name.substr(0, 4));
							size_t m = WBSF::as<size_t>(name.substr(4, 2)) - 1;
							if (year >= 2005 && m < 12)
							{
								CTPeriod p2(CTRef(year, m, FIRST_DAY, FIRST_HOUR), CTRef(year, m, LAST_DAY, LAST_HOUR));
								if (period.IsIntersect(p2))
									paths1.push_back(dir1[d1].m_filePath);
							}
						}
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

							callback.PushTask(string("Get files list from ") + NAME_NET[s] + ": " + name + " (" + ToString(paths2.size()) + " directories)", paths2.size()*nb_pass);


							for (size_t d2 = 0; d2 != paths2.size() && msg; d2++)
							{
								for (size_t HH = 0; HH < nb_pass; HH++)
								{

									string URL = paths2[d2].first + "/";
									CTRef TRef = paths2[d2].second;
									URL += FormatA("rap_130_%d%02d%02d_??00_%03d.grb2", TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, HH);
									CFileInfoVector fileListTmp;
									msg = FindFiles(pConnection, URL, fileListTmp);
									fileList.insert(fileList.end(), fileListTmp.begin(), fileListTmp.end());

									msg += callback.StepIt();
								}
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
			else if (s == S_NCEP)
			{
				CTRef now = CTRef::GetCurrentTRef(CTM::HOURLY, true);

				//if (period.End() >= now - 24)
				//{
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

						size_t prod = WBSF::as<size_t>(Get(PRODUCT));


						for (size_t h = 0; h < 24; h++)
						{
							CTRef TRef(year, m, d, h);
							for (size_t HH = 0; HH < nb_pass; HH++)
							{

								if (TRef < now)
								{
									string output_file_path = GetOutputFilePath(TRef, HH);
									if (NeedDownload(output_file_path))
									{
										string URL = dir1[d1].m_filePath + FormatA("rap.t%02dz.awp130%sf%02d.grib2", h, PRODUCT_NAME[prod], HH);
										paths.push_back(URL);
									}
								}
							}//for all hours
						}//nb pass
						msg += callback.StepIt();
					}//for all dir1

					callback.PushTask(string("Get files info from ") + NAME_NET[s] + ": " + ToString(paths.size()) + " files", paths.size());
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
			else if (s == S_UCAR)
			{
				callback.AddMessage("Ucar is not available on FTP");
			}

			pConnection->Close();
			pSession->Close();
		}//if msg


		return msg;

	}

	ERMsg CUIRapidUpdateCycle::ComputePrcp(size_t s, set<string> date_to_update, CCallback& callback)const
	{
		ERMsg msg;

		if (date_to_update.empty())
			date_to_update = GetAll(s, callback);

		callback.PushTask("Compute RAP precipitation (" + ToString(date_to_update.size()) + ")", date_to_update.size());
		callback.AddMessage("Compute RAP precipitation : " + ToString(date_to_update.size()));

		for (auto it = date_to_update.begin(); it != date_to_update.end() && msg; it++)
		{
			CTRef TRef;
			TRef.FromFormatedString(*it);
			string outputFilePath = GetOutputFilePath(TRef, 0);
			msg += CreateGeoTif(outputFilePath, callback);
			msg += callback.StepIt();
		}

		callback.PopTask();


		return msg;
	}



	ERMsg CUIRapidUpdateCycle::CreateGeoTif(const string& inputFilePath, CCallback& callback)const
	{
		ERMsg msg;

		CTRef TRef = GetLocalTRef(inputFilePath);
		string inputFilePath2 = GetOutputFilePath(TRef - 1, 1);

		string outputFilePath = inputFilePath;
		ReplaceString(outputFilePath, "_000.grb2", "_000.tif");

		string VRTFilePath = inputFilePath;
		ReplaceString(VRTFilePath, "_000.grb2", "_000.vrt");

		if (GoodGrib(inputFilePath) || GoodGrib(inputFilePath2))
		{
			int nbBands = 0;
			CSfcDatasetCached DSin1;
			if (GoodGrib(inputFilePath))
			{
				DSin1.m_variables_to_load.set();
				DSin1.m_variables_to_load.reset(H_PRCP);
				msg += DSin1.open(inputFilePath, true);
				if (msg)
					nbBands += (int)DSin1.get_variables().count();
			}

			CSfcDatasetCached DSin2;
			if (GoodGrib(inputFilePath2))
			{
				DSin2.m_variables_to_load.set(H_PRCP);
				msg += DSin2.open(inputFilePath2, true);
				if (msg)
				{
					ASSERT(DSin2.get_variables().count() == 1);
					nbBands += (int)DSin2.get_variables().count();
				}
			}

			if (msg)
			{
				

				CBaseOptions options;
				DSin1.UpdateOption(options);
				options.m_nbBands = nbBands;
				options.m_outputType = GDT_Float32;
				options.m_dstNodata = GetDefaultNoData(GDT_Float32);
				options.m_bOverwrite = true;

				CGDALDatasetEx DSout;
				msg += DSout.CreateImage(outputFilePath, options);
				if (msg)
				{
					
					for (size_t v = 0, bb = 0; v < DSin1.get_variables().size(); v++)
					{
						CSfcDatasetCached&  DSin = (v == H_PRCP) ? DSin2 : DSin1;
						if (DSin.IsOpen())
						{
							size_t b = DSin.get_band(v);

							if (b != NOT_INIT)
							{
								GDALRasterBand* pBandin = DSin.GetRasterBand(b);
								GDALRasterBand* pBandout = DSout.GetRasterBand(bb);


								ASSERT(DSin.GetRasterXSize() == DSout.GetRasterXSize());
								ASSERT(DSin.GetRasterYSize() == DSout.GetRasterYSize());

								vector<float> data(DSin.GetRasterXSize()*DSin.GetRasterYSize());
								pBandin->RasterIO(GF_Read, 0, 0, DSin.GetRasterXSize(), DSin.GetRasterYSize(), &(data[0]), DSin.GetRasterXSize(), DSin.GetRasterYSize(), GDT_Float32, 0, 0);
								pBandout->RasterIO(GF_Write, 0, 0, DSin.GetRasterXSize(), DSin.GetRasterYSize(), &(data[0]), DSin.GetRasterXSize(), DSin.GetRasterYSize(), GDT_Float32, 0, 0);

								if (pBandin->GetDescription())
									pBandout->SetDescription(pBandin->GetDescription());

								if (pBandin->GetMetadata())
									pBandout->SetMetadata(pBandin->GetMetadata());

								bb++;
							}
						}
						DSout->FlushCache();

					}

					DSout.Close();
				}

				DSin1.close();
				DSin2.close();

			}//if dataset open



			if (msg)
			{
				//copy the file to fully use compression with GDAL_translate
				msg += RenameFile(outputFilePath, outputFilePath + "2");
				//string argument = "-ot Float32 -co COMPRESS=LZW -co PREDICTOR=3 -co TILED=YES -co BLOCKXSIZE=128 -co BLOCKYSIZE=128 \"" + outputFilePath + "2" + "\" \"" + outputFilePath + "\"";
				//do not support block
				string argument = "-ot Float32 -co COMPRESS=LZW -co PREDICTOR=3 \"" + outputFilePath + "2" + "\" \"" + outputFilePath + "\"";
				string command = "\"" + GetApplicationPath() + "External\\gdal_translate.exe\" " + argument;
				msg += WinExecWait(command);
				msg += RemoveFile(outputFilePath + "2");
				//msg += callback.StepIt();


			}
		}//good grib


		return msg;
	}

	set<string> CUIRapidUpdateCycle::GetAll(size_t s, CCallback& callback)const
	{
		ERMsg msg;

		size_t prod = WBSF::as<size_t>(Get(PRODUCT));
		string workingDir = GetDir(WORKING_DIR) + PRODUCT_NAME[prod] + "//";
		
		CTPeriod p = GetPeriod(s);


		set<string> date_to_update;
		StringVector years = WBSF::GetDirectoriesList(workingDir + "*");
		for (StringVector::const_iterator it1 = years.begin(); it1 != years.end() && msg; it1++)
		{
			string year = *it1;

			if (p.IsIntersect(CTPeriod(ToInt(year), ToInt(year)).as(CTM::HOURLY)))
			{

				StringVector months = WBSF::GetDirectoriesList(workingDir + *it1 + "\\*");
				for (StringVector::const_iterator it2 = months.begin(); it2 != months.end() && msg; it2++)
				{
					string month = *it2;
					if (p.IsIntersect(CTPeriod(ToInt(year), ToSizeT(month) - 1, ToInt(year), ToSizeT(month) - 1, 0).as(CTM::HOURLY)))
					{
						StringVector days = WBSF::GetDirectoriesList(workingDir + *it1 + "\\" + *it2 + "\\*");
						for (StringVector::const_iterator it3 = days.begin(); it3 != days.end() && msg; it3++)
						{
							string day = *it3;
							if (p.IsIntersect(CTPeriod(ToInt(year), ToSizeT(month) - 1, ToSizeT(day) - 1, ToInt(year), ToSizeT(month) - 1, ToSizeT(day) - 1).as(CTM::HOURLY)))
							{
								string filter = FormatA("%s%s\\%s\\%s\\*.grb2", workingDir.c_str(), year.c_str(), month.c_str(), day.c_str());
								StringVector files = WBSF::GetFilesList(filter, 2, true);
								for (StringVector::const_iterator it4 = files.begin(); it4 != files.end() && msg; it4++)
								{
									string title = GetFileTitle(*it4);
									ASSERT(title.length() == 25);
									string hour = title.substr(17, 2);

									CTRef TRef(ToInt(year), ToSizeT(month) - 1, ToSizeT(day) - 1, ToSizeT(hour));
									string tifFilePath = GetOutputFilePath(TRef, 0);
									ReplaceString(tifFilePath, "_000.grb2", "_000.tif");
									if (!WBSF::FileExists(tifFilePath))
									{
										date_to_update.insert(TRef.GetFormatedString("%Y-%m-%d-%H"));
									}
								}

								msg += callback.StepIt(0);
							}//for all days
						}//if intersect day
					}//if intersect month
				}//for all months
			}//if intersect year
		}//for all years

		return date_to_update;
	}




	ERMsg CUIRapidUpdateCycle::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		return msg;
	}


	ERMsg CUIRapidUpdateCycle::GetGribsList(CTPeriod p, CGribsMap& gribsList, CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		size_t prod = WBSF::as<size_t>(Get(PRODUCT));
		bool compute_prcp = as<bool>(COMPUTE_HOURLY_PRCP);

		int firstYear = p.Begin().GetYear();
		int lastYear = p.End().GetYear();
		size_t nbYears = lastYear - firstYear + 1;

		for (size_t y = 0; y < nbYears; y++)
		{
			int year = firstYear + int(y);

			string filter = compute_prcp ? "\\*_000.tif" : "\\*_000.grb2";
			StringVector fileList = GetFilesList(workingDir + PRODUCT_NAME[prod] + "\\" + ToString(year) + filter, FILE_PATH, true);
			for (size_t i = 0; i < fileList.size(); i++)
			{
				CTRef TRef = GetLocalTRef(fileList[i]);
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

	CTRef CUIRapidUpdateCycle::GetLocalTRef(string filePath)
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