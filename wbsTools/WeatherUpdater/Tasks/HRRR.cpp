#include "StdAfx.h"
#include "HRRR.h"
#include "Basic/WeatherStation.h"
#include "Geomatic/ShapeFileBase.h"
#include "TaskFactory.h"
#include "Geomatic/TimeZones.h"
#include "Geomatic/SfcGribsDatabase.h"
#include "UI/Common/SYShowMessage.h"

#include "WeatherBasedSimulationString.h"
#include "../Resource.h"
#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;

namespace WBSF
{
	//HRRR(SFC) historical since at least 2017
	//https://pando-rgw01.chpc.utah.edu/hrrr/sfc/20180101/hrrr.t00z.wrfsfcf00.grib2
	//https://pando-rgw01.chpc.utah.edu/hrrr/sfc/20170101/hrrr.t00z.wrfsfcf00.grib2

	//*********************************************************************
	const char* CHRRR::SERVER_NAME[NB_SOURCES][NB_SERVER_TYPE] = { {"pando-rgw01.chpc.utah.edu" ,""},{"nomads.ncep.noaa.gov", "ftp.ncep.noaa.gov"} };
	const char* CHRRR::SERVER_PATH[NB_SOURCES][NB_SERVER_TYPE] = { { "/hrrr/%s/%04d%02d%02d/","" },{ "/pub/data/nccf/com/hrrr/prod/", "/pub/data/nccf/com/hrrr/prod/" } };
	const char* CHRRR::PRODUCT_ABR[NB_SOURCES][NB_PRODUCT] = { {"sfc","prs"}, {"sfc","nat"} };

	CHRRR::CHRRR(const std::string& workingDir) :
		m_workingDir(workingDir),
		m_product(HRRR_SFC),
		m_serverType(HTTP_SERVER),
		m_bShowWINSCP(false),
		m_compute_prcp(true),
		m_update_last_n_days(0),
		m_createDailyGeotiff(false)
	{}

	CHRRR::~CHRRR(void)
	{}



