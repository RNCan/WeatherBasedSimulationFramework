#include "StdAfx.h"
#include "EnvCanGribForecast.h"
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
	const char* CEnvCanGribForecast::SERVER_NAME = "dd.weather.gc.ca";

	const char* CEnvCanGribForecast::FORECAST_VAR_NAME[NB_FORECAST_VAR] = { "PRES_SFC_0", "TMP_TGL_2", "DPT_TGL_2", "PRATE_SFC_0", "WIND_TGL_10", "WDIR_TGL_10", "DLWRF_SFC_0" };
	const TVarH CEnvCanGribForecast::FORECAST_VARIABLES[NB_FORECAST_VAR] = {H_PRES, H_TAIR, H_TDEW, H_PRCP, H_WNDS, H_WNDD, H_SRAD};
	static const size_t NB_MISS_DAY_TO_IGNORE_FORECAST = 7;

	//TSOIL_SFC_0 : soil temperaturte
	//TSOIL_DBLL_100 : deep soil temperature

	//WTMP_SFC_0   water temperture
	//RH_SFC_0		relative humidity 
	
	static const size_t MAX_FORECAST_HOURS = 48;


	CEnvCanGribForecast::CEnvCanGribForecast(void)
	{}
	
	CEnvCanGribForecast::~CEnvCanGribForecast(void)
	{}


	
