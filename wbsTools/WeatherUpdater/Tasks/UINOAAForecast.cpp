#include "StdAfx.h"
#include "UINOAAForecast.h"
#include "Basic/WeatherStation.h"
#include "Geomatic/ShapeFileBase.h"
#include "TaskFactory.h"
#include "Geomatic/TimeZones.h"
#include "cctz\time_zone.h"


#include "WeatherBasedSimulationString.h"
#include "../Resource.h"

#pragma warning(disable: 4275 4251)
#include "GDAL\include\gdal_priv.h"


//National Digital Forecast Database (NDFD)
//
//https://graphical.weather.gov/xml/SOAP_server/ndfdXMLclient.php?whichClient=NDFDgen&lat=38.99&lon=-77.01&listLatLon=&lat1=&lon1=&lat2=&lon2=&resolutionSub=&listLat1=&listLon1=&listLat2=&listLon2=&resolutionList=&endPoint1Lat=&endPoint1Lon=&endPoint2Lat=&endPoint2Lon=&listEndPoint1Lat=&listEndPoint1Lon=&listEndPoint2Lat=&listEndPoint2Lon=&zipCodeList=&listZipCodeList=&centerPointLat=&centerPointLon=&distanceLat=&distanceLon=&resolutionSquare=&listCenterPointLat=&listCenterPointLon=&listDistanceLat=&listDistanceLon=&listResolutionSquare=&citiesLevel=&listCitiesLevel=&sector=&gmlListLatLon=&featureType=&requestedTime=&startTime=&endTime=&compType=&propertyName=&product=time-series&begin=2004-01-01T00%3A00%3A00&end=2021-03-25T00%3A00%3A00&Unit=m&temp=temp&dew=dew&wspd=wspd&wdir=wdir&Submit=Submit
//https://graphical.weather.gov/xml/SOAP_server/ndfdXML.htm
//https://graphical.weather.gov/xml/SOAP_server/ndfdXMLclient.php?whichClient=LatLonListSubgrid


//ftp://tgftp.nws.noaa.gov/SL.us008001/ST.opnl/DF.gr2/DC.ndfd/AR.conus/VP.001-003
//ftp ://tgftp.nws.noaa.gov/SL.us008001/ST.expr/DF.gr2/DC.ndfd/AR.conus/VP.001-003
//ftp://tgftp.nws.noaa.gov/SL.us008001/ST.opnl/DF.gr2/DC.ndgd/GT.glmp/AR.conus/

//ftp://tgftp.nws.noaa.gov/SL.us008001/ST.expr/DF.gr2/DC.ndfd/AR.alaska/VP.001-003/


//ftp://ftp.wpc.ncep.noaa.gov/5km_qpf
//


//http://forecast.weather.gov/MapClick.php?lat=45.025&lon=-88.6581&lg=english&&FcstType=graphical
//http://forecast.weather.gov/MapClick.php?lat=45.025&lon=-88.6581&unit=0&lg=english&FcstType=digital

//http://forecast.weather.gov/MapClick.php?lat=45.025&lon=-88.6581&unit=1&FcstType=digital&AheadHour=96
//http://forecast.weather.gov/MapClick.php?w0=t&w1=td&w2=wc&w3=sfcwind&w3u=2&w4=sky&w5=pop&w6=rh&w7=rain&AheadHour=0&Submit=Submit&FcstType=digital&textField1=44.9055&textField2=-122.8107&site=all&unit=1&dd=&bw=
//http://forecast.weather.gov/MapClick.php?w0=t&w1=td&w3=sfcwind&w3u=2&w6=rh&w13u=0&w16u=1&w17u=1&AheadHour=96&FcstType=digital&textField1=37.8236&textField2=-78.2536&site=all&unit=1&dd=&bw=&BackDay.x=79&BackDay.y=9&BackDay=48
//http://forecast.weather.gov/MapClick.php?w0=t&w1=td&w3=sfcwind&w3u=1&w6=rh&w13u=0&w16u=1&w17u=1&AheadHour=48&FcstType=digital&textField1=37.8236&textField2=-78.2536&site=all&unit=1&dd=&bw=&AheadDay.x=55&AheadDay.y=5&AheadDay=96
//http://forecast.weather.gov/product_sites.php?site=RIW&product=SFT


