#include "StdAfx.h"
#include "UIGribForecast.h"
#include "Basic/FileStamp.h"
#include "UI/Common/SYShowMessage.h"
#include "Geomatic/TimeZones.h"
//#include "cctz\time_zone.h"
#include "TaskFactory.h"
#include "WeatherBasedSimulationString.h"
#include "../Resource.h"
#include "HRDPS.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;


namespace WBSF
{

	//forecast
	//http://www.emc.ncep.noaa.gov/mmb/research/meso.products.html
	//http://nomads.ncep.noaa.gov/pub/data/nccf/com/nam/prod/nam.20170615/nam.t12z.conusnest.hiresf00.tm00.grib2
	//http://nomads.ncep.noaa.gov/pub/data/nccf/com/nam/prod/
	//http://nomads.ncep.noaa.gov/pub/data/nccf/com/rap/prod/rap.20170615/

	//ftp://ftp.ncep.noaa.gov/pub/data/nccf/com/nam/prod/
	//ftp://ftpprd.ncep.noaa.gov/pub/data/nccf/com/nam/prod/
	//ftp://ftpprd.ncep.noaa.gov/pub/data/nccf/com/rap/prod/
	//ftp://ftpprd.ncep.noaa.gov/pub/data/nccf/com/hrrr/prod/


	//ftp nomads
	//ftp://nomads.ncdc.noaa.gov/NAM/Grid218/201705/20170531/
	//pour estimation de surface, c'est très bien et beaucoup plus petit que HRRR
	//High-Resolution Window (HIRESW) Forecast System 
	//ftp://ftpprd.ncep.noaa.gov/pub/data/nccf/com/hiresw/prod/hiresw.20170616/
	//GFS Ensemble Forecast System
	//ftp://ftp.ncep.noaa.gov/pub/data/nccf/com/gens/prod/gefs.20170616/12/ndgd_gb2/
	//Real-Time Mesoscale Analysis (RTMA) Products (meme extent que Hires)
	//ftp://ftp.ncep.noaa.gov/pub/data/nccf/com/rtma/prod/rtma2p5.20170616/


	//HRDPS Canada 4 km
	//http://dd.weather.gc.ca/model_hrdps/continental/grib2
	//HRRR USA  3km 
	//http://nomads.ncep.noaa.gov/pub/data/nccf/com/hrrr/prod



