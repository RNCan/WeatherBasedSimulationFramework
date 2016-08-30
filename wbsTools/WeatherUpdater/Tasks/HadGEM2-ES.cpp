#include "StdAfx.h"




#include "HadGEM2-ES.h"
#include <boost\multi_array.hpp>
//#include <boost/iostreams/stream.hpp>
//#include <boost/iostreams/filtering_streambuf.hpp>
//#include <boost/iostreams/filter/gzip.hpp>
//#include <boost/iostreams/copy.hpp>

#include "Basic/units.hpp"
#include "Basic/Statistic.h"
#include "Simulation/MonthlyMeanGrid.h"
#include "UI/Common/SYShowMessage.h"



#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"
#include "cpl_conv.h"
#include "ogr_srs_api.h"


#include "../Resource.h"
#include "WeatherBasedSimulationString.h"
#include "TaskFactory.h"


using namespace std;
using namespace netCDF;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::WEATHER;
using namespace WBSF::NORMALS_DATA;


namespace WBSF
{


	static const int FIRST_YEAR = 1950;
	static const int LAST_YEAR = 2100;
	static const size_t NB_YEARS = LAST_YEAR - FIRST_YEAR + 1;

	//*********************************************************************
	const char* CHadGEM2_ES_MMGCreator::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "InputPath", "OutputFilepath", };
	const size_t CHadGEM2_ES_MMGCreator::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_FILEPATH };
	const UINT CHadGEM2_ES_MMGCreator::ATTRIBUTE_TITLE_ID = IDS_MMG_HADGEM2_ES_P;
	const UINT CHadGEM2_ES_MMGCreator::DESCRIPTION_TITLE_ID = ID_TASK_HADGEM2_ES;


	const char* CHadGEM2_ES_MMGCreator::CLASS_NAME(){ static const char* THE_CLASS_NAME = "HADGEM2_ES_MMG_CREATOR";  return THE_CLASS_NAME; }
	CTaskBase::TType CHadGEM2_ES_MMGCreator::ClassType()const { return CTaskBase::TOOLS; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CHadGEM2_ES_MMGCreator::CLASS_NAME(), (createF)CHadGEM2_ES_MMGCreator::create);


	CHadGEM2_ES_MMGCreator::CHadGEM2_ES_MMGCreator(void)
	{
	}

	CHadGEM2_ES_MMGCreator::~CHadGEM2_ES_MMGCreator(void)
	{
	}


	std::string CHadGEM2_ES_MMGCreator::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case INPUT_PATH:		str = GetString(IDS_STR_FILTER_NC); break;
		case OUTPUT_FILEPATH:	str = GetString(IDS_STR_FILTER_MMG); break;
			//case RCP:				str = "RCP 2.6|RCP 4.5|RCP 6.0|RCP 8.5"; break;
		};

		return str;

	}

	std::string CHadGEM2_ES_MMGCreator::Default(size_t i)const
	{
		std::string str;

		return str;
	}



	//*******************************************************************************************************



	const char* CHadGEM2_ES_MMGCreator::VARIABLES_NAMES[NB_VARIABLES] = { "tasmin", "tasmax", "pr", "huss", "sfcWind" };
	const char* CHadGEM2_ES_MMGCreator::RCP_NAME[NB_RCP] = { "RCP26", "RCP45", "RCP60", "RCP85" };

	string GetPeriod(const std::string& filePath)
	{
		string title = GetFileTitle(filePath);
		return title.substr(title.length() - 17);
	}


	ERMsg CHadGEM2_ES_MMGCreator::Execute(CCallback& callback)
	{
		ERMsg msg;


		//	return ExportPoint("D:\\CanRCM4\\Test\\Quebec daily 1950-2100 RCP85.csv", RCP85, CGeoPoint(-71.38, 46.75, PRJ_WGS_84), callback);
		CPLSetConfigOption("GDAL_CACHEMAX", "1000");

		int firstYear = FIRST_YEAR;
		int lastYear = LAST_YEAR;
		int nbYears = lastYear - firstYear + 1;

		//Get extents
		CGeoExtents extents = GetExtents();

		//Load land/water profile
		GetLandWaterProfile(m_landWaterMask);
		//GetDEM(m_DEM);


		CBaseOptions options;
		GetMapOptions(options);

		CMonthlyMeanGrid MMG;
		MMG.m_firstYear = firstYear;
		MMG.m_lastYear = lastYear;

		MMG.m_supportedVariables[TMIN_MN] = true;
		MMG.m_supportedVariables[TMAX_MN] = true;
		MMG.m_supportedVariables[DEL_STD] = true;
		MMG.m_supportedVariables[EPS_STD] = true;
		MMG.m_supportedVariables[PRCP_TT] = true;
		MMG.m_supportedVariables[SPEH_MN] = true;
		MMG.m_supportedVariables[RELH_MN] = true;
		MMG.m_supportedVariables[RELH_SD] = true;
		MMG.m_supportedVariables[WNDS_MN] = true;
		MMG.m_supportedVariables[WNDS_SD] = true;

		CTRef firstTRef(1949, DECEMBER);

		callback.PushTask("Create Hadley GEM2-ES MMG", nbYears * NB_RCP);

		for (size_t rcp = 0; rcp < NB_RCP&&msg; rcp++)
		{
			string filePathOut = Get(OUTPUT_FILEPATH);
			WBSF::SetFileTitle(filePathOut, WBSF::GetFileTitle(filePathOut) + "_" + RCP_NAME[rcp]);
			msg = MMG.Save(filePathOut);
			if (!msg)
				return msg;

			callback.PushTask(string("Open output images for ") + RCP_NAME[rcp], NB_FIELDS);

			CGDALDatasetEx grid[NB_FIELDS];
			for (size_t v = 0; v < NB_FIELDS; v++)
			{
				string filePathOut = MMG.GetFilePath(v);
				if (!filePathOut.empty())
					msg += grid[v].CreateImage(filePathOut, options);

				msg += callback.StepIt();
			}

			callback.PopTask();

			if (!msg)
				return msg;


			size_t b = 0;
			CMonthlyVariableVector data;
			msg += GetMMGForRCP(rcp, data, callback);


			callback.PushTask(string("Save output images for ") + RCP_NAME[rcp], NB_FIELDS);
			for (size_t mm = 0; mm < data.size() && msg; mm++)
			{
				int year = (firstTRef + mm).GetYear();
				if (year >= FIRST_YEAR && year <= LAST_YEAR)
				{
					for (size_t v = 0; v < data[mm].size() && msg; v++)
					{
						if (grid[v].IsOpen())
						{
							GDALRasterBand* pBand = grid[v].GetRasterBand(b);
							pBand->RasterIO(GF_Write, 0, 0, extents.m_xSize, extents.m_ySize, &(data[mm][v][0]), extents.m_xSize, extents.m_ySize, GDT_Float32, 0, 0);
							msg += callback.StepIt();
						}
					}

					b++;
				}
			}

			
			callback.PopTask();

			callback.PushTask("Close MMG", NB_FIELDS);

			for (size_t v = 0; v < NB_FIELDS&&msg; v++)
			{
				grid[v].Close();
				msg += callback.StepIt();
			}

			callback.PopTask();

			msg += callback.StepIt();
		}


		callback.PopTask();

		return msg;

	}


	string CHadGEM2_ES_MMGCreator::GetProjectionWKT(){ return PRJ_WGS_84_WKT; }

	CGeoExtents CHadGEM2_ES_MMGCreator::GetExtents()
	{
		
		
		CGeoExtents extents;
		extents.m_xMin = -0.9375000000000000-180;
		extents.m_xMax = 359.0625000000000000 - 180;
		extents.m_yMin = -90.6250000000000000;
		extents.m_yMax = 90.6250000000000000;
		extents.m_xSize = 192;
		extents.m_ySize = 145;
		extents.m_xBlockSize = 192;
		extents.m_yBlockSize = 1;
		//ASSERT(extents.XRes() == 10000);

		return extents;
	}

	void CHadGEM2_ES_MMGCreator::GetMapOptions(CBaseOptions& options)
	{

		CGeoExtents extents = GetExtents();

		int nbYears = LAST_YEAR - FIRST_YEAR + 1;
		options.m_prj = GetProjectionWKT();
		options.m_extents = extents;
		options.m_nbBands = 12 * nbYears;
		options.m_dstNodata = -999;
		options.m_outputType = GDT_Float32;
		options.m_format = "GTIFF";
		options.m_bOverwrite = true;
		options.m_bComputeStats = true;
		options.m_createOptions.push_back(string("COMPRESS=LZW"));
		//options.m_createOptions.push_back(string("BIGTIFF=YES"));
	}

	void CHadGEM2_ES_MMGCreator::ConvertData(size_t v, std::vector<float>& data)const
	{
		using namespace units::values;
		
		CGeoExtents extents = GetExtents();
		std::vector<float> dataII(data.size());

		for (size_t i = 0; i < data.size(); i++)
		{
			int x = (i + extents.m_xSize/2) % extents.m_xSize;
			int y = extents.m_ySize - int(i / extents.m_xSize) - 1;

			int ii = y*extents.m_xSize + x;

			if (data[i] < 1.0E20 )
			{
				switch (v)
				{
				case V_TMIN:	dataII[ii] = (float)Celsius(K(data[i])).get(); break; //K --> °C
				case V_TMAX:	dataII[ii] = (float)Celsius(K(data[i])).get(); break; //K --> °C
				case V_PRCP:	dataII[ii] = (float)(data[i] * 60 * 60 * 24); break; //kg/(m²s) --> mm/day 
				//case V_SPEH:	dataII[ii] = data[i];  break;
				case V_SPEH:	dataII[ii] = (float)(data[i] * 1000); break; //kg[H2O]/kg[air] --> g[H2O]/kg[air]
				case V_WNDS:	dataII[ii] = (float)kph(meters_per_second(data[i])).get(); break; //m/s --> km/h
				default: ASSERT(false);
				}
			}
			else
			{
				dataII[ii] = -999;
			}
		}
		
		data = dataII;
		//data.swap(dataII);
	}


	ERMsg CHadGEM2_ES_MMGCreator::GetFileList(size_t rcp, vector<array<string, NB_VARIABLES>>& fileList)const
	{
		ERMsg msg;

		string RCPName = RCP_NAME[rcp];

		for (size_t v = 0; v < NB_VARIABLES; v++)
		{
			string fileName = string(VARIABLES_NAMES[v]) + "_day_HadGEM2-ES_historical_r4i1p1_????????-????????.nc";
			string path = GetDir(INPUT_PATH) + "historical" + "\\" + fileName;
			StringVector list = WBSF::GetFilesList(path);
			std::sort(list.begin(), list.end());//ordered by time

			fileName = string(VARIABLES_NAMES[v]) + "_day_HadGEM2-ES_" + RCPName + "_r4i1p1_????????-????????.nc";
			path = GetDir(INPUT_PATH) + RCPName + "\\" + fileName;
			StringVector tmp = WBSF::GetFilesList(path);//Get files
			std::sort(tmp.begin(), tmp.end());//ordered by time

			list.insert(list.end(), tmp.begin(), tmp.end());

			if (fileList.empty())
				fileList.resize(list.size());

			ASSERT(fileList.size() == list.size());
			if (fileList.size() == list.size())
			{
				for (size_t i = 0; i < list.size(); i++)
					fileList[i][v] = list[i];
			}
			else
			{
				msg.ajoute(string("The number of files for variable ") + VARIABLES_NAMES[v] +" is not the same ");
			}
		}

		return msg;

	}


	void CHadGEM2_ES_MMGCreator::ComputeMontlyStatistic(size_t i, const COneMonthData& data, CMonthlyVariables& montlhyStat)
	{


		array < CStatistic, NB_VARIABLES> stat;
		

		for (size_t d = 0; d < data.size(); d++)
		{
			for (size_t v = 0; v < data[d].size(); v++)
			{
				ASSERT(data[d][v][i]>-999);
				stat[v] += data[d][v][i];
			}
		}

		montlhyStat[TMIN_MN][i] = stat[V_TMIN][MEAN];
		montlhyStat[TMAX_MN][i] = stat[V_TMAX][MEAN];


		CStatistic statTmin;
		CStatistic statTmax;
		CStatistic statTmin_max;
		CStatistic statHr;

		for (size_t d = 0; d < data.size(); d++)
		{
			double deltaTmin = data[d][V_TMIN][i] - montlhyStat[TMIN_MN][i];
			double deltaTmax = data[d][V_TMAX][i] - montlhyStat[TMAX_MN][i];
			statTmin += deltaTmin;
			statTmax += deltaTmax;
			statTmin_max += deltaTmin * deltaTmax;

			double Hs = data[d][V_SPEH][i];
			double Hr = Hs2Hr(data[d][V_TMIN][i], data[d][V_TMAX][i], Hs);
			ASSERT(Hr >= 0 && Hr <= 100);

			statHr+=Hr;
		}


		montlhyStat[TMNMX_R][i] = statTmin_max[SUM] / sqrt(statTmin[SUM²] * statTmax[SUM²]);
		montlhyStat[DEL_STD][i] = statTmin[STD_DEV];
		montlhyStat[EPS_STD][i] = statTmax[STD_DEV];

		montlhyStat[PRCP_TT][i] = stat[V_PRCP][SUM];
		//standard deviation of prcp is not computed in MMG

		montlhyStat[SPEH_MN][i] = float(stat[V_SPEH][MEAN]);
		montlhyStat[RELH_MN][i] = float(statHr[MEAN]);
		montlhyStat[RELH_SD][i] = float(statHr[STD_DEV]);
									
		montlhyStat[WNDS_MN][i] = float(stat[V_WNDS][MEAN]);
		montlhyStat[WNDS_SD][i] = float(stat[V_WNDS][STD_DEV]);

	}


	//typedef shared_ptr<COneDayData> CCOneDayDataPtr;
	
	ERMsg CHadGEM2_ES_MMGCreator::GetMMGForRCP(size_t rcp, CMonthlyVariableVector& dataOut, CCallback& callback)
	{
		ERMsg msg;

		//static const TVarH H_VARIABLES[NB_VARIABLES] = { H_TAIR, H_TRNG, H_PRCP, H_RELH, H_WNDS };

		CGeoExtents extents = GetExtents();

		vector<array<string, NB_VARIABLES>> fileList;
		msg = GetFileList(rcp, fileList);
		if (!msg)
			return msg;


		callback.PushTask(string("Create data for RPC = ") + RCP_NAME[rcp], fileList.size());



		//open files


		size_t mmm = 0;
		for (size_t i = 0; i < fileList.size() && msg; i++)
		//for (size_t i = fileList.size() - 1; i < fileList.size() && msg; i--)
		{
			NcFilePtrArray ncFiles;


			for (size_t v = 0; v < NB_VARIABLES&&msg; v++)
			{
				if (FileExists(fileList[i][v]))
				{
					try
					{
						ncFiles[v] = NcFilePtr(new NcFile(fileList[i][v], NcFile::read));
						msg += callback.StepIt(0);
					}
					catch (exceptions::NcException& e)
					{
						msg.ajoute(e.what());
						msg.ajoute(string("Unable to open file : ") + fileList[i][v]);
					}

				}
				else
				{
					msg.ajoute("Missing file : " + fileList[i][v]); 
				}
			}

			if (msg)
			{


				auto timeGrid = ncFiles.front()->getDim("time");
				size_t nbDays = timeGrid.getSize();
				size_t nbMonths = nbDays / 30;
				ASSERT(nbDays % 30 == 0);

				string p = GetPeriod(fileList[i].front());
				callback.PushTask(string("Create data for period = ") + p, nbMonths * 30);

				dataOut.resize(dataOut.size() + nbMonths);
				for (size_t mm = 0; mm < nbMonths; mm++)
					for (size_t v = 0; v < NB_FIELDS; v++)
						dataOut[mmm + mm][v].insert(dataOut[mmm + mm][v].begin(), extents.m_ySize*extents.m_xSize, -999);

				//data is composed of 30 days per month by 12 months
				for (size_t mm = 0; mm < nbMonths && msg; mm++)
				{
					COneMonthData data;

					for (size_t d = 0; d < 30 && msg; d++)
					{
						ASSERT(data[d].size() == ncFiles.size());
						for (size_t v = 0; v < data[d].size() && msg; v++)
							data[d][v].resize(extents.m_ySize*extents.m_xSize);

						for (size_t v = 0; v < ncFiles.size() && msg; v++)
						{
							try
							{
								auto timeGrid = ncFiles[v]->getDim("time");
								auto latGrid = ncFiles[v]->getDim("lat");
								auto lonGrid = ncFiles[v]->getDim("lon");
								ASSERT(timeGrid.getSize() == nbDays);

								size_t nbLat = latGrid.getSize();
								size_t nbLon = lonGrid.getSize();

								vector<size_t> startp = { { mm * 30 + d, 0, 0 } };
								vector<size_t> countp = { { 1, nbLat, nbLon } };

								NcVar& var = ncFiles[v]->getVar(VARIABLES_NAMES[v]);
								var.getVar(startp, countp, &(data[d][v][0]));
							}
							catch (exceptions::NcException& e)
							{
								msg.ajoute(e.what());
								msg.ajoute(string("error processing file : ") + fileList[i][v]);
							}

							ConvertData(v, data[d][v]);
						}

						msg += callback.StepIt();

					}//for all days (30) of the month 


#pragma omp parallel for
					for (__int64 i = 0; i < (int)extents.m_ySize*extents.m_xSize; i++)
					{
						if (m_landWaterMask[i])
							ComputeMontlyStatistic((size_t)i, data, dataOut[mmm]);
					}
					mmm++;
				}//for all months

				callback.PopTask();
			}//if msg

			
			msg += callback.StepIt();
		}//for all files


		callback.PopTask();

		return msg;
	}


	ERMsg CHadGEM2_ES_MMGCreator::GetLandWaterProfile(CLandWaterBitset& landWaterMask)
	{
	
		ERMsg msg;
		string filePath = GetDir(INPUT_PATH) + "sftlf_fx_HadGEM2-ES_historical_r0i0p0.nc";
		try
		{
			//open files
			NcFile ncFile(filePath, NcFile::read);
			
			auto latGrid = ncFile.getDim("lat");
			auto lonGrid = ncFile.getDim("lon");
			size_t nbLat = latGrid.getSize();
			size_t nbLon = lonGrid.getSize();
			

			CGeoExtents extents = GetExtents();
			vector<float> data(extents.m_xSize*extents.m_ySize);
			vector<size_t> startp = { { 0, 0 } };
			vector<size_t> countp = { { nbLat, nbLon } };

			NcVar& var = ncFile.getVar("sftlf");
			var.getVar(startp, countp, &(data[0]));

			landWaterMask.resize(data.size());

			for (size_t i = 0; i < data.size(); i++)
			{
				int x = (i + extents.m_xSize / 2 )% extents.m_xSize;
				int y = extents.m_ySize - int(i / extents.m_xSize) - 1;

				int ii = y*extents.m_xSize + x;
				landWaterMask[ii] = data[i] > 0;
			}
				
		}
		catch (exceptions::NcException& e)
		{
			msg.ajoute(e.what());
		}

		return msg;
	}



	ERMsg CHadGEM2_ES_MMGCreator::ExportPoint(string filePath, int rcp, CGeoPoint pt, CCallback& callback)
	{
		ERMsg msg;


		ofStream file;
		msg = file.open(filePath);
		if (msg)
		{
			CGeoExtents extents = GetExtents();
			GetLandWaterProfile(m_landWaterMask);
			CMatrix<float> DEM;

			GetDEM(DEM);

			callback.PushTask("Export Point", NB_VARIABLES);
			//WBSF::Get
			//pt.Reproject(::GetReProjection(pt.GetPrjID(), extents.GetPrjID()));
			ASSERT(extents.IsInside(pt));
			//X = 17.25   Y = 2.10 

			callback.AddMessage(string("X = ") + to_string(pt.m_x));
			callback.AddMessage(string("Y = ") + to_string(pt.m_y));
			CGeoPointIndex xy = extents.CoordToXYPos(pt);
			xy.m_y = extents.m_ySize - xy.m_y - 1;//reverse lat

			array< vector<float>, NB_VARIABLES>  data;
			try
			{
				vector<array<string, NB_VARIABLES>> filesList;
				msg = GetFileList(rcp, filesList);
				if (!msg)
					return msg;

				callback.PushTask(string("Extract point data for RPC = ") + RCP_NAME[rcp], filesList.size()*NB_VARIABLES);

				//open files
				for (size_t i = 0; i < filesList.size() && msg; i++)
				{
					for (size_t v = 0; v < NB_VARIABLES && msg; v++)
					{
						NcFile ncFile(filesList[i][v], NcFile::read);

						auto timeGrid = ncFile.getDim("time");
						size_t nbDays = timeGrid.getSize();
						//auto latGrid = ncFile->getDim("lat");
						//auto lonGrid = ncFile->getDim("lon");

						NcVar& var = ncFile.getVar(VARIABLES_NAMES[v]);
						size_t curPos = data[v].size();
						data[v].resize(data[v].size() + nbDays);

						vector<size_t> startp = { { 0, size_t(xy.m_y), size_t(xy.m_x) } };
						vector<size_t> countp = { { nbDays, 1, 1 } };

						//vector<size_t> startp(var.getDimCount());
						//startp[1] = xy.m_y;//lat
						//startp[2] = xy.m_x;//lon
						//vector<size_t> countp(var.getDimCount());
						//countp[0] = var.getDims().at(0).getSize();
						//countp[1] = 1;
						//countp[2] = 1;

						var.getVar(startp, countp, &(data[v][curPos]));

						msg += callback.StepIt();
					}
				}
			}
			catch (exceptions::NcException& e)
			{
				msg.ajoute(e.what());
				return msg;
			}

			//callback.PushTask(string("Save ") + filePath, filesList.size()*NB_VARIABLES);

			//convert data
			for (size_t v = 0; v < NB_VARIABLES && msg; v++)
			{
				for (size_t i = 0; i < data[v].size(); i++)
				{
					if (data[v][i] < 1.0E20)
					{
						using namespace units::values;
						switch (v)
						{
						case V_TMIN:	data[v][i] = (float)Celsius(K(data[v][i])).get(); break; //K --> °C
						case V_TMAX:	data[v][i] = (float)Celsius(K(data[v][i])).get(); break; //K --> °C
						case V_PRCP:	data[v][i] = (float)(data[v][i] * 60 * 60 * 24); break; //kg/(m²s) --> mm/day 
						case V_SPEH:	data[v][i] = (float)(data[v][i] * 1000);  break;		//kg[H2O]/kg[air] --> g[H2O]/kg[air]
						case V_WNDS:	data[v][i] = (float)kph(meters_per_second(data[v][i])).get(); break; //m/s --> km/h
						default: ASSERT(false);
						}
					}
					else
					{
						data[v][i] = -999;
					}
				}
			}


			//save file
			file << "Year,Month,Day,Tmin,Tmax,Tair,Prcp,Tdew,Hr,Hs,Pv,WndS" << endl;
			for (size_t d = 0; d < data[0].size(); d++)
			{
				//CTRef TRef = TRefBase + d;
				int year = 1950 + int(d / 365);
				CJDayRef JD(0, d % 365);
				file << year << "," << JD.GetMonth() + 1 << "," << JD.GetDay() + 1 << ",";
				double Tmin = data[V_TMIN][d];
				double Tmax = data[V_TMAX][d];
				double Tair = (Tmin + Tmax) / 2;
				double prcp = data[V_PRCP][d];
				double Hs = data[V_SPEH][d];
				double Pv = Hs2Pv(Hs);
				double Hr = Pv2Hr(Tair, Pv);
				double Td = Hr2Td(Tair, Hr);
				double ws = data[V_WNDS][d];

				file << Tmin << "," << Tmax << "," << Tair << "," << prcp << "," << Td << "," << Hr << "," << Hs << "," << Pv << "," << ws << endl;
			}

			file.close();
		}

		return msg;
	}
	
	void CHadGEM2_ES_MMGCreator::GetDEM(CMatrix<float>& DEM)
	{
		string filePath = GetDir(INPUT_PATH) + "orog_fx_HadGEM2 - ES_historical_r0i0p0.nc";
		NcFile ncFile(filePath, NcFile::read);

		CGeoExtents extents = GetExtents();

		vector<float> data(extents.m_xSize*extents.m_ySize);
		vector<size_t> startp(4);
		vector<size_t> countp(4);
		countp[0] = 1;
		countp[1] = 1;
		countp[2] = extents.m_ySize;
		countp[3] = extents.m_xSize;


		NcVar& var = ncFile.getVar("topo");
		var.getVar(startp, countp, &(data[0]));


		DEM.resize(extents.m_ySize, extents.m_xSize);

		for (size_t i = 0; i < DEM.size_y(); i++)
			for (size_t j = 0; j < DEM.size_x(); j++)
				DEM[i][j] = data[i*DEM.size_x() + j];
	}

	/*void CHadGEM2_ES_MMGCreator::GetDEM(CMatrix<float>& DEM)
	{
		string fileTitle("orog_" + m_regionName + "-22");
		string filePath = GetFilePath(fileTitle);
		NcFile ncFile(filePath, NcFile::read);

		vector<float> data(m_extents.m_xSize*m_extents.m_ySize);
		vector<size_t> startp(3);
		vector<size_t> countp(3);
		countp[0] = 1;
		countp[1] = m_extents.m_ySize;
		countp[2] = m_extents.m_xSize;


		NcVar& var = ncFile.getVar("orog");
		var.getVar(startp, countp, &(data[0]));


		DEM.resize(m_extents.m_ySize, m_extents.m_xSize);

		for (size_t i = 0; i<DEM.size_y(); i++)
			for (size_t j = 0; j<DEM.size_x(); j++)
				DEM[i][j] = data[i*DEM.size_x() + j];
	}
*/


	


}
