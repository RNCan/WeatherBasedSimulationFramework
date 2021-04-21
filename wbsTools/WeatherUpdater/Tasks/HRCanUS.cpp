#include "StdAfx.h"
#include "HRCanUS.h"
#include "HRDPS.h"
#include "UI/Common/SYShowMessage.h"
#include "Geomatic/SfcGribsDatabase.h"


#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"

#include "WeatherBasedSimulationString.h"
#include "../Resource.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;

namespace WBSF
{


	CHRCanUS::CHRCanUS(const std::string& workingDirCan, const std::string& workingDirUS, const std::string& workingDir) :
		m_workingDirCan(workingDirCan),
		m_workingDirUS(workingDirUS),
		m_workingDir(workingDir),
		m_update_last_n_days(7)
	{
	}

	CHRCanUS::~CHRCanUS(void)
	{}



	ERMsg CHRCanUS::Execute(CCallback& callback)
	{
		ERMsg msg;

		GDALSetCacheMax64(128 * 1024 * 1024);

		set<string> date_to_update;

		CTRef TRef = CTRef::GetCurrentTRef(CTM::DAILY);
		for (size_t d = 0; d < m_update_last_n_days; d++)
		{
			date_to_update.insert(TRef.GetFormatedString("%Y%m%d"));
			TRef--;
		}

		msg += CreateHourlyCanUS(date_to_update, callback);
		if (msg)
			msg += CreateDailyCanUS(date_to_update, callback);


		return msg;
	}

	ERMsg CHRCanUS::CreateCanUSGribList(CCallback& callback)const
	{
		ERMsg msg;

		//string filter = FormatA("%s%d\\", .c_str(), year, m + 1, d + 1, year, m + 1, d + 1);
		StringVector years_str = WBSF::GetDirectoriesList(m_workingDir + "????");

		set<int> years;
		for (size_t y = 0; y < years_str.size(); y++)
		{
			int year = ToInt(years_str[y]);
			if (year != 0)
				years.insert(year);
		}

		if (!years.empty())
		{
			//create hourly gribs list
			CTPeriod p = CTPeriod(CTRef(*years.begin(), JANUARY, DAY_01, 0), CTRef(*years.rbegin(), DECEMBER, DAY_31, 23));
			if (*years.rbegin() == WBSF::GetCurrentYear())
				p.End() = CTRef::GetCurrentTRef(CTM::HOURLY);

			CGribsMap gribsList;
			msg += GetGribsList(p, gribsList, callback);
			msg += gribsList.save(m_workingDir + "CanUS_H.gribs");

			//create daily gribs list
			p.Transform(CTM::DAILY);
			msg += GetGribsList(p, gribsList, callback);
			msg += gribsList.save(m_workingDir + "CanUS_D.gribs");
		}

		return msg;
	}


