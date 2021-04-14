//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//******************************************************************************
#include "stdafx.h"

#include "Basic/ModelStat.h"
#include "Basic/FrequencyTable.h"
#include "Basic/NormalsDatabase.h"
#include "Basic/DailyDatabase.h"
#include "Basic/HourlyDatabase.h"
#include "Basic/Shore.h"
#include "Basic/WeatherDatabaseCreator.h"
#include "FileManager/FileManager.h"
#include "Simulation/AdvancedNormalStation.h"
#include "Simulation/WGInputAnalysis.h"
#include "Simulation/ExecutableFactory.h"
#include "Simulation/WeatherGeneration.h"
#include "Simulation/WeatherGenerator.h"
//#include "Simulation/WeatherGradient.h"
#include "WeatherBasedSimulationString.h"

using namespace std;
using namespace WBSF::NORMALS_DATA;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::DIMENSION;

namespace WBSF
{

	static const int NB_DAY_MIN_MONTHLY = 25;
	static const int NB_DAY_MIN_ANNUAL = 340;
	//**********************************************************************
	//CWGInputAnalysis

	//LOWEST, HIGHEST, , STAT_R² 
	const size_t CWGInputAnalysis::STATISTICS[S_NB_STAT] = { NB_VALUE, MEAN_Y, MEAN_X, BIAS, MAE, RMSE, STAT_R² };
	const char* CWGInputAnalysis::XML_FLAG = "WGInputAnalysis";
	const char* CWGInputAnalysis::MEMBERS_NAME[NB_MEMBERS_EX] = { "Kind", "ExportMatch", "MatchName" };
	const int CWGInputAnalysis::CLASS_NUMBER = CExecutableFactory::RegisterClass(CWGInputAnalysis::GetXMLFlag(), &CWGInputAnalysis::CreateObject);

	static const char* CAT_ID[4] = { "T", "P", "H", "W" };
	static const StringVector CAT_TITLE(IDS_SIM_CATEGORY_NAME, ";|");


	CWeatherDatabase& GetObsDB(CWeatherGenerator& WG) { return WG.GetWGInput().IsHourly() ? (CWeatherDatabase&)(*WG.GetHourlyDB()) : (CWeatherDatabase&)(*WG.GetDailyDB()); }

	ERMsg GetSimulation(CWeatherGenerator& WG, CWeatherStation& simStation, CCallback& callback)
	{
		ERMsg msg;
		if (WG.GetWGInput().IsHourly())
			msg = WG.GetHourly(simStation, callback);
		else
			msg = WG.GetDaily(simStation, callback);

		return msg;
	}


	CStatistic GetStat(size_t s, const CStatisticXY& station)
	{
		

		CStatistic stat;
		if (s == CWGInputAnalysis::S_MEAN_OBS)
			stat = station.Y();
		else if (s == CWGInputAnalysis::S_MEAN_SIM)
			stat = station.X();
		else
			stat = station[CWGInputAnalysis::STATISTICS[s]];

		return stat;

	}


	CWGInputAnalysis::CWGInputAnalysis()
	{
		Reset();
	}

	CWGInputAnalysis::~CWGInputAnalysis()
	{}

	void CWGInputAnalysis::Reset()
	{
		CExecutable::Reset();
		m_kind = MATCH_STATION_NORMALS;
		m_bExportMatch = false;
		m_matchName = "MatchStations.csv";
		m_name = "WGInputAnalysis";
	}

	CWGInputAnalysis::CWGInputAnalysis(const CWGInputAnalysis& in)
	{
		operator=(in);
	}


	CWGInputAnalysis& CWGInputAnalysis::operator =(const CWGInputAnalysis& in)
	{
		if (&in != this)
		{
			CExecutable::operator =(in);
			m_kind = in.m_kind;
			m_bExportMatch = in.m_bExportMatch;
			m_matchName = in.m_matchName;
		}

		ASSERT(*this == in);
		return *this;
	}

	bool CWGInputAnalysis::operator == (const CWGInputAnalysis& in)const
	{
		bool bEqual = true;

		if (CExecutable::operator !=(in))bEqual = false;
		if (m_kind != in.m_kind)bEqual = false;
		if (m_bExportMatch != in.m_bExportMatch)bEqual = false;
		if (m_matchName != in.m_matchName)bEqual = false;

		return bEqual;
	}

