#include "StdAfx.h"
#include "HRRR.h"
#include "Basic/WeatherStation.h"
#include "Geomatic/ShapeFileBase.h"
#include "TaskFactory.h"
#include "Geomatic/TimeZones.h"
//#include "cctz\time_zone.h"

#include "WeatherBasedSimulationString.h"
#include "../Resource.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;

namespace WBSF
{

	//*********************************************************************
	const char* CHRRR::SERVER_NAME[NB_SERVER_TYPE] = {"nomads.ncep.noaa.gov", "ftp.ncep.noaa.gov"};
	const char* CHRRR::SERVER_PATH[NB_SERVER_TYPE] = { "/pub/data/nccf/com/hrrr/prod/", "/pub/data/nccf/com/hrrr/prod/" };
	const char* CHRRR::NAME[NB_SOURCES] = {"nat", "sfc"};
	const double CHRRR::MINIMUM_SIZE[NB_SOURCES] = { 550, 90};

	CHRRR::CHRRR(const std::string& workingDir):
		m_workingDir(workingDir),
		m_source(HRRR_3D),
		m_serverType(HTTP_SERVER),
		m_bShowWINSCP(false)
	{}
	
	CHRRR::~CHRRR(void)
	{}



	//****************************************************************************************************
	ERMsg CHRRR::Execute(CCallback& callback)
	{
		ERMsg msg;

		switch(m_serverType)
		{
			case HTTP_SERVER:msg=ExecuteHTTP(callback); break;
			case FTP_SERVER:msg = ExecuteFTP(callback); break;
			default:ASSERT(false);
		}

		return msg;
	}

	ERMsg CHRRR::ExecuteFTP(CCallback& callback)
	{
		ERMsg msg;

		int nbDownloaded = 0;

		
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(SERVER_NAME[FTP_SERVER], 1);
		callback.AddMessage("");


		CFileInfoVector fileList;
		msg = GetFilesToDownload(fileList, callback);
	
		if (msg)
		{

			size_t nbFileToDownload = fileList.size();

			string scriptFilePath = m_workingDir + "script.txt";
			WBSF::RemoveFile(scriptFilePath + ".log");


			callback.PushTask("Download HRRR gribs from FTP: " + ToString(fileList.size()) + " files", fileList.size());
			callback.AddMessage("Number of HRRR files to download gribs from FTP: " + ToString(fileList.size()) + " files");
			for (size_t i = 0; i < fileList.size() && msg; i++)
			{
				ofStream stript;
				msg = stript.open(scriptFilePath);
				if (msg)
				{

					string outputFilePath = GetOutputFilePath(fileList[i].m_filePath);
					string tmpFilePaht = GetPath(outputFilePath) + GetFileName(fileList[i].m_filePath);
					CreateMultipleDir(GetPath(outputFilePath));

					stript << "open ftp://anonymous:anonymous%40example.com@" << SERVER_NAME[FTP_SERVER] << endl;
					stript << "cd " << GetPath(fileList[i].m_filePath) << endl;
					stript << "lcd " << GetPath(tmpFilePaht) << endl;
					stript << "get " << GetFileName(tmpFilePaht) << endl;
					stript << "exit" << endl;
					stript.close();

					
					//# Execute the script using a command like:
					string command = "\"" + GetApplicationPath() + "External\\WinSCP.exe\" " + string(m_bShowWINSCP ? "/console " : "") + "-timeout=300 -passive=on /log=\"" + scriptFilePath + ".log\" /ini=nul /script=\"" + scriptFilePath;
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
									msg = RenameFile(tmpFilePaht, outputFilePath);
								}
								else
								{
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
			}

			callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbDownloaded));
			callback.PopTask();
		}

