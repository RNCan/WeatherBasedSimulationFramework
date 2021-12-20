//***********************************************************************

#include <bitset>
#include "Basic/UtilTime.h"
#include "Geomatic/GDALBasic.h"
#include "Geomatic/MODISDataset.h"

namespace WBSF
{
	
	typedef std::deque<std::deque<std::vector<__int16>>> OutputData;

	class CMODISIndicesOption : public CBaseOptions
	{
	public:


		enum TFilePath		{ INPUT_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };


		CMODISIndicesOption();
		virtual ERMsg ParseOption(int argc, char* argv[]);
		virtual ERMsg ProcessOption(int& i, int argc, char* argv[]);

		CMODISIndiciesVector m_despike;
		
		std::bitset<MODIS::NB_INDICES> m_indices;
		std::array<size_t, 2> m_scenes;
		bool m_bVirtual;
		size_t m_rings;
		std::vector<double> m_weight;
		std::string m_weight_str;
	};


	class CMODISIndices
	{
	public:

		ERMsg Execute();

		std::string GetDescription() { return  std::string("MODISIndices version ") + VERSION + " (" + __DATE__ + ")"; }

		ERMsg OpenAll(CMODISDataset& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS);

		void ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder);
		void ProcessBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, OutputData& outputData);
		void WriteBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, CGDALDatasetEx& outputDS, OutputData& outputData);
		void CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS);
		ERMsg CreateVirtual(CMODISDataset& inputDS);


		CMODISIndicesOption m_options;

		static const char* VERSION;
		static const int NB_THREAD_PROCESS;
	};
}