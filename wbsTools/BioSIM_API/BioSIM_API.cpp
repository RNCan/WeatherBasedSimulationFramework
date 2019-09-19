#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include "BioSIM_API.h"
#include "Basic/NormalsDatabase.h"
#include "Basic/WeatherDefine.h"
#include "Basic/DynamicRessource.h"
#include "Simulation/WeatherGradient.h"
#include "Simulation/WeatherGenerator.h"
#include "FileManager/FileManager.h"
#include "WeatherBasedSimulationString.h"
#include "Basic/Shore.h"
//#include "json\json11.hpp"
//using namespace json11;
//#pragma warning(disable: 4275 4251)
//#include "GDAL_priv.h"
//#include "ogr_srs_api.h"
#include "Geomatic/UtilGDAL.h"

//using namespace boost;
using namespace std;

namespace py = pybind11;

using namespace WBSF;
using namespace HOURLY_DATA;
using namespace NORMALS_DATA;
extern HMODULE g_hDLL;

namespace WBSF
{

	static std::string get_string(ERMsg msg)
	{
		std::string str;
		if (msg)
		{
			str = "Success";
		}
		else
		{
			for (UINT i = 0; i < msg.dimension(); i++)
			{
				str += ANSI_UTF8(msg[i]) + "\n";
			}
		}

		return str;
	}

	//******************************************************************************************************************************


	const char* WeatherGeneratorOptions::NAME[NB_PAPAMS] =
	{
		"VARIABLES", "SOURCE", "GENERATION", "NB_REPLICATIONS",
		"LATITUDE", "LONGITUDE", "ELEVATION", "SLOPE", "ORIENTATION",
		"NB_NEAREST_NEIGHBOR", "FIRST_YEAR", "LAST_YEAR","NB_YEARS", "SEED", "NORMALS_INFO"
	};

	WeatherGeneratorOptions::WeatherGeneratorOptions()
	{


		m_sourceType = 1;
		m_generationType = 1;
		m_variables = "TN T TX P";
		m_normals_info = "1981-2010";
		m_latitude = -999;
		m_longitude = -999;
		m_elevation = -999;
		m_slope = -999;
		m_orientation = -999;
		m_nb_nearest_neighbor = 4;
		m_nb_replications = 1;
		m_nb_years =1;
		m_first_year = CTRef::GetCurrentTRef().GetYear();
		m_last_year = CTRef::GetCurrentTRef().GetYear();
		m_seed = 0;
	}

	ERMsg WeatherGeneratorOptions::parse(const string& str_options)
	{
		ERMsg msg;

		StringVector args(str_options, "&");
		for (size_t i = 0; i < args.size(); i++)
		{
			StringVector option(args[i], "=");
			if (option.size() == 2)
			{
				auto it = std::find(begin(NAME), end(NAME), MakeUpper(option[0]));
				if (it != end(NAME))
				{
					size_t o = distance(begin(NAME), it);
					switch (o)
					{
					case VARIABLES:				m_variables = option[1]; break;
					case SOURCE_TYPE:
					{
						if (IsEqual(option[1], "FromNormals"))
							m_sourceType = 0;
						else if (IsEqual(option[1], "FromObservation"))
							m_sourceType = 1;
						else
							msg.ajoute(option[1] + " is not a valid source type. Select FromNormals or FromObservation.");
						break;
					}
					case NORMALS_INFO:			m_normals_info = option[1]; break;
					case GENERATION_TYPE:
					{
						if (IsEqual(option[1], "Hourly"))
							m_generationType = 0;
						else if (IsEqual(option[1], "Daily"))
							m_generationType = 1;
						else
							msg.ajoute(option[1] + " is not a valid generation type. Select Daily or Hourly.");

						break;
					}
					case LATITUDE:				
					{
						m_latitude = ToDouble(option[1]);
						if(m_latitude<-90 || m_latitude>90)
							msg.ajoute(option[1] + " is not a valid latitude. Latitude must between -90 and 90.");
						break;
					}
					case LONGITUDE:				
					{
						m_longitude = ToDouble(option[1]);
						if (m_longitude < -180 || m_longitude >180)
							msg.ajoute(option[1] + " is not a valid longitude. Longitude must between -180 and 180.");
						break;
					}
					case ELEVATION:				
					{
						m_elevation = ToDouble(option[1]);
						if (m_elevation < -100 || m_elevation >8000)
							msg.ajoute(option[1] + " is not a valid elevation. Elevation can by missing or between -100 and 8000 meters.");
						break;
					}
					case SLOPE:					m_slope = ToDouble(option[1]); break;
					case ORIENTATION:			m_orientation = ToDouble(option[1]); break;
					case NB_NEAREST_NEIGHBOR:	
					{
						m_nb_nearest_neighbor = ToSizeT(option[1]);
						if (m_nb_nearest_neighbor == 0)
							msg.ajoute(option[1] + " is not a valid nearest neighbor. nearest neighbor.");
						
						break;
					}
					case FIRST_YEAR:		
					{
						m_first_year = ToInt(option[1]);
						if (m_first_year < 1900|| m_first_year>GetCurrentYear())
							msg.ajoute(option[1] + " is not a valid year. First year must between 1900 and current year.");

						break;
					}
					case LAST_YEAR:	
					{
						m_last_year = ToInt(option[1]);
						if (m_last_year < 1900 || m_last_year>GetCurrentYear())
							msg.ajoute(option[1] + " is not a valid year. Last year must between 1900 and current year.");

						break;
					}
					case SEED:				    m_seed = ToInt(option[1]); break;
					default: ASSERT(false);
					}
				}
				else
				{
					msg.ajoute("Invalid options in argument " + to_string(i + 1) + "( " + args[i] + ")");
				}
			}
			else
			{
				msg.ajoute("error in argument " + to_string(i + 1) + "( " + args[i] + ")");
			}
		}

		if (m_first_year > m_last_year)
			msg.ajoute("First year must be lower than last year.");


		return msg;
	}

