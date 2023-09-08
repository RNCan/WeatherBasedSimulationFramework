//***********************************************************************


#include "Basic/UtilTime.h"
//#include "Basic/Mtrx.h"
#include "Geomatic/GDALBasic.h"
#include "Geomatic/LandsatDataset1.h"


namespace WBSF
{


	class CBreaksImageOption : public CBaseOptions
	{
	public:


		enum TFilePath		{ BREAKS_FILE_PATH, INPUT_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };
		//enum TDebugBands	{ D_JDAY, NB_IMAGES, NB_DEBUG_BANDS };
		//enum TInfoImage		{ I_CAPTOR, I_PATH, I_ROW};

		CBreaksImageOption();
		virtual ERMsg ParseOption(int argc, char* argv[]);
		virtual ERMsg ProcessOption(int& i, int argc, char* argv[]);

		
		bool m_bDebug;
		int m_firstYear;
		//bool m_bFilterTCB;
		//size_t m_bufferTCB;
		//double m_TCBthreshold[2];
		
		
		void InitFileInfo(Landsat1::CLandsatDataset& inputDS);
		std::vector<Landsat1::CLandsatFileInfo> m_info;

		//static const char* DEBUG_NAME[NB_DEBUG_BANDS];
	};

	
	typedef std::deque < std::vector<__int16> > OutputData;
	//typedef std::deque < std::vector<__int16>> DebugData;

	class CBreaksImage
	{
	public:



		ERMsg Execute();

		std::string GetDescription();

		ERMsg OpenAll(CGDALDatasetEx& breaksDS, Landsat1::CLandsatDataset& inputDS, CGDALDatasetEx& maskDS, Landsat1::CLandsatDataset& outputDS);
		void ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder1, CBandsHolder& bandHolder2);
		void ProcessBlock(int xBlock, int yBlock, CBandsHolder& bandHolder1, CBandsHolder& bandHolder2, OutputData& outputData);
		void WriteBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, CGDALDatasetEx& outputDS, OutputData& outputData);
		void CloseAll(CGDALDatasetEx& breaksDS, CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS);


		CBreaksImageOption m_options;


		static const char* VERSION;
		static const int NB_THREAD_PROCESS;
	};

}