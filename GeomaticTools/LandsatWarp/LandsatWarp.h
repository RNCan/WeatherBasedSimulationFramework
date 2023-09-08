//***********************************************************************


#include "Geomatic/GDALBasic.h"
#include "Basic/UtilTime.h"
#include "Geomatic/LandsatDataset1.h"

namespace WBSF
{
	class CLandsatCloudCleaner;

	class CLandsatWarpOption : public CBaseOptions
	{
	public:


		enum TFilePath		{ INPUT_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };

		CLandsatWarpOption();
		virtual ERMsg ParseOption(int argc, char* argv[]);
		virtual ERMsg ProcessOption(int& i, int argc, char* argv[]);


		std::string m_rename;
		Landsat1::TCorr8 m_corr8;
	};


	typedef std::deque<std::deque<std::vector<__int16>>> OutputData;


	class CLandsatWarp
	{
	public:

		ERMsg Execute();

		std::string GetDescription() { return  std::string("LandsatWarp version ") + VERSION + " (" + __DATE__ + ")"; }
		//void AllocateMemory(size_t sceneSize, GeoBasic::CGeoSize blockSize, OutputData& outputData, DebugData& debugData, OutputData& statsData);

		ERMsg OpenInput(Landsat1::CLandsatDataset& inputDS, CGDALDatasetEx& maskDS);
		ERMsg OpenOutput(Landsat1::CLandsatDataset& inputDS, Landsat1::CLandsatDataset& outputDS, const std::set<size_t>& selected);

		void ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder);
		void ProcessBlock(int xBlock, int yBlock, std::set<size_t>& imagesList, CBandsHolder& bandHolder, OutputData& outputData);
		void WriteBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, CGDALDatasetEx& outputDS, OutputData& outputData);
		void CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS);

		std::set<size_t> GetImageList(int xBlock, int yBlock, CBandsHolder& bandHolder, Landsat1::CLandsatDataset& input);


		CLandsatWarpOption m_options;

		static const char* VERSION;
		static const int NB_THREAD_PROCESS;
	};
}