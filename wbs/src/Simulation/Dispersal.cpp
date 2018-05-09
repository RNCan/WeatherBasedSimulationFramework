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

		if (filter[PARAMETER])
			filter.set(VARIABLE);

		msg = m_pParent->GetParentInfo(fileManager, info, filter);
		if (msg)
		{
			//CTPeriod pIn = info.m_period;
			if (filter[LOCATION])
			{
			}

			if (filter[PARAMETER])
			{
				//que faire s'il y a déjè des parameterset????
				const CModelOutputVariableDefVector& vars = info.m_variables;

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
				}
			}

			if (filter[REPLICATION])
			{
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
			info.SetOutputDefinition(outputVar);
			info.SetTPeriod(period);


			//size_t vMale = Find(vars, "MaleExodus");
			//size_t vFemale = Find(vars, "FemaleExodus");
			//size_t vAdult = Find(vars, "Adults");
			//size_t vOvipositingAdult = Find(vars, "OvipositingAdult");
			//size_t vBroods = Find(vars, "Brood");

			//if (vMale != NOT_INIT || vFemale != NOT_INIT )
			//{
			//	//size_t nbReplication = pResult->GetMetadata().GetNbReplications();
			//	size_t totalMoths = 0;
			//	for (size_t l = 0; l < pResult->GetMetadata().GetLocations().size(); l++)
			//	{
			//		for (size_t p = 0; p < pResult->GetMetadata().GetParameterSet().size(); p++)
			//		{
			//			for (size_t r = 0; r < pResult->GetMetadata().GetNbReplications(); r++)
			//			{
			//				CNewSectionData section;
			//				pResult->GetSection(l, p, r, section);

			//				for (size_t t = 0; t < section.GetRows(); t++)
			//				{
			//					if (period.IsInside(section.GetTRef(t)))
			//					{
			//						if (section[t][vMale].IsInit() && section[t][vFemale].IsInit())
			//						{
			//							size_t nbMoths = ceil(section[t][vMale][MEAN]) + ceil(section[t][vFemale][MEAN]);
			//							totalMoths += nbMoths;
			//						}
			//					}
			//				}
			//			}
			//		}
			//	}

			//	info.SetNbReplications(totalMoths);
			//}
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
		ASSERT(NB_ATM_OUTPUT == 29);
		outputVar.push_back(CModelOutputVariableDef("FlightNo", "FlightNo", "", "Flight numero for this moth", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("Scale", "Scale", "", "Scale factor the moth", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("Sex", "Sex", "m=0|f=1", "Sex of moth", CTM(CTM::ATEMPORAL), 0));
		outputVar.push_back(CModelOutputVariableDef("A", "A", "cm²", "Forewing surface area", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("M", "M", "g", "Dry weight", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("G", "G", "", "Gravidity", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("EggsLaid", "EggsLaid", "", "EggsLaid", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("State", "State", "", "State of moth", CTM(CTM::ATEMPORAL), 0));
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
		outputVar.push_back(CModelOutputVariableDef("Wh", "Whorizontal", "km/h", "Mean horizontal velocity", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("Wv", "Wvertical", "km/h", "Mean vertical velocity", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("D", "Direction", "°", "Direction", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("Distance", "Distance", "m", "Distance", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("DistanceOrigine", "DistanceOrigine", "m", "DistanceOrigine", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("FlightTime", "FlightTime", "h", "FlightTime", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("LiftoffTime", "LiftoffTime", "", "Decimal hour", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("LandingTime", "LandingTime", "", "Decimal hour", CTM(CTM::ATEMPORAL), 5));
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

	ERMsg CDispersal::Execute(const CFileManager& fileManager, CCallback& callback)
	{
		ERMsg msg;

		GIntBig test = GDALGetCacheMax64();
		GDALSetCacheMax64(128 * 1024 * 1024);


		CATMWorld world;
		world.m_world_param = m_parameters.m_world;
		world.m_moths_param = m_parameters.m_moths;
		world.m_nb_max_threads = CTRL.m_nbMaxThreads;
		ofStream output_file;

		string DEM_filepath, gribs_filepath, hourly_DB_filepath, defoliation_filepath, host_filepath, distraction_filepath, water_filepath;
		string outputPath = GetPath(fileManager);		//Generate output path
		string DBFilePath = GetDBFilePath(outputPath);		//Generate DB file path
		string outputFilePath = !m_parameters.m_world.m_outputFileTitle.empty() && m_parameters.m_world.m_bOutputSubHourly ? fileManager.GetOutputPath() + m_parameters.m_world.m_outputFileTitle + ".csv" : "";

		if (!m_parameters.m_world.m_simulationPeriod.IsInside(m_parameters.m_world.m_flightPeriod))
			msg.ajoute("Flight period (" + m_parameters.m_world.m_flightPeriod.GetFormatedString() + ") must be inside simulation period (" + m_parameters.m_world.m_simulationPeriod.GetFormatedString() + ")");

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

		if ((m_parameters.m_world.UseHourlyDB() || m_parameters.m_world.m_PSource == CATMWorldParamters::PRCP_WEATHER_STATION ))
		{
			if (!m_parameters.m_world.m_hourly_DB_name.empty())
				msg += fileManager.Hourly().GetFilePath(m_parameters.m_world.m_hourly_DB_name, hourly_DB_filepath);
			else
				msg.ajoute("Hourly database is not defined");
		}

		if (!outputFilePath.empty())
			msg += output_file.open(outputFilePath);




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

		enum TInput { I_YEAR, I_MONTH, I_DAY, I_SEX, I_A, I_M, I_G, I_F0, I_F, NB_INPUTS };
		static const char* VARIABLE_NAME[NB_INPUTS] = { "Year", "Month","Day","sex","A", "M", "G", "F°", "F" };


		bool bMissing = false;
		std::array<size_t, NB_INPUTS> varsPos;
		for (size_t i = 0; i < NB_INPUTS; i++)
		{
			varsPos[i] = Find(vars, VARIABLE_NAME[i]);
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
				//if (msg)
				//world.m_GEO2WATER = GetReProjection(PRJ_WGS_84, world.m_water_DS.GetPrjID());
			}
			else if (world.m_moths_param.m_maxFlights > 1)
			{
				msg.ajoute("maximum flights is more than 1 but there is no water map. Reset maximum flights to 1 or provide water map.");
			}

			callback.StepIt();
		}

		callback.PopTask();


		CTPeriod period = world.m_world_param.m_simulationPeriod;
		ASSERT(period.GetTM().Type() == CTM::DAILY);
		if (!period.IsIntersect(world.m_weather.GetEntireTPeriod()))
		{
			msg.ajoute("Weather period doesn't intersect simulation period");
		}

		if (!msg)
			return msg;



		const CLocationVector& locations = metadata.GetLocations();
		callback.PushTask("Init dispersal moths", metadata.GetNbReplications() *locations.size()*metadata.GetParameterSet().size());

		CGeoExtents extents = world.m_DEM_DS.GetExtents();
		extents.Reproject(GetReProjection(world.m_DEM_DS.GetPrjID(), PRJ_WGS_84));
		
		//period.Transform(CTM(CTM::DAILY));

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
						std::array<double, NB_INPUTS> v;

						for (size_t i = 0; i < varsPos.size(); i++)
							v[i] = section[t][varsPos[i]][MEAN];

						CTRef readyToFly = CTRef(int(v[I_YEAR]), size_t(v[I_MONTH]) - 1, size_t(v[I_DAY]) - 1);
						int shift = world.m_moths_param.m_ready_to_fly_shift[v[I_SEX]];
						if (period.IsInside(readyToFly + shift) )
						{
							CSBWMoth moth(world);

							moth.m_loc = l;
							moth.m_par = p;
							moth.m_rep = rr;
							moth.m_readyToFly = readyToFly;//daily reference of ready moths
							moth.m_scale = 1;
							moth.m_sex = v[I_SEX];//sex (MALE=0, FEMALE=1)
							moth.m_A = v[I_A];
							moth.m_M = v[I_M];
							moth.m_G = v[I_G];
							moth.m_Fᵒ = v[I_F0];
							moth.m_broods = 0;
							moth.m_eggsLeft = v[I_F];
							moth.m_location = locations[l];
							moth.m_newLocation = locations[l];
							moth.m_pt = locations[l];

							if (extents.IsInside(moth.m_pt))
								world.m_moths.push_back(moth);
							else
								callback.AddMessage("WARNING: Simulation point outside elevation map");

							rr++;
							nbReplications = max(nbReplications, rr);

						}//is inside simulation period
					}//for all rows

					msg += callback.StepIt();
				}//for all replications


			}//for all paramterset
		}//for all locations


		callback.PopTask();

		//CTPeriod outputPeriod = world.get_moths_period();
		CTPeriod outputPeriod = world.m_world_param.m_simulationPeriod;
		outputPeriod.End()++;//add one day at the end
		outputPeriod.Transform(CTM::HOURLY);


		callback.AddMessage("Execute dispersal with " + ToString(world.m_moths.size()) + " moths");
		callback.AddMessage("Output period: " + outputPeriod.GetFormatedString());
		callback.AddMessage("Output replications (max moths per location):" + ToString(nbReplications));



		metadata.SetNbReplications(nbReplications);
		metadata.SetTPeriod(outputPeriod);
		//ASSERT(world.m_moths.size() <= metadata.GetNbReplications());

		CATMOutputMatrix output(locations.size());
		for (size_t l = 0; l < output.size(); l++)
		{
			output[l].resize(metadata.GetParameterSet().size());//the number of input variables
			for (size_t p = 0; p < output[l].size(); p++)
			{
				output[l][p].resize(nbReplications);
				for (size_t r = 0; r < output[l][p].size(); r++)
				{
					output[l][p][r].Init(outputPeriod, VMISS);
				}
			}
		}

		if (!world.m_moths.empty())
		{ 
			msg = world.Execute(output, output_file, callback);
			if (msg)
			{
				for (size_t l = 0; l < output.size() && msg; l++)
				{
					for (size_t p = 0; p < output[l].size() && msg; p++)
					{
						for (size_t r = 0; r < output[l][p].size(); r++)
						{
							size_t no = result.GetSectionNo(l, p, r);
							msg += result.SetSection(no, output[l][p][r]);
							msg += callback.StepIt(0);
						}
					}
				}


				if (m_parameters.m_world.m_bCreateEggMaps)
				{
					string outputFilePath = fileManager.GetOutputMapPath() + m_parameters.m_world.m_eggMapsTitle + ".tif";
					msg = world.CreateEggDepositionMap(outputFilePath, output, callback);
				}
			}
		}


		result.Close();

		return msg;
	}


	ERMsg CDispersal::Execute2(const CFileManager& fileManager, CCallback& callback)
	{
		ERMsg msg;

		GIntBig test = GDALGetCacheMax64();
		GDALSetCacheMax64(128 * 1024 * 1024);


		CATMWorld world;
		world.m_world_param = m_parameters.m_world;
		world.m_moths_param = m_parameters.m_moths;
		world.m_nb_max_threads = CTRL.m_nbMaxThreads;
		ofStream output_file;

		string DEM_filepath, gribs_filepath, hourly_DB_filepath, defoliation_filepath, host_filepath, distraction_filepath, water_filepath;
		string outputPath = GetPath(fileManager);		//Generate output path
		string DBFilePath = GetDBFilePath(outputPath);		//Generate DB file path
		string outputFilePath = !m_parameters.m_world.m_outputFileTitle.empty() && m_parameters.m_world.m_bOutputSubHourly ? fileManager.GetOutputPath() + m_parameters.m_world.m_outputFileTitle + ".csv" : "";

		if (!m_parameters.m_world.m_simulationPeriod.IsInside(m_parameters.m_world.m_flightPeriod))
			msg.ajoute("Flight period (" + m_parameters.m_world.m_flightPeriod.GetFormatedString() + ") must be inside simulation period (" + m_parameters.m_world.m_simulationPeriod.GetFormatedString() + ")");

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

		if ((m_parameters.m_world.UseHourlyDB() || m_parameters.m_world.m_PSource == CATMWorldParamters::PRCP_WEATHER_STATION ))
		{
			if (!m_parameters.m_world.m_hourly_DB_name.empty())
				msg += fileManager.Hourly().GetFilePath(m_parameters.m_world.m_hourly_DB_name, hourly_DB_filepath);
			else
				msg.ajoute("Hourly database is not defined");
		}

		if (!outputFilePath.empty())
			msg += output_file.open(outputFilePath);




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

		enum TInput { I_YEAR, I_MONTH, I_DAY, I_SEX, I_A, I_M, I_G, I_F0, I_F, NB_INPUTS };
		static const char* VARIABLE_NAME[NB_INPUTS] = { "Year", "Month","Day","sex","A", "M", "G", "F°", "F" };


		bool bMissing = false;
		std::array<size_t, NB_INPUTS> varsPos;
		for (size_t i = 0; i < NB_INPUTS; i++)
		{
			varsPos[i] = Find(vars, VARIABLE_NAME[i]);
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
				//if (msg)
				//world.m_GEO2WATER = GetReProjection(PRJ_WGS_84, world.m_water_DS.GetPrjID());
			}
			else if (world.m_moths_param.m_maxFlights > 1)
			{
				msg.ajoute("maximum flights is more than 1 but there is no water map. Reset maximum flights to 1 or provide water map.");
			}

			callback.StepIt();
		}

		callback.PopTask();

		if (!msg)
			return msg;



		const CLocationVector& locations = metadata.GetLocations();
		callback.PushTask("Init dispersal moths", metadata.GetNbReplications() *locations.size()*metadata.GetParameterSet().size());

		CGeoExtents extents = world.m_DEM_DS.GetExtents();
		extents.Reproject(GetReProjection(world.m_DEM_DS.GetPrjID(), PRJ_WGS_84));
		CTPeriod period = world.m_world_param.m_simulationPeriod;
		ASSERT(period.GetTM().Type() == CTM::DAILY);
		//period.Transform(CTM(CTM::DAILY));

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
						std::array<double, NB_INPUTS> v;

						for (size_t i = 0; i < varsPos.size(); i++)
							v[i] = section[t][varsPos[i]][MEAN];

						CTRef TRef = CTRef(int(v[I_YEAR]), size_t(v[I_MONTH]) - 1, size_t(v[I_DAY]) - 1/*, size_t(v[I_HOUR])*/);
						if (period.IsInside(TRef))
						{
							CSBWMoth moth(world);

							moth.m_loc = l;
							moth.m_par = p;
							moth.m_rep = rr;
							moth.m_readyToFly = TRef;//daily reference of ready moths
							moth.m_scale = 1;
							moth.m_sex = v[I_SEX];//sex (MALE=0, FEMALE=1)
							moth.m_A = v[I_A];
							moth.m_M = v[I_M];
							moth.m_G = v[I_G];
							moth.m_Fᵒ = v[I_F0];
							moth.m_broods = 0;// v[I_B];
							moth.m_eggsLeft = v[I_F];// v[I_E];
							moth.m_location = locations[l];
							moth.m_newLocation = locations[l];
							moth.m_pt = locations[l];

							//moth.m_pt.m_alt = 10;
							if (extents.IsInside(moth.m_pt))
								world.m_moths.push_back(moth);
							else
								callback.AddMessage("WARNING: Simulation point outside elevation map");

							rr++;
							nbReplications = max(nbReplications, rr);

						}//is inside simulation period
					}//for all rows

					msg += callback.StepIt();
				}//for all replications


			}//for all paramterset
		}//for all locations


		callback.PopTask();

		if (!world.m_moths.empty())
		{

			CTPeriod outputPeriod = world.m_world_param.m_simulationPeriod;
			outputPeriod.End()++;//add one day at the end
			outputPeriod.Transform(CTM::HOURLY);


			callback.AddMessage("Execute dispersal with " + ToString(world.m_moths.size()) + " moths");
			callback.AddMessage("Output period: " + outputPeriod.GetFormatedString());
			callback.AddMessage("Output replications (max moths per location):" + ToString(nbReplications));
			callback.PushTask("Execute dispersal for year = " + ToString(period.Begin().GetYear()) + " (" + ToString(period.GetNbDay()) + " days)", period.GetNbDay());


			metadata.SetNbReplications(nbReplications);
			metadata.SetTPeriod(outputPeriod);

			world.Init(callback);


			const int nbSubPerHour = 3600 / world.m_world_param.m_outputFrequency;


			//open 10 min file
			CATMOutputMatrix sub_output;
			if (output_file.is_open())
			{
				//write file header

				output_file << "l,p,r,Year,Month,Day,Hour,Minute,Second,";
				output_file << "flight,scale,sex,A,M,G,EggsLaid,state,x,y,lat,lon,";
				output_file << "T,P,U,V,W,";
				output_file << "MeanHeight,CurrentHeight,DeltaHeight,HorizontalSpeed,VerticalSpeed,Direction,Distance,DistanceFromOrigine,Defoliation" << endl;
			}

			//for all days
			for (CTRef TRef = period.Begin(); TRef <= period.End() && msg; TRef++)
			{
				//get all ready to flight flyers for this day
				vector<CSBWMothsIt> ready_to_fly = world.GetFlyers(TRef);

				if (!ready_to_fly.empty())
				{
					//get sunset hours for this day
					CTimePeriod UTC_period = world.get_UTC_sunset_period(TRef, ready_to_fly);

					//load hours around sunset
					UTC_period.first -= 4 * 3600;
					UTC_period.second += 4 * 3600;

					vector<__int64> weather_time = world.GetWeatherTime(UTC_period, callback);

					//Load weather for sunset
					if (!weather_time.empty())
						msg = world.LoadWeather(TRef, weather_time, callback);
						

					if (msg)
					{

						//allocate memory
						CTPeriod p(CTRef(TRef.GetYear(), TRef.GetMonth(), TRef.GetDay(), 12), CTRef(TRef.GetYear(), TRef.GetMonth(), TRef.GetDay() + 1, 12));

						CATMOutputMatrix output(locations.size());
						for (size_t l = 0; l < output.size(); l++)
						{
							output[l].resize(metadata.GetParameterSet().size());//the number of input variables
							for (size_t p = 0; p < output[l].size(); p++)
							{
								output[l][p].resize(nbReplications);
								for (size_t r = 0; r < output[l][p].size(); r++)
								{
									output[l][p][r].Init(p, VMISS);
								}
							}
						}

						if (msg && !sub_output.empty())
						{
							ASSERT(p.size()*nbSubPerHour < LONG_MAX);
							__int64 begin = CTimeZones::TRef2Time(p.Begin()) / world.m_world_param.m_outputFrequency;
							__int64 end = CTimeZones::TRef2Time(p.End()) / world.m_world_param.m_outputFrequency;

							CTPeriod outputPeriod(CTRef(begin, 0, 0, 0, CTM::ATEMPORAL), CTRef(end, 0, 0, 0, CTM::ATEMPORAL));
							sub_output.resize(output.size());
							for (size_t l = 0; l < sub_output.size(); l++)
							{
								sub_output[l].resize(output[l].size());//the number of input variables
								for (size_t p = 0; p < output[l].size(); p++)
								{
									sub_output[l][p].resize(output[l][p].size());
									for (size_t r = 0; r < sub_output[l][p].size(); r++)
									{
										sub_output[l][p][r].Init(outputPeriod, VMISS);
									}
								}
							}
						}

						vector<CSBWMothsIt> flyers;
						vector<CSBWMothsIt> nonflyers;
						//init all moths : broods and liffoff time
						for (size_t i = 0; i < ready_to_fly.size(); i++)
						{
							if (ready_to_fly[i]->init(TRef))
								flyers.push_back(ready_to_fly[i]);
							else
								nonflyers.push_back(ready_to_fly[i]);
						}

						callback.AddMessage("Dispersal for " + TRef.GetFormatedString("%Y-%m-%d"));
						callback.AddMessage("Flyers = " + to_string(flyers.size()), 1);
						callback.AddMessage("Non flyers = " + to_string(nonflyers.size()), 1);


						if (!flyers.empty())
						{
							msg = world.Execute(TRef, flyers, output, sub_output, callback);
							if (msg && !sub_output.empty())
							{
								//save sub-hourly output
								for (size_t l = 0; l < sub_output.size(); l++)
								{
									sub_output[l].resize(output[l].size());
									for (size_t p = 0; p < output[l].size(); p++)
									{
										sub_output[l][p].resize(output[l][p].size());
										for (size_t r = 0; r < sub_output[l][p].size(); r++)
										{
											for (size_t t = 0; t < sub_output[l][p][r].size(); t++)
											{
												size_t seconds = 0;// (t * m_world_param.m_outputFrequency) % (24 * 3600);
												size_t hours = size_t(t / nbSubPerHour);
												size_t minutes = (t % nbSubPerHour) * (world.m_world_param.m_outputFrequency / 60);
												ASSERT(seconds % 60 == 0);

												output_file << l + 1 << "," << p + 1 << "," << r + 1 << ",";
												output_file << TRef.GetYear() << "," << TRef.GetMonth() + 1 << "," << TRef.GetDay() + 1 << "," << hours << "," << minutes << "," << seconds - 60 * minutes;
												for (size_t v = 0; v < NB_ATM_OUTPUT; v++)
													output_file << "," << sub_output[l][p][r][t][v];
												output_file << endl;
											}//for all time step
										}//for all replications
									}//for all parameters
								}//for all locations
							}//if sub hourly output
						}//if flyers

						if (!nonflyers.empty())
						{
							CTRef TRef18 = TRef.as(CTM::HOURLY);
							TRef18.m_hour = 18;


							//only update output for eggs laid
							for (size_t i = 0; i < nonflyers.size(); i++)
							{
								CSBWMoth& flyer = *(nonflyers[i]);

								CTRef UTCRef = CTimeZones::LocalTRef2UTCTRef(TRef18, flyer.m_location);
								__int64 UTCTime = CTimeZones::TRef2Time(UTCRef);

								flyer.FillOutput(TRef18, output);
								flyer.live(UTCTime);
								flyer.FillOutput(TRef18++, output);
							}
						}//have non flyers


						//save result
						for (size_t l = 0; l < output.size() && msg; l++)
						{
							for (size_t p = 0; p < output[l].size() && msg; p++)
							{
								for (size_t r = 0; r < output[l][p].size(); r++)
								{
									size_t no = result.GetSectionNo(l, p, r);
									msg += result.SetSection(no, output[l][p][r]);
									msg += callback.StepIt(0);
								}
							}
						}

					}//if msg
				}//have ready to flight moths

				msg += callback.StepIt();
			}//for all valid days


			callback.PopTask();



			if (m_parameters.m_world.m_bCreateEggMaps)
			{
				string outputFilePath = fileManager.GetOutputMapPath() + m_parameters.m_world.m_eggMapsTitle + ".tif";
				//a faire...
			//	msg = world.CreateEggDepositionMap(outputFilePath, output, callback);
			}
		}//if have moths to simulate




		result.Close();

		return msg;
	}



}
