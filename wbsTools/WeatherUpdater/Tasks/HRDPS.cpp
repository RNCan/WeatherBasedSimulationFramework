#include "StdAfx.h"
#include "HRDPS.h"
#include "Basic/WeatherStation.h"
#include "Geomatic/ShapeFileBase.h"
#include "TaskFactory.h"
#include "Geomatic/TimeZones.h"
#include "cctz\time_zone.h"

#include "WeatherBasedSimulationString.h"
#include "../Resource.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;

namespace WBSF
{

	//*********************************************************************
	const char* CHRDPS::SERVER_NAME = "dd.weather.gc.ca";
	const char* CHRDPS::SERVER_PATH = "/model_hrdps/continental/grib2/";
	//const char* CHRDPS::FORECAST_VAR_NAME[NB_FORECAST_VAR] = { "PRES_SFC_0", "TMP_TGL_2", "DPT_TGL_2", "PRATE_SFC_0", "WIND_TGL_10", "WDIR_TGL_10", "DLWRF_SFC_0" };
	//const TVarH CHRDPS::FORECAST_VARIABLES[NB_FORECAST_VAR] = {H_PRES, H_TAIR2, H_TDEW, H_PRCP, H_WNDS, H_WNDD, H_SRAD2};
	//TSOIL_SFC_0 : soil temperaturte
	//TSOIL_DBLL_100 : deep soil temperature

	//WTMP_SFC_0   water temperture
	//RH_SFC_0		relative humidity 
	
	static const size_t MAX_FORECAST_HOURS = 48;


	CHRDPS::CHRDPS(const std::string& workingDir):
		m_workingDir(workingDir)
	{}
	
	CHRDPS::~CHRDPS(void)
	{}


	
//*********************************************************************
	
	string CHRDPS::GetRemoteFilePath(size_t HH, size_t hhh, const string& fileName)const
	{
		string str_hhh = FormatA("%03d", hhh);
		string str_HH = FormatA("%02d", HH);

		

		return  SERVER_PATH + str_HH + "/" + str_hhh + "/" + fileName;
	}