//https://digital.weather.gov/
//http://w2.weather.gov/climate/index.php?map=1&date=20170325
//http://ndfd.weather.gov/technical.htm




//http://forecast.weather.gov/MapClick.php?lat=45.025&lon=-88.6581&FcstType=digitalDWML
//http://www.weather.gov/mqt/







//http://www.wrh.noaa.gov/total_forecast/getprod.php?wfo=pqr&pil=PFM&sid=PQR





using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;

namespace WBSF
{
	const char* CUINOAAForecast::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir" };
	const size_t CUINOAAForecast::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH };
	const UINT CUINOAAForecast::ATTRIBUTE_TITLE_ID = IDS_UPDATER_NOAA_FORECAST_P;
	const UINT CUINOAAForecast::DESCRIPTION_TITLE_ID = ID_TASK_NOAA_FORECAST;

	const char* CUINOAAForecast::CLASS_NAME(){ static const char* THE_CLASS_NAME = "NOAAForecast";  return THE_CLASS_NAME; }
	CTaskBase::TType CUINOAAForecast::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUINOAAForecast::CLASS_NAME(), (createF)CUINOAAForecast::create);


	//
	//, 
	const size_t CUINOAAForecast::NB_VARS[NB_DATA_TYPE] = { NB_HOURLY_VARS, NB_DAILY_VARS };

	const char* CUINOAAForecast::VAR_FILE_NAME[NB_DATA_TYPE][NB_VARS_MAX] =
	{
		{ "ds.temp.bin", "ds.td.bin", "ds.rhm.bin", "ds.wspd.bin", "ds.wdir.bin" },
		{ "ds.mint.bin", "ds.maxt.bin", "ds.qpf.bin", "ds.minrh.bin", "ds.maxrh.bin" }
	};

	const TVarH CUINOAAForecast::FORECAST_VARIABLES[NB_DATA_TYPE][NB_VARS_MAX] =
	{
		{ H_TAIR2, H_TDEW, H_RELH, H_WNDS, H_WNDD },
		{ H_TMIN2, H_TMAX2, H_PRCP, H_RELH, H_RELH }
	};

	const char* CUINOAAForecast::FORECAST_TYPE[NB_FORECAST_TYPE] = { "VP.001-003", "VP.004-007" };

	//ftp://tgftp.nws.noaa.gov/SL.us008001/ST.opnl/DF.gr2/DC.ndfd/AR.conus/VP.001-003
	//ftp ://tgftp.nws.noaa.gov/SL.us008001/ST.expr/DF.gr2/DC.ndfd/AR.conus/VP.001-003
	//ftp://tgftp.nws.noaa.gov/SL.us008001/ST.opnl/DF.gr2/DC.ndgd/GT.glmp/AR.conus





	CUINOAAForecast::CUINOAAForecast(void)
	{
		m_bOpen = false;
	}
	

	CUINOAAForecast::~CUINOAAForecast(void)
	{}


	std::string CUINOAAForecast::Option(size_t i)const
	{
		string str;
		//switch (i)
		//{
		//};
		return str;
	}

	std::string CUINOAAForecast::Default(size_t i)const
	{
		std::string str;
		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "NOAA\\NDFD\\"; break;
		};

		return str;
	}


	//***********************************************************************************************************************
	// download section



	std::string CUINOAAForecast::GetInputFilePath(size_t t, size_t f, size_t v)
	{
		return string("SL.us008001/ST.opnl/DF.gr2/DC.ndfd/AR.conus/") + FORECAST_TYPE[f]  + "/" + VAR_FILE_NAME[t][v];
	}


	std::string CUINOAAForecast::GetOutputFilePath(size_t t, size_t f, size_t v)const
	{
		static const char* SUB_DIR[NB_DATA_TYPE] = { "Hourly\\", "Daily\\" };
		return string(GetDir(WORKING_DIR)) + SUB_DIR[t] + FORECAST_TYPE[f] + "\\" + VAR_FILE_NAME[t][v];
	}


	//******************************************************
	ERMsg CUINOAAForecast::Execute(CCallback& callback)
	{

		ERMsg msg;

		string workingDir = GetDir(WORKING_DIR);
		msg = CreateMultipleDir(workingDir);

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage("tgftp.nws.noaa.gov", 1);
		callback.AddMessage("");

		size_t nbDownload = 0;
	
	
		callback.PushTask("load NOAA Forecast (" + ToString((NB_HOURLY_VARS + NB_DAILY_VARS) * 2) + " gribs files )", (NB_HOURLY_VARS + NB_DAILY_VARS) * 2);

		CInternetSessionPtr pSession;
		CFtpConnectionPtr pConnection;

		msg = GetFtpConnection("tgftp.nws.noaa.gov", pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS);
		if (msg)
		{
			for (size_t t = 0; t < NB_DATA_TYPE&&msg; t++)
			{
				for (size_t f = 0; f < NB_FORECAST_TYPE&&msg; f++)
				{
					for (size_t v = 0; v < NB_VARS_MAX&&msg; v++)
					{
						string inputFilePath = GetInputFilePath(t, f, v);
						string outputFilePath = GetOutputFilePath(t, f, v);
						msg += UtilWWW::CopyFile(pConnection, inputFilePath, outputFilePath);
						if (msg)
							nbDownload++;

						msg += callback.StepIt();

					}
				}
			}

			pConnection->Close();
			pSession->Close();

		}


		callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbDownload));
		callback.PopTask();
		return msg;
	}

	
	ERMsg CUINOAAForecast::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		stationList.clear();

	
		
		return msg;
	}


	ERMsg CUINOAAForecast::OpenDatasets(CCallback& callback)
	{
		ERMsg msg;
		
		
		if (!m_bOpen)
		{
			GDALSetCacheMax64( GIntBig(1024) * 1024 * 1024 * 6);
			callback.PushTask("load NOAA Forecast (" + ToString((NB_HOURLY_VARS + NB_DAILY_VARS) * 2) + " gribs files )", (NB_HOURLY_VARS + NB_DAILY_VARS) * 2);
			for (size_t t = 0; t < NB_DATA_TYPE&&msg; t++)
			{
				for (size_t f = 0; f < NB_FORECAST_TYPE&&msg; f++)
				{
					for (size_t v = 0; v < NB_VARS_MAX&&msg; v++)
					{
						string outputFilePath = GetOutputFilePath(t,f,v);
						msg += m_datasets[t][f][v].OpenInputImage(outputFilePath);
						msg += callback.StepIt();

					}
				}
			}

			callback.PopTask();

			if (msg)
			{

				m_geo2gribs.Set(PRJ_WGS_84, m_datasets[0][0][0].GetPrjID());
				m_extents = m_datasets[0][0][0].GetExtents();
				//CGeoRect rect(-180, 0, 0, 90, PRJ_WGS_84);

			}


			m_bOpen = true;
		}


		return msg;
	}



	ERMsg CUINOAAForecast::GetWeatherStation(const std::string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		ASSERT(!station.m_ID.empty());
		ASSERT(station.m_lat != -999);
		ASSERT(station.m_lon != -999);

		CTRef current = CTRef::GetCurrentTRef(TM);
		CWVariablesCounter counter = station.GetVariablesCount();
		CTRef TairEnd = counter.GetTPeriod().End();
		ASSERT(TairEnd <= current);

		//station must have data in the last 2 weeks
		if (current - TairEnd < 14)
		{
			cctz::time_zone zone;
			CTimeZones::GetZone(station, zone);

			msg = OpenDatasets(callback);

			if (msg)
			{
				//string name = GetFileName(m_datasets.front().GetFilePath());

				cctz::time_zone zone;
				if (CTimeZones::GetZone(station, zone))
				{
					CGeoPoint3D pt(station);
					pt.Reproject(m_geo2gribs);


					if (m_extents.IsInside(pt))
					{

						CGeoPointIndex xy = m_extents.CoordToXYPos(pt);

						//size_t nbLayers = m_datasets.front().GetRasterCount();

						//GDALDataset* pDataset = m_datasets.front().Dataset();
						//GDALRasterBand * pBand1 = pDataset->GetRasterBand(1);
						//GDALRasterBand * pBand2 = pDataset->GetRasterBand(nbLayers);

						//const char* strTime1 = pBand1->GetMetadataItem("GRIB_FORECAST_SECONDS");
						//const char* strTime2 = pBand2->GetMetadataItem("GRIB_FORECAST_SECONDS");
						//size_t nbHours = WBSF::as<size_t>(strTime2)-WBSF::as<size_t>(strTime1);
						//size_t timeStep = nbHours / nbLayers;

						//ASSERT(timeStep == 3);

						//CWeatherAccumulator accumulator(TM);
						//for (size_t t = 0; t < NB_DATA_TYPE&&msg; t++)
						size_t t = TM.IsHourly() ? DATA_HOURLY : DATA_DAILY;
						//{
						for (size_t f = 0; f < NB_FORECAST_TYPE&&msg; f++)
						{
							for (size_t v = 0; v < m_datasets[t][f].size() && msg; v++)
							{
								ASSERT(m_datasets[t][f][v].IsOpen());

								for (size_t b = 0; b < m_datasets[t][f][v].GetRasterCount() && msg; b++)
								{
									const char* strTime = m_datasets[t][f][v].GetRasterBand(b)->GetMetadataItem("GRIB_VALID_TIME");
									__int64 UTCTime = WBSF::as<__int64>(strTime);
									CTRef UTCTRef = CTimeZones::UTCTime2UTCTRef(UTCTime);
									CTRef TRef = CTimeZones::UTCTRef2LocalTRef(UTCTRef, zone);

									//strTime = m_datasets[vv].GetRasterBand(b + 1)->GetMetadataItem("GRIB_VALID_TIME");
									//UTCTime = WBSF::as<__int64>(strTime);
									//UTCTRef = CTimeZones::UTCTime2UTCTRef(UTCTime);
									//CTRef TRef2 = CTimeZones::UTCTRef2LocalTRef(UTCTRef, zone);


									double value = m_datasets[t][f][v].ReadPixel(b, xy);
									//								double value2 = m_datasets[vv].ReadPixel(b+1, xy);

									if (fabs(m_datasets[t][f][v].GetNoData(b) - value) > 1)
										//								fabs(m_datasets[vv].GetNoData(b+1) - value2) > 1 )
									{

										TVarH var = FORECAST_VARIABLES[t][v];
										if (var == H_WNDS)
										{
											value *= 3600 / 1000;//m/s --> km/h
											//value2 *= 3600 / 1000;//m/s --> km/h
										}

										//for (size_t i = 0; i < timeStep; i++)
										//{
										//accumulator.Add(TRef, FORECAST_VARIABLES[v], value);
										//}

										ASSERT(TM.IsHourly());// a faire daily

										//for (CTRef TRef = TRef1; TRef <= TRef2; TRef++)

										//double value = ((TRef - TRef1)*value2 + (TRef2 - TRef)*value1) / (TRef2 - TRef1);

										if (!station[TRef][var].IsInit())
											station.GetHour(TRef).SetStat(var, value);
										//station[accumulator.GetTRef()].SetData(accumulator);

									}
								}


								//if (accumulator.TRefIsChanging(TRef) || b == nbLayers - 1)
								//{
								//	if (station[accumulator.GetTRef()].GetVariables().none())//don't override observation
								//		station[accumulator.GetTRef()].SetData(accumulator);
								//}
							}
						}
						//}
					}//if is inside
				}//if is in zone
			}//if msg
		}//if there is data for the current year


		return msg;
	}

}