	ERMsg CHRCanUS::CreateHourlyCanUS(set<string> date_to_update, CCallback& callback)
	{
		ERMsg msg;

		static const char* prj_str = "+proj=lcc +lat_0=40 +lon_0=-96 +lat_1=20 +lat_2=60 +x_0=0 +y_0=0 +ellps=GRS80 +towgs84=0,0,0,0,0,0,0 +units=m +no_defs";
		msg += CProjectionManager::CreateProjection(prj_str);
		CProjectionPtr prj = CProjectionManager::GetPrj(prj_str);

		callback.PushTask("Find hourly CanUS to update: " + ToString(date_to_update.size()) + " days", date_to_update.size() * 24);
		set<string> hours_to_update;


		for (set<string>::const_iterator it = date_to_update.begin(); it != date_to_update.end() && msg; it++)
		{
			string year = it->substr(0, 4);
			string month = it->substr(4, 2);
			string day = it->substr(6, 2);


			for (size_t h = 0; h < 24; h++)
			{
				size_t h1 = size_t(h / 6) * 6;
				size_t h2 = h % 6;

				string file_path_can = FormatA("%s%s\\%s\\%s\\HRDPS_%s%s%s%02d-%03d.tif", m_workingDirCan.c_str(), year.c_str(), month.c_str(), day.c_str(), year.c_str(), month.c_str(), day.c_str(), h1, h2);
				string file_path_usa = FormatA("%s%s\\%s\\%s\\hrrr.t%02dz.wrfsfcf00.tif", m_workingDirUS.c_str(), year.c_str(), month.c_str(), day.c_str(), h);
				string file_path_canus = FormatA("%s%s\\%s\\%s\\CanUS_%s%s%s%02d.tif", m_workingDir.c_str(), year.c_str(), month.c_str(), day.c_str(), year.c_str(), month.c_str(), day.c_str(), h);

				CFileInfo info_can = GetFileInfo(file_path_can);
				CFileInfo info_usa = GetFileInfo(file_path_usa);
				CFileInfo info_canus = GetFileInfo(file_path_canus);

				if (info_can.m_time != -1 && info_usa.m_time != -1 &&
					(info_canus.m_time < info_can.m_time || info_canus.m_time < info_usa.m_time))
				{
					string key = FormatA("%s%02d", it->c_str(), h);
					hours_to_update.insert(key);
				}

				/*CGDALDatasetEx DS;
				if (DS.OpenInputImage(filePath))
				{
					DS.Close();
					bDownload = false;
				}*/
				msg += callback.StepIt();
			}//for all hours

		}//for all days


		callback.PopTask();

		callback.PushTask("Create hourly CanUS: " + ToString(hours_to_update.size()) + " hours", hours_to_update.size());
		callback.AddMessage("Create hourly CanUS: " + ToString(hours_to_update.size()) + " hours");


		for (auto it = hours_to_update.begin(); it != hours_to_update.end() && msg; it++)
		{
			string date = *it;
			string year = date.substr(0, 4);
			string month = date.substr(4, 2);
			string day = date.substr(6, 2);
			string hour = date.substr(8, 2);

			size_t h = ToSizeT(hour);
			size_t h1 = size_t(h / 6) * 6;
			size_t h2 = h % 6;

			string file_path_in_can = FormatA("%s%s\\%s\\%s\\HRDPS_%s%s%s%02d-%03d.tif", m_workingDirCan.c_str(), year.c_str(), month.c_str(), day.c_str(), year.c_str(), month.c_str(), day.c_str(), h1, h2);
			string file_path_in_usa = FormatA("%s%s\\%s\\%s\\hrrr.t%02dz.wrfsfcf00.tif", m_workingDirUS.c_str(), year.c_str(), month.c_str(), day.c_str(), h);
			string file_path_out = FormatA("%s%s\\%s\\%s\\CanUS_%s%s%s%02d.tif", m_workingDir.c_str(), year.c_str(), month.c_str(), day.c_str(), year.c_str(), month.c_str(), day.c_str(), h);
			WBSF::CreateMultipleDir(GetPath(file_path_out));

			string file_path_tmp1 = file_path_out + "tmp_can.tif";
			string file_path_tmp2 = file_path_out + "tmp_usa.tif";

			std::array<CSfcDatasetCached, 2> DSin;
			//create and open reprojected can
			string argument = "-te -2700000 -1600000 3300000 3200000 -tr 2500 2500 -r cubic -te_srs \"+proj=lcc +lat_0=40 +lon_0=-96 +lat_1=20 +lat_2=60 +x_0=0 +y_0=0 +ellps=GRS80 +towgs84=0,0,0,0,0,0,0 +units=m +no_defs\" -t_srs \"+proj=lcc +lat_0=40 +lon_0=-96 +lat_1=20 +lat_2=60 +x_0=0 +y_0=0 +ellps=GRS80 +towgs84=0,0,0,0,0,0,0 +units=m +no_defs\" -co COMPRESS=LZW -co PREDICTOR=3 -co TILED=YES -co BLOCKXSIZE=256 -co BLOCKYSIZE=256 -overwrite \"";
			string command = "\"" + GetApplicationPath() + "External\\gdalwarp.exe\" " + argument + file_path_in_can + "\" \"" + file_path_tmp1 + "\"";
			msg += WinExecWait(command);
			msg += callback.StepIt(0);

			command = "\"" + GetApplicationPath() + "External\\gdalwarp.exe\" " + argument + file_path_in_usa + "\" \"" + file_path_tmp2 + "\"";
			msg += WinExecWait(command);
			msg += callback.StepIt(0);

			if (msg)
			{
				msg += DSin[1].open(file_path_tmp2, true);
				msg += DSin[0].open(file_path_tmp1, true);

				GribVariables var_in = DSin[0].get_variables() & DSin[1].get_variables();


				if (msg && var_in.count() > 6)
				{
					float no_data_out = 9999;


					CBaseOptions options;
					DSin[0].UpdateOption(options);
					options.m_format = "GTIFF";
					options.m_nbBands = var_in.count();
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

						callback.PushTask("Create " + file_path_out, var_in.count());

						size_t b_out = 0;
						for (size_t v = 0; v < var_in.size() && msg; v++)
						{
							if (var_in.test(v))
							{
								vector<std::array<float, 2>> data_out(options.m_extents.GetNbPixels());

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

										ASSERT(data.size() == data_out.size());
										for (size_t xy = 0; xy < data.size(); xy++)
										{
											//remove negative precipitation
											if (v == H_PRCP && data[xy] < 0.01)
												data[xy] = 0;

											if (data[xy] > -1E10 && fabs(data[xy] - no_data_in) > 0.1)
												data_out[xy][i] = data[xy];
											else
												data_out[xy][i] = no_data_out;
										}

									}

									msg += callback.StepIt(0);
								}

								vector<float> data(DSout.GetRasterXSize()*DSout.GetRasterYSize(), no_data_out);
								ASSERT(data.size() == data_out.size());

								for (size_t xy = 0; xy < data.size() && msg; xy++)
								{
									if (data_out[xy][0] != no_data_out && data_out[xy][1] != no_data_out)
									{
										int x = int(xy % DSout.GetRasterXSize());
										int y = int(xy / DSout.GetRasterXSize());
										double fx1 = 1.0 - max(0.0, min(1.0, (x - 1940.0) / (2040.0 - 1940.0)));
										double fy1 = max(0.0, min(1.0, (y - 760.0) / (860.0 - 760.0)));

										//X : 1940-2050 
										//Y:990-760
										double f2 = fx1 * fy1;
										double f1 = 1 - f2;
										data[xy] = f1 * data_out[xy][0] + f2 * data_out[xy][1];

									}
									else if (data_out[xy][0] != no_data_out)
									{
										data[xy] = data_out[xy][0];
									}
									else if (data_out[xy][1] != no_data_out)
									{
										data[xy] = data_out[xy][1];
									}

									msg += callback.StepIt(0);
								}

								GDALRasterBand* pBandout = DSout.GetRasterBand(b_out);
								pBandout->RasterIO(GF_Write, 0, 0, DSout.GetRasterXSize(), DSout.GetRasterYSize(), &(data[0]), DSout.GetRasterXSize(), DSout.GetRasterYSize(), GDT_Float32, 0, 0);
								pBandout->SetDescription(CSfcGribDatabase::META_DATA[v][M_DESC]);
								pBandout->SetMetadataItem("GRIB_COMMENT", CSfcGribDatabase::META_DATA[v][M_COMMENT]);
								pBandout->SetMetadataItem("GRIB_ELEMENT", CSfcGribDatabase::META_DATA[v][M_ELEMENT]);
								pBandout->SetMetadataItem("GRIB_SHORT_NAME", CSfcGribDatabase::META_DATA[v][M_SHORT_NAME]);
								pBandout->SetMetadataItem("GRIB_UNIT", CSfcGribDatabase::META_DATA[v][M_UNIT]);
								b_out++;


								msg += callback.StepIt();
							}//if var used
						}//for all variable

						DSout.Close(options);

						callback.PopTask();
					}//out open

					for (size_t i = 0; i < DSin.size(); i++)
						DSin[i].close();

					if (msg)
					{
						//convert with gdal_translate to optimize size
						string argument = "-ot Float32 -a_nodata 9999 -stats -co COMPRESS=LZW -co PREDICTOR=3 -co TILED=YES -co BLOCKXSIZE=256 -co BLOCKYSIZE=256";
						string command = "\"" + GetApplicationPath() + "External\\gdal_translate.exe\" " + argument + " \"" + file_path_out + "2\" \"" + file_path_out + "\"";
						msg += WinExecWait(command);
						msg += RemoveFile(file_path_out + "2");
					}

				}//if input images open

