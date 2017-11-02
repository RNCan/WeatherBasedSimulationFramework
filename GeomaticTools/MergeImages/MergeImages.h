//***********************************************************************
#include "Basic/UtilTime.h"
#include "Basic/Mtrx.h"
#include "Geomatic/GDALBasic.h"
#include "Geomatic/LandsatDataset.h"


namespace WBSF
{

	class CMergeImagesOption : public CBaseOptions
	{
	public:


		enum TFilePath		{ INPUT_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };
		enum TDebugBands	{ D_CAPTOR, D_PATH, D_ROW, D_YEAR, D_MONTH, D_DAY, D_JDAY, NB_IMAGES, SCENE, SORT_TEST, NB_DEBUG_BANDS };
		

		enum TStat			{ S_LOWEST, S_MEAN, S_STD_DEV, S_HIGHEST, NB_STATS };
		enum TMerge			{ UNKNOWN = -1, OLDEST, NEWEST, MAX_NDVI, BEST_PIXEL, SECOND_BEST, MEDIAN_NDVI, MEDIAN_NBR, MEDIAN_NDMI, NB_MERGE_TYPE };


		static const short BANDS_STATS[NB_STATS];
		static const char* MERGE_TYPE_NAME[NB_MERGE_TYPE];
		static const char* DEBUG_NAME[NB_DEBUG_BANDS];
		static const char* STAT_NAME[NB_STATS];

		static short GetMergeType(const char* str);

		

		CMergeImagesOption();
		virtual ERMsg ParseOption(int argc, char* argv[]);
		virtual ERMsg ProcessOption(int& i, int argc, char* argv[]);

		size_t m_mergeType;
		size_t m_medianType;
		bool m_bDebug;
		bool m_bExportStats;

	};
	

	typedef std::deque < std::vector< std::vector<__int16>>> OutputData;
	typedef std::deque < std::vector< std::vector<__int32>>> DebugData;
	typedef std::deque < std::vector< std::vector<__int16>>> StatData;
	
	typedef std::multimap<CTRef, size_t > Test1Vector;
	typedef std::set<CTRef> CTRefSet;


	class CMergeImages
	{
	public:

		ERMsg Execute();

		std::string GetDescription() { return  std::string("MergeImages version ") + VERSION + " (" + __DATE__ + ")"; }
		void InitMemory(size_t sceneSize, CGeoSize blockSize, OutputData& outputData, DebugData& debugData, StatData& statsData);

		ERMsg OpenInput(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS);
		ERMsg OpenOutput(CGDALDatasetEx& outputDS, CGDALDatasetEx& debugDS, CGDALDatasetEx& statsDS);
		std::string GetBandFileTitle(const std::string& filePath, size_t b);

		void ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder);
		void ProcessBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, OutputData& outputData, DebugData& debugData, StatData& statsData, CGDALDatasetEx& inputDS);
		void WriteBlock(int xBlock, int yBlock, CGDALDatasetEx& outputDS, CGDALDatasetEx& debugDS, CGDALDatasetEx& statsDS, OutputData& outputData, DebugData& debugData, StatData& statsData);
		void CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS, CGDALDatasetEx& debugDS, CGDALDatasetEx& statsDS);

		static CTRef GetCriterion(CLandsatPixel& pixel, size_t type);
		static size_t get_iz(const Test1Vector& imageList, size_t type1, const Test1Vector& imageList2, size_t type2);
		static Test1Vector::const_iterator get_it(const Test1Vector& imageList, size_t type);
		static Test1Vector::const_iterator get_it(const Test1Vector& imageList, size_t type1, const Test1Vector& imageList2, size_t type2);


		CMergeImagesOption m_options;

		static const char* VERSION;
		static const size_t NB_THREAD_PROCESS;
	};
}