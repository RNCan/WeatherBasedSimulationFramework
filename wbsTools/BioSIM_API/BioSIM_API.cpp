#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include "BioSIM_API.h"
#include "Basic/NormalsDatabase.h"
#include "Basic/WeatherDefine.h"
#include "Basic/DynamicRessource.h"
#include "Simulation/WeatherGradient.h"
#include "Simulation/WeatherGenerator.h"
#include "ModelBase/CommunicationStream.h"
#include "FileManager/FileManager.h"
#include "WeatherBasedSimulationString.h"
#include "Basic/Shore.h"

 
//#include "json\json11.hpp" 
//using namespace json11;
//#pragma warning(disable: 4275 4251)
//#include "GDAL_priv.h"
//#include "ogr_srs_api.h"
#include "Geomatic/UtilGDAL.h"

//#define _REMOVE_FPOS_SEEKPOS
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/gzip.hpp>
//#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/copy.hpp>

#pragma warning(disable: 4275 4251 4005)

#include "gdal_priv.h"



//using namespace boost;
using namespace std;

namespace py = pybind11;

using namespace WBSF;
using namespace WBSF::WEATHER;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::NORMALS_DATA;
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


	static std::string serialize_xml(const zen::XmlElement& xml)
	{
		std::string stream;
		zen::implementation::serialize(xml, stream, "\n", "    ", 0);

		return stream;

	}

	static CLocation GetLocation(const std::string& metadata)
	{
		CLocation location;


		zen::XmlDoc doc = zen::parse(metadata);
		zen::XmlElement* pLoc = doc.root().getChild(CLocation::GetXMLFlag());
		ASSERT(pLoc);
		if (pLoc)
			readStruc(*pLoc, location);

		return location;
	}

	//******************************************************************************************************************************

	ERMsg CSfcGribExtractor::open(const std::string& gribsList)
	{
		ERMsg msg;

		msg = m_gribsList.load(gribsList);
		for (auto it = m_gribsList.begin(); it != m_gribsList.end(); it++)
		{
			if (m_sfcDS.find(it->first) == m_sfcDS.end())
			{
				m_sfcDS[it->first].reset(new CSfcDatasetCached);
				msg += m_sfcDS[it->first]->open(it->second, true);
			}
		}

		if (!m_sfcDS.empty())
		{
			GEO_2_WEA = CProjectionTransformation(PRJ_WGS_84, m_sfcDS.begin()->second->GetPrjID());
			m_extents = m_sfcDS.begin()->second->GetExtents();
		}

		return msg;
	}

	void CSfcGribExtractor::load_all()
	{
	}

	void CSfcGribExtractor::extract(CGeoPoint pt, CWeatherYears& data)
	{
		//for optimization, we select only 2 wind variables
		//if (var.test(H_WNDS) && var.test(H_WNDD))
		//{
		//	var.reset(H_UWND);
		//	var.reset(H_VWND);
		//}
		//else if (var.test(H_UWND) && var.test(H_VWND))
		//{
		//	var.reset(H_WNDS);
		//	var.reset(H_WNDD);
		//}
		//else
		//{
		//	var.reset(H_UWND);
		//	var.reset(H_VWND);
		//	var.reset(H_WNDS);
		//	var.reset(H_WNDD);
		//}
		//sfcDS.m_variables_to_load = var;

		pt.Reproject(GEO_2_WEA);
		if (m_extents.IsInside(pt))
		{
			for (auto it = m_sfcDS.begin(); it != m_sfcDS.end(); it++)
			{
				//CTRef localTRef = CTimeZones::UTCTRef2LocalTRef(TRef, stations[i]);
				CWeatherDay& day = data.GetDay(it->first);
				//it->second->get_weather(pt, data);//estimate weather at location
				it->second->get_nearest(pt, day);
			}
		}
	}



	//******************************************************************************************************************************


	const char* WeatherGeneratorOptions::PARAM_NAME[NB_PAPAMS] =
	{
		"VARIABLES", "SOURCE", "GENERATION", "REPLICATIONS",
		"ID", "NAME", "LATITUDE", "LONGITUDE", "ELEVATION", "SLOPE", "ORIENTATION",
		"NB_NEAREST_NEIGHBOR", "FIRST_YEAR", "LAST_YEAR","NB_YEARS",
		"SEED", "NORMALS_INFO", "COMPRESS"
	};

	WeatherGeneratorOptions::WeatherGeneratorOptions()
	{
		m_sourceType = 1;
		m_generationType = 1;
		m_variables = "TN+T+TX+P";
		m_normals_info = "1981-2010";
		m_latitude = -999;
		m_longitude = -999;
		m_elevation = -999;
		m_slope = -999;
		m_orientation = -999;
		m_nb_nearest_neighbor = 4;
		m_replications = 1;
		m_nb_years = 1;
		m_first_year = CTRef::GetCurrentTRef().GetYear();
		m_last_year = CTRef::GetCurrentTRef().GetYear();
		m_seed = 0;
		m_compress = true;
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
				auto it = std::find(begin(PARAM_NAME), end(PARAM_NAME), MakeUpper(option[0]));
				if (it != end(PARAM_NAME))
				{
					size_t o = distance(begin(PARAM_NAME), it);
					switch (o)
					{
					case VARIABLES:				m_variables = option[1]; ReplaceString(m_variables, "+", " ");  break;
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
					case KEY_ID: m_ID = option[1]; break;
					case NAME:m_name = option[1]; break;
					case LATITUDE:
					{
						m_latitude = ToDouble(option[1]);
						if (m_latitude < -90 || m_latitude>90)
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
						if (m_first_year < 1900 || m_first_year>GetCurrentYear())
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
					case NB_YEARS:			m_nb_years = ToInt(option[1]); break;
					case SEED:				   m_seed = ToInt(option[1]); break;
					case COMPRESS:			m_compress = ToBool(option[1]); break;
					case REPLICATIONS:		   m_replications = ToInt(option[1]); break;
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
			msg.ajoute("Last year (" + to_string(m_last_year) + ") must be greater or equal to first year (" + to_string(m_first_year) + ") .");


		return msg;
	}

	//Load WGInput 
	CWGInput WeatherGeneratorOptions::GetWGInput()const
	{
		//Load WGInput 
		CWGInput WGInput;


		WGInput.m_variables = m_variables;
		WGInput.m_sourceType = m_sourceType;
		WGInput.m_generationType = m_generationType;
		WGInput.m_nbNormalsYears = m_nb_years;
		WGInput.m_firstYear = m_first_year;
		WGInput.m_lastYear = m_last_year;
		WGInput.m_bUseForecast = true;
		WGInput.m_bUseGribs = false;

		WGInput.m_nbNormalsStations = m_nb_nearest_neighbor;
		WGInput.m_nbDailyStations = m_nb_nearest_neighbor;
		WGInput.m_nbHourlyStations = m_nb_nearest_neighbor;
		WGInput.m_nbGribPoints = m_nb_nearest_neighbor;
		//WGInput.m_albedo;
		WGInput.m_seed = m_seed;
		WGInput.m_bXValidation = false;
		WGInput.m_bSkipVerify = true;
		WGInput.m_bNoFillMissing = false;
		WGInput.m_bUseShore = true;
		//CWVariables m_allowedDerivedVariables;
		//CSearchRadius m_searchRadius;

		return WGInput;
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
			GDALSetCacheMax64(128 * 1024 * 1024);
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
							m_pGribsDB.reset(new CSfcGribExtractor);
							msg += m_pGribsDB->open(option[1]);
							if (msg)
								m_pGribsDB->load_all();

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
			//m_pWeatherGenerator->SetGribsDB(m_pGribsDB);
		}
		catch (...)
		{
			int i;
			i = 0;
		}

		return get_string(msg);
	}


	ERMsg WeatherGenerator::ComputeElevation(double latitude, double longitude, double& elevation)
	{
		ERMsg msg;

		if (m_pDEM && m_pDEM->IsOpen())
		{
			CGeoPoint pt(longitude, latitude, PRJ_WGS_84);

			CGeoPointIndex xy = m_pDEM->GetExtents().CoordToXYPos(pt);
			if (m_pDEM->GetExtents().IsInside(xy))
			{
				elevation = m_pDEM->ReadPixel(0, xy);

				if (fabs(elevation - m_pDEM->GetNoData(0)) < 0.1)
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

		return msg;
	}


	teleIO WeatherGenerator::Generate(const std::string& str_options)
	{
		ERMsg msg;
		CCallback callback;
		teleIO output;
		//string output;

		try
		{
			CTimer timer(TRUE);
			WeatherGeneratorOptions options;
			msg = options.parse(str_options);

			if (msg)
			{

				if (options.m_elevation < -100)
					msg = ComputeElevation(options.m_latitude, options.m_longitude, options.m_elevation);

				if (msg)
				{
					CLocation location(options.m_name, options.m_ID, options.m_latitude, options.m_longitude, options.m_elevation);

					//Load WGInput 
					CWGInput WGInput = options.GetWGInput();

					//init random generator
					CRandomGenerator rand(WGInput.m_seed);
					unsigned long seed = rand.Rand(1, CRandomGenerator::RAND_MAX_INT);

					m_pWeatherGenerator->SetSeed(seed);
					m_pWeatherGenerator->SetNbReplications(options.m_replications);
					m_pWeatherGenerator->SetWGInput(WGInput);
					m_pWeatherGenerator->SetTarget(location);

					msg = m_pWeatherGenerator->Generate();

					if (msg)
					{
						std::bitset<CWeatherGenerator::NB_WARNING> warning = m_pWeatherGenerator->GetWarningBits();

						// Compress
						std::stringstream sender;
						CStatistic::SetVMiss(-999);

						boost::iostreams::filtering_streambuf<boost::iostreams::input> out;
						if (options.m_compress)
						{
							boost::iostreams::gzip_params p;
							p.file_name = "data.csv";
							out.push(boost::iostreams::gzip_compressor(p));
						}
						out.push(sender);


						for (size_t r = 0; r < m_pWeatherGenerator->GetNbReplications() && msg; r++)
						{
							//write info and weather to the stream
							const CSimulationPoint& weather = m_pWeatherGenerator->GetWeather(r);
							CTM TM = weather.GetTM();
							msg = ((CWeatherYears&)weather).SaveData(sender, TM, ',');

						}   // for replication


						zen::XmlElement root("Metadata");
						zen::writeStruc(location, root.addChild(CLocation::GetXMLFlag()));
						output.m_metadata = serialize_xml(root);

						std::stringstream compressed;
						boost::iostreams::copy(out, compressed);

						output.m_compress = options.m_compress;
						if (options.m_compress)
							output.m_data = py::bytes(compressed.str());
						else
							output.m_text = compressed.str();


						//catch (const boost::iostreams::gzip_error& exception)
						//{
						//	int error = exception.error();
						//	if (error == boost::iostreams::gzip::zlib_error)
						//	{
						//		//check for all error code    
						//		msg.ajoute(exception.what());
						//	}
						//}

						//file.close();


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

		output.m_msg = get_string(msg);
		output.m_comment = ANSI_UTF8(callback.GetMessages());

		return output;
	}


	teleIO WeatherGenerator::GenerateGribs(const std::string& str_options)
	{
		ERMsg msg;
		CCallback callback;
		teleIO output;

		try
		{
			//GDALSetCacheMax64(128 * 1024 * 1024);

			CTimer timer(TRUE);
			WeatherGeneratorOptions options;
			msg = options.parse(str_options);

			if (msg)
			{

				if (options.m_elevation < -100)
					msg = ComputeElevation(options.m_latitude, options.m_longitude, options.m_elevation);

				if (msg)
				{
					CLocation location(options.m_name, options.m_ID, options.m_latitude, options.m_longitude, options.m_elevation);


					CWeatherYears data;
					m_pGribsDB->extract(location, data);

					std::stringstream stream;
					CStatistic::SetVMiss(-999);

					boost::iostreams::filtering_streambuf<boost::iostreams::input> out;
					if (options.m_compress)
					{
						boost::iostreams::gzip_params p;
						p.file_name = "data.csv";
						out.push(boost::iostreams::gzip_compressor(p));
					}
					out.push(stream);


					CTM TM = data.GetTM();
					CWeatherFormat format(TM, options.m_variables);//get default format
					msg = data.SaveData(stream, TM, format, ',');

					zen::XmlElement root("Metadata");
					zen::writeStruc(location, root.addChild(CLocation::GetXMLFlag()));
					output.m_metadata = serialize_xml(root);



					std::stringstream compressed;
					boost::iostreams::copy(out, compressed);

					output.m_compress = options.m_compress;
					if (options.m_compress)
						output.m_data = py::bytes(compressed.str());
					else
						output.m_text = compressed.str();
				}
			}

			timer.Stop();

		}
		catch (...)
		{
			int i;
			i = 0;
		}


		output.m_msg = get_string(msg);
		output.m_comment = ANSI_UTF8(callback.GetMessages());


		return output;
	}

	teleIO WeatherGenerator::GetNormals(const std::string& str_options)
	{
		ERMsg msg;
		CCallback callback;
		teleIO output;

		try
		{
			CTimer timer(TRUE);
			WeatherGeneratorOptions options;
			msg = options.parse(str_options);

			if (msg)
			{

				if (options.m_elevation < -100)
					msg = ComputeElevation(options.m_latitude, options.m_longitude, options.m_elevation);

				if (msg)
				{
					CLocation location(options.m_name, options.m_ID, options.m_latitude, options.m_longitude, options.m_elevation);

					//Load WGInput 
					CWGInput WGInput = options.GetWGInput();

					CRandomGenerator rand(WGInput.m_seed);
					unsigned long seed = rand.Rand(1, CRandomGenerator::RAND_MAX_INT);

					m_pWeatherGenerator->SetSeed(seed);
					m_pWeatherGenerator->SetNbReplications(options.m_replications);
					m_pWeatherGenerator->SetWGInput(WGInput);
					m_pWeatherGenerator->SetTarget(location);

					CNormalsStation normals;
					msg = m_pWeatherGenerator->GetNormals(normals, callback);


					if (msg)
					{
						// Compress
						std::stringstream stream;
						//CStatistic::SetVMiss(-999);

						boost::iostreams::filtering_streambuf<boost::iostreams::input> out;
						if (options.m_compress)
						{
							boost::iostreams::gzip_params p;
							p.file_name = "data.csv";
							out.push(boost::iostreams::gzip_compressor(p));
						}
						out.push(stream);

						SaveNormals(stream, normals);
						zen::XmlElement root("Metadata");
						zen::writeStruc(location, root.addChild(CLocation::GetXMLFlag()));
						output.m_metadata = serialize_xml(root);


						std::stringstream compressed;
						boost::iostreams::copy(out, compressed);

						output.m_compress = options.m_compress;
						if (options.m_compress)
							output.m_data = py::bytes(compressed.str());
						else
							output.m_text = compressed.str();

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

		output.m_msg = get_string(msg);
		output.m_comment = ANSI_UTF8(callback.GetMessages());

		return output;

	}

	void WeatherGenerator::TestThreads(const std::string& str_options)
	{
		
		size_t nb_loops = stoi(str_options);

		std::vector<double> tmp;
		for (size_t i = 0; i < nb_loops; i++)
		{
			double x = 100 + ((double)rand())/RAND_MAX;
			double a = x * x;
			double b = sqrt(a);
			double c = log(b);
			tmp.push_back(c);
		}
		
		
	}

	void WeatherGenerator::SaveNormals(std::ostream& out, const CNormalsStation& normals)
	{
		//write header
		out << "Month";
		for (size_t f = 0; f != NB_FIELDS; f++)
			out << ',' << GetFieldHeader(f);

		out << endl;

		for (size_t m = 0; m != normals.size(); m++)
		{
			string line = FormatA("%02d", int(m + 1));

			for (size_t f = 0; f != normals[m].size(); f++)
			{
				if (!IsMissing(normals[m][f]))
				{
					string format = "%7." + to_string(GetNormalDataPrecision((int)f)) + "f";
					string value = FormatA(format.c_str(), normals[m][f]);
					line += "," + value;
				}
				else
				{
					line += ", -999.0";
				}

			}

			out << line << endl;
		}
	}

	//******************************************************************************************************************************




	const char* ModelExecutionOptions::PARAM_NAME[NB_PAPAMS] =
	{
		"PARAMETERS", "REPLICATIONS", "SEED", "COMPRESS"
	};

	ModelExecutionOptions::ModelExecutionOptions()
	{
		m_replications = 1;
		m_seed = 0;
		m_compress = true;
	}

	ERMsg ModelExecutionOptions::parse(const string& str_options)
	{
		ERMsg msg;

		StringVector args(str_options, "&");
		for (size_t i = 0; i < args.size(); i++)
		{
			StringVector option(args[i], "=");
			if (option.size() == 2)
			{
				auto it = std::find(begin(PARAM_NAME), end(PARAM_NAME), MakeUpper(option[0]));
				if (it != end(PARAM_NAME))
				{
					size_t o = distance(begin(PARAM_NAME), it);
					switch (o)
					{
					case PARAMETERS:	m_parameters = option[1]; break;
					case SEED:			m_seed = ToInt(option[1]); break;
					case COMPRESS:		m_compress = ToBool(option[1]); break;
					case REPLICATIONS:	m_replications = ToInt(option[1]); break;
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

		return msg;
	}

	//Load WGInput 
	ERMsg ModelExecutionOptions::GetModelInput(const CModel& model, CModelInput& modelInput)const
	{
		ERMsg msg;

		//Get default parameters
		model.GetDefaultParameter(modelInput);

		//update parameters		
		StringVector args(m_parameters, "+");//parameters separate by space, must not have space in name
		for (size_t i = 0; i < args.size(); i++)
		{
			StringVector option(args[i], ":");
			if (option.size() == 2)
			{
				string name = option[0];
				auto it = find_if(modelInput.begin(), modelInput.end(), [name](const CModelInputParam& m) -> bool { return WBSF::IsEqual(m.m_name, name); });
				if (it != modelInput.end())
				{
					it->m_value = option[1];
				}
				else
				{
					msg.ajoute("Invalid model input parameters " + to_string(i + 1) + "( " + args[i] + ")");
				}
			}
			else
			{
				msg.ajoute("error in  model input parameters  " + to_string(i + 1) + "( " + args[i] + ")");
			}
		}

		return msg;
	}

	//******************************************************************************************************************************


	const char* ModelExecution::NAME[NB_PAPAMS] = { "MODEL" };


	ModelExecution::ModelExecution(const std::string &)
	{

	}

	std::string ModelExecution::Initialize(const std::string& str_options)
	{
		ERMsg msg;

		try
		{
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
						case MODEL:
						{

							m_pModel.reset(new CModel);
							msg += m_pModel->Load(option[1]);
							if (msg)
								msg += m_pModel->LoadDLL();

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
		}
		catch (...)
		{
			int i;
			i = 0;
		}

		return get_string(msg);
	}


	teleIO ModelExecution::Execute(const std::string& str_options, const teleIO& input)
	{
		ASSERT(m_pModel);

		teleIO output;

		if (m_pModel.get() == nullptr)
		{
			output.m_msg = "Model is not define yet. Call Initialize first";
			return output;
		}

		

		ERMsg msg;
		CCallback callback;

		ModelExecutionOptions options;
		msg += options.parse(str_options);
		if (msg)
		{

			CSimulationPointVector simulationPoints;
			msg += LoadWeather(input, simulationPoints);



			CModelInput modelInput;
			msg += options.GetModelInput(*m_pModel, modelInput);
			CParameterVector pOut = m_pModel->GetOutputDefinition().GetParametersVector();
			string header;
			for (size_t i = 0; i < pOut.size(); i++)
				header += (i==0?"":",") + pOut[i].m_name;
			//load files in memory for stream transfer
	//		stringstream staticDataStream;
			//msg = LoadStaticData(fileManager, model, modelInputVector.m_pioneer, staticDataStream);
			//if (msg)
			//	msg = model.SetStaticData(staticDataStream);

	//		staticDataStream.clear();//clear any bits set
		//	staticDataStream.str(std::string());

			//if (!msg)
				//return msg;

			if (msg)
			{
				// Compress
				std::stringstream stream;
				//CStatistic::SetVMiss(-999);

				boost::iostreams::filtering_streambuf<boost::iostreams::input> out;
				if (options.m_compress)
				{
					boost::iostreams::gzip_params p;
					p.file_name = "data.csv";
					out.push(boost::iostreams::gzip_compressor(p));
				}
				out.push(stream);


				CRandomGenerator rand(options.m_seed);
				vector<unsigned long> seeds(options.m_replications);
				for (vector<unsigned long>::iterator it = seeds.begin(); it != seeds.end(); it++)
					*it = rand.Rand(1, CRandomGenerator::RAND_MAX_INT);


				for (size_t s = 0; s < simulationPoints.size() && msg; s++)
				{
					//write info and weather to the stream
					const CSimulationPoint& simulationPoint = simulationPoints[s];
					ASSERT(!simulationPoint.empty());

					for (size_t r = 0; r < seeds.size() && msg; r++)
					{
						stringstream inStream;
						stringstream outStream;

						//get transfer info
						CTransferInfoIn info = FillTransferInfo(*m_pModel, simulationPoint, modelInput, seeds[r], s*seeds.size() + r, seeds.size()*simulationPoints.size());
						CCommunicationStream::WriteInputStream(info, simulationPoint, inStream);

						msg += m_pModel->RunModel(inStream, outStream);	// call DLL
						if (msg)
						{
							//get output from stream
							CTransferInfoOut infoOut;
							CModelStatVector result;
							msg += CCommunicationStream::ReadOutputStream(outStream, infoOut, result);
							//section.SetMissing(model.GetMissValue());
							if (msg)
							{
								result.SetHeader(header);
								result.Save(stream);
								//msg.ajoute(FormatMsg(IDS_SIM_SIMULATION_ERROR, ToString(1), location.m_name, location.m_ID));
							}

						}// if (msg)
					}//for all model replications
				}//for all weather replications




				std::stringstream compressed;
				boost::iostreams::copy(out, compressed);

				zen::XmlElement root("Metadata");
				zen::writeStruc(modelInput, root.addChild(CModelInput::GetXMLFlag()));
				output.m_metadata = serialize_xml(root);


				output.m_compress = options.m_compress;
				if (options.m_compress)
					output.m_data = py::bytes(compressed.str());
				else
					output.m_text = compressed.str();

			}
		}

		output.m_msg = get_string(msg);
		output.m_comment = ANSI_UTF8(callback.GetMessages());

		return output;
	}

	
	CTransferInfoIn ModelExecution::FillTransferInfo(const CModel& model, const CLocation& locations, const CModelInput& modelInput, size_t seed, size_t r, size_t n_r)
	{
		ASSERT(model.GetTransferFileVersion() == CModel::VERSION_STREAM);

		CTransferInfoIn info;
		info.m_transferTypeVersion = CModel::VERSION_STREAM;
		info.m_modelName = model.GetName();

		info.m_locCounter = CCounter(0, 1);
		info.m_paramCounter = CCounter(0, 1);
		info.m_repCounter = CCounter(r, n_r);


		info.m_loc = locations;
		info.m_inputParameters = modelInput.GetParametersVector();
		info.m_outputVariables = model.GetOutputDefinition().GetParametersVector();
		info.m_seed = seed;
		info.m_TM = model.m_outputTM;
		info.m_language = CRegistry::ENGLISH;

		return info;
	}

	std::string ModelExecution::GetWeatherVariablesNeeded()
	{
		ASSERT(m_pModel);
		if (m_pModel.get() == nullptr)
			return "Model is not define yet. Call Initialize first";
		string variable = m_pModel->m_variables.to_string();
		ReplaceString(variable, " ", "+");

		return ANSI_UTF8(variable);
	}


	std::string ModelExecution::GetDefaultParameters()const
	{
		ASSERT(m_pModel);
		if (m_pModel.get() == nullptr)
		{
			return "Model is not define yet. Call Initialize first";
		}

		string params;
		CModelInput modelInput;
		m_pModel->GetDefaultParameter(modelInput);

		//update parameters		
		for (size_t i = 0; i < modelInput.size(); i++)
		{
			params+=(i>0?"+":"") + modelInput[i].m_name + ":"+ modelInput[i].m_value;
		}


		return ANSI_UTF8(params);
	}

	std::string ModelExecution::Help()const
	{ 
		ASSERT(m_pModel);
		if (m_pModel.get() == nullptr)
		{
			return "Model is not define yet. Call Initialize first";
		}

		return ANSI_UTF8(m_pModel->GetDocumentation());
	}

	
	std::string ModelExecution::Test()const
	{
		return "123";
	}

	//**********************************************************************************************************************
	ERMsg SaveWeather(const CSimulationPointVector& simulationPoints, teleIO& IO)
	{
		ERMsg msg;

		CLocation loc = simulationPoints.front();
		string test = zen::to_string(loc, "Location", "1");

		// Compress
		std::stringstream sender;
		CStatistic::SetVMiss(-999);

		boost::iostreams::filtering_streambuf<boost::iostreams::input> out;
		if (IO.m_compress)
		{
			boost::iostreams::gzip_params p;
			p.file_name = "data.csv";
			out.push(boost::iostreams::gzip_compressor(p));
		}
		out.push(sender); 


		for (size_t r = 0; r < simulationPoints.size() && msg; r++)
		{
			//write info and weather to the stream
			const CSimulationPoint& weather = simulationPoints[r];
			CTM TM = weather.GetTM();
			msg = ((CWeatherYears&)weather).SaveData(sender, TM, ',');

		}   // for replication


		std::stringstream compressed;
		boost::iostreams::copy(out, compressed);

		if (IO.m_compress)
			IO.m_data = py::bytes(compressed.str());
		else
			IO.m_text = compressed.str();


		return msg;
	}

	ERMsg LoadWeather(const teleIO& IO, CSimulationPointVector& simulationPoints)
	{
		ERMsg msg;

		CLocation location = GetLocation(IO.m_metadata);
		//zen::from_string(loc, IO.m_metadata);

		try
		{
			boost::iostreams::filtering_istreambuf in;
			if (IO.m_compress)
				in.push(boost::iostreams::gzip_decompressor());

			std::string s = py::cast<std::string>(IO.m_data);
			stringstream stream(IO.m_compress ? s : IO.m_text);
			in.push(stream);

			std::istream incoming(&in);

			StringVector replications;
			string line;
			while (std::getline(incoming, line) && msg)
			{
				if (!line.empty())
				{
					bool bNewReplication = WBSF::Find(line.substr(0, 4), "Year");
					if (bNewReplication)
					{
						replications.push_back(line + "\r\n");
					}
					else
					{
						replications.back() += line + "\r\n";
					}//good number of column
				}//line not empty
			}//for all lines


			simulationPoints.resize(replications.size());
			for (size_t i = 0; i < replications.size(); i++)
			{
				((CLocation&)simulationPoints[i]) = location;
				simulationPoints[i].m_ID = to_string(i + 1);
				simulationPoints[i].m_name = "Replication" + to_string(i + 1);
				simulationPoints[i].Parse(replications[i]);
			}

		}//try
		catch (const boost::iostreams::gzip_error& exception)
		{
			int error = exception.error();
			if (error == boost::iostreams::gzip::zlib_error)
			{
				//check for all error code    
				msg.ajoute(exception.what());
			}
		}

		return msg;
	}
}


namespace WBSF
{

	PYBIND11_MODULE(BioSIM_API, m)
	{
		
		py::class_<teleIO>(m, "teleIO")
			.def(py::init<bool, const std::string&, const std::string&, const std::string&, const std::string&, const pybind11::bytes&>())
			.def_readonly("compress", &teleIO::m_compress)
			.def_readonly("msg", &teleIO::m_msg)
			.def_readonly("comment", &teleIO::m_comment)
			.def_readonly("metadata", &teleIO::m_metadata)
			.def_readonly("text", &teleIO::m_text)
			.def_readonly("data", &teleIO::m_data)
			.def("__repr__",[](const teleIO &IO) {return "compress= " + to_string(IO.m_compress) + ",msg="+IO.m_msg + ",comment="+IO.m_comment+",metadata="+IO.m_metadata+",Length of data=" + to_string(IO.m_compress?((std::string)IO.m_data).length():IO.m_text.length());})
			;

		py::class_<WeatherGenerator>(m, "WeatherGenerator")
			.def(py::init<const std::string &>())
			.def("Initialize", &WeatherGenerator::Initialize)
			.def("Generate", &WeatherGenerator::Generate)
			.def("GenerateGribs", &WeatherGenerator::GenerateGribs)
			.def("GetNormals", &WeatherGenerator::GetNormals)
			.def("TestThreads", &WeatherGenerator::TestThreads)
			;
		 
		py::class_<ModelExecution>(m, "Model")
			.def(py::init<const std::string &>())
			.def("Initialize", &ModelExecution::Initialize)
			.def("Execute", &ModelExecution::Execute)
			.def("GetWeatherVariablesNeeded", &ModelExecution::GetWeatherVariablesNeeded)
			.def("GetDefaultParameters", &ModelExecution::GetDefaultParameters)
			.def("Help", &ModelExecution::Help)
			.def("Test", &ModelExecution::Test)
			;


#ifdef VERSION_INFO
		m.attr("__version__") = VERSION_INFO;
#else
		m.attr("__version__") = "dev";
#endif
	}

}  // WBSF
