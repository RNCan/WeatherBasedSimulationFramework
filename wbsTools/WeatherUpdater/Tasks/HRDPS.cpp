#include "StdAfx.h"
#include "HRDPS.h"
#include "Basic/WeatherStation.h"
#include "UI/Common/SYShowMessage.h"
#include "Geomatic/ShapeFileBase.h"
#include "TaskFactory.h"
#include "Geomatic/TimeZones.h"
#include "boost/tokenizer.hpp"
#include "Geomatic/SfcGribsDatabase.h"

//#include "cctz\time_zone.h"
#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"

#include "WeatherBasedSimulationString.h"
#include "../Resource.h"

#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;
using namespace boost;

namespace WBSF
{

	//precipitation historique 10 km
	//http://collaboration.cmc.ec.gc.ca/science/outgoing/capa.grib/

	//HRDPA:
	//http://dd.weather.gc.ca/analysis/precip/hrdpa/grib2/polar_stereographic/24/

	//*********************************************************************
	const char* CHRDPS::SERVER_NAME = "dd.weather.gc.ca";
	const char* CHRDPS::SERVER_PATH = "/model_hrdps/continental/grib2/";
	static const size_t MAX_FORECAST_HOURS = 48;


	CHRDPS::CHRDPS(const std::string& workingDir) :
		m_workingDir(workingDir),
		m_bCreateDailyGeotiff(true),
		//m_createHistiricalGeotiff(true),
		m_update_last_n_days(7),
		m_bForecast(false),
		m_max_hours(24),
		m_bHRDPA6h(false)
	{
		m_variables.set();
		m_levels.FromString("1015|1000|0985|0970|0950|0925|0900|0875|0850|0800|0750");//for ISBL var
		m_heights.FromString("2|10|40|80|120");//for TGL var
	}

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
		CTRef TRef = CHRDPS::GetTRef(fileName, false);
		return FormatA("%s%d\\%02d\\%02d\\%02d\\%s", m_workingDir.c_str(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, TRef.GetHour(), fileName.c_str());
		//return 	GetOutputFilePath(CTRef, size_t HH);
	}


	string CHRDPS::GetOutputFilePath(CTRef TRef, size_t HH)const
	{
		return FormatA("%s%d\\%02d\\%02d\\HRDPS_%4d%02d%02d%02d-%03d.tif", m_workingDir.c_str(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, TRef.GetHour(), HH);
	}


	size_t CHRDPS::GetHRDPSVariable(string title)
	{
		//CMC_hrdps_continental_ABSV_ISBL_0250_ps2.5km_2016122918_P000-00
		WBSF::ReplaceString(title, "GUST_MAX", "GUST-MAX");
		WBSF::ReplaceString(title, "GUST_MIN", "GUST-MIN");

		StringVector parts(title, "_");
		ASSERT(parts.size() == 9);
		string name = parts[3] + "_" + parts[4];
		WBSF::ReplaceString(name, "GUST-MAX", "GUST_MAX");
		WBSF::ReplaceString(name, "GUST-MIN", "GUST_MIN");

		return CHRDPSVariables::GetVariable(name);
	}

	size_t CHRDPS::GetLevel(string title)
	{
		WBSF::ReplaceString(title, "GUST_MAX", "GUST-MAX");
		WBSF::ReplaceString(title, "GUST_MIN", "GUST-MIN");

		//CMC_hrdps_continental_ABSV_ISBL_0250_ps2.5km_2016122918_P000-00
		StringVector parts(title, "_");
		ASSERT(parts.size() == 9);
		return CHRDPSVariables::GetLevel(parts[5]);
	}



	ERMsg CHRDPS::GetLatestHH(size_t& HH, CCallback& callback)const
	{
		ERMsg msg;

		HH = NOT_INIT;

		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg = GetHttpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);
		if (msg)
		{
			vector<pair<CTRef, size_t>> latest;
			for (size_t h = 0; h < 24; h += 6)
			{
				string remotePath = GetRemoteFilePath(h, 0, "*P000-00.grib2");

				CFileInfoVector fileListTmp;
				if (FindFiles(pConnection, remotePath, fileListTmp) && !fileListTmp.empty())
				{
					for (size_t i = 0; i < fileListTmp.size(); i++)
						latest.push_back(make_pair(GetTRef(fileListTmp[i].m_filePath, false), h));
				}
			}

			sort(latest.begin(), latest.end());

			if (!latest.empty())
				HH = latest.back().second;

		}

		return msg;
	}

	//****************************************************************************************************
	//ERMsg UpdateAll(CCallback& callback)
	//{
	//	ERMsg msg;

	//	string working_dir = "Z:/HRG/HRDPS(SFC)/";
	//	StringVector file_to_update;
	//	vector<pair<string, string>> year_month;
	//	StringVector years = WBSF::GetDirectoriesList(working_dir + "*");

	//	for (StringVector::const_iterator it1 = years.begin(); it1 != years.end() && msg; it1++)
	//	{
	//		string year = *it1;
	//		if (ToInt(year) > 1900 && ToInt(year) < 2100)
	//		{
	//			StringVector months = WBSF::GetDirectoriesList(working_dir + *it1 + "\\*");
	//			for (StringVector::const_iterator it2 = months.begin(); it2 != months.end() && msg; it2++)
	//				year_month.push_back(make_pair(*it1, *it2));
	//		}
	//	}

	//	callback.PushTask("Get all file to update (" + to_string(year_month.size()) + " months)", year_month.size());
	//	callback.AddMessage("Get all file to update (" + to_string(year_month.size()) + " months)");


	//	for (auto it = year_month.begin(); it != year_month.end() && msg; it++)
	//	{
	//		string year = it->first;
	//		string month = it->second;
	//		if (month != "06")
	//		{
	//			StringVector days = WBSF::GetDirectoriesList(working_dir + year + "\\" + month + "\\*");
	//			for (StringVector::const_iterator it3 = days.begin(); it3 != days.end() && msg; it3++)
	//			{
	//				string day = *it3;

	//				string filter = working_dir + year + "\\" + month + "\\" + day + "\\HRDPS_*.tif";
	//				StringVector file = WBSF::GetFilesList(filter, 2);
	//				file_to_update.insert(file_to_update.end(), file.begin(), file.end());
	//				msg += callback.StepIt(0);
	//			}//for all days
	//		}
	//		msg += callback.StepIt();
	//	}//for all months

	//	callback.PopTask();

	//	callback.PushTask("Convert files(" + to_string(year_month.size()) + " months)", file_to_update.size());
	//	

	//	for (size_t i = 0; i != file_to_update.size() && msg; i++)
	//	{
	//		//string outputFilePath = file_to_update[i] + "2";

	//		//if (GoodGrib(file_to_update[i]))
	//		//{
	//		CSfcData1d DSin;
	//		DSin.m_variables_to_load.set();
	//		msg += DSin.open(file_to_update[i], true);

	//		if (msg)
	//		{
	//			CBaseOptions options;
	//			DSin.UpdateOption(options);
	//			//options.m_nbBands = nbBands;
	//			//options.m_outputType = GDT_Float32;
	//			//options.m_dstNodata = GetDefaultNoData(GDT_Float32);
	//			options.m_bOverwrite = true;

	//			CGDALDatasetEx DSout;
	//			msg += DSout.CreateImage(file_to_update[i] + "2", options);
	//			if (msg)
	//			{

	//				for (size_t v = 0; v < DSin.get_variables().size(); v++)
	//				{
	//					size_t b = DSin.get_band(v);

	//					if (b != NOT_INIT)
	//					{
	//						GDALRasterBand* pBandin = DSin.GetRasterBand(b);
	//						GDALRasterBand* pBandout = DSout.GetRasterBand(b);

	//						ASSERT(DSin.GetRasterXSize() == DSout.GetRasterXSize());
	//						ASSERT(DSin.GetRasterYSize() == DSout.GetRasterYSize());

	//						vector<float> data(DSin.GetRasterXSize()*DSin.GetRasterYSize());
	//						pBandin->RasterIO(GF_Read, 0, 0, DSin.GetRasterXSize(), DSin.GetRasterYSize(), &(data[0]), DSin.GetRasterXSize(), DSin.GetRasterYSize(), GDT_Float32, 0, 0);
	//						pBandout->RasterIO(GF_Write, 0, 0, DSin.GetRasterXSize(), DSin.GetRasterYSize(), &(data[0]), DSin.GetRasterXSize(), DSin.GetRasterYSize(), GDT_Float32, 0, 0);

	//						if (v == H_VWND)
	//						{
	//							//change v wind by geopot
	//							
	//							pBandout->SetDescription(CHRDPS::CSfcGribDatabase::META_DATA[H_GHGT][CHRDPS::M_DESC]);
	//							pBandout->SetMetadataItem("GRIB_COMMENT", CHRDPS::CSfcGribDatabase::META_DATA[H_GHGT][CHRDPS::M_COMMENT]);
	//							pBandout->SetMetadataItem("GRIB_ELEMENT", CHRDPS::CSfcGribDatabase::META_DATA[H_GHGT][CHRDPS::M_ELEMENT]);
	//							pBandout->SetMetadataItem("GRIB_SHORT_NAME", CHRDPS::CSfcGribDatabase::META_DATA[H_GHGT][CHRDPS::M_SHORT_NAME]);
	//							pBandout->SetMetadataItem("GRIB_UNIT", CHRDPS::CSfcGribDatabase::META_DATA[H_GHGT][CHRDPS::M_UNIT]);

