#include "Basic/NormalsDatabase.h"
#include "Basic/WeatherDefine.h"
#include "basic/ModelStat.h"
#include "Simulation/WeatherGenerator.h"
#include "ModelBase/ModelInput.h"
#include "ModelBase/Model.h"

namespace pybind11 { class bytes; }

namespace WBSF
{

	//IO class between c++ and python
	class teleIO
	{
	public:

		bool m_compress;		//if output is compress or not
		std::string m_msg;		//error message
		std::string m_comment;	//comments
		std::string m_metadata;	//output metadata
		std::string m_text;		//output data if not compress
		pybind11::bytes m_data;	//output data if compress

	};



	class WeatherGeneratorOptions
	{
	public:

		enum TParam { VARIABLES, SOURCE_TYPE, GENERATION_TYPE, REPLICATIONS, KEY_ID, NAME, LATITUDE, LONGITUDE, ELEVATION, SLOPE, ORIENTATION, NB_NEAREST_NEIGHBOR, FIRST_YEAR, LAST_YEAR, NB_YEARS, SEED, NORMALS_INFO, COMPRESS, NB_PAPAMS };
		static const char* PARAM_NAME[NB_PAPAMS];

		WeatherGeneratorOptions();
		ERMsg parse(const std::string& options);
		CWGInput GetWGInput()const;

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

	
	typedef std::shared_ptr<CGDALDatasetEx> CGDALDatasetExPtr;
	typedef std::shared_ptr<CWeatherGenerator> CWeatherGeneratorPtr;
	

	class CSfcGribExtractor
	{
	public:

		ERMsg open(const std::string& gribsList);
		void extract(CGeoPoint pt, CWeatherYears& data);
		void load_all();

	protected:

		CGribsMap m_gribsList;
		std::map<CTRef, CSfcDatasetCachedPtr> m_sfcDS;

		CProjectionTransformation GEO_2_WEA;
		CGeoExtents m_extents;

	};

	typedef std::shared_ptr<CSfcGribExtractor> CSfcGribExtractorPtr;



	typedef std::shared_ptr<CModel> CModelPtr;

	class WeatherGenerator
	{

	public:

		enum TParam { SHORE, NORMALS, DAILY, HOURLY, GRIBS, DEM, NB_PAPAMS };
		static const char* NAME[NB_PAPAMS];

		WeatherGenerator(const std::string &);
		std::string Initialize(const std::string& str_options);
		teleIO Generate(const std::string& str_options);
		teleIO GenerateGribs(const std::string& str_options);
		teleIO GetNormals(const std::string& str_options);
		
	protected:
		
		CNormalsDatabasePtr m_pNormalDB;
		CDailyDatabasePtr m_pDailyDB;
		CHourlyDatabasePtr m_pHourlyDB;
		CSfcGribExtractorPtr m_pGribsDB;
		//CSfcGribDatabasePtr m_pGribsDB;
		CGDALDatasetExPtr m_pDEM;
		
		CWeatherGeneratorPtr m_pWeatherGenerator;


		ERMsg ComputeElevation(double latitude, double longitude, double& elevation);
		void SaveNormals(std::ostream& out, const CNormalsStation& normals);
		


	};



	//*************************************************************************************************
	//Model
	class ModelExecutionOptions
	{
	public:

		enum TParam { PARAMETERS, REPLICATIONS, SEED, COMPRESS, NB_PAPAMS };
		static const char* PARAM_NAME[NB_PAPAMS];

		ModelExecutionOptions();
		ERMsg parse(const std::string& options);
		ERMsg GetModelInput(const CModel& model, CModelInput& modelInput)const;

		std::string m_parameters; //input param [coma format]

		size_t m_replications;
		int m_seed;//0 for random seed else fixed seed 
		bool m_compress;
	};

	class CTransferInfoIn;
	class ModelExecution
	{

	public:

		enum TParam { MODEL, NB_PAPAMS };
		static const char* NAME[NB_PAPAMS];

		ModelExecution(const std::string &);
		std::string Initialize(const std::string& str_options);
		teleIO Execute(const std::string& str_options, const teleIO& input);


	protected:
		
		CModelPtr m_pModel;
		
		//ERMsg RunModel(const ModelExecutionOptions& options, const CSimulationPointVector& simulationPoints, CModelStatVector& output, CCallback& callback);
		static CTransferInfoIn FillTransferInfo(const CModel& model, const CLocation& locations, const CModelInput& modelInput, size_t seed, size_t r, size_t n_r);
	};


	ERMsg LoadWeather(const teleIO& IO, CSimulationPointVector& simulationPoints);
}//WBSF