				if (FileExists(file_path_tmp1))
					msg += RemoveFile(file_path_tmp1);

				if (FileExists(file_path_tmp2))
					msg += RemoveFile(file_path_tmp2);

			}//tmp file created

			msg += callback.StepIt();
		}//for all hours

		callback.PopTask();

		return msg;
	}



	ERMsg CHRCanUS::CreateDailyCanUS(set<string> date_to_update, CCallback& callback)
	{
		ERMsg msg;

		float no_data_out = 9999;
		CStatistic::SetVMiss(no_data_out);
		CTRef now = CTRef::GetCurrentTRef(CTM::DAILY);

		//static const char* prj_str = "+proj=lcc +lat_0=40 +lon_0=-96 +lat_1=20 +lat_2=60 +x_0=0 +y_0=0 +ellps=GRS80 +towgs84=0,0,0,0,0,0,0 +units=m +no_defs";
		//msg = CProjectionManager::CreateProjection(prj_str);
		//CProjectionPtr prj = CProjectionManager::GetPrj(prj_str);


		callback.PushTask("Create daily CanUS: " + ToString(date_to_update.size()) + " days", date_to_update.size());
		callback.AddMessage("Create daily CanUS: " + ToString(date_to_update.size()) + " days");


		for (set<string>::const_iterator it = date_to_update.begin(); it != date_to_update.end() && msg; it++)
		{
			string year = it->substr(0, 4);
			string month = it->substr(4, 2);
			string day = it->substr(6, 2);


			string file_path_out = FormatA("%s%s\\%s\\CanUS_%s%s%s.tif", m_workingDir.c_str(), year.c_str(), month.c_str(), year.c_str(), month.c_str(), day.c_str());
			WBSF::CreateMultipleDir(GetPath(file_path_out));

			string filter_canus_h = FormatA("%s%s\\%s\\%s\\CanUS_%s%s%s??.tif", m_workingDir.c_str(), year.c_str(), month.c_str(), day.c_str(), year.c_str(), month.c_str(), day.c_str());
			StringVector canus_files_h = WBSF::GetFilesList(filter_canus_h, 2, true);

			//get last update time
			__time64_t lastUpdate = -1;
			for (StringVector::iterator it4 = canus_files_h.begin(); it4 != canus_files_h.end() && msg; )
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
					callback.AddMessage("Remove invalid CanUS GeoTiff " + *it4);
					msg += RemoveFile(*it4);
					it4 = canus_files_h.erase(it4);
				}
			}

			CTRef TRef;
			TRef.FromFormatedString(year + "-" + month + "-" + day);

			bool bNotUptodate = GetFileInfo(file_path_out).m_time < lastUpdate;
			bool bCreate = (TRef < now) && bNotUptodate;


			if (bCreate)
			{
				callback.PushTask("Create CanUS daily GeoTiff: " + file_path_out, canus_files_h.size() * 10);
				CBaseOptions options;

				GribVariables var_in_all;
				vector<array<CStatistic, NB_VAR_GRIBS>> stats;

				for (auto it = canus_files_h.begin(); it != canus_files_h.end() && msg; it++)
				{
					float no_data_out = 9999;

					CSfcDatasetCached DSin;
					msg += DSin.open(*it, true);

					if (msg)
					{
						ASSERT(stats.empty() || stats.size() == DSin.GetExtents().GetNbPixels());

						if (stats.empty())
						{
							DSin.UpdateOption(options);
							stats.resize(DSin.GetExtents().GetNbPixels());
						}

						GribVariables var_in = DSin.get_variables();
						var_in_all |= var_in;

						size_t b_out = 0;
						for (size_t v = 0; v < var_in.size() && msg; v++)
						{
							if (var_in.test(v))
							{
								size_t b = DSin.get_band(v);
								ASSERT(b != NOT_INIT);


								vector<float> data(DSin.GetExtents().GetNbPixels());
								float no_data_in = DSin.GetNoData(b);
								GDALRasterBand* pBandin = DSin.GetRasterBand(b);
								pBandin->RasterIO(GF_Read, 0, 0, DSin.GetRasterXSize(), DSin.GetRasterYSize(), &(data[0]), DSin.GetRasterXSize(), DSin.GetRasterYSize(), GDT_Float32, 0, 0);
								pBandin->FlushCache();

								for (size_t xy = 0; xy < data.size() && msg; xy++)
								{
									if (fabs(data[xy] - no_data_in) > 0.1)
										stats[xy][v] += data[xy];

									msg += callback.StepIt(0);
								}

								msg += callback.StepIt();
							}
						}


						DSin.close();
					}//if msg
				}//for all inputs

				if (var_in_all[H_TAIR])
				{
					var_in_all.set(H_TMIN);
					var_in_all.set(H_TMAX);
				}

				var_in_all.reset(H_UWND);
				var_in_all.reset(H_VWND);

				callback.PopTask();

				if (msg)
				{
					callback.PushTask("Save CanUS daily GeoTiff: " + file_path_out, var_in_all.count());


					options.m_format = "GTIFF";
					options.m_nbBands = var_in_all.count();
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
						for (size_t v = 0; v < var_in_all.size() && msg; v++)
						{
							if (var_in_all.test(v))
							{
								vector<float> data(DSout.GetRasterXSize()*DSout.GetRasterYSize(), no_data_out);
								ASSERT(stats.size() == data.size());

								for (size_t xy = 0; xy < stats.size(); xy++)
								{
									size_t vv = (v == H_TMIN || v == H_TMAX) ? H_TAIR : v;
									if (stats[xy][vv][NB_VALUE] >= 22)
									{
										TStat stats_type = v == H_TMIN ? LOWEST : v == H_TMAX ? HIGHEST : v == H_PRCP ? SUM : MEAN;
										data[xy] = stats[xy][vv][stats_type];
									}
								}

								GDALRasterBand* pBandout = DSout.GetRasterBand(b_out);
								pBandout->RasterIO(GF_Write, 0, 0, DSout.GetRasterXSize(), DSout.GetRasterYSize(), &(data[0]), DSout.GetRasterXSize(), DSout.GetRasterYSize(), GDT_Float32, 0, 0);
								pBandout->SetDescription(CSfcGribDatabase::META_DATA[v][M_DESC]);
								pBandout->SetMetadataItem("GRIB_COMMENT", CSfcGribDatabase::META_DATA[v][M_COMMENT]);
								pBandout->SetMetadataItem("GRIB_ELEMENT", CSfcGribDatabase::META_DATA[v][M_ELEMENT]);
								pBandout->SetMetadataItem("GRIB_SHORT_NAME", CSfcGribDatabase::META_DATA[v][M_SHORT_NAME]);
								pBandout->SetMetadataItem("GRIB_UNIT", CSfcGribDatabase::META_DATA[v][M_UNIT]);

								msg += callback.StepIt();
								b_out++;
							}//if var used
						}//for all variables

						DSout.Close(options);
					}//out open

					if (msg)
					{
						string argument = "-ot Float32 -a_nodata 9999 -stats -co COMPRESS=LZW -co PREDICTOR=3 -co TILED=YES -co BLOCKXSIZE=256 -co BLOCKYSIZE=256";// -a_srs \"" + prj_str ;
						string command = "\"" + GetApplicationPath() + "External\\gdal_translate.exe\" " + argument + " \"" + file_path_out + "2\" \"" + file_path_out + "\"";
						msg += WinExecWait(command);
						msg += RemoveFile(file_path_out + "2");
					}

					callback.PopTask();
				}
			}//create output


			msg += callback.StepIt();

		}//for all days

		callback.PopTask();



		return msg;
	}


	ERMsg CHRCanUS::GetGribsList(CTPeriod p, CGribsMap& gribsList, CCallback& callback)const
	{
		ERMsg msg;

		CTPeriod pp = p.GetTType() == CTM::HOURLY ? p.as(CTM(CTM::DAILY)) : p.as(CTM(CTM::MONTHLY));

		callback.PushTask(string("Create Gribs (")+ (p.GetTType() == CTM::HOURLY ? "hourly":"daily")+")", pp.size());
		for (CTRef TRef = pp.Begin(); TRef <= pp.End()&&msg; TRef++)
		{
			int year = TRef.GetYear();
			size_t m = TRef.GetMonth();
			size_t d = TRef.GetDay();
			size_t h = TRef.GetHour();

			string file_path;
			if (p.GetTM().Type() == CTM::HOURLY)
				file_path = FormatA("%s%d\\%02d\\%02d\\CanUS_%d%02d%02d%02d.tif", m_workingDir.c_str(), year, m + 1, d + 1, year, m + 1, d + 1, h);
			else
				file_path = FormatA("%s%d\\%02d\\CanUS_%d%02d%02d.tif", m_workingDir.c_str(), year, m + 1, year, m + 1, d + 1);

			if (FileExists(file_path))
				gribsList[TRef] = file_path;

			msg += callback.StepIt();
			//			{
			//				size_t d = TRef.GetDay();
			////				for (size_t d = 0; d < GetNbDayPerMonth(year, m); d++)
			//	//			{
			//				string filter = FormatA("%s%d\\%02d\\%02d\\CANUS_%d%02d%02d*.tif", m_workingDir.c_str(), year, m + 1, d + 1, year, m + 1, d + 1);
			//				StringVector list2 = GetFilesList(m_workingDir + ToString(year) + filter, FILE_PATH, false);
			//				list1.insert(list1.end(), list2.begin(), list2.end());
			//	//		}
			//			}
			//			else
			//			{
			//				string filter = FormatA("%s%d\\%02d\\CANUS_%d%02d*.tif", m_workingDir.c_str(), year, m + 1, year, m + 1);
			//				


//			int year = TRef.GetYear();
//			size_t m = TRef.GetMonth();
//

//			StringVector list1;
//			if (p.GetTM().Type() == CTM::HOURLY)
//			{
//				size_t d = TRef.GetDay();
////				for (size_t d = 0; d < GetNbDayPerMonth(year, m); d++)
//	//			{
//				string filter = FormatA("%s%d\\%02d\\%02d\\CANUS_%d%02d%02d*.tif", m_workingDir.c_str(), year, m + 1, d + 1, year, m + 1, d + 1);
//				StringVector list2 = GetFilesList(m_workingDir + ToString(year) + filter, FILE_PATH, false);
//				list1.insert(list1.end(), list2.begin(), list2.end());
//	//		}
//			}
//			else
//			{
//				string filter = FormatA("%s%d\\%02d\\CANUS_%d%02d*.tif", m_workingDir.c_str(), year, m + 1, year, m + 1);
//				StringVector list2 = GetFilesList(m_workingDir + ToString(year) + filter, FILE_PATH, false);
//				list1.insert(list1.end(), list2.begin(), list2.end());
//			}
//
//			for (size_t i = 0; i < list1.size(); i++)
//			{
//				CTRef TRef = GetLocalTRef(list1[i]);
//				gribsList[TRef] = list1[i];
//			}

		}

		callback.PopTask();
		/*
				int firstYear = p.Begin().GetYear();
				int lastYear = p.End().GetYear();
				size_t nbYears = lastYear - firstYear + 1;

				StringVector list1;
				for (size_t y = 0; y < nbYears; y++)
				{
					int year = firstYear + int(y);
					for (size_t m = 0; m < 12; m++)
					{
						if (p.GetTM().Type() == CTM::HOURLY)
						{
							for (size_t d = 0; d < GetNbDayPerMonth(year,m); d++)
							{
								string filter = FormatA("%s%d\\%02d\\%02d\\CANUS_%d%02d%02d*.tif", m_workingDir.c_str(), year, m+1, d+1, year, m + 1, d + 1);
								StringVector list2 = GetFilesList(m_workingDir + ToString(year) + filter, FILE_PATH, false);
								list1.insert(list1.end(), list2.begin(), list2.end());
							}
						}
						else
						{
							string filter = FormatA("%s%d\\%02d\\CANUS_%d%02d*.tif", m_workingDir.c_str(), year, m + 1, year, m + 1);
							StringVector list2 = GetFilesList(m_workingDir + ToString(year) + filter, FILE_PATH, false);
							list1.insert(list1.end(), list2.begin(), list2.end());
						}

						for (size_t i = 0; i < list1.size(); i++)
						{
							CTRef TRef = GetLocalTRef(list1[i]);
							if (p.IsInside(TRef))
								gribsList[TRef] = list1[i];
						}

					}
				}


				for (size_t i = 0; i < list1.size(); i++)
				{
					CTRef TRef = GetLocalTRef(list1[i]);
					if ( p.IsInside(TRef))
						gribsList[TRef] = list1[i];
				}
		*/


		return msg;
	}


}