	//****************************************************************************************************
	ERMsg CHRRR::Execute(CCallback& callback)
	{
		ERMsg msg;

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(m_workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(SERVER_NAME[m_source][m_serverType], 1);
		callback.AddMessage("");

		GDALSetCacheMax64(128 * 1024 * 1024);

		switch (m_source)
		{
		case MESO_WEST:
			switch (m_serverType)
			{
			case HTTP_SERVER:msg = msg = ExecuteHistorical(callback); break;
			case FTP_SERVER: msg.ajoute("ftp not available for HRRR archived files"); break;
			default:ASSERT(false);
			}
			break;

		case NOMADS:
			switch (m_serverType)
			{
			case HTTP_SERVER:msg = ExecuteHTTP(callback); break;
			case FTP_SERVER:msg = ExecuteFTP(callback); break;
			default:ASSERT(false);
			}
			break;

		default:ASSERT(false);
		}
		return msg;
	}

	ERMsg CHRRR::ExecuteFTP(CCallback& callback)
	{
		ERMsg msg;

		size_t nbDownloaded = 0;
		set<string> date_to_update;



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
					CreateMultipleDir(GetPath(outputFilePath));

					stript << "open ftp://anonymous:anonymous%40example.com@" << SERVER_NAME[NOMADS][FTP_SERVER] << endl;
					stript << "cd " << GetPath(fileList[i].m_filePath) << endl;
					stript << "lcd \"" << GetPath(outputFilePath) << "\"" << endl;
					stript << "get " << GetFileName(outputFilePath) << endl;
					stript << "exit" << endl;
					stript.close();


					//# Execute the script using a command like:
					string command = "\"" + GetApplicationPath() + "External\\WinSCP.exe\" " + string(m_bShowWINSCP ? "/console " : "") + "-timeout=300 -passive=on /log=\"" + scriptFilePath + ".log\" /ini=nul /script=\"" + scriptFilePath;
					DWORD exit_code;
					msg = WBSF::WinExecWait(command, "", SW_SHOW, &exit_code);
					if (msg)
					{
						//verify if the file finish with 7777

						if (exit_code == 0 && FileExists(outputFilePath))
						{
							if (GoodGrib(outputFilePath))
							{
								nbDownloaded++;
								CTRef TRef = GetRemoteTRef(fileList[i].m_filePath);
								date_to_update.insert(TRef.GetFormatedString("%Y-%m-%d-%H"));
							}
							else
							{
								msg = WBSF::RemoveFile(outputFilePath);
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


			//if (date_to_update.empty() && m_createHistoricalGeotiff)
				//date_to_update = GetAll(callback);
			if (m_update_last_n_days > 0)
			{
				set<string> last_n_days = Getlast_n_days(m_update_last_n_days, callback);
				date_to_update.insert(last_n_days.begin(), last_n_days.end());
			}

			msg = CreateHourlyGeotiff(date_to_update, callback);

			if (msg && m_createDailyGeotiff)
				msg = CreateDailyGeotiff(date_to_update, callback);


		}

		return msg;
	}

	ERMsg CHRRR::GetFilesToDownload(CFileInfoVector& fileList, CCallback& callback)
	{
		ERMsg msg;


		CInternetSessionPtr pSession;
		CFtpConnectionPtr pConnection;

		msg = GetFtpConnection(SERVER_NAME[NOMADS][FTP_SERVER], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", true, 5, callback);
		if (!msg)
			return msg;

		callback.PushTask(string("Get files list from: ") + SERVER_PATH[NOMADS][FTP_SERVER], 2);

		CFileInfoVector dir;
		msg = FindDirectories(pConnection, SERVER_PATH[NOMADS][FTP_SERVER], dir);
		for (CFileInfoVector::const_iterator it1 = dir.begin(); it1 != dir.end() && msg; it1++)
		{
			CFileInfoVector fileListTmp;
			msg = FindFiles(pConnection, it1->m_filePath + "conus/hrrr.t??z.wrf" + PRODUCT_ABR[NOMADS][m_product] + "f00.grib2", fileListTmp);

			if (msg && m_compute_prcp)
				msg = FindFiles(pConnection, it1->m_filePath + "conus/hrrr.t??z.wrf" + PRODUCT_ABR[NOMADS][m_product] + "f01.grib2", fileListTmp);

			for (CFileInfoVector::iterator it = fileListTmp.begin(); it != fileListTmp.end() && msg; it++)
			{
				if (GetFileExtension(it->m_filePath) == ".grib2")
				{
					string outputFilePath = GetOutputFilePath(it->m_filePath);
					if (!GoodGrib(outputFilePath))
						fileList.push_back(*it);

				}
			}

			msg += callback.StepIt();
		}


		pConnection->Close();
		pSession->Close();

		callback.PopTask();

		return msg;
	}

	//****************************************************************************************************

	ERMsg CHRRR::ExecuteHTTP(CCallback& callback)
	{
		ERMsg msg;


		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg = GetHttpConnection(SERVER_NAME[NOMADS][HTTP_SERVER], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);
		if (!msg)
			return msg;



		callback.PushTask(string("Get files list from: ") + SERVER_PATH[NOMADS][HTTP_SERVER], 2);
		CFileInfoVector fileList;
		StringVector fileListPrcp;


		CFileInfoVector dir;
		msg = FindDirectories(pConnection, SERVER_PATH[NOMADS][HTTP_SERVER], dir, false, callback);
		for (CFileInfoVector::const_iterator it1 = dir.begin(); it1 != dir.end() && msg; it1++)
		{
			CFileInfoVector fileListTmp;
			msg = FindFiles(pConnection, it1->m_filePath + "conus/hrrr.t??z.wrf" + PRODUCT_ABR[NOMADS][m_product] + "f00.grib2", fileListTmp);

			if (msg && m_compute_prcp)
				msg = FindFiles(pConnection, it1->m_filePath + "conus/hrrr.t??z.wrf" + PRODUCT_ABR[NOMADS][m_product] + "f01.grib2", fileListTmp);

			for (CFileInfoVector::iterator it = fileListTmp.begin(); it != fileListTmp.end() && msg; it++)
			{
				if (GetFileExtension(it->m_filePath) == ".grib2")
				{
					string outputFilePath = GetOutputFilePath(it->m_filePath);
					if (!GoodGrib(outputFilePath))
					{
						fileList.push_back(*it);
					}
				}
			}

			msg += callback.StepIt();
		}


		callback.PopTask();

		set<string> date_to_update;


		callback.PushTask("Download HRRR gribs (" + ToString(fileList.size()) + ")", fileList.size());
		callback.AddMessage("Number of HRRR gribs to download from HTTP: " + ToString(fileList.size()));

		int nbDownloaded = 0;
		for (CFileInfoVector::iterator it = fileList.begin(); it != fileList.end() && msg; it++)
		{
			string outputFilePath = GetOutputFilePath(it->m_filePath);


			//callback.PushTask("Download HRRR gribs:" + outputFilePath, NOT_INIT);

			CreateMultipleDir(GetPath(outputFilePath));
			msg = CopyFile(pConnection, it->m_filePath, outputFilePath, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE, false, callback);
			if (msg && FileExists(outputFilePath))
			{
				if (GoodGrib(outputFilePath))
				{
					nbDownloaded++;
					CTRef TRef = GetRemoteTRef(it->m_filePath);
					date_to_update.insert(TRef.GetFormatedString("%Y-%m-%d-%H"));
				}
				else
				{
					msg = WBSF::RemoveFile(outputFilePath);
				}
			}

			//callback.PopTask();
			msg += callback.StepIt();
		}

		pConnection->Close();
		pSession->Close();



		callback.AddMessage("Number of HRRR gribs downloaded: " + ToString(nbDownloaded));
		callback.PopTask();

		//if (date_to_update.empty() && m_createHistoricalGeotiff)
			//date_to_update = GetAll(callback);
		if (m_update_last_n_days > 0)
		{
			set<string> last_n_days = Getlast_n_days(m_update_last_n_days, callback);
			date_to_update.insert(last_n_days.begin(), last_n_days.end());
		}

		/*for(int d=1; d<26; d++)
			for (int h = 0; h < 24; h++)
				date_to_update.insert(FormatA("2019-09-%02d-%02d", d, h));
*/

		msg = CreateHourlyGeotiff(date_to_update, callback);

		if (msg && m_createDailyGeotiff)
			msg = CreateDailyGeotiff(date_to_update, callback);


		return msg;
	}

	ERMsg CHRRR::CreateHourlyGeotiff(std::set<std::string> date_to_update, CCallback& callback)const
	{
		ERMsg msg;


		callback.PushTask("Create hourly HRRR Geotiff (" + ToString(date_to_update.size()) + " hours)", date_to_update.size());
		callback.AddMessage("Create hourly HRRR Geotiff: " + ToString(date_to_update.size()));

		for (auto it = date_to_update.begin(); it != date_to_update.end() && msg; it++)
		{
			CTRef TRef;
			TRef.FromFormatedString(*it);
			string outputFilePath = GetOutputFilePath(TRef, 0);
			msg += CreateHourlyGeotiff(outputFilePath, callback);
			msg += callback.StepIt();
		}

		callback.PopTask();

		return msg;
	}


	ERMsg CHRRR::CreateHourlyGeotiff(const string& inputFilePath, CCallback& callback)const
	{
		ERMsg msg;

		CTRef TRef = GetLocalTRef(inputFilePath);
		string inputFilePath2 = GetOutputFilePath(TRef - 1, 1);

		string outputFilePath = inputFilePath;
		ReplaceString(outputFilePath, "f00.grib2", "f00.tif");

		if (GoodGrib(inputFilePath) || GoodGrib(inputFilePath2))
		{
			//bool bComputeWind = false;

			int nbBands = 0;
			CSfcDatasetCached DSin1;
			if (GoodGrib(inputFilePath))
			{
				DSin1.m_variables_to_load.reset(H_PRCP);
				DSin1.m_variables_to_load.reset(H_WNDS);//use computed wind speed and direction from U and V
				DSin1.m_variables_to_load.reset(H_SNOW);//no data available
				DSin1.m_variables_to_load.reset(H_SWE);//no data available
				msg += DSin1.open(inputFilePath, true);
				if (msg)
					nbBands += (int)DSin1.get_variables().count();


				//size_t bS = DSin1.get_band(H_WNDS);
				size_t bU = DSin1.get_band(H_UWND);
				size_t bV = DSin1.get_band(H_VWND);
				if (bU != NOT_INIT && bV != NOT_INIT)
					nbBands += 2;
			}

			CSfcDatasetCached DSin2;
			if (GoodGrib(inputFilePath2))
			{
				DSin2.m_variables_to_load.reset();
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
				float no_data = 9999;// GetDefaultNoData(GDT_Int16);
				CBaseOptions options;
				DSin1.UpdateOption(options);
				options.m_nbBands = nbBands;
				options.m_outputType = GDT_Float32;
				options.m_dstNodata = no_data;
				options.m_bOverwrite = true;
				options.m_createOptions.push_back("COMPRESS=LZW");
				options.m_createOptions.push_back("PREDICTOR=3");
				options.m_createOptions.push_back("TILED=YES");
				options.m_createOptions.push_back("BLOCKXSIZE=256");
				options.m_createOptions.push_back("BLOCKYSIZE=256");

				CGDALDatasetEx DSout;
				msg += DSout.CreateImage(outputFilePath + "2", options);
				if (msg)
				{
					for (size_t v = 0, bb = 0; v < DSin1.get_variables().size(); v++)
					{
						CSfcDatasetCached&  DSin = (v == H_PRCP) ? DSin2 : DSin1;
						size_t b = DSin.get_band(v);

						if (b != NOT_INIT)
						{
							GDALRasterBand* pBandin = DSin.GetRasterBand(b);
							GDALRasterBand* pBandout = DSout.GetRasterBand(bb);


							ASSERT(DSin.GetRasterXSize() == DSout.GetRasterXSize());
							ASSERT(DSin.GetRasterYSize() == DSout.GetRasterYSize());

							float no_data_in = DSin.GetNoData(b);
							vector<float> data(DSin.GetRasterXSize()*DSin.GetRasterYSize());
							pBandin->RasterIO(GF_Read, 0, 0, DSin.GetRasterXSize(), DSin.GetRasterYSize(), &(data[0]), DSin.GetRasterXSize(), DSin.GetRasterYSize(), GDT_Float32, 0, 0);
							//replace no data
							for (size_t xy = 0; xy < data.size(); xy++)
							{
								if (fabs(data[xy] - no_data_in) < 0.1)
									data[xy] = no_data;
							}
							pBandout->RasterIO(GF_Write, 0, 0, DSin.GetRasterXSize(), DSin.GetRasterYSize(), &(data[0]), DSin.GetRasterXSize(), DSin.GetRasterYSize(), GDT_Float32, 0, 0);


							if (pBandin->GetDescription())
								pBandout->SetDescription(pBandin->GetDescription());

							if (pBandin->GetMetadata())
								pBandout->SetMetadata(pBandin->GetMetadata());

							bb++;
						}

						DSout->FlushCache();

						if (v == H_WNDS)
						{
							ASSERT(b == NOT_INIT);//wind speed ignored here
							//add wind direction from U and V
							size_t bU = DSin1.get_band(H_UWND);
							size_t bV = DSin1.get_band(H_VWND);


							if (bU != NOT_INIT && bV != NOT_INIT)
							{
								GDALRasterBand* pBandinU = DSin.GetRasterBand(bU);
								GDALRasterBand* pBandinV = DSin.GetRasterBand(bV);
								GDALRasterBand* pBandoutS = DSout.GetRasterBand(bb);
								GDALRasterBand* pBandoutD = DSout.GetRasterBand(bb + 1);


								ASSERT(DSin.GetRasterXSize() == DSout.GetRasterXSize());
								ASSERT(DSin.GetRasterYSize() == DSout.GetRasterYSize());

								float no_data_inU = DSin.GetNoData(bU);
								float no_data_inV = DSin.GetNoData(bV);
								vector<float> dataU(DSin.GetRasterXSize()*DSin.GetRasterYSize());
								vector<float> dataV(DSin.GetRasterXSize()*DSin.GetRasterYSize());
								pBandinU->RasterIO(GF_Read, 0, 0, DSin.GetRasterXSize(), DSin.GetRasterYSize(), &(dataU[0]), DSin.GetRasterXSize(), DSin.GetRasterYSize(), GDT_Float32, 0, 0);
								pBandinV->RasterIO(GF_Read, 0, 0, DSin.GetRasterXSize(), DSin.GetRasterYSize(), &(dataV[0]), DSin.GetRasterXSize(), DSin.GetRasterYSize(), GDT_Float32, 0, 0);

								vector<float> dataS(DSin.GetRasterXSize()*DSin.GetRasterYSize(), no_data);
								vector<float> dataD(DSin.GetRasterXSize()*DSin.GetRasterYSize(), no_data);
								//replace no data
								for (size_t xy = 0; xy < dataU.size(); xy++)
								{
									if (fabs(dataU[xy] - no_data_inU) > 0.1 &&
										fabs(dataV[xy] - no_data_inV) > 0.1)
									{
										dataS[xy] = sqrt(dataU[xy] * dataU[xy] + dataV[xy] * dataV[xy]);
										dataD[xy] = (float)GetWindDirection(dataU[xy], dataV[xy], true);//approximation of wind direction
									}
								}

								pBandoutS->RasterIO(GF_Write, 0, 0, DSin.GetRasterXSize(), DSin.GetRasterYSize(), &(dataS[0]), DSin.GetRasterXSize(), DSin.GetRasterYSize(), GDT_Float32, 0, 0);
								pBandoutS->SetDescription(CSfcGribDatabase::META_DATA[H_WNDS][M_DESC]);
								pBandoutS->SetMetadataItem("GRIB_COMMENT", CSfcGribDatabase::META_DATA[H_WNDS][M_COMMENT]);
								pBandoutS->SetMetadataItem("GRIB_ELEMENT", CSfcGribDatabase::META_DATA[H_WNDS][M_ELEMENT]);
								pBandoutS->SetMetadataItem("GRIB_SHORT_NAME", CSfcGribDatabase::META_DATA[H_WNDS][M_SHORT_NAME]);
								pBandoutS->SetMetadataItem("GRIB_UNIT", CSfcGribDatabase::META_DATA[H_WNDS][M_UNIT]);


								pBandoutD->RasterIO(GF_Write, 0, 0, DSin.GetRasterXSize(), DSin.GetRasterYSize(), &(dataD[0]), DSin.GetRasterXSize(), DSin.GetRasterYSize(), GDT_Float32, 0, 0);
								pBandoutD->SetDescription(CSfcGribDatabase::META_DATA[H_WNDD][M_DESC]);
								pBandoutD->SetMetadataItem("GRIB_COMMENT", CSfcGribDatabase::META_DATA[H_WNDD][M_COMMENT]);
								pBandoutD->SetMetadataItem("GRIB_ELEMENT", CSfcGribDatabase::META_DATA[H_WNDD][M_ELEMENT]);
								pBandoutD->SetMetadataItem("GRIB_SHORT_NAME", CSfcGribDatabase::META_DATA[H_WNDD][M_SHORT_NAME]);
								pBandoutD->SetMetadataItem("GRIB_UNIT", CSfcGribDatabase::META_DATA[H_WNDS][H_WNDD]);


								bb += 2;
							}
						}
					}

					DSout.Close();
				}

				DSin1.close();
				DSin2.close();

			}//if dataset open



			if (msg)
			{
				//copy the file to fully use compression with GDAL_translate
				string gdal_data_path = GetApplicationPath() + "External\\gdal-data";
				string projlib_path = GetApplicationPath() + "External\\projlib";

				//-stats : do not include stat to avoid the creation of the xml file
				string option = "--config GDAL_DATA \"" + gdal_data_path + "\" --config PROJ_LIB \"" + projlib_path + "\"";
				string argument = "-unscale -ot Float32 -co COMPRESS=LZW -co PREDICTOR=3 -co TILED=YES -co BLOCKXSIZE=256 -co BLOCKYSIZE=256";
				string command = "\"" + GetApplicationPath() + "External\\gdal_translate.exe\" " + option + " " + argument + " \"" + outputFilePath + "2" + "\" \"" + outputFilePath + "\"";

				//string argument = "-ot Float32 -stats -co COMPRESS=LZW -co PREDICTOR=3 -co TILED=YES -co BLOCKXSIZE=256 -co BLOCKYSIZE=256 \"" + outputFilePath + "2" + "\" \"" + outputFilePath + "\"";
				//string command = "\"" + GetApplicationPath() + "External\\gdal_translate.exe\" " + argument;
				msg += WinExecWait(command);
				msg += RemoveFile(outputFilePath + "2");
			}
		}//good grib


		return msg;
	}

	std::set<std::string> CHRRR::Getlast_n_days(size_t nb_days, CCallback& callback)
	{
		ERMsg msg;

		std::set<std::string> date_to_update;
		

		callback.PushTask("Get last file to update: " + to_string(nb_days) + " days", nb_days*24);
		
		CTRef TRef = CTRef::GetCurrentTRef(CTM::HOURLY);
		for (size_t d = 0; d < nb_days * 24&&msg; d++)
		{
			
			string file_path = GetOutputFilePath(TRef, 0);
			if (GoodGrib(file_path))
			{
				string tifFilePath = file_path;
				ReplaceString(tifFilePath, "f00.grib2", "f00.tif");
				
				CGDALDatasetEx DS;
				if (DS.OpenInputImage(tifFilePath))
				{
					DS.Close();
				
				}
				else
				{
					if (FileExists(tifFilePath))
					{
						callback.AddMessage("Remove invalid HRRR GeoTiff " + tifFilePath);
						msg += RemoveFile(tifFilePath);
					}
					date_to_update.insert(TRef.GetFormatedString("%Y-%m-%d-%H"));
					
				}
			}
			else
			{
				if (FileExists(file_path))
				{
					callback.AddMessage("Remove invalid HRRR gribs " + file_path);
					msg += RemoveFile(file_path);
				}
			}
		

			msg += callback.StepIt();
			TRef--;
		}//for all days

		callback.PopTask();

		return date_to_update;
	}


	ERMsg CHRRR::ExecuteHistorical(CCallback& callback)
	{
		ERMsg msg;

		size_t nbFilesToDownload = 0;
		size_t nbDownloaded = 0;
		set<string> date_to_update;

		vector< array<bool, 2>> bGrbNeedDownload;
		bGrbNeedDownload.resize(m_period.size());

		size_t nb_pass = m_compute_prcp ? 2 : 1;

		callback.PushTask(string("Find gribs to update for period ") + m_period.GetFormatedString("%1 ---- %2") + ": " + to_string(m_period.size()) + " hours", m_period.size());

		for (CTRef h = m_period.Begin(); h <= m_period.End()&&msg; h++)
		{
			size_t hh = (h - m_period.Begin());
			for (size_t i = 0; i < nb_pass; i++)
			{
				bGrbNeedDownload[hh][i] = NeedDownload(GetOutputFilePath(h, i));
				nbFilesToDownload += bGrbNeedDownload[hh][i] ? 1 : 0;
			}
			msg += callback.StepIt();
		}
		callback.PopTask();



		callback.PushTask(string("Download HRRR gribs from \"") + SERVER_NAME[MESO_WEST][HTTP_SERVER] + "\" for period " + m_period.GetFormatedString("%1 ---- %2") + ": " + to_string(nbFilesToDownload) + " files", nbFilesToDownload);
		callback.AddMessage(string("Download HRRR gribs from \"") + SERVER_NAME[MESO_WEST][HTTP_SERVER] + "\" for period " + m_period.GetFormatedString("%1 ---- %2") + ": " + to_string(nbFilesToDownload) + " files");

		if (nbFilesToDownload > 0)
		{
			size_t nbTry = 0;
			CTRef curH = m_period.Begin();

			while (curH < m_period.End() && msg)
			{
				nbTry++;

				CInternetSessionPtr pSession;
				CHttpConnectionPtr pConnection;
				msg = GetHttpConnection(SERVER_NAME[MESO_WEST][HTTP_SERVER], pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", true, 5, callback);


				if (msg)
				{
					try
					{
						while (curH <= m_period.End() && msg)
						{
							const char* str_p = PRODUCT_ABR[MESO_WEST][m_product];
							int y = curH.GetYear();
							int m = int(curH.GetMonth() + 1);
							int d = int(curH.GetDay() + 1);
							int hs = int(curH.GetHour());
							size_t hh = (curH - m_period.Begin());

							for (size_t i = 0; i < nb_pass; i++)
							{

								if (bGrbNeedDownload[hh][i])
								{
									//download gribs file

									string URL = FormatA(SERVER_PATH[MESO_WEST][HTTP_SERVER], str_p, y, m, d) + FormatA("hrrr.t%02dz.wrf%sf%02d.grib2", hs, str_p, i);
									string outputPath = GetOutputFilePath(curH, i);
									CreateMultipleDir(GetPath(outputPath));

									msg += CopyFile(pConnection, URL, outputPath, INTERNET_FLAG_SECURE | INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_EXISTING_CONNECT, true, callback);
									if (msg)
									{
										if (GoodGrib(outputPath))
										{
											nbDownloaded++;
											date_to_update.insert(curH.GetFormatedString("%Y-%m-%d-%H"));
										}
										else
										{
											callback.AddMessage("Invalid HRRR: " + outputPath);
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
							}

							if (msg)
								curH++;
						}
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


		if (m_update_last_n_days > 0)
		{
			set<string> last_n_days = Getlast_n_days(m_update_last_n_days, callback);
			date_to_update.insert(last_n_days.begin(), last_n_days.end());
		}

		msg = CreateHourlyGeotiff(date_to_update, callback);

		if (msg && m_createDailyGeotiff)
			msg = CreateDailyGeotiff(date_to_update, callback);


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

	CTRef CHRRR::GetRemoteTRef(const string& filePath)
	{
		string fileName = GetFileName(filePath);
		string tmp1 = filePath.substr(34, 8);

		CTRef TRef;
		StringVector tmp2(fileName, ".");
		ASSERT(tmp2.size() == 4);


		ASSERT(tmp1.length() == 8);
		if (tmp1.length() == 8 && tmp2.size() == 4)
		{
			int year = WBSF::as<int>(tmp1.substr(0, 4));
			size_t m = WBSF::as<size_t >(tmp1.substr(4, 2)) - 1;
			size_t d = WBSF::as<size_t >(tmp1.substr(6, 2)) - 1;
			size_t h = WBSF::as<size_t >(tmp2[1].substr(1, 2));
			TRef = CTRef(year, m, d, h);
		}

		return TRef;

	}

	string CHRRR::GetOutputFilePath(const string& filePath)const
	{
		CTRef TRef = GetRemoteTRef(filePath);
		string title = GetFileTitle(filePath);
		return FormatA("%s%d\\%02d\\%02d\\%s.grib2", m_workingDir.c_str(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, title.c_str());
	}

	string CHRRR::GetOutputFilePath(CTRef TRef, size_t HH)const
	{
		return FormatA("%s%04d\\%02d\\%02d\\hrrr.t%02dz.wrf%sf%02d.grib2", m_workingDir.c_str(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, TRef.GetHour(), PRODUCT_ABR[NOMADS][m_product], HH);
	}

	ERMsg CHRRR::CreateDailyGeotiff(set<string> date_to_update, CCallback& callback)const
	{
		ERMsg msg;

		set<string> date_to_update_day;
		for (auto it = date_to_update.begin(); it != date_to_update.end() && msg; it++)
		{
			CTRef TRef;
			TRef.FromFormatedString(*it);
			date_to_update_day.insert(TRef.as(CTM::DAILY).GetFormatedString("%Y-%m-%d"));
		}

		callback.PushTask("Create HRRR daily Geotiff (" + to_string(date_to_update_day.size()) + " days)", date_to_update_day.size());
		callback.AddMessage("Create HRRR daily Geotiff  (" + to_string(date_to_update_day.size()) + " days)");

		for (auto it = date_to_update_day.begin(); it != date_to_update_day.end() && msg; it++)
		{
			string year = it->substr(0, 4);
			string month = it->substr(5, 2);
			string day = it->substr(8, 2);

			string filter = m_workingDir + year + "\\" + month + "\\" + day + "\\hrrr.t??z.wrfsfcf00.tif";
			StringVector filesList = WBSF::GetFilesList(filter, 2);

			if (filesList.size() == 24)
			{
				string file_path_out = FormatA("%s%s\\%s\\%s\\hrrrd_%s%s%s.tif", m_workingDir.c_str(), year.c_str(), month.c_str(), day.c_str(), year.c_str(), month.c_str(), day.c_str());
				msg += CreateDailyGeotiff(filesList, file_path_out, callback);
				msg += callback.StepIt();
			}
		}

		callback.PopTask();

		return msg;
	}

	ERMsg CHRRR::CreateDailyGeotiff(StringVector& filesList, const std::string& file_path_out, CCallback& callback)const
	{
		ERMsg msg;

		GribVariables var_in;
		vector<CSfcDatasetCached> DSin(filesList.size());
		map<size_t, const char*> description;
		map<size_t, char**> meta_data;


		for (size_t i = 0; i < filesList.size(); i++)
		{
			msg += DSin[i].open(filesList[i], true);
			if (msg)
			{
				var_in = var_in | DSin[i].get_variables();
				for (size_t v = 0; v < DSin[i].get_variables().size() && msg; v++)
				{
					if (var_in.test(v))
					{
						size_t b = DSin[i].get_band(v);
						ASSERT(b != NOT_INIT); //in HRRR all bands are available

						GDALRasterBand* poBand = DSin[i]->GetRasterBand(int(b) + 1);//one base in direct load
						if (poBand->GetDescription())
							description[v] = poBand->GetDescription();

						if (poBand->GetMetadata())
							meta_data[v] = poBand->GetMetadata();

					}
				}
			}
		}

		if (msg)
		{
			GribVariables var_out = var_in;

			if (var_out[H_TAIR])
			{
				//if temperature present, add min and max
				var_out.set(H_TMIN);
				var_out.set(H_TMAX);
			}



			float no_data = 9999;

			CBaseOptions options;
			DSin[0].UpdateOption(options);
			options.m_extents.m_xBlockSize = 256;
			options.m_extents.m_yBlockSize = 256;
			options.m_nbBands = var_out.count();
			options.m_outputType = GDT_Float32;
			options.m_dstNodata = no_data;
			options.m_bOverwrite = true;
			options.m_bComputeStats = true;
			options.m_createOptions.push_back("COMPRESS=LZW");
			options.m_createOptions.push_back("PREDICTOR=3");
			options.m_createOptions.push_back("TILED=YES");
			options.m_createOptions.push_back("BLOCKXSIZE=256");
			options.m_createOptions.push_back("BLOCKYSIZE=256");

			CGDALDatasetEx DSout;
			msg += DSout.CreateImage(file_path_out + "2", options);

			if (msg)
			{
				CGeoExtents extents = DSout.GetExtents();
				callback.PushTask("Create daily HRRR Geotiff for " + GetFileTitle(file_path_out) + ": " + ToString(filesList.size()) + " hours", var_in.count()*extents.YNbBlocks()*extents.XNbBlocks());

				for (size_t by = 0; by < extents.YNbBlocks(); by++)
				{
					for (size_t bx = 0; bx < extents.XNbBlocks(); bx++)
					{
						size_t b_out = 0;
						for (size_t v = 0; v < var_out.size() && msg; v++)
						{
							if (var_in.test(v) && var_out.test(v))
							{
								CGeoExtents block_ext = extents.GetBlockExtents(int(bx), int(by));
								vector<CStatistic> stat;
								//(block_ext.GetNbPixels());

								for (size_t i = 0; i < DSin.size() && msg; i++)
								{
									size_t b = DSin[i].get_band(v);
									ASSERT(b != NOT_INIT); //in HRRR all bands are available

									if (b != NOT_INIT)
									{
										//copy part of the date
										//CGeoExtents ext = DSin[i].GetExtents().GetBlockExtents(int(i), int(j));

										GDALRasterBand* poBand = DSin[i]->GetRasterBand(int(b) + 1);//one base in direct load
										int nXBlockSize, nYBlockSize;
										poBand->GetBlockSize(&nXBlockSize, &nYBlockSize);
										if (stat.empty())
											stat.resize(nXBlockSize*nYBlockSize);

										GDALDataType type = poBand->GetRasterDataType();

										vector<float> data;

										if (nXBlockSize == options.m_extents.m_xBlockSize &&
											nYBlockSize == options.m_extents.m_yBlockSize &&
											type == GDT_Float32)
										{
											data.resize(nXBlockSize*nYBlockSize, no_data);
											poBand->ReadBlock(int(bx), int(by), &(data[0]));
											poBand->FlushBlock(int(bx), int(by));
											poBand->FlushCache();
										}//
										else//if not the same block size or type
										{
											CGeoExtents blockExtent = extents.GetBlockExtents(int(bx), int(by));
											data.resize(DSout.GetRasterXSize()*DSout.GetRasterYSize());

											poBand->RasterIO(GF_Read, blockExtent.m_xMin, blockExtent.m_yMin, blockExtent.m_xBlockSize, blockExtent.m_yBlockSize, &(data[0]), blockExtent.m_xBlockSize, blockExtent.m_yBlockSize, GDT_Float32, 0, 0);
											ASSERT(data.size() == stat.size());
										}

										for (size_t xy = 0; xy < data.size(); xy++)
										{
											if (fabs(data[xy] - no_data) > 0.1)
												stat[xy] += data[xy];
										}


									}//if valid band
								}//for all input band


								if (v == H_TAIR)
								{
									GDALRasterBand* pBandout = DSout.GetRasterBand(b_out);

									int nXBlockSize, nYBlockSize;
									pBandout->GetBlockSize(&nXBlockSize, &nYBlockSize);

									vector<float> data(nXBlockSize*nYBlockSize, no_data);
									ASSERT(data.size() == stat.size());

									//add min and max
									for (size_t xy = 0; xy < data.size(); xy++)
									{
										if (stat[xy].IsInit())
											data[xy] = stat[xy][LOWEST];
									}




									pBandout->WriteBlock(int(bx), int(by), &(data[0]));


									//pBandout->RasterIO(GF_Write, 0, 0, DSout.GetRasterXSize(), DSout.GetRasterYSize(), &(data[0]), DSout.GetRasterXSize(), DSout.GetRasterYSize(), GDT_Float32, 0, 0);
									pBandout->SetDescription("2[m] HTGL=\"Specified height level above ground\"");
									pBandout->SetMetadataItem("GRIB_COMMENT", "Minimum Temperature [C]");
									pBandout->SetMetadataItem("GRIB_ELEMENT", "TMIN");
									pBandout->SetMetadataItem("GRIB_SHORT_NAME", "2-HTGL");
									pBandout->SetMetadataItem("GRIB_UNIT", "[C]");

									b_out++;
								}

								{
									size_t stat_type = (v == H_PRCP) ? SUM : MEAN;
									GDALRasterBand* pBandout = DSout.GetRasterBand(b_out);

									int nXBlockSize, nYBlockSize;
									pBandout->GetBlockSize(&nXBlockSize, &nYBlockSize);

									vector<float> data(nXBlockSize*nYBlockSize, no_data);
									ASSERT(data.size() == stat.size());

									for (size_t xy = 0; xy < data.size(); xy++)
									{
										if (stat[xy].IsInit())
											data[xy] = stat[xy][stat_type];
									}


									pBandout->WriteBlock(int(bx), int(by), &(data[0]));
									pBandout->SetDescription(description[v]);
									pBandout->SetMetadata(meta_data[v]);
									b_out++;
								}

								if (v == H_TAIR)
								{
									GDALRasterBand* pBandout = DSout.GetRasterBand(b_out);

									int nXBlockSize, nYBlockSize;
									pBandout->GetBlockSize(&nXBlockSize, &nYBlockSize);

									vector<float> data(nXBlockSize*nYBlockSize, no_data);
									ASSERT(data.size() == stat.size());

									for (size_t xy = 0; xy < data.size(); xy++)
									{
										if (stat[xy].IsInit())
											data[xy] = stat[xy][HIGHEST];
									}

									pBandout->WriteBlock(int(bx), int(by), &(data[0]));

									pBandout->SetDescription("2[m] HTGL=\"Specified height level above ground\"");
									pBandout->SetMetadataItem("GRIB_COMMENT", "Maximum Temperature [C]");
									pBandout->SetMetadataItem("GRIB_ELEMENT", "TMAX");
									pBandout->SetMetadataItem("GRIB_SHORT_NAME", "2-HTGL");
									pBandout->SetMetadataItem("GRIB_UNIT", "[C]");


									b_out++;
								}

								msg += callback.StepIt();
							}//if var used
						}//for all variable
					}//for all block x
				}//for all block y

				DSout.Close(options);

				callback.PopTask();
			}//out open

			for (size_t i = 0; i < filesList.size(); i++)
				DSin[i].close();

			if (msg)
			{
				//convert with gdal_translate to optimize size
				string gdal_data_path = GetApplicationPath() + "External\\gdal-data";
				string projlib_path = GetApplicationPath() + "External\\projlib";

				//-stats : do not include stat to avoid the creation of the xml file
				string option = "--config GDAL_DATA \"" + gdal_data_path + "\" --config PROJ_LIB \"" + projlib_path + "\"";
				string argument = "-unscale -ot Float32 -a_nodata 9999 -co COMPRESS=LZW -co PREDICTOR=3 -co TILED=YES -co BLOCKXSIZE=256 -co BLOCKYSIZE=256";
				string command = "\"" + GetApplicationPath() + "External\\gdal_translate.exe\" " + option + " " + argument + " \"" + file_path_out + "2" + "\" \"" + file_path_out + "\"";

				//string argument = "-ot Float32 -a_nodata 9999 -stats -co COMPRESS=LZW -co PREDICTOR=3 -co TILED=YES -co BLOCKXSIZE=256 -co BLOCKYSIZE=256 \"" + file_path_out + "2" + "\" \"" + file_path_out + "\"";
				//string command = "\"" + GetApplicationPath() + "External\\gdal_translate.exe\" " + argument;
				msg += WinExecWait(command);
				msg += RemoveFile(file_path_out + "2");
			}


		}//if msg


		return msg;
	}


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
		//		CTRef UTCRef = GetRemoteTRef(name);
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
		//						if (station[accumulator.GetRemoteTRef()].GetVariables().none() )//don't override observation
		//							station[accumulator.GetRemoteTRef()].SetData(accumulator);
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

		//							if (v == H_SRAD)
		//								value *= 3600;//J/m² --> W/m²

		//							if (v == H_PRES)
		//								value /= 100;//Pa --> hPa

		//							if (v == H_WNDS)
		//								value *= 3600/1000;//Pa --> hPa


		//							if (v == H_TAIR)
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

		//				if (accumulator.GetRemoteTRef().IsInit())
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

	ERMsg CHRRR::GetGribsList(CTPeriod p, CGribsMap& gribsList, CCallback& callback)
	{
		ERMsg msg;

		int firstYear = p.Begin().GetYear();
		int lastYear = p.End().GetYear();
		size_t nbYears = lastYear - firstYear + 1;

		for (size_t y = 0; y < nbYears; y++)
		{
			int year = firstYear + int(y);

			StringVector list1;
			string filter = m_compute_prcp ? "\\*f00.tif" : "\\*f00.grib2";
			list1 = GetFilesList(m_workingDir + ToString(year) + filter, FILE_PATH, true);

			for (size_t i = 0; i < list1.size(); i++)
			{
				CTRef TRef = GetLocalTRef(list1[i]);
				if (p.IsInside(TRef))
					gribsList[TRef] = list1[i];
			}

		}



		return msg;
	}

	CTRef CHRRR::GetLocalTRef(string filePath)
	{
		CTRef TRef;

		string name = GetFileTitle(filePath);
		filePath = GetPath(filePath);
		string dir1 = WBSF::GetLastDirName(filePath);
		while (WBSF::IsPathEndOk(filePath))
			filePath = filePath.substr(0, filePath.length() - 1);
		filePath = GetPath(filePath);
		string dir2 = WBSF::GetLastDirName(filePath);
		while (WBSF::IsPathEndOk(filePath))
			filePath = filePath.substr(0, filePath.length() - 1);
		filePath = GetPath(filePath);
		string dir3 = WBSF::GetLastDirName(filePath);

		int year = WBSF::as<int>(dir3);
		size_t m = WBSF::as<int>(dir2) - 1;
		size_t d = WBSF::as<int>(dir1) - 1;
		size_t h = WBSF::as<int>(name.substr(6, 2));
		TRef = CTRef(year, m, d, h);

		return TRef;
	}



}