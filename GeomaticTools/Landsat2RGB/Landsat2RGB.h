//***********************************************************************


#include "Geomatic/GDALBasic.h"
#include "Basic/UtilTime.h"

namespace WBSF
{
	class CLandsatCloudCleaner;

	class CLandsatRGBOption : public CBaseOptions
	{
	public:


		enum TFilePath		{ INPUT_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };

		static short GetMergeType(const char* str);

		CLandsatRGBOption();
		virtual ERMsg ParseOption(int argc, char* argv[]);
		virtual ERMsg ProcessOption(int& i, int argc, char* argv[]);

		//std::vector<std::string> m_imagesName;
		int m_scene;
	};

	typedef unsigned __int8 Color8;
	typedef std::deque<std::vector<Color8>> OutputData;


	class CLandsatRGB
	{
	public:

		ERMsg Execute();

		std::string GetDescription() { return  std::string("LandsatRGB version ") + VERSION + " (" + __DATE__ + ")"; }

		ERMsg OpenInput(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS);
		ERMsg OpenOutput(CGDALDatasetEx& outputDS);

		void ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder);
		void ProcessBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, OutputData& outputData);
		void WriteBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, CGDALDatasetEx& outputDS, OutputData& outputData);
		void CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS);

		CLandsatRGBOption m_options;

		static const char* VERSION;
		static const int NB_THREAD_PROCESS;
	};
}