//#include "pch.h"
//Entity.cpp

#include "storage_credential.h"
#include "storage_account.h"
#include "blob/blob_client.h"
#include <boost/algorithm/string/predicate.hpp> 


#include "BioSIM_API.h"
#include <iostream>
#include "Basic/NormalsDatabase.h"
#include "Basic/WeatherDefine.h"

#include "Basic/Shore.h"
#include "basic/ModelStat.h"
#include "Geomatic/UtilGDAL.h"
#include "ModelBase/CommunicationStream.h"
#include "ModelBase/ModelInput.h"
#include "ModelBase/Model.h"
#include "Simulation/WeatherGradient.h"
#include "Simulation/WeatherGenerator.h"
#include "FileManager/FileManager.h"
#include "WeatherBasedSimulationString.h"

//#define _REMOVE_FPOS_SEEKPOS
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/copy.hpp>

#pragma warning(disable: 4275 4251 4005)
#include "gdal_priv.h"

#ifdef _OPENMP
#include <omp.h>
#endif


//using namespace boost;
using namespace std;
using namespace azure::storage_lite;

using namespace WBSF;
using namespace WBSF::WEATHER;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::NORMALS_DATA;
//
//namespace Core
//{
//	Entity::Entity(const char* name, float xPos, float yPos)
//		: m_Name(name), m_XPos(xPos), m_YPos(yPos)
//	{
//		std::cout << "Created the Entity object!" << std::endl;
//	}
//	void Entity::Move(float deltaX, float deltaY)
//	{
//		m_XPos += deltaX;
//		m_YPos += deltaY;
//		std::cout << "Moved " << m_Name << " to (" << m_XPos << ", " << m_YPos << ")." << std::endl;
//	}
//}
//



namespace WBSF
{

	static std::unique_ptr<CGlobalDLLData> pGLOBAL_DLL_DATA;


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
	//
	const char* CGlobalDLLData::NAME[NB_PAPAMS] = { "ModelsPath", "Azure", "Shore", "DEM" };



	void CGlobalDLLData::clear()
	{
		m_model_path.clear();
		m_Azure_DLL_file_path.clear();
		m_shore_file_path.clear();
		m_DEM_file_path.clear();
	}


