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

	//*********************************************************************
	const char* CUIRapidUpdateCycle::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "Begin", "End", "Product" };
	const size_t CUIRapidUpdateCycle::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_DATE, T_DATE, T_COMBO_INDEX };
	const UINT CUIRapidUpdateCycle::ATTRIBUTE_TITLE_ID = IDS_UPDATER_RAP_P; 
	const UINT CUIRapidUpdateCycle::DESCRIPTION_TITLE_ID = ID_TASK_RAP;

	const char* CUIRapidUpdateCycle::CLASS_NAME(){ static const char* THE_CLASS_NAME = "RapidUpdateCycle";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIRapidUpdateCycle::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIRapidUpdateCycle::CLASS_NAME(), (createF)CUIRapidUpdateCycle::create);


	
	const char* CUIRapidUpdateCycle::SERVER_NAME = "nomads.ncdc.noaa.gov";
	const char* CUIRapidUpdateCycle::INPUT_FORMAT1 = "/data/rucanl/%4d%02d/%4d%02d%02d/rap_252_%4d%02d%02d_%02d00_%03d%s";
	const char* CUIRapidUpdateCycle::INPUT_FORMAT2 = "/data/rucanl/%4d%02d/%4d%02d%02d/ruc2anl_130_%4d%02d%02d_%02d00_%03d%s";
	const char* CUIRapidUpdateCycle::INPUT_FORMAT3 = "/data/rucanl/%4d%02d/%4d%02d%02d/rap_130_%4d%02d%02d_%02d00_%03d%s";
	const char* CUIRapidUpdateCycle::INPUT_FORMAT4 = "/data/rap130/%4d%02d/%4d%02d%02d/rap_130_%4d%02d%02d_%02d00_%03d%s";
	const char* CUIRapidUpdateCycle::NAM_FORMAT =    "/data/nam/%4d%02d/%4d%02d%02d/nam_218_%4d%02d%02d_%02d00_%03d%s";
	const char* CUIRapidUpdateCycle::FTP_SERVER_NAME[NB_SOURCES] = { "nomads.ncdc.noaa.gov", "www.ftp.ncep.noaa.gov" };
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
		case PRODUCT:	str = "0"; break;
		};

		return str;
	}
		
	

	//************************************************************************************************************
	//Load station definition list section

	string CUIRapidUpdateCycle::GetInputFilePath(CTRef TRef, bool bGrib, bool bRAP, bool bForecast)const
	{
		
		if (bForecast)
			TRef--;

		int y = TRef.GetYear();
		int m = int(TRef.GetMonth() + 1);
		int d = int(TRef.GetDay() + 1);
		int h = int(TRef.GetHour());

		string filePath;
		if (bRAP)
		{
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
		}
		else
		{
			
			int forecastH = h % 6;
			h = int(h / 5) * 6;
			filePath = FormatA(NAM_FORMAT, y, m, y, m, d, y, m, d, h, forecastH, bGrib ? ".grb" : ".inv");
			
		}

		return filePath;
	}

	string CUIRapidUpdateCycle::GetOutputFilePath(CTRef TRef, bool bGrib, bool bRAP, bool bForecast)const
	{
		static const char* OUTPUT_FORMAT = "%s%s\\%4d\\%02d\\%02d\\%s%4d%02d%02d_%02d00_%03d%s";
		

		string workingDir = GetDir(WORKING_DIR);
		size_t prod = WBSF::as<size_t>(Get(PRODUCT));


		int y = TRef.GetYear();
		int m = int(TRef.GetMonth() + 1);
		int d = int(TRef.GetDay() + 1);
		int h = int(TRef.GetHour());
		int forecastH = 0;
		
		if (bRAP)
		{
			/*if (bForecast)
			{
				forecastH = 1;
			}*/
		}
		else
		{
			forecastH = h % 6;
			h = int(h / 6) * 6;
		}
		
		return FormatA(OUTPUT_FORMAT, workingDir.c_str(), PRODUCT_NAME[prod], y, m, d, (bRAP ? "rap_130_" : "nam_218_"), y, m, d, h, forecastH, bGrib ? ".grb2" : ".inv");
//		return FormatA(OUTPUT_FORMAT, workingDir.c_str(), y, m, d, y, m, d, h, forecastH, bGrib ? ".grb2" : ".inv");
	}

	ERMsg CUIRapidUpdateCycle::DownloadGrib(CHttpConnectionPtr& pConnection, CTRef TRef, bool bGrib, bool bRAP, bool bForecast, CCallback& callback)const
	{
		ERMsg msg;

		string inputPath = GetInputFilePath(TRef, bGrib, bRAP, bForecast);
		string outputPath = GetOutputFilePath(TRef, bGrib, bRAP, bForecast);
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


	bool CUIRapidUpdateCycle::NeedDownload(const string& filePath)const
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

	CTPeriod CUIRapidUpdateCycle::GetPeriod()const
	{
		CTPeriod p;

		StringVector t1(Get(FIRST_DATE), "-/");
		StringVector t2(Get(LAST_DATE), "-/");
		if (t1.size() == 3 && t2.size() == 3)
			p = CTPeriod(CTRef(ToInt(t1[0]), ToSizeT(t1[1]) - 1, ToSizeT(t1[2]) - 1, FIRST_HOUR), CTRef(ToInt(t2[0]), ToSizeT(t2[1]) - 1, ToSizeT(t2[2]) - 1, LAST_HOUR));

		return p;
	}


	ERMsg CUIRapidUpdateCycle::Execute(CCallback& callback)
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
			if (today - period.Begin() > 360)
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

	ERMsg CUIRapidUpdateCycle::ExecuteHTTP(CTPeriod period, CCallback& callback)
	{
		ERMsg msg;
		int nbFilesToDownload = 0;
		int nbDownloaded = 0;

		CArray<bool> bGrbNeedDownload;
		bGrbNeedDownload.SetSize(period.size());

		for (CTRef h = period.Begin(); h <= period.End(); h++)
		{
			size_t hh = (h - period.Begin());

			bGrbNeedDownload[hh] = NeedDownload(GetOutputFilePath(h, true, true, false));

//			if (bGrbNeedDownload[hh] && as<bool>(USE_NAM))
				//bGrbNeedDownload[hh] = NeedDownload(GetOutputFilePath(h, true, false, false));

			nbFilesToDownload += bGrbNeedDownload[hh] ? 1 : 0;

			msg += callback.StepIt(0);
		}


		callback.PushTask("Download RAP gribs for period: " + period.GetFormatedString() + " (" + ToString(nbFilesToDownload) + " gribs)", nbFilesToDownload);

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
							//download inventory
							msg = DownloadGrib(pConnection, h, false, true, false, callback);
							if (FileExists(GetOutputFilePath(h, false, true, false)))
							{
								//download gribs file
								msg = DownloadGrib(pConnection, h, true, true, false, callback);
								if (msg && !FileExists(GetOutputFilePath(h, true, true, false)))
									msg += RemoveFile(GetOutputFilePath(h, false, true, false));
							}
							
							//now try with 1 hour forecast
							if (msg && !FileExists(GetOutputFilePath(h, true, true, false)))
							{
								//download inventory
								msg = DownloadGrib(pConnection, h, false, true, true, callback);
								if (FileExists(GetOutputFilePath(h, false, true, true)))
								{
									//download gribs file
									msg = DownloadGrib(pConnection, h, true, true, true, callback);
									if (msg && !FileExists(GetOutputFilePath(h, true, true, true)))
										msg += RemoveFile(GetOutputFilePath(h, false, true, true));
								}
							}


							//now try with NAM product
							//if (msg && !FileExists(GetOutputFilePath(h, true, true, false)) && as<bool>(USE_NAM))
							//{
							//	msg = DownloadGrib(pConnection, h, false, false, false, callback);
							//	if (msg && FileExists(GetOutputFilePath(h, false, false, false)))
							//	{
							//		msg = DownloadGrib(pConnection, h, true, false, false, callback);
							//		if (msg && !FileExists(GetOutputFilePath(h, true, false, false)))
							//		{
							//				
							//			//if .gribs does not exist, remove .inv files
							//			//a better solution can mayby done here by copying a valid .inv file???
							//			msg += RemoveFile(GetOutputFilePath(h, false, false, false));
							//		}
							//	}
							//}
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
						msg.asgType(ERMsg::OK);
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

	ERMsg CUIRapidUpdateCycle::ExecuteFTP(CTPeriod period, CCallback& callback)
	{
		ERMsg msg;
		
		int nbDownloaded = 0;

		
		CFileInfoVector fileList[2];
		for (size_t s = 0; s < 2; s++)
		{
			msg = GetFilesToDownload(s, period, fileList[s], callback);
			CleanList(s, fileList[s]);
		}

		size_t nbFileToDownload = fileList[0].size() + fileList[1].size();
		callback.PushTask("Download RAP gribs for period: " + period.GetFormatedString() + " (" + ToString(nbFileToDownload) + " gribs)", nbFileToDownload);

		for (size_t s = 0; s < 2; s++)
		{
			int nbRun = 0;
			size_t curH = 0;

			while (curH < fileList[s].size() && msg)
			{
				nbRun++;

				CInternetSessionPtr pSession;
				CFtpConnectionPtr pConnection;

				msg = GetFtpConnection(FTP_SERVER_NAME[s], pConnection, pSession);

				if (msg)
				{
					TRY
					{
						for (; curH < fileList[s].size() && msg; curH++)
						{
							CTRef TRef = GetTRef(s, fileList[s][curH].m_filePath);
							//download inventory
							string outputFilePaht = GetOutputFilePath(TRef, true, true, false);
							CreateMultipleDir(GetPath(outputFilePaht));

							msg = CopyFile(pConnection, fileList[s][curH].m_filePath, outputFilePaht, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE);

							if (msg)
							{
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

					//clean connection
					pConnection->Close();
					pSession->Close();

					//if an error occur: try again
					if (!msg && !callback.GetUserCancel())
					{
						if (nbRun < 5)
						{
							callback.AddMessage(msg);
							msg = ERMsg();

							callback.PushTask("Waiting 30 seconds for server...", 600);
							for (int i = 0; i < 600 && msg; i++)
							{
								Sleep(50);//wait 50 milisec
								msg += callback.StepIt();
							}
							callback.PopTask();
						}
					}
				}
			}
		}

		callback.AddMessage(FormatMsg(IDS_UPDATE_END, ToString(nbDownloaded), ToString(nbFileToDownload)));
		callback.PopTask();


		return msg;
	}

	CTRef CUIRapidUpdateCycle::GetTRef(size_t s, const string& remote)
	{
		CTRef TRef;

		if (s == S_NOMADS)
		{
			//rap_130_%d%02d%02d_??
			string name = GetFileTitle(remote);
			int year = WBSF::as<int>(name.substr(8, 4));
			size_t m = WBSF::as<size_t >(name.substr(12,2))-1;
			size_t d = WBSF::as<size_t >(name.substr(14,2))-1;
			size_t h = WBSF::as<size_t >(name.substr(17,2));
			
			TRef = CTRef(year, m, d, h);
		}
		else if (s == S_NCEP)
		{
			string name1 = WBSF::GetLastDirName(GetPath(remote));
			string name2 = GetFileTitle(remote);
			int year = WBSF::as<int>(name1.substr(4, 4));
			size_t m = WBSF::as<size_t >(name1.substr(8, 2))-1;
			size_t d = WBSF::as<size_t >(name1.substr(10, 2))-1;
			size_t h = WBSF::as<size_t >(name2.substr(5, 2));

			TRef = CTRef(year, m, d, h);
		}

		return TRef;
	}

	void CUIRapidUpdateCycle::CleanList(size_t s, CFileInfoVector& fileList1)
	{

		CFileInfoVector fileList2;
		fileList2.reserve(fileList1.size());
		for (size_t i = 0; i < fileList1.size(); i++)
		{
			CTRef TRef = GetTRef(s, fileList1[i].m_filePath);
			
			string filePath = GetOutputFilePath(TRef, true, true, false);
			CFileStamp fileStamp(filePath);
			//CTime lastUpdate = ;
			if (fileList1[i].m_time > fileStamp.m_time)
				fileList2.push_back(fileList1[i]);
		}

		fileList1 = fileList2;
	}

	ERMsg CUIRapidUpdateCycle::GetFilesToDownload(size_t s, CTPeriod period, CFileInfoVector& fileList, CCallback& callback)
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
				msg = FindDirectories(pConnection, "/33/ruc13/*", dir1);
				if (msg)
				{
					callback.PushTask(string("Get files list from: ") + FTP_SERVER_NAME[s], dir1.size());
					for (size_t d1 = 0; d1 != dir1.size() && msg; d1++)
					{
						string name = WBSF::GetLastDirName(dir1[d1].m_filePath);
						int year = WBSF::as<int>(name.substr(0, 4));
						size_t m = WBSF::as<size_t>(name.substr(4, 2)) - 1;
						CTPeriod p2(CTRef(year, m, FIRST_DAY, FIRST_HOUR), CTRef(year, m, LAST_DAY, LAST_HOUR));

						if (period.IsIntersect(p2))
						{
							CFileInfoVector dir2;
							msg = FindDirectories(pConnection, dir1[d1].m_filePath + "*", dir2);
							if (msg)
							{
								callback.PushTask(string("Get files list from: ") + name, dir2.size());
								for (size_t d2 = 0; d2 != dir2.size() && msg; d2++)
								{
									string name = WBSF::GetLastDirName(dir2[d2].m_filePath);
									size_t d = WBSF::as<int>(name.substr(6, 2)) - 1;
									CTRef TRef(year, m, d, FIRST_HOUR);

									if (period.IsInside(TRef))
									{
										string URL = dir2[d2].m_filePath + "/";
										URL += FormatA("rap_130_%d%02d%02d_??00_000.grb2", TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1);
										CFileInfoVector fileListTmp;
										msg = FindFiles(pConnection, URL, fileListTmp);
										fileList.insert(fileList.end(), fileListTmp.begin(), fileListTmp.end());
									}

									msg += callback.StepIt();
								}//for all dir2

								callback.PopTask();
							}//if msg
						}//if is inside

						msg += callback.StepIt();
					}//for all dir1

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
						callback.PushTask(string("Get files list from: ") + FTP_SERVER_NAME[s], dir1.size());
						for (size_t d1 = 0; d1 != dir1.size() && msg; d1++)
						{
							string name1 = WBSF::GetLastDirName(GetPath(dir1[d1].m_filePath));
							int year = WBSF::as<int>(name1.substr(4, 4));
							size_t m = WBSF::as<size_t >(name1.substr(8, 2))-1;
							size_t d = WBSF::as<size_t >(name1.substr(10, 2))-1;
							CTRef TRef(year, m, d, FIRST_HOUR);
							if (period.IsInside(TRef))
							{
								//string name = WBSF::GetLastDirName(dir1[d1].m_filePath);
								size_t prod = WBSF::as<size_t>(Get(PRODUCT));
								string URL = dir1[d1].m_filePath + "rap.t??z.awp130" + PRODUCT_NAME[prod] + "f00.grib2";

								CFileInfoVector fileListTmp;
								msg = FindFiles(pConnection, URL, fileListTmp);
								fileList.insert(fileList.end(), fileListTmp.begin(), fileListTmp.end());
							}
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
		size_t m = WBSF::as<int>(name.substr(12, 2))-1;
		size_t d = WBSF::as<int>(name.substr(14, 2))-1;
		size_t h = WBSF::as<int>(name.substr(17, 2));
		size_t hh = WBSF::as<int>(name.substr(22, 3));


		return CTRef(year, m, d, h + hh);
	}
}