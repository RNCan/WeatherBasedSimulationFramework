#include "StdAfx.h"
#include "UIGribForecast.h"
#include "HRDPS.h"
#include "HRRR.h"
#include "Basic/FileStamp.h"
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
	//filtered
	//URL =	https://nomads.ncep.noaa.gov/cgi-bin/filter_nam.pl?
	//file=nam.t00z.awphys00.tm00.grib2&lev_1000_mb=on&lev_10_m_above_ground=on&lev_2_m_above_ground=on&lev_800_mb=on&lev_80_m_above_ground=on&lev_825_mb=on&lev_850_mb=on&lev_875_mb=on&lev_900_mb=on&lev_90-60_mb_above_ground=on&lev_925_mb=on&lev_950_mb=on&lev_975_mb=on&lev_surface=on&var_APCP=on&var_DZDT=on&var_HGT=on&var_PRES=on&var_RH=on&var_TMP=on&var_UGRD=on&var_VGRD=on&var_VVEL=on&leftlon=0&rightlon=360&toplat=90&bottomlat=-90&dir=%2Fnam.20200622


	//*********************************************************************
	const char* CUIGribForecast::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "Product", "Dimension", "Variable", "MaxHour", "DeleteAfter" };
	const size_t CUIGribForecast::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_COMBO_INDEX, T_COMBO_INDEX, T_COMBO_INDEX, T_STRING_SELECT, T_STRING_SELECT };
	const UINT CUIGribForecast::ATTRIBUTE_TITLE_ID = IDS_UPDATER_GRIB_FORECAST_P;
	const UINT CUIGribForecast::DESCRIPTION_TITLE_ID = ID_TASK_GRIB_FORECAST;

	const char* CUIGribForecast::CLASS_NAME() { static const char* THE_CLASS_NAME = "GribForecast";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIGribForecast::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIGribForecast::CLASS_NAME(), (createF)CUIGribForecast::create);

	const char* CUIGribForecast::PRODUCT_NAME[NB_PRODUCTS] = { "HRDPS", "HRRR", "RAP", "NAM", "NAM3km", "WRF-ARW", "NMMB", "GFS" };
	string CUIGribForecast::GetProductName(size_t product, size_t dimension, bool bName)
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

	CUIGribForecast::CUIGribForecast(void)
	{}

	CUIGribForecast::~CUIGribForecast(void)
	{}


	std::string CUIGribForecast::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case PRODUCT: str = "HRDPS|HRRR|RAP|NAM|NAM (3km)|HiResW(WRF-ARW)|HiResW(NMMB)|GFS(0.25)"; break;
		case DIMENSION:str = "2d (Surface)|3d"; break;
		case VARIABLE:str = "Supported by BioSIM|All"; break;
		};
		return str;
	}

	std::string CUIGribForecast::Default(size_t i)const
	{
		std::string str;
		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()); break;
		case PRODUCT: str = "3"; break;
		case DIMENSION:str = "0"; break;
		case VARIABLE:str = "0"; break;
		case MAX_HOUR: str = "48"; break;
		case DELETE_AFTER: str = "7"; break;
		};

		return str;
	}



	//************************************************************************************************************
	//Load station definition list section
	string CUIGribForecast::GetDirectoryName(size_t product, size_t dimension)
	{
		string name = PRODUCT_NAME[product];
		if (dimension == D_SURFACE)
			name += "(SFC)";
		else if (dimension == D_3D)
			name += "(3D)";

		return name;
	}

	string CUIGribForecast::GetOutputFilePath(size_t product, size_t dimension, CTRef TRef, size_t HH, std::string ext)const
	{
		static const char* OUTPUT_FORMAT = "%s%4d%02d%02d\\%s_%4d%02d%02d_%02d00_%03d%s";
		string workingDir = GetDir(WORKING_DIR) + "Forecast\\" + GetDirectoryName(product, dimension) + "\\";

		int y = TRef.GetYear();
		int m = int(TRef.GetMonth() + 1);
		int d = int(TRef.GetDay() + 1);
		int h = int(TRef.GetHour());

		return FormatA(OUTPUT_FORMAT, workingDir.c_str(), y, m, d, PRODUCT_NAME[product], y, m, d, h, HH, ext.c_str());
	}



	//*************************************************************************************************


	ERMsg CUIGribForecast::Execute(CCallback& callback)
	{
		ERMsg msg;

		size_t product = as<size_t>(PRODUCT);
		size_t dimension = as<size_t>(DIMENSION);
		size_t variable = as<size_t>(VARIABLE);
		string product_name = GetProductName(product, dimension);

		if (product == P_HRDPS)
			return ExecuteHRDPS(callback);

		if (product == P_HRRR && dimension == D_3D)
		{
			//msg.ajoute("ERROR: HRRR 3d is not available throught the filter API");
			return ExecuteHRRR(callback);;
		}


		size_t nbDownload = 0;

		vector<pair<string, string>> fileList;
		msg = GetFilesToDownload(fileList, callback);
		CTPeriod p = CleanList(fileList);

		callback.PushTask(string("Download ") + product_name + " filtered gribs: " + to_string(fileList.size()) + " files " + p.GetFormatedString("(%1 -- %2)"), fileList.size());
		callback.AddMessage(string("Download ") + product_name + " filtered gribs : " + to_string(fileList.size()) + " files " + p.GetFormatedString("(%1 -- %2)"));

		size_t nbTry = 0;
		size_t curI = 0;

		while (curI < fileList.size() && msg)
		{
			nbTry++;

			CInternetSessionPtr pSession;
			CHttpConnectionPtr pConnection;
			msg = GetHttpConnection("nomads.ncep.noaa.gov", pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);
			if (msg)
			{
				try
				{
					for (size_t i = curI; i < fileList.size() && msg; i++)
					{
						CTRef TRef = GetTRef(product, fileList[i]);
						size_t HH = GetHH(product, fileList[i]);

						ASSERT(TRef.IsValid());

						string outputPath = GetOutputFilePath(product, dimension, TRef, HH, ".grb2");
						CreateMultipleDir(GetPath(outputPath));

						string URL = FormatA("/cgi-bin/filter_%s.pl?", product_name.c_str());
						URL += "lev_surface=on&lev_2_m_above_ground=on&lev_10_m_above_ground=on";
						if (dimension == D_3D)
						{
							if (product != P_GFS)
								URL += "&lev_80_m_above_ground=on&lev_750_mb=on&lev_775_mb=on&lev_800_mb=on&lev_825_mb=on&lev_850_mb=on&lev_875_mb=on&lev_900_mb=on&lev_925_mb=on&lev_950_mb=on&lev_975_mb=on&lev_1000_mb=on";
							else
								URL += "&lev_80_m_above_ground=on&lev_750_mb=on&lev_800_mb=on&lev_850_mb=on&lev_900_mb=on&lev_925_mb=on&lev_950_mb=on&lev_975_mb=on&lev_1000_mb=on";
						}




						if (variable == V_BIOSIM_VAR)
						{
							//&var_RH=on
							URL += "&var_DPT=on&var_HGT=on&var_PRES=on&var_TMP=on&var_UGRD=on&var_VGRD=on&var_WEASD=on&var_TCDC=on";

							if (product != P_RAP && product != P_WRF_ARW && product != P_NMMB)
								URL += "&var_DSWRF=on";
							if (dimension == D_3D && product != P_HRRR)
								URL += "&var_VVEL=on";
							if (product != P_WRF_ARW && product != P_NMMB)
								URL += "&var_SNOD=on";

						}
						else if (variable == V_ALL)
						{
							URL += "&all_var=on";
						}

						URL += "&dir=" + fileList[curI].first;
						URL += "&file=" + fileList[curI].second;
						callback.PushTask(string("Downlaod file ") + GetFileName(outputPath), NOT_INIT);
						msg += CopyFile(pConnection, URL, outputPath, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE, true);
						callback.PopTask();


						if (msg)
						{
							if (GoodGrib(outputPath))
							{
								//download precipitation
								string prcpOutputPath;


								if ((product == P_NAM || product == P_NAM_NEST_CONUS ||
									product == P_WRF_ARW || product == P_NMMB || product == P_GFS) &&
									HH >= 1)
								{
									prcpOutputPath = GetOutputFilePath(product, dimension, TRef, HH + 1, "_prcp.tif");
									string file = GetRemoveFile(product, fileList[curI].second, HH + 1);

	
									string URL1 = FormatA("/cgi-bin/filter_%s.pl?lev_surface=on&var_APCP=on", product_name.c_str());
									URL1 += "&dir=" + fileList[curI].first;
									URL1 += "&file=" + fileList[curI].second;

									string URL2 = FormatA("/cgi-bin/filter_%s.pl?lev_surface=on&var_APCP=on", product_name.c_str());
									URL2 += "&dir=" + fileList[curI].first;
									URL2 += "&file=" + file;

									msg += CopyFile(pConnection, URL1, prcpOutputPath + "1", INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE, true, callback);
									msg += CopyFile(pConnection, URL2, prcpOutputPath + "2", INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE, true, callback);
									if (msg)
										msg += CreateHourlyPrcp(prcpOutputPath + "1", prcpOutputPath + "2", prcpOutputPath, callback);
								}
								else
								{
									string URL;
									prcpOutputPath = GetOutputFilePath(product, dimension, TRef, HH + 1, "_prcp.grb2");

									string file = GetRemoveFile(product, fileList[curI].second, HH + 1);
									
									URL = FormatA("/cgi-bin/filter_%s.pl?lev_surface=on&var_APCP=on", product_name.c_str());
									URL += "&dir=" + fileList[curI].first;
									URL += "&file=" + file;

									msg += CopyFile(pConnection, URL, prcpOutputPath, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_DONT_CACHE, true, callback);
								}

								if (msg)
								{

									msg += CreateHourlyGeotiff(outputPath, prcpOutputPath, callback);
									if (msg)
									{
										if (product == P_GFS)
										{
											SetFileExtension(outputPath, ".tif");
											Shift_180(outputPath, callback);
										}

										nbDownload++;
									}
								}
							}
							else
							{
								//remove file
								msg += RemoveFile(outputPath);
							}

							curI++;
							msg += callback.StepIt();
						}
					}
				}
				catch (CException* e)
				{
					//msg = UtilWin::SYGetMessage(*e);
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

		callback.AddMessage(string("Number of ") + product_name + " filtered gribs downloaded: " + ToString(nbDownload));
		callback.PopTask();


		msg += RemoveOldFile(callback);

		return msg;
	}

	ERMsg CUIGribForecast::CreateVRT(const string& inputFilePath, const string& inputPrcpFilePath, const string& file_path_vrt)
	{
		ERMsg msg;

		CGDALDatasetEx DS1;
		msg += DS1.OpenInputImage(inputFilePath);
		CGDALDatasetEx DS2;//DS2 dcan be invalid
		bool bValidPrcp = DS2.OpenInputImage(inputPrcpFilePath);

		if (msg)
		{
			ASSERT(!bValidPrcp || DS1.GetRasterXSize() == DS2.GetRasterXSize());
			ASSERT(!bValidPrcp || DS1.GetRasterYSize() == DS2.GetRasterYSize());
			
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

				size_t b = 0;
				//for (StringVector::iterator it4 = fileList.begin(); it4 != fileList.end() && msg; it4++)
				for (; b < DS1.GetRasterCount() && msg; b++)
				{
					string new_description = meta_data[b]["description"];
					StringVector description(meta_data[b]["description"], "=");
					if (description.size() == 2)
					{
						new_description = description[0] + " \"" + meta_data[b]["GRIB_COMMENT"] + "\"";
					}

					oFile << "  <VRTRasterBand dataType=\"Float64\" band=\"" << ToString(b + 1) << "\">" << endl;
					oFile << "    <Description>" << new_description << "</Description>" << endl;
					oFile << "    <Metadata>" << endl;
					oFile << "      <MDI key=\"GRIB_COMMENT\">" << meta_data[b]["GRIB_COMMENT"] << "</MDI>" << endl;
					oFile << "      <MDI key=\"GRIB_ELEMENT\">" << meta_data[b]["GRIB_ELEMENT"] << "</MDI>" << endl;
					oFile << "      <MDI key=\"GRIB_SHORT_NAME\">" << meta_data[b]["GRIB_SHORT_NAME"] << "</MDI>" << endl;
					//if (meta_data[b]["GRIB_ELEMENT"] == "TMP")
//						oFile << "      <MDI key=\"GRIB_UNIT\">" << "[K]" << "</MDI>" << endl;
	//				else
					oFile << "      <MDI key=\"GRIB_UNIT\">" << meta_data[b]["GRIB_UNIT"] << "</MDI>" << endl;
					oFile << "      <MDI key=\"GRIB_FORECAST_SECONDS\">" << meta_data[b]["GRIB_FORECAST_SECONDS"] << "</MDI>" << endl;
					oFile << "    </Metadata>" << endl;

					//double nodat = DS1.GetNoData(b);
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


				}

				if (bValidPrcp)
				{
					//add prcp

					DS2.GetBandsMetaData(meta_data);
					string new_description = meta_data[0]["description"];
					StringVector description(meta_data[0]["description"], "=");
					if (description.size() == 2)
					{
						new_description = description[0] + " \"" + meta_data[0]["GRIB_COMMENT"] + "\"";
					}

					oFile << "  <VRTRasterBand dataType=\"Float64\" band=\"" << ToString(b + 1) << "\">" << endl;
					oFile << "    <Description>" << new_description << "</Description>" << endl;
					oFile << "    <Metadata>" << endl;
					oFile << "      <MDI key=\"GRIB_COMMENT\">" << meta_data[0]["GRIB_COMMENT"] << "</MDI>" << endl;
					oFile << "      <MDI key=\"GRIB_ELEMENT\">" << meta_data[0]["GRIB_ELEMENT"] << "</MDI>" << endl;
					oFile << "      <MDI key=\"GRIB_SHORT_NAME\">" << meta_data[0]["GRIB_SHORT_NAME"] << "</MDI>" << endl;
					oFile << "      <MDI key=\"GRIB_UNIT\">" << meta_data[0]["GRIB_UNIT"] << "</MDI>" << endl;
					oFile << "      <MDI key=\"GRIB_FORECAST_SECONDS\">" << meta_data[0]["GRIB_FORECAST_SECONDS"] << "</MDI>" << endl;
					oFile << "    </Metadata>" << endl;

					oFile << "    <NoDataValue>9999</NoDataValue>" << endl;
					oFile << "    <ComplexSource>" << endl;
					oFile << "      <SourceFilename relativeToVRT=\"1\">" << GetFileName(inputPrcpFilePath) << "</SourceFilename>" << endl;
					oFile << "      <SourceBand>1</SourceBand>" << endl;
					oFile << "      <SourceProperties RasterXSize=\"" + to_string(DS2.GetRasterXSize()) + "\" RasterYSize=\"" + to_string(DS2.GetRasterYSize()) + "\" DataType=\"Float64\" BlockXSize=\"" + to_string(DS2.GetRasterXSize()) + "\" BlockYSize=\"1\" />" << endl;
					oFile << "      <SrcRect xOff=\"0\" yOff=\"0\" xSize=\"" + to_string(DS2.GetRasterXSize()) + "\" ySize=\"" + to_string(DS2.GetRasterYSize()) + "\" />" << endl;
					oFile << "      <DstRect xOff=\"0\" yOff=\"0\" xSize=\"" + to_string(DS2.GetRasterXSize()) + "\" ySize=\"" + to_string(DS2.GetRasterYSize()) + "\" />" << endl;
					oFile << "      <NODATA>9999</NODATA>" << endl;
					oFile << "    </ComplexSource>" << endl;
					oFile << "  </VRTRasterBand>" << endl;
				}

				oFile << "</VRTDataset>" << endl;
				oFile.close();
			}

			DS1.Close();
			if(bValidPrcp)
				DS2.Close();
		}//if msg

		return msg;
	}

	ERMsg CUIGribForecast::CreateHourlyGeotiff(const string& inputFilePath, const string& inputPrcpFilePath, CCallback& callback)const
	{
		ERMsg msg;

		callback.PushTask(string("Create Geotiff ") + GetFileTitle(inputFilePath), NOT_INIT);


		string file_path_vrt = inputFilePath;
		SetFileExtension(file_path_vrt, ".vrt");
		msg += CreateVRT(inputFilePath, inputPrcpFilePath, file_path_vrt);

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
		msg += RemoveFile(inputPrcpFilePath);
		msg += RemoveFile(file_path_vrt);


		//verify tha geotif is valid
		if( !GoodGeotiff(file_path_tif) )
		{
			msg.ajoute("Invalid hourly file:" + file_path_tif);
			msg += RemoveFile(file_path_tif);
		}


		callback.PopTask();

		return msg;
	}

	ERMsg CUIGribForecast::Shift_180(const string& inputFilePath, CCallback& callback)
	{
		ERMsg msg;

		CGDALDatasetEx DSin;
		msg += DSin.OpenInputImage(inputFilePath);
		if (msg)
		{

			float no_data = 9999;
			CBaseOptions options;
			DSin.UpdateOption(options);
			options.m_bOverwrite = true;
			options.m_extents.m_xMin -= 180;
			options.m_extents.m_xMax -= 180;

			options.m_createOptions.push_back("COMPRESS=LZW");
			options.m_createOptions.push_back("PREDICTOR=3");
			options.m_createOptions.push_back("TILED=YES");
			options.m_createOptions.push_back("BLOCKXSIZE=256");
			options.m_createOptions.push_back("BLOCKYSIZE=256");

			CGDALDatasetEx DSout;
			msg += DSout.CreateImage(inputFilePath + "2", options);
			if (msg)
			{
				callback.PushTask(string("Shift 180° ") + GetFileTitle(inputFilePath), DSin.GetRasterCount());
				for (size_t b = 0; b < DSin.GetRasterCount() && msg; b++)
				{
					GDALRasterBand* pBandin = DSin.GetRasterBand(b);
					GDALRasterBand* pBandout = DSout.GetRasterBand(b);

					ASSERT(DSin.GetRasterXSize() == DSout.GetRasterXSize());
					ASSERT(DSin.GetRasterYSize() == DSout.GetRasterYSize());

					vector<float> data(DSin.GetRasterXSize()*DSin.GetRasterYSize());
					pBandin->RasterIO(GF_Read, 0, 0, DSin.GetRasterXSize(), DSin.GetRasterYSize(), &(data[0]), DSin.GetRasterXSize(), DSin.GetRasterYSize(), GDT_Float32, 0, 0);
					for (size_t y = 0; y < DSin.GetRasterYSize() && msg; y++)
					{
						for (size_t x = 0; x < DSin.GetRasterXSize() / 2; x++)
						{
							size_t xy1 = y * DSin.GetRasterXSize() + x;
							size_t xy2 = y * DSin.GetRasterXSize() + DSin.GetRasterXSize() / 2 + x;
							float tmp = data[xy1];
							data[xy1] = data[xy2];
							data[xy2] = tmp;
						}
						msg += callback.StepIt(0);
					}
					pBandout->RasterIO(GF_Write, 0, 0, DSin.GetRasterXSize(), DSin.GetRasterYSize(), &(data[0]), DSin.GetRasterXSize(), DSin.GetRasterYSize(), GDT_Float32, 0, 0);


					if (pBandin->GetDescription())
						pBandout->SetDescription(pBandin->GetDescription());

					if (pBandin->GetMetadata())
						pBandout->SetMetadata(pBandin->GetMetadata());

					msg += callback.StepIt();
				}

				DSout->FlushCache();
				DSout.Close();

				callback.PopTask();
			}

			DSin.Close();

		}//if dataset open



		if (msg)
		{
			//copy the file to fully use compression with GDAL_translate
			// -stats do not use stat to avoid creation of .xml file
//			string argument = "-ot Float32 -co COMPRESS=LZW -co PREDICTOR=3 -co TILED=YES -co BLOCKXSIZE=256 -co BLOCKYSIZE=256 \"" + inputFilePath + "2" + "\" \"" + inputFilePath + "\"";
			//string command = "\"" + GetApplicationPath() + "External\\gdal_translate.exe\" " + argument;
			string gdal_data_path = GetApplicationPath() + "External\\gdal-data";
			string projlib_path = GetApplicationPath() + "External\\projlib";
			string plugin_path = GetApplicationPath() + "External\\gdalplugins";
			string option = "--config GDAL_DATA \"" + gdal_data_path + "\" --config PROJ_LIB \"" + projlib_path + "\" --config GDAL_DRIVER_PATH \"" + plugin_path + "\"";

			string argument = "-unscale -ot Float32 -co COMPRESS=LZW -co PREDICTOR=3 -co TILED=YES -co BLOCKXSIZE=256 -co BLOCKYSIZE=256";
			string command = "\"" + GetApplicationPath() + "External\\gdal_translate.exe\" " + option + " " + argument + " \"" + inputFilePath + "2" + "\" \"" + inputFilePath + "\"";

			msg += WinExecWait(command);
			msg += RemoveFile(inputFilePath + "2");
		}



		return msg;
	}

	ERMsg CUIGribForecast::CreateHourlyPrcp(const string& inputFilePath1, const string& inputFilePath2, const string& outputFilePath, CCallback& callback)const
	{
		ERMsg msg;

		if (GoodGrib(inputFilePath1) && GoodGrib(inputFilePath2))
		{
			string gdal_data_path = GetApplicationPath() + "External\\gdal-data";
			string projlib_path = GetApplicationPath() + "External\\projlib";
			string plugin_path = GetApplicationPath() + "External\\gdalplugins";
			string option = "--config GDAL_DATA \"" + gdal_data_path + "\" --config PROJ_LIB \"" + projlib_path + "\" --config GDAL_DRIVER_PATH \"" + plugin_path + "\"";

			string argument = "-e \"prcp=max(0,round( (i2b1-i1b1)*100)/100)\" -ot Float32 -dstNoData 9999 -stats -overwrite -co COMPRESS=LZW -co TILED=YES -co BLOCKXSIZE=256 -co BLOCKYSIZE=256";
			string command = "\"" + GetApplicationPath() + "External\\ImageCalculator.exe\" " + option + " " + argument + " \"" + inputFilePath1 + "\" \"" + inputFilePath2 + "\" \"" + outputFilePath + "\"";
			msg += WinExecWait(command);
			msg += callback.StepIt(0);

			GDALDataset* poDS = (GDALDataset *)GDALOpenEx(outputFilePath.c_str(), GDAL_OF_UPDATE | GDAL_OF_RASTER | GDAL_OF_VERBOSE_ERROR, NULL, NULL, NULL);
			if (poDS != NULL)
			{
				//update metadata
				GDALRasterBand* pBandout = poDS->GetRasterBand(1);
				pBandout->SetDescription(CSfcGribDatabase::META_DATA[H_PRCP][M_DESC]);
				pBandout->SetMetadataItem("GRIB_COMMENT", CSfcGribDatabase::META_DATA[H_PRCP][M_COMMENT]);
				pBandout->SetMetadataItem("GRIB_ELEMENT", CSfcGribDatabase::META_DATA[H_PRCP][M_ELEMENT]);
				pBandout->SetMetadataItem("GRIB_SHORT_NAME", CSfcGribDatabase::META_DATA[H_PRCP][M_SHORT_NAME]);
				//pBandout->SetMetadataItem("GRIB_FORECAST_SECONDS", "");
				pBandout->SetMetadataItem("GRIB_UNIT", CSfcGribDatabase::META_DATA[H_PRCP][M_UNIT]);

				GDALClose((GDALDatasetH)poDS);
			}
			else
			{
				msg.ajoute(CPLGetLastErrorMsg());
				msg.ajoute("Invalid prcp file:" + outputFilePath);
				msg += RemoveFile(outputFilePath);
			}
		}

		msg += RemoveFile(inputFilePath1);
		msg += RemoveFile(inputFilePath2);


		return msg;
	}

	ERMsg CUIGribForecast::ExecuteHRDPS(CCallback& callback)
	{
		ERMsg msg;


		size_t dimension = as<size_t>(DIMENSION);
		size_t variable = as<size_t>(VARIABLE);

		string workingDir = GetDir(WORKING_DIR) + "Forecast\\"+GetDirectoryName(P_HRDPS, dimension) + "\\";
		CreateMultipleDir(workingDir);


		CHRDPS HRDPS(workingDir);
		HRDPS.m_bForecast = true;
		HRDPS.m_bHRDPA6h = true;
		HRDPS.m_max_hours = 48;
		HRDPS.m_update_last_n_days = 0;
		HRDPS.m_bCreateDailyGeotiff = false;//create daily CanUS instead


		if (variable == D_SURFACE)
		{
			CHRDPSVariables sfc("APCP_SFC|DSWRF_SFC|HGT_SFC|PRES_SFC|SNOD_SFC|TCDC_SFC");
			CHRDPSVariables tlg("DPT_TGL|RH_TGL|TMP_TGL|WDIR_TGL|WIND_TGL");

			HRDPS.m_variables = (sfc | tlg);
			HRDPS.m_heights.FromString("2|10");
		}
		else
		{
			CHRDPSVariables sfc("APCP_SFC|DSWRF_SFC|HGT_SFC|PRES_SFC|SNOD_SFC|TCDC_SFC");
			CHRDPSVariables tlg("DPT_TGL|RH_TGL|TMP_TGL");
			CHRDPSVariables isbl("UGRD_ISBL|VGRD_ISBL|VVEL_ISBL");


			HRDPS.m_variables = (sfc | tlg | isbl);
			HRDPS.m_heights.FromString("2|10|40|80|120");
			HRDPS.m_levels.FromString("1015|1000|0985|0970|0950|0925|0900|0875|0850|0800|0750");
		}

		msg = HRDPS.Execute(callback);

		return msg;

	}

	ERMsg CUIGribForecast::ExecuteHRRR(CCallback& callback)
	{
		string workingDir = GetDir(WORKING_DIR) + "Forecast\\" + GetDirectoryName(P_HRRR, D_3D) + "\\";
		CreateMultipleDir(workingDir);

		CHRRR HRRR(workingDir);
		//HRRR.m_bForecast = true; a faire
		HRRR.m_product = CHRRR::HRRR_3D;
		HRRR.m_source = CHRRR::NOMADS;
		HRRR.m_serverType = CHRRR::HTTP_SERVER;
		HRRR.m_bShowWINSCP = false;
		HRRR.m_period = CTPeriod();
		HRRR.m_compute_prcp = true;
		HRRR.m_update_last_n_days = 0;
		HRRR.m_createDailyGeotiff = false;//create daily CanUS instead

		return HRRR.Execute(callback);
	}



	ERMsg CUIGribForecast::GetFilesToDownload(std::vector<std::pair<std::string, std::string>>& fileList, CCallback& callback)
	{
		ERMsg msg;

		size_t product = as<size_t>(PRODUCT);
		size_t dimension = as<size_t>(DIMENSION);
		ASSERT(product != P_HRDPS);


		CInternetSessionPtr pSession;
		CHttpConnectionPtr pConnection;

		msg = GetHttpConnection("nomads.ncep.noaa.gov", pConnection, pSession, PRE_CONFIG_INTERNET_ACCESS, "", "", false, 5, callback);
		msg += callback.StepIt(0);
		if (msg)
		{
			string product_name = GetProductName(product, dimension);
			//string directory = GetProductName(product, dimension, false);

			//get dates
			string URL = "/cgi-bin/filter_" + product_name + ".pl?";

			string source;
			UtilWWW::GetPageText(pConnection, URL, source);

			//vector<string> directories;

			source = FindString(source, "<table border=0>", "</table>");
			if (!source.empty())
			{
				//bool bFirst = true;
				size_t posBegin = source.find("\">");
				if (posBegin != string::npos)//take the first one
				{
					string dir = FindString(source, "\">", "</a>", posBegin);

					if (!dir.empty())
					{
						string directory;
						if (product == P_GFS)
						{
							string URL2 = URL + "dir=/" + dir;
							string tmp;
							UtilWWW::GetPageText(pConnection, URL2, tmp);

							tmp = FindString(tmp, "<table border=0>", "</table>");
							size_t posBegin = tmp.find("\">");
							while (posBegin != string::npos)//take the last one
							{
								string sub_dir = FindString(tmp, "\">", "</a>", posBegin);

								if (!sub_dir.empty())
									directory = "/" + dir + "/" + sub_dir;
							}


						}
						else
						{
							if (product == P_HRRR)
								dir += "/conus";


							directory = "/" + dir;
						}


						URL = FormatA("/cgi-bin/filter_%s.pl?dir=%s", product_name.c_str(), directory.c_str());
						UtilWWW::GetPageText(pConnection, URL, source);

						source = FindString(source, "<select name=", "</select>");
						size_t posBegin = source.find("<option value=");
						while (posBegin != string::npos)
						{
							string file = FindString(source, "<option value=\"", "\">", posBegin);
							if (!file.empty())
								fileList.push_back(make_pair(directory, file));
						}
						//bFirst = false;
					}
				}



				//callback.PushTask(string("Get files list from: ") + URL, directories.size());
				//for (size_t i = 0; i < directories.size() && msg; i++)
				//{



				//msg += callback.StepIt();
			}//if page not empty

			//callback.PopTask();


			pConnection->Close();
			pSession->Close();
		}//if msg

		return msg;
	}

	CTRef CUIGribForecast::GetTRef(size_t s, const std::pair<std::string, std::string>& remote)
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

	size_t CUIGribForecast::GetHH(size_t s, const std::pair<std::string, std::string>& remote)
	{
		StringVector tmp(remote.second, ".");
		ASSERT(tmp.size() == 4 || tmp.size() == 5 || tmp.size() == 6);
		static const size_t POS_NAME[NB_PRODUCTS] = { 2,2,2,2,3,3,3,4 };
		string name = tmp[POS_NAME[s]];

		size_t nbDigit = (s == P_GFS) ? 3 : 2;
		size_t hh = WBSF::as<size_t >(name.substr(name.length() - nbDigit));

		return hh;
	}

	bool CUIGribForecast::IsNeeded(size_t product, const std::pair<std::string, std::string>& remote)
	{
		bool bNeeded = true;

		if (product == P_RAP)
			bNeeded = remote.second.find("130") != string::npos;
		else if (product == P_WRF_ARW)
			bNeeded = remote.second.find("arw") != string::npos;
		else if (product == P_NMMB)
			bNeeded = remote.second.find("nmmb") != string::npos;
		else if (product == P_GFS)
			bNeeded = remote.second.find(".anl") == string::npos;


		return bNeeded;
	}

	/*bool CUIGribForecast::GoodGeotiff(const std::string& filePath)const
	{
		bool bGood = false;
		
		CGDALDatasetEx DS;
		if (DS.OpenInputImage(filePath))
		{
			DS.Close();
			bGood = true;
		}

		return bGood;
	}*/

	CTPeriod CUIGribForecast::CleanList(std::vector<std::pair<std::string, std::string>>& fileList)
	{
		CTPeriod p;

		size_t product = as<size_t>(PRODUCT);
		size_t dimension = as<size_t>(DIMENSION);
		size_t max_hours = as<size_t>(MAX_HOUR);
		CTRef nowUTC = CTRef::GetCurrentTRef(CTM::HOURLY, true);

		map<CTRef, std::pair<std::string, std::string>> fileListCleanned;

		//do a list of all newest forecast 
		for (auto it = fileList.begin(); it != fileList.end(); it++)
		{
			CTRef TRef = GetTRef(product, *it);
			size_t HH = GetHH(product, *it);
			if (HH <= max_hours)
			{
				ASSERT(TRef.IsValid());

				string outputPath = GetOutputFilePath(product, dimension, TRef, HH, ".tif");

				bool bNeededFile = IsNeeded(product, *it);
				bool bIsForecast = (TRef+HH) >= nowUTC;

				if (bNeededFile && bIsForecast)
				{
					p += TRef + HH;
					fileListCleanned[TRef + HH] = *it;
				}
			}
		}

		//Kepp only file that not already exist
		fileList.clear();
		fileList.reserve(fileListCleanned.size());
		for (auto it = fileListCleanned.begin(); it != fileListCleanned.end(); it++)
		{
			CTRef TRef = GetTRef(product, it->second);
			size_t HH = GetHH(product, it->second);
			string outputPath = GetOutputFilePath(product, dimension, TRef, HH, ".tif");

			if(!GoodGeotiff(outputPath))
				fileList.push_back(it->second);
		}
			

		return p;
	}


	ERMsg CUIGribForecast::RemoveOldFile(CCallback& callback)
	{
		ERMsg msg;

		size_t product = as<size_t>(PRODUCT);
		size_t dimension = as<size_t>(DIMENSION);
		int delete_after = as<int>(DELETE_AFTER);

		string workingDir = GetDir(WORKING_DIR) + "Forecast\\" + GetDirectoryName(product, dimension) + "\\";

		if (product == P_HRDPS)
		{
			CHRDPS::Clean(delete_after * 24, workingDir, callback);
		}
		else if (product == P_HRRR && dimension == D_3D)
		{
			//CHRRR::Clean(delete_after, workingDir, callback);
		}
		else
		{

			CTRef today = CTRef::GetCurrentTRef(CTM::DAILY);

			//StringVector filesList;
			StringVector dirList;

			StringVector dates = WBSF::GetDirectoriesList(workingDir + "20??????");//for security, need years
			for (size_t i = 0; i < dates.size(); i++)
			{
				int year = WBSF::as<int>(dates[i].substr(0, 4));
				size_t m = WBSF::as<size_t>(dates[i].substr(4, 2)) - 1;
				size_t d = WBSF::as<size_t>(dates[i].substr(6, 2)) - 1;
				CTRef date(year, m, d);
				
				if (today - date > delete_after)
				{
					dirList.push_back(workingDir + dates[i]);
				}
			}//for all dates

			string comment = string("Remove old ") + PRODUCT_NAME[product] + " forecast (" + to_string(dirList.size()) + " directories)";
			callback.PushTask(comment, dirList.size());
			callback.AddMessage(comment);


			//remove directory
			for (size_t i = 0; i != dirList.size() && msg; i++)
			{
				StringVector filesList = GetFilesList(workingDir + dates[i] + "/*.tif");
				for (size_t i = 0; i != filesList.size() && msg; i++)
				{
					msg += RemoveFile(filesList[i]);
					msg += callback.StepIt(0);
				}

				WBSF::RemoveDirectory(dirList[i]);
				msg += callback.StepIt();
			}


			callback.PopTask();
		}

		return msg;
	}

	ERMsg CUIGribForecast::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		return msg;
	}


	ERMsg CUIGribForecast::GetGribsList(CTPeriod p, CGribsMap& gribsList, CCallback& callback)
	{
		ERMsg msg;

		size_t product = as<size_t>(PRODUCT);
		size_t dimension = as<size_t>(DIMENSION);
		string workingDir = GetDir(WORKING_DIR) + "Forecast\\"+GetDirectoryName(product, dimension) + "\\";



		int firstYear = p.Begin().GetYear();
		int lastYear = p.End().GetYear();
		size_t nbYears = lastYear - firstYear + 1;

		for (size_t y = 0; y < nbYears; y++)
		{
			int year = firstYear + int(y);

			StringVector fileList = GetFilesList(workingDir + "\\*.tif", FILE_PATH, true);
			for (size_t i = 0; i < fileList.size(); i++)
			{
				CTRef TRef = GetTRef(fileList[i]);
				if (p.IsInside(TRef))
					gribsList[TRef] = fileList[i];
			}
		}


		return msg;
	}

	ERMsg CUIGribForecast::GetWeatherStation(const std::string& stationName, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		return msg;
	}

	CTRef CUIGribForecast::GetTRef(string filePath)
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

	string CUIGribForecast::GetRemoveFile(size_t product, string file, size_t HH)
	{
		
		char* formatDigit = (product == P_GFS) ? "%03d" : "%02d";
		string strHH = FormatA(formatDigit, HH);
		if (product == P_HRRR)
			file.replace(file.begin() + 17, file.begin() + 19, strHH);
		else if (product == P_RAP)
			file.replace(file.begin() + 20, file.begin() + 22, strHH);
		else if (product == P_NAM)
			file.replace(file.begin() + 15, file.begin() + 17, strHH);
		else if (product == P_NAM_NEST_CONUS)
			file.replace(file.begin() + 25, file.begin() + 27, strHH);
		else if (product == P_WRF_ARW)
			file.replace(file.begin() + 21, file.begin() + 23, strHH);
		else if (product == P_NMMB)
			file.replace(file.begin() + 22, file.begin() + 24, strHH);
		else if (product == P_GFS)
			file.replace(file.begin() + 21, file.begin() + 24, strHH);
		
		return file;
	}
}