	//*********************************************************************
	const char* CUIGribForecast::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "Sources", "MaxHour", "ShowWinSCP", "Variables" };
	const size_t CUIGribForecast::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_COMBO_INDEX, T_STRING, T_BOOL, T_STRING_SELECT };
	const UINT CUIGribForecast::ATTRIBUTE_TITLE_ID = IDS_UPDATER_GRIB_FORECAST_P;
	const UINT CUIGribForecast::DESCRIPTION_TITLE_ID = ID_TASK_GRIB_FORECAST;

	const char* CUIGribForecast::CLASS_NAME() { static const char* THE_CLASS_NAME = "GribForecast";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIGribForecast::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIGribForecast::CLASS_NAME(), (createF)CUIGribForecast::create);

	const char* CUIGribForecast::SOURCES_NAME[NB_SOURCES] = { "HRDPS", "HRRR", "HRRR(SRF)", "RAP(P)", "RAP(B)", "NAM" };
	const char* CUIGribForecast::SERVER_NAME[NB_SOURCES] = { "dd.weather.gc.ca", "ftpprd.ncep.noaa.gov", "ftpprd.ncep.noaa.gov", "ftpprd.ncep.noaa.gov", "ftpprd.ncep.noaa.gov", "ftpprd.ncep.noaa.gov" };
	const char* CUIGribForecast::SERVER_PATH[NB_SOURCES] = { "model_hrdps/continental/grib2/", "pub/data/nccf/com/hrrr/prod/", "pub/data/nccf/com/hrrr/prod/", "pub/data/nccf/com/rap/prod/", "pub/data/nccf/com/rap/prod/", "pub/data/nccf/com/nam/prod/" };

	CUIGribForecast::CUIGribForecast(void)
	{}

	CUIGribForecast::~CUIGribForecast(void)
	{}

	std::string CUIGribForecast::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case SOURCES:	str = "HRDPS (canada)|HRRR (3D)|HRRR (SFC)|RAP P (Canada/USA)|RAP B (Canada/USA)|NAM (Canada/USA)"; break;
		case HRDPS_VARS: str = CHRDPSVariables::GetHRDPSSelectionString(); break;
		};
		return str;
	}

	std::string CUIGribForecast::Default(size_t i)const
	{
		std::string str;
		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "Forecast\\"; break;
		case SOURCES: str = "0"; break;
		case MAX_HOUR: str = "24"; break;
		case HRDPS_VARS: str = "PRATE_SFC|PRES_SFCDPT_TGL|RH_TGL|TMP_TGL|WDIR_TGL|WIND_TGL"; break;
		};

		return str;
	}



	//************************************************************************************************************
	//Load station definition list section


	//*************************************************************************************************

	bool CUIGribForecast::GoodGrib(const string& filePath)const
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



	ERMsg CUIGribForecast::Execute(CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		CreateMultipleDir(workingDir);


		int nbDownload = 0;
		size_t source = as<size_t>(SOURCES);
		__int32 max_hours = as<__int32>(MAX_HOUR);

		if (source == N_HRDPS)
		{
			CHRDPS HRDPS(workingDir + SOURCES_NAME[N_HRDPS] + "\\");
			HRDPS.m_bForecast = true;
			HRDPS.m_variables = Get(HRDPS_VARS);
			HRDPS.m_max_hours = max_hours;
			msg = HRDPS.Execute(callback);

		}
		else
		{
			string scriptFilePath = workingDir + "script.txt";
			WBSF::RemoveFile(scriptFilePath + ".log");


			CFileInfoVector fileList;
			msg = GetFilesToDownload(source, fileList, callback);
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

					stript << "open ftp://anonymous:anonymous%40example.com@" << SERVER_NAME[source] << endl;

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
				//CInternetSessionPtr pSession;
				//CFtpConnectionPtr pConnection;

				//msg = GetFtpConnection(SERVER_NAME[source], pConnection, pSession);

				//
				//for (size_t i = curI; i != fileList.size() && msg; i++)
				//{
				//	string outputFilePath = GetLocaleFilePath(source, fileList[i].m_filePath);
				//	CreateMultipleDir(GetPath(outputFilePath));
				//	//if (!FileExists(outputFilePath))
				//	//{
				//	callback.PushTask("Download forecast gribs:" + outputFilePath, NOT_INIT);
				//	msg = CopyFile(pConnection, fileList[i].m_filePath, outputFilePath, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE);
				//	callback.PopTask();
				//	//}

				//	
				//	if (msg && FileExists(outputFilePath))
				//	{
				//		nbDownload++;
				//		nbRun = 0;
				//		curI++;
				//		msg += callback.StepIt();
				//	}

				//}

				//pConnection->Close();
				//pSession->Close();

				//if (!msg && !callback.GetUserCancel())
				//{
				//	callback.AddMessage(msg);
				//	msg = ERMsg();
				//}
			}
		}


		//delete old files
		Clean(source);

		callback.AddMessage(string("Number of ") + SOURCES_NAME[source] + " forecast gribs downloaded: " + ToString(nbDownload));
		callback.PopTask();

		return msg;
	}


	ERMsg CUIGribForecast::Clean(size_t source)
	{
		ERMsg msg;


		return msg;
	}

	CTPeriod CUIGribForecast::CleanList(size_t s, CFileInfoVector& fileList1)
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
			if (NeedDownload(filePath))
			{
				CTRef TRefUTC = GetTRef(s, filePath);
				p += TRefUTC;
				fileList1.push_back(it->second);
			}
		}

		return p;
	}




	ERMsg CUIGribForecast::GetFilesToDownload(size_t source, CFileInfoVector& fileList, CCallback& callback)
	{
		ERMsg msg;


		callback.PushTask(string("Get files list from: ") + SERVER_PATH[source], 2);

		ASSERT(source != N_HRDPS);

		CInternetSessionPtr pSession;
		CFtpConnectionPtr pConnection;

		msg = GetFtpConnection(SERVER_NAME[source], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", true, 5, callback);
		msg += callback.StepIt(0);
		if (msg)
		{
			CTRef TRef = GetLatestTRef(source, pConnection);
			if (TRef.IsInit())
			{
				string URL = SERVER_PATH[source];
				switch (source)
				{
				case N_HRRR:	URL += FormatA("hrrr.%4d%02d%02d/conus/hrrr.t??z.wrfnatf??.grib2", TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1); break;
				case N_HRRR_SRF:URL += FormatA("hrrr.%4d%02d%02d/conus/hrrr.t??z.wrfsfcf??.grib2", TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1); break;
				case N_RAP_P:	URL += FormatA("rap.%4d%02d%02d/rap.t??z.awp130pgrbf??.grib2", TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1); break;
				case N_RAP_B:	URL += FormatA("rap.%4d%02d%02d/rap.t??z.awp130bgrbf??.grib2", TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1); break;
				case N_NAM:		URL += FormatA("nam.%4d%02d%02d/nam.t??z.awphys??.tm00.grib2", TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1); break;
				default: ASSERT(false);
				}

				msg = FindFiles(pConnection, URL, fileList);
			}
		}

		pConnection->Close();
		pSession->Close();


		callback.PopTask();

		return msg;
	}

	std::string CUIGribForecast::GetRemoteFilePath(size_t source, CTRef TRef, size_t HH)
	{
		ASSERT(TRef.GetType() == CTM::HOURLY);
		string filePath = SERVER_PATH[source];

		switch (source)
		{
		case N_HRDPS:	filePath += FormatA("%02d/%02d/", TRef.GetHour(), HH); break;
		case N_HRRR:	filePath += FormatA("hrrr.%4d%02d%02d/conus/hrrr.t%02dz.wrfnatf%02d.grib2", TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, TRef.GetHour(), HH); break;
		case N_HRRR_SRF:filePath += FormatA("hrrr.%4d%02d%02d/conus/hrrr.t%02dz.wrfsfcf%02d.grib2", TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, TRef.GetHour(), HH); break;
		case N_RAP_P:	filePath += FormatA("rap.%4d%02d%02d/rap.t%02dz.awp130pgrbf%02d.grib2", TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, TRef.GetHour(), HH); break;
		case N_RAP_B:	filePath += FormatA("rap.%4d%02d%02d/rap.t%02dz.awp130bgrbf%02d.grib2", TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, TRef.GetHour(), HH); break;
		case N_NAM:		filePath += FormatA("nam.%4d%02d%02d/nam.t%02dz.awphys%02d.tm00.grib2", TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, TRef.GetHour(), HH); break;
		}

		return  filePath;
	}

	string CUIGribForecast::GetLocaleFilePath(size_t s, const string& remote)const
	{
		string workingDir = GetDir(WORKING_DIR);


		//CTRef TRef = GetTRef(source, remote);
		//string filePath = FormatA("%s%d%02d%02d\\%s", workingDir, TRef.GetYear(), TRef.GetMonth(), TRef.GetDay(), GetFileName(remote).c_str());
		string filePath = workingDir + SOURCES_NAME[s] + "\\";
		switch (s)
		{
		case N_HRDPS:	break;
		default:
		{
			string dir = WBSF::GetLastDirName(GetPath(remote));
			//std::replace(dir.begin(), dir.end(), '.', '\\');
			size_t pos = dir.find(".");
			string date = dir.substr(pos + 1);
			string fileName = GetFileName(remote);
			filePath += date + "\\" + fileName;
		}

		}

		return  filePath;
	}


	CTRef CUIGribForecast::GetLatestTRef(size_t source, CFtpConnectionPtr& pConnection)const
	{
		ERMsg msg;


		string URL = SERVER_PATH[source];
		switch (source)
		{
		case N_HRRR:	URL += "conus/hrrr.*"; break;
		case N_HRRR_SRF:URL += "conus/hrrr.*"; break;
		case N_RAP_P:	URL += "rap.*"; break;
		case N_RAP_B:	URL += "rap.*"; break;
		case N_NAM:		URL += "nam.*"; break;
		default: ASSERT(false);
		}


		CFileInfoVector fileListTmp;
		UtilWWW::FindDirectories(pConnection, URL, fileListTmp);

		//CTRef TRef;

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
				case N_HRRR:	URL += FormatA("hrrr.%4d%02d%02d/conus/hrrr.t??z.wrfnatf00.grib2", TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1); break;
				case N_HRRR_SRF:URL += FormatA("hrrr.%4d%02d%02d/conus/hrrr.t??z.wrfsfcf00.grib2", TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1); break;
				case N_RAP_P:	URL += FormatA("rap.%4d%02d%02d/rap.t??z.awp130pgrbf00.grib2", TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1); break;
				case N_RAP_B:	URL += FormatA("rap.%4d%02d%02d/rap.t??z.awp130bgrbf00.grib2", TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1); break;
				case N_NAM:		URL += FormatA("nam.%4d%02d%02d/nam.t??z.awphys00.tm00.grib2", TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1); break;
				default: ASSERT(false);
				}



				CFileInfoVector fileListTmp;
				if (FindFiles(pConnection, URL, fileListTmp))
				{
					for (size_t i = 0; i < fileListTmp.size(); i++)
					{
						string name = GetFileTitle(fileListTmp[i].m_filePath);
						size_t pos = name.find('.');
						if (pos != NOT_INIT)
						{
							name = name.substr(pos + 1);
							size_t h = WBSF::as<size_t>(name.substr(1, 2));

							latest2.insert(TRef + h);
						}
					}
				}
			}
		}


		CTRef TRef;
		if (!latest2.empty())
			TRef = *latest2.rbegin();;



		return TRef;
	}


	ERMsg CUIGribForecast::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		return msg;
	}

	ERMsg CUIGribForecast::GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;
		return msg;
	}

	ERMsg CUIGribForecast::GetGribsList(CTPeriod p, std::map<CTRef, std::string>& gribsList, CCallback& callback)
	{
		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);


		size_t source = as<size_t>(SOURCES);
		if (source == N_HRDPS)
		{
			CHRDPS HRDPS(workingDir + SOURCES_NAME[N_HRDPS] + "\\");
			HRDPS.m_bForecast = true;
			HRDPS.m_variables = Get(HRDPS_VARS);
			HRDPS.m_max_hours = as<__int32>(MAX_HOUR);
			msg = HRDPS.GetGribsList(p, gribsList, callback);
		}
		else
		{

			int firstYear = p.Begin().GetYear();
			int lastYear = p.End().GetYear();
			size_t nbYears = lastYear - firstYear + 1;
			//CTRef today = CTRef::GetCurrentTRef();

			for (size_t y = 0; y < nbYears; y++)
			{
				int year = firstYear + int(y);


				string path = workingDir + string(SOURCES_NAME[source]) + "\\";
				StringVector list = WBSF::GetDirectoriesList(path + "*");

				for (size_t i = 0; i < list.size(); i++)
				{


					if (list[i].length() == 8)
					{
						string filter = path + list[i] + "\\*.grib2";
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
		}

		return msg;
	}

	CTRef CUIGribForecast::GetTRef(size_t source, string filePath)
	{
		CTRef TRef;
		ASSERT(source != N_HRDPS);

		static const size_t POS_HOUR1[NB_SOURCES] = { 0, 6, 6, 5, 5, 5 };
		static const size_t POS_HOUR2[NB_SOURCES] = { 0, 17, 17, 20, 20, 15 };

		string name = GetFileTitle(filePath);
		string dir1 = WBSF::GetLastDirName(GetPath(filePath));

		int year = WBSF::as<int>(dir1.substr(0, 4));
		size_t m = WBSF::as<int>(dir1.substr(4, 2)) - 1;
		size_t d = WBSF::as<int>(dir1.substr(6, 2)) - 1;
		size_t h = WBSF::as<int>(name.substr(POS_HOUR1[source], 2));
		size_t hh = WBSF::as<int>(name.substr(POS_HOUR2[source], 2));

		TRef = CTRef(year, m, d, h) + hh;

		return TRef;
	}

	size_t CUIGribForecast::GetHH(size_t source, string filePath)
	{
		static const size_t HH_POS[NB_SOURCES] = { 0, 17, 17, 21, 16 };
		string name = GetFileTitle(filePath);
		ASSERT(name.size() >= HH_POS[source] + 2);

		string HHstr = name.substr(HH_POS[source], 2);
		ASSERT(HHstr.size() == 2);
		size_t HH = WBSF::as<size_t>(HHstr);

		return HH;
	}
}