	ERMsg CWGInputAnalysis::GetParentInfo(const CFileManager& fileManager, CParentInfo& info, CParentInfoFilter filter)const
	{
		ERMsg msg;

		ASSERT(m_pParent);
		const CWeatherGeneration& parent = dynamic_cast<const CWeatherGeneration&>(*m_pParent);
		CTPeriod completenessPeriod;
		CLocationVector completenessLocation;
		//same as weather generator variables
		if (m_kind == DB_COMPLETENESS &&
			(filter[LOCATION] || filter[TIME_REF]))
		{
			CWGInput WGInput;
			msg = parent.GetWGInput(fileManager, WGInput);

			//find daily file apth if any

			if (msg && WGInput.UseDaily())
			{
				std::string filePath;
				msg = fileManager.Daily().GetFilePath(WGInput.m_dailyDBName, filePath);

				CDailyDatabase DB1;
				if (msg)
					msg = DB1.Open(filePath);

				if (msg)
				{
					//CSearchResultVector result;
					//DB1.GetStationList(result);
					completenessLocation.resize(DB1.size());
					for (size_t i = 0; i < DB1.size(); i++)
						completenessLocation[i] = DB1[i];

					const set<int>& years = DB1.GetYears();
					if (!years.empty())
						completenessPeriod = CTPeriod(*years.begin(), *years.rbegin());


					DB1.Close();
				}

			}

			if (msg && WGInput.UseHourly())
			{
				std::string filePath;
				msg = fileManager.Hourly().GetFilePath(WGInput.m_hourlyDBName, filePath);
				CHourlyDatabase DB1;
				if (msg)
					msg = DB1.Open(filePath);

				if (msg)
				{
					/*CSearchResultVector result;
					DB1.GetStationList(result);
					completenessLocation.resize(result.size());
					for (size_t i = 0; i < result.size(); i++)
						completenessLocation[i] = result[i].m_location;*/
					completenessLocation.resize(DB1.size());
					for (size_t i = 0; i < DB1.size(); i++)
						completenessLocation[i] = DB1[i];


					const set<int>& years = DB1.GetYears();
					if (!years.empty())
						completenessPeriod = CTPeriod(*years.begin(), *years.rbegin());

					DB1.Close();
				}

			}

		}

		if (filter[LOCATION])
		{
			if (m_kind == DB_COMPLETENESS)
				info.m_locations = completenessLocation;
			else
				msg = m_pParent->GetParentInfo(fileManager, info, LOCATION);

		}

		if (filter[PARAMETER])
		{

			info.m_parameterset.clear();



			CWGInput WGInput;
			msg += parent.GetWGInput(fileManager, WGInput);

			if (m_kind == MATCH_STATION_NORMALS)
			{
				//get standardized normal variables

				bitset<NB_CATEGORIES> categories = GetCategories(WGInput.m_variables);
				for (size_t c = 0; c < NB_CATEGORIES; c++)
				{
					if (categories[c])
					{
						CModelInput modelInput;

						modelInput.SetName(CAT_ID[c]);
						modelInput.push_back(CModelInputParam(CAT_ID[c], CAT_TITLE[c]));
						info.m_parameterset.push_back(modelInput);
					}
				}

				info.m_parameterset.m_variation.push_back(CParameterVariation("Category", true, CModelInputParameterDef::kMVString));
			}
			else if (m_kind == GRADIENTS_INPUTS)
			{
				//get standardized normal variables
				for (size_t g = 0; g < GRADIENT::NB_GRADIENT; g++)
				{
					CModelInput modelInput;
					static const char* NAME[4] = { "Tmin","Tmax","Prcp","Tdew"};

					string name = NAME[g];
					modelInput.SetName(name);
					modelInput.push_back(CModelInputParam(name, name));
					info.m_parameterset.push_back(modelInput);
				}
				
				info.m_parameterset.m_variation.push_back(CParameterVariation("Gradient", true, CModelInputParameterDef::kMVString));
			}
			else if (m_kind == KERNEL_VALIDATION || m_kind == XVALIDATION_NORMALS || m_kind == ESTIMATE_ERROR_NORMALS)
			{
				//get standardized normal variables
				bitset<NB_CATEGORIES> categories = GetCategories(WGInput.m_variables);
				CWVariables variables = GetCategoryVariables(categories);

				//for all variable in the category
				for (size_t f = 0; f < NORMALS_DATA::NB_FIELDS; f++)
				{
					if (variables[NORMALS_DATA::F2V(f)])
					{
						CModelInput modelInput;
						modelInput.SetName(NORMALS_DATA::GetFieldHeader(f));
						modelInput.push_back(CModelInputParam("Field", NORMALS_DATA::GetFieldHeader(f)));
						info.m_parameterset.push_back(modelInput);
					}
				}

				info.m_parameterset.m_variation.push_back(CParameterVariation("Field", true, CModelInputParameterDef::kMVString));

			}
			else if (m_kind == ESTIMATE_ERROR_OBSERVATIONS || m_kind == XVALIDATION_OBSERVATIONS ||
				m_kind == MATCH_STATION_OBSERVATIONS)
			{
				//for all variable in the categories
				for (size_t v = 0; v < NB_VAR_H; v++)
				{
					if (WGInput.m_variables[v])
					{
						CModelInput modelInput;
						modelInput.SetName(GetVariableName(v));
						modelInput.push_back(CModelInputParam("Variable", GetVariableName(v)));
						info.m_parameterset.push_back(modelInput);
					}
				}
				info.m_parameterset.m_variation.push_back(CParameterVariation("Variable", true, CModelInputParameterDef::kMVString));
			}
			else if (m_kind == LAST_OBSERVATION || m_kind == EXTRACT_NORMALS || m_kind == MISSING_OBSERVATIONS || m_kind == DB_COMPLETENESS)
			{
				info.m_parameterset.push_back(CModelInput());//no input parameters
			}

		}

		if (filter[REPLICATION])
		{
			if (m_kind == MATCH_STATION_NORMALS || m_kind == MATCH_STATION_OBSERVATIONS)
			{
				CWGInput WGInput;
				msg += parent.GetWGInput(fileManager, WGInput);

				if (m_kind == MATCH_STATION_NORMALS)
					info.m_nbReplications = WGInput.m_nbNormalsStations;
				else if (m_kind == MATCH_STATION_OBSERVATIONS)
					info.m_nbReplications = WGInput.IsHourly() ? WGInput.m_nbHourlyStations : WGInput.m_nbDailyStations;
			}
			else if (m_kind == GRADIENTS_INPUTS)
			{
				info.m_nbReplications = CWeatherGradient::NB_STATION_REGRESSION_LOCAL;
			}
			else
			{
				info.m_nbReplications = 1;
			}

		}

		if (filter[TIME_REF])
		{
			if (m_kind == EXTRACT_NORMALS)
			{
				CTM TM(CTM::MONTHLY, CTM::OVERALL_YEARS);
				info.m_period = CTPeriod(CTRef(YEAR_NOT_INIT, JANUARY, 0, 0, TM), CTRef(YEAR_NOT_INIT, DECEMBER, 0, 0, TM));
			}
			else if (m_kind == GRADIENTS_INPUTS)
			{
				CTM TM(CTM::ANNUAL, CTM::OVERALL_YEARS);
				info.m_period = CTPeriod(CTRef(YEAR_NOT_INIT, 0, 0, 0, TM), CTRef(YEAR_NOT_INIT, 0, 0, 0, TM));
			}
			else if (m_kind == LAST_OBSERVATION || m_kind == MATCH_STATION_OBSERVATIONS || m_kind == MISSING_OBSERVATIONS)
			{
				msg += m_pParent->GetParentInfo(fileManager, info, TIME_REF);

				CTM TM(CTM::ANNUAL);
				info.m_period.Transform(TM);
			}
			else if (m_kind == DB_COMPLETENESS)
			{
				info.m_period = completenessPeriod;
			}
			else
			{
				CTM TM(CTM::ANNUAL, CTM::OVERALL_YEARS);
				info.m_period = CTPeriod(CTRef(YEAR_NOT_INIT, 0, 0, 0, TM), CTRef(YEAR_NOT_INIT, 0, 0, 0, TM));
			}
		}

		if (filter[VARIABLE])
		{
			info.m_variables.clear();

			if (m_kind == MATCH_STATION_NORMALS || m_kind == MATCH_STATION_OBSERVATIONS)
			{
				info.m_variables.push_back(CModelOutputVariableDef("Station number", "Station number", "", "Station number"));
				info.m_variables.push_back(CModelOutputVariableDef("Latitude", "Latitude", "°", "Latitude"));
				info.m_variables.push_back(CModelOutputVariableDef("Longitude", "Longitude", "°", "Longitude"));
				info.m_variables.push_back(CModelOutputVariableDef("Elevation", "Elevation", "m", "Elevation"));
				info.m_variables.push_back(CModelOutputVariableDef("ShoreDistance", "ShoreDistance", "km", "Approximate distance of Shore"));
				info.m_variables.push_back(CModelOutputVariableDef("Distance", "Distance", "km", "Strait distance"));
				info.m_variables.push_back(CModelOutputVariableDef("DeltaElev", "DeltaElev", "m", "Difference of elevation"));
				info.m_variables.push_back(CModelOutputVariableDef("DeltaShore", "DeltaShore", "m", "Difference of shore distance"));
				info.m_variables.push_back(CModelOutputVariableDef("Virtual Distance", "Virtual Distance", "km", "Distance including delta elevation and delta shore"));
				info.m_variables.push_back(CModelOutputVariableDef("Weight", "Weight", "%", "Theorical Weigth"));
			}
			else if (m_kind == GRADIENTS_INPUTS)
			{
				info.m_variables.push_back(CModelOutputVariableDef("X", "X", "", "Delta longitude"));
				info.m_variables.push_back(CModelOutputVariableDef("Y", "Y", "", "Delta latitude"));
				info.m_variables.push_back(CModelOutputVariableDef("Z", "Z", "", "Delta elevation"));
				info.m_variables.push_back(CModelOutputVariableDef("S", "S", "", "Delta shore"));
				for(size_t m=0; m<12; m++)
					info.m_variables.push_back(CModelOutputVariableDef(WBSF::GetMonthName(m), WBSF::GetMonthName(m), "", WBSF::GetMonthName(m)));
			}
			else if (m_kind == ESTIMATE_ERROR_NORMALS || m_kind == ESTIMATE_ERROR_OBSERVATIONS || m_kind == KERNEL_VALIDATION || m_kind == XVALIDATION_NORMALS || m_kind == XVALIDATION_OBSERVATIONS)
			{
				for (size_t s = 0; s < S_NB_STAT; s++)
				{
					std::string name = CStatistic::GetName((int)STATISTICS[s]);
					std::string title = CStatistic::GetTitle((int)STATISTICS[s]);
					info.m_variables.push_back(CModelOutputVariableDef(name, title, "", ""));
				}
			}
			else if (m_kind == EXTRACT_NORMALS)
			{
				for (size_t f = 0; f < NB_FIELDS; f++)
				{
					std::string name = GetFieldHeader(f);
					std::string title = GetFieldTitle(f);
					const char* UNITS[NB_FIELDS] = { "°C", "°C", "", "", "", "", "", "", "", "mm", "", "°C", "%", "", "km/h", "" };
					std::string units = UNITS[f];	//GetFieldUnits(v);
					std::string description = "";	//todo
					info.m_variables.push_back(CModelOutputVariableDef(name, title, units, description));
				}
			}
			else if (m_kind == LAST_OBSERVATION || m_kind == MISSING_OBSERVATIONS || m_kind == DB_COMPLETENESS)
			{
				//same as weather generator variables
				msg += m_pParent->GetParentInfo(fileManager, info, VARIABLE);
			}
		}
		//}

		return msg;
	}


	ERMsg CWGInputAnalysis::Execute(const CFileManager& fileManager, CCallback& callback)
	{
		ERMsg msg;

		//	callback.PushLevel();

		ASSERT(m_pParent);
		ASSERT(dynamic_cast<const CWeatherGeneration*>(m_pParent) != NULL);
		const CWeatherGeneration& parent = dynamic_cast<const CWeatherGeneration&>(*m_pParent);

		CResult resultDB;
		msg = resultDB.Open(GetDBFilePath(GetPath(fileManager)), std::fstream::out | std::fstream::binary);
		if (msg)
		{
			callback.AddMessage(GetString(IDS_WG_PROCESS_INPUT_ANALYSIS));
			callback.AddMessage(resultDB.GetFilePath(), 1);


			CParentInfo info;
			msg = GetParentInfo(fileManager, info);
			if (msg)
			{
				CDBMetadata& metadata = resultDB.GetMetadata();
				metadata.SetLocations(info.m_locations);
				metadata.SetParameterSet(info.m_parameterset);
				metadata.SetNbReplications(info.m_nbReplications);
				metadata.SetTPeriod(info.m_period);
				metadata.SetOutputDefinition(info.m_variables);

				callback.AddMessage(FormatMsg(IDS_SIM_CREATE_DATABASE, m_name));
				callback.AddMessage(resultDB.GetFilePath(), 1);


				switch (m_kind)
				{
				case LAST_OBSERVATION:				msg = LastObservation(fileManager, resultDB, callback); break;
				case MATCH_STATION_NORMALS:
				case MATCH_STATION_OBSERVATIONS:	msg = MatchStation(fileManager, resultDB, callback); break;
				case ESTIMATE_ERROR_NORMALS:		msg = NormalError(fileManager, resultDB, callback); break;
				case ESTIMATE_ERROR_OBSERVATIONS:	msg = ObservationsError(fileManager, resultDB, callback); break;
				case XVALIDATION_NORMALS:			msg = XValidationNormal(fileManager, resultDB, callback); break;
				case XVALIDATION_OBSERVATIONS:		msg = XValidationObservations(fileManager, resultDB, callback); break;
				case KERNEL_VALIDATION:				msg = KernelValidation(fileManager, resultDB, callback); break;
				case EXTRACT_NORMALS:				msg = ExtractNormal(fileManager, resultDB, callback); break;
				case GRADIENTS_INPUTS:				msg = ExtractGradientInputs(fileManager, resultDB, callback); break;
				case MISSING_OBSERVATIONS:			msg = GetNbMissingObservations(fileManager, resultDB, callback); break;
				case DB_COMPLETENESS:				msg = GetCompleteness(fileManager, resultDB, callback); break;
				default: ASSERT(false);
				}

			}

			resultDB.Close();
		}

		//callback.PopLevel();

		return msg;
	}