	//						}
	//						else
	//						{
	//							string test1 = pBandin->GetDescription();
	//							if (pBandin->GetDescription())
	//								pBandout->SetDescription(pBandin->GetDescription());

	//							string test2 = *pBandin->GetMetadata();
	//							if (pBandin->GetMetadata())
	//								pBandout->SetMetadata(pBandin->GetMetadata());
	//						}
	//					}

	//					//							DSout->FlushCache();

	//				}

	//				DSout.Close();
	//			}//out open

	//			DSin.close();

	//			if (msg)
	//			{
	//				//do not support block
	//				string argument = "-ot Float32 -co COMPRESS=LZW -co PREDICTOR=3 -co TILED=YES -co BLOCKXSIZE=256 -co BLOCKYSIZE=256 \"" + file_to_update[i] + "2" + "\" \"" + file_to_update[i] + "\"";
	//				string command = "\"" + GetApplicationPath() + "External\\gdal_translate.exe\" " + argument;
	//				msg += WinExecWait(command);
	//				msg += RemoveFile(file_to_update[i] + "2");
	//			}
	//		}//in open

	//		msg += callback.StepIt();
	//	}// for all files

	//	callback.PopTask();

	//	return msg;
	//}


	ERMsg CHRDPS::Execute(CCallback& callback)
	{
		//set<string> t;
		//t.insert("20200101");
		//return CreateHourlyGeotiff(t, callback);

		GDALSetCacheMax64(128 * 1024 * 1024);


		ERMsg msg;

		set<string> date_to_update;

		//download HRDPA 
		if (m_variables.test(APCP_SFC) && m_bHRDPA6h && !m_bForecast)
		{
			CHRDPA HRDPA;
			HRDPA.m_bByHRDPS = true;
			HRDPA.m_workingDir = m_workingDir;
			HRDPA.m_type = CHRDPA::TYPE_06HOURS;
			HRDPA.m_product = CHRDPA::HRDPA;
			HRDPA.m_max_hours = 48;

			//only add message in case of error, continue anyway
			StringVector HRDPAFiles;
			ERMsg msg_tmp = HRDPA.Execute(callback, HRDPAFiles);
			if (!msg_tmp)
				callback.AddMessage(msg_tmp);

			callback.AddMessage("");

			//rebuild GeoTiff with the new HRDPA
			for (size_t i = 0; i < HRDPAFiles.size(); i++)
			{
				CTRef TRef = CHRDPA::GetTRef(HRDPAFiles[i]) - 6;
				date_to_update.insert(TRef.GetFormatedString("%Y%m%d"));
			}
		}

		callback.AddMessage("Updating HRDPS");
		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(m_workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(SERVER_NAME, 1);
		callback.AddMessage("--------------------------");


		size_t lastestHH = NOT_INIT;
		msg = GetLatestHH(lastestHH, callback);

		if (!msg)
			return msg;

		callback.PushTask(string("Get directories list from: ") + SERVER_PATH, 4);



		CFileInfoVector dir1;
		CFileInfoVector dir2;


		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg = GetHttpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);
		if (!msg)
			return msg;


		try
		{
			msg = FindDirectories(pConnection, SERVER_PATH, dir1);// 00, 06, 12, 18

			for (CFileInfoVector::const_iterator it1 = dir1.begin(); it1 != dir1.end() && msg; it1++)
			{
				size_t HH = as<size_t>(GetLastDirName(it1->m_filePath));


				CFileInfoVector tmp;
				msg = FindDirectories(pConnection, it1->m_filePath, tmp);//000 to ~048
				for (CFileInfoVector::const_iterator it2 = tmp.begin(); it2 != tmp.end() && msg; it2++)
				{
					string path = it2->m_filePath;
					path = GetPath(path.substr(0, path.length() - 1));
					size_t HH = as<size_t>(GetLastDirName(path));
					size_t hhh = as<size_t>(GetLastDirName(it2->m_filePath));

					bool bDownload = m_bForecast ? (HH == lastestHH) : (hhh < 7);
					if (bDownload)
						dir2.push_back(*it2);
				}
				//				dir2.insert(dir2.end(), tmp.begin(), tmp.end());
				msg += callback.StepIt();
			}
		}
		catch (CException* e)
		{
			msg = UtilWin::SYGetMessage(*e);
		}

		pConnection->Close();
		pSession->Close();

		callback.PopTask();

		CTRef nowUTC = CTRef::GetCurrentTRef(CTM::HOURLY, true);

		CFileInfoVector fileList;
		if (msg)
		{

			callback.PushTask(string("Get files list from: ") + SERVER_PATH + " (" + to_string(dir2.size()) + " directories)", dir2.size());
			size_t nbTry = 0;

			CFileInfoVector::const_iterator it2 = dir2.begin();
			while (it2 != dir2.end() && msg)
			{
				nbTry++;

				CInternetSessionPtr pSession;
				CHttpConnectionPtr pConnection;
				msg = GetHttpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);

				if (msg)
				{
					pSession->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 15000);
					try
					{
						while (it2 != dir2.end() && msg)
						{
							size_t hhh = as<size_t>(GetLastDirName(it2->m_filePath));
							/*string path = it2->m_filePath;
							path = GetPath(path.substr(0, path.length() - 1));
							size_t HH = as<size_t>(GetLastDirName(path));
							size_t hhh0 = as<size_t>(GetLastDirName(it2->m_filePath));
							size_t hhh = Gethhh(GetFileTitle(it2->m_filePath));
							ASSERT(hhh=hhh0);*/
							//	bool bDownload = m_bForecast ? (HH == lastestHH) : (hhh < 7);
								//if (bDownload)
								//{
							CFileInfoVector fileListTmp;
							msg = FindFiles(pConnection, it2->m_filePath + "*.grib2", fileListTmp);
							for (CFileInfoVector::iterator it = fileListTmp.begin(); it != fileListTmp.end() && msg; it++)
							{
								string fileName = GetFileName(it->m_filePath);
								string outputFilePath = GetOutputFilePath(fileName);
								size_t var = GetHRDPSVariable(fileName);
								size_t level = GetLevel(fileName);
								CTRef TRefUTC = GetTRef(fileName, true);
								int forecastHours = TRefUTC - nowUTC;

								if (var < m_variables.size())
								{
									if (m_variables.test(var))
									{
										bool bKeep1 = !m_variables.Is(var, HRDPS_TGL) || m_heights.find(level) != m_heights.end();
										bool bKeep2 = !m_variables.Is(var, HRDPS_ISBL) || m_levels.find(level) != m_levels.end();
										bool bKeep3 = m_bForecast ? (forecastHours >= 0 && forecastHours <= m_max_hours) : (var == APCP_SFC || var == DSWRF_SFC || hhh < 6);

										if (bKeep1 && bKeep2 && bKeep3)
										{
											if (NeedDownload(outputFilePath))
											{
												fileList.push_back(*it);
											}
										}
									}//if wanted variable
								}//Humm new variable
								else
								{
									callback.AddMessage("Unknowns HRDPS var: " + fileName);
								}
							}//for all files
						//}//take only 6 first hours

							msg += callback.StepIt();
							it2++;
							nbTry = 0;
						}//for all directory

					}
					catch (CException* e)
					{
						if (nbTry < 5)
						{
							callback.AddMessage(UtilWin::SYGetMessage(*e));
							msg += WaitServer(30, callback);
						}
						else
						{
							msg = UtilWin::SYGetMessage(*e);
						}
					}

					pConnection->Close();
					pSession->Close();

				}
			}

