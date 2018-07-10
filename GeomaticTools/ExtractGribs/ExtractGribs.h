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

		//enum TOutputType  { O_IMAGES, O_CSV, NB_OUTPUT_TYPE};
		enum TFilePath	  { GRIBS_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };
		
		CExtractGribsOption();
		virtual ERMsg ParseOption(int argc, char* argv[]);
		virtual ERMsg ProcessOption(int& i, int argc, char* argv[]);


		std::vector<double> m_levels;
		StringVector m_variables;
		std::string m_locations_file_path;
		bool m_bWS;
		bool m_bWD;

		bool m_bOutputCSV;
		//CLocation m_locations;
	};



	class CGribsWeather
	{
	public:


		CGribsWeather()
		{
		}

		ERMsg open(const std::string& filepath, CCallback& callback);
		ERMsg Discard(CCallback& callback);


		ERMsg load(__int64 UTCWeatherTime, CCallback& callback);
		//CATMWeatherCuboidsPtr get_cuboids(const CGeoPoint3D& pt, __int64 UTCWeatherTime)const;

		//CATMVariables get_weather(const CGeoPoint3D& pt, __int64 UTCWeatherTime, __int64 UTCCurrentTime)const;
		std::string get_image_filepath(__int64 UTCWeatherTime)const;

		CGeoPoint3DIndex get_xyz(const CGeoPoint3D& pt, __int64 UTCWeatherTime)const;

		CGeoPointIndex get_xy(const CGeoPoint& pt, __int64 UTCWeatherTime)const;
		size_t get_level(const CGeoPointIndex& xy, const CGeoPoint3D& pt, __int64 UTCWeatherTime, bool bLow)const;
		double GetFirstAltitude(const CGeoPointIndex& xy, __int64 UTCWeatherTime)const;

		bool is_init()const { return !m_filepath_map.empty(); }

		size_t GetGribsPrjID(__int64 UTCWeatherTime)const;

		CGDALDatasetCachedPtr& at(__int64 UTCWeatherTime) { return m_p_weather_DS.at(UTCWeatherTime); }
		bool IsLoaded(__int64 UTCWeatherTime)const;

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
		//void AllocateMemory(size_t sceneSize, GeoBasic::CGeoSize blockSize, OutputData& outputData, DebugData& debugData, OutputData& statsData);

		ERMsg OpenAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CLocationVector& locations, CGDALDatasetEx outputDS, ofStream& CSV_file);
		void ReadBlock(int xBlock, int yBlock, __int64 UTCWeatherTime, CGribsWeather& weather);
		void ProcessBlock(int xBlock, int yBlock, __int64 UTCWeatherTime, CGribsWeather& weather, OutputData& outputData);
		void WriteBlock(int xBlock, int yBlock, CGDALDatasetEx& outputDS, ofStream& CSV_file, OutputData& outputData);
		void CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS, ofStream& CSV_file);

		CExtractGribsOption m_options;

		static const char* VERSION;
		static const int NB_THREAD_PROCESS;
	};
}