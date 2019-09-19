#include "Basic/NormalsDatabase.h"
#include "Basic/WeatherDefine.h"
#include "Simulation/WeatherGenerator.h"


namespace WBSF
{
	class WeatherGeneratorOptions
	{
	public:

		enum TParam { VARIABLES, SOURCE_TYPE, GENERATION_TYPE, NB_REPLICATIONS, LATITUDE, LONGITUDE, ELEVATION, SLOPE, ORIENTATION, NB_NEAREST_NEIGHBOR, FIRST_YEAR, LAST_YEAR, NB_YEARS, SEED, NORMALS_INFO, NB_PAPAMS };
		static const char* NAME[NB_PAPAMS];

		WeatherGeneratorOptions();
		ERMsg parse(const std::string& options);

		std::string m_variables;
		int m_sourceType; //from normal=0, from observation=1
		std::string m_normals_info;//if from normals, period and CC specification ex.: 2071-2100 ECCC RCM2 RCP 4.5
		int m_generationType; //hourly=0; daily=1
		double m_latitude;
		double m_longitude;
		double m_elevation;
		double m_slope;
		double m_orientation;
		size_t m_nb_nearest_neighbor;
		size_t m_nb_replications;
		size_t m_nb_years;
		int m_first_year;
		int m_last_year;
		int m_seed;//0 for random seed else fixed seed 
	};

	typedef std::shared_ptr<CGDALDatasetEx> CGDALDatasetExPtr;
	typedef std::shared_ptr<CWeatherGenerator> CWeatherGeneratorPtr;
	

	class WeatherGenerator
	{

	public:

		enum TParam { SHORE, NORMALS, DAILY, HOURLY, GRIBS, DEM, NB_PAPAMS };
		static const char* NAME[NB_PAPAMS];

		WeatherGenerator(const std::string &);
		std::string Initialize(const std::string& str_options);
		std::string Generate(const std::string& str_options);
//		std::string GetNormals(const std::string& str_options);

	protected:

		//std::string m_normals_file_path;
		//std::string m_daily_file_path;
		//std::string m_hourly_file_path;
		//std::string m_gribs_file_path;
		//std::string m_DEM_file_path;

		CNormalsDatabasePtr m_pNormalDB;
		CDailyDatabasePtr m_pDailyDB;
		CHourlyDatabasePtr m_pHourlyDB;
		CSfcGribDatabasePtr m_pGribsDB;
		CGDALDatasetExPtr m_pDEM;
		
		CWeatherGeneratorPtr m_pWeatherGenerator;


	};
	
}//WBSF