			callback.PopTask();

		}//if msg


		if (msg)
		{

			callback.PushTask("Download HRDPS gribs (" + ToString(fileList.size()) + ")", fileList.size());
			callback.AddMessage("Number of HRDPS gribs to download: " + ToString(fileList.size()));

			size_t nbDownload = 0;
			size_t nbTry = 0;
			CFileInfoVector::iterator it = fileList.begin();
			while (it != fileList.end() && msg)
			{
				nbTry++;

				CInternetSessionPtr pSession;
				CHttpConnectionPtr pConnection;
				msg = GetHttpConnection(SERVER_NAME, pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);

				if (msg)
				{
					pSession->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 15000);
					pSession->SetOption(INTERNET_OPTION_DATA_RECEIVE_TIMEOUT, 15000);

					try
					{
						while (it != fileList.end() && msg)
						{
							string fileName = GetFileName(it->m_filePath);
							string outputFilePath = GetOutputFilePath(fileName);

							CreateMultipleDir(GetPath(outputFilePath));
							msg = CopyFile(pConnection, it->m_filePath, outputFilePath, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_EXISTING_CONNECT);
							if (msg)
							{
								ASSERT(GoodGrib(outputFilePath));
								nbDownload++;
								it++;
								nbTry = 0;
								msg += callback.StepIt();
								string tif_file_path = GetGeotiffFilePath(outputFilePath);

								date_to_update.insert(GetFileTitle(tif_file_path).substr(6, 8));
							}
						}
					}
					catch (CException* e)
					{
						if (nbTry < 5)
						{
							callback.AddMessage(UtilWin::SYGetMessage(*e));
							msg += WaitServer(30, callback);
						}
						else
						{
							msg = UtilWin::SYGetMessage(*e);
						}
					}

					pConnection->Close();
					pSession->Close();
				}

			}

			callback.AddMessage("Number of HRDPS gribs downloaded: " + ToString(nbDownload));
			callback.PopTask();
		}

		/*for (int m = 1; m <= 9; m++)
		{
			for (int d = 1; d <= GetNbDayPerMonth(m - 1); d++)
			{
				string file_path_out = FormatA("%s2019\\%02d\\%02d\\HRDPSD_2019%02d%02d.tif", m_workingDir.c_str(), m, d, m, d);
				if (!WBSF::FileExists(file_path_out))
					date_to_update.insert(FormatA("2019%02d%02d", m, d));
			}
		}


		msg = CreateDailyGeotiff(date_to_update, callback);
		return msg;*/

		if (/*date_to_update.empty() &&*/ m_update_last_n_days > 0)
		{
			set<string> last_n_days = Getlast_n_days(m_update_last_n_days, callback);
			date_to_update.insert(last_n_days.begin(), last_n_days.end());
		}

		if (!date_to_update.empty())
		{
			if (msg && m_variables.test(APCP_SFC))
				msg = CreateHourlyPrcp(date_to_update, callback);

			if (msg && m_variables.test(DSWRF_SFC))
				msg = CreateHourlySRad(date_to_update, callback);

			//now, create GeoTiff
			if (msg)
			{
				msg += CreateHourlyGeotiff(date_to_update, callback);
			}

			//now, create daily GeoTiff
			if (msg && m_bCreateDailyGeotiff)
				msg = CreateDailyGeotiff(date_to_update, callback);
		}

		return msg;
	}

	string CHRDPS::GetGeotiffFilePath(std::string workingDir, string outputFilePath)
	{
		string title = GetFileTitle(outputFilePath);
		size_t pos = title.find("2.5km_");
		ASSERT(pos != string::npos);

		string year = title.substr(pos + 6, 4);
		string month = title.substr(pos + 10, 2);
		string day = title.substr(pos + 12, 2);
		size_t h1 = WBSF::as<size_t>(title.substr(pos + 14, 2));
		size_t h2 = WBSF::as<size_t>(title.substr(pos + 18, 3));

		string tifFilePath = FormatA("%s%s\\%s\\%s\\HRDPS_%s%s%s%02d-%03d.tif", workingDir.c_str(), year.c_str(), month.c_str(), day.c_str(), year.c_str(), month.c_str(), day.c_str(), h1, h2);

		return tifFilePath;
	}



	ERMsg CHRDPS::CreateHourlyGeotiff(set<string> outputPath, CCallback& callback)
	{
		ERMsg msg;

		//CTRef now = CTRef::GetCurrentTRef(CTM::HOURLY);
		size_t nbHours = m_bForecast ? 48 : 6;

		//Compute the number of hours to update
		std::map< string, StringVector>  to_update_map;

		callback.PushTask("Find HRDPS hourly GeoTiff to update: " + ToString(outputPath.size()) + " days", outputPath.size() * 4 * nbHours);
		//callback.AddMessage("Create hourly GeoTiff: " + ToString(outputPath.size()) + " days");

		for (set<string>::const_iterator it = outputPath.begin(); it != outputPath.end() && msg; it++)
		{
			string year = it->substr(0, 4);
			string month = it->substr(4, 2);
			string day = it->substr(6, 2);



			for (size_t h1 = 0; h1 < 24 && msg; h1 += 6)
			{

				for (size_t h2 = 0; h2 < nbHours && msg; h2++)
				{
					CTRef TRef(ToInt(year), ToSizeT(month) - 1, ToSizeT(day) - 1, h1);
					CTRef TRef2 = (h2 == 0) ? (TRef - 6) : TRef;
					size_t H2 = (h2 == 0) ? 6 : h2;

					string filter1 = FormatA("%s%04d\\%02d\\%02d\\%02d\\*%04d%02d%02d%02d_P%03d-00.grib2", m_workingDir.c_str(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, TRef.GetHour(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, TRef.GetHour(), h2);
					string filter2 = FormatA("%s%04d\\%02d\\%02d\\%02d\\*%04d%02d%02d%02d_P%03d-00.tif", m_workingDir.c_str(), TRef2.GetYear(), TRef2.GetMonth() + 1, TRef2.GetDay() + 1, TRef2.GetHour(), TRef2.GetYear(), TRef2.GetMonth() + 1, TRef2.GetDay() + 1, TRef2.GetHour(), H2);

					StringVector fileList1 = WBSF::GetFilesList(filter1, 2, true);
					StringVector fileList2 = WBSF::GetFilesList(filter2, 2, true);
					__time64_t lastUpdate = -1;

					//remove prcp and invalid file
					for (StringVector::iterator it4 = fileList1.begin(); it4 != fileList1.end() && msg; )
					{
						if (GoodGrib(*it4))
						{
							lastUpdate = max(lastUpdate, GetFileInfo(*it4).m_time);
							string fileName = GetFileName(*it4);
							if (WBSF::Find(fileName, "APCP_SFC") || WBSF::Find(fileName, "DSWRF_SFC"))
								it4 = fileList1.erase(it4);
							else
								it4++;
						}
						else
						{
							callback.AddMessage("Remove invalid grib " + *it4);
							msg += RemoveFile(*it4);
							it4 = fileList1.erase(it4);
						}
					}

					//remove prcp and invalid file
					for (StringVector::iterator it4 = fileList2.begin(); it4 != fileList2.end() && msg; )
					{
						CGDALDatasetEx DS;
						if (DS.OpenInputImage(*it4))
						{
							DS.Close();
							lastUpdate = max(lastUpdate, GetFileInfo(*it4).m_time);
							it4++;
						}
						else
						{
							callback.AddMessage("Remove invalid GeoTiff " + *it4);
							msg += RemoveFile(*it4);
							it4 = fileList2.erase(it4);
						}
					}


					string file_path_tif = FormatA("%s%s\\%s\\%s\\HRDPS_%s%s%s%02d-%03d.tif", m_workingDir.c_str(), year.c_str(), month.c_str(), day.c_str(), year.c_str(), month.c_str(), day.c_str(), h1, h2);
					//string file_path_tif_new = FormatA("%s%s\\%s\\%s\\HRDPS_%s%s%s%02d.tif", m_workingDir.c_str(), year.c_str(), month.c_str(), day.c_str(), year.c_str(), month.c_str(), day.c_str(), h);
					//if (FileExists(file_path_tif_old))
					//	RenameFile(file_path_tif_old, file_path_tif_new);

					//verify that the file is to update
					if ((!fileList1.empty() || !fileList2.empty()) &&
						GetFileInfo(file_path_tif).m_time < lastUpdate)
					{
						size_t h = h1 + h2;
						string key = FormatA("%s%02d", it->c_str(), h);
						to_update_map[key] = fileList1;
						to_update_map[key].insert(to_update_map[key].end(), fileList2.begin(), fileList2.end());
						sort(to_update_map[key].begin(), to_update_map[key].end());
					}

					msg += callback.StepIt();
				}//h2
			}//h1

		}//for all days

		callback.PopTask();


		//string nbHours_test = FormatA("%s%s\\%s\\%s\\CMC_hrdps_continental_TMP_TGL_2_ps2.5km_*.grib2", m_workingDir.c_str(), year.c_str(), month.c_str(), day.c_str());
		//StringVector nbHours = WBSF::GetFilesList(nbHours_test, 2, true);
		callback.PushTask("Create HRDPS hourly GeoTiff: " + ToString(to_update_map.size()) + " hours", to_update_map.size());
		callback.AddMessage("Create HRDPS hourly GeoTiff: " + ToString(to_update_map.size()) + " hours");


		for (auto it = to_update_map.begin(); it != to_update_map.end() && msg; it++)
		{

			string date = it->first;
			string year = date.substr(0, 4);
			string month = date.substr(4, 2);
			string day = date.substr(6, 2);
			string hour = date.substr(8, 2);

			size_t h = ToSizeT(hour);
			size_t h1 = size_t(h / 6) * 6;
			size_t h2 = h % 6;


			StringVector fileList = it->second;

			//string file_path_vrt = FormatA("%s%s\\%s\\%s\\HRDPS_%s%s%s%02d.vrt", m_workingDir.c_str(), year.c_str(), month.c_str(), day.c_str(), year.c_str(), month.c_str(), day.c_str(), h);
			string file_path_vrt = FormatA("%s%s\\%s\\%s\\HRDPS_%s%s%s%02d-%03d.vrt", m_workingDir.c_str(), year.c_str(), month.c_str(), day.c_str(), year.c_str(), month.c_str(), day.c_str(), h1, h2);
			string file_path_tif = file_path_vrt; SetFileExtension(file_path_tif, ".tif");

			//verify that the file is to update
			map< size_t, string> pos;
			for (StringVector::iterator it4 = fileList.begin(); it4 != fileList.end() && msg; it4++)
			{
				string fileName = GetFileName(*it4);
				string title = GetFileTitle(fileName);
				StringVector tmp(title, "_");
				ASSERT(tmp.size() == 9);

				string strVar = tmp[3];
				size_t var = CSfcDatasetCached::get_var(strVar);

				if (var != NOT_INIT)
					pos[var] = *it4;
			}

			string vrt_path = GetPath(file_path_vrt);

			ofStream oFile;
			msg = oFile.open(file_path_vrt);
			if (msg)
			{
				oFile << "<VRTDataset rasterXSize=\"2576\" rasterYSize=\"1456\">" << endl;
				oFile << "  <SRS>PROJCS[\"unnamed\",GEOGCS[\"Coordinate System imported from GRIB file\",DATUM[\"unknown\",SPHEROID[\"Sphere\",6371229,0]],PRIMEM[\"Greenwich\",0],UNIT[\"degree\",0.0174532925199433]],PROJECTION[\"Polar_Stereographic\"],PARAMETER[\"latitude_of_origin\",60],PARAMETER[\"central_meridian\",252],PARAMETER[\"scale_factor\",90],PARAMETER[\"false_easting\",0],PARAMETER[\"false_northing\",0]]</SRS>" << endl;
				oFile << "  <GeoTransform>-2.0991274944969425e+006, 2.5000000000000000e+003, 0.0000000000000000e+000,-2.0993885214996245e+006, 0.0000000000000000e+000,-2.5000000000000000e+003</GeoTransform>" << endl;

				int b = 1;
				//for (StringVector::iterator it4 = fileList.begin(); it4 != fileList.end() && msg; it4++)
				for (auto it4 = pos.begin(); it4 != pos.end() && msg; it4++, b++)
				{
					size_t var = it4->first;

					string fileName = GetFileName(it4->second);
					string title = GetFileTitle(fileName);
					string relFileName = GetRelativePath(vrt_path, it4->second);

					oFile << "  <VRTRasterBand dataType=\"Float64\" band=\"" << ToString(b) << "\">" << endl;

					oFile << "    <Description>" << CSfcGribDatabase::META_DATA[var][M_DESC] << "</Description>" << endl;
					oFile << "    <Metadata>" << endl;
					oFile << "      <MDI key=\"GRIB_COMMENT\">" << CSfcGribDatabase::META_DATA[var][M_COMMENT] << "</MDI>" << endl;
					oFile << "      <MDI key=\"GRIB_ELEMENT\">" << CSfcGribDatabase::META_DATA[var][M_ELEMENT] << "</MDI>" << endl;
					oFile << "      <MDI key=\"GRIB_SHORT_NAME\">" << CSfcGribDatabase::META_DATA[var][M_SHORT_NAME] << "</MDI>" << endl;
					oFile << "      <MDI key=\"GRIB_UNIT\">" << CSfcGribDatabase::META_DATA[var][M_UNIT] << "</MDI>" << endl;
					oFile << "    </Metadata>" << endl;

					oFile << "    <NoDataValue>9999</NoDataValue>" << endl;
					oFile << "    <ComplexSource>" << endl;
					oFile << "      <SourceFilename relativeToVRT=\"1\">" << relFileName << "</SourceFilename>" << endl;
					oFile << "      <SourceBand>1</SourceBand>" << endl;
					oFile << "      <SourceProperties RasterXSize=\"2576\" RasterYSize=\"1456\" DataType=\"Float64\" BlockXSize=\"2576\" BlockYSize=\"1\" />" << endl;
					oFile << "      <SrcRect xOff=\"0\" yOff=\"0\" xSize=\"2576\" ySize=\"1456\" />" << endl;
					oFile << "      <DstRect xOff=\"0\" yOff=\"0\" xSize=\"2576\" ySize=\"1456\" />" << endl;
					oFile << "      <NODATA>9999</NODATA>" << endl;
					oFile << "    </ComplexSource>" << endl;
					oFile << "  </VRTRasterBand>" << endl;

				}

				oFile << "</VRTDataset>" << endl;
				oFile.close();

				//Create GeoTiff from vrt


				string prj4 = "+proj=stere +lat_0=90 +lat_ts=60 +lon_0=252 +x_0=0 +y_0=0 +R=6371229 +units=m +no_defs";
				string argument = "-stats -ot Float32 -co COMPRESS=LZW -co PREDICTOR=3 -co TILED=YES -co BLOCKXSIZE=256 -co BLOCKYSIZE=256 -a_srs \"" + prj4 + "\"";
				string command = "\"" + GetApplicationPath() + "External\\gdal_translate.exe\" " + argument + " \"" + file_path_vrt + "\" \"" + file_path_tif + "\"";
				msg += WinExecWait(command);

				//remove vrt
				msg += RemoveFile(file_path_vrt);
			}//if msg

			msg += callback.StepIt();
		}//for all hours

		callback.PopTask();

		return msg;
	}

	ERMsg CHRDPS::GetPrcpHourToUpdate(set<string> date_to_update, vector < CPrcpHourToUpdate>& hour_to_update, CCallback& callback)const
	{
		ERMsg msg;

		hour_to_update.clear();
		callback.PushTask("Get hourly precipitation to update", date_to_update.size() * 4);

		for (set<string>::const_iterator it = date_to_update.begin(); it != date_to_update.end() && msg; it++)
		{
			string year = it->substr(0, 4);
			string month = it->substr(4, 2);
			string day = it->substr(6, 2);

			CTRef date(ToInt(it->substr(0, 4)), ToSizeT(it->substr(4, 2)) - 1, ToSizeT(it->substr(6, 2)) - 1);
			//
			for (size_t h1 = 0; h1 < 24 && msg; h1 += 6)
			{
				CTRef TRef = CTRef(date.GetYear(), date.GetMonth(), date.GetDay(), h1) + 6;
				string HRDPA_file_path = FormatA("%s%s\\%s\\%s\\%02d\\CMC_HRDPA_APCP-006-0700cutoff_SFC_0_ps2.5km_%4d%02d%02d%02d_000.tif", m_workingDir.c_str(), year.c_str(), month.c_str(), day.c_str(), h1, TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, TRef.GetHour());

				bool bContinue = m_bHRDPA6h ? FileExists(HRDPA_file_path) : true;
				if (bContinue)
				{
					size_t nb_hours = 0;
					size_t last_hour = 0;
					string HRDPS_file_path_last;

					size_t h2 = 1;
					string HRDPS_file_path = FormatA("%s%s\\%s\\%s\\%02d\\CMC_hrdps_continental_APCP_SFC_0_ps2.5km_%s%s%s%02d_P%03d-00.grib2", m_workingDir.c_str(), year.c_str(), month.c_str(), day.c_str(), h1, year.c_str(), month.c_str(), day.c_str(), h1, h2);
					while (FileExists(HRDPS_file_path))
					{
						nb_hours++;
						last_hour = h2;
						HRDPS_file_path_last = HRDPS_file_path;
						h2++;
						HRDPS_file_path = FormatA("%s%s\\%s\\%s\\%02d\\CMC_hrdps_continental_APCP_SFC_0_ps2.5km_%s%s%s%02d_P%03d-00.grib2", m_workingDir.c_str(), year.c_str(), month.c_str(), day.c_str(), h1, year.c_str(), month.c_str(), day.c_str(), h1, h2);
						msg += callback.StepIt(0);
					}

					////create hourly precipitation
					for (size_t h2 = 1; h2 <= last_hour && msg; h2++)
					{
						string HRDPS_file_path = FormatA("%s%s\\%s\\%s\\%02d\\CMC_hrdps_continental_APCP_SFC_0_ps2.5km_%s%s%s%02d_P%03d-00.grib2", m_workingDir.c_str(), year.c_str(), month.c_str(), day.c_str(), h1, year.c_str(), month.c_str(), day.c_str(), h1, h2);
						string out_file_path = HRDPS_file_path;
						SetFileExtension(out_file_path, ".tif");
						//if (!FileExists(out_file_path))//create file only if they are not already created
						CGDALDatasetEx DS;
						if (DS.OpenInputImage(out_file_path))
						{
							DS.Close();
						}
						else
						{
							if (FileExists(out_file_path))
							{
								callback.AddMessage("Remove invalid HRDPS GeoTiff " + out_file_path);
								msg += RemoveFile(out_file_path);
							}

							string HRDPS_file_path_prev = FormatA("%s%s\\%s\\%s\\%02d\\CMC_hrdps_continental_APCP_SFC_0_ps2.5km_%s%s%s%02d_P%03d-00.grib2", m_workingDir.c_str(), year.c_str(), month.c_str(), day.c_str(), h1, year.c_str(), month.c_str(), day.c_str(), h1, h2 - 1);

							CPrcpHourToUpdate pfp;
							pfp.m_date = date;
							pfp.m_nb_hours = nb_hours;
							pfp.m_last_hour = last_hour;
							pfp.m_HRDPA_file_path = HRDPA_file_path;

							pfp.m_HRDPS_file_path_last = HRDPS_file_path_last;
							pfp.m_HRDPS_file_path1 = h2 == 1 ? "" : HRDPS_file_path_prev;
							pfp.m_HRDPS_file_path2 = HRDPS_file_path;
							pfp.m_out_file_path = out_file_path;
							hour_to_update.push_back(pfp);

							msg += callback.StepIt(0);
						}//if output file doesn't exist
					}//h2
				}//continue

				msg += callback.StepIt();
			}//h1

		}//for all day

		callback.PopTask();

		return msg;
	}

	ERMsg CHRDPS::CreateHourlyPrcp(set<string> date_to_update, CCallback& callback)const
	{
		ERMsg msg;

		vector <CPrcpHourToUpdate> hour_to_update;
		msg = GetPrcpHourToUpdate(date_to_update, hour_to_update, callback);

		callback.PushTask("Create hourly precipitation (" + to_string(hour_to_update.size()) + " hours)", hour_to_update.size());
		callback.AddMessage("Create hourly precipitation (" + to_string(hour_to_update.size()) + " hours)");


		for (vector < CPrcpHourToUpdate>::const_iterator it = hour_to_update.begin(); it != hour_to_update.end() && msg; it++)
		{
			string argument;

			if (!it->m_HRDPS_file_path1.empty() && !it->m_HRDPS_file_path2.empty())
			{
				if (m_bHRDPA6h)
					argument = "-e \"prcp=max(0,round( if(i3b1>0, i4b1*(i2b1-i1b1)/i3b1, i4b1/" + to_string(it->m_nb_hours) + ")*100)/100)\" -ot Float32 -dstNoData 9999 -stats -overwrite -co COMPRESS=LZW -co TILED=YES -co BLOCKXSIZE=256 -co BLOCKYSIZE=256 \"" + it->m_HRDPS_file_path1 + "\" \"" + it->m_HRDPS_file_path2 + "\" \"" + it->m_HRDPS_file_path_last + "\" \"" + it->m_HRDPA_file_path + "\" \"" + it->m_out_file_path + "\"";
				else
					argument = "-e \"prcp=max(0,round( (i2b1-i1b1)*100)/100)\" -ot Float32 -dstNoData 9999 -stats -overwrite -co COMPRESS=LZW -co TILED=YES -co BLOCKXSIZE=256 -co BLOCKYSIZE=256 \"" + it->m_HRDPS_file_path1 + "\" \"" + it->m_HRDPS_file_path2 + "\" \"" + it->m_out_file_path + "\"";
			}
			else
			{
				ASSERT(!it->m_HRDPS_file_path2.empty());

				if (m_bHRDPA6h)
					argument = "-e \"prcp=max(0,round(if(i2b1>0,  i3b1*i1b1/i2b1, i3b1/" + to_string(it->m_nb_hours) + ")*100)/100)\" -ot Float32 -dstNoData 9999 -stats -overwrite -co COMPRESS=LZW -co TILED=YES -co BLOCKXSIZE=256 -co BLOCKYSIZE=256 \"" + it->m_HRDPS_file_path2 + "\" \"" + it->m_HRDPS_file_path_last + "\" \"" + it->m_HRDPA_file_path + "\" \"" + it->m_out_file_path + "\"";
				else
					argument = "-e \"prcp=max(0,round( i1b1*100)/100)\" -ot Float32 -dstNoData 9999 -stats -overwrite -co COMPRESS=LZW -co TILED=YES -co BLOCKXSIZE=256 -co BLOCKYSIZE=256 \"" + it->m_HRDPS_file_path2 + "\" \"" + it->m_out_file_path + "\"";
			}

			ASSERT(!argument.empty());

			string command = "\"" + GetApplicationPath() + "External\\ImageCalculator.exe\" " + argument;
			msg += WinExecWait(command);
			msg += callback.StepIt();
		}

		callback.PopTask();

		return msg;
	}


	ERMsg CHRDPS::CreateHourlySRad(set<string> date_to_update, CCallback& callback)const
	{
		ERMsg msg;

		callback.PushTask("Get solar radiation to update (" + to_string(date_to_update.size()) + " days)", date_to_update.size() * 4);


		size_t total_hours = 0;
		for (set<string>::const_iterator it = date_to_update.begin(); it != date_to_update.end() && msg; it++)
		{
			string year = it->substr(0, 4);
			string month = it->substr(4, 2);
			string day = it->substr(6, 2);
			for (size_t h1 = 0; h1 < 24 && msg; h1 += 6)
			{
				for (size_t h2 = 0; h2 < 48; h2++)
				{
					string HRDPS_file_path1 = FormatA("%s%s\\%s\\%s\\%02d\\CMC_hrdps_continental_DSWRF_SFC_0_ps2.5km_%s%s%s%02d_P%03d-00.grib2", m_workingDir.c_str(), year.c_str(), month.c_str(), day.c_str(), h1, year.c_str(), month.c_str(), day.c_str(), h1, h2);
					string HRDPS_file_path2 = FormatA("%s%s\\%s\\%s\\%02d\\CMC_hrdps_continental_DSWRF_SFC_0_ps2.5km_%s%s%s%02d_P%03d-00.grib2", m_workingDir.c_str(), year.c_str(), month.c_str(), day.c_str(), h1, year.c_str(), month.c_str(), day.c_str(), h1, h2 + 1);

					string out_file_path = HRDPS_file_path2;
					SetFileExtension(out_file_path, ".tif");

					bool b1 = FileExists(HRDPS_file_path1) && FileExists(HRDPS_file_path2);
					bool b2 = FileExists(HRDPS_file_path2) && h2 == 0;
					bool b3 = FileExists(out_file_path);

					if ((b1 || b2) && !b3)
						total_hours++;
				}

				msg += callback.StepIt();
			}
		}

		callback.PopTask();


		callback.PushTask("Create hourly solar radiation (" + to_string(total_hours) + " hours)", total_hours);
		callback.AddMessage("Create hourly solar radiation (" + to_string(total_hours) + " hours)");

		for (set<string>::const_iterator it = date_to_update.begin(); it != date_to_update.end() && msg; it++)
		{

			string year = it->substr(0, 4);
			string month = it->substr(4, 2);
			string day = it->substr(6, 2);

			for (size_t h1 = 0; h1 < 24 && msg; h1 += 6)
			{
				for (size_t h2 = 0; h2 < 48 && msg; h2++)
				{
					string HRDPS_file_path1 = FormatA("%s%s\\%s\\%s\\%02d\\CMC_hrdps_continental_DSWRF_SFC_0_ps2.5km_%s%s%s%02d_P%03d-00.grib2", m_workingDir.c_str(), year.c_str(), month.c_str(), day.c_str(), h1, year.c_str(), month.c_str(), day.c_str(), h1, h2);
					string HRDPS_file_path2 = FormatA("%s%s\\%s\\%s\\%02d\\CMC_hrdps_continental_DSWRF_SFC_0_ps2.5km_%s%s%s%02d_P%03d-00.grib2", m_workingDir.c_str(), year.c_str(), month.c_str(), day.c_str(), h1, year.c_str(), month.c_str(), day.c_str(), h1, h2 + 1);

					string out_file_path = HRDPS_file_path2;
					SetFileExtension(out_file_path, ".tif");

					//if (!FileExists(out_file_path))
					CGDALDatasetEx DS;
					if (DS.OpenInputImage(out_file_path))
					{
						DS.Close();
						//lastUpdate = max(lastUpdate, GetFileInfo(*it4).m_time);
					}
					else
					{
						if (FileExists(out_file_path))
						{
							callback.AddMessage("Remove invalid HRDPS GeoTiff " + out_file_path);
							msg += RemoveFile(out_file_path);
						}
						string argument;

						//j/m� (sum of one hour) -> watt/m�
						if (FileExists(HRDPS_file_path1) && FileExists(HRDPS_file_path2))
						{
							argument = "-e \"srad=max(0, round( (i2b1-i1b1)*10/3600)/10)\" -ot Float32 -dstNoData 9999 -stats -overwrite -co COMPRESS=LZW -co TILED=YES -co BLOCKXSIZE=256 -co BLOCKYSIZE=256 \"" + HRDPS_file_path1 + "\" \"" + HRDPS_file_path2 + "\" \"" + out_file_path + "\"";
						}
						else if (FileExists(HRDPS_file_path2) && h2 == 0)
						{
							argument = "-e \"srad=max(0,round( i1b1*10/3600)/10)\" -ot Float32 -dstNoData 9999 -stats -overwrite -co COMPRESS=LZW -co TILED=YES -co BLOCKXSIZE=256 -co BLOCKYSIZE=256 \"" + HRDPS_file_path2 + "\" \"" + out_file_path + "\"";
						}

						if (!argument.empty())
						{
							string command = "\"" + GetApplicationPath() + "External\\ImageCalculator.exe\" " + argument;
							msg += WinExecWait(command);
							msg += callback.StepIt();
						}
					}
				}
			}
		}

		callback.PopTask();

		return msg;
	}

	//****************************************************************************************************
	ERMsg CHRDPS::CreateDailyGeotiff(set<string> date_to_update, CCallback& callback)const
	{
		ERMsg msg;


		callback.PushTask("Create daily GeoTiff (" + to_string(date_to_update.size()) + " days)", date_to_update.size());
		callback.AddMessage("Create daily GeoTiff  (" + to_string(date_to_update.size()) + " days)");

		for (auto it = date_to_update.begin(); it != date_to_update.end() && msg; it++)
		{
			string year = it->substr(0, 4);
			string month = it->substr(4, 2);
			string day = it->substr(6, 2);

			string filter = m_workingDir + year + "\\" + month + "\\" + day + "\\HRDPS_*.tif";
			StringVector filesList = WBSF::GetFilesList(filter, 2);

			if (filesList.size() == 24)
			{

				string file_path_out = FormatA("%s%s\\%s\\%s\\HRDPSD_%s%s%s.tif", m_workingDir.c_str(), year.c_str(), month.c_str(), day.c_str(), year.c_str(), month.c_str(), day.c_str());

				GribVariables var_in;
				vector<CSfcDatasetCached> DSin(filesList.size());
				for (size_t i = 0; i < filesList.size(); i++)
				{
					msg += DSin[i].open(filesList[i], true);
					if (msg)
						var_in = var_in | DSin[i].get_variables();
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

					callback.PushTask("Create daily GeoTiff for " + year + "-" + month + "-" + day + ": " + ToString(filesList.size()) + " hours", var_in.count()*filesList.size());

					float no_data_out = 9999;

					CBaseOptions options;
					DSin[0].UpdateOption(options);
					options.m_nbBands = var_out.count();
					options.m_outputType = GDT_Float32;
					options.m_dstNodata = no_data_out;
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
						size_t b_out = 0;
						for (size_t v = 0; v < var_out.size() && msg; v++)
						{
							if (var_in.test(v) && var_out.test(v))
							{
								vector<CStatistic> stat(options.m_extents.GetNbPixels());

								for (size_t i = 0; i < DSin.size() && msg; i++)
								{
									size_t b = DSin[i].get_band(v);
									if (b != NOT_INIT)
									{
										float no_data_in = DSin[i].GetNoData(b);
										GDALRasterBand* pBandin = DSin[i].GetRasterBand(b);

										vector<float> data(DSin[i].GetRasterXSize()*DSin[i].GetRasterYSize());
										pBandin->RasterIO(GF_Read, 0, 0, DSin[i].GetRasterXSize(), DSin[i].GetRasterYSize(), &(data[0]), DSin[i].GetRasterXSize(), DSin[i].GetRasterYSize(), GDT_Float32, 0, 0);
										pBandin->FlushCache();

										ASSERT(data.size() == stat.size());
										for (size_t xy = 0; xy < data.size(); xy++)
										{
											//remove negative precipitation
											if (v == H_PRCP && data[xy] < 0.01)
												data[xy] = 0;

											if (data[xy] > -1E10 && fabs(data[xy] - no_data_in) > 0.1)
												stat[xy] += data[xy];
										}

									}

									msg += callback.StepIt();
								}

								if (v == H_TAIR)
								{
									vector<float> data(DSout.GetRasterXSize()*DSout.GetRasterYSize(), no_data_out);
									ASSERT(data.size() == stat.size());

									//add min and max
									for (size_t xy = 0; xy < data.size(); xy++)
									{
										if (stat[xy].IsInit())
											data[xy] = stat[xy][LOWEST];
									}



									GDALRasterBand* pBandout = DSout.GetRasterBand(b_out);
									pBandout->RasterIO(GF_Write, 0, 0, DSout.GetRasterXSize(), DSout.GetRasterYSize(), &(data[0]), DSout.GetRasterXSize(), DSout.GetRasterYSize(), GDT_Float32, 0, 0);
									pBandout->SetDescription(CSfcGribDatabase::META_DATA[H_TMIN][M_DESC]);
									pBandout->SetMetadataItem("GRIB_COMMENT", CSfcGribDatabase::META_DATA[H_TMIN][M_COMMENT]);
									pBandout->SetMetadataItem("GRIB_ELEMENT", CSfcGribDatabase::META_DATA[H_TMIN][M_ELEMENT]);
									pBandout->SetMetadataItem("GRIB_SHORT_NAME", CSfcGribDatabase::META_DATA[H_TMIN][M_SHORT_NAME]);
									pBandout->SetMetadataItem("GRIB_UNIT", CSfcGribDatabase::META_DATA[H_TMIN][M_UNIT]);

									b_out++;
								}

								{
									size_t stat_type = (v == H_PRCP/* || v == H_SRAD*/) ? SUM : MEAN;
									vector<float> data(DSout.GetRasterXSize()*DSout.GetRasterYSize(), no_data_out);
									ASSERT(data.size() == stat.size());

									for (size_t xy = 0; xy < data.size(); xy++)
									{
										if (stat[xy].IsInit())
											data[xy] = stat[xy][stat_type];
									}

									GDALRasterBand* pBandout = DSout.GetRasterBand(b_out);
									pBandout->RasterIO(GF_Write, 0, 0, DSout.GetRasterXSize(), DSout.GetRasterYSize(), &(data[0]), DSout.GetRasterXSize(), DSout.GetRasterYSize(), GDT_Float32, 0, 0);
									pBandout->SetDescription(CSfcGribDatabase::META_DATA[v][M_DESC]);
									pBandout->SetMetadataItem("GRIB_COMMENT", CSfcGribDatabase::META_DATA[v][M_COMMENT]);
									pBandout->SetMetadataItem("GRIB_ELEMENT", CSfcGribDatabase::META_DATA[v][M_ELEMENT]);
									pBandout->SetMetadataItem("GRIB_SHORT_NAME", CSfcGribDatabase::META_DATA[v][M_SHORT_NAME]);
									pBandout->SetMetadataItem("GRIB_UNIT", CSfcGribDatabase::META_DATA[v][M_UNIT]);
									b_out++;
								}

								if (v == H_TAIR)
								{
									vector<float> data(DSout.GetRasterXSize()*DSout.GetRasterYSize(), no_data_out);
									ASSERT(data.size() == stat.size());

									for (size_t xy = 0; xy < data.size(); xy++)
									{
										if (stat[xy].IsInit())
											data[xy] = stat[xy][HIGHEST];
									}

									GDALRasterBand* pBandout = DSout.GetRasterBand(b_out);
									pBandout->RasterIO(GF_Write, 0, 0, DSout.GetRasterXSize(), DSout.GetRasterYSize(), &(data[0]), DSout.GetRasterXSize(), DSout.GetRasterYSize(), GDT_Float32, 0, 0);
									pBandout->SetDescription(CSfcGribDatabase::META_DATA[H_TMAX][M_DESC]);
									pBandout->SetMetadataItem("GRIB_COMMENT", CSfcGribDatabase::META_DATA[H_TMAX][M_COMMENT]);
									pBandout->SetMetadataItem("GRIB_ELEMENT", CSfcGribDatabase::META_DATA[H_TMAX][M_ELEMENT]);
									pBandout->SetMetadataItem("GRIB_SHORT_NAME", CSfcGribDatabase::META_DATA[H_TMAX][M_SHORT_NAME]);
									pBandout->SetMetadataItem("GRIB_UNIT", CSfcGribDatabase::META_DATA[H_TMAX][M_UNIT]);

									b_out++;
								}


							}//if var used
						}//for all variable

						DSout.Close(options);
					}//out open

					for (size_t i = 0; i < filesList.size(); i++)
						DSin[i].close();

					if (msg)
					{
						//convert with gdal_translate to optimize size
						string prj4 = "+proj=stere +lat_0=90 +lat_ts=60 +lon_0=252 +x_0=0 +y_0=0 +R=6371229 +units=m +no_defs";
						string argument = "-ot Float32 -a_nodata 9999 -stats -co COMPRESS=LZW -co PREDICTOR=3 -co TILED=YES -co BLOCKXSIZE=256 -co BLOCKYSIZE=256 -a_srs \"" + prj4 + "\" \"" + file_path_out + "2" + "\" \"" + file_path_out + "\"";
						string command = "\"" + GetApplicationPath() + "External\\gdal_translate.exe\" " + argument;
						msg += WinExecWait(command);
						msg += RemoveFile(file_path_out + "2");
					}

				}//if msg


			}//if 24 hours

			callback.PopTask();
			msg += callback.StepIt();
		}// for all dates

		callback.PopTask();

		return msg;
	}


	std::set<std::string> CHRDPS::Getlast_n_days(size_t nb_days, CCallback& callback)
	{
		ERMsg msg;

		std::set<std::string> date_to_update;

		//vector<pair<string, string>> year_month;
		//StringVector years = WBSF::GetDirectoriesList(m_workingDir + "*");

		//for (StringVector::const_iterator it1 = years.begin(); it1 != years.end() && msg; it1++)
		//{
		//	string year = *it1;
		//	if (ToInt(year) > 1900 && ToInt(year) < 2100)
		//	{
		//		StringVector months = WBSF::GetDirectoriesList(m_workingDir + *it1 + "\\*");
		//		for (StringVector::const_iterator it2 = months.begin(); it2 != months.end() && msg; it2++)
		//		{
		//			//year_month.push_back(m_workingDir + *it1 + "\\" + *it2 + "\\*");
		//			year_month.push_back(make_pair(*it1, *it2));
		//		}
		//	}
		//}

		callback.PushTask("Get historical HRDPS to update (" + to_string(m_update_last_n_days) + " days)", m_update_last_n_days);
		callback.AddMessage("Get historical HRDPS files to update (" + to_string(m_update_last_n_days) + " days)");


		//for (auto it = year_month.begin(); it != year_month.end() && msg; it++)
		CTRef TRef = CTRef::GetCurrentTRef(CTM::DAILY);
		for (size_t d = 0; d < nb_days; d++)
		{
			//date_to_update.insert(TRef.GetFormatedString("%Y%m%d"));

			//}

			//{
				//string year = it->first;
				//string month = it->second;
				//StringVector days = WBSF::GetDirectoriesList(m_workingDir + year + "\\" + month + "\\*");
	//			for (StringVector::const_iterator it3 = days.begin(); it3 != days.end() && msg; it3++)
				//{
			//string day = *it3;
			string filter0 = FormatA("%s%04d\\%02d\\%02d\\*", m_workingDir.c_str(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1 );
			StringVector hours = WBSF::GetDirectoriesList(filter0);
			for (StringVector::const_iterator it4 = hours.begin(); it4 != hours.end() && msg; it4++)
			{
				string hour = *it4;
				size_t h1 = WBSF::as<size_t>(hour);

				for (size_t h2 = 0; h2 < 6; h2++)
				{
					string filter1 = FormatA("%s%04d\\%02d\\%02d\\%02d\\*%04d%02d%02d%02d_P%03d-00.grib2", m_workingDir.c_str(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, h1, TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, h1, h2);
					string filter2 = FormatA("%s%04d\\%02d\\%02d\\%02d\\*%04d%02d%02d%02d_P%03d-00.tif", m_workingDir.c_str(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, h1, TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, h1, h2);

					if (!WBSF::GetFilesList(filter1, 2, true).empty() || !WBSF::GetFilesList(filter2, 2, true).empty())
					{
						string tifFilePath = FormatA("%s%04d\\%02d\\%02d\\HRDPS_%04d%02d%02d%02d-%03d.tif", m_workingDir.c_str(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, h1, h2);

						/*if (!WBSF::FileExists(tifFilePath))
						{
							string date = FormatA("%04d\\%02d\\%02d", TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1);
							date_to_update.insert(date);
						}*/
						CGDALDatasetEx DS;
						if (DS.OpenInputImage(tifFilePath))
						{
							DS.Close();
							//lastUpdate = max(lastUpdate, GetFileInfo(*it4).m_time);
						}
						else
						{
							if (FileExists(tifFilePath))
							{
								callback.AddMessage("Remove invalid HRDPS GeoTiff " + tifFilePath);
								msg += RemoveFile(tifFilePath);
							}
								
							//TRef.GetFormatedString();
							//string date = FormatA("%04d\\%02d\\%02d", TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1);
							date_to_update.insert(TRef.GetFormatedString("%Y%m%d"));
							//date_to_update.insert(date);
						}
					}

					msg += callback.StepIt(0);
				}
			}//for all hours

			TRef--;

			msg += callback.StepIt();
		}//for all days


	//}//for all months

		callback.PopTask();

		if (!msg)
			date_to_update.clear();

		return date_to_update;
	}


	//****************************************************************************************************


	ERMsg CHRDPS::GetStationList(CLocationVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		stationList.clear();

		return msg;
	}


	ERMsg CHRDPS::GetGribsList(CTPeriod p, CGribsMap& gribsList, CCallback& callback)
	{
		ERMsg msg;

		CTPeriod p_daily = p.as(CTM::DAILY);
		for (CTRef TRef = p_daily.Begin(); TRef <= p_daily.End(); TRef++)
		{
			int year = TRef.GetYear();
			size_t m = TRef.GetMonth();
			size_t d = TRef.GetDay();

			string filter = FormatA("%s%04d\\%02d\\%02d\\HRDPS_%04d%02d%02*.tif", m_workingDir.c_str(), year, m + 1, d + 1, year, m + 1, d + 1);
			StringVector fileList = WBSF::GetFilesList(filter);
			for (size_t i = 0; i < fileList.size(); i++)
			{
				string title = GetFileTitle(fileList[i]);
				int year = WBSF::as<int>(title.substr(6, 4));
				size_t m = WBSF::as<int>(title.substr(10, 2)) - 1;
				size_t d = WBSF::as<int>(title.substr(12, 2)) - 1;
				size_t h = WBSF::as<int>(title.substr(14, 2));
				size_t hh = WBSF::as<int>(title.substr(17, 3));
				size_t max_hh = m_bForecast ? 48 : 6;
				if (hh < max_hh)
				{
					CTRef TRef2 = CTRef(year, m, d, h) + hh;
					if (p.IsInside(TRef2))
					{
						gribsList[TRef2] = fileList[i];
					}
				}
			}

		}

		return msg;
	}

	CTRef CHRDPS::GetTRef(string file_path, bool bAddForecast)
	{
		string title = GetFileTitle(file_path);
		WBSF::ReplaceString(title, "GUST_MAX", "GUST-MAX");
		WBSF::ReplaceString(title, "GUST_MIN", "GUST-MIN");

		CTRef TRef;
		StringVector tmp(title, "_");
		ASSERT(tmp.size() == 9 || tmp.size() == 8);
		if (tmp.size() == 9 || tmp.size() == 8)
		{
			//size_t i = 7;
			ASSERT(tmp[7].length() == 10);
			int year = WBSF::as<int>(tmp[7].substr(0, 4));
			size_t m = WBSF::as<size_t >(tmp[7].substr(4, 2)) - 1;
			size_t d = WBSF::as<size_t >(tmp[7].substr(6, 2)) - 1;
			size_t h = WBSF::as<size_t >(tmp[7].substr(8, 2));


			TRef = CTRef(year, m, d, h);

			if (bAddForecast)
			{
				ASSERT(tmp[8].length() == 7);
				size_t hh = WBSF::as<size_t >(tmp[8].substr(1, 3));

				TRef += int(hh);
			}
		}

		return TRef;

	}

	size_t CHRDPS::GetHH(const string& title)const
	{
		CTRef CTRef = GetTRef(title, false);
		return CTRef.GetHour();
	}

	size_t CHRDPS::Gethhh(string title)const
	{

		WBSF::ReplaceString(title, "GUST_MAX", "GUST-MAX");
		WBSF::ReplaceString(title, "GUST_MIN", "GUST-MIN");

		size_t hhh = NOT_INIT;
		StringVector tmp(title, "_");
		if (tmp.size() == 9)
		{
			size_t i = tmp.size() - 1;
			ASSERT(tmp[i].length() > 4);
			string str_hhh = tmp[i].substr(1, 3);
			hhh = WBSF::as<size_t>(str_hhh);
		}


		CTRef CTRef1 = GetTRef(title, false);
		CTRef CTRef2 = GetTRef(title, true);

		ASSERT((CTRef2 - CTRef1) == hhh);

		return CTRef2 - CTRef1;

		//	return hhh;
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
		//		size_t vv = GetHRDPSVariable(*it);
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
		//						if (station[accumulator.GetTRef()].GetHRDPSVariables().none() )//don't override observation
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

		//							if (v == H_SRAD)
		//								value *= 3600;//J/m� --> W/m�

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

		//				if (accumulator.GetTRef().IsInit())
		//				{
		//					if (station[accumulator.GetTRef()].GetHRDPSVariables().none())//don't override observation
		//						station[accumulator.GetTRef()].SetData(accumulator);
		//				}
		//			}//if is inside
		//		}//if is in zone
		//	}//if msg
		//}//if there is data for the current year
		////}


		return msg;
	}

	ERMsg CHRDPS::Clean(int delete_after, string workingDir, CCallback& callback)
	{
		ERMsg msg;


		CTRef nowUTC = CTRef::GetCurrentTRef(CTM::HOURLY, true);
		StringVector filesList;
		StringVector dirList;

		StringVector years = WBSF::GetDirectoriesList(workingDir + "*");//for security, need years
		for (size_t y = 0; y < years.size(); y++)
		{
			int year = WBSF::as<int>(years[y]);
			if (year >= 2000 && year <= 2100)
			{
				bool bRemoveDirY = true;

				StringVector months = WBSF::GetDirectoriesList(workingDir + to_string(year) + "/*");//for security
				for (size_t m = 0; m < months.size(); m++)
				{
					size_t month = WBSF::as<size_t>(months[m]);
					if (month < 12)
					{
						bool bRemoveDirM = true;
						StringVector days = WBSF::GetDirectoriesList(workingDir + to_string(year) + "/" + months[m] + "/*");//for security
						for (size_t d = 0; d < days.size(); d++)
						{

							size_t day = WBSF::as<size_t>(months[m]);
							if (day < 31)
							{
								bool bRemoveDirD = true;
								StringVector hours = WBSF::GetDirectoriesList(workingDir + to_string(year) + "/" + months[m] + "/" + days[d] + "/*");//for security
								for (size_t h = 0; h < hours.size(); h++)
								{
									size_t hour = WBSF::as<size_t>(hours[h]);
									if (hour < 24)
									{
										StringVector filesListTmp = GetFilesList(workingDir + to_string(year) + "/" + months[m] + "/" + days[d] + "/" + hours[h] + "/*.*");//don't put extension because of the 2.5km trouble

										bool bRemoveDirH = true;
										for (size_t f = 0; f < filesListTmp.size(); f++)
										{
											CTRef TRefUTC = GetTRef(filesListTmp[f], true);
											//size_t hhh = as<size_t>(GetLastDirName(it2->m_filePath));
											//CTRef TRefUTC = GetTRef(GetFileTitle(it2->m_filePath)) + hhh;
											int passHours = nowUTC - TRefUTC;



											if (passHours > delete_after)
											{
												filesList.push_back(filesListTmp[f]);
												filesList.push_back(GetGeotiffFilePath(workingDir, filesListTmp[f]));
											}
											else
											{
												bRemoveDirH = false;
												bRemoveDirD = false;
												bRemoveDirM = false;
												bRemoveDirY = false;
											}
										}

										if (bRemoveDirH)
											dirList.push_back(workingDir + to_string(year) + "/" + months[m] + "/" + days[d] + "/" + hours[h]);
									}//<24
								}//for all hours

								if (bRemoveDirD)
									dirList.push_back(workingDir + to_string(year) + "/" + months[m] + "/" + days[d]);
							}//<31
						}//for all days

						if (bRemoveDirM)
							dirList.push_back(workingDir + to_string(year) + "/" + months[m]);
					}//<12
				}//for all months

				if (bRemoveDirY)
					dirList.push_back(workingDir + to_string(year));
			}//2000-2100
		}//for all years

		callback.PushTask("Delete old HRDPS forecast (" + to_string(filesList.size()) + " files)", filesList.size());
		callback.AddMessage("Delete old HRDPS forecast (" + to_string(filesList.size()) + " files)");
		for (size_t i = 0; i != filesList.size() && msg; i++)
		{
			msg += RemoveFile(filesList[i]);
			msg += callback.StepIt();
		}

		//remove directory

		for (size_t i = 0; i != dirList.size() && msg; i++)
		{
			WBSF::RemoveDirectory(dirList[i]);
		}

		callback.PopTask();

		return msg;
	}


	//******************************************************************************************************************

	const char* CHRDPSVariables::CATEGORY[NB_HRDPS_CATEGORY] = { "SFC", "TGL", "ISBL", "ISBY", "ETAL", "EATM", "MSL", "DBLY", "DBLL", "NTAT" };
	StringVector CHRDPSVariables::DESCRIPTION[NB_HRDPS_CATEGORY];


	const char* CHRDPSVariables::NAME[NB_HRDPS_VARIABLES] =
	{
		"4LFTX_SFC", "ALBDO_SFC", "APCP_SFC", "DLWRF_SFC", "DSWRF_SFC", "HGT_SFC", "ICEC_SFC", "LAND_SFC", "LHTFL_SFC", "NLWRS_SFC", "NSWRS_SFC", "PRATE_SFC",
		"PRES_SFC", "SHOWA_SFC", "SHTFL_SFC", "SNOD_SFC", "SPFH_SFC", "TCDC_SFC", "TSOIL_SFC", "WEAFR_SFC", "WEAPE_SFC", "WEARN_SFC", "WEASN_SFC", "WTMP_SFC",
		"GUST_SFC","ICETK_SFC","RH_SFC","SOILVIC_SFC", "GUST_MAX_SFC", "GUST_MIN_SFC", "SDEN_SFC", "SFCWRO_SFC", "SDWE_SFC", "HPBL_SFC", "PTYPE_SFC", "SKINT_SFC",
		"DEN_TGL", "DEPR_TGL", "DPT_TGL", "RH_TGL", "SPFH_TGL", "TMP_TGL", "UGRD_TGL", "VGRD_TGL", "WDIR_TGL", "WIND_TGL","GUST_MAX_TGL","GUST_MIN_TGL","GUST_TGL",
		"ABSV_ISBL", "DEPR_ISBL", "HGT_ISBL", "RH_ISBL", "SPFH_ISBL", "TMP_ISBL", "UGRD_ISBL", "VGRD_ISBL", "VVEL_ISBL", "WDIR_ISBL", "WIND_ISBL", "MU-VT-LI_ISBL", "SHWINX_ISBL",
		"HGT_ISBY",
		"CAPE_ETAL", "HLCY_ETAL",
		"CWAT_EATM",
		"PRMSL_MSL",
		"SOILW_DBLY",
		"TSOIL_DBLL", "SOILW_DBLL",
		"DSWRF_NTAT", "ULWRF_NTAT", "USWRF_NTAT"

	};

	//Accumulated Freezing Rain|Accumulated Ice Pellets|Accumulated Rain|Accumulated Snow

	const size_t CHRDPSVariables::CAT_RANGE[NB_HRDPS_CATEGORY][2] =
	{
		{LFTX_SFC, LAST_SFC},
		{DEN_TGL, LAST_TGL},
		{ABSV_ISBL, LAST_ISBL},
		{HGT_ISBY,LAST_ISBY},
		{CAPE_ETAL, LAST_ETAL},
		{CWAT_EATM,LAST_EATM},
		{PRMSL_MSL,LAST_MSL},
		{SOILW_DBLY,LAST_DBLY},
		{TSOIL_DBLL,LAST_DBLL},
		{DSWRF_NTAT,LAST_NTAT},
	};

	bool CHRDPSVariables::Is(size_t var, THRDPSCategory c)
	{
		return var >= CAT_RANGE[c][0] && var < CAT_RANGE[c][1];
	}


	string CHRDPSVariables::GetHRDPSSelectionString(size_t c)
	{
		ASSERT(c < NB_HRDPS_CATEGORY);

		string select;

		for (size_t i = CAT_RANGE[c][0]; i < CAT_RANGE[c][1]; i++)
		{
			select += string(CHRDPSVariables::GetName(i)) + "=" + string(CHRDPSVariables::GetName(i)) + ":" + CHRDPSVariables::GetDescription(i) + "|";
		}

		return select;
	}


	void CHRDPSVariables::LoadDescription()
	{
		if (DESCRIPTION[0].empty())
		{
			for (size_t c = 0; c < NB_HRDPS_CATEGORY; c++)
			{
				DESCRIPTION[c].LoadString(IDS_HRDPS_VAR_SFC + UINT(c), "|");
				ASSERT(DESCRIPTION[c].size() == GetNbVar(c));
			}
		}

	}

	size_t CHRDPSVariables::GetCat(size_t var)
	{
		ASSERT(var < NB_HRDPS_VARIABLES);

		size_t c = NOT_INIT;
		for (size_t i = 0; i < NB_HRDPS_CATEGORY&&c == NOT_INIT; i++)
		{
			if (var >= CAT_RANGE[i][0] && var < CAT_RANGE[i][1])
				c = i;
		}

		return c;
	}

	size_t CHRDPSVariables::GetVarPos(size_t var)
	{
		ASSERT(var < NB_HRDPS_VARIABLES);

		size_t c = GetCat(var);
		return var - CAT_RANGE[c][0];
	}

	size_t CHRDPSVariables::GetVariable(const std::string& in)
	{
		string& name = TrimConst(in);
		size_t vv = NOT_INIT;
		for (size_t v = 0; v < NB_HRDPS_VARIABLES&&vv == NOT_INIT; v++)
		{
			if (WBSF::IsEqual(name, NAME[v]))
				vv = v;
		}

		return vv;
	}

	size_t CHRDPSVariables::GetCategory(const std::string& name)
	{
		size_t vv = NOT_INIT;
		for (size_t v = 0; v < NB_HRDPS_CATEGORY&&vv == NOT_INIT; v++)
		{
			if (WBSF::IsEqual(name, CATEGORY[v]))
				vv = v;
		}

		return vv;
	}

	size_t CHRDPSVariables::GetLevel(const std::string& name)
	{
		//some time is a range??
		return as<int>(name);
	}

	CHRDPSVariables& CHRDPSVariables::operator = (std::string in)
	{
		Trim(in);
		if (in == "----")
		{
			reset();
		}
		else if (in.empty())
		{
			set();
		}
		else
		{
			reset();

			StringVector str(in, ",;|");

			for (size_t i = 0; i < str.size(); i++)
			{
				size_t v = GetVariable(str[i]);
				if (v != NOT_INIT)
					set(v);
			}
		}

		return *this;
	}

	//*********************************************************************

	CHRDPSLevels::CHRDPSLevels(std::string str)
	{
		FromString(str);
	}

	void CHRDPSLevels::FromString(std::string str)
	{
		clear();
		tokenizer<escaped_list_separator<char> > tk(str, escaped_list_separator<char>("\\", ",|;", "\""));
		for (tokenizer<escaped_list_separator<char> >::iterator i(tk.begin()); i != tk.end(); ++i)
		{
			insert(ToSizeT(*i));
		}
	}

	std::string CHRDPSLevels::ToString()const
	{
		string str;
		for (auto it(begin()); it != end(); ++it)
		{
			if (!str.empty())
				str += "|";

			str += to_string(*it);
		}

		return str;
	}

	//*********************************************************************



}