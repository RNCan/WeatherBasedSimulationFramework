#include "StdAfx.h"
#include "UIGribHistorical.h"
#include "Basic/FileStamp.h"
#include "HRDPS.h"
#include "HRRR.h"
//#include "Basic/CSV.h"
#include "Basic/WeatherStation.h"
#include "Geomatic/SfcGribsDatabase.h"
#include "UI/Common/SYShowMessage.h"


#include "TaskFactory.h"
#include "WeatherBasedSimulationString.h"
#include "../Resource.h"

#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;


namespace WBSF
{
	//NAM analysis
	//ftp://nomads.ncdc.noaa.gov/NAM/Grid218/

	//https://nomads.ncdc.noaa.gov/data/


	//filtered
	//URL =	https://nomads.ncep.noaa.gov/cgi-bin/filter_nam.pl?
	//file=nam.t00z.awphys00.tm00.grib2&lev_1000_mb=on&lev_10_m_above_ground=on&lev_2_m_above_ground=on&lev_800_mb=on&lev_80_m_above_ground=on&lev_825_mb=on&lev_850_mb=on&lev_875_mb=on&lev_900_mb=on&lev_90-60_mb_above_ground=on&lev_925_mb=on&lev_950_mb=on&lev_975_mb=on&lev_surface=on&var_APCP=on&var_DZDT=on&var_HGT=on&var_PRES=on&var_RH=on&var_TMP=on&var_UGRD=on&var_VGRD=on&var_VVEL=on&leftlon=0&rightlon=360&toplat=90&bottomlat=-90&dir=%2Fnam.20200622