	//*******************************************************************************
	//*******************************************************************************
	ERMsg CWGInputAnalysis::MatchStation(const CFileManager& fileManager, CResult& resultDB, CCallback& callback)
	{
		ERMsg msg;


		CWGInput WGInput;
		msg = GetWGInput(fileManager, WGInput);


		if (msg && m_kind == MATCH_STATION_OBSERVATIONS && WGInput.IsNormals())
			msg.ajoute("No observations from normals weather generation");


		CWeatherGenerator WG;
		if (msg)
			msg = InitDefaultWG(fileManager, WG, callback);


		ofStream oFile;
		if (msg && m_bExportMatch)
		{
			string filePath = fileManager.GetOutputPath() + m_matchName;
			if (GetFileExtension(m_matchName).empty())
				filePath += ".csv";
			msg += oFile.open(filePath);
			if (msg)
			{
				oFile << "LocNo,LocID,LocName,LocLat,LocLon,LocElev,LocShore,Variable,";
				if (m_kind == MATCH_STATION_OBSERVATIONS)
					oFile << "Year,";

				oFile << "MatchNo,StationID,StationName,Latitude,Longitude,Elevation,ShoreDistance,Distance,DeltaElev,DeltaShore,VirtualDistance,Weight" << endl;
			}
		}


		if (!msg)
			return msg;


		const CLocationVector& locations = resultDB.GetMetadata().GetLocations();


		string message;
		switch (m_kind)
		{
		case MATCH_STATION_NORMALS:			message = "Match Normals Weather Stations"; break;
		case MATCH_STATION_OBSERVATIONS:	message = "Match Observed Weather Stations"; break;
		default: ASSERT(false);
		}


		CWVariables variables = WG.GetWGInput().m_variables;

		callback.PushTask(message, WG.GetWGInput().GetNbYears()*variables.count()*locations.size());
		//callback.SetNbStep(WG.GetWGInput().GetNbYears()*variables.count()*locations.size());


		for (size_t l = 0; l < locations.size() && msg; l++)
		{
			if (m_kind == MATCH_STATION_NORMALS)
			{
				static const TVarH VARIABLE_FOR_CATEGORY[4] = { H_TAIR, H_PRCP, H_RELH, H_WNDS };
				bitset<NB_CATEGORIES> categories = GetCategories(WGInput.m_variables);

				//static const char* CAT_ID[4] = { "T","P","H","W" };
				for (size_t c = 0; c < NB_CATEGORIES; c++)
				{
					if (categories[c])
					{
						TVarH v = GetCategoryLeadVariable(c);

						CTM TM(CTM::ANNUAL, CTM::OVERALL_YEARS);
						size_t nbStations = WG.GetWGInput().m_nbNormalsStations;


						CSearchResultVector searchResultArray;
						msg += WG.GetNormalDB()->Search(searchResultArray, locations[l], WG.GetWGInput().GetNbNormalsToSearch(), WGInput.m_searchRadius[v], VARIABLE_FOR_CATEGORY[c], -999, true, true, WGInput.m_bUseShore);
						if (!searchResultArray.empty() && WG.GetWGInput().XVal())
							searchResultArray.erase(searchResultArray.begin());

						//remove error if the variables can be derived
						if (!msg && WG.GetWGInput().m_allowedDerivedVariables[VARIABLE_FOR_CATEGORY[c]])
							msg = ERMsg();

						if (msg)
						{
							//sort(searchResultArray.begin(), searchResultArray.end(), CSearchResultSort());
							vector<double> weight = searchResultArray.GetStationWeight(/*true, WEATHER::SHORE_DISTANCE_FACTOR>0*/);
							for (size_t j = 0; j < searchResultArray.size() && msg; j++)
							{
								CNewSectionData section(1, 10, CTRef(YEAR_NOT_INIT, 0, 0, 0, TM));
								section[0][0] = searchResultArray[j].m_index + 1;//index in base one
								section[0][1] = searchResultArray[j].m_location.m_lat;
								section[0][2] = searchResultArray[j].m_location.m_lon;
								section[0][3] = searchResultArray[j].m_location.m_elev;
								section[0][4] = searchResultArray[j].m_location.GetShoreDistance() / 1000;
								section[0][5] = locations[l].GetDistance(searchResultArray[j].m_location, false, false) / 1000;
								section[0][6] = searchResultArray[j].m_deltaElev;
								section[0][7] = (locations[l].GetShoreDistance() - searchResultArray[j].m_location.GetShoreDistance()) / 1000;
								section[0][8] = searchResultArray[j].m_distance / 1000;
								section[0][9] = weight[j] * 100;

								msg += resultDB.AddSection(section, callback);

								if (oFile.is_open())
								{
									string line;
									line += FormatA("%d,%s,%s,%.5lf,%.5lf,%.1lf,%.1lf,%s,", l + 1, locations[l].m_ID.c_str(), locations[l].m_name.c_str(), locations[l].m_lat, locations[l].m_lon, locations[l].m_elev, locations[l].GetShoreDistance() / 1000, CAT_ID[c]);
									line += FormatA("%d,%s,%s,%.5lf,%.5lf,%.1lf,%.1lf,%.1lf,%.1lf,%.1lf,%.1lf,%.1lf", j + 1, searchResultArray[j].m_location.m_ID.c_str(), searchResultArray[j].m_location.m_name.c_str(), section[0][1][MEAN], section[0][2][MEAN], section[0][3][MEAN], section[0][4][MEAN], section[0][5][MEAN], section[0][6][MEAN], section[0][7][MEAN], section[0][8][MEAN], section[0][9][MEAN]);


									std::replace(line.begin(), line.end(), ',', CTRL.m_listDelimiter);
									std::replace(line.begin(), line.end(), '.', CTRL.m_decimalDelimiter);

									oFile << line << endl;
								}

								msg += callback.StepIt();
							}
						}
					}
				}

			}
			else
			{
				size_t vv = 0;
				for (TVarH v = H_FIRST_VAR; v < NB_VAR_H&&msg; v++)
				{
					if (variables[v])
					{
						const char* pVarID = GetVariableAbvr(v);
						CWeatherDatabase& obsDB = GetObsDB(WG);
						size_t nbYears = WG.GetWGInput().GetNbYears();
						size_t nbStations = WG.GetWGInput().GetNbObservationToSearch();

						vector<CNewSectionData> section;
						section.insert(section.begin(), nbStations, CNewSectionData(nbYears, 10, CTRef(WG.GetWGInput().GetFirstYear())));

						for (size_t y = 0; y < nbYears&&msg; y++)
						{
							int year = WG.GetWGInput().GetFirstYear() + int(y);

							CSearchResultVector searchResultArray;
							msg += obsDB.Search(searchResultArray, locations[l], nbStations, WGInput.m_searchRadius[v], v, year, true, true, WGInput.m_bUseShore);

							if (!searchResultArray.empty() && WG.GetWGInput().XVal())
								searchResultArray.erase(searchResultArray.begin());

							//remove error if the variables can be derived
							if (!msg&&WG.GetWGInput().m_allowedDerivedVariables[v])
								msg = ERMsg();



							vector<double> weight = searchResultArray.GetStationWeight(/*true, WEATHER::SHORE_DISTANCE_FACTOR>0*/);
							for (size_t r = 0; r < searchResultArray.size(); r++)
							{
								section[r][y][0] = searchResultArray[r].m_index;
								section[r][y][1] = searchResultArray[r].m_location.m_lat;
								section[r][y][2] = searchResultArray[r].m_location.m_lon;
								section[r][y][3] = searchResultArray[r].m_location.m_elev;
								section[r][y][4] = searchResultArray[r].m_location.GetShoreDistance() / 1000;
								section[r][y][5] = locations[l].GetDistance(searchResultArray[r].m_location, false, false) / 1000;
								section[r][y][6] = searchResultArray[r].m_deltaElev;
								section[r][y][7] = (locations[l].GetShoreDistance() - searchResultArray[r].m_location.GetShoreDistance()) / 1000;
								section[r][y][8] = searchResultArray[r].m_distance / 1000;
								section[r][y][9] = weight[r] * 100;

								if (oFile.is_open())
								{
									string line;
									line += FormatA("%d,%s,%s,%.5lf,%.5lf,%.1lf,%.1lf,%s,%d,", l + 1, locations[l].m_ID.c_str(), locations[l].m_name.c_str(), locations[l].m_lat, locations[l].m_lon, locations[l].m_elev, locations[l].GetShoreDistance() / 1000, pVarID, year);
									line += FormatA("%d,%s,%s,%.5lf,%.5lf,%.1lf,%.1lf,%.1lf,%.1lf,%.1lf,%.1lf,%.1lf", r + 1, searchResultArray[r].m_location.m_ID.c_str(), searchResultArray[r].m_location.m_name.c_str(), section[r][y][1][MEAN], section[r][y][2][MEAN], section[r][y][3][MEAN], section[r][y][4][MEAN], section[r][y][5][MEAN], section[r][y][6][MEAN], section[r][y][7][MEAN], section[r][y][8][MEAN], section[r][y][9][MEAN]);


									std::replace(line.begin(), line.end(), ',', CTRL.m_listDelimiter);
									std::replace(line.begin(), line.end(), '.', CTRL.m_decimalDelimiter);

									oFile << line << endl;
								}
							}

							msg += callback.StepIt();
						}//for all years


						for (size_t r = 0; r < section.size(); r++)
						{
							size_t e = resultDB.GetSectionNo(l, vv, r);
							resultDB.SetSection(e, section[r]);
						}

						vv++;
					}//variables used?
				}//for all variables
			}//normals/observations
		}//for all locations

		if (oFile.is_open())
			oFile.close();

		callback.PopTask();

		return msg;
	}

