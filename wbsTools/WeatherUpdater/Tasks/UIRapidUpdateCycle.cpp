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


	//*********************************************************************
	const char* CUIRapidUpdateCycle::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "Begin", "End", "UseNAM" };
	const size_t CUIRapidUpdateCycle::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_DATE, T_DATE, T_BOOL };
	const UINT CUIRapidUpdateCycle::ATTRIBUTE_TITLE_ID = IDS_UPDATER_NOMAD_RUC_P;
	const UINT CUIRapidUpdateCycle::DESCRIPTION_TITLE_ID = ID_TASK_NOMAD_RUC;

	const char* CUIRapidUpdateCycle::CLASS_NAME(){ static const char* THE_CLASS_NAME = "RapidUpdateCycle";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIRapidUpdateCycle::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIRapidUpdateCycle::CLASS_NAME(), (createF)CUIRapidUpdateCycle::create);



	const char* CUIRapidUpdateCycle::SERVER_NAME = "nomads.ncdc.noaa.gov";
	const char* CUIRapidUpdateCycle::INPUT_FORMAT1 = "/data/rucanl/%4d%02d/%4d%02d%02d/rap_252_%4d%02d%02d_%02d00_%03d%s";
	const char* CUIRapidUpdateCycle::INPUT_FORMAT2 = "/data/rucanl/%4d%02d/%4d%02d%02d/ruc2anl_130_%4d%02d%02d_%02d00_%03d%s";
	const char* CUIRapidUpdateCycle::INPUT_FORMAT3 = "/data/rucanl/%4d%02d/%4d%02d%02d/rap_130_%4d%02d%02d_%02d00_%03d%s";
	const char* CUIRapidUpdateCycle::INPUT_FORMAT4 = "/data/rap130/%4d%02d/%4d%02d%02d/rap_130_%4d%02d%02d_%02d00_%03d%s";
	const char* CUIRapidUpdateCycle::NAM_FORMAT =    "/data/nam/%4d%02d/%4d%02d%02d/nam_218_%4d%02d%02d_%02d00_%03d%s";


	CUIRapidUpdateCycle::CUIRapidUpdateCycle(void)
	{}
	
	CUIRapidUpdateCycle::~CUIRapidUpdateCycle(void)
	{}

	
	std::string CUIRapidUpdateCycle::Default(size_t i)const
	{
		std::string str;
		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "RAP\\"; break;
		case FIRST_DATE:
		case LAST_DATE:   str = CTRef::GetCurrentTRef().GetFormatedString("%Y-%m-%d"); break;
		case USE_NAM:	str = "1"; break;
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
		static const char* OUTPUT_FORMAT = "%s%4d\\%02d\\%02d\\%s%4d%02d%02d_%02d00_%03d%s";
		//if (bForecast)
			//TRef--;

		string workingDir = GetDir(WORKING_DIR);// GetAbsoluteFilePath(m_path);
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
		
		return FormatA(OUTPUT_FORMAT, workingDir.c_str(), y, m, d, (bRAP ? "rap_130_" : "nam_218_"), y, m, d, h, forecastH, bGrib ? ".grb2" : ".inv");
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


		int nbFilesToDownload = 0;
		int nbDownloaded = 0;

		CTPeriod period = GetPeriod();
		if (!period.IsInit() || period.Begin() > period.End())
		{
			msg.ajoute("Invalid period");
			return msg;
		}

		CArray<bool> bGrbNeedDownload;
		bGrbNeedDownload.SetSize(period.size());

		for (CTRef h = period.Begin(); h <= period.End(); h++)
		{
			size_t hh = (h - period.Begin());

			bGrbNeedDownload[hh] = NeedDownload(GetOutputFilePath(h, true, true, false));

			if (bGrbNeedDownload[hh] && as<bool>(USE_NAM))
				bGrbNeedDownload[hh] = NeedDownload(GetOutputFilePath(h, true, false, false));

			nbFilesToDownload += bGrbNeedDownload[hh] ? 1 : 0;

			msg += callback.StepIt(0);
		}


		callback.PushTask("Download gribs for period: " + period.GetFormatedString() + " (" + ToString(nbFilesToDownload) + " gribs)", nbFilesToDownload);

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
							if (msg && !FileExists(GetOutputFilePath(h, true, true, false)) && as<bool>(USE_NAM))
							{
								msg = DownloadGrib(pConnection, h, false, false, false, callback);
								if (msg && FileExists(GetOutputFilePath(h, false, false, false)))
								{
									msg = DownloadGrib(pConnection, h, true, false, false, callback);
									if (msg && !FileExists(GetOutputFilePath(h, true, false, false)))
									{
											
										//if .gribs does not exist, remove .inv files
										//a better solution can mayby done here by copying a valid .inv file???
										msg += RemoveFile(GetOutputFilePath(h, false, false, false));
									}
								}
							}
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


	ERMsg CUIRapidUpdateCycle::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

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