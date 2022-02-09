#pragma once

#include <string>
#include <memory>
#include "Basic/ERMsg.h"


#ifdef EXPORT_DLL
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT __declspec(dllimport)
#endif


#pragma warning( disable : 4251)




namespace WBSF
{

	class CGDALDatasetEx;
	class CWeatherGenerator;
	class CNormalsDatabase;
	class CDailyDatabase;
	class CHourlyDatabase;
	class CNormalsStation;
	class CWGInput;
	class CModel;
	class CLocation;
	class CModelInput;
	class CWeatherStationVector;
	class CTransferInfoIn;
	typedef CWeatherStationVector CSimulationPointVector;



	typedef std::shared_ptr<CNormalsDatabase> CNormalsDatabasePtr;
	typedef std::shared_ptr<CDailyDatabase> CDailyDatabasePtr;
	typedef std::shared_ptr<CHourlyDatabase> CHourlyDatabasePtr;
	typedef std::shared_ptr<CGDALDatasetEx> CGDALDatasetExPtr;
	typedef std::shared_ptr<CWeatherGenerator> CWeatherGeneratorPtr;
	typedef std::shared_ptr<CModel> CModelPtr;


	class CGlobalDLLData
	{
	public:

		enum TParam { MODELS_PATH, AZURE_DLL, SHORE, DEM, DAILY_CACHE_SIZE, NB_PAPAMS };
		static const char* NAME[NB_PAPAMS];


		CGlobalDLLData() {clear();}
		void clear();
		ERMsg parse(const std::string& str_init);


		
		std::string m_model_path;
		std::string m_Azure_DLL_file_path;
		std::string m_shore_file_path;
		std::string m_DEM_file_path;
		size_t m_daily_cache_size;

	

		CGDALDatasetExPtr m_pDEM;
	};

	class DLL_EXPORT CBioSIM_API_GlobalData
	{
	public:

		//		BioSIM_API_DLL();
		std::string InitGlobalData(const std::string& str_options);
	};


	//IO class between c++ and python
	class DLL_EXPORT CTeleIO
	{
	public:

		CTeleIO(bool compress = false, const std::string& msg = "", const std::string& comment = "", const std::string& metadata = "", const std::string& data = "")
		{

			m_compress = compress;		//if output is compress or not
			m_msg = msg;		//error message
			m_comment = comment;	//comments
			m_metadata = metadata;	//output metadatan in XML
			m_data = data;	//output data 
		}


		
		bool m_compress;		//if output is compress or not
		std::string m_msg;		//error message
		std::string m_comment;	//comments
		std::string m_metadata;	//output metadatan in XML
		std::string m_data;		//output data 

	};


	class CWeatherGeneratorInit
	{

	public:

		//DEFAULT_ENDPOINTS_PROTOCOL, ENDPOINT_SUFFIX, 
		enum TParam { ACCOUNT_NAME, ACCOUNT_KEY, CONTAINER_NAME,/* SHORE,*/ NORMALS, DAILY, HOURLY, GRIBS, /*DEM,*/ NB_PAPAMS };
		static const char* NAME[NB_PAPAMS];

		CWeatherGeneratorInit();
		void clear();
		ERMsg parse(const std::string& options);
		bool IsAzure()const { return !m_account_name.empty() && !m_account_key.empty() && !m_container_name.empty(); }

		std::string m_account_name;
		std::string m_account_key;
		std::string m_container_name;

		//std::string m_shore_name;
		std::string m_normal_name;
		std::string m_daily_name;
		//std::string m_DEM_name;
	};




	class CWeatherGeneratorOptions
	{
	public:

		enum TParam { VARIABLES, SOURCE_TYPE, GENERATION_TYPE, REPLICATIONS, KEY_ID, NAME, LATITUDE, LONGITUDE, ELEVATION, SLOPE, ORIENTATION, NB_NEAREST_NEIGHBOR, FIRST_YEAR, LAST_YEAR, NB_YEARS, SEED, NORMALS_INFO, COMPRESS, NB_PAPAMS };
		static const char* PARAM_NAME[NB_PAPAMS];

		CWeatherGeneratorOptions();
		ERMsg parse(const std::string& options);
		void GetWGInput(CWGInput& WGInput)const;

		std::string m_variables;
		int m_sourceType; //from normal=0, from observation=1
		std::string m_normals_info;//if from normals, period and CC specification ex.: 2071-2100 ECCC RCM2 RCP 4.5
		int m_generationType; //hourly=0; daily=1
		std::string m_ID;
		std::string m_name;
		double m_latitude;
		double m_longitude;
		double m_elevation;
		double m_slope;
		double m_orientation;
		size_t m_nb_nearest_neighbor;
		size_t m_replications;
		size_t m_nb_years;
		int m_first_year;
		int m_last_year;
		int m_seed;//0 for random seed else fixed seed 
		bool m_compress;
	};


	

	class DLL_EXPORT CWeatherGeneratorAPI
	{

	public:

		enum TParam { SHORE, NORMALS, DAILY, HOURLY, GRIBS, DEM, NB_PAPAMS };
		static const char* NAME[NB_PAPAMS];

		CWeatherGeneratorAPI(const std::string &);
		std::string Initialize(const std::string& str_options);
		CTeleIO Generate(const std::string& str_options);
		//CTeleIO GenerateGribs(const std::string& str_options);
		CTeleIO GetNormals(const std::string& str_options);

		//void TestThreads(const std::string& str_options);

	protected:

		CWeatherGeneratorInit m_init;

		CNormalsDatabasePtr m_pNormalDB;
		CDailyDatabasePtr m_pDailyDB;
		//CHourlyDatabasePtr m_pHourlyDB;
		//CSfcGribExtractorPtr m_pGribsDB;
		//CSfcGribDatabasePtr m_pGribsDB;
		//CGDALDatasetExPtr m_pDEM;

		CWeatherGeneratorPtr m_pWeatherGenerator;
		

		ERMsg ComputeElevation(double latitude, double longitude, double& elevation);
		void SaveNormals(std::ostream& out, const CNormalsStation& normals);
	};



	//*************************************************************************************************
	//Model

	
	class CModelExecutionOptions
	{
	public:

		enum TParam { PARAMETERS, REPLICATIONS, SEED, COMPRESS, NB_PAPAMS };
		static const char* PARAM_NAME[NB_PAPAMS];

		CModelExecutionOptions();
		ERMsg parse(const std::string& options);
		ERMsg GetModelInput(const CModel& model, CModelInput& modelInput)const;

		std::string m_parameters; //input param [space format]

		size_t m_replications;
		int m_seed;//0 for random seed else fixed seed 
		bool m_compress;
	};


	

	class DLL_EXPORT CModelExecutionAPI
	{

	public:

		enum TParam { MODEL, NB_PAPAMS };
		static const char* NAME[NB_PAPAMS];

		CModelExecutionAPI(const std::string &);
		std::string Initialize(const std::string& str_options);
		CTeleIO Execute(const std::string& str_options, const CTeleIO& input);
		std::string GetWeatherVariablesNeeded();
		std::string GetDefaultParameters()const;
		std::string Help()const;
		

	protected:

		CModelPtr m_pModel;

		
		static void FillTransferInfo(const CModel& model, const CLocation& locations, const CModelInput& modelInput, size_t seed, size_t r, size_t n_r, CTransferInfoIn& info);
	};

	
	ERMsg LoadWeather(const CTeleIO& IO, CSimulationPointVector& simulationPoints);
}//WBSF