	//*******************************************************************************

	ERMsg CWGInputAnalysis::KernelValidation(const CFileManager& fileManager, CResult& resultDB, CCallback& callback)
	{
		CTM TM(CTM::ANNUAL, CTM::OVERALL_YEARS);

		ERMsg msg;

		CWGInput WGInput;
		msg = GetWGInput(fileManager, WGInput);


		if (msg && !WGInput.IsNormals())
			msg.ajoute("The kernel validation can only be done from disaggregation weather generation");


		CWeatherGenerator WGin;
		if (msg)
			msg = InitDefaultWG(fileManager, WGin, callback);


		if (!msg)
			return msg;


		const CLocationVector& locations = resultDB.GetMetadata().GetLocations();
		

		array < CStatisticXY, NB_FIELDS> overallStat;

		//limit category to basic variable
		bitset<NB_CATEGORIES> categories = GetCategories(WGInput.m_variables);
		CWVariables variables = GetCategoryVariables(categories);

		callback.PushTask("Kernel Validation", categories.count()*locations.size());
		callback.AddMessage("Nb replications = " + ToString(WGin.GetNbReplications()));
		callback.AddMessage("Nb years = " + ToString(WGInput.GetNbYears()));
		if (WGInput.m_bXValidation)
		{
			callback.AddMessage("WARNING: remove nearest station ignored");
			WGInput.m_bXValidation = false;
		}

		if (WGInput.GetNbYears() < 10)
			callback.AddMessage("WARNING: nb years lesser thant 10");


		CStatistic::SetVMiss(VMISS);

		vector<size_t> locPos;
		CWeatherGeneration::GetLocationIndexGrid(locations, locPos);

#pragma omp parallel for schedule(static, 1) shared(resultDB, msg) 
		for (__int64 ll = 0; ll < (__int64)locPos.size(); ll++)
		{
#pragma omp flush(msg)
			if (msg)
			{
				size_t l = locPos[ll];

				//for (size_t l = 0; l < locations.size() && msg; l++)
				//{
				vector< array < array < CStatisticXY, NB_FIELDS>, 12>> stationStat(WGin.GetNbReplications());
				for (size_t c = 0; c < NB_CATEGORIES && msg; c++)//for all categories
				{
					if (categories[c])//if this categories is selected
					{
						CWeatherGenerator WG = WGin;
						CWGInput WGInputTmp(WGInput);
						WGInputTmp.m_variables = GetCategoryVariables(c);
						WG.SetWGInput(WGInputTmp);
						TVarH v = GetCategoryLeadVariable(c);

						//fin stations for this variables
						CSearchResultVector weatherStationsI;
						msg = WG.GetNormalDB()->Search(weatherStationsI, locations[l], 1, WGInputTmp.m_searchRadius[v], WGInputTmp.m_variables, -999, true, true, WGInput.m_bUseShore);

						if (msg && weatherStationsI.front().m_distance < 5000 && weatherStationsI.front().m_deltaElev < 50)
						{
							CNormalsStation obsStation;
							WG.GetNormalDB()->Get(obsStation, weatherStationsI[0].m_index);

							// init the loc part of WGInput
							WG.SetTarget(obsStation);
							msg = WG.Generate(callback);//create data

							if (msg)
							{
								for (size_t r = 0; r < WG.GetNbReplications(); r++)
								{
									CAdvancedNormalStation simStation;
									msg = simStation.FromDaily(WG.GetWeather(r), (int)WG.GetWeather(r).GetNbYears());

									for (size_t f = 0; f < NORMALS_DATA::NB_FIELDS&&msg; f++)
									{
										if (WGInputTmp.m_variables[NORMALS_DATA::F2V(f)])
										{
											for (size_t m = 0; m < 12; m++)
											{
												stationStat[r][m][f].Add(simStation[m][f], obsStation[m][f]);
//#pragma omp critical(SAVE_STATS)
	//											overallStat[f].Add(simStation[m][f], obsStation[m][f]);
											}
										}
									}
								}
							}
						}//for nearest weather stations I
						else
						{
							callback.AddMessage("WARNING: " + locations[l].m_name + " is not eligible for kernel validation for category " + CAT_TITLE[c]);
						}

						msg += callback.StepIt();

					}//if this category is used
				}//for all category	


				//save data 
				for (size_t f = 0, ff = 0; f < NB_FIELDS&&msg; f++)
				{
					if (variables[F2V(f)])
					{
						CNewSectionData section(1, S_NB_STAT, CTRef(YEAR_NOT_INIT, 0, 0, 0, TM));

						CStatisticXY all;
						for (size_t r = 0; r < stationStat.size(); r++)
						{
							for (size_t m = 0; m < 12; m++)
							{
								double obs = stationStat[r][m][f][MEAN_Y];
								double sim = stationStat[r][m][f][MEAN_X];
								overallStat[f].Add(sim, obs);
								all.Add(sim, obs);

								for (size_t s = 0; s < S_NB_STAT; s++)
									section[0][s] += stationStat[r][m][f][STATISTICS[s]];

							}
						}

						//add only once
						section[0][S_STAT_R²] = all[STATISTICS[S_STAT_R²]];
						size_t sectionNo = resultDB.GetSectionNo(l, ff, 0);
						resultDB.SetSection(sectionNo, section);
						ff++;
					}//if selected field
				}
			}//if msg
		}// for all locations

		callback.AddMessage("");
		callback.AddMessage("Overall statistics:");
		callback.AddMessage("Variable\tN\tObserved\tSimulated\tBias\tMAD\tRMSE\tR²");
		for (size_t f = 0; f < NORMALS_DATA::NB_FIELDS&&msg; f++)
		{
			if (variables[NORMALS_DATA::F2V(f)])
			{
				std::string str = std::string(NORMALS_DATA::GetFieldTitle(f));
				//str += "\t" + ToString(overallStat[f][NB_VALUE], 4);
				for (size_t s = 0; s < S_NB_STAT; s++)
					str += "\t" + ToString(overallStat[f][STATISTICS[s]], 4);

				callback.AddMessage(str);
			}
		}

		callback.PopTask();

		return msg;
	}


	//*******************************************************************************


