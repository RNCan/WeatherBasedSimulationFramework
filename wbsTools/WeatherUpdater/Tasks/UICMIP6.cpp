#include "StdAfx.h"

#include "UICMIP6.h"
//#include "CMIP6.h"
#include <boost\multi_array.hpp>
#include "Basic/units.hpp"
#include "Basic/Statistic.h"
#include "Simulation/MonthlyMeanGrid.h"
#include "UI/Common/SYShowMessage.h"
#include "Geomatic/SfcGribsDatabase.h"


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

//CPIM6 dowload data:
//https://esgf-node.llnl.gov/search/cmip6/

//Cordex data
//https://esgf-index1.ceda.ac.uk/search/cordex-ceda/

namespace WBSF
{


	//*********************************************************************
	const char* CUICMIP6::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "FirstYear", "LastYear", "Frequency", "Model", "SSP", "MinLandWater" };
	const size_t CUICMIP6::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_STRING, T_STRING, T_COMBO_STRING, T_COMBO_STRING, T_COMBO_STRING, T_STRING };
	const UINT CUICMIP6::ATTRIBUTE_TITLE_ID = IDS_UPDATER_CMIP5_P;
	const UINT CUICMIP6::DESCRIPTION_TITLE_ID = ID_TASK_CMIP5;



	const char* CUICMIP6::CLASS_NAME() { static const char* THE_CLASS_NAME = "CMIP6";  return THE_CLASS_NAME; }
	CTaskBase::TType CUICMIP6::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUICMIP6::CLASS_NAME(), (createF)CUICMIP6::create);
	enum TFileNameComponent { C_VAR, C_TIME, C_MODEL, C_SSP, C_REP, C_GN, C_PERIOD, C_VERSION, NB_COMPONENTS };

	CUICMIP6::CUICMIP6(void)
	{
	}

	CUICMIP6::~CUICMIP6(void)
	{
	}


	std::string CUICMIP6::Option(size_t i)const
	{
		string str;

		switch (i)
		{
		case WORKING_DIR:		str = GetString(IDS_STR_FILTER_NC); break;
		case FREQUENCY:			str = "Daily|Monthly"; break;
		case MODEL:				str = "ACCESS-CM2|ACCESS-ESM1-5|BCC-CSM2-MR|CanESM5|CMCC-ESM2|EC-Earth3|GFDL-ESM4|INM-CM5-0|MIROC6|MPI-ESM1-2-HR|MRI-ESM2-0|NorESM2-MM|TaiESM1|UKESM1-0-LL"; break;
		case SSP:				str = "ssp126|ssp245|ssp370|ssp585"; break;
		case MIN_LAND_WATER:    str = "50.0"; break;
		};

		return str;

	}



	std::string CUICMIP6::Default(size_t i)const
	{
		std::string str;

		switch (i)
		{

		case WORKING_DIR:		str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "CMIP6\\"; break;
		case FIRST_YEAR:		str = "1851"; break;
		case LAST_YEAR:			str = "2100"; break;
		case FREQUENCY:			str = "Daily"; break;
		case MODEL:				str = "MIROC-ES2L"; break;
		case SSP:				str = "ssp245"; break;
		case MIN_LAND_WATER:    str = "50.0"; break;

		};

		return str;
	}



	//*******************************************************************************************************


	//https://pcmdi.llnl.gov/search/cmip5/
	const char* CUICMIP6::VARIABLES_NAMES[NB_CMIP6_VARIABLES] = { "tasmin", "tasmax", "pr", "huss", "sfcWind" };


	string GetPeriod(const std::string& filePath)
	{
		string title = GetFileTitle(filePath);
		return title.substr(title.length() - 17);
	}


	ERMsg CUICMIP6::Execute(CCallback& callback)
	{
		ERMsg msg;


		bool bCreateGribs = false;
		if (bCreateGribs)
		{
			if (Get(FREQUENCY) == "Daily")
				msg = CreateDailyGribs(callback);
			else
			{

				CLocationVector loc;
				//msg = loc.Load("D:\\Travaux\\CMIP6\\Loc\\NormalsCanada-USA1981-2010subset100kmClean.csv");
				msg = loc.Load("D:\\Travaux\\CMIP6\\Loc\\Dayton.csv");

				ofStream out;
				//msg += out.open("D:\\Travaux\\CMIP6\\Output\\replications.csv");
				//out << "KeyID,ripf,Month,Variable,Value" << endl;
				msg += out.open("D:\\Travaux\\CMIP6\\Output\\replications_Dayton.csv");
				out << "KeyID,r,i,p,f,Year,Month,Tmin,Tmax,Prcp,SpeH,WindS" << endl;


				string model = "CanESM5";
				string ssp = "ssp245";
				string filepath_out = "G:\\MMG\\Replication.mmg";

				string ripf = FormatA("r%di%dp%df%d", 1, 1, 1, 1);
				CreateMonthlyGribs(filepath_out, ripf, callback);
				
				//static const char* REPLICATION[6] = { "r10i1p1f1", "r10i1p2f1", "r11i1p1f1", "r11i1p2f1", "r12i1p1f1", "r12i1p2f1" };
				for (size_t r = 1; r < 20 && msg; r++)
				{
					for (size_t p = 1; p < 3 && msg; p++)
					{
						string ripf = FormatA("r%di%dp%df%d", r, 1, p, 1);
						//msg += CreateMonthlyGribs(filepath_out, ripf, callback);


						if (msg)
						{
							//Create csv file
							string filePathOut = filepath_out;
							WBSF::SetFileTitle(filePathOut, WBSF::GetFileTitle(filePathOut) + "_" + model + "_" + ssp + "_" + ripf);


							CMonthlyMeanGrid MMG;
							if (MMG.Open(filePathOut, callback))
							{
								for (size_t l = 0; l < loc.size() && msg; l++)
								{
									//double monthlyMean[12][NORMALS_DATA::NB_FIELDS];
									//if (MMG.GetNormals(1981, 30, 4, 500000, 2, loc[l], monthlyMean, callback))
									//for (size_t y = 0; y < 30 && msg; y++)
									//{
										//int year = int(1981 + y);
										//for (size_t m = 0; m < 12 && msg; m++)
										//{
											/*for (size_t f = 0; f < NB_FIELDS && msg; f++)
											{
												if (f == TMIN_MN || f == TMAX_MN || f == PRCP_TT || f == SPEH_MN || f == WNDS_MN)
												{
													string var_name = GetFieldHeader(f);
													if (f == SPEH_MN)
														var_name = "SPEH_MN";
													string tmp = FormatA("%s,%s,%2d,%s,%.3lg", loc[l].m_ID.c_str(), ripf.c_str(), m + 1, var_name.c_str(), monthlyMean[m][f]);
													out << tmp << endl;
												}
											}*/
											//}
									std::vector< std::array<std::array<float, NORMALS_DATA::NB_FIELDS>, 12>> values;
									if (MMG.GetMonthlyValues(1981, 30, 4, 500000, 2, loc[l], values, callback))
									{
										for (size_t y = 0; y < 30 && msg; y++)
										{

											int year = int(1981 + y);
											for (size_t m = 0; m < 12 && msg; m++)
											{
												string tmp = FormatA("%s,%d,%d,%d,%d,%4d,%02d,%.2f,%.2f,%.2f,%.2f,%.2f", loc[l].m_ID.c_str(), r, 1, p, 1, year, m + 1, values[y][m][TMIN_MN], values[y][m][TMAX_MN], values[y][m][PRCP_TT], values[y][m][SPEH_MN], values[y][m][WNDS_MN]);
												out << tmp << endl;
												msg += callback.StepIt(0);
											}//for all month
										}//for all year
									}//if msg

								}//for all loc
							}
						}
					}
				}

				out.close();
			}

		}


		msg.ajoute("Download of CMPI6 data is not implemented yet");


		return msg;
	}

	ERMsg CUICMIP6::CreateMMG(string filePathOut, CCallback& callback)
	{
		ERMsg msg;

		CPLSetConfigOption("GDAL_CACHEMAX", "1000");

		string model = Get(MODEL);
		string ssp = Get(SSP);
		string working_dir = GetDir(WORKING_DIR) + model + "\\";
		float minLandWater = as<float>(MIN_LAND_WATER);
		int first_year = as<int>(FIRST_YEAR);
		int last_year = as<int>(LAST_YEAR);
		CTPeriod valid_period(CTRef(first_year, JANUARY, DAY_01), CTRef(last_year, DECEMBER, DAY_31));


		//int firstYear = FIRST_YEAR;
		//int lastYear = LAST_YEAR;
		int nbYears = last_year - first_year + 1;

		string sftlf_filepath = working_dir + "sftlf_fx_" + model + ".nc";
		string new_sftlf_filepath = working_dir + "sftlf_fx_" + model + ".tif";

		if (!FileExists(new_sftlf_filepath))
			msg += save_sftlf(sftlf_filepath, new_sftlf_filepath);


		//Load land/water profile
		vector<float> landWaterMask;
		msg = get_sftlf(new_sftlf_filepath, landWaterMask);

		string orog_filepath = working_dir + "orog_fx_" + model + ".nc";
		string new_orog_filepath = working_dir + "orog_fx_" + model + ".tif";

		if (!FileExists(new_orog_filepath))
			msg += save_orog(orog_filepath, new_orog_filepath, landWaterMask, minLandWater);


		CBaseOptions options;
		msg = GetMapOptions(new_orog_filepath, options);
		options.m_nbBands = 12 * nbYears;



		CMonthlyMeanGrid MMG;
		MMG.m_firstYear = first_year;
		MMG.m_lastYear = last_year;

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


		WBSF::SetFileTitle(filePathOut, WBSF::GetFileTitle(filePathOut) + "_" + model + "_" + ssp);
		msg = MMG.Save(filePathOut);
		if (!msg)
			return msg;

		callback.PushTask(string("Open output images for ") + ssp, NB_FIELDS);

		size_t nb_grib_open = 0;
		array< CGDALDatasetEx, NB_FIELDS> grid;
		for (size_t v = 0; v < grid.size(); v++)
		{
			string filePathOut = MMG.GetFilePath(v);
			if (!filePathOut.empty())
			{
				msg += grid[v].CreateImage(filePathOut, options);
				nb_grib_open++;
			}

			msg += callback.StepIt();
		}

		callback.PopTask();

		if (!msg)
			return msg;


		CMonthlyVariableVector data;
		msg += GetMMGForSSP(model, ssp, valid_period, options.m_extents, landWaterMask, minLandWater, data, callback);

		if (msg)
		{
			callback.PushTask(string("Save output images for ") + ssp, nb_grib_open);
			for (size_t f = 0; f < grid.size() && msg; f++)
			{

				if (grid[f].IsOpen())
				{
					callback.PushTask(GetFieldTitle(f), data.size());
					ASSERT(data.size() == grid[f].GetRasterCount());
					for (size_t b = 0; b < data.size() && msg; b++)
					{
						GDALRasterBand* pBand = grid[f].GetRasterBand(b);
						pBand->RasterIO(GF_Write, 0, 0, grid[f].GetRasterXSize(), grid[f].GetRasterYSize(), &(data[b][f][0]), grid[f].GetRasterXSize(), grid[f].GetRasterYSize(), GDT_Float32, 0, 0);
						msg += callback.StepIt();
					}

					grid[f].Close();
					callback.PopTask();

					msg += callback.StepIt();
				}


			}
			callback.PopTask();
		}



		return msg;

	}


	string CUICMIP6::GetProjectionWKT() { return PRJ_WGS_84_WKT; }

	ERMsg CUICMIP6::GetMapOptions(string orog_filepath, CBaseOptions& options)const
	{
		ERMsg msg;


		CGDALDatasetEx DS;
		msg = DS.OpenInputImage(orog_filepath);
		if (msg)
		{
			DS.UpdateOption(options);
			options.m_dstNodata = -999;
			options.m_outputType = GDT_Float32;
			options.m_format = "GTIFF";
			options.m_bOverwrite = true;
			options.m_bComputeStats = true;
		}

		return msg;
	}

	void CUICMIP6::ConvertData(size_t v, std::vector<float>& data)const
	{
		using namespace units::values;


		std::vector<float> dataII(data.size());

		for (size_t i = 0; i < data.size(); i++)
		{
			size_t ii = i;

			if (data[i] < 1.0E20)
			{
				switch (v)
				{
				case V_TMIN:	dataII[ii] = (float)Celsius(K(data[i])).get(); break; //K --> °C
				case V_TMAX:	dataII[ii] = (float)Celsius(K(data[i])).get(); break; //K --> °C
				case V_PRCP:	dataII[ii] = (float)(data[i] * 60 * 60 * 24); break; //kg/(m²s) --> mm/day 
				case V_SPEH:	dataII[ii] = (float)(data[i] * 1000); break; //kg[H2O]/kg[air] --> g[H2O]/kg[air]
				case V_WNDS:	dataII[ii] = (float)data[i] * 3600 / 1000; break;//(float)kph(meters_per_second(data[i])).get(); break; //m/s --> km/h
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

	CTPeriod CUICMIP6::get_period(const string& period)
	{

		ASSERT(period.size() == 13 || period.size() == 17);
		StringVector tmp(period, "-");
		ASSERT(tmp.size() == 2);

		CTRef p1;
		CTRef p2;
		if (period.size() == 17)//Daily
		{
			p1.FromFormatedString(tmp[0], "%Y%m%d", "");
			p2.FromFormatedString(tmp[1], "%Y%m%d", "");
		}
		else if (period.size() == 13)//Montlhy
		{
			p1.FromFormatedString(tmp[0], "%Y%m", "");
			p2.FromFormatedString(tmp[1], "%Y%m", "");
		}

		return CTPeriod(p1, p2);
	}

	ERMsg CUICMIP6::GetFileList(std::string model, std::string ssp, string ripf, std::string frequency, const CTPeriod& valid_period, CMIP6FileList& fileList)const
	{
		ERMsg msg;

		set<string> replications;

		string path = GetDir(WORKING_DIR) + model + "\\NetCDF" + frequency + "\\*.nc";
		StringVector list = WBSF::GetFilesList(path);
		for (size_t i = 0; i < list.size(); i++)
		{
			StringVector tmp(GetFileTitle(list[i]), "_");

			if (tmp.size() == NB_COMPONENTS || tmp.size() == NB_COMPONENTS - 1)//some file have only 7 component
			{

				string i_model = tmp[C_MODEL];
				string i_ssp = tmp[C_SSP];
				string i_time = tmp[C_TIME];
				string i_var = tmp[C_VAR];
				string i_period = tmp[C_PERIOD];
				string i_rep = tmp[C_REP];
				//string i_version = tmp[C_VERSION];

				auto test = find(begin(VARIABLES_NAMES), end(VARIABLES_NAMES), i_var);
				size_t var = distance(begin(VARIABLES_NAMES), test);

				CTPeriod p = get_period(i_period);

				if (i_model == model && (i_ssp == ssp || i_ssp == "historical") &&
					(i_time == "day" || i_time == "Amon") && var < NB_CMIP6_VARIABLES &&
					(ripf.empty() || ripf == i_rep) && valid_period.IsIntersect(p))
				{
					replications.insert(i_rep);
					//versions.insert(i_version);

					fileList[i_period].push_back(list[i]);
				}
			}
		}


		if (replications.size() > 1)
			msg.ajoute("Multiple replication found. ");

		//All period must have the same number of variables
		for (auto it = fileList.begin(); it != fileList.end() && msg; it++)
		{
			string i_period = it->first;

			const std::vector<std::string>& varList = it->second;
			if (varList.size() != NB_CMIP6_VARIABLES)
			{
				msg.ajoute("Some file is missing for period: " + i_period);
			}
		}



		return msg;

	}


	void CUICMIP6::ComputeMontlyStatistic(size_t i, size_t ii, const COneMonthData& data, CMonthlyVariables& montlhyStat)
	{
		array < CStatistic, NB_CMIP6_VARIABLES> stat;

		for (size_t d = 0; d < data.size(); d++)
		{
			for (size_t v = 0; v < data[d].size(); v++)
			{
				ASSERT(data[d][v][i] > -999);
				stat[v] += data[d][v][i];
			}
		}

		montlhyStat[TMIN_MN][ii] = stat[V_TMIN][MEAN];
		montlhyStat[TMAX_MN][ii] = stat[V_TMAX][MEAN];


		CStatistic statTmin;
		CStatistic statTmax;
		CStatistic statTmin_max;
		CStatistic statHr;

		for (size_t d = 0; d < data.size(); d++)
		{
			double deltaTmin = data[d][V_TMIN][i] - montlhyStat[TMIN_MN][ii];
			double deltaTmax = data[d][V_TMAX][i] - montlhyStat[TMAX_MN][ii];
			statTmin += deltaTmin;
			statTmax += deltaTmax;
			statTmin_max += deltaTmin * deltaTmax;

			double Hs = data[d][V_SPEH][i];
			double Hr = Hs2Hr(data[d][V_TMIN][i], data[d][V_TMAX][i], Hs);
			ASSERT(Hr >= 0 && Hr <= 100);

			statHr += Hr;
		}


		montlhyStat[TMNMX_R][ii] = statTmin_max[SUM] / sqrt(statTmin[SUM²] * statTmax[SUM²]);
		montlhyStat[DEL_STD][ii] = statTmin[STD_DEV];
		montlhyStat[EPS_STD][ii] = statTmax[STD_DEV];

		montlhyStat[PRCP_TT][ii] = stat[V_PRCP][SUM];
		//standard deviation of prcp is not computed in MMG

		montlhyStat[SPEH_MN][ii] = float(stat[V_SPEH][MEAN]);
		montlhyStat[RELH_MN][ii] = float(statHr[MEAN]);
		montlhyStat[RELH_SD][ii] = float(statHr[STD_DEV]);

		montlhyStat[WNDS_MN][ii] = float(stat[V_WNDS][MEAN]);
		montlhyStat[WNDS_SD][ii] = float(stat[V_WNDS][STD_DEV]);
	}

	size_t CUICMIP6::GetVar(string name)
	{
		StringVector tmp(GetFileTitle(name), "_");
		ASSERT(tmp.size() == NB_COMPONENTS || tmp.size() == NB_COMPONENTS - 1);
		string i_var = tmp[C_VAR];
		auto test = find(begin(VARIABLES_NAMES), end(VARIABLES_NAMES), i_var);

		return distance(begin(VARIABLES_NAMES), test);
	}

	//

	ERMsg CUICMIP6::GetMMGForSSP(std::string model, std::string ssp, const CTPeriod& valid_period, const CGeoExtents& extents, const vector<float>& landWaterMask, float minLandWater, CMonthlyVariableVector& dataOut, CCallback& callback)
	{
		ERMsg msg;


		CMIP6FileList fileList;
		msg = GetFileList(model, ssp, "", "", valid_period, fileList);
		if (!msg)
			return msg;

		callback.PushTask(string("Process all files (") + to_string(fileList.size()) + ") for model " + model + " " + ssp + " and period " + valid_period.GetFormatedString("%1 to %2"), fileList.size());

		dataOut.resize(valid_period.as(CTM::MONTHLY).size());
		for (size_t mm = 0; mm < dataOut.size(); mm++)
			for (size_t v = 0; v < NB_FIELDS; v++)
				dataOut[mm][v].insert(dataOut[mm][v].begin(), extents.m_ySize * extents.m_xSize, -999);


		COneMonthData daily_data;
		CTRef next_TRef;
		bool bWarningFixed30DaysData = false;
		bool bWarningMissingFeb29 = false;



		//open files
		for (auto it = fileList.begin(); it != fileList.end() && msg; it++)
		{
			string i_period = it->first;
			CTPeriod period = get_period(i_period);
			CTPeriod intersect = valid_period.Intersect(period);

			double x_min = -999;

			size_t nbLat = 0;
			size_t nbLon = 0;
			size_t nbdays = 0;

			bool bIsMissingFeb29 = false;
			size_t leap_correction = 0;// NOT_INIT;
			bool bIsFixed30DaysData = false;


			NcFilePtrArray ncFiles;

			const std::vector<std::string>& varList = it->second;
			ASSERT(varList.size() == NB_CMIP6_VARIABLES);
			for (size_t i = 0; i < varList.size() && msg; i++)
			{
				try
				{
					size_t v = GetVar(varList[i]);
					ncFiles[v] = NcFilePtr(new NcFile(varList[i], NcFile::read));

					if (i == 0)
					{
						auto timeGrid = ncFiles[v]->getDim("time");
						auto latGrid = ncFiles[v]->getDim("lat");
						auto lonGrid = ncFiles[v]->getDim("lon");

						nbdays = timeGrid.getSize();
						nbLat = latGrid.getSize();
						nbLon = lonGrid.getSize();


						if (nbdays != period.size())
						{
							size_t nbLeapdays = 0;
							for (CTRef TRef = period.Begin(); TRef <= period.End(); TRef++)
								if (TRef.GetMonth() == FEBRUARY && TRef.GetDay() == DAY_29)
									nbLeapdays++;

							if (nbdays + nbLeapdays == period.size())
							{
								bIsMissingFeb29 = true;

								//compute all leap year since the beginning of th period)
								for (CTRef TRef = period.Begin(); TRef < intersect.Begin(); TRef++)
									if (TRef.GetMonth() == FEBRUARY && TRef.GetDay() == DAY_29)
										leap_correction++;



								if (!bWarningMissingFeb29)
								{
									bWarningMissingFeb29 = true;
									callback.AddMessage("WARNING: input files have missing February 29");
								}
							}
							else
							{
								if (nbdays == period.as(CTM::MONTHLY).size() * 30)
								{
									bIsFixed30DaysData = true;

									if (!bWarningFixed30DaysData)
									{
										bWarningFixed30DaysData = true;
										callback.AddMessage("WARNING: input files have fixed 30 days by months");
									}
								}
								else
								{
									msg.ajoute("Incompatible netCDF time size (" + to_string(nbdays) + "for period size " + to_string(period.size()));
								}
							}
						}
					}
					msg += callback.StepIt(0);
				}
				catch (exceptions::NcException& e)
				{
					msg.ajoute(e.what());
					msg.ajoute(string("Unable to open file : ") + varList[i]);
				}
			}

			if (msg)
			{


				auto timeGrid = ncFiles.front()->getDim("time");
				size_t nbDays = timeGrid.getSize();
				size_t nbMonths = intersect.as(CTM::MONTHLY).size();
				//ASSERT(nbDays == period.size());

				//CTPeriod intersect = valid_period.Intersect(period);
				callback.PushTask(string("Create data for period ") + intersect.GetFormatedString("%1 to %2"), intersect.size());

				for (size_t mm = 0; mm < nbMonths && msg; mm++)
				{

					CTRef montly_Tref = intersect.Begin().as(CTM::MONTHLY) + mm;
					//if (valid_period.as(CTM::MONTHLY).IsInside(montly_Tref))
					{
						CTPeriod daily_period = CTPeriod(montly_Tref.as(CTM::DAILY, CTRef::FIRST_TREF), montly_Tref.as(CTM::DAILY, CTRef::LAST_TREF));

						if (!next_TRef.IsInit())
						{
							daily_data.resize(daily_period.size());
							for (size_t d = 0; d < daily_period.size(); d++)
								for (size_t v = 0; v < daily_data[d].size(); v++)
									daily_data[d][v].resize(extents.m_ySize * extents.m_xSize);
						}


						double x_min = -999;
						for (CTRef TRef = next_TRef.IsInit() ? next_TRef : daily_period.Begin(); TRef <= daily_period.End() && TRef <= period.End() && msg; TRef++)
						{
							
							if (bIsMissingFeb29)
							{
								if (TRef.GetMonth() == FEBRUARY && TRef.GetDay() == DAY_29)
									leap_correction++;
							}

							//if (intersect.IsInside(TRef))
							//{
								size_t d = (size_t)(TRef - daily_period.Begin());
								size_t dd = TRef - period.Begin();
								if (bIsMissingFeb29)
								{
									ASSERT(dd > 0 || leap_correction == 0);
									dd -= leap_correction;
								}
								else if (bIsFixed30DaysData)
								{
									size_t mm = TRef.as(CTM::MONTHLY) - period.as(CTM::MONTHLY).Begin();
									dd = mm * 30 + min(size_t(DAY_30), TRef.GetDay());
								}
								//size_t dd = TRef - period.Begin();

								ASSERT(d < daily_data.size());
								ASSERT(daily_data[d].size() == ncFiles.size());


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
										//ASSERT(dd < timeGrid.getSize());

										if (x_min == -999)
										{
											NcVar& var_lon = ncFiles[v]->getVar("lon");
											vector < double> x(nbLon);
											var_lon.getVar(&(x[0]));
											x_min = x.front();
										}



										vector<size_t> startp = { {dd, 0, 0 } };
										vector<size_t> countp = { { 1, nbLat, nbLon } };

										NcVar& var = ncFiles[v]->getVar(VARIABLES_NAMES[v]);
										var.getVar(startp, countp, &(daily_data[d][v][0]));
									}
									catch (exceptions::NcException& e)
									{
										msg.ajoute(e.what());
										//msg.ajoute(string("period: ") + i_period + ", variable: " + VARIABLES_NAMES[v]);
										msg.ajoute(string("processing variable : ") + VARIABLES_NAMES[v] + " for date " + TRef.GetFormatedString());
									}

									ConvertData(v, daily_data[d][v]);
								}
							//}

							msg += callback.StepIt();

						}//for all days of the month 

						//if we have to continue on the next file
						if (daily_period.End() > period.End())
							next_TRef = daily_period.End() + 1;
						else
							next_TRef.clear();

						bool bRevertImage = x_min > -10;
						if (msg && !next_TRef.IsInit())
						{
#pragma omp parallel for
							for (__int64 i = 0; i < (__int64)extents.m_ySize * extents.m_xSize; i++)
							{
								size_t mmm = montly_Tref - valid_period.Begin().as(CTM::MONTHLY);

								size_t ii = i;
								if (bRevertImage)
								{
									size_t x = (i + extents.m_xSize / 2) % extents.m_xSize;
									size_t y = extents.m_ySize - size_t(i / extents.m_xSize) - 1;
									ii = y * extents.m_xSize + x;
								}
								else
								{
									size_t x = i % extents.m_xSize;
									size_t y = extents.m_ySize - size_t(i / extents.m_xSize) - 1;
									ii = y * extents.m_xSize + x;
								}

								if (landWaterMask[ii] >= minLandWater)
								{
									ComputeMontlyStatistic((size_t)i, ii, daily_data, dataOut[mmm]);
								}
							}
						}
					}//does this month is inside valid period
				}//for all months

				callback.PopTask();
			}//if msg


			msg += callback.StepIt();
		}//for all period


		callback.PopTask();

		return msg;
	}


	//ERMsg CUICMIP6::GetLandWaterProfile(std::string sftlf_filepath, CLandWaterMask& landWaterMask)
	//{

	//	ERMsg msg;

	//	bool bRevertImage = false;
	//	
	//	try
	//	{
	//		//open files
	//		NcFile ncFile(sftlf_filepath, NcFile::read);

	//		auto latGrid = ncFile.getDim("lat");
	//		auto lonGrid = ncFile.getDim("lon");
	//		size_t nbLat = latGrid.getSize();
	//		size_t nbLon = lonGrid.getSize();


	//		//CGeoExtents extents = GetExtents();
	//		vector<float> data(nbLat * nbLon);
	//		vector<size_t> startp = { { 0, 0 } };
	//		vector<size_t> countp = { { nbLat, nbLon } };

	//		NcVar& var = ncFile.getVar("sftlf");
	//		var.getVar(startp, countp, &(data[0]));

	//		landWaterMask.resize(data.size());

	//		for (size_t i = 0; i < data.size(); i++)
	//		{
	//			size_t ii = i;
	//			if (bRevertImage)
	//			{
	//				size_t x = (i + nbLon / 2) % nbLon;
	//				size_t y = nbLat - size_t(i / nbLon) - 1;
	//				ii = y * nbLon + x;
	//			}
	//			else
	//			{
	//				size_t x = i % nbLon;
	//				size_t y = nbLat - size_t(i / nbLon) - 1;
	//				ii = y * nbLon + x;
	//			}

	//			landWaterMask[ii] = data[i];
	//			ASSERT(landWaterMask[ii] >= 0 && landWaterMask[ii] <= 100);
	//		}

	//	}
	//	catch (exceptions::NcException& e)
	//	{
	//		msg.ajoute(e.what());
	//	}



	//	return msg;
	//}


	ERMsg CUICMIP6::save_sftlf(std::string sftlf_filepath, std::string new_sftlf_filepath)
	{
		ERMsg msg;
		//CGeoExtents extents = GetExtents();


		string tif_filepath = sftlf_filepath + ".tif";
		//convert nc into GeoTIFF
		string argument = "-a_srs \"+proj=longlat +datum=WGS84 +no_defs\" -ot Float32 -stats -co COMPRESS=LZW -co PREDICTOR=3 -co TILED=YES -co BLOCKXSIZE=64 -co BLOCKYSIZE=64 \"" + sftlf_filepath + "\" \"" + tif_filepath + "\"";
		string command = "\"" + GetApplicationPath() + "External\\gdal_translate.exe\" " + argument;
		msg += WinExecWait(command);


		if (msg)
		{
			CGDALDatasetEx DS;
			msg = DS.OpenInputImage(tif_filepath);
			if (msg)
			{
				GDALRasterBand* pBand = DS.GetRasterBand(0);

				vector<float> data(DS.GetRasterXSize() * DS.GetRasterYSize());
				pBand->RasterIO(GF_Read, 0, 0, DS.GetRasterXSize(), DS.GetRasterYSize(), &(data[0]), DS.GetRasterXSize(), DS.GetRasterYSize(), GDT_Float32, 0, 0);

				CBaseOptions options;
				DS.UpdateOption(options);
				int nbYears = LAST_YEAR - FIRST_YEAR + 1;

				bool bRevertImage = options.m_extents.m_xMin > -90;
				double shift = bRevertImage ? 180 /*- options.m_extents.XRes()*/ : 0;

				options.m_extents.m_xMin -= shift;
				options.m_extents.m_xMax -= shift;
				options.m_nbBands = 1;
				options.m_dstNodata = -999;
				options.m_outputType = GDT_Float32;
				options.m_format = "GTIFF";
				options.m_bOverwrite = true;
				options.m_bComputeStats = true;

				vector<float> tmp(DS.GetRasterXSize() * DS.GetRasterYSize());

				for (size_t i = 0; i < data.size(); i++)
				{
					size_t ii = i;
					if (bRevertImage)
					{
						size_t x = (i + DS.GetRasterXSize() / 2) % DS.GetRasterXSize();
						size_t y = size_t(i / DS.GetRasterXSize());
						ii = y * DS.GetRasterXSize() + x;
					}
					else
					{
						size_t x = i % DS.GetRasterXSize();
						size_t y = size_t(i / DS.GetRasterXSize());
						ii = y * DS.GetRasterXSize() + x;
					}

					tmp[ii] = data[i];
				}

				data = tmp;

				CGDALDatasetEx DS_out;
				DS_out.CreateImage(new_sftlf_filepath, options);

				GDALRasterBand* pBand_out = DS_out.GetRasterBand(0);
				pBand_out->RasterIO(GF_Write, 0, 0, options.m_extents.m_xSize, options.m_extents.m_ySize, &(data[0]), options.m_extents.m_xSize, options.m_extents.m_ySize, GDT_Float32, 0, 0);

				DS_out.Close();
			}//if msg
		}//if msg

		return msg;
	}



	ERMsg CUICMIP6::load_geotif(std::string filepath, vector<float>& data)
	{
		ERMsg msg;

		CGDALDatasetEx DS;
		msg = DS.OpenInputImage(filepath);
		if (msg)
		{
			GDALRasterBand* pBand = DS.GetRasterBand(0);
			data.resize(DS.GetRasterXSize() * DS.GetRasterYSize());
			pBand->RasterIO(GF_Read, 0, 0, DS.GetRasterXSize(), DS.GetRasterYSize(), &(data[0]), DS.GetRasterXSize(), DS.GetRasterYSize(), GDT_Float32, 0, 0);
			DS.Close();
		}

		return msg;
	}

	ERMsg CUICMIP6::save_orog(std::string orog_filepath, std::string new_orog_filepath, const vector<float>& landWaterMask, float minLandWater)
	{
		ERMsg msg;
		//CGeoExtents extents = GetExtents();


		string tif_filepath = orog_filepath + ".tif";
		//convert nc into GeoTIFF
		string argument = "-a_srs \"+proj=longlat +datum=WGS84 +no_defs\" -ot Float32 -stats -co COMPRESS=LZW -co PREDICTOR=3 -co TILED=YES -co BLOCKXSIZE=64 -co BLOCKYSIZE=64 \"" + orog_filepath + "\" \"" + tif_filepath + "\"";
		string command = "\"" + GetApplicationPath() + "External\\gdal_translate.exe\" " + argument;
		msg += WinExecWait(command);

		if (msg)
		{
			CGDALDatasetEx DS;
			msg = DS.OpenInputImage(tif_filepath);
			if (msg)
			{
				GDALRasterBand* pBand = DS.GetRasterBand(0);

				vector<float> data(DS.GetRasterXSize() * DS.GetRasterYSize());
				pBand->RasterIO(GF_Read, 0, 0, DS.GetRasterXSize(), DS.GetRasterYSize(), &(data[0]), DS.GetRasterXSize(), DS.GetRasterYSize(), GDT_Float32, 0, 0);

				CBaseOptions options;
				DS.UpdateOption(options);
				int nbYears = LAST_YEAR - FIRST_YEAR + 1;

				bool bRevertImage = options.m_extents.m_xMin > -90;
				double shift = bRevertImage ? 180 /*- options.m_extents.XRes()*/ : 0;

				options.m_extents.m_xMin -= shift;
				options.m_extents.m_xMax -= shift;
				options.m_nbBands = 1;
				options.m_dstNodata = -999;
				options.m_outputType = GDT_Float32;
				options.m_format = "GTIFF";
				options.m_bOverwrite = true;
				options.m_bComputeStats = true;

				vector<float> DEM(DS.GetRasterXSize() * DS.GetRasterYSize());

				for (size_t i = 0; i < DEM.size(); i++)
				{
					size_t ii = i;
					if (bRevertImage)
					{
						size_t x = (i + DS.GetRasterXSize() / 2) % DS.GetRasterXSize();
						size_t y = size_t(i / DS.GetRasterXSize());
						ii = y * DS.GetRasterXSize() + x;
					}

					if (landWaterMask[ii] > minLandWater)
						DEM[ii] = data[i];
					else
						DEM[ii] = -999;
				}



				CGDALDatasetEx DS_out;
				DS_out.CreateImage(new_orog_filepath, options);

				GDALRasterBand* pBand_out = DS_out.GetRasterBand(0);
				pBand_out->RasterIO(GF_Write, 0, 0, options.m_extents.m_xSize, options.m_extents.m_ySize, &(DEM[0]), options.m_extents.m_xSize, options.m_extents.m_ySize, GDT_Float32, 0, 0);

				DS_out.Close();
			}//if msg
		}//if msg


		return msg;
	}


	ERMsg CUICMIP6::get_orog(std::string orog_filepath, vector<float>& data, float new_no_data)
	{
		ERMsg msg;

		msg = load_geotif(orog_filepath, data);

		if (msg)
		{
			float no_data = -999;
			if (new_no_data != no_data)
			{
				for (size_t i = 0; i < data.size(); i++)
					if (fabs(data[i] - no_data) < 0.001)
						data[i] = new_no_data;
			}
		}

		return msg;
	}



	ERMsg CUICMIP6::CreateDailyGribs(CCallback& callback)
	{
		ERMsg msg;

		string model = Get(MODEL);
		string ssp = Get(SSP);
		string working_dir = GetDir(WORKING_DIR) + model + "\\";
		float minLandWater = as<float>(MIN_LAND_WATER);
		int first_year = as<int>(FIRST_YEAR);
		int last_year = as<int>(LAST_YEAR);
		CTPeriod valid_period(CTRef(first_year, JANUARY, DAY_01), CTRef(last_year, DECEMBER, DAY_31));


		float no_data_out = 9999;
		CStatistic::SetVMiss(no_data_out);




		//Create land/water profile GeoTIFF
		string sftlf_filepath = working_dir + "sftlf_fx_" + model + ".nc";
		string new_sftlf_filepath = working_dir + "sftlf_fx_" + model + ".tif";

		if (!FileExists(new_sftlf_filepath))
			msg += save_sftlf(sftlf_filepath, new_sftlf_filepath);


		//Load land/water profile
		vector<float> landWaterMask;
		if (msg)
			msg += get_sftlf(new_sftlf_filepath, landWaterMask);



		//Create elevation
		string orog_filepath = working_dir + "orog_fx_" + model + ".nc";
		string new_orog_filepath = working_dir + "orog_fx_" + model + ".tif";
		SetFileExtension(new_orog_filepath, ".tif");

		if (!FileExists(new_orog_filepath))
			msg += save_orog(orog_filepath, new_orog_filepath, landWaterMask, minLandWater);


		//Load elevation
		vector<float> orog;
		if (msg)
			msg += get_orog(new_orog_filepath, orog, no_data_out);


		bool bWarningFixed30DaysData = false;
		bool bWarningMissingFeb29 = false;

		CMIP6FileList fileList;
		if (msg)
			msg += GetFileList(model, ssp, "", "", valid_period, fileList);

		if (!msg)
			return msg;



		CBaseOptions options;
		GetMapOptions(new_orog_filepath, options);
		CGeoExtents extents = options.m_extents;


		callback.PushTask(string("Process all files (") + to_string(fileList.size()) + ") for model " + model + " " + ssp + " and period " + valid_period.GetFormatedString("%1 to %2"), fileList.size());

		//open files
		for (auto it = fileList.begin(); it != fileList.end() && msg; it++)
		{
			string i_period = it->first;
			CTPeriod period = get_period(i_period);
			CTPeriod intersect = valid_period.Intersect(period);



			double x_min = -999;

			size_t nbLat = 0;
			size_t nbLon = 0;
			size_t nbdays = 0;

			bool bIsMissingFeb29 = false;
			size_t leap_correction = 0;// NOT_INIT;
			bool bIsFixed30DaysData = false;


			NcFilePtrArray ncFiles;
			array<NcVar, NB_CMIP6_VARIABLES> var;

			const std::vector<std::string>& varList = it->second;

			string i_ssp;
			if (!varList.empty())
			{
				StringVector tmp(GetFileTitle(varList[0]), "_");
				ASSERT(tmp.size() == NB_COMPONENTS || tmp.size() == NB_COMPONENTS - 1);
				i_ssp = tmp[C_SSP];
			}




			ASSERT(varList.size() == NB_CMIP6_VARIABLES);
			for (size_t i = 0; i < varList.size() && msg; i++)
			{

				try
				{
					size_t v = GetVar(varList[i]);
					ncFiles[v] = NcFilePtr(new NcFile(varList[i], NcFile::read));
					var[v] = ncFiles[v]->getVar(VARIABLES_NAMES[v]);
					msg += callback.StepIt(0);

					if (i == 0)
					{
						auto timeGrid = ncFiles[v]->getDim("time");
						auto latGrid = ncFiles[v]->getDim("lat");
						auto lonGrid = ncFiles[v]->getDim("lon");

						nbdays = timeGrid.getSize();
						nbLat = latGrid.getSize();
						nbLon = lonGrid.getSize();


						if (nbdays != period.size())
						{
							size_t nbLeapdays = 0;
							for (CTRef TRef = period.Begin(); TRef <= period.End(); TRef++)
								if (TRef.GetMonth() == FEBRUARY && TRef.GetDay() == DAY_29)
									nbLeapdays++;

							if (nbdays + nbLeapdays == period.size())
							{
								bIsMissingFeb29 = true;

								//compute all leap year since the beginning of th period)
								for (CTRef TRef = period.Begin(); TRef < intersect.Begin(); TRef++)
									if (TRef.GetMonth() == FEBRUARY && TRef.GetDay() == DAY_29)
										leap_correction++;



								if (!bWarningMissingFeb29)
								{
									bWarningMissingFeb29 = true;
									callback.AddMessage("WARNING: input files have missing February 29");
								}
							}
							else
							{
								if (nbdays == period.as(CTM::MONTHLY).size() * 30)
								{
									bIsFixed30DaysData = true;

									if (!bWarningFixed30DaysData)
									{
										bWarningFixed30DaysData = true;
										callback.AddMessage("WARNING: input files have fixed 30 days by months");
									}
								}
								else
								{
									msg.ajoute("Incompatible netCDF time size (" + to_string(nbdays) + "for period size " + to_string(period.size()));
								}
							}
						}

						NcVar& var_lon = ncFiles[v]->getVar("lon");
						vector < double> x(nbLon);
						var_lon.getVar(&(x[0]));
						x_min = x.front();

					}
				}
				catch (exceptions::NcException& e)
				{
					msg.ajoute(e.what());
					msg.ajoute(string("Unable to open file : ") + varList[i]);
				}
			}

			if (msg)
			{
				COneVariableLayer data(nbLat * nbLon);
				callback.PushTask(string("Create data for period ") + intersect.GetFormatedString("%1 to %2"), intersect.size());

				for (CTRef TRef = intersect.Begin(); TRef <= intersect.End() && msg; TRef++)
				{
					if (bIsMissingFeb29)
					{
						if (TRef.GetMonth() == FEBRUARY && TRef.GetDay() == DAY_29)
							leap_correction++;
					}

					string filepath_out = FormatA("%sGribs\\%d\\%02d\\%s_%s_%d%02d%02d.tif", working_dir.c_str(), TRef.GetYear(), TRef.GetMonth() + 1, model.c_str(), i_ssp.c_str(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1);

					ASSERT(valid_period.IsInside(TRef));
					if (!FileExists(filepath_out))
					{
						CreateMultipleDir(GetPath(filepath_out));


						CBaseOptions options_out = options;
						options.m_nbBands = 4;
						options.m_outputType = GDT_Float32;
						options.m_dstNodata = no_data_out;
						options.m_bOverwrite = true;
						options.m_bComputeStats = true;
						options.m_createOptions.push_back("COMPRESS=LZW");
						options.m_createOptions.push_back("PREDICTOR=3");
						options.m_createOptions.push_back("TILED=YES");
						options.m_createOptions.push_back("BLOCKXSIZE=64");
						options.m_createOptions.push_back("BLOCKYSIZE=64");

						CGDALDatasetEx DSout;
						msg += DSout.CreateImage(filepath_out, options);
						if (msg)
						{
							//set elevation
							GDALRasterBand* pBandout = DSout.GetRasterBand(0);
							pBandout->RasterIO(GF_Write, 0, 0, DSout.GetRasterXSize(), DSout.GetRasterYSize(), &(orog[0]), DSout.GetRasterXSize(), DSout.GetRasterYSize(), GDT_Float32, 0, 0);
							pBandout->SetDescription(CSfcGribDatabase::META_DATA[H_GHGT][M_DESC]);
							pBandout->SetMetadataItem("GRIB_COMMENT", CSfcGribDatabase::META_DATA[H_GHGT][M_COMMENT]);
							pBandout->SetMetadataItem("GRIB_ELEMENT", CSfcGribDatabase::META_DATA[H_GHGT][M_ELEMENT]);
							pBandout->SetMetadataItem("GRIB_SHORT_NAME", CSfcGribDatabase::META_DATA[H_GHGT][M_SHORT_NAME]);
							pBandout->SetMetadataItem("GRIB_UNIT", CSfcGribDatabase::META_DATA[H_GHGT][M_UNIT]);

							for (size_t v = 0; v < 3 && msg; v++)
							{
								try
								{
									size_t dd = TRef - period.Begin();
									if (bIsMissingFeb29)
									{
										ASSERT(dd > 0 || leap_correction == 0);
										dd -= leap_correction;
									}
									else if (bIsFixed30DaysData)
									{
										size_t mm = TRef.as(CTM::MONTHLY) - period.as(CTM::MONTHLY).Begin();
										dd = mm * 30 + min(size_t(DAY_30), TRef.GetDay());
									}

									vector<size_t> startp = { {dd, 0, 0 } };
									vector<size_t> countp = { { 1, nbLat, nbLon } };
									var[v].getVar(startp, countp, &(data[0]));
								}
								catch (exceptions::NcException& e)
								{
									msg.ajoute(e.what());
									msg.ajoute(string("processing variable : ") + VARIABLES_NAMES[v] + " for date " + TRef.GetFormatedString());
								}

								ConvertData(v, data);


								if (msg)
								{
									bool bRevertImage = x_min > -10;

									COneVariableLayer tmp(data.size());

									for (__int64 i = 0; i < (__int64)extents.m_ySize * extents.m_xSize; i++)
									{
										size_t ii = i;
										if (bRevertImage)
										{
											size_t x = (i + extents.m_xSize / 2) % extents.m_xSize;
											size_t y = extents.m_ySize - size_t(i / extents.m_xSize) - 1;
											ii = y * extents.m_xSize + x;
										}
										else
										{
											size_t x = i % extents.m_xSize;
											size_t y = extents.m_ySize - size_t(i / extents.m_xSize) - 1;
											ii = y * extents.m_xSize + x;
										}


										if (landWaterMask[ii] >= minLandWater)
										{
											tmp[ii] = data[i];
										}
										else
										{
											tmp[ii] = no_data_out;
										}
									}


									data = tmp;


									static const size_t VAR[3] = { H_TMIN, H_TMAX, H_PRCP };
									size_t vv = VAR[v];
									GDALRasterBand* pBandout = DSout.GetRasterBand(v + 1);
									pBandout->RasterIO(GF_Write, 0, 0, DSout.GetRasterXSize(), DSout.GetRasterYSize(), &(data[0]), DSout.GetRasterXSize(), DSout.GetRasterYSize(), GDT_Float32, 0, 0);
									pBandout->SetDescription(CSfcGribDatabase::META_DATA[vv][M_DESC]);
									pBandout->SetMetadataItem("GRIB_COMMENT", CSfcGribDatabase::META_DATA[vv][M_COMMENT]);
									pBandout->SetMetadataItem("GRIB_ELEMENT", CSfcGribDatabase::META_DATA[vv][M_ELEMENT]);
									pBandout->SetMetadataItem("GRIB_SHORT_NAME", CSfcGribDatabase::META_DATA[vv][M_SHORT_NAME]);
									pBandout->SetMetadataItem("GRIB_UNIT", CSfcGribDatabase::META_DATA[vv][M_UNIT]);
								}//if var used
							}//for all variables

							DSout.Close(options);

							//if (msg)
							//{
							//	string argument = "-ot Float32 -a_nodata 9999 -stats -co COMPRESS=LZW -co PREDICTOR=3 -co TILED=YES -co BLOCKXSIZE=256 -co BLOCKYSIZE=256";// -a_srs \"" + prj_str ;
							//	string command = "\"" + GetApplicationPath() + "External\\gdal_translate.exe\" " + argument + " \"" + filepath_out + "2\" \"" + filepath_out + "\"";
							//	msg += WinExecWait(command);
							//	msg += RemoveFile(filepath_out + "2");
							//	if (FileExists(filepath_out + "2.aux.xml"))
							//		RemoveFile(filepath_out + "2.aux.xml");
							//}
						}//out open
					}//is valid day

					msg += callback.StepIt();

				}//for all days

				callback.PopTask();
				msg += callback.StepIt();
			}//if msg
		}//for all input files


		callback.PopTask();

		return msg;
	}


	ERMsg CUICMIP6::load_orog_sftlf(std::vector<float>& orog, std::vector<float>& sftlf, float no_data_out, CCallback& callback)
	{
		ERMsg msg;

		string model = Get(MODEL);
		string working_dir = GetDir(WORKING_DIR) + model + "\\";
		float minLandWater = as<float>(MIN_LAND_WATER);



		//Create land/water profile GeoTIFF
		string sftlf_filepath = working_dir + "sftlf_fx_" + model + ".nc";
		string new_sftlf_filepath = working_dir + "sftlf_fx_" + model + ".tif";

		if (!FileExists(new_sftlf_filepath))
			msg += save_sftlf(sftlf_filepath, new_sftlf_filepath);


		//Load land/water profile
		if (msg)
			msg += get_sftlf(new_sftlf_filepath, sftlf);



		//Create elevation
		string orog_filepath = working_dir + "orog_fx_" + model + ".nc";
		string new_orog_filepath = working_dir + "orog_fx_" + model + ".tif";
		SetFileExtension(new_orog_filepath, ".tif");

		if (!FileExists(new_orog_filepath))
			msg += save_orog(orog_filepath, new_orog_filepath, sftlf, minLandWater);


		//Load elevation
		if (msg)
			msg += get_orog(new_orog_filepath, orog, no_data_out);

		return msg;
	}

	CBaseOptions CUICMIP6::GetMapOption()const
	{
		CBaseOptions options;

		string model = Get(MODEL);
		string working_dir = GetDir(WORKING_DIR) + model + "\\";
		string new_orog_filepath = working_dir + "orog_fx_" + model + ".tif";

		ASSERT(FileExists(new_orog_filepath));
		GetMapOptions(new_orog_filepath, options);

		return options;
	}

	ERMsg CUICMIP6::GetMonthlyData(const CMIP6FileList& fileList, const CTPeriod& valid_period, const CGeoExtents& extents, const std::vector<float>& sftlf, float minLandWater, float no_data_out, COneMonthData& data, CCallback& callback)
	{
		ERMsg msg;

		//open files
		for (auto it = fileList.begin(); it != fileList.end() && msg; it++)
		{
			string i_period = it->first;
			CTPeriod period = get_period(i_period);
			CTPeriod intersect = valid_period.Intersect(period);

			double x_min = -999;

			size_t nbLat = 0;
			size_t nbLon = 0;
			size_t nbMonths = 0;

			NcFilePtrArray ncFiles;
			array<NcVar, NB_CMIP6_VARIABLES> var;

			const std::vector<std::string>& varList = it->second;

			string i_ssp;
			if (!varList.empty())
			{
				StringVector tmp(GetFileTitle(varList[0]), "_");
				ASSERT(tmp.size() == NB_COMPONENTS || tmp.size() == NB_COMPONENTS - 1);
				i_ssp = tmp[C_SSP];
			}


			//Open input
			ASSERT(varList.size() == NB_CMIP6_VARIABLES);
			for (size_t i = 0; i < varList.size() && msg; i++)
			{
				try
				{
					size_t v = GetVar(varList[i]);
					ncFiles[v] = NcFilePtr(new NcFile(varList[i], NcFile::read));
					var[v] = ncFiles[v]->getVar(VARIABLES_NAMES[v]);
					msg += callback.StepIt(0);

					if (i == 0)
					{
						auto timeGrid = ncFiles[v]->getDim("time");
						auto latGrid = ncFiles[v]->getDim("lat");
						auto lonGrid = ncFiles[v]->getDim("lon");

						nbMonths = timeGrid.getSize();
						nbLat = latGrid.getSize();
						nbLon = lonGrid.getSize();

						NcVar& var_lon = ncFiles[v]->getVar("lon");
						vector < double> x(nbLon);
						var_lon.getVar(&(x[0]));
						x_min = x.front();

					}
				}
				catch (exceptions::NcException& e)
				{
					msg.ajoute(e.what());
					msg.ajoute(string("Unable to open file : ") + varList[i]);
				}
			}

			if (msg)
			{
				callback.PushTask(string("Create data for period ") + intersect.GetFormatedString("%1 to %2"), intersect.size());

				for (CTRef TRef = intersect.Begin(); TRef <= intersect.End() && msg; TRef++)// for all months
				{
					size_t m = TRef - valid_period.Begin();
					size_t mm = TRef - period.Begin();

					for (size_t v = 0; v < NB_CMIP6_VARIABLES && msg; v++)
					{
						data[m][v].resize(nbLat * nbLon);
						COneVariableLayer tmp(nbLat * nbLon);

						try
						{
							vector<size_t> startp = { {mm, 0, 0 } };
							vector<size_t> countp = { { 1, nbLat, nbLon } };
							var[v].getVar(startp, countp, &(tmp[0]));
						}
						catch (exceptions::NcException& e)
						{
							msg.ajoute(e.what());
							msg.ajoute(string("processing variable : ") + VARIABLES_NAMES[v] + " for date " + TRef.GetFormatedString());
						}

						ConvertData(v, tmp);


						if (msg)
						{
							bool bRevertImage = x_min > -10;


							ASSERT(tmp.size() == extents.m_ySize * extents.m_xSize);
							for (__int64 i = 0; i < (__int64)extents.m_ySize * extents.m_xSize; i++)
							{
								size_t ii = i;
								if (bRevertImage)
								{
									size_t x = (i + extents.m_xSize / 2) % extents.m_xSize;
									size_t y = extents.m_ySize - size_t(i / extents.m_xSize) - 1;
									ii = y * extents.m_xSize + x;
								}
								else
								{
									size_t x = i % extents.m_xSize;
									size_t y = extents.m_ySize - size_t(i / extents.m_xSize) - 1;
									ii = y * extents.m_xSize + x;
								}


								data[m][v][ii] = (sftlf[ii] >= minLandWater) ? tmp[i] : no_data_out;
							}//for all pixel

						}//if msg

					}//for all variables

					msg += callback.StepIt();

				}//for all months

				callback.PopTask();
				msg += callback.StepIt();
			}//if msg
		}//for all input files

		return msg;
	}

	ERMsg CUICMIP6::CreateMonthlyGribs(string filepath_out, string ripf, CCallback& callback)
	{
		ERMsg msg;

		string model = Get(MODEL);
		string ssp = Get(SSP);
		string working_dir = GetDir(WORKING_DIR) + model + "\\";
		float minLandWater = as<float>(MIN_LAND_WATER);
		int first_year = as<int>(FIRST_YEAR);
		int last_year = as<int>(LAST_YEAR);
		CTPeriod valid_period(CTRef(first_year, JANUARY), CTRef(last_year, DECEMBER));


		float no_data_out = -999;
		CStatistic::SetVMiss(no_data_out);

		std::vector<float> orog;
		std::vector<float> sftlf;
		msg = load_orog_sftlf(orog, sftlf, no_data_out, callback);


		CMIP6FileList fileList;
		if (msg)
			msg += GetFileList(model, ssp, ripf, "(Monthly)", valid_period, fileList);

		if (!msg)
			return msg;




		CBaseOptions options = CUICMIP6::GetMapOption();
		options.m_nbBands = valid_period.size();
		CGeoExtents extents = options.m_extents;



		CMonthlyMeanGrid MMG;
		MMG.m_firstYear = first_year;
		MMG.m_lastYear = last_year;

		MMG.m_supportedVariables[TMIN_MN] = true;
		MMG.m_supportedVariables[TMAX_MN] = true;
		MMG.m_supportedVariables[DEL_STD] = false;
		MMG.m_supportedVariables[EPS_STD] = false;
		MMG.m_supportedVariables[PRCP_TT] = true;
		MMG.m_supportedVariables[SPEH_MN] = true;
		MMG.m_supportedVariables[RELH_MN] = false;
		MMG.m_supportedVariables[RELH_SD] = false;
		MMG.m_supportedVariables[WNDS_MN] = true;
		MMG.m_supportedVariables[WNDS_SD] = false;

		string filePathOut = filepath_out;
		WBSF::SetFileTitle(filePathOut, WBSF::GetFileTitle(filePathOut) + "_" + model + "_" + ssp + "_" + ripf);
		msg = MMG.Save(filePathOut);
		if (!msg)
			return msg;

		callback.PushTask(string("Open output images for ") + ssp, NB_CMIP6_VARIABLES);

		size_t nb_grib_open = 0;
		array< CGDALDatasetEx, NB_CMIP6_VARIABLES> grid;
		for (size_t v = 0; v < grid.size(); v++)
		{
			static const size_t VAR_TO_FIELDS[6] = { TMIN_MN,TMAX_MN,PRCP_TT,SPEH_MN,WNDS_MN };
			size_t f = VAR_TO_FIELDS[v];
			string filePathOut = MMG.GetFilePath(f);
			if (!filePathOut.empty())
			{
				msg += grid[v].CreateImage(filePathOut, options);
				nb_grib_open++;
			}

			msg += callback.StepIt();
		}

		callback.PopTask();

		if (!msg)
			return msg;

		callback.PushTask(string("Process all files (") + to_string(fileList.size()) + ") for model " + model + " " + ssp + " and period " + valid_period.GetFormatedString("%1 to %2"), fileList.size());

		//CMonthlyVariableVector data;
		COneMonthData data(valid_period.size());
		msg = GetMonthlyData(fileList, valid_period, extents, sftlf, minLandWater, no_data_out, data, callback);

		//save result
		if (msg)
		{
			callback.PushTask(string("Save output images for ") + ssp, nb_grib_open);
			for (size_t v = 0; v < grid.size() && msg; v++)
			{

				if (grid[v].IsOpen())
				{
					callback.PushTask(GetFieldTitle(v), data.size());
					ASSERT(data.size() == grid[v].GetRasterCount());
					for (size_t m = 0; m < data.size() && msg; m++)
					{
						GDALRasterBand* pBand = grid[v].GetRasterBand(m);
						if (data[m][v].size() == grid[v].GetRasterXSize() * grid[v].GetRasterYSize())
							pBand->RasterIO(GF_Write, 0, 0, grid[v].GetRasterXSize(), grid[v].GetRasterYSize(), &(data[m][v][0]), grid[v].GetRasterXSize(), grid[v].GetRasterYSize(), GDT_Float32, 0, 0);
						//else
							//pBand->RasterIO(GF_Write, 0, 0, grid[v].GetRasterXSize(), grid[v].GetRasterYSize(), &(data[m][v][0]), grid[v].GetRasterXSize(), grid[v].GetRasterYSize(), GDT_Float32, 0, 0);
						msg += callback.StepIt();
					}

					grid[v].Close();
					callback.PopTask();

					msg += callback.StepIt();
				}


			}
			callback.PopTask();
		}





		callback.PopTask();

		return msg;
	}


	ERMsg CUICMIP6::GetGribsList(CTPeriod p, CGribsMap& gribsList, CCallback& callback)
	{
		ERMsg msg;


		string model = Get(MODEL);
		string ssp = Get(SSP);
		string working_dir = GetDir(WORKING_DIR) + model + "\\Gribs\\";


		callback.PushTask(string("Create Gribs (") + (p.GetTType() == CTM::HOURLY ? "hourly" : "daily") + ")", p.size());
		for (CTRef TRef = p.Begin(); TRef <= p.End() && msg; TRef++)
		{
			int year = TRef.GetYear();
			size_t m = TRef.GetMonth();
			size_t d = TRef.GetDay();

			string file_path = FormatA("%s%d\\%02d\\%s_%s_%d%02d%02d.tif", working_dir.c_str(), year, m + 1, model.c_str(), "historical", year, m + 1, d + 1);

			if (FileExists(file_path))
			{
				gribsList[TRef] = file_path;
			}
			else
			{
				string file_path = FormatA("%s%d\\%02d\\%s_%s_%d%02d%02d.tif", working_dir.c_str(), year, m + 1, model.c_str(), ssp.c_str(), year, m + 1, d + 1);

				if (FileExists(file_path))
					gribsList[TRef] = file_path;
			}



			msg += callback.StepIt();


		}

		callback.PopTask();



		return msg;
	}



}