		return msg;
	}
	
	ERMsg CHRRR::GetFilesToDownload(CFileInfoVector& fileList, CCallback& callback)
	{
		ERMsg msg;


		CInternetSessionPtr pSession;
		CFtpConnectionPtr pConnection;

		msg = GetFtpConnection(SERVER_NAME[FTP_SERVER], pConnection, pSession);
		if (!msg)
			return msg;

		callback.PushTask(string("Get files list from: ") + SERVER_PATH[FTP_SERVER], 2);

		CFileInfoVector dir;
		msg = FindDirectories(pConnection, SERVER_PATH[FTP_SERVER], dir);
		for (CFileInfoVector::const_iterator it1 = dir.begin(); it1 != dir.end() && msg; it1++)
		{
			CFileInfoVector fileListTmp;
			msg = FindFiles(pConnection, it1->m_filePath + "hrrr.t*z.wrf" + NAME[m_source] + "f00.grib2", fileListTmp);

			for (CFileInfoVector::iterator it = fileListTmp.begin(); it != fileListTmp.end() && msg; it++)
			{
				string outputFilePath = GetOutputFilePath(it->m_filePath);
				CFileInfo info;


				if (!WBSF::GetFileInfo(outputFilePath, info))
				{
					fileList.push_back(*it);
				}
				else
				{

					if (info.m_size < MINIMUM_SIZE[m_source] * 1024 * 1024)
					{
						fileList.push_back(*it);
					}
				}
			}

			msg += callback.StepIt();
		}


		pConnection->Close();
		pSession->Close();

		callback.PopTask();

		return msg;
	}

	//ERMsg CHRRR::ExecuteFTP(CCallback& callback)
	//{
	//	ERMsg msg;

	//	callback.AddMessage(GetString(IDS_UPDATE_DIR));
	//	callback.AddMessage(m_workingDir, 1);
	//	callback.AddMessage(GetString(IDS_UPDATE_FROM));
	//	callback.AddMessage(SERVER_NAME[FTP_SERVER], 1);
	//	callback.AddMessage("");

	//	CInternetSessionPtr pSession;
	//	CFtpConnectionPtr pConnection;

	//	msg = GetFtpConnection(SERVER_NAME[FTP_SERVER], pConnection, pSession);
	//	if (!msg)
	//		return msg;



	//	callback.PushTask(string("Get files list from: ") + SERVER_PATH[FTP_SERVER], 2);
	//	CFileInfoVector fileList;


	//	CFileInfoVector dir;
	//	msg = FindDirectories(pConnection, SERVER_PATH[FTP_SERVER], dir);
	//	for (CFileInfoVector::const_iterator it1 = dir.begin(); it1 != dir.end() && msg; it1++)
	//	{
	//		CFileInfoVector fileListTmp;
	//		msg = FindFiles(pConnection, it1->m_filePath + "hrrr.t*z.wrf" + NAME[m_source] + "f00.grib2", fileListTmp);

	//		for (CFileInfoVector::iterator it = fileListTmp.begin(); it != fileListTmp.end() && msg; it++)
	//		{
	//			string outputFilePath = GetOutputFilePath(it->m_filePath);
	//			CFileInfo info;


	//			if (!WBSF::GetFileInfo(outputFilePath, info))
	//			{
	//				fileList.push_back(*it);
	//			}
	//			else
	//			{
	//				
	//				if (info.m_size < MINIMUM_SIZE[m_source] * 1024 * 1024)
	//				{
	//					fileList.push_back(*it);
	//				}
	//			}
	//		}

	//		msg += callback.StepIt();
	//	}


	//	callback.PopTask();


	//	callback.AddMessage("Number of HRRR gribs to download: " + ToString(fileList.size()));
	//	callback.PushTask("Download HRRR gribs (" + ToString(fileList.size()) + ")", fileList.size());

	//	int nbDownload = 0;
	//	for (CFileInfoVector::iterator it = fileList.begin(); it != fileList.end() && msg; it++)
	//	{
	//		//string fileName = GetFileName(it->m_filePath);
	//		string outputFilePath = GetOutputFilePath(it->m_filePath);

	//		callback.PushTask("Download HRRR gribs:" + outputFilePath, NOT_INIT);

	//		CreateMultipleDir(GetPath(outputFilePath));
	//		msg = CopyFile(pConnection, it->m_filePath, outputFilePath, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE);
	//		if (msg && FileExists(outputFilePath))
	//		{
	//			nbDownload++;

	//			//now copy index file
	//			msg = CopyFile(pConnection, it->m_filePath + ".idx", outputFilePath + ".idx", INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE);
	//			//if (!FileExists(outputFilePath + ".idx"))
	//			//{
	//			//	//if .idx does not exist
	//			//	msg += RemoveFile(outputFilePath);
	//			//}
	//		}

	//		callback.PopTask();
	//		msg += callback.StepIt();
	//	}

	//	pConnection->Close();
	//	pSession->Close();


	//	callback.AddMessage("Number of HRRR gribs downloaded: " + ToString(nbDownload));
	//	callback.PopTask();


	//	return msg;
	//}

	//****************************************************************************************************

	ERMsg CHRRR::ExecuteHTTP(CCallback& callback)
	{
		ERMsg msg;

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(m_workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(SERVER_NAME[HTTP_SERVER], 1);
		callback.AddMessage("");

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg = GetHttpConnection(SERVER_NAME[HTTP_SERVER], pConnection, pSession);
		if (!msg)
			return msg;



		callback.PushTask(string("Get files list from: ") + SERVER_PATH[HTTP_SERVER], 2);
		CFileInfoVector fileList;


		CFileInfoVector dir;
		msg = FindDirectories(pConnection, SERVER_PATH[HTTP_SERVER], dir);
		for (CFileInfoVector::const_iterator it1 = dir.begin(); it1 != dir.end() && msg; it1++)
		{
			CFileInfoVector fileListTmp;
			msg = FindFiles(pConnection, it1->m_filePath + "hrrr.t*z.wrf" + NAME[m_source] + "f00.grib2", fileListTmp);

			for (CFileInfoVector::iterator it = fileListTmp.begin(); it != fileListTmp.end() && msg; it++)
			{
				string outputFilePath = GetOutputFilePath(it->m_filePath);
				CFileInfo info;


				if (!WBSF::GetFileInfo(outputFilePath, info))
				{
					fileList.push_back(*it);
				}
				else
				{
					if (info.m_size < MINIMUM_SIZE[m_source] * 1024 * 1024 )
					{
						fileList.push_back(*it);
					}
				}
			}

			msg += callback.StepIt();
		}


		callback.PopTask();


		
		callback.PushTask("Download HRRR gribs (" + ToString(fileList.size()) + ")", fileList.size());
		callback.AddMessage("Number of HRRR gribs to download from HTTP: " + ToString(fileList.size()));

		int nbDownload = 0;
		for (CFileInfoVector::iterator it = fileList.begin(); it != fileList.end() && msg; it++)
		{
			//string fileName = GetFileName(it->m_filePath);
			string outputFilePath = GetOutputFilePath(it->m_filePath);
			
			callback.PushTask("Download HRRR gribs:" + outputFilePath, NOT_INIT);

			CreateMultipleDir(GetPath(outputFilePath));
			msg = CopyFile(pConnection, it->m_filePath, outputFilePath, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE);
			if (msg && FileExists(outputFilePath))
			{
				nbDownload++;
				
				//now copy index file
				msg = CopyFile(pConnection, it->m_filePath + ".idx", outputFilePath + ".idx", INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE);
				//if (!FileExists(outputFilePath + ".idx"))
				//{
				//	//if .idx does not exist
				//	msg += RemoveFile(outputFilePath);
				//}
			}

			callback.PopTask();
			msg += callback.StepIt();
		}

		pConnection->Close();
		pSession->Close();


		callback.AddMessage("Number of HRRR gribs downloaded: " + ToString(nbDownload));
		callback.PopTask();


		return msg;
	}

	//****************************************************************************************************

	ERMsg CHRRR::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		stationList.clear();
		//msg.ajoute("Can't extract station from grid");
		//msg.ajoute("Can be used only as forecast extraction");

		return msg;
	}

	CTRef CHRRR::GetTRef(const string& filePath)const
	{

		string dir = GetLastDirName(GetPath(filePath));
		string fileName = GetFileName(filePath);

		CTRef TRef;
		//hrrr.t07z.wrfnatf04.grib2 
		StringVector tmp(fileName, ".");
		ASSERT(tmp.size() == 4);
		if (dir.length() == 13 && tmp.size() == 4)
		{
			int year = WBSF::as<int>(dir.substr(5, 4));
			size_t m = WBSF::as<size_t >(dir.substr(9, 2)) - 1;
			size_t d = WBSF::as<size_t >(dir.substr(11, 2)) - 1;
			size_t h = WBSF::as<size_t >(tmp[1].substr(1, 2));
			TRef = CTRef(year, m, d, h);
		}

		return TRef;

	}

	string CHRRR::GetOutputFilePath(const string& filePath)const
	{
		CTRef TRef = GetTRef(filePath);
		string fileName = GetFileName(filePath);
		return FormatA("%s%d\\%02d\\%02d\\%s", m_workingDir.c_str(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, fileName.c_str());
	}


	//ERMsg CHRRR::OpenDatasets(CCallback& callback)
	//{
	//	ERMsg msg;
	//	StringVector filesList = GetFilesList(GetOutputFilePath("*.grib2"));

	//	callback.PushTask("load HRRR gribs files (" + ToString(filesList.size()) + ")", filesList.size());
	//	for (StringVector::const_iterator it = filesList.begin(); it != filesList.end() && msg; it++)
	//	{
	//			//size_t hhh = Gethhh(*it); ASSERT(hhh <= 52);
	//			//size_t vv = GetVariable(*it);
	//			//ASSERT(vv != NOT_INIT);

	//		//msg += m_datasets[hhh][vv].OpenInputImage(*it);
	//		msg += callback.StepIt();
	//		
	//	}

	//	//msg += m_datasets[0][0].OpenInputImage("E:/Travaux/Install/DemoBioSIM/Update/EnvCan/Forecast/HRDPS/CMC_hrdps_continental_DPT_TGL_2_ps2.5km_2016050806_P000-00.grib2");
	//	callback.PopTask();

	//	if (msg)
	//	{
	//		m_geo2gribs.Set(PRJ_WGS_84, m_datasets[0][0].GetPrjID());
	//		msg += m_geo2gribs.Create();
	//	}
	//	
	//	return msg;
	//}
	//Extraction section

	ERMsg CHRRR::GetWeatherStation(const std::string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		//if (ID.empty())
		//{
		//ASSERT(!station.m_ID.empty());
		//ASSERT(station.m_lat != -999);
		//ASSERT(station.m_lon != -999);

		//int year = WBSF::GetCurrentYear();
		//

		////no forecast are added on old data
		//if (station.IsYearInit(year))
		//{
		//	if (!m_datasets[0][0].IsOpen())
		//		msg = OpenDatasets(callback);

		//	if (msg)
		//	{
		//		string name = GetFileName(m_datasets[0][0].GetFilePath());
		//		CTRef UTCRef = GetTRef(name);
		//		cctz::time_zone zone;
		//		if (CTimeZones::GetZone(station, zone))
		//		{
		//			CGeoPoint3D pt(station);
		//			pt.Reproject(m_geo2gribs);

		//			const CGeoExtents& extents = m_datasets[0][0].GetExtents();
		//			CGeoRect rect(-180, 0, 0, 90, PRJ_WGS_84);
		//			
		//			//rect = CGeoRect(-141, 42, 0, 90, PRJ_WGS_84);//HRDPS have stange values outside canada

		//			if (extents.IsInside(pt) && rect.IsInside(station))
		//			{
		//				CGeoPointIndex xy = extents.CoordToXYPos(pt);

		//				CWeatherAccumulator accumulator(TM);
		//				for (size_t h = 0; h < 24&&msg; h++)
		//				{
		//					CTRef TRef = CTimeZones::UTCTRef2LocalTRef(UTCRef + h, zone);
		//					if (accumulator.TRefIsChanging(TRef))
		//					{
		//						if (station[accumulator.GetTRef()].GetVariables().none() )//don't override observation
		//							station[accumulator.GetTRef()].SetData(accumulator);
		//					}
		//					//station[TRef + hh].SetStat(v, value);
		//					float Tair = -999;
		//					float Tdew = -999;
		//					for (size_t vv = 0; vv < NB_FORECAST_VAR&&msg; vv++)
		//					{
		//						if (m_datasets[h][vv].IsOpen())
		//						{
		//							TVarH v = FORECAST_VARIABLES[vv];
		//							float value = m_datasets[h][vv].ReadPixel(0, xy);
		//							
		//							if (v == H_PRCP)
		//								value *= 3600;//mm/s --> mm for one hours

		//							if (v == H_SRAD2)
		//								value *= 3600;//J/m² --> W/m²

		//							if (v == H_PRES)
		//								value /= 100;//Pa --> hPa

		//							if (v == H_WNDS)
		//								value *= 3600/1000;//Pa --> hPa


		//							if (v == H_TAIR2)
		//								Tair = value;

		//							if (v == H_TDEW)
		//								Tdew = value;
		//							//for (size_t hh = 0; hh < delta_h; hh++)

		//							accumulator.Add(TRef, v, value);
		//						}
		//					}

		//					if (Tdew>-999 && Tair>-999)
		//						accumulator.Add(TRef, H_RELH, WBSF::Td2Hr(Tair, Tdew));
		//				}

		//				if (accumulator.GetTRef().IsInit())
		//				{
		//					if (station[accumulator.GetTRef()].GetVariables().none())//don't override observation
		//						station[accumulator.GetTRef()].SetData(accumulator);
		//				}
		//			}//if is inside
		//		}//if is in zone
		//	}//if msg
		//}//if there is data for the current year
		////}


		return msg;
	}

}