	//******************************************************************************************************************************


	const char* WeatherGenerator::NAME[NB_PAPAMS] = { "SHORE", "NORMALS", "DAILY", "HOURLY", "GRIBS", "DEM" };


	WeatherGenerator::WeatherGenerator(const std::string &)
	{

	}

	std::string WeatherGenerator::Initialize(const std::string& str_options)
	{
		ERMsg msg;

		try
		{
			//CPLSetConfigOption("GDAL_CACHEMAX", "2048");
			RegisterGDAL();


			CDynamicResources::set(g_hDLL);
			CCallback callback;

			StringVector args(str_options, "&");
			for (size_t i = 0; i < args.size(); i++)
			{
				StringVector option(args[i], "=");
				if (option.size() == 2)
				{
					auto it = std::find(begin(NAME), end(NAME), MakeUpper(option[0]));
					if (it != end(NAME))
					{
						size_t o = distance(begin(NAME), it);
						switch (o)
						{
						case SHORE:
						{
							msg += CShore::SetShore(option[1]);
							break;
						}
						case NORMALS:
						{
							m_pNormalDB.reset(new CNormalsDatabase);
							msg += m_pNormalDB->Open(option[1], CNormalsDatabase::modeRead, callback, true);
							if (msg)
								msg += m_pNormalDB->OpenSearchOptimization(callback);//open here to be thread safe

							break;
						}
						case DAILY:
						{
							m_pDailyDB.reset(new CDailyDatabase);
							msg += m_pDailyDB->Open(option[1], CDailyDatabase::modeRead, callback, true);
							if (msg)
								msg += m_pDailyDB->OpenSearchOptimization(callback);//open here to be thread safe

							break;
						}

						case HOURLY:
						{
							m_pHourlyDB.reset(new CHourlyDatabase);
							msg += m_pHourlyDB->Open(option[1], CHourlyDatabase::modeRead, callback, true);
							if (msg)
								msg += m_pHourlyDB->OpenSearchOptimization(callback);//open here to be thread safe

							break;
						}

						case GRIBS:
						{
							m_pGribsDB.reset(new CSfcGribDatabase);
							msg += m_pGribsDB->Open(option[1]);
							if (msg)
								msg += m_pGribsDB->OpenSearchOptimization(CCallback());//open here to be thread safe

							break;
						}

						case DEM:
						{
							m_pDEM.reset(new CGDALDatasetEx);
							msg += m_pDEM->OpenInputImage(option[1]);

							break;
						}

						default: ASSERT(false);
						}
					}
					else
					{
						msg.ajoute("Invalid options in argument " + to_string(i + 1) + "( " + args[i] + ")");
					}
				}
				else
				{
					msg.ajoute("error in argument " + to_string(i + 1) + "( " + args[i] + ")");
				}
			}


			m_pWeatherGenerator.reset(new CWeatherGenerator);
			m_pWeatherGenerator->SetNormalDB(m_pNormalDB);
			m_pWeatherGenerator->SetDailyDB(m_pDailyDB);
			m_pWeatherGenerator->SetHourlyDB(m_pHourlyDB);
			m_pWeatherGenerator->SetGribsDB(m_pGribsDB);
		}
		catch (...)
		{
			int i;
			i = 0;
		}

		return get_string(msg);
	}


