//***********************************************************************


#include "geomatic/UtilGDAL.h"
#include "geomatic/LandsatDataset2.h"

namespace WBSF
{
	//class CLandsatCloudCleaner;

	typedef std::deque<std::vector<Color8>> OutputData;

	class CLandsat2RGBOption : public CBaseOptions
	{
	public:


		enum TFilePath		{ INPUT_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };

		//static short GetMergeType(const char* str);

		CLandsat2RGBOption();
		virtual ERMsg ParseOption(int argc, char* argv[]);
		virtual ERMsg ProcessOption(int& i, int argc, char* argv[]);

		bool IsBusting(Color8 R, Color8 G, Color8 B)const
		{
			bool r = (R < m_bust[0] || R > m_bust[1]);
			bool g = (G < m_bust[0] || G > m_bust[1]);
			bool b = (B < m_bust[0] || B > m_bust[1]);
			return (r || g || b);// && (!r || !g || !b);//if the three is busting, we keep it (white)
			//return ((r && !g && !b) || (!r && g && !b) || (!r && !g && b));//XOR operation at 3
		}
		
		//std::array<size_t, 2> m_scenes;
		std::array<int, 2> m_bust;
		bool m_bVirtual;

		std::vector< Landsat2::CBandStats> m_stats;
	};


	class CLandsat2RGB
	{
	public:

		ERMsg Execute();

		std::string GetDescription() { return  std::string("Landsat2RGB version ") + VERSION + " (" + __DATE__ + ")"; }

		ERMsg OpenAll(Landsat2::CLandsatDataset& inputDS, CGDALDatasetEx& maskDS, std::vector<CGDALDatasetEx>& outputDS);
		void ReadBlock(Landsat2::CLandsatDataset& inputDS, int xBlock, int yBlock, Landsat2::CLandsatWindow& block_data);
		void ProcessBlock(int xBlock, int yBlock, Landsat2::CLandsatWindow& block_data, size_t z, OutputData& outputData);
		void WriteBlock(int xBlock, int yBlock, CGDALDatasetEx& outputDS, OutputData& outputData);
		ERMsg CreateVirtual(Landsat2::CLandsatDataset& inputDS);
		void CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, std::vector<CGDALDatasetEx>& outputDS);
		

		CLandsat2RGBOption m_options;

		static const char* VERSION;
		static const int NB_THREAD_PROCESS;
	};
}