//***********************************************************************


#include "Basic/UtilTime.h"
//#include "Basic/Mtrx.h"
#include "Geomatic/GDALBasic.h"
#include "Geomatic/LandsatDataset.h"


namespace WBSF
{


	class CMedianImageOption : public CBaseOptions
	{
	public:

		
		enum TFilePath		{ INPUT_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };
		enum TDebugBands	{ D_JDAY, NB_IMAGES, NB_DEBUG_BANDS };
		enum TMean { NO_MEAN=-1, M_STANDARD, M_ALWAYS2, NB_MEAN_TYPE };
		static const char*  MEAN_NAME[NB_MEAN_TYPE];
		

		CMedianImageOption();
		virtual ERMsg ParseOption(int argc, char* argv[]);
		virtual ERMsg ProcessOption(int& i, int argc, char* argv[]);

		
		bool m_bDebug;
		Landsat::TCorr8 m_corr8;
		TMean m_meanType;
		bool m_bBestMedian;
		//bool m_bFilterTCB;
		//size_t m_bufferTCB;
		//double m_TCBthreshold[2];
		
		
		void InitFileInfo(CLandsatDataset& inputDS);
		std::vector<CLandsatFileInfo> m_info;

		static const char* DEBUG_NAME[NB_DEBUG_BANDS];
	};

	typedef __int16 OutputDataType;
	typedef std::deque < std::vector<OutputDataType> > OutputData;
	typedef std::deque < std::vector<__int16>> DebugData;

	class CMedianImage
	{
	public:



		ERMsg Execute();

		std::string GetDescription();
		void AllocateMemory(size_t sceneSize, CGeoSize blockSize, OutputData& outputData);

		ERMsg OpenAll(CLandsatDataset& inputDS, CGDALDatasetEx& maskDS, CLandsatDataset& outputDS, CGDALDatasetEx& debugDS);
		void ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder);
		void ProcessBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, OutputData& outputData, DebugData& debugData);
		void WriteBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, CGDALDatasetEx& outputDS, CGDALDatasetEx& debugDS, OutputData& outputData, DebugData& debugData);
		void CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS, CGDALDatasetEx& debugDS);


		CMedianImageOption m_options;


		static const char* VERSION;
		static const int NB_THREAD_PROCESS;
	};

}