	ERMsg CWGInputAnalysis::XValidationNormal(const CFileManager& fileManager, CResult& resultDB, CCallback& callback)
	{
		CTM TM(CTM::ANNUAL, CTM::OVERALL_YEARS);

		ERMsg msg;

		CWGInput WGInput;
		msg = GetWGInput(fileManager, WGInput);

		if (msg && !WGInput.IsNormals())
			msg.ajoute("The normals cross-validation can only be done from disaggregation weather generation");


		CWeatherGenerator WGin;
		if (msg)
			msg = InitDefaultWG(fileManager, WGin, callback);

		if (!msg)
			return msg;


		const CLocationVector& locations = resultDB.GetMetadata().GetLocations();

		array < CStatisticXY, NORMALS_DATA::NB_FIELDS> overallStat;

		//limit category to basic variable
		bitset<NB_CATEGORIES> categories = GetCategories(WGInput.m_variables);
		CWVariables variables = GetCategoryVariables(categories);

		callback.PushTask("Normals X-Validation", categories.count()*locations.size());
		CStatistic::SetVMiss(VMISS);

		//open search to avoid thread problem
		msg = WGin.GetNormalDB()->OpenSearchOptimization(callback);


		//	for (size_t l = 0; l < locations.size() && msg; l++)
			//{
		vector<size_t> locPos;
		CWeatherGeneration::GetLocationIndexGrid(locations, locPos);

#pragma omp parallel for schedule(static, 1) shared(resultDB, msg) 
		for (__int64 ll = 0; ll < (__int64)locPos.size(); ll++)
		{
#pragma omp flush(msg)
			if (msg)
			{
				size_t l = locPos[ll];
				array < array < CStatisticXY, NORMALS_DATA::NB_FIELDS>, 12> stationStat;

				for (size_t c = 0; c < NB_CATEGORIES && msg; c++)//for all categories
				{
					if (categories[c])//if this category is selected
					{
						CWeatherGenerator WG = WGin;
						CWGInput WGInputTmp(WGInput);
						WGInputTmp.m_variables = GetCategoryVariables(c);
						WGInputTmp.m_bXValidation = true;
						WG.SetWGInput(WGInputTmp);
						TVarH v = GetCategoryLeadVariable(c);

						//find the nearest station for this variable
						CSearchResultVector weatherStationsI;
						msg = WG.GetNormalDB()->Search(weatherStationsI, locations[l], 1, WGInputTmp.m_searchRadius[v], WGInputTmp.m_variables, -999, true, true, WGInput.m_bUseShore);

						//if this stations is nearest 5km and less than 50 meters (delta elevation)
						if (msg && weatherStationsI.front().m_distance < 5000 && weatherStationsI.front().m_deltaElev < 50)
						{
							CNormalsStation obsStation;
							WG.GetNormalDB()->Get(obsStation, weatherStationsI[0].m_index);

							// init the loc part of WGInput
							WG.SetTarget(obsStation);
							msg = WG.Initialize();//create gradient

							if (msg)
							{
								CNormalsStation simStation;
								WG.GetNormals(simStation, callback);


								for (size_t f = 0; f < NORMALS_DATA::NB_FIELDS&&msg; f++)
								{
									if (obsStation.HaveField(f) && simStation.HaveField(f))
									{
										for (size_t m = 0; m < 12; m++)
										{
											stationStat[m][f].Add(simStation[m][f], obsStation[m][f]);
#pragma omp critical(SAVE_STATS)
											overallStat[f].Add(simStation[m][f], obsStation[m][f]);
										}
									}
								}
							}
						}//if it's a weather station and they have data

						msg += callback.StepIt();
#pragma omp flush(msg)
					}//if this category is used
				}//for all category	


				//save data 
				for (size_t f = 0, ff = 0; f < NORMALS_DATA::NB_FIELDS&&msg; f++)
				{
					if (variables[NORMALS_DATA::F2V(f)])
					{
						CNewSectionData section(1, S_NB_STAT, CTRef(YEAR_NOT_INIT, 0, 0, 0, TM));

						CStatisticXY allMonths;
						for (size_t m = 0; m < 12; m++)
						{
							if (stationStat[m][f].GetX().IsInit() && stationStat[m][f].GetY().IsInit())
							{
								double obs = stationStat[m][f][MEAN_Y];
								double sim = stationStat[m][f][MEAN_X];
								allMonths.Add(sim, obs);

								for (size_t s = 0; s < S_NB_STAT; s++)
									section[0][s] += stationStat[m][f][STATISTICS[s]];
							}
						}

						//add only once
						section[0][S_STAT_R²] = allMonths[STATISTICS[S_STAT_R²]];

						size_t sectionNo = resultDB.GetSectionNo(l, ff, 0);
						resultDB.SetSection(sectionNo, section);
						ff++;
						//resultDB.AddSection(section);
					}//if selected field
				}
			}// for all locations

		}//if msg

		callback.AddMessage("");
		callback.AddMessage("Overall statistics:");
		callback.AddMessage("Variable\tN\tObserved\tSimulated\tBias\tMAD\tRMSE\tR²");
		for (size_t f = 0; f < NORMALS_DATA::NB_FIELDS&&msg; f++)
		{
			if (variables[NORMALS_DATA::F2V(f)])
			{
				std::string str = std::string(NORMALS_DATA::GetFieldTitle(f));
				//str += "\t" + ToString(overallStat[f][NB_VALUE], 4);
				for (size_t s = 0; s < S_NB_STAT; s++)
					str += "\t" + ToString(overallStat[f][STATISTICS[s]], 4);

				callback.AddMessage(str);
			}
		}

		return msg;
	}




	ERMsg CWGInputAnalysis::XValidationObservations(const CFileManager& fileManager, CResult& resultDB, CCallback& callback)
	{
		ERMsg msg;

		CWGInput WGInput;
		msg = GetWGInput(fileManager, WGInput);
		
		set<int> years;
		for (int year = WGInput.GetFirstYear(); year <= WGInput.GetLastYear(); year++)
			years.insert(year);


		if (msg && WGInput.IsNormals())
			msg.ajoute("The observation cross-validation can only be done from observation weather generation");

		CWeatherGenerator WGin;
		if (msg)
			msg = InitDefaultWG(fileManager, WGin, callback);

		if (!msg)
			return msg;


		const CLocationVector& locations = resultDB.GetMetadata().GetLocations();
		CWeatherDatabase& obsDB = GetObsDB(WGin);


		CStatisticXY overallStat[NB_VAR_H];

		//limit category to basic variable
		CWVariables variables = WGInput.m_variables;

		callback.PushTask("X-Validation of observation", variables.count()*locations.size());
		CStatistic::SetVMiss(VMISS);

		if (WGInput.m_bXValidation)
		{
			callback.AddMessage("WARNING: remove nearest station ignored");
			WGInput.m_bXValidation = false;
		}


		vector<size_t> locPos;
		CWeatherGeneration::GetLocationIndexGrid(locations, locPos);

#pragma omp parallel for schedule(static, 1) shared(resultDB, msg) 
		for (__int64 ll = 0; ll < (__int64)locPos.size(); ll++)
		{
#pragma omp flush(msg)
			if (msg)
			{
				size_t l = locPos[ll];

				array<CStatisticXY, NB_VAR_H> stationStat;

				for (TVarH v = H_FIRST_VAR; v < NB_VAR_H && msg; v++)//for all categories
				{
					if (variables[v])//if this variable is selected
					{
						//fin stations for this variables
						CSearchResultVector weatherStationsI;
						obsDB.Search(weatherStationsI, locations[l], 1, -1, v, -999, true, true, WGInput.m_bUseShore);

						if (!weatherStationsI.empty() && weatherStationsI.front().m_distance < 5000 && weatherStationsI.front().m_deltaElev < 50)
						{
							CWeatherStation obsStation;
							obsDB.Get(obsStation, weatherStationsI[0].m_index, years);

							CWeatherGenerator WG = WGin;
							CWGInput WGInputTmp(WGInput);
							WGInputTmp.m_variables = v;
							WGInputTmp.m_bXValidation = true;
							WG.SetWGInput(WGInputTmp);
							WG.SetTarget(obsStation);

							msg = WG.Initialize();//create gradient

							if (msg)
							{
								CWeatherStation simStation;
								((CLocation&)simStation) = obsStation;
								msg = (WG.GetWGInput().IsHourly()) ? WG.GetHourly(simStation, callback) : WG.GetDaily(simStation, callback);

								CTPeriod period = simStation.GetEntireTPeriod();
								period.Intersect(obsStation.GetEntireTPeriod());

								for (CTRef TRef = period.Begin(); TRef != period.End(); TRef++)
								{
									if (simStation[TRef][v].IsInit() && obsStation[TRef][v].IsInit())
									{
										stationStat[v].Add(simStation[TRef][v], obsStation[TRef][v]);
#pragma omp critical(SAVE_STATS)
										overallStat[v].Add(simStation[TRef][v], obsStation[TRef][v]);
									}
								}
							}//if(msg)
						}//if this year have this station

						msg += callback.StepIt();
#pragma omp flush(msg)
					}//if this variable is used
				}//for all variable	


				//save data 
				for (size_t v = 0, vv = 0; v < NB_VAR_H&&msg; v++)
				{
					if (variables[v])
					{
						CTM TM(CTM::ANNUAL, CTM::OVERALL_YEARS);
						CNewSectionData  section(1, S_NB_STAT, CTRef(YEAR_NOT_INIT, 0, 0, 0, TM));

						for (size_t s = 0; s < S_NB_STAT; s++)
							section[0][s] = GetStat(s, stationStat[v]);


						//for all statistics
						size_t no = resultDB.GetSectionNo(l, vv, 0);
						resultDB.SetSection(no, section);
						vv++;
					}//if selected variable
				}
			}//if(msg)
		}// for all locations



		callback.AddMessage("");
		callback.AddMessage("Overall statistics:");
		callback.AddMessage("Variable\tN\tObserved\tSimulated\tBias\tMAD\tRMSE\tR²");
		for (size_t v = 0; v < NB_VAR_H&&msg; v++)
		{
			if (variables[v])
			{
				std::string str = std::string(GetVariableName(v));
				//str += "\t" + ToString(overallStat[v][NB_VALUE], 4);
				for (size_t s = 0; s < S_NB_STAT; s++)
					str += "\t" + ToString(overallStat[v][STATISTICS[s]], 4);

				callback.AddMessage(str);
			}
		}

		callback.PopTask();

		return msg;
	}

