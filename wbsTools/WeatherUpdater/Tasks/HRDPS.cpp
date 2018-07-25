#include "StdAfx.h"
#include "HRDPS.h"
#include "Basic/WeatherStation.h"
#include "UI/Common/SYShowMessage.h"
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

	string CHRDPSVariables::GetHRDPSSelectionString()
	{
		string select;
		for (size_t i = 0; i < NB_HRDPS_VARIABLES; i++)
			select += string(CHRDPSVariables::GetName(i)) + "=" + string(CHRDPSVariables::GetName(i)) + ":" + CHRDPSVariables::GetDescription(i) + "|";

		return select;
	}


	//*********************************************************************
	const char* CHRDPS::SERVER_NAME = "dd.weather.gc.ca";
	const char* CHRDPS::SERVER_PATH = "/model_hrdps/continental/grib2/";
	static const size_t MAX_FORECAST_HOURS = 48;


	CHRDPS::CHRDPS(const std::string& workingDir) :
		m_workingDir(workingDir),
		m_bCreateVRT(true),
		m_bForecast(false),
		m_max_hours(99)
	{
		m_variables.set();
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
		CTRef TRef = CHRDPS::GetTRef(fileName);
		return FormatA("%s%d\\%02d\\%02d\\%02d\\%s", m_workingDir.c_str(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1, TRef.GetHour(), fileName.c_str());
	}

	size_t CHRDPS::GetVariable(const string& fileName)
	{
		//CMC_hrdps_continental_ABSV_ISBL_0250_ps2.5km_2016122918_P000-00
		StringVector parts(fileName, "_");
		ASSERT(parts.size() == 9);
		return CHRDPSVariables::GetVariable(parts[3] + "_" + parts[4]);
	}

	size_t CHRDPS::GetLevel(const string& fileName)
	{
		//CMC_hrdps_continental_ABSV_ISBL_0250_ps2.5km_2016122918_P000-00
		StringVector parts(fileName, "_");
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
					string fileTitle = GetFileName(fileListTmp.front().m_filePath);
					latest.push_back(make_pair(GetTRef(fileTitle), h));
				}
			}

			sort(latest.begin(), latest.end());

			if (!latest.empty())
				HH = latest.back().second;

		}

		return msg;
	}

	//****************************************************************************************************
	ERMsg CHRDPS::Execute(CCallback& callback)
	{
		ERMsg msg;

		callback.AddMessage(GetString(IDS_UPDATE_DIR));
		callback.AddMessage(m_workingDir, 1);
		callback.AddMessage(GetString(IDS_UPDATE_FROM));
		callback.AddMessage(SERVER_NAME, 1);
		callback.AddMessage("");

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
				dir2.insert(dir2.end(), tmp.begin(), tmp.end());
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
							string path = it2->m_filePath;
							path = GetPath(path.substr(0, path.length() - 1));
							size_t HH = as<size_t>(GetLastDirName(path));
							size_t hhh = as<size_t>(GetLastDirName(it2->m_filePath));
							CTRef TRefUTC = GetTRef(GetFileTitle(it2->m_filePath));
		
							bool bDownload = m_bForecast ? (HH == lastestHH && hhh >= 6 && (TRefUTC-nowUTC) <= m_max_hours) : hhh < 6;
							if (bDownload)
							{
								CFileInfoVector fileListTmp;
								msg = FindFiles(pConnection, it2->m_filePath + "*.grib2", fileListTmp);
								for (CFileInfoVector::iterator it = fileListTmp.begin(); it != fileListTmp.end() && msg; it++)
								{
									string fileName = GetFileName(it->m_filePath);
									string outputFilePath = GetOutputFilePath(fileName);
									size_t var = GetVariable(fileName);

									if (var < m_variables.size())
									{
										if (m_variables.test(var))
										{
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
										}//if wanted variable
									}//Humm new variable
									else
									{
										callback.AddMessage("HRDPS var unknowns : " + fileName);
									}
								}//for all files
							}//take only 6 first hours

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

		set<string> ouputsPath;
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

					try
					{
						while (it != fileList.end() && msg)
						{
							string fileName = GetFileName(it->m_filePath);
							string outputFilePath = GetOutputFilePath(fileName);

							CreateMultipleDir(GetPath(outputFilePath));
							msg = CopyFile(pConnection, it->m_filePath, outputFilePath, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_EXISTING_CONNECT );
							if (msg)
							{
								nbDownload++;
								ouputsPath.insert(GetVRTFilePath(outputFilePath));
							}


							msg += callback.StepIt();
							it++;
							nbTry = 0;
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


		//now, create .vrt and index file
		if (msg && m_bCreateVRT)
			msg = CreateVRT(ouputsPath, callback);


		return msg;
	}
	
	string CHRDPS::GetVRTFilePath(string outputFilePath)
	{
		
		//string path = GetPath(outputFilePath);
		string title = GetFileTitle(outputFilePath);
		size_t pos = title.find("2.5km_");
		ASSERT(pos != string::npos);

		string year = title.substr(pos+6, 4);
		string month = title.substr(pos + 10, 2);
		string day = title.substr(pos + 12, 2);
		size_t h1 = WBSF::as<size_t>(title.substr(pos + 14, 2));
		size_t h2 = WBSF::as<size_t>(title.substr(pos + 18, 3));
		//size_t h1 = (h / 6) * 6;
		//size_t h2 = (h % 6);

		//CTRef now = CTRef::GetCurrentTRef(CTM::HOURLY);

		string VRTFilePath = FormatA("%s%s\\%s\\%s\\HRDPS_%s%s%s%02d-%03d.vrt", m_workingDir.c_str(), year.c_str(), month.c_str(), day.c_str(), year.c_str(), month.c_str(), day.c_str(), h1, h2);

		//create VRT
		//string filter = FormatA("%s%s\\%s\\%s\\%02d\\*%s%s%s%02d_P%03d-00.grib2", m_workingDir.c_str(), year.c_str(), month.c_str(), day.c_str(), h1, year.c_str(), month.c_str(), day.c_str(), h1, h2);
		//string HH = FormatA("%02d", h1);

		return VRTFilePath;
	}

	ERMsg CHRDPS::CreateVRT(set<string> outputPath, CCallback& callback)
	{
		ERMsg msg;

		CTRef now = CTRef::GetCurrentTRef(CTM::HOURLY);

		//StringVector filePaths;
		if (outputPath.empty())
		{
			StringVector years = WBSF::GetDirectoriesList(m_workingDir + "*");
			for (StringVector::const_iterator it1 = years.begin(); it1 != years.end() && msg; it1++)
			{
				string year = *it1;
				StringVector months = WBSF::GetDirectoriesList(m_workingDir + *it1 + "\\*");
				for (StringVector::const_iterator it2 = months.begin(); it2 != months.end() && msg; it2++)
				{
					string month = *it2;
					StringVector days = WBSF::GetDirectoriesList(m_workingDir + *it1 + "\\" + *it2 + "\\*");
					for (StringVector::const_iterator it3 = days.begin(); it3 != days.end() && msg; it3++)
					{
						string day = *it3;

						StringVector hours = WBSF::GetDirectoriesList(m_workingDir + *it1 + "\\" + *it2 + "\\" + *it3 + "\\*");
						for (StringVector::const_iterator it4 = hours.begin(); it4 != hours.end() && msg; it4++)
						{
							string hour = *it4;
							size_t h1 = WBSF::as<size_t>(hour);

							//string filter = FormatA("%s%s\\%s\\%s\\%02d\\*%s%s%s%02d_P%03d-00.grib2", m_workingDir.c_str(), year.c_str(), month.c_str(), day.c_str(), h1, year.c_str(), month.c_str(), day.c_str(), h1, h2);
							//StringVector fileList = WBSF::GetFilesList(filter, 2, true);
				
							//sort(fileList.begin(), fileList.end());
							//for (StringVector::iterator it5 = fileList.begin(); it5 != fileList.end() && msg; it5++)
							//{
							//	//string VRTFilePath = FormatA("%s%s\\%s\\%s\\HRDPS_%s%s%s%02d.vrt", m_workingDir.c_str(), year.c_str(), month.c_str(), day.c_str(), year.c_str(), month.c_str(), day.c_str(), h);
							//	string VRTFilePath = GetVRTFilePath(*it5);
							//	outputPath.insert(VRTFilePath);
							//}

							for (size_t h2 = 0; h2 < 6; h2++)
							{
								//size_t h = h1 + h2;
								string VRTFilePath = FormatA("%s%s\\%s\\%s\\HRDPS_%s%s%s%02d-%03d.vrt", m_workingDir.c_str(), year.c_str(), month.c_str(), day.c_str(), year.c_str(), month.c_str(), day.c_str(), h1, h2);

								if (!WBSF::FileExists(VRTFilePath))
								{
									outputPath.insert(VRTFilePath);
								}

								msg += callback.StepIt(0);
							}
						}//for all hours
					}//for all days
				}//for all months
			}//for all years
		}

		callback.PushTask("Create VRT: " + ToString(outputPath.size()) + " files", outputPath.size());


		for (set<string>::const_iterator it = outputPath.begin(); it != outputPath.end() && msg; it++)
		{
			string title = GetFileTitle(*it);
			string year = title.substr(6, 4);
			string month = title.substr(10, 2);
			string day = title.substr(12, 2);
			size_t h1 = WBSF::as<size_t>(title.substr(14, 2));
			size_t h2 = WBSF::as<size_t>(title.substr(17, 3));
			//size_t h1 = (h / 6) * 6;
			//size_t h2 = (h % 6);
		
			//create VRT
			string filter = FormatA("%s%s\\%s\\%s\\%02d\\*%s%s%s%02d_P%03d-00.grib2", m_workingDir.c_str(), year.c_str(), month.c_str(), day.c_str(), h1, year.c_str(), month.c_str(), day.c_str(), h1, h2);
			string HH = FormatA("%02d", h1);

			StringVector fileList = WBSF::GetFilesList(filter, 2, true);
			sort(fileList.begin(), fileList.end());

			//if (fileList.size() > 300)
			{
				//very long to bild vrt (~12 min???)
				//msg += BuildVRT(VRTFilePath, fileList, true, EXEPath);

				ofStream oFile;
				msg = oFile.open(*it);
				if (msg)
				{
					oFile << "<VRTDataset rasterXSize=\"2576\" rasterYSize=\"1456\">" << endl;
					oFile << "  <SRS>PROJCS[\"unnamed\",GEOGCS[\"Coordinate System imported from GRIB file\",DATUM[\"unknown\",SPHEROID[\"Sphere\",6371229,0]],PRIMEM[\"Greenwich\",0],UNIT[\"degree\",0.0174532925199433]],PROJECTION[\"Polar_Stereographic\"],PARAMETER[\"latitude_of_origin\",60],PARAMETER[\"central_meridian\",252],PARAMETER[\"scale_factor\",90],PARAMETER[\"false_easting\",0],PARAMETER[\"false_northing\",0]]</SRS>" << endl;
					oFile << "  <GeoTransform>-2.0991274944969425e+006, 2.5000000000000000e+003, 0.0000000000000000e+000,-2.0993885214996245e+006, 0.0000000000000000e+000,-2.5000000000000000e+003</GeoTransform>" << endl;

					int b = 1;
					for (StringVector::iterator it4 = fileList.begin(); it4 != fileList.end() && msg; it4++, b++)
					{
						string fileName = GetFileName(*it4);

						oFile << "  <VRTRasterBand dataType=\"Float64\" band=\"" << ToString(b) << "\">" << endl;
						oFile << "    <NoDataValue>9999</NoDataValue>" << endl;
						oFile << "    <ComplexSource>" << endl;
						oFile << "      <SourceFilename relativeToVRT=\"1\">" << HH << "\\" << fileName << "</SourceFilename>" << endl;
						oFile << "      <SourceBand>1</SourceBand>" << endl;
						oFile << "      <SourceProperties RasterXSize=\"2576\" RasterYSize=\"1456\" DataType=\"Float64\" BlockXSize=\"2576\" BlockYSize=\"1\" />" << endl;
						oFile << "      <SrcRect xOff=\"0\" yOff=\"0\" xSize=\"2576\" ySize=\"1456\" />" << endl;
						oFile << "      <DstRect xOff=\"0\" yOff=\"0\" xSize=\"2576\" ySize=\"1456\" />" << endl;
						oFile << "      <NODATA>9999</NODATA>" << endl;
						oFile << "</ComplexSource>" << endl;
						oFile << "  </VRTRasterBand>" << endl;
					}

					oFile << "</VRTDataset>" << endl;
					oFile.close();
				}
			}//if valid layer

			msg += callback.StepIt();

		}//for all files

		callback.PopTask();
		return msg;
	}



	//****************************************************************************************************


	ERMsg CHRDPS::GetStationList(CLocationVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		stationList.clear();
		//msg.ajoute("Can't extract station from grid");
		//msg.ajoute("Can be used only as forecast extraction");

		return msg;
	}


	ERMsg CHRDPS::GetGribsList(CTPeriod p, std::map<CTRef, std::string>& gribsList, CCallback& callback)
	{
		ERMsg msg;

		CTPeriod p_daily = p.as(CTM::DAILY);
		for (CTRef TRef = p.Begin(); TRef != p.End(); TRef++)
		{
			int year = TRef.GetYear();
			size_t m = TRef.GetMonth();
			size_t d = TRef.GetDay();
//			size_t h = TRef.GetHour();
			
			string filter = FormatA("%s%04d\\%02d\\%02d\\HRDPS_%04d%02d%02*.vrt", m_workingDir.c_str(), year, m + 1, d + 1, year, m + 1, d + 1);
			StringVector fileList = WBSF::GetFilesList(filter);
			for (size_t i = 0; i < fileList.size(); i++)
			{
				string title = GetFileTitle(fileList[i]);
				int year = WBSF::as<int>(title.substr(6, 4));
				size_t m = WBSF::as<int>(title.substr(10, 2)) - 1;
				size_t d = WBSF::as<int>(title.substr(12, 2)) - 1;
				size_t h = WBSF::as<int>(title.substr(14, 2));
				size_t hh = WBSF::as<int>(title.substr(17, 3));
				CTRef TRef = CTRef(year,m,d,h) + hh;
//				CTRef TRef = GetTRef(fileList[i]);
				if (p.IsInside(TRef))
					gribsList[TRef] = fileList[i];
			}

		}

		return msg;
	}

	CTRef CHRDPS::GetTRef(const string& fileName)
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

	size_t CHRDPS::Gethhh(const string& title)const
	{
		size_t hhh = NOT_INIT;
		StringVector tmp(title, "_");
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

	//******************************************************************************************************************
	const char* CHRDPSVariables::NAME[NB_HRDPS_VARIABLES] =
	{
		"4LFTX_SFC", "ALBDO_SFC", "APCP_SFC", "DLWRF_SFC", "DSWRF_SFC", "HGT_SFC", "ICEC_SFC", "LAND_SFC", "LHTFL_SFC", "NLWRS_SFC", "NSWRS_SFC", "PRATE_SFC",
		"PRES_SFC", "SHOWA_SFC", "SHTFL_SFC", "SNOD_SFC", "SPFH_SFC", "TCDC_SFC", "TSOIL_SFC", "WEAFR_SFC", "WEAPE_SFC", "WEARN_SFC", "WEASN_SFC", "WTMP_SFC",
		"GUST_SFC","ICETK_SFC","RH_SFC","SOILVIC_SFC",
		"DEN_TGL", "DEPR_TGL", "DPT_TGL", "RH_TGL", "SPFH_TGL", "TMP_TGL", "UGRD_TGL", "VGRD_TGL", "WDIR_TGL", "WIND_TGL",
		"ABSV_ISBL", "DEPR_ISBL", "HGT_ISBL", "RH_ISBL", "SPFH_ISBL", "TMP_ISBL", "UGRD_ISBL", "VGRD_ISBL", "VVEL_ISBL", "WDIR_ISBL", "WIND_ISBL",
		"CAPE_ETAL", "CWAT_EATM", "DSWRF_NTAT", "HGT_ISBY", "HLCY_ETAL", "PRMSL_MSL", "SOILW_DBLY", "TSOIL_DBLL", "ULWRF_NTAT", "USWRF_NTAT",
	};






	const char* CHRDPSVariables::CATEGORY[NB_HRDPS_CATEGORY] = { "SFC", "TGL", "ISBL", "ETAL", "EATM", "NTAT", "ISBY", "MSL", "DBLY", "DBLL" };
	StringVector CHRDPSVariables::DESCRIPTION;


	void CHRDPSVariables::LoadDescription()
	{
		if (DESCRIPTION.empty())
			DESCRIPTION.LoadString(IDS_HRDPS_VAR, "|");
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
}