//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
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

	const char* CDispersalParamters::MEMBERS_NAME[NB_MEMBERS] = { "World", "ATM" };


	//*******************************************************************************
	const char* CDispersal::XML_FLAG = "Dispersal";
	const char* CDispersal::MEMBERS_NAME[NB_MEMBERS_EX] = { "Parameters" };
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
		outputVar.push_back(CModelOutputVariableDef("Sex", "Sex", "m=0|f=1", "Sex of flyer", CTM(CTM::ATEMPORAL), 0));
		outputVar.push_back(CModelOutputVariableDef("A", "A", "cm²", "Forewing surface area", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("M", "M", "g", "Dry weight", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("G", "G", "", "Gravidity", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("EggsLaid", "EggsLaid", "", "EggsLaid", CTM(CTM::ATEMPORAL), 5));
		outputVar.push_back(CModelOutputVariableDef("State", "State", "", "State of flyer", CTM(CTM::ATEMPORAL), 0));
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
		double distance = sqrt(U*U + V*V);//distance in km
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
		GDALSetCacheMax64(128*1024*1024);
		

		//char test[100] = { 0 };
		//CPLGetConfigOption("GDAL_CACHEMAX", test);
		//CPLSetConfigOption("GDAL_CACHEMAX", "64");//limit the size of the cache for each layers


		CATMWorld world;
		world.m_parameters1 = m_parameters.m_world;
		world.m_parameters2 = m_parameters.m_ATM;
		world.m_nb_max_threads = CTRL.m_nbMaxThreads;
		ofStream output_file;

		string DEM_filepath, gribs_filepath, hourly_DB_filepath, defoliation_filepath, host_filepath, distraction_filepath, water_filepath;
		string outputPath = GetPath(fileManager);		//Generate output path
		string DBFilePath = GetDBFilePath(outputPath);		//Generate DB file path
		string outputFilePath = !m_parameters.m_world.m_outputFileTitle.empty() && m_parameters.m_world.m_bOutputSubHourly ? fileManager.GetOutputPath() + m_parameters.m_world.m_outputFileTitle + ".csv" : "";


		if (!m_parameters.m_world.m_DEM_name.empty())
			msg += fileManager.MapInput().GetFilePath(m_parameters.m_world.m_DEM_name, DEM_filepath);
		else
			msg.ajoute("A DEM must be supply");

		if (!m_parameters.m_world.m_defoliation_name.empty())
			msg += fileManager.MapInput().GetFilePath(m_parameters.m_world.m_defoliation_name, defoliation_filepath);
//		if (!m_parameters.m_world.m_host_name.empty())
	//		msg += fileManager.MapInput().GetFilePath(m_parameters.m_world.m_host_name, host_filepath);
		//if (!m_parameters.m_world.m_distraction_name.empty())
			//msg += fileManager.MapInput().GetFilePath(m_parameters.m_world.m_distraction_name, distraction_filepath);
		if (!m_parameters.m_world.m_water_name.empty())
			msg += fileManager.MapInput().GetFilePath(m_parameters.m_world.m_water_name, water_filepath);
		if (m_parameters.m_world.UseGribs())
		{
			if (!m_parameters.m_world.m_gribs_name.empty())
				msg += fileManager.Gribs().GetFilePath(m_parameters.m_world.m_gribs_name, gribs_filepath);
			else
				msg.ajoute("Gribs file is not defined");
		}
			
		if ((m_parameters.m_world.UseHourlyDB() || m_parameters.m_ATM.m_PSource == CATMParameters::PRCP_WEATHER_STATION || m_parameters.m_ATM.m_broodTSource == CATMParameters::BROOD_T_WEATHER_STATION))
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

		//if (result.GetMetadata().GetNbReplications() > 1)
			//callback.AddMessage("WARNING: only the first replication will be taken");

		
		//init output info
		CDBMetadata& metadata = result.GetMetadata();
		GetInputDBInfo(pResult, metadata);
		
		const CModelOutputVariableDefVector& vars = pResult->GetMetadata().GetOutputDefinition();
		
		
		enum TInput { I_YEAR, I_MONTH, I_DAY, I_HOUR, I_MINUTE, I_SECOND, I_SEX, I_A, I_M, I_G, I_F0, I_F, I_B, I_E, NB_INPUTS };
		static const char* VARIABLE_NAME[NB_INPUTS] = { "Year", "Month","Day","Hour","Minute","Second","sex","A", "M", "G", "F°", "F", "Broods", "Eggs" };

		bool bMissing = false;
		std::array<size_t, NB_INPUTS> varsPos;
		for (size_t i = 0; i < NB_INPUTS; i++)
		{
			varsPos[i] = Find(vars, VARIABLE_NAME[i]);
			bMissing = bMissing || varsPos[i] == NOT_INIT;
		}
			

		if (bMissing)
		{
			msg.ajoute("Invalid dispersal variables input. Variable \"Year\", \"Month\", \"Day\",\"Hour\",\"Minute\", \"Second\", \"Sex\", \"A\", \"M\", \"G\", \"F°\", \"F\", \"Broods\", \"Eggs\" must be defined");
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
			else if (world.m_parameters1.m_maxFliyers > 1)
			{
				msg.ajoute("maximum flyers is more than 1 but there is no defoliation map. Reset maximum flyers to 1 or provide defoliation map.");
			}

			callback.StepIt();
		}

		//if (!host_filepath.empty())
			//msg += world.m_host_DS.OpenInputImage(host_filepath);

		//callback.StepIt();
		//if (!distraction_filepath.empty())
			//msg += world.m_distraction_DS.OpenInputImage(distraction_filepath);

		//callback.StepIt();

				
		if (msg)
		{
			if (!water_filepath.empty())
			{
				msg += world.m_water_DS.OpenInputImage(water_filepath);
				//if (msg)
				//world.m_GEO2WATER = GetReProjection(PRJ_WGS_84, world.m_water_DS.GetPrjID());
			}
			else if (world.m_parameters1.m_maxFliyers > 1)
			{
				msg.ajoute("maximum flyers is more than 1 but there is no water map. Reset maximum flyers to 1 or provide water map.");
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
		CTPeriod period = world.m_parameters1.m_simulationPeriod;
		period.Transform(CTM(CTM::HOURLY));
		
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
						
						CTRef TRef = CTRef(int(v[I_YEAR]), size_t(v[I_MONTH]) - 1, size_t(v[I_DAY]) - 1, size_t(v[I_HOUR]));
						if (period.IsInside(TRef))
						{
							CFlyer flyer(world);

							flyer.m_loc = l;
							flyer.m_par = p;
							flyer.m_rep = rr;
							flyer.m_localTRef = TRef;//assume daylignt time
							flyer.m_liftoffOffset = v[I_MINUTE] * 60 + v[I_SECOND];
							flyer.m_scale = 1;
							flyer.m_sex = v[I_SEX];//sex (MALE=0, FEMALE=1)
							flyer.m_A = v[I_A];
							flyer.m_M = v[I_M];
							flyer.m_G = v[I_G];
							flyer.m_Fᵒ = v[I_F0];
							flyer.m_broods = v[I_B];
							flyer.m_eggsLeft = v[I_E];
							flyer.m_location = locations[l];
							flyer.m_newLocation = locations[l];
							flyer.m_pt = locations[l];

							//flyer.m_pt.m_alt = 10;
							if (extents.IsInside(flyer.m_pt))
								world.m_flyers.push_back(flyer);
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

		CTPeriod outputPeriod = world.get_period(false);
		callback.AddMessage("Execute dispersal with " + ToString(world.m_flyers.size()) + " moths");
		callback.AddMessage("Output period: " + outputPeriod.GetFormatedString());
		callback.AddMessage("Output replications (max moths per location):" + ToString(nbReplications));
		


		metadata.SetNbReplications(nbReplications);
		//ASSERT(world.m_flyers.size() <= metadata.GetNbReplications());

		CATMOutputMatrix output(locations.size());
		for (size_t l = 0; l < output.size(); l++)
		{
			output[l].resize(metadata.GetParameterSet().size());//the number of input variables
			for (size_t p = 0; p < output[l].size(); p++)
			{
				output[l][p].resize(nbReplications);
				for (size_t r = 0; r < output[l][p].size(); r++)
				{
					output[l][p][r].Init(outputPeriod.GetNbRef(), outputPeriod.Begin(), VMISS);
				}
			}
		}

		if (!world.m_flyers.empty())
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

	//ERMsg CDispersal::Execute2(const CFileManager& fileManager, CCallback& callback)
	//{
	//	ERMsg msg;

	//	GIntBig test = GDALGetCacheMax64();
	//	GDALSetCacheMax64(128 * 1024 * 1024);


	//	//char test[100] = { 0 };
	//	//CPLGetConfigOption("GDAL_CACHEMAX", test);
	//	//CPLSetConfigOption("GDAL_CACHEMAX", "64");//limit the size of the cache for each layers


	//	CATMWorld world;
	//	world.m_parameters1 = m_parameters.m_world;
	//	world.m_parameters2 = m_parameters.m_ATM;
	//	world.m_nb_max_threads = CTRL.m_nbMaxThreads;
	//	ofStream output_file;

	//	//host_filepath, distraction_filepath,
	//	string DEM_filepath, gribs_filepath, hourly_DB_filepath, defoliation_filepath, water_filepath;
	//	string outputPath = GetPath(fileManager);		//Generate output path
	//	string DBFilePath = GetDBFilePath(outputPath);		//Generate DB file path
	//	string outputFilePath = !m_parameters.m_world.m_outputFileTitle.empty() && m_parameters.m_world.m_bOutputSubHourly ? fileManager.GetOutputPath() + m_parameters.m_world.m_outputFileTitle + ".csv" : "";


	//	if (!m_parameters.m_world.m_DEM_name.empty())
	//		msg += fileManager.MapInput().GetFilePath(m_parameters.m_world.m_DEM_name, DEM_filepath);
	//	else
	//		msg.ajoute("A DEM must be supply");

	//	if (!m_parameters.m_world.m_defoliation_name.empty())
	//		msg += fileManager.MapInput().GetFilePath(m_parameters.m_world.m_defoliation_name, defoliation_filepath);
	//	if (!m_parameters.m_world.m_host_name.empty())
	//		msg += fileManager.MapInput().GetFilePath(m_parameters.m_world.m_host_name, host_filepath);
	//	if (!m_parameters.m_world.m_distraction_name.empty())
	//		msg += fileManager.MapInput().GetFilePath(m_parameters.m_world.m_distraction_name, distraction_filepath);
	//	if (!m_parameters.m_world.m_water_name.empty())
	//		msg += fileManager.MapInput().GetFilePath(m_parameters.m_world.m_water_name, water_filepath);
	//	if (m_parameters.m_world.UseGribs())
	//	{
	//		if (!m_parameters.m_world.m_gribs_name.empty())
	//			msg += fileManager.Gribs().GetFilePath(m_parameters.m_world.m_gribs_name, gribs_filepath);
	//		else
	//			msg.ajoute("Gribs file is not defined");
	//	}

	//	if ((m_parameters.m_world.UseHourlyDB() || m_parameters.m_ATM.m_PSource == CATMParameters::PRCP_WEATHER_STATION || m_parameters.m_ATM.m_broodTSource == CATMParameters::BROOD_T_WEATHER_STATION))
	//	{
	//		if (!m_parameters.m_world.m_hourly_DB_name.empty())
	//			msg += fileManager.Hourly().GetFilePath(m_parameters.m_world.m_hourly_DB_name, hourly_DB_filepath);
	//		else
	//			msg.ajoute("Hourly database is not defined");
	//	}

	//	if (!outputFilePath.empty())
	//		msg += output_file.open(outputFilePath);




	//	CResultPtr pResult = m_pParent->GetResult(fileManager);
	//	msg += pResult->Open();

	//	//open outputDB
	//	CResult result;
	//	msg += result.Open(DBFilePath, std::fstream::binary | std::fstream::out | std::fstream::trunc);

	//	if (!msg)
	//		return msg;

	//	//if (result.GetMetadata().GetNbReplications() > 1)
	//	//callback.AddMessage("WARNING: only the first replication will be taken");


	//	//init output info
	//	CDBMetadata& metadata = result.GetMetadata();
	//	GetInputDBInfo(pResult, metadata);

	//	const CModelOutputVariableDefVector& vars = pResult->GetMetadata().GetOutputDefinition();


	//	enum TInput { I_YEAR, I_MONTH, I_DAY, I_HOUR, I_MINUTE, I_SECOND, I_SEX, I_A, I_M, I_G, I_F0, I_F, I_B, I_E, NB_INPUTS };
	//	static const char* VARIABLE_NAME[NB_INPUTS] = { "Year", "Month", "Day", "Hour", "Minute", "Second", "sex", "A", "M", "G", "F°", "F", "Broods", "Eggs" };

	//	bool bMissing = false;
	//	std::array<size_t, NB_INPUTS> varsPos;
	//	for (size_t i = 0; i < NB_INPUTS; i++)
	//	{
	//		varsPos[i] = Find(vars, VARIABLE_NAME[i]);
	//		bMissing = bMissing || varsPos[i] == NOT_INIT;
	//	}


	//	if (bMissing)
	//	{
	//		msg.ajoute("Invalid dispersal variables input. Variable \"Year\", \"Month\", \"Day\",\"Hour\",\"Minute\", \"Second\", \"Sex\", \"A\", \"M\", \"G\", \"F°\", \"F\", \"Broods\", \"Eggs\" must be defined");
	//		return msg;
	//	}

	//	callback.PushTask("Open Dispersal's Input", 6);


	//	if (msg)
	//	{
	//		msg += world.m_DEM_DS.OpenInputImage(DEM_filepath);
	//		//if (msg)
	//		//{
	//		//world.m_GEO2DEM = GetReProjection(PRJ_WGS_84, world.m_DEM_DS.GetPrjID());
	//		//world.m_extents = world.m_DEM_DS.GetExtents();
	//		//}

	//	}


	//	if (!msg)
	//		return msg;

	//	//Create projection

	//	msg += world.m_weather.Load(gribs_filepath, hourly_DB_filepath, callback);
	//	//if (msg)
	//	//world.m_GEO2GRIBS = GetReProjection(PRJ_WGS_84, world.m_weather.GetGribsPrjID());


	//	callback.StepIt();
	//	if (!defoliation_filepath.empty())
	//	{
	//		msg += world.m_defoliation_DS.OpenInputImage(defoliation_filepath);
	//		//if (msg)
	//		//world.m_GEO2DEFOLIATION = GetReProjection(PRJ_WGS_84, world.m_defoliation_DS.GetPrjID());
	//	}
	//	else if (world.m_parameters1.m_maxFliyers > 1)
	//	{
	//		msg.ajoute("maximum flyers is more than 1 but there is no defoliation map. Reset maximum flyers to 1 or provide defoliation map.");
	//	}



	//	callback.StepIt();
	//	if (!host_filepath.empty())
	//		msg += world.m_host_DS.OpenInputImage(host_filepath);

	//	callback.StepIt();
	//	if (!distraction_filepath.empty())
	//		msg += world.m_distraction_DS.OpenInputImage(distraction_filepath);

	//	callback.StepIt();



	//	if (!water_filepath.empty())
	//	{
	//		msg += world.m_water_DS.OpenInputImage(water_filepath);
	//		//if (msg)
	//		//world.m_GEO2WATER = GetReProjection(PRJ_WGS_84, world.m_water_DS.GetPrjID());
	//	}
	//	else if (world.m_parameters1.m_maxFliyers > 1)
	//	{
	//		msg.ajoute("maximum flyers is more than 1 but there is no water map. Reset maximum flyers to 1 or provide water map.");
	//	}

	//	callback.StepIt();
	//	if (!msg)
	//		return msg;

	//	callback.PopTask();

	//	const CLocationVector& locations = metadata.GetLocations();
	//	callback.PushTask("Init dispersal moths", metadata.GetNbReplications() *locations.size()*metadata.GetParameterSet().size());

	//	CGeoExtents extents = world.m_DEM_DS.GetExtents();
	//	extents.Reproject(GetReProjection(world.m_DEM_DS.GetPrjID(), PRJ_WGS_84));
	//	CTPeriod period = world.m_parameters1.m_simulationPeriod;
	//	period.Transform(CTM(CTM::HOURLY));

	//	size_t nbReplications = 0;
	//	for (size_t l = 0; l < locations.size() && msg; l++)
	//	{
	//		for (size_t p = 0; p < pResult->GetMetadata().GetParameterSet().size() && msg; p++)
	//		{
	//			size_t rr = 0;
	//			for (size_t r = 0; r < pResult->GetMetadata().GetNbReplications() && msg; r++)
	//			{

	//				CNewSectionData section;
	//				pResult->GetSection(l, p, r, section);
	//				assert(section.GetCols() == pResult->GetNbCols(false));

	//				for (size_t t = 0; t < section.GetRows() && msg; t++)
	//				{
	//					std::array<double, NB_INPUTS> v;

	//					for (size_t i = 0; i < varsPos.size(); i++)
	//						v[i] = section[t][varsPos[i]][MEAN];

	//					CTRef TRef = CTRef(int(v[I_YEAR]), size_t(v[I_MONTH]) - 1, size_t(v[I_DAY]) - 1, size_t(v[I_HOUR]));
	//					if (period.IsInside(TRef))
	//					{
	//						CFlyer flyer(world);

	//						flyer.m_loc = l;
	//						flyer.m_par = p;
	//						flyer.m_rep = rr;
	//						flyer.m_localTRef = TRef;//assume daylignt time
	//						flyer.m_liftoffOffset = v[I_MINUTE] * 60 + v[I_SECOND];
	//						flyer.m_scale = 1;
	//						flyer.m_sex = v[I_SEX];//sex (MALE=0, FEMALE=1)
	//						flyer.m_A = v[I_A];
	//						flyer.m_M = v[I_M];
	//						flyer.m_G = v[I_G];
	//						flyer.m_Fᵒ = v[I_F0];
	//						flyer.m_broods = v[I_B];
	//						flyer.m_eggsLeft = v[I_E];
	//						flyer.m_location = locations[l];
	//						flyer.m_newLocation = locations[l];
	//						flyer.m_pt = locations[l];

	//						flyer.m_pt.m_alt = 10;
	//						if (extents.IsInside(flyer.m_pt))
	//							world.m_flyers.push_back(flyer);
	//						else
	//							callback.AddMessage("WARNING: Simulation point outside elevation map");

	//						rr++;
	//						nbReplications = max(nbReplications, rr);

	//					}//is inside simulation period
	//				}//for all rows

	//				msg += callback.StepIt();
	//			}//for all replications


	//		}//for all paramterset
	//	}//for all locations


	//	callback.PopTask();

	//	
	//	if (!world.m_flyers.empty())
	//	{
	//		CTPeriod outputPeriod = world.get_period(false);
	//		callback.AddMessage("Execute dispersal with " + ToString(world.m_flyers.size()) + " moths");
	//		callback.AddMessage("Output period: " + outputPeriod.GetFormatedString());
	//		callback.AddMessage("Output replications (max moths per location):" + ToString(nbReplications));

	//		metadata.SetNbReplications(nbReplications);

	//		

	//		//init weather
	//		set<int> years = world.get_years();

	//		for (set<int>::const_iterator it = years.begin(); it != years.end() && msg; it++)
	//		{
	//			int year = *it;
	//			
	//			//get all days to simulate
	//			set<CTRef> TRefs = world.get_TRefs(year);

	//			callback.PushTask("Execute Dispersal for year = " + ToString(year) + " (" + ToString(TRefs.size()) + " days)", TRefs.size());
	//			//simulate for all days
	//			for (set<CTRef>::const_iterator it = TRefs.begin(); it != TRefs.end() && msg; it++)
	//			{
	//				CATMOutputMatrix output(locations.size());
	//				for (size_t l = 0; l < output.size(); l++)
	//				{
	//					output[l].resize(metadata.GetParameterSet().size());//the number of input variables
	//					for (size_t p = 0; p < output[l].size(); p++)
	//					{
	//						output[l][p].resize(nbReplications);
	//						for (size_t r = 0; r < output[l][p].size(); r++)
	//						{
	//							output[l][p][r].Init(outputPeriod.GetNbRef(), outputPeriod.Begin(), VMISS);
	//						}
	//					}
	//				}
	//				msg = world.Execute(output, output_file, callback);
	//				if (msg)
	//				{
	//					for (size_t l = 0; l < output.size() && msg; l++)
	//					{
	//						for (size_t p = 0; p < output[l].size() && msg; p++)
	//						{
	//							for (size_t r = 0; r < output[l][p].size(); r++)
	//							{
	//								size_t no = result.GetSectionNo(l, p, r);
	//								msg += result.SetSection(no, output[l][p][r]);
	//								msg += callback.StepIt(0);
	//							}
	//						}
	//					}


	//					if (m_parameters.m_world.m_bCreateEggMaps)
	//					{
	//						string outputFilePath = fileManager.GetOutputMapPath() + m_parameters.m_world.m_eggMapsTitle + ".tif";
	//						msg = world.CreateEggDepositionMap(outputFilePath, output, callback);
	//					}
	//				}
	//			}
	//		}
	//	}


	//	result.Close();

	//	return msg;
	//}
	//
	//
	//ERMsg CMothFlight::XValidation(CCallback& callback)
	//{
	//	ERMsg msg;
	//
	//	callback.AddMessage(GetString(IDS_CREATE_DATABASE));
	//	callback.AddMessage(GetAbsoluteFilePath(m_outputFilePath), 1);
	//
	//	CHourlyDatabase DB;
	//	msg += DB.Open(GetAbsoluteFilePath(m_hourly_DB_name));
	//
	//	CLocationVector observations;
	//	msg += observations.Load(GetAbsoluteFilePath(m_locationFilePath));
	//
	//	CGDALDatasetEx DEM;
	//	msg += DEM.OpenInputImage(GetAbsoluteFilePath(m_DEM_name));
	//
	//
	//	ofStream outputFile;
	//	msg += outputFile.open(GetAbsoluteFilePath(m_outputFilePath));
	//
	//
	//	if (!msg)
	//		return msg;
	//
	//
	//	outputFile << "DateTime,Name,ID,Latitude,Longitude,Elevation,Uobs,Usim,Vobs,Vsim,WsObs,WsSim,WdObs,WdSim" << endl;
	//
	//	CWVariables filter("WS WD");
	//	set<CTRef> datesTmp;
	//	std::set<CLocation, location_compare > locations;
	//
	//	for (CLocationVector::iterator it = observations.begin(); it != observations.end() && msg; it++)
	//	{
	//		CTRef TRef;
	//		TRef.FromFormatedString(it->GetSSI("FlightActivity"), -1, "-/ ", 1);
	//		datesTmp.insert(TRef);
	//		locations.insert(CLocation(it->m_name, it->m_ID, it->m_lat, it->m_lon, it->m_elev));
	//	}
	//
	//	vector<CTRef> dates(datesTmp.begin(), datesTmp.end());
	//
	//	if (!msg)
	//		return msg;
	//
	//
	//
	//	ASSERT(!dates.empty());
	//	ASSERT(dates.begin()->GetYear() == dates.rbegin()->GetYear());
	//	int year = dates.begin()->GetYear();
	//
	//
	//	callback.AddMessage("Number of observations: " + to_string(observations.size()));
	//	callback.AddMessage("Number of locations: " + to_string(locations.size()));
	//	callback.AddMessage("Number of flight dates: " + to_string(dates.size()));
	//
	//	CSearchResultVector resultVector;
	//	msg = DB.Search(resultVector, *locations.begin(), 500, filter, year);
	//
	//	if (!msg)
	//		return msg;
	//
	//	callback.AddMessage("A total of " + to_string(resultVector.size()) + " weather stations will be used");
	//
	//	callback.SetCurrentDescription("Extract wind informations for period...");
	//	callback.SetNbStep(resultVector.size());
	//
	//	map<size_t, CWeatherStation> stations;
	//	//For all station
	//	for (int i = 0; i<resultVector.size() && msg; i++)
	//	{
	//		size_t index = resultVector[i].m_index;
	//		msg += DB.Get(stations[index], index, year);
	//		msg += callback.StepIt();
	//	}//for all stations
	//
	//	if (!msg)
	//		return msg;
	//
	//	callback.SetCurrentDescription("Compute wind speed XValidation...");
	//	callback.SetNbStep(stations.size()*dates.size()*m_flightDurationMax);
	//
	//	CRandomGenerator random;
	//	random.Randomize();
	//
	//	for (map<size_t, CWeatherStation>::const_iterator it = stations.begin(); it != stations.end(); it++)
	//	{
	//		const CWeatherStation& station = it->second;
	//
	//		for (size_t d = 0; d < dates.size(); d++)
	//		{
	//			CTRef TRef = dates[d];
	//			size_t flightDuration = m_flightDurationMax;
	//			double flightHeight = m_flightHeightSD == 0 ? m_flightHeightMean : max(10.0, random.RandNormal(m_flightHeightMean, m_flightHeightSD));
	//			int windStability = m_windStability;
	//
	//			for (size_t h = 0; h < flightDuration && msg; h++)
	//			{
	//				CTRef hTRef = CTRef(TRef.GetYear(), TRef.GetMonth(), TRef.GetDay(), m_firstFlightHour) + h;
	//
	//				CStatistic wsObs = station[hTRef].GetData(HOURLY_DATA::H_WNDS);
	//				CStatistic wdObs = station[hTRef].GetData(HOURLY_DATA::H_WNDD);
	//
	//				if (wsObs.IsInit() && wdObs.IsInit())
	//				{
	//					double θ = Deg2Rad(90 - wdObs[MEAN]);
	//					double Uobs = cos(θ)*wsObs[MEAN];
	//					double Vobs = sin(θ)*wsObs[MEAN];
	//					double WsObs = sqrt(Uobs*Uobs + Vobs*Vobs);
	//
	//					CGridPointVectorPtr ptsU(new CGridPointVector); ptsU->m_bGeographic = true;
	//					CGridPointVectorPtr ptsV(new CGridPointVector); ptsV->m_bGeographic = true;
	//
	//
	//					//CSearchResultVector result;
	//					//msg = DB.Search(result, station, m_nbWeatherStations*3, filter, year);
	//					//for (size_t ss = 0; ss < result.size() && ptsU->size()<m_nbWeatherStations; ss++)
	//					for (map<size_t, CWeatherStation>::const_iterator it2 = stations.begin(); it2 != stations.end(); it2++)
	//					{
	//						if (it2->first != it->first)
	//						{
	//							const CWeatherStation& tmp = it2->second;
	//							//size_t index = result[ss].m_index;
	//							CStatistic wndSpd = tmp[hTRef].GetData(HOURLY_DATA::H_WNDS);
	//							CStatistic wndDir = tmp[hTRef].GetData(HOURLY_DATA::H_WNDD);
	//
	//							if (wndSpd.IsInit() && wndDir.IsInit())
	//							{
	//								double ws = wndSpd[MEAN];
	//								double wd = wndDir[MEAN];
	//								ASSERT(ws >= 0 && ws < 150);
	//								ASSERT(wd >= 0 && wd <= 360);
	//
	//								double θ = Deg2Rad(90 - wd);
	//								double U = cos(θ)*ws;
	//								double V = sin(θ)*ws;
	//
	//								ptsU->push_back(CGridPoint(tmp.m_x, tmp.m_y, 10, 0, 0, U, tmp.m_lat, tmp.GetPrjID()));
	//								ptsV->push_back(CGridPoint(tmp.m_x, tmp.m_y, 10, 0, 0, V, tmp.m_lat, tmp.GetPrjID()));
	//							}
	//						}
	//					}
	//
	//					CGridInterpolParam param;
	//					param.m_IWDModel = CGridInterpolParam::IWD_CLASIC;
	//					param.m_power = IWD_POWER;
	//					param.m_nbPoints = m_nbWeatherStations;
	//					param.m_bGlobalLimit = false;
	//					param.m_bGlobalLimitToBound = false;
	//					param.m_maxDistance = 1000000;
	//					param.m_bUseElevation = false;
	//					param.m_noData = -999;
	//
	//					CIWD iwdU;
	//					CIWD iwdV;
	//
	//					iwdU.SetDataset(ptsU); iwdU.SetParam(param);
	//					iwdV.SetDataset(ptsV); iwdV.SetParam(param);
	//
	//					msg += iwdU.Initialization();
	//					msg += iwdV.Initialization();
	//
	//
	//
	//					//CGridInterpolParamVector parametersetU;
	//					//CGridInterpolParamVector parametersetV;
	//					//CGridInterpolParamVector parametersetWs;
	//					//iwdU.GetParamterset(parametersetU);
	//					//iwdV.GetParamterset(parametersetV);
	//					//iwdWs.GetParamterset(parametersetWs);
	//					////iwdU.SetParam(parametersetU[0]);
	//					////iwdV.SetParam(parametersetV[0]);
	//					////iwdWs.SetParam(parametersetWs[0]);
	//
	//					//vector<pair<double, size_t>> UR²(parametersetU.size());
	//					//for (size_t u = 0; u < parametersetU.size(); u++)
	//					//{
	//					//	iwdU.SetParam(parametersetU[u]);
	//					//	UR²[u] = make_pair(iwdU.GetOptimizedR²(), u);
	//					//}
	//
	//					//vector<pair<double, size_t>> VR²(parametersetV.size());
	//					//for (size_t v = 0; v < parametersetV.size(); v++)
	//					//{
	//					//	iwdV.SetParam(parametersetV[v]);
	//					//	VR²[v] = make_pair(iwdV.GetOptimizedR²(), v);
	//					//}
	//
	//					//vector<pair<double, size_t>> WsR²(parametersetWs.size());
	//					//for (size_t w = 0; w < parametersetWs.size(); w++)
	//					//{
	//					//	iwdWs.SetParam(parametersetWs[w]);
	//					//	WsR²[w] = make_pair(iwdWs.GetOptimizedR²(), w);
	//					//}
	//					//sort(UR².begin(), UR².end());
	//					//sort(VR².begin(), VR².end());
	//					//sort(WsR².begin(), WsR².end());
	//
	//					//iwdU.SetParam(parametersetU[UR².back().second]);
	//					//iwdV.SetParam(parametersetV[VR².back().second]);
	//					//iwdWs.SetParam(parametersetWs[WsR².back().second]);
	//
	//					//
	//					//msg += iwdU.Initialization();
	//					//msg += iwdV.Initialization();
	//					//msg += iwdWs.Initialization();
	//
	//					//Um += parametersetU[UR².back().second].m_IWDModel;
	//					//Up += parametersetU[UR².back().second].m_power;
	//					//Vm += parametersetV[VR².back().second].m_IWDModel;
	//					//Vp += parametersetV[VR².back().second].m_power;
	//					//Wm += parametersetWs[WsR².back().second].m_IWDModel;
	//					//Wp += parametersetWs[WsR².back().second].m_power;
	//
	//					//callback.AddMessage("U: " + iwdU.GetFeedbackBestParam() + "\t"+ ToString(UR².back().first,3));
	//					//callback.AddMessage("V: " + iwdV.GetFeedbackBestParam() + "\t" + ToString(VR².back().first, 3));
	//					//callback.AddMessage("Ws: " + iwdWs.GetFeedbackBestParam() + "\t" + ToString(WsR².back().first, 3));
	//					//iwdU.GetOptimizedR²();
	//					//iwdV.GetOptimizedR²();
	//					//iwdWs.GetOptimizedR²();
	//					/*callback.AddMessage("U:\t" + ToString(iwdU.GetOptimizedR²(), 3));
	//					callback.AddMessage("V:\t" + ToString(iwdV.GetOptimizedR²(), 3));
	//					callback.AddMessage("W:\t" + ToString(iwdWs.GetOptimizedR²(), 3));
	//					*/
	//
	//					CGridPoint pt(station.m_x, station.m_y, 10, 0, 0, 0, station.m_lat, station.GetPrjID());
	//
	//					double U = iwdU.Evaluate(pt);	//wind speed U component [km/h]
	//					double V = iwdV.Evaluate(pt);	//wind speed V component [km/h]
	//
	//					double ws = sqrt(U*U + V*V);
	//					double wd = fmod(5 * PI / 2 - atan2(V, U), 2 * PI) / PI * 180;
	//
	//					outputFile << hTRef.GetFormatedString();
	//					for (int m = 0; m < 5; m++)
	//						outputFile << "," + station.GetMember(m);
	//
	//
	//					outputFile << "," << Uobs << "," << ToString(U, 1) << "," << Vobs << "," << ToString(V, 1);
	//					outputFile << "," << WsObs << "," << ToString(ws, 1) << "," << wdObs[MEAN] << "," << ToString(wd, 1);
	//					outputFile << endl;
	//
	//					CGridInterpolBase::FreeMemoryCache();
	//				}//if have observation
	//				msg += callback.StepIt();
	//			}//for all hour
	//		}//for all dates
	//	}//for all weather station
	//
	//	outputFile.close();
	//
	//	return msg;
	//}
	//
	//



}