//*********************************************************************
	
	//This table provides the variable name, level, abbreviation, units and a link to additional grib2 encoding information for each parameter encoded in GRIB2 format					
	//Number	Variable	Level	Abbreviation	Units	Description
	//0-	Surface Pressure	Surface	PRES_SFC_0	Pascal	Sections 0 to 6
	//1-	Precipitation rate	Surface	PRATE_SFC_0	kg m-2 sec-1	Sections 0 to 6
	//2-	Mean Sea Level Pressure	MSL	PRMSL_MSL_0	Pascal	Sections 0 to 6
	//3-	Temperature	2m above ground	TMP_TGL_2m	Kelvin	Sections 0 to 6
	//4-	Area-averaged Surface Humidity	Surface	SPFH_SFC_0	kg kg-1	Sections 0 to 6
	//5-	Dew Point temperature	Surface	DPT_SFC_0	Kelvin	Sections 0 to 6
	//6-	Accumulated Precipitation	Surface	APCP_SFC_0	kg m-2	Sections 0 to 6
	//7-	Convective precipitation	Surface	ACPCP_SFC_0	kg m-2	Sections 0 to 6
	//8-	Snow Depth	Surface	SNOD_SFC_0	meter	Sections 0 to 6
	//10-	Downward incident solar flux	Surface	DSWRF_SFC_0	Joules m-2	Sections 0 to 6
	//11-	Downward Long Wave Radiative Flux	Surface	DLWRF_SFC_0	Joules m-2	Sections 0 to 6
	//12-	Net Long Wave Radiation at Surface (Accumulated)	Surface	NLWRS_SFC_0	Joules m-2	Sections 0 to 6
	//13-	Net Short Wave Radiation at Surface (Accumulated)	Surface	NSWRS_SFC_0	Joules m-2	Sections 0 to 6
	//22-	Wind speed - Module	10m above ground	WIND_TGL_10m	metres per second	Sections 0 to 6
	//23-	Wind direction	10m above ground	WDIR_TGL_10m	Degrees relative to i,j components of grid	Sections 0 to 6
	//30-	Precipitable water	Column	CWAT_EATM_0	kg m-2	Sections 0 to 6
	//31-	Upward Long Wave Radiation Flux	Nominal top of atmosphere	ULWRF_NTAT_0	W m-2	Sections 0 to 6
	//32-	Albedo	Surface	ALBDO_SFC_0	Percent	Sections 0 to 6
	//34-	Soil temperature near the surface	0-10 cm below ground	TSOIL_SFC_0	Kelvin	Sections 0 to 6
	//35-	Deep soil temperature	10 cm below ground	TSOIL_DBLL_100	Kelvin	Sections 0 to 6
	//36-	Soil moisture	0-10 cm below ground	SOILW_DBLY_10	m3/m3	Sections 0 to 6
	//37-	Model topography - Smoothed	Surface	HGT_SFC_0	m2/s2	Sections 0 to 6
	//38-	Land cover	Surface	LAND_SFC_0	fraction	Sections 0 to 6
	//272-	Water temperature	Surface	WTMP_SFC_0	Kelvin	Sections 0 to 6
	//273-	Accumulated Precipitation type - Snow	Surface	WEASN_SFC_0	meter	Sections 0 to 6
	//274-	Accumulated Precipitation type - Rain	Surface	WEARN_SFC_0	kg m-2	Sections 0 to 6
	//275-	Accumulated Precipitation type - Ice Pellets	Surface	WEAPE_SFC_0	meter	Sections 0 to 6
	//276-	Accumulated Precipitation type - Freezing Rain	Surface	WEAFR_SFC_0	meter	Sections 0 to 6

	//ftp://tgftp.nws.noaa.gov/SL.us008001/ST.opnl/MT.rap_CY.00/RD.20160507/
	//ftp://tgftp.nws.noaa.gov/SL.us008001/ST.opnl/MT.nam_CY.00/RD.20160507/PT.grid_DF.gr2/
	//ftp://ftpprd.ncep.noaa.gov/pub/data/nccf/com/rap/prod/narre.20160507/ensprod/
	//ftp://ftpprd.ncep.noaa.gov/pub/data/nccf/com/nam/prod/nam.20160506/
	string CEnvCanGribForecast::GetRemoteFilePath(size_t HH, size_t hhh, const string& fileName)const
	{
		string str_hhh = FormatA("%03d", hhh);
		string str_HH = FormatA("%02d", HH);

		string base = (m_type == GT_HRDPS) ? "model_hrdps/continental/grib2/" : "model_gem_regional/10km/grib2/";

		return  base + str_HH + "/" + str_hhh + "/" + fileName;
	}

	string CEnvCanGribForecast::GetOutputFilePath(const string& fileName)const
	{
		string tmp = fileName;
		ReplaceString(tmp, "2.5km", "2,5km");
		return m_workingDir + tmp;
	}

	ERMsg CEnvCanGribForecast::RemoveOldForecast(CCallback& callback)
	{
		ERMsg msg;
		StringVector filesList = GetFilesList(GetOutputFilePath("*"));//dont add extention because of the 2.5km

		callback.PushTask("Delete old gribs files", filesList.size());
		for (StringVector::const_iterator it = filesList.begin(); it != filesList.end()&&msg; it++)
		{
			if (IsEqual(GetFileExtension(*it), ".grib2"))
			{
				ERMsg msgTemp = RemoveFile(*it);
				if (!msg)
					callback.AddMessage(msgTemp);

				msg += callback.StepIt();
			}
		}

		callback.PopTask();

		return msg;
	}
	
	size_t CEnvCanGribForecast::GetVariable(const string& fileName)const
	{
		size_t  vv = NOT_INIT;
		for (size_t v = 0; v < NB_FORECAST_VAR&&vv == NOT_INIT; v++)
		{
			if (Find(fileName, FORECAST_VAR_NAME[v]))
				vv = v;
		}

		return vv;
	}

	size_t CEnvCanGribForecast::GetLatestHH(CHttpConnectionPtr& pConnection)const
	{
		ERMsg msg;

		size_t HH = NOT_INIT;

		vector<pair<CTRef, size_t>> latest;
		for (size_t h = 0; h < 24; h += 6)
		{
			string remotePath = GetRemoteFilePath(h, 0, (m_type == GT_HRDPS) ? "*P000-00.grib2":"*P000.grib2");

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
	ERMsg CEnvCanGribForecast::Execute(CCallback& callback)
	{
		ERMsg msg;

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(m_workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(SERVER_NAME, 1);
		callback.AddMessage("");

		//delete old files
		msg = RemoveOldForecast(callback);
		if (!msg)
			return msg;

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg = GetHttpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);
		if (!msg)
			return msg;
		

		size_t delta_h = m_type == GT_HRDPS ? 1 : 3;
		callback.PushTask("Download gribs list", (MAX_FORECAST_HOURS / delta_h) );


		StringVector fileList;
		//for (size_t HH = 0; HH < 24 && msg; HH+=6)
		size_t HH = GetLatestHH(pConnection);
		if (HH!=NOT_INIT)
		{
			for (size_t hhh = 0; hhh <= MAX_FORECAST_HOURS && msg; hhh += delta_h)
			{
				string remotePath = CEnvCanGribForecast::GetRemoteFilePath(HH, hhh, "*.grib2");

				CFileInfoVector fileListTmp;
				msg = FindFiles(pConnection, remotePath, fileListTmp);

				//keep only some variables
				for (CFileInfoVector::iterator it = fileListTmp.begin(); it != fileListTmp.end(); it++)
				{
					string fileName = GetFileName(it->m_filePath);
					size_t hhh = Gethhh(fileName);
					size_t vv = GetVariable(fileName);
					if (hhh <= 48 && vv != NOT_INIT)
						fileList.push_back(it->m_filePath);
				}

				msg += callback.StepIt();
			}
		}
		
		callback.PopTask();


		callback.AddMessage("Number of gribs to download: " + ToString(fileList.size()));
		callback.PushTask("Download gribs (" + ToString(fileList.size()) +")", fileList.size());

		int nbDownload = 0;
		for (size_t i = 0; i < fileList.size() && msg; i++)
		{
			string outputFilePath = GetOutputFilePath(GetFileName(fileList[i]));

			CreateMultipleDir(GetPath(outputFilePath));
			msg = CopyFile(pConnection, fileList[i], outputFilePath.c_str(), INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE);
			if (msg)
				nbDownload++;

			msg += callback.StepIt();
		}

		pConnection->Close();
		pSession->Close();


		callback.AddMessage("Number of gribs downloaded: " + ToString(nbDownload));
		callback.PopTask();


		return msg;
	}

	//****************************************************************************************************

	ERMsg CEnvCanGribForecast::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		stationList.clear();
		//msg.ajoute("Can't extract station from grid");
		//msg.ajoute("Can be used only as forecast extraction");

		return msg;
	}

	CTRef CEnvCanGribForecast::GetTRef(const string& fileName)const
	{
		CTRef TRef;
		StringVector tmp(fileName, "_");
		ASSERT(tmp.size() == 9 || tmp.size() == 8);
		if (tmp.size() == 9 || tmp.size() == 8)
		{
			size_t i = (m_type == GT_HRDPS) ? 7 : 6;
			ASSERT(tmp[i].length() == 10);
			int year = WBSF::as<int>(tmp[i].substr(0, 4));
			size_t m = WBSF::as<size_t >(tmp[i].substr(4, 2)) - 1;
			size_t d = WBSF::as<size_t >(tmp[i].substr(6, 2)) - 1;
			size_t h = WBSF::as<size_t >(tmp[i].substr(8, 2));
			TRef = CTRef(year, m, d, h);
		}

		return TRef;
	}

	size_t CEnvCanGribForecast::GetHH(const string& fileName)const
	{
		CTRef CTRef = GetTRef(fileName);
		return CTRef.GetHour();
	}

	size_t CEnvCanGribForecast::Gethhh(const string& fileName)const
	{
		StringVector tmp(fileName, "_");
		ASSERT(tmp.size() == 9 || tmp.size() == 8);

		size_t i = tmp.size()-1;// (m_type == GT_HRDPS) ? 8 : 7;
		ASSERT(tmp[i].length() > 4);
		string str_hhh = tmp[i].substr(1, 3);

		return WBSF::as<size_t>(str_hhh);
	}

	ERMsg CEnvCanGribForecast::OpenDatasets(CCallback& callback)
	{
		ERMsg msg;
		StringVector filesList = GetFilesList(GetOutputFilePath("*"));//don't put extensionbea cause of the 2.5km trouble

		callback.PushTask("load gribs files (" + ToString(filesList.size()) + ")", filesList.size());
		for (StringVector::const_iterator it = filesList.begin(); it != filesList.end() && msg; it++)
		{
			if (IsEqual(GetFileExtension(*it), ".grib2"))
			{
				size_t hhh = Gethhh(*it); ASSERT(hhh <= 52);
				size_t vv = GetVariable(*it);
				ASSERT(vv != NOT_INIT);

				msg += m_datasets[hhh][vv].OpenInputImage(*it);
				msg += callback.StepIt();
			}
		}

		//msg += m_datasets[0][0].OpenInputImage("E:/Travaux/Install/DemoBioSIM/Update/EnvCan/Forecast/HRDPS/CMC_hrdps_continental_DPT_TGL_2_ps2.5km_2016050806_P000-00.grib2");
		callback.PopTask();

		if (msg)
		{
			m_geo2gribs.Set(PRJ_WGS_84, m_datasets[0][0].GetPrjID());
			msg += m_geo2gribs.Create();
		}
		
		return msg;
	}
	//Extraction section

	ERMsg CEnvCanGribForecast::GetWeatherStation(const std::string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		//if (ID.empty())
		//{
		ASSERT(!station.m_ID.empty());
		ASSERT(station.m_lat != -999);
		ASSERT(station.m_lon != -999);

		//int year = WBSF::GetCurrentYear();
		

		//no forecast are added on old data
//		if (station.IsYearInit(year))
		CTRef current = CTRef::GetCurrentTRef(TM);
		station.GetStat(H_TAIR);//force to compute stat before call GetVariablesCount
		CWVariablesCounter counter = station.GetVariablesCount();
		CTRef TRefEnd = counter.GetTPeriod().End();
		ASSERT(TRefEnd <= current);

		//station must have data in the last 2 weeks
		if (current.as(CTM::DAILY) - TRefEnd.as(CTM::DAILY) < 	NB_MISS_DAY_TO_IGNORE_FORECAST )
		{
			array<bool, NB_VAR_H> bAddForecast;
			for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)
				bAddForecast[v] = current.as(CTM::DAILY) - counter[v].second.End().as(CTM::DAILY) < NB_MISS_DAY_TO_IGNORE_FORECAST;


			if (!m_datasets[0][0].IsOpen())
				msg = OpenDatasets(callback);

			if (msg)
			{
				string name = GetFileName(m_datasets[0][0].GetFilePath());
				CTRef UTCRef = GetTRef(name);
				//cctz::time_zone zone;
				//__int64 zone = CTimeZones::GetTimeZone(station);
				
				{
					CGeoPoint3D pt(station);
					pt.Reproject(m_geo2gribs);

					const CGeoExtents& extents = m_datasets[0][0].GetExtents();
					CGeoRect rect(-180, 0, 0, 90, PRJ_WGS_84);
					if (m_type == GT_HRDPS)
						rect = CGeoRect(-141, 42, 0, 90, PRJ_WGS_84);//HRDPS have stange values outside canada

					if (extents.IsInside(pt) && rect.IsInside(station))
					{
						CGeoPointIndex xy = extents.CoordToXYPos(pt);
						size_t delta_h = (m_type == GT_HRDPS) ? 1 : 3;

						CWeatherAccumulator accumulator(TM);
						for (size_t h = 0; h <= MAX_FORECAST_HOURS&&msg; h += delta_h)
						{
							CTRef TRef = CTimeZones::UTCTRef2LocalTRef(UTCRef + h, station);
							if (accumulator.TRefIsChanging(TRef))
							{
								if (station[accumulator.GetTRef()].GetVariables().none() )//don't override observation
									station[accumulator.GetTRef()].SetData(accumulator);
							}
							//station[TRef + hh].SetStat(v, value);
							float Tair = -999;
							float Tdew = -999;
							for (size_t vv = 0; vv < NB_FORECAST_VAR&&msg; vv++)
							{
								TVarH v = FORECAST_VARIABLES[vv];
								if (bAddForecast[v] && m_datasets[h][vv].IsOpen())
								{
									float value = m_datasets[h][vv].ReadPixel(0, xy);
									
									if (v == H_PRCP)
										value *= 3600;//mm/s --> mm for one hours

									if (v == H_SRAD)
										value *= 3600;//J/m² --> W/m²

									if (v == H_PRES)
										value /= 100;//Pa --> hPa

									if (v == H_WNDS)
										value *= 3600/1000;//Pa --> hPa


									if (v == H_TAIR)
										Tair = value;

									if (v == H_TDEW)
										Tdew = value;

									accumulator.Add(TRef, v, value);
								}
							}

							if (Tdew>-999 && Tair>-999)
								accumulator.Add(TRef, H_RELH, WBSF::Td2Hr(Tair, Tdew));
						}

						if (accumulator.GetTRef().IsInit())
						{
							if (station[accumulator.GetTRef()].GetVariables().none() )//don't override observation
								station[accumulator.GetTRef()].SetData(accumulator);
						}
					}//if is inside
				}//if is in zone
			}//if msg
		}//if there is data for the current year
		//}


		return msg;
	}

}