	std::string WeatherGenerator::Generate(const std::string& str_options)
	{
		ERMsg msg;
		CCallback callback;
		string output;

		try
		{
			CTimer timer(TRUE);
			WeatherGeneratorOptions options;
			msg = options.parse(str_options);

			if (msg)
			{

				if (options.m_elevation < -100)
				{
					if (m_pDEM && m_pDEM->IsOpen())
					{
						CGeoPoint pt(options.m_longitude, options.m_latitude, PRJ_WGS_84);

						CGeoPointIndex xy = m_pDEM->GetExtents().CoordToXYPos(pt);
						if (m_pDEM->GetExtents().IsInside(xy))
						{
							options.m_elevation = m_pDEM->ReadPixel(0, xy);

							if (fabs(options.m_elevation - m_pDEM->GetNoData(0)) < 0.1)
							{
								msg.ajoute("DEM is not available for the lat/lon coordinate.");
							}
						}
						else
						{
							msg.ajoute("Lat/lon outside DEM extents.");
						}
					}
					else
					{
						msg.ajoute("Elevation and DEM is not provided. Invalid elevation.");
					}
				}

				if (msg)
				{
					CLocation location("", "", options.m_latitude, options.m_longitude, options.m_elevation);

					//Load WGInput 
					CWGInput WGInput;

					WGInput.m_variables = options.m_variables;
					WGInput.m_sourceType = options.m_sourceType;
					WGInput.m_generationType = options.m_generationType;
					WGInput.m_nbNormalsYears = options.m_nb_years;
					WGInput.m_firstYear = options.m_first_year;
					WGInput.m_lastYear = options.m_last_year;
					WGInput.m_bUseForecast = true;
					WGInput.m_bUseGribs = false;

					WGInput.m_nbNormalsStations = options.m_nb_nearest_neighbor;
					WGInput.m_nbDailyStations = options.m_nb_nearest_neighbor;
					WGInput.m_nbHourlyStations = options.m_nb_nearest_neighbor;
					WGInput.m_nbGribPoints = options.m_nb_nearest_neighbor;
					//WGInput.m_albedo;
					WGInput.m_seed = options.m_seed;
					WGInput.m_bXValidation = false;
					WGInput.m_bSkipVerify = true;
					WGInput.m_bNoFillMissing = false;
					WGInput.m_bUseShore = true;
					//CWVariables m_allowedDerivedVariables;
					//CSearchRadius m_searchRadius;

					CRandomGenerator rand(WGInput.m_seed);
					unsigned long seed = rand.Rand(1, CRandomGenerator::RAND_MAX_INT);

					m_pWeatherGenerator->SetSeed(seed);
					m_pWeatherGenerator->SetNbReplications(options.m_nb_replications);
					m_pWeatherGenerator->SetWGInput(WGInput);
					m_pWeatherGenerator->SetTarget(location);

					msg = m_pWeatherGenerator->Generate();

					if (msg)
					{
						std::bitset<CWeatherGenerator::NB_WARNING> warning = m_pWeatherGenerator->GetWarningBits();

						ostringstream stream;

						for (size_t r = 0; r < m_pWeatherGenerator->GetNbReplications() && msg; r++)
						{
							//write info and weather to the stream
							const CSimulationPoint& weather = m_pWeatherGenerator->GetWeather(r);


							CStatistic::SetVMiss(-999);
							CTM TM = weather.GetTM();
							//CWVariables variable(weather.GetVariables());

							//CWeatherFormat format(TM, variable);//get default format

							//string header = format.GetHeader() + "\n";
							//stream.write(header.c_str(), header.length());

							msg = ((CWeatherYears&)weather).SaveData(stream, TM, ',');

							output = stream.str();
						}   // for replication



						CWeatherGenerator::OutputWarning(warning, callback);
					}
				}
			}	// if (msg)

			timer.Stop();

		}
		catch (...)
		{
			int i;
			i = 0;
		}

		if (!msg)
			output = get_string(msg);

		return output;// get_string(msg, callback, output);
	}

}



namespace WBSF
{

	PYBIND11_MODULE(BioSIM_API, m)
	{

		py::class_<WeatherGenerator>(m, "WeatherGenerator")
			.def(py::init<const std::string &>())
			.def("Initialize", &WeatherGenerator::Initialize)
			.def("Generate", &WeatherGenerator::Generate)
			;


#ifdef VERSION_INFO
		m.attr("__version__") = VERSION_INFO;
#else
		m.attr("__version__") = "dev";
#endif
	}

}  // WBSF
