//***********************************************************************
#include "Basic/UtilTime.h"
#include "Basic/Mtrx.h"
#include "Geomatic/GDALBasic.h"
#include "Geomatic/LandsatDataset.h"


namespace WBSF
{

	class CLandsatCloudCleaner;
	class CMergeImagesOption : public CBaseOptions
	{
	public:


		enum TFilePath		{ INPUT_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };
		enum TDebugBands	{ D_CAPTOR, D_PATH, D_ROW, D_YEAR, D_MONTH, D_DAY, D_JDAY, NB_IMAGES, SCENE, SORT_TEST, ISOLATED, BUFFER, NB_DEBUG_BANDS };

		enum TStat			{ S_LOWEST, S_MEAN, S_STD_DEV, S_HIGHEST, NB_STATS };
		enum TMerge			{ UNKNOWN = -1, OLDEST, NEWEST, MAX_NDVI, BEST_PIXEL, SECOND_BEST, BEST_NEWEST, NB_MERGE_TYPE };


		static const short BANDS_STATS[NB_STATS];
		static const char* MERGE_TYPE_NAME[NB_MERGE_TYPE];
		static const char* DEBUG_NAME[NB_DEBUG_BANDS];
		static const char* STAT_NAME[NB_STATS];

		static short GetMergeType(const char* str);

		CMergeImagesOption();
		virtual ERMsg ParseOption(int argc, char* argv[]);
		virtual ERMsg ProcessOption(int& i, int argc, char* argv[]);
		bool IsBusting(Color8 R, Color8 G, Color8 B)const
		{
			bool r = (R < m_bust[0] || R > m_bust[1]);
			bool g = (G < m_bust[0] || G > m_bust[1]);
			bool b = (B < m_bust[0] || B > m_bust[1]);
			return (r || g || b);
		}


		std::string m_cloudsCleanerModel;
		std::array<std::string, 2> m_mosaicFilePath;
		size_t m_mergeType;
		bool m_bDebug;
		bool m_bExportStats;

		std::array<int, 2> m_clear;
		int m_buffer;
		std::array<int, 2> m_bust;
		int m_QA;
		__int16 m_QAmiss;
		double m_meanDmax;
		


		//audit
		size_t m_nbPixelDT;
		size_t m_nbPixel;
		CStatistic m_nbImages;
	};

	

	typedef std::deque < std::vector< std::vector<__int16>>> OutputData;
	typedef std::deque < std::vector< std::vector<__int32>>> DebugData;
	typedef std::deque < std::vector< std::vector<__int16>>> StatData;
	
	
	typedef std::multimap<CTRef, size_t> Test1Vector;
	typedef std::set<CTRef> CTRefSet;


	class CMergeImages
	{
	public:

		ERMsg Execute();

		std::string GetDescription() { return  std::string("MergeImages version ") + VERSION + " (" + __DATE__ + ")"; }
		void InitMemory(size_t sceneSize, CGeoSize blockSize, OutputData& outputData, DebugData& debugData, StatData& statsData);

		ERMsg OpenInput(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CLandsatDataset& mosaicDS1, CLandsatDataset& mosaicDS2);
		ERMsg OpenOutput(CGDALDatasetEx& outputDS, CGDALDatasetEx& debugDS, CGDALDatasetEx& statsDS);
		std::string GetBandFileTitle(const std::string& filePath, size_t b);

		void ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, CBandsHolder& mosaicBH1, CBandsHolder& mosaicBH2);
		void ProcessBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, CGDALDatasetEx& inputDS, CBandsHolder& mosaicBH1, CBandsHolder& mosaicBH2, CGDALDatasetEx& outputDS, OutputData& outputData, DebugData& debugData, StatData& statsData, CLandsatCloudCleaner& cloudsCleaner);
		void WriteBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, CGDALDatasetEx& outputDS, CGDALDatasetEx& debugDS, CGDALDatasetEx& statsDS, OutputData& outputData, DebugData& debugData, StatData& statsData);
		void CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CLandsatDataset& mosaicDS1, CLandsatDataset& mosaicDS2, CGDALDatasetEx& outputDS, CGDALDatasetEx& debugDS, CGDALDatasetEx& statsDS);

		static size_t get_iz(Test1Vector& imageList, size_t type);
		static Test1Vector::iterator get_it(Test1Vector& imageList, size_t type);
		
		
		bool IsIsolatedPixel(int x, int y, const CMatrix<size_t>& firstChoice, const CMatrix<size_t>& selectedChoice)const;
		bool IsCloud(CLandsatCloudCleaner& cloudsCleaner, const CLandsatPixel & pixel1, const CLandsatPixel & pixel2);
		
		size_t GetCloudFreeIz(Test1Vector& imageList, size_t mergeType, CLandsatWindow& preWindow, CLandsatWindow& window, CLandsatWindow& posWindow, int x, int y, CLandsatCloudCleaner& cloudsCleaner);
		CLandsatPixel GetPixel(CLandsatWindow& window, size_t iz1, size_t iz2, int x, int y);
		__int16 GetMeanQA(CLandsatWindow& window, CGeoSize size, size_t iz, int x, int y);
		//bool IsIsolatedPixel(int x, int y, CLandsatCloudCleaner& cloudsCleaner, const CMatrix<int>& DTCode)const;

		CMergeImagesOption m_options;

		static const char* VERSION;
		static const int NB_THREAD_PROCESS;
	};
}