	ERMsg CWGInputAnalysis::NormalError(const CFileManager& fileManager, CResult& resultDB, CCallback& callback)
	{
		ERMsg msg;

		CWeatherGenerator WG;
		msg = InitDefaultWG(fileManager, WG, callback);

		if (!msg)
			return msg;

		const CWGInput& WGInput = WG.GetWGInput();
		const CLocationVector& locations = resultDB.GetMetadata().GetLocations();

		CStatisticXY overallStat[NORMALS_DATA::NB_FIELDS];

		//limit category to basic variable
		//CWVariables variables = WGInput.m_variables;
		//bitset<NB_CATEGORIES> categories = GetCategory(variables);
		bitset<NB_CATEGORIES> categories = GetCategories(WGInput.m_variables);
		CWVariables variables = GetCategoryVariables(categories);

		callback.PushTask("Estimate of gradients error for Normals", categories.count()*locations.size());
		//callback.SetNbStep(categories.count()*locations.size());


		CStatistic::SetVMiss(VMISS);


		for (size_t l = 0; l < locations.size() && msg; l++)
		{
			array<array<CStatisticXYW, NORMALS_DATA::NB_FIELDS>, 12>  stationStat;

			for (size_t c = 0; c < NB_CATEGORIES && msg; c++)//for all categories
			{
				if (categories[c])//if this categories is selected
				{
					CWGInput WGInputTmp(WGInput);
					WGInputTmp.m_variables = GetCategoryVariables(c);
					WGInputTmp.m_bXValidation = true;
					WG.SetWGInput(WGInputTmp);
					TVarH v = GetCategoryLeadVariable(c);

					//fin stations for this variables
					CSearchResultVector weatherStationsI;
					msg = WG.GetNormalDB()->Search(weatherStationsI, locations[l], WGInputTmp.m_nbNormalsStations, WGInputTmp.m_searchRadius[v], WGInputTmp.m_variables, -999, true, true, WGInput.m_bUseShore);

					std::vector<double> weightI = weatherStationsI.GetStationWeight(/*true, WEATHER::SHORE_DISTANCE_FACTOR>0*/);
					assert(weightI.size() == weatherStationsI.size());

					for (size_t ll = 0; ll < weatherStationsI.size() && msg; ll++)
					{
						CNormalsStation obsStation;
						WG.GetNormalDB()->Get(obsStation, weatherStationsI[ll].m_index);

						// init the loc part of WGInput
						WG.SetTarget(obsStation);
						msg = WG.Initialize();//create gradient

						if (msg)
						{
							CNormalsStation simStation;
							WG.GetNormals(simStation, callback);

							for (size_t f = 0; f < NORMALS_DATA::NB_FIELDS&&msg; f++)
							{
								if (WGInputTmp.m_variables[NORMALS_DATA::F2V(f)])
								{
									for (size_t m = 0; m < 12; m++)
									{
										stationStat[m][f].Add(simStation[m][f], obsStation[m][f], weightI[ll]);
									}
								}
							}
						}
					}//for nearest weather stations I

					msg += callback.StepIt();

				}//if this category is used
			}//for all category	


			//save data 
			for (size_t f = 0; f < NORMALS_DATA::NB_FIELDS&&msg; f++)
			{
				if (variables[NORMALS_DATA::F2V(f)])
				{
					CTM TM(CTM::ANNUAL, CTM::OVERALL_YEARS);
					CNewSectionData  section(1, S_NB_STAT, CTRef(0, 0, 0, 0, TM));

					CStatisticXY allMonths;
					CStatisticXYW allMonthsW;
					for (size_t m = 0; m < 12; m++)
					{
						double obs = stationStat[m][f][MEAN_Y];
						double sim = stationStat[m][f][MEAN_X];
						overallStat[f].Add(sim, obs);
						allMonths.Add(sim, obs);

						for (size_t s = 0; s < S_NB_STAT; s++)
							section[0][s] += stationStat[m][f][STATISTICS[s]];

					}

					//add only once
					section[0][S_RMSE] = allMonths[STATISTICS[S_RMSE]];
					section[0][S_STAT_R²] = allMonths[STATISTICS[S_STAT_R²]];

					//for all statistics
					resultDB.AddSection(section);
				}//if selected field
			}
		}// for all locations



		callback.AddMessage("");
		callback.AddMessage("Overall statistics:");
		callback.AddMessage("Variable\tN\tObserved\tSimulated\tBias\tMAD\tRMSE\tR²");
		for (size_t f = 0; f < NORMALS_DATA::NB_FIELDS&&msg; f++)
		{
			if (variables[NORMALS_DATA::F2V(f)])
			{
				std::string str = std::string(NORMALS_DATA::GetFieldTitle(f));
				//str += "\t" + ToString(overallStat[f][NB_VALUE], 4);
				for (size_t s = 0; s < S_NB_STAT; s++)
					str += "\t" + ToString(overallStat[f][STATISTICS[s]], 4);

				callback.AddMessage(str);
			}
		}

		callback.PopTask();

		return msg;

	}

	ERMsg CWGInputAnalysis::ObservationsError(const CFileManager& fileManager, CResult& resultDB, CCallback& callback)
	{

		ERMsg msg;

		CWGInput WGInput;
		msg = GetWGInput(fileManager, WGInput);

		if (msg && WGInput.IsNormals())
			msg.ajoute("Estimate of observation gradients error can only be done from observation weather generation");

		if (!msg)
			return msg;

		CWeatherGenerator WG;
		msg = InitDefaultWG(fileManager, WG, callback);



		CStatisticXY overallStat[NB_VAR_H];

		CWeatherDatabase& obsDB = GetObsDB(WG);
		size_t nbStations = WGInput.IsHourly() ? WGInput.m_nbHourlyStations : WGInput.m_nbDailyStations;
		const CLocationVector& locations = resultDB.GetMetadata().GetLocations();
		CWVariables variables = WGInput.m_variables;
		size_t nbYears = WGInput.GetNbYears();

		callback.PushTask("Estimate of gradients error for observations", variables.count()*locations.size()*nbYears);
		//callback.SetNbStep(variables.count()*locations.size()*nbYears);
		CStatistic::SetVMiss(VMISS);


		for (size_t l = 0; l < locations.size() && msg; l++)
		{

			CTReferencedMatrix<CStatisticXYW> stationStat(WGInput.GetTPeriod(), NB_VAR_H);

			for (TVarH v = H_FIRST_VAR; v < NB_VAR_H && msg; v++)//for all category
			{
				if (variables[v])//if this variable is selected
				{

					for (size_t y = 0; y < nbYears&&msg; y++)
					{
						int year = WGInput.GetFirstYear() + int(y);


						//fin stations for this variables
						CSearchResultVector weatherStationsI;

						//don't process error, just skip it
						obsDB.Search(weatherStationsI, locations[l], nbStations, WGInput.m_searchRadius[v], v, year, true, true, WGInput.m_bUseShore);

						vector<double> weightI = weatherStationsI.GetStationWeight(/*true, WEATHER::SHORE_DISTANCE_FACTOR>0*/);
						for (size_t ll = 0; ll < weatherStationsI.size() && msg; ll++)
						{
							CWeatherStation obsStation;
							msg = obsDB.Get(obsStation, weatherStationsI[ll].m_index);

							if (msg)
							{
								CWGInput WGInputTmp(WGInput);
								WGInputTmp.m_variables = v;
								WGInputTmp.m_bXValidation = true;
								WGInputTmp.m_firstYear = year;
								WGInputTmp.m_lastYear = year;

								WG.SetWGInput(WGInputTmp);
								WG.SetTarget(obsStation);
								msg = WG.Initialize();//create gradient


								CWeatherStation simStation;
								msg = (WG.GetWGInput().IsHourly()) ? WG.GetHourly(simStation, callback) : WG.GetDaily(simStation, callback);

								if (msg)
								{
									CTPeriod period = simStation.GetEntireTPeriod();
									period.Intersect(obsStation.GetEntireTPeriod());

									for (CTRef TRef = period.Begin(); TRef != period.End(); TRef++)
									{
										if (simStation[TRef][v].IsInit() && obsStation[TRef][v].IsInit())
										{
											stationStat[TRef][v].Add(simStation[TRef][v], obsStation[TRef][v], weightI[ll]);
										}
									}
								}
							}//if msg
						}//for nearest weather stations I

						msg += callback.StepIt();
					}//for all years
				}//if this category is used
			}//for all category	


			//save data 
			for (size_t v = 0; v < NB_VAR_H&&msg; v++)
			{
				if (variables[v])
				{
					CTM TM(CTM::ANNUAL, CTM::OVERALL_YEARS);
					CNewSectionData  section(1, S_NB_STAT, CTRef(0, 0, 0, 0, TM));

					CTPeriod period = stationStat.m_period;


					CStatisticXY all;
					for (CTRef TRef = period.Begin(); TRef != period.End(); TRef++)
					{
						double obs = stationStat[TRef][v][MEAN_Y];
						double sim = stationStat[TRef][v][MEAN_X];
						if (obs > CStatistic::GetVMiss() && sim > CStatistic::GetVMiss())
						{
							overallStat[v].Add(sim, obs);
							all.Add(sim, obs);

							for (size_t s = 0; s < S_NB_STAT; s++)
								section[0][s] += stationStat(TRef, v)[STATISTICS[s]];
						}
					}

					//add only once
					section[0][S_STAT_R²] = all[STATISTICS[S_STAT_R²]];

					//for all statistics
					resultDB.AddSection(section);
				}//if selected field
			}
		}// for all locations



		callback.AddMessage("");
		callback.AddMessage("Overall statistics:");
		callback.AddMessage("Variable\tN\tObserved\tSimulated\tBias\tMAD\tRMSE\tR²");
		for (size_t v = 0; v < NB_VAR_H&&msg; v++)
		{
			if (variables[v])
			{
				std::string str = std::string(GetVariableName(v));
				//str += "\t" + ToString(overallStat[v][NB_VALUE], 4);
				for (size_t s = 0; s < S_NB_STAT; s++)
					str += "\t" + ToString(overallStat[v][STATISTICS[s]], 4);

				callback.AddMessage(str);
			}
		}

		callback.PopTask();

		return msg;

	}