	ERMsg CGlobalDLLData::parse(const std::string& str_init)
	{
		ERMsg msg;

		clear();

		StringVector args(str_init, "&");
		for (size_t i = 0; i < args.size(); i++)
		{
			//StringVector option(args[i], "=");
			string::size_type pos = args[i].find('=');
			if (pos != string::npos)
			{
				string key = args[i].substr(0, pos);
				string value = args[i].substr(pos + 1);

				//auto it = std::find(begin(NAME), end(NAME), MakeUpper(option[0]));
				//string str1 = option[0];
				auto it = std::find_if(begin(NAME), end(NAME), [&str1 = key](const auto& str2) { return boost::iequals(str1, str2); });
				if (it != end(NAME))
				{
					size_t o = distance(begin(NAME), it);
					switch (o)
					{
					case MODELS_PATH: m_model_path = value; break;
					case AZURE_DLL: m_Azure_DLL_file_path = value; break;
					case SHORE: m_shore_file_path = value; break;
					case DEM: m_DEM_file_path = value; break;
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
	};

	std::string CBioSIM_API_GlobalData::InitGlobalData(const std::string& str_init)
	{
		ERMsg msg;

		if (pGLOBAL_DLL_DATA.get() == nullptr)
			pGLOBAL_DLL_DATA.reset(new CGlobalDLLData);

		msg = pGLOBAL_DLL_DATA->parse(str_init);

		if (msg)
		{
			try
			{
				CCallback callback;

				if (!pGLOBAL_DLL_DATA->m_model_path.empty())
				{
					if (!WBSF::IsPathEndOk(pGLOBAL_DLL_DATA->m_model_path))
						pGLOBAL_DLL_DATA->m_model_path += "/";
				}

				if (!pGLOBAL_DLL_DATA->m_Azure_DLL_file_path.empty())
				{

					WBSF::CWeatherDatabase::set_azure_dll_filepath(pGLOBAL_DLL_DATA->m_Azure_DLL_file_path);
				}


				//if (!pGLOBAL_DLL_DATA->m_shore_file_path.empty())
				//{

					//m_CS.Enter();
					/*if (CShore::GetShore().get() == nullptr)
					{
						if (m_init.IsAzure())
						{
							blob_client client(account, 16);

							std::stringstream azure_stream;
							auto ret = client.download_blob_to_stream(m_init.m_container_name, m_init.m_shore_name, 0, 0, azure_stream).get();
							if (ret.success())
							{
								CApproximateNearestNeighborPtr pShore = make_shared<CApproximateNearestNeighbor>();

								*pShore << azure_stream;
								CShore::SetShore(pShore);
							}
							else
							{
								msg.ajoute("Failed to download shore, error: " + ret.error().code + ", " + ret.error().code_name);
							}
						}
						else
						{
							msg += CShore::SetShore(m_init.m_shore_name);
						}
					}*/


				msg += CShore::SetShore(pGLOBAL_DLL_DATA->m_shore_file_path);

				//}

				if (!pGLOBAL_DLL_DATA->m_DEM_file_path.empty())
				{
					pGLOBAL_DLL_DATA->m_pDEM.reset(new CGDALDatasetEx);
					msg += pGLOBAL_DLL_DATA->m_pDEM->OpenInputImage(pGLOBAL_DLL_DATA->m_DEM_file_path);
				}

				
			}
			catch (...)
			{
				int i;
				i = 0;
			}
		}

		return get_string(msg);
	}





	//******************************************************************************************************************************

	//"DefaultEndpointsProtocol","EndpointSuffix",
	const char* CWeatherGeneratorInit::NAME[NB_PAPAMS] = { "AccountName","AccountKey","ContainerName",/*"Shore",*/ "Normals", "Daily", "Hourly", "GRIBS"/*, "DEM"*/ };

	CWeatherGeneratorInit::CWeatherGeneratorInit()
	{
		clear();
	}

	void CWeatherGeneratorInit::clear()
	{
		m_account_name.clear();
		m_account_key.clear();
		m_container_name.clear();
		//	m_shore_name.clear();
		m_normal_name.clear();
		m_daily_name.clear();
		//m_DEM_name.clear();
	}

	ERMsg CWeatherGeneratorInit::parse(const std::string& str_init)
	{
		ERMsg msg;

		clear();

		StringVector args(str_init, "&");
		for (size_t i = 0; i < args.size(); i++)
		{
			//StringVector option(args[i], "=");
			string::size_type pos = args[i].find('=');
			if (pos != string::npos)
			{
				string key = args[i].substr(0, pos);
				string value = args[i].substr(pos + 1);

				//auto it = std::find(begin(NAME), end(NAME), MakeUpper(option[0]));
				//string str1 = option[0];
				auto it = std::find_if(begin(NAME), end(NAME), [&str1 = key](const auto& str2) { return boost::iequals(str1, str2); });
				if (it != end(NAME))
				{
					size_t o = distance(begin(NAME), it);
					switch (o)
					{
						//						case DEFAULT_ENDPOINTS_PROTOCOL:
					case ACCOUNT_NAME:m_account_name = value; break;
					case ACCOUNT_KEY:m_account_key = value; break;
						//						case ENDPOINT_SUFFIX:
					case CONTAINER_NAME:m_container_name = value; break;
						//case SHORE: m_shore_name = value; break;
					case NORMALS: m_normal_name = value; break;
					case DAILY: m_daily_name = value; break;
						//case DEM:m_DEM_name = value; break;
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
	};


	//******************************************************************************************************************************


	const char* CWeatherGeneratorOptions::PARAM_NAME[NB_PAPAMS] =
	{
		"VARIABLES", "SOURCE", "GENERATION", "REPLICATIONS",
		"ID", "NAME", "LATITUDE", "LONGITUDE", "ELEVATION", "SLOPE", "ORIENTATION",
		"NB_NEAREST_NEIGHBOR", "FIRST_YEAR", "LAST_YEAR","NB_YEARS",
		"SEED", "NORMALS_INFO", "COMPRESS"
	};

	CWeatherGeneratorOptions::CWeatherGeneratorOptions()
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

	ERMsg CWeatherGeneratorOptions::parse(const string& str_options)
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
	void CWeatherGeneratorOptions::GetWGInput(CWGInput& WGInput)const
	{
		//Load WGInput 
		//CWGInput WGInput;


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

		//return WGInput;
	}


	//******************************************************************************************************************************

	CCriticalSection m_CS;//to protect shore 


	CWeatherGeneratorAPI::CWeatherGeneratorAPI(const std::string&)
	{
	}



	std::string CWeatherGeneratorAPI::Initialize(const std::string& str_init)
	{
		ERMsg msg;



		msg = m_init.parse(str_init);

		if (msg)
		{
			try
			{

				CCallback callback;



				std::shared_ptr<storage_credential> cred;
				std::shared_ptr<storage_account> account;
				if (m_init.IsAzure())
				{
					cred = std::make_shared<shared_key_credential>(m_init.m_account_name, m_init.m_account_key);
					account = std::make_shared<storage_account>(m_init.m_account_name, cred, /* use_https */ true);
				}

				/*if (!m_init.m_shore_name.empty())
				{

					m_CS.Enter();
					if (CShore::GetShore().get() == nullptr)
					{
						if (m_init.IsAzure())
						{
							blob_client client(account, 16);

							std::stringstream azure_stream;
							auto ret = client.download_blob_to_stream(m_init.m_container_name, m_init.m_shore_name, 0, 0, azure_stream).get();
							if (ret.success())
							{
								CApproximateNearestNeighborPtr pShore = make_shared<CApproximateNearestNeighbor>();

								*pShore << azure_stream;
								CShore::SetShore(pShore);
							}
							else
							{
								msg.ajoute("Failed to download shore, error: " + ret.error().code + ", " + ret.error().code_name);
							}
						}
						else
						{
							msg += CShore::SetShore(m_init.m_shore_name);
						}
					}
					m_CS.Leave();
				}*/
				if (!m_init.m_normal_name.empty())
				{
					m_pNormalDB.reset(new CNormalsDatabase);


					if (m_init.IsAzure())
					{
						blob_client client(account, 16);
						std::stringstream azure_stream;
						auto ret = client.download_blob_to_stream(m_init.m_container_name, m_init.m_normal_name, 0, 0, azure_stream).get();
						if (ret.success())
						{
							try
							{
								boost::iostreams::filtering_istreambuf in;
								in.push(boost::iostreams::gzip_decompressor());
								in.push(azure_stream);
								std::istream incoming(&in);

								size_t version = 0;
								incoming.read((char*)(&version), sizeof(version));
								if (version == CNormalsDatabase::VERSION)
								{
									incoming >> *m_pNormalDB;
									m_pNormalDB->CreateAllCanals();//create here to be thread safe
								}
								else
								{
									msg.ajoute("Normal binary database (version = " + to_string(version) + ") was not created with he latest version (" + to_string(CNormalsDatabase::VERSION) + "). Rebuild new binary.");
								}

							}
							catch (const boost::iostreams::gzip_error& exception)
							{
								int error = exception.error();
								if (error == boost::iostreams::gzip::zlib_error)
								{
									//check for all error code    
									msg.ajoute(exception.what());
								}
							}
						}
						else
						{
							msg.ajoute("Failed to download Normals, error: " + ret.error().code + ", " + ret.error().code_name);
						}
					}
					else
					{
						if (IsEqual(GetFileExtension(m_init.m_normal_name), ".NormalsDB"))
						{
							msg += m_pNormalDB->Open(m_init.m_normal_name);
							if (msg)
								m_pNormalDB->OpenSearchOptimization(callback);
						}
						else if (IsEqual(GetFileExtension(m_init.m_normal_name), ".gz"))
						{
							msg += m_pNormalDB->LoadFromBinary(m_init.m_normal_name);
							if (msg)
								m_pNormalDB->CreateAllCanals();
						}
						else
						{
							msg.ajoute("Invalid Normals database extension: " + m_init.m_normal_name);
						}
					}

					if (!m_init.m_daily_name.empty())
					{
						m_pDailyDB.reset(new CDailyDatabase(200));


						if (m_init.IsAzure())
						{
							m_pDailyDB->m_account_name = m_init.m_account_name;
							m_pDailyDB->m_account_key = m_init.m_account_key;
							m_pDailyDB->m_container_name = m_init.m_container_name;
							m_pDailyDB->m_DB_blob = GetPath(m_init.m_daily_name) + GetFileTitle(m_init.m_daily_name);
							m_pDailyDB->LoadAzureDLL();


							blob_client client(account, 16);
							std::stringstream azure_stream;
							auto ret = client.download_blob_to_stream(m_init.m_container_name, m_init.m_daily_name, 0, 0, azure_stream).get();
							if (ret.success())
							{
								try
								{
									boost::iostreams::filtering_istreambuf in;
									in.push(boost::iostreams::gzip_decompressor());
									in.push(azure_stream);
									std::istream incoming(&in);

									size_t version = 0;
									incoming.read((char*)(&version), sizeof(version));

									if (version == CDailyDatabase::VERSION)
									{
										incoming >> *m_pDailyDB;
										m_pDailyDB->CreateAllCanals();//create here to be thread safe
									}
									else
									{
										msg.ajoute("Daily binary database (version = " + to_string(version) + ") was not created with he latest version (" + to_string(CNormalsDatabase::VERSION) + "). Rebuild new binary.");
									}

								}
								catch (const boost::iostreams::gzip_error& exception)
								{
									int error = exception.error();
									if (error == boost::iostreams::gzip::zlib_error)
									{
										//check for all error code    
										msg.ajoute(exception.what());
									}
								}

							}
							else
							{
								msg.ajoute("Failed to download Daily, error: " + ret.error().code + ", " + ret.error().code_name);
							}
						}
						else
						{
							if (IsEqual(GetFileExtension(m_init.m_daily_name), ".DailyDB"))
							{
								msg += m_pDailyDB->Open(m_init.m_daily_name, CDailyDatabase::modeRead, callback, true);
								if (msg)
									msg += m_pDailyDB->OpenSearchOptimization(callback);//open here to be thread safe
							}
							else if (IsEqual(GetFileExtension(m_init.m_daily_name), ".gz"))
							{

								//m_pDailyDB->m_DB_blob = GetPath(m_init.m_daily_name) + GetFileTitle(m_init.m_daily_name);
								//m_pDailyDB->LoadAzureDLL();

								msg += m_pDailyDB->LoadFromBinary(m_init.m_daily_name);
								if (msg)
									m_pDailyDB->CreateAllCanals();
							}
							else
							{
								msg.ajoute("Invalid Daily database extension: " + m_init.m_daily_name);
							}
						}
					}

					/*if (!m_init.m_DEM_name.empty())
					{
						if (m_init.IsAzure())
						{
							blob_client client(account, 16);
						}
						else
						{
							m_pDEM.reset(new CGDALDatasetEx);
							msg += m_pDEM->OpenInputImage(m_init.m_DEM_name);
						}
					}*/
				}

				if (msg)
				{
					m_pWeatherGenerator.reset(new CWeatherGenerator);
					m_pWeatherGenerator->SetNormalDB(m_pNormalDB);
					m_pWeatherGenerator->SetDailyDB(m_pDailyDB);
				}
			}
			catch (...)
			{
				int i;
				i = 0;
			}
		}

		return get_string(msg);
	}


	ERMsg CWeatherGeneratorAPI::ComputeElevation(double latitude, double longitude, double& elevation)
	{
		ERMsg msg;

		if (pGLOBAL_DLL_DATA->m_pDEM && pGLOBAL_DLL_DATA->m_pDEM->IsOpen())
		{
			CGeoPoint pt(longitude, latitude, PRJ_WGS_84);

			CGeoPointIndex xy = pGLOBAL_DLL_DATA->m_pDEM->GetExtents().CoordToXYPos(pt);
			if (pGLOBAL_DLL_DATA->m_pDEM->GetExtents().IsInside(xy))
			{
				elevation = pGLOBAL_DLL_DATA->m_pDEM->ReadPixel(0, xy);

				if (fabs(elevation - pGLOBAL_DLL_DATA->m_pDEM->GetNoData(0)) < 0.1)
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


	CTeleIO CWeatherGeneratorAPI::Generate(const std::string& str_options)
	{
		ASSERT(m_pWeatherGenerator != nullptr);

		ERMsg msg;
		CCallback callback;
		CTeleIO output;
		//string output;

		try
		{
			CTimer timer(TRUE);
			CWeatherGeneratorOptions options;
			msg = options.parse(str_options);

			if (msg)
			{

				if (options.m_elevation < -100)
					msg = ComputeElevation(options.m_latitude, options.m_longitude, options.m_elevation);

				if (msg)
				{
					CLocation location(options.m_name, options.m_ID, options.m_latitude, options.m_longitude, options.m_elevation);

					//Load WGInput 
					CWGInput WGInput;
					options.GetWGInput(WGInput);

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
						//if (options.m_compress)
						output.m_data = compressed.str();
						//else
							//output.m_text = compressed.str();


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


	CTeleIO CWeatherGeneratorAPI::GetNormals(const std::string& str_options)
	{
		ERMsg msg;
		CCallback callback;
		CTeleIO output;

		try
		{
			CTimer timer(TRUE);
			CWeatherGeneratorOptions options;
			msg = options.parse(str_options);

			if (msg)
			{

				if (options.m_elevation < -100)
					msg = ComputeElevation(options.m_latitude, options.m_longitude, options.m_elevation);

				if (msg)
				{
					CLocation location(options.m_name, options.m_ID, options.m_latitude, options.m_longitude, options.m_elevation);

					//Load WGInput 
					//CWGInput WGInput = options.GetWGInput();
					CWGInput WGInput;
					options.GetWGInput(WGInput);

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
						output.m_data = compressed.str();
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


	void CWeatherGeneratorAPI::SaveNormals(std::ostream& out, const CNormalsStation& normals)
	{
		string end_line = "\r\n";

		//write header
		out << "Month";
		for (size_t f = 0; f != NB_FIELDS; f++)
			out << ',' << GetFieldHeader(f);

		out << end_line;

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

			out << line << end_line;
		}
	}


	//******************************************************************************************************************************
	const char* CModelExecutionOptions::PARAM_NAME[NB_PAPAMS] =
	{
		"PARAMETERS", "REPLICATIONS", "SEED", "COMPRESS"
	};

	CModelExecutionOptions::CModelExecutionOptions()
	{
		m_replications = 1;
		m_seed = 0;
		m_compress = true;
	}

	ERMsg CModelExecutionOptions::parse(const string& str_options)
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
	ERMsg CModelExecutionOptions::GetModelInput(const CModel& model, CModelInput& modelInput)const
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


	const char* CModelExecutionAPI::NAME[NB_PAPAMS] = { "MODEL" };


	CModelExecutionAPI::CModelExecutionAPI(const std::string&)
	{

	}

	std::string CModelExecutionAPI::Initialize(const std::string& str_options)
	{
		ERMsg msg;

		try
		{
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

							string model_file_path = option[1];
							if(WBSF::GetPath(model_file_path).empty() && !pGLOBAL_DLL_DATA->m_model_path.empty())
							{
								model_file_path = pGLOBAL_DLL_DATA->m_model_path + model_file_path;
								if(!IsEqualNoCase( GetFileExtension(model_file_path), ".mdl" ))
									model_file_path += ".mdl";
							}

							msg += m_pModel->Load(model_file_path);
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


	CTeleIO CModelExecutionAPI::Execute(const std::string& str_options, const CTeleIO& input)
	{
		ASSERT(m_pModel);

		CTeleIO output;

		if (m_pModel.get() == nullptr)
		{
			output.m_msg = "Model is not define yet. Call Initialize first";
			return output;
		}



		ERMsg msg;
		CCallback callback;

		CModelExecutionOptions options;
		msg += options.parse(str_options);
		if (msg)
		{

			CSimulationPointVector simulationPoints;
			msg += LoadWeather(input, simulationPoints);


			//simulationPoints.GetVariables() == ;

			CModelInput modelInput;
			msg += options.GetModelInput(*m_pModel, modelInput);
			CParameterVector pOut = m_pModel->GetOutputDefinition().GetParametersVector();
			string header;
			for (size_t i = 0; i < pOut.size(); i++)
				header += (i == 0 ? "" : ",") + pOut[i].m_name;
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
				ASSERT(!simulationPoints.empty());
				//verify nb years and variables

				if (simulationPoints.begin()->GetNbYears() < m_pModel->GetNbYearMin() || simulationPoints.begin()->GetNbYears() > m_pModel->GetNbYearMax())
					msg.ajoute(FormatMsg(IDS_BSC_NB_YEAR_INVALID, ToString(simulationPoints.begin()->GetNbYears()), m_pModel->GetName(), ToString(m_pModel->GetNbYearMin()), ToString(m_pModel->GetNbYearMax())));

				msg += m_pModel->VerifyInputs(simulationPoints.begin()->GetSSIHeader(), simulationPoints.GetVariables());
			}

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

				size_t m_seedType = 0; // RANDOM_FOR_ALL
				size_t total_runs = simulationPoints.size() * options.m_replications;
				size_t total_seeds = (m_seedType < 2) ? total_runs : options.m_replications;

				CRandomGenerator rand(m_seedType % 2 ? CRandomGenerator::FIXE_SEED : CRandomGenerator::RANDOM_SEED);
				vector<unsigned long> seeds;
				for (size_t i = 0; i < total_seeds; i++)
					seeds.push_back(rand.Rand(1, CRandomGenerator::RAND_MAX_INT));



				for (size_t s = 0; s < simulationPoints.size() && msg; s++)
				{
					//write info and weather to the stream
					const CSimulationPoint& simulationPoint = simulationPoints[s];
					ASSERT(!simulationPoint.empty());

					for (size_t r = 0; r < options.m_replications && msg; r++)
					{
						stringstream inStream;
						stringstream outStream;

						//get transfer info
						size_t seed_pos = m_seedType < 2 ? s * options.m_replications + r : r;
						CTransferInfoIn info;
						FillTransferInfo(*m_pModel, simulationPoint, modelInput, seeds[seed_pos], s * options.m_replications + r, options.m_replications * simulationPoints.size(), info);
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
				//if (options.m_compress)
				output.m_data = compressed.str();
				//else
					//output.m_text = compressed.str();

			}
		}

		output.m_msg = get_string(msg);
		output.m_comment = ANSI_UTF8(callback.GetMessages());

		return output;
	}


	void CModelExecutionAPI::FillTransferInfo(const CModel& model, const CLocation& locations, const CModelInput& modelInput, size_t seed, size_t r, size_t n_r, CTransferInfoIn& info)
	{
		ASSERT(model.GetTransferFileVersion() == CModel::VERSION_STREAM);

		//	CTransferInfoIn info;
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

		//		return info;
	}

	std::string CModelExecutionAPI::GetWeatherVariablesNeeded()
	{
		ASSERT(m_pModel);
		if (m_pModel.get() == nullptr)
			return "Model is not define yet. Call Initialize first";
		string variable = m_pModel->m_variables.to_string();
		ReplaceString(variable, " ", "+");

		return ANSI_UTF8(variable);
	}


	std::string CModelExecutionAPI::GetDefaultParameters()const
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
			params += (i > 0 ? "+" : "") + modelInput[i].m_name + ":" + modelInput[i].m_value;
		}


		return ANSI_UTF8(params);
	}

	std::string CModelExecutionAPI::Help()const
	{
		ASSERT(m_pModel);
		if (m_pModel.get() == nullptr)
		{
			return "Model is not define yet. Call Initialize first";
		}

		return ANSI_UTF8(m_pModel->GetDocumentation());
	}



	//**********************************************************************************************************************
	ERMsg SaveWeather(const CSimulationPointVector& simulationPoints, CTeleIO& IO)
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


		IO.m_data = compressed.str();


		return msg;
	}

	ERMsg LoadWeather(const CTeleIO& IO, CSimulationPointVector& simulationPoints)
	{
		ERMsg msg;

		CLocation location = GetLocation(IO.m_metadata);
		//zen::from_string(loc, IO.m_metadata);

		try
		{
			boost::iostreams::filtering_istreambuf in;
			if (IO.m_compress)
				in.push(boost::iostreams::gzip_decompressor());

			std::string s = IO.m_data;
			stringstream stream(s);
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



