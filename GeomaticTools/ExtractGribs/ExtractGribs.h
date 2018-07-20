//***********************************************************************


#include "Geomatic/GDALBasic.h"
#include "Basic/UtilTime.h"
#include "Simulation/ATM.h"

namespace WBSF
{
	class CLandsatCloudCleaner;

	class CExtractGribsOption : public CBaseOptions
	{
	public:

		enum { ATM_WNDS = NB_ATM_VARIABLES_EX, ATM_WNDD, NB_ATM_VARIABLES_EX2 };
		//enum TOutputType  { O_IMAGES, O_CSV, NB_OUTPUT_TYPE};
		enum TFilePath	  { GRIBS_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };
		
		CExtractGribsOption();
		CExtractGribsOption(const CExtractGribsOption& in):
			CBaseOptions(in)
		{
			m_levels = in.m_levels;
			m_variables = in.m_variables;
			m_times = in.m_times;
			m_locations_file_path = in.m_locations_file_path;
			m_time_step = in.m_time_step;
		}

		virtual ERMsg ParseOption(int argc, char* argv[]);
		virtual ERMsg ProcessOption(int& i, int argc, char* argv[]);


		std::vector<double> m_levels;
		std::vector<size_t> m_variables;
		std::vector<__int64> m_times;
		std::string m_locations_file_path;
		size_t m_time_step;
		
		CProjectionTransformation toWea;
		CProjectionTransformation toGeo;
	};



	class CGribsWeather
	{
	public:


		CGribsWeather()
		{}

		ERMsg open(const std::string& filepath, CBaseOptions& options, CCallback& callback= CCallback::DEFAULT_CALLBACK);
		ERMsg close(CCallback& callback = CCallback::DEFAULT_CALLBACK);
		void UpdateOption(CBaseOptions& options);

		
		ERMsg load(__int64 UTCWeatherTime, CCallback& callback = CCallback::DEFAULT_CALLBACK);
		void read_block(int xBlock, int yBlock, __int64 UTCWeatherTime );
		CATMVariables get_weather(CGeoPoint3D pt, __int64 UTCWeatherTime, __int64 UTCCurrentTime)const;
		//CATMWeatherCuboidsPtr get_cuboids(const CGeoPoint3D& pt, __int64 UTCWeatherTime)const;

		//CATMVariables get_weather(const CGeoPoint3D& pt, __int64 UTCWeatherTime, __int64 UTCCurrentTime)const;
		std::string get_image_filepath(__int64 UTCWeatherTime)const;

		CGeoPoint3DIndex get_xyz(const CGeoPoint3D& pt, __int64 UTCWeatherTime)const;

		CGeoPointIndex get_xy(const CGeoPoint& pt, __int64 UTCWeatherTime)const;
		size_t get_level(const CGeoPointIndex& xy, const CGeoPoint3D& pt, __int64 UTCWeatherTime, bool bLow)const;
		double GetFirstAltitude(const CGeoPointIndex& xy, __int64 UTCWeatherTime=0)const;

		bool is_init()const { return !m_filepath_map.empty(); }

		size_t GetPrjID(__int64 UTCWeatherTime=0)const;
		CGeoExtents GetExtents(__int64 UTCWeatherTime=0)const;
		size_t GetRasterCount(__int64 UTCWeatherTime=0)const;

		CGDALDatasetCachedPtr& at(__int64 UTCWeatherTime) { return m_p_weather_DS.at(UTCWeatherTime); }
		bool IsLoaded(__int64 UTCWeatherTime)const;
		CATMWeatherCuboidsPtr get_cuboids(const CGeoPoint3D& pt, __int64 UTCWeatherTime)const;

		__int64 GetNearestFloorTime(__int64 UTCTime)const;
		__int64 GetNextTime(__int64 UTCTime)const;
		CTPeriod GetEntireTPeriod()const;

	protected:

		std::string m_filePathGribs;

		TTimeFilePathMap m_filepath_map;
		CTimeDatasetMap m_p_weather_DS;
	};

	typedef std::deque<std::vector<float>> OutputData;


	class CExtractGribs
	{
	public:

		ERMsg Execute();

		

		std::string GetDescription() { return  std::string("ExtractGribs version ") + VERSION + " (" + __DATE__ + ")"; }
		
		ERMsg OpenAll(CGribsWeather& inputDS, CLocationVector& locations, CGDALDatasetEx& outputDS, ofStream& CSV_file);
		void ReadBlock(int xBlock, int yBlock, __int64 UTCWeatherTime, CGribsWeather& weather);
		void ProcessBlock(int xBlock, int yBlock, CGribsWeather& weather, OutputData& outputData);
		void WriteBlock(int xBlock, int yBlock, OutputData& outputData, CGDALDatasetEx& outputDS);
		void ProcessBlock(CLocationVector& locations, CGribsWeather& weather, OutputData& outputData);
		void WriteBlock(CLocationVector& locations, OutputData& outputData, ofStream& CSV_file);
		
		void CloseAll(CGribsWeather& inputDS, CGDALDatasetEx& outputDS, ofStream& CSV_file);

		CExtractGribsOption m_options;
		


		static const char* VERSION;
		static const int NB_THREAD_PROCESS;
	};
}