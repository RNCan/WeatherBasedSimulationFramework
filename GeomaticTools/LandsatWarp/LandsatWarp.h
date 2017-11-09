//***********************************************************************


#include "Geomatic/GDALBasic.h"
#include "Basic/UtilTime.h"
#include "Geomatic/LandsatDataset.h"

namespace WBSF
{
	class CLandsatCloudCleaner;

	class CLandsatWarpOption : public CBaseOptions
	{
	public:


		enum TFilePath		{ INPUT_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };

		static short GetMergeType(const char* str);

		CLandsatWarpOption();
		virtual ERMsg ParseOption(int argc, char* argv[]);
		virtual ERMsg ProcessOption(int& i, int argc, char* argv[]);

	};


	typedef std::deque<std::deque<std::vector<__int16>>> OutputData;


	class CLandsatWarp
	{
	public:

		ERMsg Execute();

		std::string GetDescription() { return  std::string("LandsatWarp version ") + VERSION + " (" + __DATE__ + ")"; }
		//void AllocateMemory(size_t sceneSize, GeoBasic::CGeoSize blockSize, OutputData& outputData, DebugData& debugData, OutputData& statsData);

		ERMsg OpenInput(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS);
		ERMsg OpenOutput(CGDALDatasetEx& outputDS);

		void ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder);
		void ProcessBlock(int xBlock, int yBlock, std::set<size_t>& imagesList, CBandsHolder& bandHolder, OutputData& outputData);
		void WriteBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, CGDALDatasetEx& outputDS, OutputData& outputData);
		void CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS);

		std::set<size_t> GetImageList(int xBlock, int yBlock, CBandsHolder& bandHolder, CLandsatDataset& input);


		CLandsatWarpOption m_options;

		static const char* VERSION;
		static const int NB_THREAD_PROCESS;
	};
}