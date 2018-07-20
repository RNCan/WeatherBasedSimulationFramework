//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 17-04-2018	Rémi Saint-Amant	Transfer liftoff here instead of in the model
// 01-01-2016	Rémi Saint-Amant	Creation
//******************************************************************************
#include "stdafx.h"
#include <boost\algorithm\string.hpp>
#include <Boost\multi_array.hpp>
#include <algorithm>
#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"


#include "Basic/Statistic.h"
#include "Basic/HourlyDatabase.h"
#include "Basic/ModelStat.h"
#include "FileManager/FileManager.h"
#include "Geomatic/IWD.h"
#include "Simulation/Dispersal.h"
#include "Simulation/ExecutableFactory.h"
#include "Simulation/ATM.h"


#include "WeatherBasedSimulationString.h"


using namespace boost;
using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::WEATHER;
using namespace WBSF::DIMENSION;


namespace WBSF
{
	enum TSBWInput { I_YEAR, I_MONTH, I_DAY, I_SEX, I_A, I_M, I_G, I_Fᵒ, I_Fᴰ, NB_SBW_INPUTS };
	static const char* SBW_VARIABLE_NAME[NB_SBW_INPUTS] = { "Year", "Month","Day","sex","A", "M", "G", "F°", "F" };

	//*******************************************************************************

	static const double IWD_POWER = 1.0;
	static const double WIND_SEED_BIAS_FACTOR = 67.3 / 58.7;

	std::string CWindStability::GetWindStabilityName(int stabType)
	{
		ASSERT(stabType >= 0 && stabType < NB_STABILITY);

		static const StringVector WIND_STABILITY_NAME(IDS_WG_WIND_STABILITY, "|;");

		return WIND_STABILITY_NAME[stabType];
	}

	//*******************************************************************************

	const char* CDispersalParamters::MEMBERS_NAME[CDispersalParamters::NB_MEMBERS] = { "World", "Moths" };


	//*******************************************************************************
	const char* CDispersal::XML_FLAG = "Dispersal";
	const char* CDispersal::MEMBERS_NAME[CDispersal::NB_MEMBERS_EX] = { "Parameters" };
	const int CDispersal::CLASS_NUMBER = CExecutableFactory::RegisterClass(CDispersal::GetXMLFlag(), &CDispersal::CreateObject);

	CDispersal::CDispersal()
	{
		ClassReset();
	}

	CDispersal::CDispersal(const CDispersal& in)
	{
		operator=(in);
	}

	CDispersal::~CDispersal()
	{}

	void CDispersal::Reset()
	{
		CExecutable::Reset();
		ClassReset();
	}

	void CDispersal::ClassReset()
	{
		m_name = "Dispersal";
		m_parameters.clear();
	}

	CDispersal& CDispersal::operator =(const CDispersal& in)
	{
		if (&in != this)
		{
			CExecutable::operator =(in);
			m_parameters = in.m_parameters;
		}

		ASSERT(*this == in);
		return *this;
	}

	bool CDispersal::operator == (const CDispersal& in)const
	{
		bool bEqual = true;

		if (CExecutable::operator !=(in))bEqual = false;
		if (m_parameters != in.m_parameters)bEqual = false;

		return bEqual;
	}

	ERMsg CDispersal::GetParentInfo(const CFileManager& fileManager, CParentInfo& info, CParentInfoFilter filter)const
	{
		ERMsg msg;

		if (filter[REPLICATION])
			filter.set(TIME_REF);
		//if (filter[PARAMETER])
			//filter.set(VARIABLE);

		msg = m_pParent->GetParentInfo(fileManager, info, filter);
		if (msg)
		{
			//CTPeriod pIn = info.m_period;
			if (filter[LOCATION])
			{
				//same as parent
			}

			if (filter[PARAMETER])
			{
				//same as parent

				/*const CModelOutputVariableDefVector& vars = info.m_variables;

				if (!vars.empty())
				{
					info.m_parameterset.clear();

					for (size_t i = 0; i < vars.size(); i++)
					{
						CModelInput modelInput;
						modelInput.push_back(CModelInputParam("Variable", vars[i].m_name));
						info.m_parameterset.push_back(modelInput);
					}
					info.m_parameterset.m_pioneer = info.m_parameterset[0];
				}*/
			}

			if (filter[REPLICATION])
			{
				ASSERT(info.m_period.GetTType() == CTM::ATEMPORAL);
				info.m_nbReplications *= info.m_period.size();
			}

			if (filter[TIME_REF])
			{
				info.m_period.Transform(CTM(CTM::HOURLY));
			}

			if (filter[VARIABLE])
			{
				GetOutputDefinition(info.m_variables);
			}
		}

		return msg;
	}