	//*********************************************************************
	const char* CUIGribHistorical::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "Product", "Dimension", "Variable", "Begin", "End" };
	const size_t CUIGribHistorical::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_COMBO_INDEX, T_COMBO_INDEX, T_COMBO_INDEX, T_DATE, T_DATE };
	const UINT CUIGribHistorical::ATTRIBUTE_TITLE_ID = IDS_UPDATER_GRIB_HISTORICAL_P;
	const UINT CUIGribHistorical::DESCRIPTION_TITLE_ID = ID_TASK_GRIB_HISTORICAL;

	const char* CUIGribHistorical::CLASS_NAME() { static const char* THE_CLASS_NAME = "GribHistorical";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIGribHistorical::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIGribHistorical::CLASS_NAME(), (createF)CUIGribHistorical::create);




	//const char* CUIGribHistorical::HTTP_SERVER_NAME[NB_SOURCES] = { "nomads.ncdc.noaa.gov", "nomads.ncep.noaa.gov" };
	//const char* CUIGribHistorical::FTP_SERVER_NAME[NB_SOURCES] = { "nomads.ncdc.noaa.gov", "www.ftp.ncep.noaa.gov" };
	const char* CUIGribHistorical::PRODUCT_NAME[NB_PRODUCTS] = { "HRDPS", "HRRR", "RAP", "NAM", "NAM3km", "WRF-ARW", "NMMB", "GFS" };
	string CUIGribHistorical::GetProductName(size_t product, size_t dimension, bool bName)
	{
		string product_name;
		string directory;

		if (product == P_HRRR)
		{
			product_name = "hrrr_2d";
			directory = "hrrr";
		}
		else if (product == P_RAP)
		{
			directory = product_name = "rap";
		}
		else if (product == P_NAM)
		{
			directory = product_name = "nam";
		}
		else if (product == P_NAM_NEST_CONUS)
		{
			product_name = "nam_conusnest";
			directory = "nam";
		}
		else if (product == P_WRF_ARW || product == P_NMMB)
		{
			product_name = "hiresconus";
			directory = "hiresw";
		}
		else if (product == P_GFS)
		{
			product_name = "gfs_0p25_1hr";
			directory = "gfs";
		}




		return bName ? product_name : directory;
	}



	CUIGribHistorical::CUIGribHistorical(void)
	{}

	CUIGribHistorical::~CUIGribHistorical(void)
	{}


	std::string CUIGribHistorical::Option(size_t i)const
	{
		string str;
		switch (i)
		{
			//	case SOURCES: str = "NOMADS=Archived (NOMADS)|NCEP=Current (NCEP)"; break;
				//case SERVER_TYPE: str = "HTTP|FTP"; break;
		case PRODUCT: str = "HRRR|RAP|NAM"; break;
		case DIMENSION:str = "2d (Surface)|3d"; break;
		case VARIABLE:str = "Supported by BioSIM|All"; break;

		};
		return str;
	}

	std::string CUIGribHistorical::Default(size_t i)const
	{
		std::string str;
		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()); break;
			//		case SERVER_TYPE: str = "0"; break;
		case PRODUCT: str = "2"; break;
		case DIMENSION:str = "0"; break;
		case VARIABLE:str = "0"; break;
		case FIRST_DATE:
		case LAST_DATE:   str = CTRef::GetCurrentTRef().GetFormatedString("%Y-%m-%d"); break;
			//case SHOW_WINSCP: str = "0"; break;
		};

		return str;
	}



	//************************************************************************************************************
	//Load station definition list section
	string CUIGribHistorical::GetDirectoryName(size_t product, size_t dimension)
	{
		string name = PRODUCT_NAME[product];
		if (dimension == D_SURFACE)
			name += "(SFC)";
		else if (dimension == D_3D)
			name += "(3D)";

		return name;
	}


	string CUIGribHistorical::GetInputFilePath(size_t product, size_t dimension, CTRef TRef, size_t HH)const
	{
		string inputPath;

		//download gribs file
		static const char* HTTP_FORMAT[NB_PRODUCTS] =
		{
			"",
			"",
			"/data/rapid-refresh/access/%s/analysis/%4d%02d/%4d%02d%02d/rap_130_%4d%02d%02d_%02d00_%03d.grb",
			"/data/north-american-mesoscale-model/access/%s/analysis/%4d%02d/%4d%02d%02d/nam_218_%4d%02d%02d_%02d00_%03d.grb",
			"",
			"",
			"",
			"/data/global-forecast-system/access/%s/analysis/%4d%02d/%4d%02d%02d/gfs_4_%4d%02d%02d_%02d00_%03d.grb"
		};

		static const char* SUB_DIR[NB_PRODUCTS] =
		{
			"",
			"",
			"rap-130-13km",
			"",
			"",
			"",
			"",
			"grid-004-0.5-degree"
		};

		static const CTRef GRIB_2_DATE[NB_PRODUCTS] =
		{
			CTRef(),
			CTRef(),
			CTRef(2009, JANUARY, DAY_01, 0),
			CTRef(2017, APRIL, DAY_10, 0),
			CTRef(),
			CTRef(),
			CTRef(),
			CTRef(2017, APRIL, DAY_10, 0)
		};

		CTRef now = CTRef::GetCurrentTRef(CTM::HOURLY, true);
		bool bHistorical = TRef <= CTRef(2020, MAY, DAY_15, 8);
		string type = bHistorical ? "historical" : SUB_DIR[product];
		int y = TRef.GetYear();
		int m = int(TRef.GetMonth() + 1);
		int d = int(TRef.GetDay() + 1);
		int h = int(TRef.GetHour());

		inputPath = FormatA(HTTP_FORMAT[product], type.c_str(), y, m, y, m, d, y, m, d, h, HH);

		if (product == P_RAP && TRef < CTRef(2012, MAY, DAY_09, 0))
			ReplaceString(inputPath, "rap", "ruc2anl");

		if (product == P_NAM && bHistorical && HH != 4 && HH != 5)
		{
			ReplaceString(inputPath, "nam", "namanl");
		}

		if (product == P_NAM && HH == 4 || HH == 5)
			ReplaceString(inputPath, "analysis", "forecast");



		if (TRef >= GRIB_2_DATE[product])//add a 2 to the format
			inputPath += "2";

		return inputPath;
	}

	string CUIGribHistorical::GetOutputFilePath(size_t product, size_t dimension, CTRef TRef, size_t HH, std::string ext)const
	{
		static const char* OUTPUT_FORMAT = "%s\\%4d\\%02d\\%02d\\%s_%4d%02d%02d_%02d00_%03d%s";
		string workingDir = GetDir(WORKING_DIR) + GetDirectoryName(product, dimension);

		int y = TRef.GetYear();
		int m = int(TRef.GetMonth() + 1);
		int d = int(TRef.GetDay() + 1);
		int h = int(TRef.GetHour());

		return FormatA(OUTPUT_FORMAT, workingDir.c_str(), y, m, d, PRODUCT_NAME[product], y, m, d, h, HH, ext.c_str());
	}



	//*************************************************************************************************

	CTPeriod CUIGribHistorical::GetPeriod()const
	{
		CTPeriod p;

		StringVector t1(Get(FIRST_DATE), "-/");
		StringVector t2(Get(LAST_DATE), "-/");
		if (t1.size() == 3 && t2.size() == 3)
			p = CTPeriod(CTRef(ToInt(t1[0]), ToSizeT(t1[1]) - 1, ToSizeT(t1[2]) - 1, FIRST_HOUR), CTRef(ToInt(t2[0]), ToSizeT(t2[1]) - 1, ToSizeT(t2[2]) - 1, LAST_HOUR));

		return p;
	}


	ERMsg CUIGribHistorical::Execute(CCallback& callback)
	{
		ERMsg msg;


		string workingDir = GetDir(WORKING_DIR);
		size_t product = as<size_t>(PRODUCT) + 1;
		size_t dimension = as<size_t>(DIMENSION);
		size_t variable = as<size_t>(VARIABLE);
		string product_name = GetProductName(product, dimension, true);

		if (product == P_HRDPS)
		{
			msg.ajoute("HRDPS is not available for historical download");
			return msg;
		}

		if (product == P_HRRR)
		{
			CHRRR HRRR(workingDir + GetDirectoryName(P_HRRR, dimension) + "\\");
			HRRR.m_product = dimension;
			HRRR.m_source = CHRRR::MESO_WEST;
			HRRR.m_serverType = CHRRR::HTTP_SERVER;
			HRRR.m_bShowWINSCP = false;
			HRRR.m_period = GetPeriod();
			HRRR.m_compute_prcp = true;
			HRRR.m_update_last_n_days = 0;
			HRRR.bUseOnlyBioSIMVar = true;
			HRRR.m_createDailyGeotiff = false;//create daily CanUS instead

			return HRRR.Execute(callback);
		}



		//https://www.ncei.noaa.gov/data/north-american-mesoscale-model/access/historical/analysis/
		//https://www.ncei.noaa.gov/data/north-american-mesoscale-model/access/analysis/


		//https://www.ncei.noaa.gov/data/rapid-refresh/access/historical/analysis/
		//https://www.ncei.noaa.gov/data/rapid-refresh/access/rap-130-13km/analysis/202007/


		//https://www.ncei.noaa.gov/data/global-forecast-system/access/historical/analysis/201910/20191001/





		CTRef now = CTRef::GetCurrentTRef(CTM::HOURLY, true);
		CTPeriod period;

		period = GetPeriod();
		if (period.End() >= now - 48)
			period.End() = now - 48;


		if (period.IsInit())
		{
			period.Transform(CTM::HOURLY);


			size_t nbFilesToDownload = 0;
			size_t nbDownloaded = 0;

			CArray<bool> bGrbNeedDownload;
			bGrbNeedDownload.SetSize(period.size());

			for (CTRef TRef = period.Begin(); TRef <= period.End(); TRef++)
			{
				size_t i = (TRef - period.Begin());
				int h = int(TRef.GetHour());
				int HH = 0;

				if (product == P_NAM || product == P_GFS)
				{
					HH = h % 6;
					h = int(h / 6) * 6;
					ASSERT(h == 0 || h == 6 || h == 12 || h == 18);
				}

				bGrbNeedDownload[i] = !GoodGeotiff(GetOutputFilePath(product, dimension, TRef, HH, ".tif"));
				nbFilesToDownload += bGrbNeedDownload[i] ? 1 : 0;

				msg += callback.StepIt(0);
			}

			callback.PushTask("Download historical " + product_name + " gribs for period " + period.GetFormatedString("%1 to %2") + ": " + to_string(nbFilesToDownload) + " files", nbFilesToDownload);
			callback.AddMessage("Download historical " + product_name + " gribs for period " + period.GetFormatedString("%1 to %2") + ": " + to_string(nbFilesToDownload) + " files");

			if (nbFilesToDownload > 0)
			{
				size_t nbTry = 0;
				CTRef curTRef = period.Begin();

				while (curTRef < period.End() && msg)
				{
					nbTry++;

					CInternetSessionPtr pSession;
					CHttpConnectionPtr pConnection;
					msg = GetHttpConnection("www.ncei.noaa.gov", pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);


					if (msg)
					{
						try
						{
							while (curTRef <= period.End() && msg)
							{
								size_t curHH = (curTRef - period.Begin());
								if (bGrbNeedDownload[curHH])
								{
									int h = int(curTRef.GetHour());
									int HH = 0;
									
									if (product == P_NAM || product == P_GFS)
									{
										HH = h % 6;
										h = int(h / 6) * 6;
										ASSERT(h == 0 || h == 6 || h == 12 || h == 18);
										
										if (HH == 0)
										{
											size_t nb_gribs = 0;
											for (size_t HH = 0; HH <= 6&&msg; HH++)
											{
												string inputPath = GetInputFilePath(product, dimension, curTRef, HH);
												string outputPath = GetOutputFilePath(product, dimension, curTRef, HH, ".grb2");
												CreateMultipleDir(GetPath(outputPath));


												msg += CopyFile(pConnection, inputPath, outputPath, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE, true, callback);
												if (GoodGrib(outputPath))
												{
													nb_gribs++;
												}
												else
												{
													WBSF::RemoveFile(outputPath);
												}
											}


											if (msg)
											{
												if(nb_gribs==6)//create hourly precipitation only if all 6 files exist
													msg += CreateHourlyPrcp(product, dimension, curTRef, callback);

												if (msg)
												{
													for (size_t HH = 0; HH < 6; HH++)
													{
														string outputPath = GetOutputFilePath(product, dimension, curTRef, HH, ".grb2");
														
														if (FileExists(outputPath) )
														{
															string prcpOutputPath = GetOutputFilePath(product, dimension, curTRef, HH, "_prcp.tif");
															if (!FileExists(prcpOutputPath))
																prcpOutputPath.clear();

															msg += CreateHourlyGeotiff(outputPath, prcpOutputPath, callback);
															if (msg)
																nbDownloaded++;
														}
													}
												}
											}

											//remove last gribs 
											string outputPath = GetOutputFilePath(product, dimension, curTRef, 6, ".grb2");
											if (FileExists(outputPath))
												msg += RemoveFile(outputPath);
										}
									}
									else
									{
										string inputPath = GetInputFilePath(product, dimension, curTRef, HH);
										string outputPath = GetOutputFilePath(product, dimension, curTRef, HH, ".grb2");
										CreateMultipleDir(GetPath(outputPath));

										msg += CopyFile(pConnection, inputPath, outputPath, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE, true, callback);
										if (msg)
										{
											if (GoodGrib(outputPath))
											{
												//download precipitation

												string prcpOutputPath;



												inputPath = GetInputFilePath(product, dimension, curTRef, HH + 1);
												prcpOutputPath = GetOutputFilePath(product, dimension, curTRef, HH + 1, "_prcp.grb2");

												msg += CopyFile(pConnection, inputPath, prcpOutputPath, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE, true, callback);

												if (msg && !GoodGrib(prcpOutputPath))
												{
													callback.AddMessage("Invalid prcp grib:" + prcpOutputPath);
													msg += RemoveFile(prcpOutputPath);
												}

												if (msg)
												{

													msg += CreateHourlyGeotiff(outputPath, prcpOutputPath, callback);
													if (msg)
														nbDownloaded++;
												}
											}



										}
										else
										{
											//remove file
											callback.AddMessage("Invalid prcp grib:" + outputPath);
											msg += RemoveFile(outputPath);
										}
									}

									if (msg)
									{
										nbTry = 0;
										msg += callback.StepIt();
									}
								}//need download

								if (msg)
									curTRef++;
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

						//clean connection
						pConnection->Close();
						pSession->Close();
					}
				}
			}

			callback.AddMessage(GetString(IDS_NB_FILES_DOWNLOADED) + ToString(nbDownloaded));
			callback.PopTask();
		}

		return msg;
	}

	size_t GetVar(const std::map<std::string, std::string>& meta_data)
	{
		size_t var;
		auto it = meta_data.find("GRIB_ELEMENT");
		if (it != meta_data.end())
			var = CSfcDatasetCached::get_var(it->second);

		return var;
	}

	string GetLevelType(const std::map<std::string, std::string>& meta_data)
	{
		string type;
		auto it = meta_data.find("GRIB_SHORT_NAME");
		ASSERT(it != meta_data.end());

		if (it != meta_data.end())
		{
			StringVector tmp(it->second, "-");
			if (tmp.size() == 2)
				type = tmp[1];
		}

		return type;
	}

	int GetLevel(const std::map<std::string, std::string>& meta_data)
	{
		int level = -999;
		auto it = meta_data.find("GRIB_SHORT_NAME");
		ASSERT(it != meta_data.end());

		if (it != meta_data.end())
		{
			StringVector tmp(it->second, "-");
			if (tmp.size() == 2)
			{
				level = ToInt(tmp[0]);
				
				auto it2 = meta_data.find("description");
				if(it2 != meta_data.end())
				{
					string description = it2->second;
					if (description.find("[hPa]") != string::npos)
						level *= 100;
				}
			}
		}

		return level;

	}


	ERMsg CUIGribHistorical::CreateVRT(const string& inputFilePath, const string& inputPrcpFilePath, const string& file_path_vrt, bool bBioSIMVar, bool bSurface)
	{
		ERMsg msg;

		CGDALDatasetEx DS1;
		msg += DS1.OpenInputImage(inputFilePath);
		CGDALDatasetEx DS2;
		if(!inputPrcpFilePath.empty())
			msg += DS2.OpenInputImage(inputPrcpFilePath);

		if (msg)
		{
			ASSERT(!DS2.IsOpen() || DS1.GetRasterXSize() == DS2.GetRasterXSize());
			ASSERT(!DS2.IsOpen() || DS1.GetRasterYSize() == DS2.GetRasterYSize());

			string prj_WKT = DS1.GetPrj()->GetWKT();

			double GT[6] = { 0 };
			DS1->GetGeoTransform(GT);

			BandsMetaData meta_data;
			DS1.GetBandsMetaData(meta_data);

			ofStream oFile;
			msg = oFile.open(file_path_vrt);
			if (msg)
			{
				oFile << "<VRTDataset rasterXSize=\"" + to_string(DS1.GetRasterXSize()) + "\" rasterYSize=\"" + to_string(DS1.GetRasterYSize()) + "\">" << endl;
				oFile << "  <SRS>" + prj_WKT + "</SRS>" << endl;
				oFile << FormatA("  <GeoTransform>%lf, %lf, %lf, %lf, %lf, %lf</GeoTransform>", GT[0], GT[1], GT[2], GT[3], GT[4], GT[5]) << endl;

				size_t bb = 0;
				for (size_t b = 0; b < DS1.GetRasterCount() && msg; b++)
				{
					bool bIn = true;
					if (bBioSIMVar)
					{
						//it is a BioSIM var?
						size_t var = GetVar(meta_data[b]);
						string levelType = GetLevelType(meta_data[b]);
						int level = GetLevel(meta_data[b]);

						bool bVar = var != NOT_INIT;
						//bool bLevelType = levelType == "ISBL" || levelType == "SFC" || levelType == "HTGL" || levelType == "HYBL";
						//bool bLevel = level <= 120 || (level >= 75000 && level <= 110000);
						bool b1 = (levelType == "SFC" || levelType == "HTGL");//&& level <= 120
						bool b2 = (levelType == "ISBL") && (level >= 75000 && level <= 110000);
						//bool b3 = levelType == "HYBL";
						bIn = bVar && (b1 || b2);
					}

					if (bIn && bSurface)
					{
						int level = GetLevel(meta_data[b]);
						bIn = level <= 10;
					}

					if (bIn)
					{
						string new_description = meta_data[b]["description"];
						StringVector description(meta_data[b]["description"], "(=");
						if (description.size() >= 1)//replace description of level by description of variable: more useful in QGIS
						{
							new_description = description[0] + " \"" + meta_data[b]["GRIB_COMMENT"] + "\"";
							
							//if (description[0].find("[hPa]") != string::npos)
							//	level *= 100;
						}
							


						oFile << "  <VRTRasterBand dataType=\"Float64\" band=\"" << ToString(bb + 1) << "\">" << endl;
						oFile << "    <Description>" << new_description << "</Description>" << endl; 
						oFile << "    <Metadata>" << endl;
						oFile << "      <MDI key=\"GRIB_COMMENT\">" << meta_data[b]["GRIB_COMMENT"] << "</MDI>" << endl;
						oFile << "      <MDI key=\"GRIB_ELEMENT\">" << meta_data[b]["GRIB_ELEMENT"] << "</MDI>" << endl;
						oFile << "      <MDI key=\"GRIB_SHORT_NAME\">" << meta_data[b]["GRIB_SHORT_NAME"] << "</MDI>" << endl;
						//if (meta_data[b]["GRIB_ELEMENT"] == "TMP")
							//oFile << "      <MDI key=\"GRIB_UNIT\">" << "[K]" << "</MDI>" << endl;
						//else
						oFile << "      <MDI key=\"GRIB_UNIT\">" << meta_data[b]["GRIB_UNIT"] << "</MDI>" << endl;
						oFile << "      <MDI key=\"GRIB_FORECAST_SECONDS\">" << meta_data[b]["GRIB_FORECAST_SECONDS"] << "</MDI>" << endl;
						oFile << "    </Metadata>" << endl;

						oFile << "    <NoDataValue>9999</NoDataValue>" << endl;
						oFile << "    <ComplexSource>" << endl;
						oFile << "      <SourceFilename relativeToVRT=\"1\">" << GetFileName(inputFilePath) << "</SourceFilename>" << endl;
						oFile << "      <SourceBand>" + to_string(b + 1) + "</SourceBand>" << endl;
						oFile << "      <SourceProperties RasterXSize=\"" + to_string(DS1.GetRasterXSize()) + "\" RasterYSize=\"" + to_string(DS1.GetRasterYSize()) + "\" DataType=\"Float64\" BlockXSize=\"" + to_string(DS1.GetRasterXSize()) + "\" BlockYSize=\"1\" />" << endl;
						oFile << "      <SrcRect xOff=\"0\" yOff=\"0\" xSize=\"" + to_string(DS1.GetRasterXSize()) + "\" ySize=\"" + to_string(DS1.GetRasterYSize()) + "\" />" << endl;
						oFile << "      <DstRect xOff=\"0\" yOff=\"0\" xSize=\"" + to_string(DS1.GetRasterXSize()) + "\" ySize=\"" + to_string(DS1.GetRasterYSize()) + "\" />" << endl;
						oFile << "      <NODATA>9999</NODATA>" << endl;
						oFile << "    </ComplexSource>" << endl;
						oFile << "  </VRTRasterBand>" << endl;
						bb++;
					}

				}

				//add prcp
				if (DS2.IsOpen())
				{
					DS2.GetBandsMetaData(meta_data);
					//find precipitation band
					size_t in_b = NOT_INIT;
					for (size_t i = 0; i < meta_data.size() && in_b == NOT_INIT; i++)
					{
						string var = meta_data[i]["GRIB_ELEMENT"];
						if (var.find("APCP") != string::npos)
							in_b = i;
					}

					if (in_b != NOT_INIT)
					{
						string new_description = meta_data[in_b]["description"];
						StringVector description(meta_data[in_b]["description"], "=(");
						if (description.size() >= 1)
							new_description = description[0] + " \"" + meta_data[in_b]["GRIB_COMMENT"] + "\"";

						oFile << "  <VRTRasterBand dataType=\"Float64\" band=\"" << ToString(bb + 1) << "\">" << endl;
						oFile << "    <Description>" << new_description << "</Description>" << endl;
						oFile << "    <Metadata>" << endl;
						oFile << "      <MDI key=\"GRIB_COMMENT\">" << meta_data[in_b]["GRIB_COMMENT"] << "</MDI>" << endl;
						oFile << "      <MDI key=\"GRIB_ELEMENT\">" << meta_data[in_b]["GRIB_ELEMENT"] << "</MDI>" << endl;
						oFile << "      <MDI key=\"GRIB_SHORT_NAME\">" << meta_data[in_b]["GRIB_SHORT_NAME"] << "</MDI>" << endl;
						oFile << "      <MDI key=\"GRIB_UNIT\">" << meta_data[in_b]["GRIB_UNIT"] << "</MDI>" << endl;
						oFile << "      <MDI key=\"GRIB_FORECAST_SECONDS\">" << meta_data[in_b]["GRIB_FORECAST_SECONDS"] << "</MDI>" << endl;
						oFile << "    </Metadata>" << endl;

						oFile << "    <NoDataValue>9999</NoDataValue>" << endl;
						oFile << "    <ComplexSource>" << endl;
						oFile << "      <SourceFilename relativeToVRT=\"1\">" << GetFileName(inputPrcpFilePath) << "</SourceFilename>" << endl;
						oFile << "      <SourceBand>" + to_string(in_b + 1) + "</SourceBand>" << endl;
						oFile << "      <SourceProperties RasterXSize=\"" + to_string(DS2.GetRasterXSize()) + "\" RasterYSize=\"" + to_string(DS2.GetRasterYSize()) + "\" DataType=\"Float64\" BlockXSize=\"" + to_string(DS2.GetRasterXSize()) + "\" BlockYSize=\"1\" />" << endl;
						oFile << "      <SrcRect xOff=\"0\" yOff=\"0\" xSize=\"" + to_string(DS2.GetRasterXSize()) + "\" ySize=\"" + to_string(DS2.GetRasterYSize()) + "\" />" << endl;
						oFile << "      <DstRect xOff=\"0\" yOff=\"0\" xSize=\"" + to_string(DS2.GetRasterXSize()) + "\" ySize=\"" + to_string(DS2.GetRasterYSize()) + "\" />" << endl;
						oFile << "      <NODATA>9999</NODATA>" << endl;
						oFile << "    </ComplexSource>" << endl;
						oFile << "  </VRTRasterBand>" << endl;

						
					}
				}

				oFile << "</VRTDataset>" << endl;
				oFile.close();
			}

			DS1.Close();
			if(DS2.IsOpen())
				DS2.Close();
		}//if msg

		return msg;
	}

	ERMsg CUIGribHistorical::CreateHourlyGeotiff(const string& inputFilePath, const string& inputPrcpFilePath, CCallback& callback)const
	{
		ERMsg msg;

		callback.PushTask(string("Create Geotiff ") + GetFileTitle(inputFilePath), NOT_INIT);

		size_t variable = as<size_t>(VARIABLE);
		size_t dimension = as<size_t>(DIMENSION);



		string file_path_vrt = inputFilePath;
		SetFileExtension(file_path_vrt, ".vrt");
		msg += CreateVRT(inputFilePath, inputPrcpFilePath, file_path_vrt, variable == V_BIOSIM_VAR, dimension == D_SURFACE);

		string file_path_tif = inputFilePath;
		SetFileExtension(file_path_tif, ".tif");

		//-stats : do not include stat to avoid the creation of the xml file
		//string argument = "-ot Float32 -co COMPRESS=LZW -co PREDICTOR=3 -co TILED=YES -co BLOCKXSIZE=256 -co BLOCKYSIZE=256";// -a_srs \"" + prj4 + "\"";
		//string command = "\"" + GetApplicationPath() + "External\\gdal_translate.exe\" " + argument + " \"" + file_path_vrt + "\" \"" + file_path_tif + "\"";
		string gdal_data_path = GetApplicationPath() + "External\\gdal-data";
		string projlib_path = GetApplicationPath() + "External\\projlib";
		string plugin_path = GetApplicationPath() + "External\\gdalplugins";
		string option = "--config GDAL_DATA \"" + gdal_data_path + "\" --config PROJ_LIB \"" + projlib_path + "\" --config GDAL_DRIVER_PATH \"" + plugin_path + "\"";

		string argument = "-unscale -ot Float32 -co COMPRESS=LZW -co PREDICTOR=3 -co TILED=YES -co BLOCKXSIZE=256 -co BLOCKYSIZE=256";
		string command = "\"" + GetApplicationPath() + "External\\gdal_translate.exe\" " + option + " " + argument + " \"" + file_path_vrt + "\" \"" + file_path_tif + "\"";

		msg += WinExecWait(command);


		msg += RemoveFile(inputFilePath);
		if(!inputPrcpFilePath.empty())
			msg += RemoveFile(inputPrcpFilePath);
		msg += RemoveFile(file_path_vrt);


		//verify that geotif is valid
		if (FileExists(file_path_tif) && !GoodGeotiff(file_path_tif))
		{
			msg.ajoute("Invalid hourly file:" + file_path_tif);
			msg += RemoveFile(file_path_tif);
		}


		callback.PopTask();

		return msg;
	}

	CTRef CUIGribHistorical::GetTRef(size_t s, const std::pair<std::string, std::string>& remote)
	{
		CTRef TRef;


		StringVector tmp1(remote.first, ".");
		ASSERT(tmp1.size() == 2);
		string date = tmp1[1].substr(0, 8);

		int year = ToInt(date.substr(0, 4));
		size_t m = ToSizeT(date.substr(4, 2)) - 1;
		size_t d = ToSizeT(date.substr(6, 2)) - 1;


		StringVector tmp2(remote.second, ".");
		ASSERT(tmp2.size() == 4 || tmp2.size() == 5 || tmp2.size() == 6);
		string name = tmp2[1];
		ASSERT(name.length() == 4);
		size_t h = WBSF::as<size_t >(name.substr(1, 2));

		return CTRef(year, m, d, h);
	}

	size_t CUIGribHistorical::GetHH(size_t s, const std::pair<std::string, std::string>& remote)
	{
		StringVector tmp(remote.second, ".");
		ASSERT(tmp.size() == 4 || tmp.size() == 5 || tmp.size() == 6);
		static const size_t POS_NAME[NB_PRODUCTS] = { 2,2,2,2,3,3,3,4 };
		string name = tmp[POS_NAME[s]];

		size_t nbDigit = (s == P_GFS) ? 3 : 2;
		size_t hh = WBSF::as<size_t >(name.substr(name.length() - nbDigit));

		return hh;
	}


	ERMsg CUIGribHistorical::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		return msg;
	}


	ERMsg CUIGribHistorical::GetGribsList(CTPeriod p, CGribsMap& gribsList, CCallback& callback)
	{
		ERMsg msg;

		size_t product = as<size_t>(PRODUCT) + 1;
		size_t dimension = as<size_t>(DIMENSION);
		string workingDir = GetDir(WORKING_DIR) + GetDirectoryName(product, dimension) + "\\";


		int firstYear = p.Begin().GetYear();
		int lastYear = p.End().GetYear();
		size_t nbYears = lastYear - firstYear + 1;

		for (size_t y = 0; y < nbYears; y++)
		{
			int year = firstYear + int(y);

			StringVector fileList = GetFilesList(workingDir + ToString(year) + "\\*.tif", FILE_PATH, true);
			for (size_t i = 0; i < fileList.size(); i++)
			{
				CTRef TRef = GetTRef(fileList[i]);
				if (p.IsInside(TRef))
					gribsList[TRef] = fileList[i];
			}
		}


		return msg;
	}

	ERMsg CUIGribHistorical::GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		return msg;
	}

	CTRef CUIGribHistorical::GetTRef(string filePath)
	{
		CTRef TRef;
		StringVector name(GetFileTitle(filePath), "_");
		ASSERT(name.size() == 4);

		if (name.size() == 4)
		{
			int year = WBSF::as<int>(name[1].substr(0, 4));
			size_t m = WBSF::as<int>(name[1].substr(4, 2)) - 1;
			size_t d = WBSF::as<int>(name[1].substr(6, 2)) - 1;
			size_t h = WBSF::as<int>(name[2].substr(0, 2));
			size_t hh = WBSF::as<int>(name[3].substr(0, 3));
			TRef = CTRef(year, m, d, h) + hh;//h+hh can be greater than 23
		}

		return TRef;
	}



	//ERMsg CUIGribHistorical::ExecuteHRRR(CCallback& callback)
	//{
	//	string workingDir = GetDir(WORKING_DIR) + GetDirectoryName(P_HRRR, D_3D) + "\\";
	//	CreateMultipleDir(workingDir);

	//	CHRRR HRRR(workingDir);
	//	HRRR.m_product = CHRRR::HRRR_3D;
	//	HRRR.m_source = CHRRR::MESO_WEST;
	//	HRRR.m_serverType = CHRRR::HTTP_SERVER;
	//	HRRR.m_bShowWINSCP = false;
	//	HRRR.m_period = GetPeriod();
	//	HRRR.m_compute_prcp = true;
	//	HRRR.m_update_last_n_days = 0;
	//	HRRR.bUseOnlyBioSIMVar = true;
	//	HRRR.m_createDailyGeotiff = false;//create daily CanUS instead

	//	return HRRR.Execute(callback);
	//}

	ERMsg CUIGribHistorical::CreateHourlyPrcp(size_t product, size_t dimension, CTRef TRef, CCallback& callback)const
	{
		ERMsg msg;

		string file_path_vrt = GetOutputFilePath(product, dimension, TRef, 0, "_prcp.vrt");

		ofStream oFile;
		msg = oFile.open(file_path_vrt);
		if (msg)
		{

			for (size_t HH = 0; HH <= 6; HH++)
			{
				string prcpOutputPath = GetOutputFilePath(product, dimension, TRef, HH, ".grb2");

				CGDALDatasetEx DS;
				msg += DS.OpenInputImage(prcpOutputPath);

				if (msg)
				{
					if (HH == 0)
					{
						double GT[6] = { 0 };
						DS->GetGeoTransform(GT);

						string prj_WKT = DS.GetPrj()->GetWKT();
						oFile << "<VRTDataset rasterXSize=\"" + to_string(DS.GetRasterXSize()) + "\" rasterYSize=\"" + to_string(DS.GetRasterYSize()) + "\">" << endl;
						oFile << "  <SRS>" + prj_WKT + "</SRS>" << endl;
						oFile << FormatA("  <GeoTransform>%lf, %lf, %lf, %lf, %lf, %lf</GeoTransform>", GT[0], GT[1], GT[2], GT[3], GT[4], GT[5]) << endl;
					}


					BandsMetaData meta_data;
					DS.GetBandsMetaData(meta_data);
					//find precipitation band
					size_t in_b = NOT_INIT;
					for (size_t i = 0; i < meta_data.size() && in_b == NOT_INIT; i++)
					{
						string var = meta_data[i]["GRIB_ELEMENT"];
						if (var.find("APCP") != string::npos)
							in_b = i;
					}

					if (in_b != NOT_INIT)
					{
						string new_description = meta_data[in_b]["description"];
						StringVector description(meta_data[in_b]["description"], "=(");
						if (description.size() >= 1)
							new_description = description[0] + " \"" + meta_data[in_b]["GRIB_COMMENT"] + "\"";

						oFile << "  <VRTRasterBand dataType=\"Float64\" band=\"" << ToString(HH+1) << "\">" << endl;
						oFile << "    <Description>" << new_description << "</Description>" << endl;
						oFile << "    <Metadata>" << endl;
						oFile << "      <MDI key=\"GRIB_COMMENT\">" << meta_data[in_b]["GRIB_COMMENT"] << "</MDI>" << endl;
						oFile << "      <MDI key=\"GRIB_ELEMENT\">" << meta_data[in_b]["GRIB_ELEMENT"] << "</MDI>" << endl;
						oFile << "      <MDI key=\"GRIB_SHORT_NAME\">" << meta_data[in_b]["GRIB_SHORT_NAME"] << "</MDI>" << endl;
						oFile << "      <MDI key=\"GRIB_UNIT\">" << meta_data[in_b]["GRIB_UNIT"] << "</MDI>" << endl;
						oFile << "      <MDI key=\"GRIB_FORECAST_SECONDS\">" << meta_data[in_b]["GRIB_FORECAST_SECONDS"] << "</MDI>" << endl;
						oFile << "    </Metadata>" << endl;

						oFile << "    <NoDataValue>9999</NoDataValue>" << endl;
						oFile << "    <ComplexSource>" << endl;
						oFile << "      <SourceFilename relativeToVRT=\"1\">" << GetFileName(prcpOutputPath) << "</SourceFilename>" << endl;
						oFile << "      <SourceBand>" + to_string(in_b + 1) + "</SourceBand>" << endl;
						oFile << "      <SourceProperties RasterXSize=\"" + to_string(DS.GetRasterXSize()) + "\" RasterYSize=\"" + to_string(DS.GetRasterYSize()) + "\" DataType=\"Float64\" BlockXSize=\"" + to_string(DS.GetRasterXSize()) + "\" BlockYSize=\"1\" />" << endl;
						oFile << "      <SrcRect xOff=\"0\" yOff=\"0\" xSize=\"" + to_string(DS.GetRasterXSize()) + "\" ySize=\"" + to_string(DS.GetRasterYSize()) + "\" />" << endl;
						oFile << "      <DstRect xOff=\"0\" yOff=\"0\" xSize=\"" + to_string(DS.GetRasterXSize()) + "\" ySize=\"" + to_string(DS.GetRasterYSize()) + "\" />" << endl;
						oFile << "      <NODATA>9999</NODATA>" << endl;
						oFile << "    </ComplexSource>" << endl;
						oFile << "  </VRTRasterBand>" << endl;
					}
				}

			}

			oFile << "</VRTDataset>" << endl;
			oFile.close();
		}

		if (msg)
		{
			for (size_t HH = 0; HH < 6; HH++)
			{

				string prcpOutputPath = GetOutputFilePath(product, dimension, TRef, HH, "_prcp.tif");

				//string argument;
				//if (HH == 1)
					//argument = "-e \"prcp=max(0,round( (i1b1)*100)/100)\" -ot Float32 -dstNoData 9999 -stats -overwrite -co COMPRESS=LZW -co TILED=YES -co BLOCKXSIZE=256 -co BLOCKYSIZE=256 \"" + file_path_vrt + "\" \"" + prcpOutputPath + "\"";
				//else
				string gdal_data_path = GetApplicationPath() + "External\\gdal-data";
				string projlib_path = GetApplicationPath() + "External\\projlib";
				string plugin_path = GetApplicationPath() + "External\\gdalplugins";
				string option = "--config GDAL_DATA \"" + gdal_data_path + "\" --config PROJ_LIB \"" + projlib_path + "\" --config GDAL_DRIVER_PATH \"" + plugin_path + "\"";

				string argument = "-e \"prcp=max(0,round( (i1b" + to_string(HH + 2) + "-i1b" + to_string(HH+1) + ")*100)/100)\" -ot Float32 -dstNoData 9999 -stats -overwrite -co COMPRESS=LZW -co TILED=YES -co BLOCKXSIZE=256 -co BLOCKYSIZE=256";

				string command = "\"" + GetApplicationPath() + "External\\ImageCalculator.exe\" " + option + " " + argument + " \"" + file_path_vrt + "\" \"" + prcpOutputPath + "\"";
				msg += WinExecWait(command);
				msg += callback.StepIt(0);

				GDALDataset* poDS = (GDALDataset *)GDALOpenEx(prcpOutputPath.c_str(), GDAL_OF_UPDATE | GDAL_OF_RASTER | GDAL_OF_VERBOSE_ERROR, NULL, NULL, NULL);
				if (poDS != NULL)
				{
					//update metadata
					GDALRasterBand* pBandout = poDS->GetRasterBand(1);
					pBandout->SetDescription(CSfcGribDatabase::META_DATA[H_PRCP][M_DESC]);
					pBandout->SetMetadataItem("GRIB_COMMENT", CSfcGribDatabase::META_DATA[H_PRCP][M_COMMENT]);
					pBandout->SetMetadataItem("GRIB_ELEMENT", CSfcGribDatabase::META_DATA[H_PRCP][M_ELEMENT]);
					pBandout->SetMetadataItem("GRIB_SHORT_NAME", CSfcGribDatabase::META_DATA[H_PRCP][M_SHORT_NAME]);
					pBandout->SetMetadataItem("GRIB_UNIT", CSfcGribDatabase::META_DATA[H_PRCP][M_UNIT]);

					GDALClose((GDALDatasetH)poDS);
				}
				else
				{
					msg.ajoute(CPLGetLastErrorMsg());
					msg.ajoute("Invalid prcp file:" + prcpOutputPath);
					msg += RemoveFile(prcpOutputPath);
				}
			}

		}

		//remove vrt
		msg += RemoveFile(file_path_vrt);


		return msg;
	}
}
