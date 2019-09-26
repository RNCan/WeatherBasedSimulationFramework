#include "Basic/NormalsDatabase.h"
#include "Basic/WeatherDefine.h"
#include "Simulation/WeatherGenerator.h"
namespace pybind11 { class bytes; }

namespace WBSF
{
	class WeatherGeneratorOptions
	{
	public:

		enum TParam { VARIABLES, SOURCE_TYPE, GENERATION_TYPE, REPLICATIONS, LATITUDE, LONGITUDE, ELEVATION, SLOPE, ORIENTATION, NB_NEAREST_NEIGHBOR, FIRST_YEAR, LAST_YEAR, NB_YEARS, SEED, NORMALS_INFO, COMPRESS, NB_PAPAMS };
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
	

	class Output
	{
	public:

		bool m_compress;
		std::string m_msg;
		std::string m_comment;
		std::string m_text;
		pybind11::bytes m_data;
		
	};


	class WeatherGenerator
	{

	public:

		enum TParam { SHORE, NORMALS, DAILY, HOURLY, GRIBS, DEM, NB_PAPAMS };
		static const char* NAME[NB_PAPAMS];

		WeatherGenerator(const std::string &);
		std::string Initialize(const std::string& str_options);
		Output Generate(const std::string& str_options);
		Output GenerateGribs(const std::string& str_options);


	protected:
		
		CNormalsDatabasePtr m_pNormalDB;
		CDailyDatabasePtr m_pDailyDB;
		CHourlyDatabasePtr m_pHourlyDB;
		CSfcGribExtractorPtr m_pGribsDB;
		//CSfcGribDatabasePtr m_pGribsDB;
		CGDALDatasetExPtr m_pDEM;
		
		CWeatherGeneratorPtr m_pWeatherGenerator;


		ERMsg ComputeElevation(double latitude, double longitude, double& elevation);

	};
	
}//WBSF