	string CHRDPS::GetOutputFilePath(const string& fileName)const
	{
		CTRef TRef = CHRDPS::GetTRef(fileName);
		return FormatA("%s%d\\%02d\\%02d\\%02d\\%s", m_workingDir.c_str(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, TRef.GetHour(), fileName.c_str());
	}

	size_t CHRDPS::GetVariable(const string& fileName)const
	{
		size_t  vv = NOT_INIT;
		/*for (size_t v = 0; v < NB_FORECAST_VAR&&vv == NOT_INIT; v++)
		{
			if (Find(fileName, FORECAST_VAR_NAME[v]))
				vv = v;
		}*/

		return vv;
	}

	size_t CHRDPS::GetLatestHH(CHttpConnectionPtr& pConnection)const
	{
		ERMsg msg;

		size_t HH = NOT_INIT;

		vector<pair<CTRef, size_t>> latest;
		for (size_t h = 0; h < 24; h += 6)
		{
			string remotePath = GetRemoteFilePath(h, 0, "*P000-00.grib2");

			CFileInfoVector fileListTmp;
			if (FindFiles(pConnection, remotePath, fileListTmp) && !fileListTmp.empty())
			{
				string fileTitle = GetFileName(fileListTmp.front().m_filePath);
				latest.push_back(make_pair(GetTRef(fileTitle), h));
			}
		}

		sort(latest.begin(), latest.end());
			
		if (!latest.empty())
			HH = latest.back().second;


		return HH;
	}

	//****************************************************************************************************
	ERMsg CHRDPS::Execute( CCallback& callback)
	{
		ERMsg msg;
		//string outputPath = GetDir(WORKING_DIR);

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(m_workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(SERVER_NAME, 1);
		callback.AddMessage("");


		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg = GetHttpConnection(SERVER_NAME, pConnection, pSession);
		if (!msg)
			return msg;



		callback.PushTask(string("Get files list from: ") + SERVER_PATH, 6 * 4);
		CFileInfoVector fileList;


		CFileInfoVector dir1;
		msg = FindDirectories(pConnection, SERVER_PATH, dir1);// 00, 06, 12, 18
		for (CFileInfoVector::const_iterator it1 = dir1.begin(); it1 != dir1.end() && msg; it1++)
		{
			CFileInfoVector dir2;
			msg = FindDirectories(pConnection, it1->m_filePath, dir2);//000 to ~048

			for (CFileInfoVector::const_iterator it2 = dir2.begin(); it2 != dir2.end() && msg; it2++)
			{
				size_t hhh = as<size_t>(GetLastDirName(it2->m_filePath));
				//size_t hhh = Gethhh(fileName);
				if (hhh < 6)
				{
					CFileInfoVector fileListTmp;
					msg = FindFiles(pConnection, it2->m_filePath + "*.grib2", fileListTmp);
					for (CFileInfoVector::iterator it = fileListTmp.begin(); it != fileListTmp.end() && msg; it++)
					{
						string fileName = GetFileName(it->m_filePath);
						string outputFilePath = GetOutputFilePath(fileName);
						//CFileInfo info;
						
						ifStream stream;
						if (!stream.open(outputFilePath))
						{
							fileList.push_back(*it);
						}
						else
						{
							//verify if the file finish with 7777
							char test[5] = { 0 };
							stream.seekg(-4, ifstream::end);
							stream.read(&(test[0]), 4);

							if (string(test) != "7777")
							{
								fileList.push_back(*it);
							}
						}
							
					}

					msg += callback.StepIt();
				}
			}
		}
	

		callback.PopTask();


		callback.AddMessage("Number of HRDPS gribs to download: " + ToString(fileList.size()));
		callback.PushTask("Download HRDPS gribs (" + ToString(fileList.size()) + ")", fileList.size());

		int nbDownload = 0;
		for (CFileInfoVector::iterator it = fileList.begin(); it != fileList.end() && msg; it++)
		{
			string fileName = GetFileName(it->m_filePath);
			string outputFilePath = GetOutputFilePath(fileName);

			CreateMultipleDir(GetPath(outputFilePath));
			msg = CopyFile(pConnection, it->m_filePath, outputFilePath, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE);
			if (msg)
				nbDownload++;

			msg += callback.StepIt();
		}

		pConnection->Close();
		pSession->Close();


		callback.AddMessage("Number of HRDPS gribs downloaded: " + ToString(nbDownload));
		callback.PopTask();


		return msg;
	}

	//****************************************************************************************************

	//ERMsg CHRDPS::GetStationList(StringVector& stationList, CCallback& callback)
	ERMsg CHRDPS::GetStationList(CLocationVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		stationList.clear();
		//msg.ajoute("Can't extract station from grid");
		//msg.ajoute("Can be used only as forecast extraction");

		return msg;
	}

	CTRef CHRDPS::GetTRef(const string& fileName)const
	{
		CTRef TRef;
		StringVector tmp(fileName, "_");
		ASSERT(tmp.size() == 9 || tmp.size() == 8);
		if (tmp.size() == 9 || tmp.size() == 8)
		{
			size_t i = 7;
			ASSERT(tmp[i].length() == 10);
			int year = WBSF::as<int>(tmp[i].substr(0, 4));
			size_t m = WBSF::as<size_t >(tmp[i].substr(4, 2)) - 1;
			size_t d = WBSF::as<size_t >(tmp[i].substr(6, 2)) - 1;
			size_t h = WBSF::as<size_t >(tmp[i].substr(8, 2));
			TRef = CTRef(year, m, d, h);
		}

		return TRef;

	}

	size_t CHRDPS::GetHH(const string& fileName)const
	{
		CTRef CTRef = GetTRef(fileName);
		return CTRef.GetHour();
	}

	size_t CHRDPS::Gethhh(const string& fileName)const
	{
		size_t hhh = NOT_INIT;
		StringVector tmp(fileName, "_");
		if (tmp.size() == 9)
		{
			size_t i = tmp.size() - 1;
			ASSERT(tmp[i].length() > 4);
			string str_hhh = tmp[i].substr(1, 3);
			hhh = WBSF::as<size_t>(str_hhh);;
		}
		
		return hhh;
	}

	

	ERMsg CHRDPS::OpenDatasets(CCallback& callback)
	{
		ERMsg msg;
		StringVector filesList = GetFilesList(GetOutputFilePath("*"));//don't put extensionbea cause of the 2.5km trouble

		//callback.PushTask("load gribs files (" + ToString(filesList.size()) + ")", filesList.size());
		//for (StringVector::const_iterator it = filesList.begin(); it != filesList.end() && msg; it++)
		//{
		//	if (IsEqual(GetFileExtension(*it), ".grib2"))
		//	{
		//		size_t hhh = Gethhh(*it); ASSERT(hhh <= 52);
		//		size_t vv = GetVariable(*it);
		//		ASSERT(vv != NOT_INIT);

		//		msg += m_datasets[hhh][vv].OpenInputImage(*it);
		//		msg += callback.StepIt();
		//	}
		//}

		////msg += m_datasets[0][0].OpenInputImage("E:/Travaux/Install/DemoBioSIM/Update/EnvCan/Forecast/HRDPS/CMC_hrdps_continental_DPT_TGL_2_ps2.5km_2016050806_P000-00.grib2");
		//callback.PopTask();

		//if (msg)
		//{
		//	m_geo2gribs.Set(PRJ_WGS_84, m_datasets[0][0].GetPrjID());
		//	msg += m_geo2gribs.Create();
		//}
		//
		return msg;
	}
	//Extraction section

	//ERMsg CHRDPS::GetWeatherStation(const std::string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	ERMsg CHRDPS::GetVirtuelStation(const CLocationVector& stations, CWVariables variables, CTPeriod p, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		//if (ID.empty())
		//{
		ASSERT(!station.m_ID.empty());
		ASSERT(station.m_lat != -999);
		ASSERT(station.m_lon != -999);

		int year = WBSF::GetCurrentYear();
		

		//no forecast are added on old data
		//if (station.IsYearInit(year))
		//{
		//	if (!m_datasets[0][0].IsOpen())
		//		msg = OpenDatasets(callback);

		//	if (msg)
		//	{
		//		string name = GetFileName(m_datasets[0][0].GetFilePath());
		//CTRef UTCRef = GetTRef(name);
		//	ds v v b	cctz::time_zone zone;
		//		if (CTimeZones::GetZone(station, zone))
		//		{
		//			CGeoPoint3D pt(station);
		//			pt.Reproject(m_geo2gribs);

		//			const CGeoExtents& extents = m_datasets[0][0].GetExtents();
		//			CGeoRect rect(-180, 0, 0, 90, PRJ_WGS_84);
		//			if (m_type == GT_HRDPS)
		//				rect = CGeoRect(-141, 42, 0, 90, PRJ_WGS_84);//HRDPS have stange values outside canada

		//			if (extents.IsInside(pt) && rect.IsInside(station))
		//			{
		//				CGeoPointIndex xy = extents.CoordToXYPos(pt);
		//				//size_t type = as<size_t>(TYPE);
		//				size_t delta_h = (m_type == GT_HRDPS) ? 1 : 3;

		//				CWeatherAccumulator accumulator(TM);
		//				for (size_t h = 0; h <= MAX_FORECAST_HOURS&&msg; h += delta_h)
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