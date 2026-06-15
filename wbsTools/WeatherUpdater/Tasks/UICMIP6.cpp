#include "StdAfx.h"

#include "UICMIP6.h"
#include <boost\multi_array.hpp>
#include "Basic/units.hpp"
#include "Basic/Statistic.h"
#include "Basic\json\json11.hpp"
#include "Basic\CallcURL.h"
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
	const char* CUICMIP6::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "DownloadData", "CreateGribs", "FirstYear", "LastYear", "GeoDomain", "Model", "SSP", "ShowCURL" };
	const size_t CUICMIP6::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_BOOL, T_BOOL, T_STRING, T_STRING, T_COMBO_STRING, T_COMBO_STRING, T_COMBO_STRING, T_BOOL };
	//T_GEORECT


	const UINT CUICMIP6::ATTRIBUTE_TITLE_ID = IDS_UPDATER_CMIP5_P;
	const UINT CUICMIP6::DESCRIPTION_TITLE_ID = ID_TASK_CMIP5;

	static const std::string serverNASA = "nex-gddp-cmip6.s3-us-west-2.amazonaws.com";

	const char* CUICMIP6::CLASS_NAME() { static const char* THE_CLASS_NAME = "CMIP6";  return THE_CLASS_NAME; }
	CTaskBase::TType CUICMIP6::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUICMIP6::CLASS_NAME(), (createF)CUICMIP6::create);
	enum TFileNameComponent { C_VAR, C_FREQUENCY, C_MODEL, C_SSP, C_RUN, C_GN, C_YEAR, C_VERSION, NB_COMPONENTS };

	enum TSSP { SSP_HISTORICAL, SSP_126, SSP_245, SSP_370, SSP_585, NB_SSP };
	static const std::array<std::string, NB_SSP> SSP_NAME = { "historical", "ssp126", "ssp245", "ssp370", "ssp585" };


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
		case GEO_DOMAIN:		str = "World|Canada-USA"; break;
		case MODEL:				str = "ACCESS-CM2|ACCESS-ESM1-5|BCC-CSM2-MR|CanESM5|CMCC-ESM2|CNRM-CM6-1|CNRM-ESM2-1|EC-Earth3|EC-Earth3-Veg-LR|FGOALS-g3|GFDL-ESM4|GISS-E2-1-G|INM-CM4-8|INM-CM5-0|KACE-1-0-G|MIROC-ES2L|MPI-ESM1-2-HR|MPI-ESM1-2-LR|MRI-ESM2-0|NorESM2-LM|NorESM2-MM|TaiESM1|UKESM1-0-LL"; break;
		case SSP:				str = "historical|ssp126|ssp245|ssp370|ssp585"; break;
		};

		return str;

	}
	//ACCESS-CM2,ACCESS-ESM1-5,CanESM5,CMCC-ESM2,CNRM-CM6-1,CNRM-ESM2-1,EC-Earth3,EC-Earth3-Veg-LR,FGOALS-g3,GFDL-ESM4,GISS-E2-1-G,INM-CM4-8,INM-CM5-0,KACE-1-0-G,MIROC-ES2L,MPI-ESM1-2-HR,MPI-ESM1-2-LR,MRI-ESM2-0,NorESM2-LM,NorESM2-MM,TaiESM1,UKESM1-0-LL


	std::string CUICMIP6::Default(size_t i)const
	{
		std::string str;

		switch (i)
		{

		case WORKING_DIR:		str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "CMIP6\\"; break;
		case DOWNLOAD_DATA: 	str = "1"; break;
		case CREATE_GRIBS:  	str = "0"; break;
		case FIRST_YEAR:		str = "1951"; break;
		case LAST_YEAR:			str = "2100"; break;
		case GEO_DOMAIN:		str = "World"; break;
		case MODEL:				str = "CanESM5"; break;
		case SSP:				str = ""; break;
		case SHOW_CURL:			str = "0"; break;
		};

		return str;
	}



	//*******************************************************************************************************


	//https://pcmdi.llnl.gov/search/cmip5/
	const char* CUICMIP6::VARIABLES_NAMES[NB_CMIP6_VARIABLES] = { "tasmin", "tasmax", "pr", "huss", "sfcWind" };//, "rsds"


	string GetPeriod(const std::string& filePath)
	{
		string title = GetFileTitle(filePath);
		return title.substr(title.length() - 17);
	}


	ERMsg CUICMIP6::Execute(CCallback& callback)
	{
		ERMsg msg;

		//Create elevation
		string working_dir = GetDir(WORKING_DIR);

		/*string new_orog_filepath = working_dir + "orog_fx_gn.tif";
		if (!FileExists(new_orog_filepath))
		{
			string orog_filepath = working_dir + "orog_fx_gn.nc";
			msg += save_orog(orog_filepath, new_orog_filepath);
		}*/




		//return ApplyCC(callback);

		bool bDownload = as<bool>(DOWNLOAD_DATA);
		if (bDownload)
		{
			//msg = Download(callback);
			//msg.ajoute("Download of CMPI6 data is not implemented yet");

			msg += DownloadFilesIndex(callback);
			if (msg)
			{
				msg += DownloadDataNASA(callback);
			}
		}


		bool bCreateGribs = as<bool>(CREATE_GRIBS);
		if (bCreateGribs)
		{
			msg = CreateDailyGribs(callback);
		}


		//	else
		//	{

		//		//CLocationVector loc;
		//		////msg = loc.Load("D:\\Travaux\\CMIP6\\Loc\\NormalsCanada-USA1981-2010subset100kmClean.csv");
		//		//msg = loc.Load("D:\\Travaux\\CMIP6\\Loc\\Dayton.csv");

		//		//ofStream out;
		//		////msg += out.open("D:\\Travaux\\CMIP6\\Output\\replications.csv");
		//		////out << "KeyID,ripf,Month,Variable,Value" << endl;
		//		//msg += out.open("D:\\Travaux\\CMIP6\\Output\\replications_Dayton.csv");
		//		//out << "KeyID,r,i,p,f,Year,Month,Tmin,Tmax,Prcp,SpeH,WindS" << endl;


		//		//string model = "CanESM5";
		//		//string ssp = "ssp245";
		//		//string filepath_out = "G:\\MMG\\Replication.mmg";

		//		//string ripf = FormatA("r%di%dp%df%d", 1, 1, 1, 1);
		//		//CreateMonthlyGribs(filepath_out, ripf, callback);
		//		//
		//		////static const char* REPLICATION[6] = { "r10i1p1f1", "r10i1p2f1", "r11i1p1f1", "r11i1p2f1", "r12i1p1f1", "r12i1p2f1" };
		//		//for (size_t r = 1; r < 20 && msg; r++)
		//		//{
		//		//	for (size_t p = 1; p < 3 && msg; p++)
		//		//	{
		//		//		string ripf = FormatA("r%di%dp%df%d", r, 1, p, 1);
		//		//		//msg += CreateMonthlyGribs(filepath_out, ripf, callback);


		//		//		if (msg)
		//		//		{
		//		//			//Create csv file
		//		//			string filePathOut = filepath_out;
		//		//			WBSF::SetFileTitle(filePathOut, WBSF::GetFileTitle(filePathOut) + "_" + model + "_" + ssp + "_" + ripf);


		//		//			CMonthlyMeanGrid MMG;
		//		//			if (MMG.Open(filePathOut, callback))
		//		//			{
		//		//				for (size_t l = 0; l < loc.size() && msg; l++)
		//		//				{
		//		//					//double monthlyMean[12][NORMALS_DATA::NB_FIELDS];
		//		//					//if (MMG.GetNormals(1981, 30, 4, 500000, 2, loc[l], monthlyMean, callback))
		//		//					//for (size_t y = 0; y < 30 && msg; y++)
		//		//					//{
		//		//						//int year = int(1981 + y);
		//		//						//for (size_t m = 0; m < 12 && msg; m++)
		//		//						//{
		//		//							/*for (size_t f = 0; f < NB_FIELDS && msg; f++)
		//		//							{
		//		//								if (f == TMIN_MN || f == TMAX_MN || f == PRCP_TT || f == SPEH_MN || f == WNDS_MN)
		//		//								{
		//		//									string var_name = GetFieldHeader(f);
		//		//									if (f == SPEH_MN)
		//		//										var_name = "SPEH_MN";
		//		//									string tmp = FormatA("%s,%s,%2d,%s,%.3lg", loc[l].m_ID.c_str(), ripf.c_str(), m + 1, var_name.c_str(), monthlyMean[m][f]);
		//		//									out << tmp << endl;
		//		//								}
		//		//							}*/
		//		//							//}
		//		//					std::vector< std::array<std::array<float, NORMALS_DATA::NB_FIELDS>, 12>> values;
		//		//					if (MMG.GetMonthlyValues(1981, 30, 4, 500000, 2, loc[l], values, callback))
		//		//					{
		//		//						for (size_t y = 0; y < 30 && msg; y++)
		//		//						{

		//		//							int year = int(1981 + y);
		//		//							for (size_t m = 0; m < 12 && msg; m++)
		//		//							{
		//		//								string tmp = FormatA("%s,%d,%d,%d,%d,%4d,%02d,%.2f,%.2f,%.2f,%.2f,%.2f", loc[l].m_ID.c_str(), r, 1, p, 1, year, m + 1, values[y][m][TMIN_MN], values[y][m][TMAX_MN], values[y][m][PRCP_TT], values[y][m][SPEH_MN], values[y][m][WNDS_MN]);
		//		//								out << tmp << endl;
		//		//								msg += callback.StepIt(0);
		//		//							}//for all month
		//		//						}//for all year
		//		//					}//if msg

		//		//				}//for all loc
		//		//			}
		//		//		}
		//		//	}
		//		//}

		//		//out.close();
		//	}

		//}





		return msg;
	}

	ERMsg CUICMIP6::Download(CCallback& callback)
	{
		ERMsg msg;

		//https://handle-esgf.dkrz.de/lp/21.14100/5db1dcab-86d0-3249-814b-c0840a3fb0e0
		//https://handle-esgf.dkrz.de/lp/21.14100/313f4894-5008-492e-a888-a2a2b10260e6
		//https://handle-esgf.dkrz.de/lp/21.14100/313f4894-5008-492e-a888-a2a2b10260e6


		//msg += DownloadFix("sftlf", callback);
		//msg += DownloadFix("orog", callback);

		msg += DownloadFilesIndex(callback);
		if (msg)
		{
			msg += DownloadDataNASA(callback);
		}

		return msg;
	}

	using namespace json11;

	bool GoodHDF(const string& filepath)
	{
		bool bGood = false;
		ifStream stream;
		if (stream.open(filepath))
		{
			char test[5] = { 0 };
			stream.read(&(test[0]), 4);
			stream.close();
			if (string(test) == "‰HDF")
				bGood = true;

			stream.close();
		}

		return bGood;
	}

	ERMsg CUICMIP6::DownloadFix(string prefix, CCallback& callback)
	{
		ERMsg msg;

		string model = Get(MODEL);
		string working_dir = GetDir(WORKING_DIR);

		string output_filepath = working_dir + model + "\\" + prefix + "_fx_" + model + ".nc";

		if (!FileExists(output_filepath))
		{
			CreateMultipleDir(GetPath(output_filepath));

			CCallcURL cURL;
			string URL = "https://esgf-node.llnl.gov/esg-search/search?project=CMIP6&offset=0&limit=10&type=Dataset&format=application%2Fsolr%2Bjson&facets=activity_id%2C+data_node%2C+source_id%2C+institution_id%2C+source_type%2C+experiment_id%2C+sub_experiment_id%2C+nominal_resolution%2C+variant_label%2C+grid_label%2C+table_id%2C+frequency%2C+realm%2C+variable_id%2C+cf_standard_name&latest=true&replica=false&query=*&experiment_id=hist-1950,historical,piControl,ssp585,ssp460,ssp370,ssp245,ssp126&source_id=" + model + "&table_id=fx&variable_id=" + prefix;

			string responce;
			msg = cURL.get_text(URL, responce);

			string error;
			const Json& root = Json::parse(responce, error);
			if (error.empty())
			{
				//ROOT.response.docs[0]

				ASSERT(root["response"]["docs"].type() == Json::ARRAY);
				const std::vector<Json>& file_list = root["response"]["docs"].array_items();

				ASSERT(file_list.size() >= 1);
				//for (Json::array::const_iterator it = file_list.begin(); it != file_list.end() && msg; it++)

				Json::array::const_iterator it = file_list.begin();
				/*ASSERT((*it)["dataset_id_template_"].type() == Json::ARRAY);
				ASSERT((*it)["directory_format_template_"].type() == Json::ARRAY);
				ASSERT((*it)["dataset_id_template_"].array_items().size() == 1);
				ASSERT((*it)["directory_format_template_"].array_items().size() == 1);
				*/

				/*string template_id = (*it)["dataset_id_template_"][0].string_value();
				template_id = ReplaceString(template_id, "%(", "|");
				template_id = ReplaceString(template_id, ")s", "|");
				StringVector tmp(template_id, "|");
				string dataset;
				for (size_t i = 0; i < tmp.size(); i++)
				{
					if (tmp[i] == ".")
					{
						dataset += ".";
					}
					else
					{
						ASSERT((*it)[tmp[i]].type() == Json::ARRAY);
						const std::vector<Json>& item = (*it)[tmp[i]].array_items();
						ASSERT(item.size()==1);
						dataset += item[0].string_value();
					}
				}*/
				/*string dataset = (*it)["master_id"].string_value();

				string template_id = (*it)["directory_format_template_"][0].string_value();
				template_id = ReplaceString(template_id, "%(", "|");
				template_id = ReplaceString(template_id, ")s", "|");
				StringVector tmp = StringVector(template_id, "|");
				string directory;
				for (size_t i = 0; i < tmp.size(); i++)
				{
					if (tmp[i] == "/")
					{
						directory += "/";
					}
					else if (tmp[i] == "root")
					{
					}
					else
					{
						if ((*it)[tmp[i]].type() == Json::ARRAY)
						{
							const std::vector<Json>& item = (*it)[tmp[i]].array_items();
							ASSERT(item.size() == 1);
							directory += item[0].string_value();
						}
						else if ((*it)[tmp[i]].type() == Json::STRING)
						{
							directory += (*it)[tmp[i]].string_value();
						}
					}
				}


				string server = (*it)["data_node"].string_value();
				string base_url = "/thredds/fileServer";
				string URL = "https://" + server + base_url + directory + dataset;*/
				//data_node
				{


					ASSERT((*it)["url"].type() == Json::ARRAY);
					const std::vector<Json>& urls = (*it)["url"].array_items();

					string orog_url1 = urls[0].string_value();
					StringVector tmp(orog_url1, "/");
					ASSERT(tmp.size() > 2 && tmp[0] == "http:");
					string server = tmp[1];


					string str_xml;
					msg = cURL.get_text("-k --connect-timeout 5 " + orog_url1, str_xml);
					if (str_xml.find("Service Unavailable") != string::npos)
						msg.ajoute("Service Unavailable");

					if (msg)
					{
						try
						{

							zen::XmlDoc doc = zen::parse(str_xml);

							string base_url = "thredds/fileServer/";
							string nc_url;


							zen::XmlIn in(doc.root());
							for (zen::XmlIn child = in["dataset"]["dataset"]; child && nc_url.empty() && msg; child.next())
							{

								string str;
								if (child.attribute("urlPath", str))
								{
									nc_url = str;
								}

							}


							if (!server.empty() && !base_url.empty() && !nc_url.empty())
							{
								string URL = "https://" + server + "/" + base_url + nc_url;

								msg = cURL.copy_file(URL, output_filepath, as<bool>(SHOW_CURL));
								if (msg)
								{
									if (!GoodHDF(output_filepath))
									{
										msg.ajoute("Invalid NetCDF file:" + output_filepath);
									}
								}
							}
						}
						catch (const zen::XmlParsingError& e)
						{
							// handle error
							msg.ajoute("Error parsing XML file: col=" + ToString(e.col) + ", row=" + ToString(e.row));
						}
					}
				}
			}//if error
			else
			{
				msg.ajoute(error);
			}

		}

		return msg;
	}



	ERMsg CUICMIP6::DownloadData(CCallback& callback)
	{
		ERMsg msg;

		string model = Get(MODEL);
		string working_dir = GetDir(WORKING_DIR);






		CCallcURL cURL;

		string variant_lable = "r1i1p1f1";

		string URL = "https://esgf-node.llnl.gov/esg-search/search?project=CMIP6&offset=0&limit=50&type=Dataset&format=application%2Fsolr%2Bjson&facets=activity_id,+data_node,+source_id,+institution_id,+source_type,+experiment_id,+sub_experiment_id,+nominal_resolution,+variant_label,+grid_label,+table_id,+frequency,+realm,+variable_id,+cf_standard_name&latest=true&replica=false&query=*&experiment_id=ssp585,ssp370,ssp126,ssp245,historical&frequency=mon&source_id=" + model + "&table_id=Amon&variable_id=tasmin,tasmax,sfcWind,huss,pr&variant_label=" + variant_lable;

		string responce;
		msg = cURL.get_URL_text(URL, responce);


		string server;
		vector<string> nc_url;

		string error;
		const Json& root = Json::parse(responce, error);
		if (error.empty())
		{
			//ROOT.response.docs[0]

			ASSERT(root["response"]["docs"].type() == Json::ARRAY);
			const std::vector<Json>& file_list = root["response"]["docs"].array_items();



			if (file_list.size() == 45)
			{
				for (Json::array::const_iterator it = file_list.begin(); it != file_list.end() && msg; it++)
				{

					ASSERT((*it)["url"].type() == Json::ARRAY);
					const std::vector<Json>& urls = (*it)["url"].array_items();

					string orog_url1 = urls[0].string_value();
					StringVector tmp(orog_url1, "/");
					ASSERT(tmp.size() > 2 && tmp[0] == "http:");
					ASSERT(server.empty() || server == tmp[1]);
					server = tmp[1];


					string str_xml;
					cURL.m_timeout = 5;
					msg = cURL.get_text("-k --connect-timeout 5 " + orog_url1, str_xml);
					if (str_xml.find("Service Unavailable") != string::npos)
						msg.ajoute("Service Unavailable");

					if (msg)
					{
						try
						{
							zen::XmlDoc doc = zen::parse(str_xml);

							zen::XmlIn in(doc.root());
							for (zen::XmlIn child = in["dataset"]["dataset"]; child && msg; child.next())
							{

								string str;
								if (child.attribute("urlPath", str))
								{
									nc_url.push_back(str);
								}
							}
						}
						catch (const zen::XmlParsingError& e)
						{
							// handle error
							msg.ajoute("Error parsing XML file: col=" + ToString(e.col) + ", row=" + ToString(e.row));
						}
					}
				}
			}//if 25 ssp(5) * var(5)

		}//if error
		else
		{
			msg.ajoute(error);
		}



		if (!nc_url.empty())
		{
			callback.PushTask(string("Download data for ") + model, nc_url.size());
			for (size_t i = 0; i < nc_url.size() && msg; i++)
			{
				string base_url = "thredds/fileServer/";
				string URL = "https://" + server + "/" + base_url + nc_url[i];

				string file_name = GetFileName(nc_url[i]);
				string output_filepath = working_dir + model + "\\NetCDF\\" + file_name;
				CreateMultipleDir(GetPath(output_filepath));

				if (!FileExists(output_filepath))
				{
					msg = cURL.copy_file(URL, output_filepath, as<bool>(SHOW_CURL));
					if (msg)
					{
						if (!GoodHDF(output_filepath))
						{
							msg.ajoute("Invalid NetCDF file:" + output_filepath);
							msg += RemoveFile(output_filepath);
						}
					}
					msg += callback.StepIt();
				}
			}

			callback.PopTask();
		}



		return msg;
	}




	ERMsg CUICMIP6::DownloadFilesIndex(CCallback& callback)
	{
		ERMsg msg;
		string working_dir = GetDir(WORKING_DIR);



		CCallcURL cURL;

		string index_filepath = working_dir + "latest_version.csv";


		//always download file index
		if (!FileExists(index_filepath))
		{

			CreateMultipleDir(working_dir);

			callback.PushTask(string("Download files index"), INDEX_NAMES.size());
			for (size_t i = 0; i < INDEX_NAMES.size() && msg; i++)
			{

				string URL = "https://" + serverNASA + "/" + INDEX_NAMES[i];
				string output_path = working_dir + INDEX_NAMES[i];
				msg += cURL.copy_file(URL, output_path);

				msg += callback.StepIt();
			}

			callback.PopTask();

			//Create a file with latest version 
			if (msg)
			{
				map< string, CNEX_GDDP_CMIP6 > index;


				callback.PushTask(string("Read files index"), INDEX_NAMES.size());
				for (size_t i = 0; i < INDEX_NAMES.size() && msg; i++)
				{
					string output_path = working_dir + INDEX_NAMES[i];

					ifStream file;
					msg += file.open(output_path);
					if (msg)
					{
						std::string md5;
						std::string URL; // Change to 'int' or 'double' if your data requires it

						// The >> operator automatically skips multiple spaces and handles row splits
						while (file >> md5 >> URL)
						{
							CNEX_GDDP_CMIP6 info(URL);
							index[info.GetID()] = info;
						}

						file.close();
					}

					msg += callback.StepIt();
				}

				callback.PopTask();

				if (msg)
				{
					//save new file with the latest version
					ofStream file;
					msg += file.open(index_filepath);
					if (msg)
					{
						file << "Source,Model,SPP,Run,Variable,FileName" << endl;

						for (map< string, CNEX_GDDP_CMIP6 >::const_iterator it = index.begin(); it != index.end() && msg; it++)
						{
							for (size_t i = 0; i < it->second.m_info.size(); ++i)
							{
								if (i > 0)
									file << ","; // Add comma between elements

								file << it->second.m_info[i];
							}

							file << endl;
							msg += callback.StepIt(0);
						}


						file.close();


					}
				}
			}
		}
		return msg;
	}

	ERMsg CUICMIP6::GetFilesIndex(std::vector<CNEX_GDDP_CMIP6>& index, CCallback& callback)
	{
		ERMsg msg;


		index.clear();

		string working_dir = GetDir(WORKING_DIR);
		string file_path = working_dir + "latest_version.csv";

		ifStream file;
		msg += file.open(file_path);
		if (msg)
		{
			string line;
			//read header
			std::getline(file, line);

			while (std::getline(file, line) && msg)
			{
				CNEX_GDDP_CMIP6 URL;

				std::stringstream ss(line);
				std::string value;

				while (std::getline(ss, value, ','))
				{
					URL.m_info.push_back(value);
				}

				assert(URL.m_info.size() == CNEX_GDDP_CMIP6::NB_INFO);
				index.push_back(URL);
				msg += callback.StepIt(0);
			}

			file.close();
		}


		return msg;

	}


	ERMsg CUICMIP6::DownloadDataNASA(CCallback& callback)
	{
		ERMsg msg;


		string working_dir = GetDir(WORKING_DIR);
		string model = Get(MODEL);
		string ssp = Get(SSP);
		int first_year = as<int>(FIRST_YEAR);
		int last_year = as<int>(LAST_YEAR);
		size_t nb_years = last_year - first_year + 1;
		//CTPeriod valid_period(CTRef(first_year, JANUARY, DAY_01), CTRef(last_year, DECEMBER, DAY_31));
		CCallcURL cURL;

		std::vector<CNEX_GDDP_CMIP6> index_all;
		msg += GetFilesIndex(index_all, callback);


		//index.reserve(index_all.size());


		size_t nb_ssp = ssp.empty() ? NB_SSP : 1;

		callback.PushTask(string("Download data for ") + model + " (" + to_string(nb_ssp) + " ssp)", nb_ssp);


		for (size_t i = 0; i < nb_ssp && msg; i++)
		{
			if (nb_ssp == NB_SSP)
				ssp = SSP_NAME[i];

			string output_path = working_dir + "NetCDF\\" + model + "\\" + ssp + "\\";
			CreateMultipleDir(output_path);






			//clean index
			std::map<int, std::vector<CNEX_GDDP_CMIP6>> index_map;
			for (std::vector<CNEX_GDDP_CMIP6>::iterator it = index_all.begin(); it != index_all.end() && msg; it++)
			{
				int year = it->GetYear();
				string file_path = output_path + it->m_info[CNEX_GDDP_CMIP6::I_FILE_NAME];

				bool b1 = it->is_good_model(model);
				bool b2 = it->is_good_spp(ssp);
				bool b3 = it->is_valid_year(first_year, last_year);
				bool b4 = find(begin(VARIABLES_NAMES), end(VARIABLES_NAMES), it->m_info[CNEX_GDDP_CMIP6::I_VARIABLE]) != end(VARIABLES_NAMES);
				bool b5 = !FileExists(file_path);//download only missing file

				if (b1 && b2 && b3 && b4 && b5)
				{
					index_map[year].push_back(*it);
				}

				msg += callback.StepIt(0);
			}


			if (msg && !index_map.empty())
			{
				callback.PushTask(string("Download data for ") + model + ", ssp=" + ssp + " (" + to_string(index_map.size()) + " years)", index_map.size());
				//for (size_t y = 0; y < nb_years && msg; y++)
				for (auto it = index_map.begin(); it != index_map.end() && msg; it++)
				{
					//int year = it->first;
					const std::vector<CNEX_GDDP_CMIP6>& index = it->second;


					string URLs_file_path = output_path + "urls.txt";

					ofStream ofile;
					msg += ofile.open(URLs_file_path);

					//Create a file list
					for (size_t i = 0; i < index.size() && msg; i++)
					{
						string URL = "https://" + serverNASA + "/" + index[i].GetURL();
						ofile << URL << endl;
					}

					ofile.close();

					msg = cURL.copy_files(URLs_file_path, output_path, as<bool>(SHOW_CURL), index.size());


					//Convert it into GeoTiff
					//for (size_t i = 0; i < index.size() && msg; i++)
					//{
					//	string filepath_in = output_path + index[i].m_info[CNEX_GDDP_CMIP6::I_FILE_NAME];
					//	string filepath_out = filepath_in;
					//	SetFileExtension(filepath_out, ".tif");

					//	//gdalwarp -nomd -multi -wo NUM_THREADS=ALL_CPUS -co NUM_THREADS=ALL_CPUS 
					//	// -of GTIFF -co NUM_THREADS=ALL_CPUS -co COMPRESS=ZSTD -co PREDICTOR=3 -co TILED=YES -co BLOCKXSIZE=256 -co BLOCKYSIZE=256 -overwrite -s_srs  "+proj=longlat +datum=WGS84 +pm=0 +lon_wrap=180"  -t_srs EPSG:4326 NETCDF:"F:\CMIP6(Daily)\NetCDF\CanESM5\historical\tasmin_day_CanESM5_historical_r1i1p1f1_gn_2014_v2.0.nc" "F:\CMIP6(Daily)\GeoTiff\CanESM5\historical\test3.tif"
					//	string gdal_data_path = GetApplicationPath() + "gdal-data";
					//	string projlib_path = GetApplicationPath() + "projlib";
					//	string plugin_path = GetApplicationPath() + "gdalplugins";

					//	string option = "--config GDAL_NUM_THREADS ALL_CPUS --config GDAL_CACHEMAX 4096 --config GDAL_NUM_THREADS ALL_CPUS --config GDAL_PAM_ENABLED NO --config GDAL_DATA \"" + gdal_data_path + "\" --config PROJ_LIB \"" + projlib_path + "\" --config GDAL_DRIVER_PATH \"" + plugin_path + "\"";
					//	string argument = "-ot Float32 -of GTIFF -co NUM_THREADS=ALL_CPUS -co COMPRESS=ZSTD -co PREDICTOR=3 -co TILED=YES -co BLOCKXSIZE=256 -co BLOCKYSIZE=256";
					//	string command = "\"" + GetApplicationPath() + "gdalwarp.exe\" " + option + " " + argument + " \"" + filepath_in + "\" \"" + filepath_out + "\"";
					//	msg += WinExecWait(command, "", SW_SHOW);
					//	if (msg && FileExists(filepath_out))
					//	{
					//		msg += RemoveFile(filepath_in);
					//	}

					//}




					msg += callback.StepIt();
				}//for all years

				callback.PopTask();
			}//if any files to download

			msg += callback.StepIt();
		}//for all spp

		callback.PopTask();

		return msg;
	}


	ERMsg CUICMIP6::CreateMMG(string filepath, CCallback& callback)
	{
		ERMsg msg;

		CPLSetConfigOption("GDAL_CACHEMAX", "4096");
		CPLSetConfigOption("GDAL_NUM_THREADS", "ALL_CPUS");
		CPLSetConfigOption("GDAL_NUM_THREADS", "ALL_CPUS");
		CPLSetConfigOption("GDAL_PAM_ENABLED", "NO");


		string working_dir = GetDir(WORKING_DIR);
		string model = Get(MODEL);
		string ssp = Get(SSP);
		bool bWorld = Get(GEO_DOMAIN) == "World";
		int first_year = as<int>(FIRST_YEAR);
		int last_year = as<int>(LAST_YEAR);
		int nb_years = last_year - first_year + 1;


		if (ssp == SSP_NAME[SSP_HISTORICAL])//no MMG for only historical
		{
			msg.ajoute("The SSP historical can't be selected to create an MMG file");
			return msg;
		}

		msg += WBSF::CreateMultipleDir(GetPath(filepath));
		if (!msg)
			return msg;


		//class member to put warning only once
		m_bWarningMissingFeb29 = false;
		m_bWarningFixed30DaysData = false;

		CTPeriod valid_period = get_period(first_year, last_year);
		//string new_orog_filepath = GetApplicationPath() + "..\\Layers\\" + (bWorld ? "orog_fx_gn_World.tif" : "orog_fx_gn_Canada-USA.tif");

		CBaseOptions options;
		msg = GetMapOptions(options);
		options.m_nbBands = 12 * nb_years;

		size_t nb_ssp = ssp.empty() ? NB_SSP - 1 : 1;
		if (nb_ssp > 1)
			callback.PushTask(string("Create MMG for ") + model + " (" + to_string(nb_ssp) + " ssp)", nb_ssp);


		for (size_t i = 0; i < nb_ssp && msg; i++)
		{
			if (nb_ssp == NB_SSP - 1)
				ssp = SSP_NAME[i + 1];

			string filePathOut = filepath;
			WBSF::SetFileTitle(filePathOut, WBSF::GetFileTitle(filePathOut) + "_" + model + "_" + ssp);
			if (!FileExists(filePathOut))
			{
				CMonthlyMeanGrid MMG;
				MMG.m_firstYear = first_year;
				MMG.m_lastYear = last_year;

				MMG.m_supportedVariables[TMIN_MN] = true;
				MMG.m_supportedVariables[TMAX_MN] = true;
				MMG.m_supportedVariables[TMNMX_R] = false;//not used in the cc modification
				MMG.m_supportedVariables[DEL_STD] = true;
				MMG.m_supportedVariables[EPS_STD] = true;
				MMG.m_supportedVariables[TACF_A1] = false;//not used in the cc modification
				MMG.m_supportedVariables[TACF_A2] = false;//not used in the cc modification
				MMG.m_supportedVariables[TACF_B1] = false;//not used in the cc modification
				MMG.m_supportedVariables[TACF_B2] = false;//not used in the cc modification
				MMG.m_supportedVariables[PRCP_TT] = true;
				MMG.m_supportedVariables[SPEH_MN] = true;
				MMG.m_supportedVariables[RELH_MN] = true;
				MMG.m_supportedVariables[RELH_SD] = true;
				MMG.m_supportedVariables[WNDS_MN] = true;
				MMG.m_supportedVariables[WNDS_SD] = true;



				msg = MMG.Save(filePathOut);
				if (msg)
				{

					size_t nb_grib_open = 0;
					array< CGDALDatasetEx, NB_FIELDS> grid;
					for (size_t v = 0; v < grid.size() && msg; v++)
					{
						string filePathOut = MMG.GetFilePath(v);
						if (!filePathOut.empty())
						{
							msg += grid[v].CreateImage(filePathOut, options);
							nb_grib_open++;
						}

						msg += callback.StepIt(0);
					}


					if (msg)
					{
						string info_str = "Create MMG for " + ssp + " (" + to_string(nb_years) + " years)";
						callback.PushTask(info_str, nb_years);
						callback.AddMessage(info_str);

						for (size_t y = 0; y < nb_years && msg; y++)
						{
							int year = int(first_year + y);

							CMonthlyVariableVector data;
							msg += GetMMGForSSP(model, year < 2015 ? SSP_NAME[SSP_HISTORICAL] : ssp, year, options.m_extents, data, callback);
							msg += SaveData(y, year, grid, data, callback);

							msg += callback.StepIt();
						}//for all years

						callback.PopTask();


						callback.PushTask(string("Close output images for ") + ssp, grid.size());
						for (size_t v = 0; v < grid.size(); v++)//always close map
						{
							if (grid[v].IsOpen())
							{
								grid[v].Close();

								string filepath_out1 = MMG.GetFilePath(v);
								string filepath_out2 = filepath_out1;
								filepath_out2 = SetFileTitle(filepath_out2, GetFileTitle(filepath_out1) + "_tmp");
								//for an unknown reason, the file is very big.
								//convert it with gdal_translate.
								string gdal_data_path = GetApplicationPath() + "gdal-data";
								string projlib_path = GetApplicationPath() + "projlib";
								string plugin_path = GetApplicationPath() + "gdalplugins";
								
								string option = "--config GDAL_NUM_THREADS ALL_CPUS --config GDAL_PAM_ENABLED NO --config GDAL_DATA \"" + gdal_data_path + "\" --config PROJ_LIB \"" + projlib_path + "\" --config GDAL_DRIVER_PATH \"" + plugin_path + "\"";
								string argument = "-overwrite -ot Float32 -co NUM_THREADS=ALL_CPUS -co BIGTIFF=YES -co COMPRESS=ZSTD -co PREDICTOR=3 -co TILED=YES -co BLOCKXSIZE=256 -co BLOCKYSIZE=256";
								string prj = "-s_srs \"+proj=longlat +datum=WGS84 +pm=0 +lon_wrap=180\" -t_srs EPSG:4326";
								string command = "\"" + GetApplicationPath() + "gdalwarp.exe\" " + option + " " + argument + " " + prj + " \"" + filepath_out1 + "\" \"" + filepath_out2 + "\"";
								msg += WinExecWait(command);
								if (msg && FileExists(filepath_out2))
								{
									msg += RemoveFile(filepath_out1);
									msg += RenameFile(filepath_out2, filepath_out1);
								}
							}

							msg += callback.StepIt();
						}

						callback.PopTask();

					}//if msg
				}//if msg
			}//MMG doesn't exist
			else
			{
				callback.AddMessage("MMG " + WBSF::GetFileTitle(filePathOut) + " already exist. Skip it");
			}

			msg += callback.StepIt();
		}//for all spp


		if (m_bWarningMissingFeb29)
			callback.AddMessage("WARNING: input files have missing February 29");
		if (m_bWarningFixed30DaysData)
			callback.AddMessage("WARNING: input files have fixed 30 days by months");



		if (nb_ssp > 1)
			callback.PopTask();

		return msg;

	}


	ERMsg CUICMIP6::SaveData(size_t y, int year, std::array< CGDALDatasetEx, NB_FIELDS>& grid, CMonthlyVariableVector& data, CCallback& callback)
	{
		ERMsg msg;



		callback.PushTask("Save output images for year " + to_string(year), grid.size() * data.size());
		for (size_t f = 0; f < grid.size() && msg; f++)
		{
			for (size_t m = 0; m < data.size() && msg; m++)
			{
				if (grid[f].IsOpen())
				{
					GDALRasterBand* pBand = grid[f].GetRasterBand(y * 12 + m);
					pBand->RasterIO(GF_Write, 0, 0, grid[f].GetRasterXSize(), grid[f].GetRasterYSize(), &(data[m][f][0]), grid[f].GetRasterXSize(), grid[f].GetRasterYSize(), GDT_Float32, 0, 0);
				}

				msg += callback.StepIt();
			}//for all months
		}//for all variables

		callback.PopTask();
		return msg;
	}

	string CUICMIP6::GetProjectionWKT() { return PRJ_WGS_84_WKT; }

	ERMsg CUICMIP6::GetMapOptions(CBaseOptions& options)const
	{
		ERMsg msg;


		//CGDALDatasetEx DS;
		//msg = DS.OpenInputImage(orog_filepath);
		//if (msg)
		//{
			//DS.UpdateOption(options);
		options.m_dstNodata = -999;
		options.m_outputType = GDT_Float32;
		options.m_format = "GTIFF";
		options.m_bOverwrite = true;
		options.m_bComputeStats = false;
		options.m_extents = CGeoExtents(0, -60, 360, 90, 1440, 600, 256, 256, PRJ_WGS_84);
		//options.m_overviewLevels = { { 2, 4, 8, 16 } };
		//options.m_createOptions.push_back("COMPRESS=LZW");
		options.m_createOptions.push_back("COMPRESS=ZSTD");
		options.m_createOptions.push_back("PREDICTOR=3");
		//options.m_createOptions.push_back("TILED=YES");
		//options.m_createOptions.push_back("BLOCKXSIZE=256");
		//options.m_createOptions.push_back("BLOCKYSIZE=256");
		options.m_createOptions.push_back("BIGTIFF=YES");
		options.m_createOptions.push_back("SPARSE_OK=TRUE");
		options.m_createOptions.push_back("NUM_THREADS=ALL_CPUS");

		//}

		return msg;
	}

	void CUICMIP6::ConvertData(size_t v, std::vector<float>& data)const
	{
		using namespace units::values;


		std::vector<float> dataII(data.size());

		for (size_t i = 0; i < data.size(); i++)
		{
			//size_t ii = i;

			if (data[i] < 1.0E20)
			{
				switch (v)
				{
				case V_TMIN:	dataII[i] = (float)Celsius(K(data[i])).get(); break; //K --> °C
				case V_TMAX:	dataII[i] = (float)Celsius(K(data[i])).get(); break; //K --> °C
				case V_PRCP:	dataII[i] = (float)(data[i] * 60 * 60 * 24); break; //kg/(m²s) --> mm/day 
				case V_SPEH:	dataII[i] = (float)(data[i] * 1000); break; //kg[H2O]/kg[air] --> g[H2O]/kg[air]
				case V_WNDS:	dataII[i] = (float)data[i] * 3600 / 1000; break;//(float)kph(meters_per_second(data[i])).get(); break; //m/s --> km/h
					//case V_SRAD:	break;
				default: ASSERT(false);
				}
			}
			else
			{
				dataII[i] = -999;
			}
		}

		data = dataII;
		//data.swap(dataII);
	}

	CTPeriod CUICMIP6::get_period(int year1, int year2)
	{
		return CTPeriod(CTRef(year1, JANUARY, DAY_01), CTRef(year2, DECEMBER, DAY_31));
	}

	ERMsg CUICMIP6::GetFileList(std::string model, std::string ssp, int year, CMIP6FileList& fileList)const
	{
		ERMsg msg;

		if (ssp == "historical" && year >= 2015)
			return msg;

		if (ssp != "historical" && year < 2015)
			return msg;



		//CTPeriod p1 = get_period(1951, 2014);
		//CTPeriod p2 = get_period(2015, 2100);
		//CTPeriod p = valid_period.Intersect(ssp == "historical" ? p1 : p2);



		string path = GetDir(WORKING_DIR) + "NetCDF\\" + model + "\\" + ssp + "\\*.nc";
		StringVector list = WBSF::GetFilesList(path);
		for (size_t i = 0; i < list.size(); i++)
		{
			StringVector tmp(GetFileTitle(list[i]), "_");
			string i_var = tmp[C_VAR];
			string i_model = tmp[C_MODEL];
			string i_ssp = tmp[C_SSP];
			int i_year = stoi(tmp[C_YEAR]);
			//CTPeriod p = get_period(year, year);
			bool b_valid_var = find(begin(VARIABLES_NAMES), end(VARIABLES_NAMES), i_var) != end(VARIABLES_NAMES);

			if (b_valid_var &&
				i_model == model &&
				i_ssp == ssp &&
				i_year == year)
			{
				fileList.push_back(list[i]);
			}
		}



		//All period must have the same number of variables
		//for (int year = p.GetFirstYear(); year <= p.GetLastYear() && msg; year++)
		//{
		//auto it = fileList.find(year);

		if (fileList.size() != NB_CMIP6_VARIABLES)
		{
			msg.ajoute(to_string(NB_CMIP6_VARIABLES - fileList.size()) + " files are missing for year: " + to_string(year));
		}

		//}



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
			assert(data[d][V_TMIN][i] > -999);
			assert(data[d][V_TMAX][i] > -999);

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

	ERMsg CUICMIP6::GetMMGForSSP(std::string model, std::string ssp, int year, const CGeoExtents& extents, CMonthlyVariableVector& dataOut, CCallback& callback)
	{
		ERMsg msg;


		//CGeoExtents original_extent(-180, -60, 180, 90, 1440, 600, 64, 64, PRJ_WGS_84);
		//bool b_Canada_USA = ((CGeoRect&)original_extent) != ((CGeoRect&)extents_in);

		//CGeoExtents extents = extents_in;
		//WBSF::Switch(original_extent.m_yMin, original_extent.m_yMax);
		//WBSF::Switch(extents.m_yMin, extents.m_yMax);
		CGeoRectIndex geo_rect = extents.CoordToXYPos(extents);


		//if (b_Canada_USA)
			//geo_rect.m_x += original_extent.m_xSize / 2;



		CMIP6FileList fileList;
		msg = GetFileList(model, ssp, year, fileList);
		if (!msg)
			return msg;


		dataOut.resize(12);
		for (size_t mm = 0; mm < dataOut.size(); mm++)
			for (size_t v = 0; v < NB_FIELDS; v++)
				dataOut[mm][v].insert(dataOut[mm][v].begin(), extents.m_ySize * extents.m_xSize, -999);



		//CTRef next_TRef;
		//bool bWarningFixed30DaysData = false;
		//bool bWarningMissingFeb29 = false;

		//open files
		CTPeriod period = get_period(year, year);
		CTPeriod intersect = period;// valid_period.Intersect(period);

		bool bIsMissingFeb29 = false;
		size_t leap_correction = 0;// NOT_INIT;
		bool bIsFixed30DaysData = false;


		NcFilePtrArray ncFiles;


		ASSERT(fileList.size() == NB_CMIP6_VARIABLES);
		for (size_t i = 0; i < fileList.size() && msg; i++)
		{
			try
			{
				size_t v = GetVar(fileList[i]);
				ncFiles[v] = NcFilePtr(new NcFile(fileList[i], NcFile::read));

				if (i == 0)
				{
					auto timeGrid = ncFiles[v]->getDim("time");
					size_t nbdays = timeGrid.getSize();

					if (nbdays != period.size())
					{
						size_t nbLeapdays = 0;
						for (CTRef TRef = period.Begin(); TRef <= period.End(); TRef++)
							if (TRef.GetMonth() == FEBRUARY && TRef.GetDay() == DAY_29)
								nbLeapdays++;

						if (nbdays + nbLeapdays == period.size())
						{
							bIsMissingFeb29 = true;
							m_bWarningMissingFeb29 = true;

							//compute all leap year since the beginning of the period)
							for (CTRef TRef = period.Begin(); TRef < intersect.Begin(); TRef++)
								if (TRef.GetMonth() == FEBRUARY && TRef.GetDay() == DAY_29)
									leap_correction++;


						}
						else
						{
							if (nbdays == period.as(CTM::MONTHLY).size() * 30)
							{
								bIsFixed30DaysData = true;
								m_bWarningFixed30DaysData = true;
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
				msg.ajoute(string("Unable to open file : ") + fileList[i]);
			}
		}

		//callback.PopTask();


		if (msg)
		{
			ASSERT(fileList.size() == NB_CMIP6_VARIABLES);

			auto timeGrid = ncFiles.front()->getDim("time");
			size_t nbDays = timeGrid.getSize();
			callback.PushTask("Compute statistic for year = " + to_string(year), nbDays * ncFiles.size());

			for (size_t mm = 0; mm < 12 && msg; mm++)
			{
				CTPeriod daily_period = CTPeriod(year, mm, DAY_01, year, mm, LAST_DAY);

				COneMonthData daily_data;

				daily_data.resize(daily_period.size());
				for (size_t d = 0; d < daily_period.size(); d++)
					for (size_t v = 0; v < daily_data[d].size(); v++)
						daily_data[d][v].resize(extents.m_ySize * extents.m_xSize);


				std::vector<float> tmp_raw_data(extents.m_ySize * extents.m_xSize);
				for (CTRef TRef = daily_period.Begin(); TRef <= daily_period.End() && TRef <= period.End() && msg; TRef++)
				{

					if (bIsMissingFeb29)
					{
						if (TRef.GetMonth() == FEBRUARY && TRef.GetDay() == DAY_29)
							leap_correction++;
					}


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

							//size_t first_lat = 0;
							//size_t first_lon = 0;
							//size_t nbLat = latGrid.getSize();
							//size_t nbLon = lonGrid.getSize();

							size_t first_lat = geo_rect.m_y;
							size_t first_lon = geo_rect.m_x;
							size_t nbLat = geo_rect.m_ySize;
							size_t nbLon = geo_rect.m_xSize;
							
							


							vector<size_t> startp = { {dd, first_lat, first_lon } };
							vector<size_t> countp = { { 1, nbLat, nbLon } };
							NcVar& var = ncFiles[v]->getVar(VARIABLES_NAMES[v]);
							
							
							var.getVar(startp, countp, &(tmp_raw_data[0]));
							//var.getVar(startp, countp, &(raw_data[d][v][0]));
							
							for (size_t r = 0; r < nbLat; ++r) 
							{
								size_t srcRowIdx = (nbLat - 1 - r) * nbLon;
								size_t destRowIdx = r * nbLon;

								// Copy row and revert the Y mapping
								std::copy(tmp_raw_data.begin() + srcRowIdx,
									tmp_raw_data.begin() + srcRowIdx + nbLon,
									daily_data[d][v].begin() + destRowIdx);
							}
						}
						catch (exceptions::NcException& e)
						{
							//msg.ajoute(e.what());
							//msg.ajoute(string("processing variable : ") + VARIABLES_NAMES[v] + " for date " + TRef.GetFormatedString());
							callback.AddMessage(e.what());
							callback.AddMessage(string("processing variable : ") + VARIABLES_NAMES[v] + " for date " + TRef.GetFormatedString());
							//try to copy the last line
							if (d > 0)
								daily_data[d][v] = daily_data[d - 1][v];
						}

						ConvertData(v, daily_data[d][v]);

						msg += callback.StepIt();
					}
				}//for all days of the month 

				//if we have to continue on the next file
				//if (daily_period.End() > period.End())
				//	next_TRef = daily_period.End() + 1;
				//else
				//	next_TRef.clear();

				//bool bRevertImage = !b_Canada_USA;
				//bool bRevertImage = false;
				if (msg)
				{

					bool stop_early = false;


#ifndef _DEBUG
#pragma omp parallel for shared(stop_early)
#endif
					for (__int64 i = 0; i < (__int64)extents.m_ySize * extents.m_xSize; i++)
					{
						if (stop_early)
							continue;

						//size_t mmm = montly_Tref - period.Begin().as(CTM::MONTHLY);

						size_t ii = i;
						/*if (bRevertImage)
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
						}*/

						if (daily_data[0][0][i] > -999)
							ComputeMontlyStatistic((size_t)i, ii, daily_data, dataOut[mm]);


#pragma omp  critical
						{
							msg += callback.StepIt(0);
							stop_early = !msg;
						}
					}
				}
			}//for all months

			callback.PopTask();
		}//if msg

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


	//ERMsg CUICMIP6::save_sftlf(std::string sftlf_filepath, std::string new_sftlf_filepath)
	//{
	//	ERMsg msg;
	//	//CGeoExtents extents = GetExtents();


	//	string tif_filepath = sftlf_filepath + ".tif";
	//	//convert nc into GeoTIFF
	//	string gdal_data_path = GetApplicationPath() + "gdal-data";
	//	string projlib_path = GetApplicationPath() + "projlib";
	//	string plugin_path = GetApplicationPath() + "gdalplugins";
	//	string option = "--config GDAL_DATA \"" + gdal_data_path + "\" --config PROJ_LIB \"" + projlib_path + "\" --config GDAL_DRIVER_PATH \"" + plugin_path + "\"";

	//	string argument = "-unscale -a_srs \"+proj=longlat +datum=WGS84 +no_defs\" -ot Float32 -stats -co COMPRESS=LZW -co PREDICTOR=3 -co TILED=YES -co BLOCKXSIZE=64 -co BLOCKYSIZE=64";
	//	string command = "\"" + GetApplicationPath() + "gdal_translate.exe\" " + option + " " + argument + " \"" + sftlf_filepath + "\" \"" + tif_filepath + "\"";;

	//	//string argument = "-a_srs \"+proj=longlat +datum=WGS84 +no_defs\" -ot Float32 -stats -co COMPRESS=LZW -co PREDICTOR=3 -co TILED=YES -co BLOCKXSIZE=64 -co BLOCKYSIZE=64 \"" + sftlf_filepath + "\" \"" + tif_filepath + "\"";
	//	//string command = "\"" + GetApplicationPath() + "gdal_translate.exe\" " + argument;
	//	msg += WinExecWait(command);


	//	if (msg)
	//	{
	//		CGDALDatasetEx DS;
	//		msg = DS.OpenInputImage(tif_filepath);
	//		if (msg)
	//		{
	//			GDALRasterBand* pBand = DS.GetRasterBand(0);

	//			vector<float> data(DS.GetRasterXSize() * DS.GetRasterYSize());
	//			pBand->RasterIO(GF_Read, 0, 0, DS.GetRasterXSize(), DS.GetRasterYSize(), &(data[0]), DS.GetRasterXSize(), DS.GetRasterYSize(), GDT_Float32, 0, 0);

	//			CBaseOptions options;
	//			DS.UpdateOption(options);
	//			int nbYears = LAST_YEAR - FIRST_YEAR + 1;

	//			bool bRevertImage = options.m_extents.m_xMin > -90;
	//			double shift = bRevertImage ? 180 /*- options.m_extents.XRes()*/ : 0;

	//			options.m_extents.m_xMin -= shift;
	//			options.m_extents.m_xMax -= shift;
	//			options.m_nbBands = 1;
	//			options.m_dstNodata = -999;
	//			options.m_outputType = GDT_Float32;
	//			options.m_format = "GTIFF";
	//			options.m_bOverwrite = true;
	//			options.m_bComputeStats = true;

	//			vector<float> tmp(DS.GetRasterXSize() * DS.GetRasterYSize());

	//			for (size_t i = 0; i < data.size(); i++)
	//			{
	//				size_t ii = i;
	//				if (bRevertImage)
	//				{
	//					size_t x = (i + DS.GetRasterXSize() / 2) % DS.GetRasterXSize();
	//					size_t y = size_t(i / DS.GetRasterXSize());
	//					ii = y * DS.GetRasterXSize() + x;
	//				}
	//				else
	//				{
	//					size_t x = i % DS.GetRasterXSize();
	//					size_t y = size_t(i / DS.GetRasterXSize());
	//					ii = y * DS.GetRasterXSize() + x;
	//				}

	//				tmp[ii] = data[i];
	//			}

	//			data = tmp;

	//			CGDALDatasetEx DS_out;
	//			DS_out.CreateImage(new_sftlf_filepath, options);

	//			GDALRasterBand* pBand_out = DS_out.GetRasterBand(0);
	//			pBand_out->RasterIO(GF_Write, 0, 0, options.m_extents.m_xSize, options.m_extents.m_ySize, &(data[0]), options.m_extents.m_xSize, options.m_extents.m_ySize, GDT_Float32, 0, 0);

	//			DS_out.Close();
	//		}//if msg
	//	}//if msg

	//	return msg;
	//}



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

	ERMsg CUICMIP6::save_orog(std::string orog_filepath, std::string new_orog_filepath)
	{
		ERMsg msg;
		//CGeoExtents extents = GetExtents();


		string tif_filepath = orog_filepath + ".tif";
		//convert nc into GeoTIFF
		string gdal_data_path = GetApplicationPath() + "gdal-data";
		string projlib_path = GetApplicationPath() + "projlib";
		string plugin_path = GetApplicationPath() + "gdalplugins";
		string option = "--config GDAL_DATA \"" + gdal_data_path + "\" --config PROJ_LIB \"" + projlib_path + "\" --config GDAL_DRIVER_PATH \"" + plugin_path + "\"";

		string argument = "-unscale -a_srs \"+proj=longlat +datum=WGS84 +no_defs\" -ot Float32 -stats -co COMPRESS=LZW -co PREDICTOR=3 -co TILED=YES -co BLOCKXSIZE=64 -co BLOCKYSIZE=64";
		string command = "\"" + GetApplicationPath() + "gdal_translate.exe\" " + option + " " + argument + " \"" + orog_filepath + "\" \"" + tif_filepath + "\"";

		//convert nc into GeoTIFF
		//string argument = "-a_srs \"+proj=longlat +datum=WGS84 +no_defs\" -ot Float32 -stats -co COMPRESS=LZW -co PREDICTOR=3 -co TILED=YES -co BLOCKXSIZE=64 -co BLOCKYSIZE=64 \"" + orog_filepath + "\" \"" + tif_filepath + "\"";
		//string command = "\"" + GetApplicationPath() + "gdal_translate.exe\" " + argument;
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


					DEM[ii] = data[i];
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

		string working_dir = GetDir(WORKING_DIR);
		string model = Get(MODEL);
		string ssp = Get(SSP);
		bool bWorld = Get(GEO_DOMAIN) == "World";
		int first_year = as<int>(FIRST_YEAR);
		int last_year = as<int>(LAST_YEAR);
		CTPeriod entire_period = get_period(first_year, last_year);


		float no_data_out = 9999;
		CStatistic::SetVMiss(no_data_out);



		string new_orog_filepath = GetApplicationPath() + "..\\Layers\\" + (bWorld ? "orog_fx_gn_World.tif" : "orog_fx_gn_Canada-USA.tif");
		//Load elevation
		vector<float> orog;
		msg += get_orog(new_orog_filepath, orog, no_data_out);


		bool bWarningFixed30DaysData = false;
		bool bWarningMissingFeb29 = false;




		CBaseOptions options;
		GetMapOptions(options);
		CGeoExtents extents = options.m_extents;

		size_t nb_ssp = ssp.empty() ? NB_SSP : 1;

		callback.PushTask(string("Create GeoTIFF for ") + model + " (" + to_string(nb_ssp) + " ssp)", nb_ssp);


		for (size_t i = 0; i < nb_ssp && msg; i++)
		{
			if (nb_ssp == NB_SSP)
				ssp = SSP_NAME[i];


			bool bHistorical = ssp == SSP_NAME[SSP_HISTORICAL];
			CTPeriod p = get_period(bHistorical ? 1951 : 2015, bHistorical ? 2014 : 2100);
			CTPeriod limited_period = entire_period.Intersect(p);


			size_t nb_years = limited_period.GetNbYears();
			callback.PushTask(string("Save output images for ") + ssp + " ( " + to_string(nb_years) + " years x " + to_string(NB_CMIP6_VARIABLES) + " variables)", nb_years * NB_CMIP6_VARIABLES);

			//#ifndef _DEBUG
			//#pragma omp parallel for
			//#endif
			for (size_t y = 0; y < nb_years && msg; y++)
			{
				int year = int(first_year + y);

				CMIP6FileList fileList;
				ERMsg msg_tmp = GetFileList(model, ssp, year, fileList);

				//#pragma omp single 
				msg += msg_tmp;


				if (msg)
				{

					//callback.PushTask(string("Process all files (") + to_string(fileList.size() ) + " files) for model " + model + " ssp=" + ssp + " and period " + valid_period.GetFormatedString("%1 to %2"), fileList.size());

										//open files
					for (CMIP6FileList::const_iterator it = fileList.begin(); it != fileList.end() && msg; it++)
					{
						//int year = it->first;
						CTPeriod period = get_period(year, year);
						CTPeriod intersect = period;// valid_period.Intersect(period);



						double x_min = -999;

						size_t nbLat = 0;
						size_t nbLon = 0;
						size_t nbdays = 0;

						bool bIsMissingFeb29 = false;
						size_t leap_correction = 0;// NOT_INIT;
						bool bIsFixed30DaysData = false;


						NcFilePtrArray ncFiles;
						array<NcVar, NB_CMIP6_VARIABLES> var;

						//const std::vector<std::string>& varList = it->second;

						string i_ssp;
						if (!fileList.empty())
						{
							StringVector tmp(GetFileTitle(fileList[0]), "_");
							ASSERT(tmp.size() == NB_COMPONENTS || tmp.size() == NB_COMPONENTS - 1);
							i_ssp = tmp[C_SSP];
						}




						ASSERT(fileList.size() == NB_CMIP6_VARIABLES);
						for (size_t i = 0; i < fileList.size() && msg; i++)
						{

							try
							{
								size_t v = GetVar(fileList[i]);
								ncFiles[v] = NcFilePtr(new NcFile(fileList[i], NcFile::read));
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

											//compute all leap year since the beginning of the period)
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
								msg.ajoute(string("Unable to open file : ") + fileList[i]);
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

								string filepath_out = FormatA("%sGeoTIFF\\%s\\%d\\%s_%s_%d%02d%02d.tif", working_dir.c_str(), model.c_str(), TRef.GetYear(), model.c_str(), i_ssp.c_str(), TRef.GetYear(), TRef.GetMonth() + 1, TRef.GetDay() + 1);

								ASSERT(period.IsInside(TRef));
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


													//if (landWaterMask[ii] >= minLandWater)
													if (data[i] > -999)
													{
														tmp[ii] = data[i];
													}
													else
													{
														tmp[ii] = no_data_out;
													}
												}


												data = tmp;

												static const array<size_t, 3> VAR = { H_TMIN, H_TMAX, H_PRCP };//use only the basic here: to do use all vars
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

										DSout.Close();//options

										//if (msg)
										//{
										//	string argument = "-ot Float32 -a_nodata 9999 -stats -co COMPRESS=LZW -co PREDICTOR=3 -co TILED=YES -co BLOCKXSIZE=256 -co BLOCKYSIZE=256";// -a_srs \"" + prj_str ;
										//	string command = "\"" + GetApplicationPath() + "gdal_translate.exe\" " + argument + " \"" + filepath_out + "2\" \"" + filepath_out + "\"";
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


						msg += callback.StepIt();
					}//for all variables
				}//if msg
			}//for all years

			callback.PopTask();

			msg += callback.StepIt();

		}//for all ssp

		callback.PopTask();

		return msg;
	}




	ERMsg CUICMIP6::GetGribsList(CTPeriod p, CGribsMap& gribsList, CCallback& callback)
	{
		ERMsg msg;


		string model = Get(MODEL);
		string ssp = Get(SSP);
		string working_dir = GetDir(WORKING_DIR) + model + "\\Gribs\\";

		if (ssp.empty())
		{
			msg.ajoute("ssp must be define. Empty ssp is not allowed to create gribs list");
			return msg;
		}


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