	size_t Find(const CModelOutputVariableDefVector& vars, string name)
	{
		size_t v = NOT_INIT;

		for (size_t vv = 0; vv < vars.size() && v == NOT_INIT; vv++)
			if (WBSF::IsEqual(vars[vv].m_name, name))
				v = vv;


		return v;
	}


	void CDispersal::GetInputDBInfo(CResultPtr& pResult, CDBMetadata& info)
	{
		const CModelOutputVariableDefVector& vars = pResult->GetMetadata().GetOutputDefinition();

		if (!vars.empty())
		{
			CModelOutputVariableDefVector outputVar;
			GetOutputDefinition(outputVar);

			CTPeriod period = m_parameters.m_world.m_simulationPeriod;
			period.Transform(CTM::HOURLY);

			info.SetLocations(pResult->GetMetadata().GetLocations());
			info.SetParameterSet(pResult->GetMetadata().GetParameterSet());
			info.SetOutputDefinition(outputVar);
			info.SetTPeriod(period);
		}
	}


	ERMsg CDispersal::GetOutputDefinition(const CFileManager& fileManager, CModelOutputVariableDefVector& outputVar)const
	{
		GetOutputDefinition(outputVar);

		return ERMsg();
	}

	void CDispersal::GetOutputDefinition(CModelOutputVariableDefVector& outputVar)const
	{
		outputVar.clear();
		ASSERT(NB_ATM_OUTPUT == 34);
		outputVar.push_back(CModelOutputVariableDef("FlightNo", "FlightNo", "", "Flight numero for this moth", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("Age", "Age", "[0..1]", "Phisiological age", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("Sex", "Sex", "m=0|f=1", "Sex of moth", CTM(CTM::ATEMPORAL), 0));
		outputVar.push_back(CModelOutputVariableDef("A", "A", "cm²", "Forewing surface area", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("M", "M", "g", "Dry weight", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("Broods", "Broods", "", "Broods", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("G", "G", "", "Gravidity", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("F", "F", "", "F", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("State", "State", "", "State of moth", CTM(CTM::ATEMPORAL), 0));
		outputVar.push_back(CModelOutputVariableDef("Flag", "Flag", "", "State flag", CTM(CTM::ATEMPORAL), 0));
		outputVar.push_back(CModelOutputVariableDef("X", "X", "m", "Current X coordinate", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("Y", "Y", "m", "Current Y coordinate", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("Latitude", "Latitude", "°", "Current latitude", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("Longitude", "Longitude", "°", "Current longitude", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("Tair", "Tair", "°C", "Air temperature", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("Prcp", "Prcp", "mm", "Precipitation", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("U", "U", "km/h", "mean U wind speed", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("V", "V", "km/h", "mean V wind speed", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("W", "W", "km/h", "mean W wind speed", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("MH", "MeanHeight", "m", "Mean flight height.", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("CH", "CurrentHeight", "m", "Current flight height. Begin at 5 meters and end at 0.", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("MDH", "MeanDeltaHeight", "m", "Mean change in height", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("MothSpeed", "MothSpeed", "", "Moth speed without wind speed", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("Wh", "Whorizontal", "km/h", "Mean horizontal velocity", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("Wv", "Wvertical", "km/h", "Mean vertical velocity", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("D", "Direction", "°", "Direction", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("Distance", "Distance", "m", "Distance", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("DistanceOrigine", "DistanceOrigine", "m", "DistanceOrigine", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("FlightTime", "FlightTime", "h", "FlightTime", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("LiftoffTime", "LiftoffTime", "Decimal hour", "", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("LandingTime", "LandingTime", "Decimal hour", "", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("LiftoffT", "LiftoffT", "°C", "Liftoff Temperature", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("LandingT", "LandingT", "°C", "Landing Temperature", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("Defoliation", "Defoliation", "", "Defoliation", CTM(CTM::ATEMPORAL), 5));

	}


	void CDispersal::writeStruc(zen::XmlElement& output)const
	{
		string test = GetMemberName(ATM_PARAMETERS);

		CExecutable::writeStruc(output);
		zen::XmlOut out(output);
		out[GetMemberName(ATM_PARAMETERS)](m_parameters);
	}

	bool CDispersal::readStruc(const zen::XmlElement& input)
	{
		CExecutable::readStruc(input);
		zen::XmlIn in(input);
		in[GetMemberName(ATM_PARAMETERS)](m_parameters);

		return true;
	}

	//*******************************************************************************
	CGeoPoint CDispersal::GetNewPosition(const CGeoPoint& pt, double U, double V)
	{
		//formaula from site:http://williams.best.vwh.net/avform.htm#LL
		//A point{ lat, lon } is a distance d out on the tc radial from point 1 if:
		//This algorithm is limited to distances such that dlon <pi / 2, i.e those that extend around less than one quarter of the circumference of the earth in longitude.A completely general, but more complicated algorithm is necessary if greater distances are allowed :

		double tc = fmod(3 * PI / 2 - atan2(V, U), 2 * PI);
		double distance = sqrt(U*U + V * V);//distance in km
		double d = distance / 6371; //distance in radian 6366.71

		double lat1 = Deg2Rad(pt.m_lat);
		double lon1 = Deg2Rad(pt.m_lon);

		double lat2 = asin(sin(lat1)*cos(d) + cos(lat1)*sin(d)*cos(tc));
		double dlon = atan2(sin(tc)*sin(d)*cos(lat1), cos(d) - sin(lat1)*sin(lat2));
		double lon2 = fmod(lon1 + dlon + PI, 2 * PI) - PI;

		CGeoPoint pt2(Rad2Deg(lon2), Rad2Deg(lat2), PRJ_WGS_84);
		ASSERT(fabs(pt.GetDistance(pt2) / 1000 - distance) < 0.1);

		return pt2;
	}






	//http://www.ndbc.noaa.gov/view_text_file.php?filename=44011h2013.txt.gz&dir=data/historical/stdmet/

	//http ://www.meds-sdmm.dfo-mpo.gc.ca/isdm-gdsi/azmp-pmza/met/plot-graph-eng.asp?a=12
	//http ://www.meds-sdmm.dfo-mpo.gc.ca/isdm-gdsi/azmp-pmza/met/plot-graph-eng.asp?a=12
	//www.meds - sdmm.dfo - mpo.gc.ca / alphapro / zmp / met / data / Natashquan_A_HLY01.zip
	//http://www.meds-sdmm.dfo-mpo.gc.ca/isdm-gdsi/azmp-pmza/met/index-eng.asp
	//http ://isdm.gc.ca/isdm-gdsi/waves-vagues/search-recherche/map-carte/index-eng.asp?MedsID=All&ID=&StnName=&Active=ON&Lat1=&Lat2=&Long1=&Long2=&sDate=&eDate=&typedisplay=map&Search=Get+Results
	//http://isdm.gc.ca/isdm-gdsi/waves-vagues/search-recherche/list-liste/data-donnees-eng.asp?medsid=C44255


	struct location_compare
	{
		bool operator() (const CLocation& l1, const CLocation& l2) const
		{
			return l1.m_lat < l2.m_lat || l1.m_lon < l2.m_lon;
		}
	};


	double SumBroods(const CNewSectionData& section, size_t vBroods)
	{
		double sumBroods = 0;
		for (size_t t = 0; t < section.GetRows(); t++)
			sumBroods += section[t][vBroods][MEAN];

		return sumBroods;
	}

	/*ERMsg CreateGribsFromNetCDF(CCallback& callback)
	{
		ERMsg msg;

		enum VAR { V_GHT, V_T, V_U, V_V, V_W, V_PRESS, V_PRCP, V_NBV };

		static const char* VN[V_NBV] = { "geopotential_height","temperature","u_unstaggered","v_unstaggered","w_unstaggered","pressure","precipitation" };

		WBSF::ofStream convert;
		msg = convert.open("D:\\Travaux\\WRF2013Test\\Convert.bat");

		WBSF::ofStream build;
		msg += build.open("D:\\Travaux\\WRF2013Test\\Build.bat");

		if (msg)
		{
			StringVector list = GetFilesList("D:\\Travaux\\WRF2013Test\\NetCDF\\*.nc");
			for (size_t i = 0; i < list.size(); i++)
			{
				string file_title = GetFileTitle(list[i]);
				string file_path = "D:\\Travaux\\WRF2013Test\\tmp\\" + file_title + ".file_list.txt";

				WBSF::ofStream file_list;

				msg += file_list.open(file_path);
				if (msg)
				{

					for (size_t v = 0; v < V_NBV; v++)
					{
						for (size_t l = 0; l < (v!= V_PRCP ?13:1); l++)
						{
							convert << "gdal_translate -b " << l+1 << " -ot Float32 -a_ullr  145750 189000 420750 -80000 -a_srs \"+proj=lcc +lat_1=30 +lat_2=60 +lat_0=48.13746 +lon_0=-71.4 +x_0=0 +y_0=0 +datum=WGS84\" NETCDF:\"D:/Travaux/WRF2013Test/NetCDF/" << file_title << ".nc\":" << VN[v] << " ./tmp/" << file_title << "." << VN[v] << "." << l << ".tif" << endl;
							file_list << "D:\\Travaux\\WRF2013Test\\tmp\\" << file_title << "." << VN[v] << "." << l << ".tif" << endl;
						}
					}

					file_list.close();
					build << "gdalBuildVRT -overwrite -separate -input_file_list " << file_path << " D:\\Travaux\\WRF2013Test\\tmp\\" << file_title << ".vrt" << endl;
					build << "gdal_translate -co compress=LZW -ot Float32 -co TILED=YES -co BLOCKXSIZE=128 -co BLOCKYSIZE=128 .\\tmp\\" << file_title << ".vrt .\\geoTIFF\\" << file_title << ".tif" << endl << endl;

					WBSF::CopyOneFile("D:\\Travaux\\WRF2013Test\\template.inv", "D:\\Travaux\\WRF2013Test\\GeoTIFF\\"+ file_title +".inv", false);
				}
			}
		}

		return msg;
	}*/

	ERMsg CDispersal::Execute(const CFileManager& fileManager, CCallback& callback)
	{
		ERMsg msg;

		//return CreateGribsFromNetCDF(callback);

		GIntBig test = GDALGetCacheMax64();
		GDALSetCacheMax64(128 * 1024 * 1024);


		CATMWorld world;
		world.m_world_param = m_parameters.m_world;
		world.m_moths_param = m_parameters.m_moths;
		world.m_nb_max_threads = CTRL.m_nbMaxThreads;

		ofStream output_file;
		ofStream sub_hourly_file;

		string DEM_filepath, gribs_filepath, hourly_DB_filepath, defoliation_filepath, host_filepath, distraction_filepath, water_filepath;
		string outputPath = GetPath(fileManager);		//Generate output path
		string DBFilePath = GetDBFilePath(outputPath);		//Generate DB file path
		string outputFilePath = !m_parameters.m_world.m_outputFileTitle.empty() && m_parameters.m_world.m_bOutputSubHourly ? fileManager.GetOutputPath() + m_parameters.m_world.m_outputFileTitle + ".csv" : "";

		//if (!m_parameters.m_world.m_simulationPeriod.IsInside(m_parameters.m_world.m_flightPeriod))
			//msg.ajoute("Flight period (" + m_parameters.m_world.m_flightPeriod.GetFormatedString() + ") must be inside simulation period (" + m_parameters.m_world.m_simulationPeriod.GetFormatedString() + ")");

		if (!m_parameters.m_world.m_DEM_name.empty())
			msg += fileManager.MapInput().GetFilePath(m_parameters.m_world.m_DEM_name, DEM_filepath);
		else
			msg.ajoute("A DEM must be supply");

		if (!m_parameters.m_world.m_defoliation_name.empty())
			msg += fileManager.MapInput().GetFilePath(m_parameters.m_world.m_defoliation_name, defoliation_filepath);
		if (!m_parameters.m_world.m_water_name.empty())
			msg += fileManager.MapInput().GetFilePath(m_parameters.m_world.m_water_name, water_filepath);
		if (m_parameters.m_world.UseGribs())
		{
			if (!m_parameters.m_world.m_gribs_name.empty())
				msg += fileManager.Gribs().GetFilePath(m_parameters.m_world.m_gribs_name, gribs_filepath);
			else
				msg.ajoute("Gribs file is not defined");
		}

		if ((m_parameters.m_world.UseHourlyDB() || m_parameters.m_world.m_PSource == CATMWorldParamters::PRCP_WEATHER_STATION))
		{
			if (!m_parameters.m_world.m_hourly_DB_name.empty())
				msg += fileManager.Hourly().GetFilePath(m_parameters.m_world.m_hourly_DB_name, hourly_DB_filepath);
			else
				msg.ajoute("Hourly database is not defined");
		}

		output_file.open(DBFilePath + ".tmp", std::fstream::binary | std::fstream::out | std::fstream::trunc);




		if (!outputFilePath.empty())
			msg += sub_hourly_file.open(outputFilePath);




		CResultPtr pResult = m_pParent->GetResult(fileManager);
		msg += pResult->Open();

		//open outputDB
		CResult result;
		msg += result.Open(DBFilePath, std::fstream::binary | std::fstream::out | std::fstream::trunc);

		if (!msg)
			return msg;


		//init output info
		CDBMetadata& metadata = result.GetMetadata();
		GetInputDBInfo(pResult, metadata);

		const CModelOutputVariableDefVector& vars = pResult->GetMetadata().GetOutputDefinition();

		bool bMissing = false;
		std::array<size_t, NB_SBW_INPUTS> varsPos;
		for (size_t i = 0; i < NB_SBW_INPUTS; i++)
		{
			varsPos[i] = Find(vars, SBW_VARIABLE_NAME[i]);
			bMissing = bMissing || varsPos[i] == NOT_INIT;
		}


		if (bMissing)
		{
			msg.ajoute("Invalid dispersal variables input. Variable \"Year\", \"Month\", \"Day\",\"Sex\", \"A\", \"M\", \"G\", \"F°\", \"F\", must be defined");
			return msg;
		}


		callback.PushTask("Open Dispersal's Input", 4);

		if (msg)
		{
			msg += world.m_DEM_DS.OpenInputImage(DEM_filepath);
			callback.StepIt();
		}

		if (msg)
		{
			msg += world.m_weather.Load(gribs_filepath, hourly_DB_filepath, callback);
			callback.StepIt();
		}

		if (msg)
		{
			if (!defoliation_filepath.empty())
			{
				msg += world.m_defoliation_DS.OpenInputImage(defoliation_filepath);
			}
			else if (world.m_moths_param.m_maxFlights > 1)
			{
				msg.ajoute("maximum flights is more than 1 but there is no defoliation map. Reset maximum flights to 1 or provide defoliation map.");
			}

			callback.StepIt();
		}

		if (msg)
		{
			if (!water_filepath.empty())
			{
				msg += world.m_water_DS.OpenInputImage(water_filepath);
			}
			else if (world.m_moths_param.m_maxFlights > 1)
			{
				msg.ajoute("maximum flights is more than 1 but there is no water map. Reset maximum flights to 1 or provide water map.");
			}

			callback.StepIt();
		}

		callback.PopTask();


		CTPeriod outputPeriod2 = world.m_world_param.m_simulationPeriod;
		outputPeriod2.End()++;//add one day at the end
		outputPeriod2.Transform(CTM::HOURLY);
		outputPeriod2.Begin().m_hour = 16;
		outputPeriod2.End().m_hour = 15;


		CTPeriod weatherPeriod = world.m_weather.GetEntireTPeriod();
		//weatherPeriod.Transform(CTM::DAILY);

		CTPeriod savedPeriod = outputPeriod2.Intersect(weatherPeriod);
		

		
		ASSERT(outputPeriod2.GetTM() == weatherPeriod.GetTM() );
		if (!outputPeriod2.IsIntersect(weatherPeriod))
		{
			msg.ajoute("Weather period doesn't intersect simulation period");
		}

		if (!msg)
			return msg;

		//if (period.End() > weatherPeriod.End())//stop simulation at the end of weather 
			//period.End() = weatherPeriod.End();


		const CLocationVector& locations = metadata.GetLocations();
		callback.PushTask("Init dispersal moths", metadata.GetNbReplications() *locations.size()*metadata.GetParameterSet().size());

		CGeoExtents extents = world.m_DEM_DS.GetExtents();
		extents.Reproject(GetReProjection(world.m_DEM_DS.GetPrjID(), PRJ_WGS_84));

		double max_moth_prop = 1;//all moths by default
		size_t nbMoths = 0;
		//size_t nbMothsPeriod = 0;
		CTPeriod period98;
		//CSBWMoths moths;
		if (world.m_world_param.m_maxFlyers > 0)//remplacer maxFlyers par maxMoths
		{
			//for optimisation, clean extra moths
			GetNbMoths(pResult, nbMoths, period98);
			if (nbMoths > world.m_world_param.m_maxFlyers*0.98)
			{
				max_moth_prop = ((double)world.m_world_param.m_maxFlyers*0.98 / nbMoths);

				callback.AddMessage("Number of moths before optimization: " + ToString(nbMoths) + " moths");
				callback.AddMessage("Keepted ratio: " + ToString(max_moth_prop * 100, 1) + " %");
			}
		}

		std::vector<std::array<size_t, 3>> IDmap;


		//double p_total = 0;
		size_t no = 0;
		size_t nbReplications = 0;
		for (size_t l = 0; l < locations.size() && msg; l++)
		{
			for (size_t p = 0; p < pResult->GetMetadata().GetParameterSet().size() && msg; p++)
			{
				size_t rr = 0;
				for (size_t r = 0; r < pResult->GetMetadata().GetNbReplications() && msg; r++)
				{
					CNewSectionData section;
					pResult->GetSection(l, p, r, section);
					assert(section.GetCols() == pResult->GetNbCols(false));

					for (size_t t = 0; t < section.GetRows() && msg; t++)
					{
						std::array<double, NB_SBW_INPUTS> v;
						for (size_t i = 0; i < varsPos.size(); i++)
							v[i] = section[t][varsPos[i]][MEAN];

						if (v[I_YEAR] > -999 && v[I_MONTH] > -999 && v[I_DAY] > -999)
						{
							//let the first and the last 1% to keep extrem
							CTRef emergingDate = CTRef(int(v[I_YEAR]), size_t(v[I_MONTH]) - 1, size_t(v[I_DAY]) - 1);
							bool bExtrem = !period98.IsInside(emergingDate);
							if (bExtrem || world.random().Randu() <= max_moth_prop)//remove moth by optimization
							{
								
								if (world.m_world_param.m_simulationPeriod.IsInside(emergingDate))
								{
									CSBWMoth moth(world);

									moth.m_ID = no;
									IDmap.push_back({ { l,p,rr } });

									moth.m_emergingDate = emergingDate;//daily reference of ready moths
									moth.m_sex = v[I_SEX];//sex (MALE=0, FEMALE=1)
									moth.m_A = v[I_A];
									moth.m_M = v[I_M];
									moth.m_G = v[I_G];
									moth.m_Fᵒ = v[I_Fᵒ];
									moth.m_Fᴰ = v[I_Fᴰ];
									moth.m_F = moth.m_Fᴰ;
									moth.m_location = locations[l];
									moth.m_newLocation = locations[l];
									moth.m_pt = locations[l];
									moth.m_UTCShift = CTimeZones::GetTimeZone(locations[l]);

									if (extents.IsInside(moth.m_pt))
										world.m_moths.push_back(moth);
									else
										callback.AddMessage("WARNING: Simulation point outside elevation map");

									rr++;
									nbReplications = max(nbReplications, rr);
									no++;

								}//is inside simulation period

								world.m_seasonalIndividuals++;
								//p_total += 1.0 / nbMothsPeriod;
							}


						}//is valid insect
					}//for all rows

					msg += callback.StepIt();
				}//for all replications
			}//for all paramterset
		}//for all locations




		callback.PopTask();

		//CTPeriod outputPeriod = period;

		callback.AddMessage("Number of moths for the entire season: " + ToString(world.m_seasonalIndividuals) + " moths");
		callback.AddMessage("Number of moths for the period: " + to_string(world.m_moths.size()) + " moths (" + to_string(100.0*world.m_moths.size() / world.m_seasonalIndividuals) + " %)");
		callback.AddMessage("Execute dispersal with: " + to_string(world.m_moths.size()) + " moths (" + to_string(100.0*world.m_moths.size() / world.m_seasonalIndividuals) + " %)");
		callback.AddMessage("Weather period:         " + world.m_weather.GetEntireTPeriod().GetFormatedString());
		callback.AddMessage("Output period:          " + outputPeriod2.GetFormatedString());
		callback.AddMessage("Output replications (max moths per location):" + ToString(nbReplications));

		metadata.SetNbReplications(nbReplications);
		metadata.SetTPeriod(outputPeriod2);

		if (!world.m_moths.empty())
		{

			/*CATMOutputMatrix output(world.m_moths.size());
			for (size_t no = 0; no < output.size(); no++)
				output[no].Init(outputPeriod, VMISS);

			msg = world.Execute(output, sub_hourly_file, callback);
			if (msg)
			{
				callback.PushTask("Save data" , output.size() );

				ATMOutput missing_output;
				missing_output.Init(outputPeriod, VMISS);

				for (size_t l = 0; l < locations.size(); l++)
				{
					for (size_t p = 0; p < metadata.GetParameterSet().size(); p++)
					{
						for (size_t r = 0; r < nbReplications; r++)
						{
							size_t section = result.GetSectionNo(l, p, r);

							std::array<size_t, 3> lpr = { {l,p,r} };
							auto it = find(IDmap2.begin(), IDmap2.end(), lpr);
							if (it != IDmap2.end())
							{
								size_t no = distance(IDmap2.begin(), it);
								msg += result.SetSection(section, output[no]);
							}
							else
							{
								msg += result.SetSection(section, missing_output);

							}
							msg += callback.StepIt();
						}
					}
				}


				callback.PopTask();

				if (msg&& m_parameters.m_world.m_bCreateEggMaps)
				{
					string outputFilePath = fileManager.GetOutputMapPath() + m_parameters.m_world.m_eggMapsTitle + ".tif";
					msg = world.CreateEggDepositionMap(outputFilePath, output, callback);
				}
			}*/



			CStatistic::SetVMiss(-999);

			world.Init(callback);


			//world.write_sub_hourly_header(output_file);

			if (sub_hourly_file.is_open())
				world.write_sub_hourly_header(sub_hourly_file);


			//get period of simulation
			CTPeriod period = world.m_world_param.m_simulationPeriod;
			callback.PushTask("Execute dispersal for year = " + ToString(period.Begin().GetYear()) + " (" + ToString(period.GetNbDay()) + " days)", period.GetNbDay() * 2);
			callback.AddMessage("Date              NotEmerged        Emerging          WaitingToFly      Flying            FinishingEggs     Finished       ");

			//for all days
			for (CTRef TRef = period.Begin(); TRef <= period.End() && msg; TRef++)
			{
				CATMOutputMatrix output;
				world.init_output(TRef, output);

				CATMOutputMatrix sub_output;
				if (sub_hourly_file.is_open())
					world.init_sub_hourly(TRef, sub_output);	//initmemory

				msg += world.ExecuteOneNight(TRef, output, sub_output, callback);

				if (msg)
					msg += world.save_output(savedPeriod, output_file, output, callback);

				if (msg && sub_hourly_file.is_open())
				{
					msg += world.save_sub_output(TRef, sub_hourly_file, sub_output, callback);
				}//if sub hourly output
			}//for all valid days


			output_file.close();
			if (sub_hourly_file.is_open())
				sub_hourly_file.close();

			callback.PopTask();

			if (msg)
				msg += copy_result(DBFilePath + ".tmp", IDmap, savedPeriod, result, callback);

			if (msg&& m_parameters.m_world.m_bCreateEggMaps)
			{
				string inputFilePath = DBFilePath + ".tmp";
				string outputFilePath = fileManager.GetOutputMapPath() + m_parameters.m_world.m_eggMapsTitle + ".tif";

				//a faire...
				//msg = world.CreateEggDepositionMap(outputFilePath, output, callback);
			}

			WBSF::RemoveFile(DBFilePath + ".tmp");
		}

		result.Close();

		return msg;
	}

	ERMsg CDispersal::copy_result(const string& file_path, const std::vector<std::array<size_t, 3>>& IDmap, CTPeriod savedPeriod, CResult& result, CCallback& callback)
	{
		ERMsg msg;
		ifStream input_file;
		input_file.open(file_path, ios_base::in | ios_base::binary);

		const size_t size_struct = sizeof(size_t) + savedPeriod.size()*(sizeof(__int32) + sizeof(float)*NB_ATM_OUTPUT);
		size_t length = input_file.length();
		ASSERT(length == IDmap.size()*size_struct);

		callback.PushTask("Save result", result.GetNbSection());

		for (size_t l = 0; l < result.GetDimension()[LOCATION] && msg; l++)
		{
			for (size_t p = 0; p < result.GetDimension()[PARAMETER] && msg; p++)
			{
				for (size_t r = 0; r < result.GetDimension()[REPLICATION] && msg; r++)
				{
					ATMOutput output;
					output.Init(savedPeriod, VMISS);

					std::array<size_t, 3> lpr = { { l,p,r } };
					auto it = find(IDmap.begin(), IDmap.end(), lpr);
					if (it != IDmap.end())
					{
						CTRef trefTest = savedPeriod.Begin();
						size_t no = input_file.read_value<size_t>();
						ASSERT(no < result.GetNbSection());

						for (size_t t = 0; t < savedPeriod.size() && msg; t++)
						{
							__int32 tmp = input_file.read_value<__int32 >();
							ASSERT(!input_file.eof());

							CTRef TRef;
							TRef.Set__int32(tmp);
							ASSERT(output.IsInside(TRef));
							ASSERT(TRef == trefTest);
							trefTest++;


							for (size_t v = 0; v < NB_ATM_OUTPUT; v++)
							{
								float value = input_file.read_value<float>();
								ASSERT(!input_file.eof());

								if (value > -999)
									output[TRef][v] = value;

								ASSERT(output[TRef][ATM_DEFOLIATION] != 0);
							}

						}
					}


					size_t section = result.GetSectionNo(l, p, r);
					msg += result.SetSection(section, output);

					msg += callback.StepIt();
				}//r
			}//p
		}//l

		input_file.close();
		callback.PopTask();

		return msg;
	}


	void CDispersal::GetNbMoths(CResultPtr pResult, size_t& nbMoths, CTPeriod& period98)
	{
		//seasonalIndividuals = 0;
		//periodIndividuals = 0;
	//	size_t seasonalIndividuals = 0;

		CStatisticEx Tref98;
		const CModelOutputVariableDefVector& vars = pResult->GetMetadata().GetOutputDefinition();
		std::array<size_t, NB_SBW_INPUTS> varsPos;
		for (size_t i = 0; i < NB_SBW_INPUTS; i++)
			varsPos[i] = Find(vars, SBW_VARIABLE_NAME[i]);

		const CLocationVector& locations = pResult->GetMetadata().GetLocations();
		for (size_t l = 0; l < pResult->GetMetadata().GetLocations().size(); l++)
		{
			for (size_t p = 0; p < pResult->GetMetadata().GetParameterSet().size(); p++)
			{
				size_t rr = 0;
				for (size_t r = 0; r < pResult->GetMetadata().GetNbReplications(); r++)
				{
					CNewSectionData section;
					pResult->GetSection(l, p, r, section);
					assert(section.GetCols() == pResult->GetNbCols(false));

					for (size_t t = 0; t < section.GetRows(); t++)
					{
						std::array<double, NB_SBW_INPUTS> v;
						for (size_t i = 0; i < varsPos.size(); i++)
							v[i] = section[t][varsPos[i]][MEAN];

						if (v[I_YEAR] > -999 && v[I_MONTH] > -999 && v[I_DAY] > -999)
						{
							nbMoths++;
							CTRef emergingDate = CTRef(int(v[I_YEAR]), size_t(v[I_MONTH]) - 1, size_t(v[I_DAY]) - 1);
							Tref98 += emergingDate;
						}
					}//for all rows
				}//for all replications
			}//for all paramterset
		}//for all locations

		period98.Begin() = Tref98.percentil(1) + 1;
		period98.End() = Tref98.percentil(99) - 1;
//		return seasonalIndividuals;
	}
}