	ERMsg CWGInputAnalysis::ExtractNormal(const CFileManager& fileManager, CResult& resultDB, CCallback& callback)
	{
		ASSERT(m_pParent);
		ERMsg msg;
		//
		CWeatherGenerator WGBase;
		msg = InitDefaultWG(fileManager, WGBase, callback);

		if (!msg)
			return msg;

		const CLocationVector& locations = resultDB.GetMetadata().GetLocations();

		callback.PushTask("Extract normals", locations.size());
		CStatistic::SetVMiss(VMISS);

		vector<size_t> locPos;
		CWeatherGeneration::GetLocationIndexGrid(locations, locPos);

#pragma omp parallel for schedule(static, 1) shared(resultDB, msg) 
		for (__int64 ll = 0; ll < (__int64)locPos.size(); ll++)
		{
#pragma omp flush(msg)
			if (msg)
			{
				//fin stations for this variables
				// init the loc part of WGInput
				size_t l = locPos[ll];
				CWeatherGenerator WG = WGBase;
				WG.SetTarget(locations[l]);
				msg = WG.Initialize();//create gradient

				if (msg)
				{
					CNormalsStation simStation;
					WG.GetNormals(simStation, callback);
					CModelStatVector section(12, CTRef(YEAR_NOT_INIT, 0, 0, 0, CTM(CTRef::MONTHLY, CTRef::OVERALL_YEARS)), NORMALS_DATA::NB_FIELDS);

					for (size_t f = 0; f < NORMALS_DATA::NB_FIELDS&&msg; f++)
					{
						for (size_t m = 0; m < 12; m++)
							section[m][f] = simStation[m][f];
					}

					resultDB.SetSection(l, section);
					msg += callback.StepIt();
#pragma omp flush(msg)
				}
			}
		}

		return msg;
	}

	ERMsg CWGInputAnalysis::ExtractGradientInputs(const CFileManager& fileManager, CResult& resultDB, CCallback& callback)
	{
		ASSERT(m_pParent);
		ERMsg msg;
		//
		CWeatherGenerator WGBase;
		msg = InitDefaultWG(fileManager, WGBase, callback);

		if (!msg)
			return msg;

		const CLocationVector& locations = resultDB.GetMetadata().GetLocations();

		callback.PushTask("Extract normals", locations.size());
		CStatistic::SetVMiss(VMISS);

		vector<size_t> locPos;
		CWeatherGeneration::GetLocationIndexGrid(locations, locPos);

//#pragma omp parallel for schedule(static, 1) shared(resultDB, msg) 
		for (__int64 ll = 0; ll < (__int64)locPos.size(); ll++)
		{
#pragma omp flush(msg)
			if (msg)
			{
				//fin stations for this variables
				// init the loc part of WGInput
				size_t l = locPos[ll];
				//CWeatherGenerator WG = WGBase;
				//WG.SetTarget(locations[l]);
				
				CNormalsDatabasePtr pNormalDB = WGBase.GetNormalDB();	//Normal database
				CWeatherGradient gradients;
				gradients.SetNormalsDatabase(pNormalDB);

				gradients.m_variables = WGBase.GetWGInput().GetNormalMandatoryVariables();

				if (gradients.m_variables[H_TMIN] || gradients.m_variables[H_TAIR] || gradients.m_variables[H_TMAX])
				{
					gradients.m_variables.set(H_TMIN);
					gradients.m_variables.set(H_TAIR);
					gradients.m_variables.set(H_TMAX);
				}

				gradients.m_allowDerivedVariables = WGBase.GetWGInput().m_allowedDerivedVariables;
				gradients.m_bXVal = WGBase.GetWGInput().m_bXValidation;
				gradients.m_bUseShore = WGBase.GetWGInput().m_bUseShore;
				gradients.m_target = locations[l];
				gradients.m_bKeepInputs = true;

				msg = gradients.CreateGradient(callback);

				if (msg)
				{
					//CModelStatVector section(12, CTRef(YEAR_NOT_INIT, 0, 0, 0, CTM(CTRef::MONTHLY, CTRef::OVERALL_YEARS)), NORMALS_DATA::NB_FIELDS);
					CTRef TRef(YEAR_NOT_INIT, 0, 0, 0, CTM(CTRef::ANNUAL, CTRef::OVERALL_YEARS));
					CNewSectionData section(1, 16, TRef);
					
					//for (size_t z = 0; z < NB_SCALE_GRADIENT&&msg; z++)
					size_t z = 0;
					{
						for (size_t g = 0; g < GRADIENT::NB_GRADIENT&&msg; g++)
						{
							const CGradientInput& input = gradients.GetInputs(z, g);
							
							for (size_t i = 0; i < input.size(); i++)
							{
								ASSERT(input[i].size() == 16);

								for (size_t j = 0; j < input[i].size(); j++)
									section[0][j] = input[i][j];

								resultDB.SetSection((l*GRADIENT::NB_GRADIENT + g)*input.size()+i, section);
							}

							
						}
					}
					
					
					
					msg += callback.StepIt();
#pragma omp flush(msg)
				}
			}
		}

		return msg;
	}

	ERMsg CWGInputAnalysis::GetNbMissingObservations(const CFileManager& fileManager, CResult& resultDB, CCallback& callback)
	{
		ERMsg msg;

		CWeatherGenerator WGBase;
		if (msg)
			msg = InitDefaultWG(fileManager, WGBase, callback);

		if (msg && WGBase.GetWGInput().IsNormals())
			msg.ajoute("The input weather generation don't use observations");

		if (!msg)
			return msg;

		const CLocationVector& locations = resultDB.GetMetadata().GetLocations();
		ASSERT(resultDB.GetNbSection() == locations.size());
		callback.PushTask("Get number of missing observations", locations.size());

		vector<size_t> locPos;
		CWeatherGeneration::GetLocationIndexGrid(locations, locPos);


		CWVariables variables = WGBase.GetWGInput().m_variables;

#pragma omp parallel for schedule(static, 1) shared(resultDB, msg) 
		for (__int64 ll = 0; ll < (__int64)locPos.size(); ll++)
		{
#pragma omp flush(msg)
			if (msg)
			{
				size_t l = locPos[ll];
				CWeatherGenerator WG = WGBase;
				WG.SetTarget(locations[l]);
				msg = WG.Initialize();
				if (msg)
				{
					CSimulationPoint simulationPoint;
					if (WG.GetWGInput().IsHourly())
					{
						msg += WG.GetHourly(simulationPoint, callback);
					}
					else
					{
						ASSERT(WG.GetWGInput().IsDaily());
						msg += WG.GetDaily(simulationPoint, callback);
					}

					if (msg)
					{
						CTRef TRef(simulationPoint.GetFirstYear());
						CNewSectionData section(simulationPoint.size(), variables.count(), TRef);

						for (size_t y = 0; y < simulationPoint.size(); y++)
						{
							//Get last day and count of data for this variable
							size_t nbRefs = simulationPoint[y].GetEntireTPeriod().size();
							CWVariablesCounter count = simulationPoint[y].GetVariablesCount();
							for (size_t v = 0, vv = 0; v < NB_VAR_H&&msg; v++)
							{
								ASSERT(nbRefs - count[v].first >= 0);

								if (variables[v])
									section[y][vv++] = nbRefs - count[v].first;
							}//for all variables
						}

						//resultDB.AddSection(section);
						resultDB.SetSection(l, section);

						msg += callback.StepIt();
#pragma omp flush(msg)
					}//if msg
				}//if msg
			}//if msg
		}//for all loc

		callback.PopTask();

		return msg;
	}

	ERMsg CWGInputAnalysis::LastObservation(const CFileManager& fileManager, CResult& resultDB, CCallback& callback)
	{
		ERMsg msg;

		CWGInput WGInput;
		msg = GetWGInput(fileManager, WGInput);


		if (msg && WGInput.IsNormals())
			msg.ajoute("The input weather generation don't use observation");


		CWeatherGenerator WG;
		if (msg)
			msg = InitDefaultWG(fileManager, WG, callback);

		if (!msg)
			return msg;

		const CLocationVector& locations = resultDB.GetMetadata().GetLocations();

		//limit category to basic variable

		callback.AddMessage(FormatMsg(IDS_SIM_CREATE_DATABASE, m_name));
		callback.AddMessage(resultDB.GetFilePath(), 1);

		CWVariables variables = WG.GetWGInput().m_variables;

		callback.PushTask("Getting number of obsevations", locations.size());

		for (size_t l = 0; l < locations.size() && msg; l++)
		{
			WG.SetTarget(locations[l]);

			CSimulationPoint simulationPoint;
			if (WG.GetWGInput().IsHourly())
				msg += WG.GetHourly(simulationPoint, callback);
			else
				msg += WG.GetDaily(simulationPoint, callback);



			CTPeriod period = simulationPoint.GetEntireTPeriod();
			CTRef TRef(simulationPoint.GetFirstYear());
			CNewSectionData section(simulationPoint.size(), variables.count(), TRef);

			for (size_t y = 0; y < simulationPoint.size() && msg; y++)
			{
				//Get last day and count of data for this variable
				CWVariablesCounter count = simulationPoint[y].GetVariablesCount();
				for (size_t v = 0, vv = 0; v < NB_VAR_H&&msg; v++)
				{
					if (variables[v])
						section[y][vv++] = count[v].first;//.second.End().GetJDay()
				}//for all variables
			}

			resultDB.AddSection(section);

			msg += callback.StepIt();
		}//for all loc

		callback.PopTask();

		return msg;
	}


	ERMsg CWGInputAnalysis::GetCompleteness(const CFileManager& fileManager, CResult& resultDB, CCallback& callback)
	{
		ERMsg msg;

		CWGInput WGInput;
		msg = GetWGInput(fileManager, WGInput);

		if (msg && WGInput.IsNormals())
			msg.ajoute("The input weather generation don't use observation");

		if (!msg)
			return msg;

		callback.AddMessage(FormatMsg(IDS_SIM_CREATE_DATABASE, m_name));
		callback.AddMessage(resultDB.GetFilePath(), 1);

		CWVariables variables = WGInput.m_variables;

		string DBFilePath;

		if (WGInput.IsHourly())
			DBFilePath = fileManager.Hourly().GetFilePath(WGInput.m_hourlyDBName);
		else
			DBFilePath = fileManager.Daily().GetFilePath(WGInput.m_dailyDBName);


		CWeatherDatabasePtr pWeatherDB = CreateWeatherDatabase(DBFilePath);
		if (pWeatherDB)
		{
			msg = pWeatherDB->Open(DBFilePath, CWeatherDatabase::modeRead, callback, WGInput.m_bSkipVerify);
			if (msg && !pWeatherDB->GetYears().empty())
			{
				callback.PushTask("Completness of obsevations", pWeatherDB->size());

				int begin = *pWeatherDB->GetYears().begin();
				int end = *pWeatherDB->GetYears().rbegin();
				size_t nbYears = end - begin + 1;

				for (size_t s = 0; s < pWeatherDB->size(); s++)
				{
					CTRef TRef(begin, 0, 0, 0, CTM(CTM::ANNUAL));
					CNewSectionData section(nbYears, variables.count(), TRef);

					std::set<int> years = pWeatherDB->GetYears(s);
					for (size_t y = 0; y < nbYears; y++)
					{
						int year = begin + int(y);
						if (years.find(year) != years.end())
						{
							CWVariablesCounter count = pWeatherDB->GetWVariablesCounter(s, year);

							for (size_t v = 0, vv = 0; v < NB_VAR_H&&msg; v++)
							{
								if (variables[v])
								{
									double compl = 100.0*count[v].first / GetNbDaysPerYear(year);
									section[y][vv++] = compl;
								}

							}//for all variables
						}
						else
						{
							for (size_t v = 0; v < section[y].size(); v++)
								section[y][v] = 0;
						}
					}

					resultDB.AddSection(section);
					msg += callback.StepIt();
				}//for all stations

				pWeatherDB->Close();
				callback.PopTask();
			}//if open
		}

		return msg;
	}


	ERMsg CWGInputAnalysis::GetWGInput(const CFileManager& fileManager, CWGInput& WGInput)
	{
		ERMsg msg;

		ASSERT(dynamic_cast<const CWeatherGeneration*>(m_pParent) != NULL);
		const CWeatherGeneration& parent = dynamic_cast<const CWeatherGeneration&>(*m_pParent);

		if (msg)
			msg = parent.GetWGInput(fileManager, WGInput);

		return msg;
	}

	ERMsg CWGInputAnalysis::InitDefaultWG(const CFileManager& fileManager, CWeatherGenerator& WG, CCallback& callback)
	{
		ERMsg msg;

		ASSERT(dynamic_cast<const CWeatherGeneration*>(m_pParent) != NULL);
		const CWeatherGeneration& parent = dynamic_cast<const CWeatherGeneration&>(*m_pParent);

		//Load WGInput 
		CWGInput WGInput;
		msg = GetWGInput(fileManager, WGInput);

		//find normal file path
		std::string NFilePath;
		if (msg)
		{
			msg = fileManager.Normals().GetFilePath(WGInput.m_normalsDBName, NFilePath);
			callback.AddMessage(FormatMsg(IDS_SIM_NORMAL_USED, NFilePath));
		}


		//find daily file apth if any
		std::string DFilePath;
		if (msg && WGInput.UseDaily())
		{
			msg = fileManager.Daily().GetFilePath(WGInput.m_dailyDBName, DFilePath);
			callback.AddMessage(FormatMsg(IDS_SIM_DAILY_USED, DFilePath));
		}


		std::string HFilePath;
		if (msg && WGInput.UseHourly())
		{
			msg = fileManager.Hourly().GetFilePath(WGInput.m_hourlyDBName, HFilePath);
			callback.AddMessage(FormatMsg(IDS_SIM_HOURLY_USED, HFilePath));
		}

		//open normal database
		CNormalsDatabasePtr pNormalDB;
		if (msg)
		{
			pNormalDB.reset(new CNormalsDatabase);
			msg = pNormalDB->Open(NFilePath, CNormalsDatabase::modeRead, callback, WGInput.m_bSkipVerify);
			if (msg)
				msg = pNormalDB->OpenSearchOptimization(callback);//open here to be thread safe
		}

		//open daily database
		CDailyDatabasePtr pDailyDB;
		if (msg && WGInput.IsDaily())
		{
			pDailyDB.reset(new CDailyDatabase);
			msg = pDailyDB->Open(DFilePath, CDailyDatabase::modeRead, callback, WGInput.m_bSkipVerify);
			if (msg)
				msg = pDailyDB->OpenSearchOptimization(callback);//open here to be thread safe
		}

		CHourlyDatabasePtr pHourlyDB;
		if (msg && WGInput.IsHourly())
		{
			pHourlyDB.reset(new CHourlyDatabase);
			msg = pHourlyDB->Open(HFilePath, CHourlyDatabase::modeRead, callback, WGInput.m_bSkipVerify);
			if (msg)
				msg = pHourlyDB->OpenSearchOptimization(callback);//open here to be thread safe
		}

		if (msg)
		{
			WG.SetWGInput(WGInput);
			WG.SetNbReplications(parent.m_nbReplications);//no replication in daily db
			WG.SetNormalDB(pNormalDB);
			WG.SetDailyDB(pDailyDB);
			WG.SetHourlyDB(pHourlyDB);


		}

		return msg;
	}


	void CWGInputAnalysis::writeStruc(zen::XmlElement& output)const
	{
		CExecutable::writeStruc(output);
		zen::XmlOut out(output);
		out[GetMemberName(KIND)](m_kind);
		out[GetMemberName(EXPORT_MATCH)](m_bExportMatch);
		out[GetMemberName(MATCH_NAME)](m_matchName);

	}

	bool CWGInputAnalysis::readStruc(const zen::XmlElement& input)
	{
		CExecutable::readStruc(input);
		zen::XmlIn in(input);
		in[GetMemberName(KIND)](m_kind);
		in[GetMemberName(EXPORT_MATCH)](m_bExportMatch);
		in[GetMemberName(MATCH_NAME)](m_matchName);


		return true;
	}

}