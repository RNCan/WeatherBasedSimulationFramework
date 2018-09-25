//***********************************************************************
#include "Geomatic/GDALBasic.h"
#include "Geomatic/LandsatDataset.h"
#include <boost\dynamic_bitset.hpp>

namespace WBSF
{
	enum TCondition{ ALL_VALID, AT_LEAST_ONE_VALID, AT_LEAST_ONE_MISSING, ALL_MISSING, NB_CONDITION };
	static const char * CONDITION_NAME[NB_CONDITION] = { "AllValid", "AtLeastOneValid", "AtLeastOneMissing", "AllMissing" };

	class CTrainingCreatorOption : public CBaseOptions
	{
	public:

		enum TFrequency{ FREQUENCY = -1 };

		CTrainingCreatorOption();

		virtual ERMsg ParseOption(int argc, char* argv[]);
		virtual ERMsg ProcessOption(int& i, int argc, char* argv[]);
		bool IsRemoved(const std::string& str)const;

		std::deque<int> m_condition;
		std::string m_XHeader;
		std::string m_YHeader;
		std::string m_THeader;
		std::string m_IHeader;
		int m_precision;
		std::array<int, 2> m_nbPixels;
		std::string m_refFilePath;
		bool m_bRefMedian;
		bool m_bExportAllBand;
		bool m_bExportAll;
		bool m_bTakeSerial;


		int GetNbPixels()const { return m_nbPixels[0] + m_nbPixels[1] + 1 + (m_bRefMedian?1:0); }

		size_t nbBandExport()const { return m_bExportAllBand ? Landsat::SCENES_SIZE : Landsat::QA; }
	};



	//***********************************************************************
	//									 
	//	Main                                                             
	//									 
	//***********************************************************************
	class CTrainingCreator
	{
	public:

		enum TCondition{ ALL_VALID, AT_LEAST_ONE_VALID, AT_LEAST_ONE_MISSING, ALL_MISSING, NB_CONDITION };
		enum TFilePath { IMAGE_FILE_PATH, INPUT_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };

		std::string GetDescription(){ return  std::string("TrainingCreator version ") + VERSION + " (" + __DATE__ + ")"; }
		ERMsg Execute();

		ERMsg OpenAll(CLandsatDataset& inputDS, CGDALDatasetEx& maskDS, CLandsatDataset& refDS, CGeoCoordTimeFile& iFile, CGeoCoordTimeFile& oFile);
		static bool AtLeastOnePointIn(const CGeoExtents& blockExtents, const CGeoCoordTimeFile& inputFile);
		void ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, CBandsHolder& bandHolderRef);
		void ProcessBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, CBandsHolder& bandHolderRef, CGeoCoordTimeFile& iFile, CGeoCoordTimeFile& oFile, boost::dynamic_bitset<size_t>& treated);
		void CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CLandsatDataset& refDS);

		CTrainingCreatorOption m_options;

		static const char * VERSION;
		static const int NB_THREAD_PROCESS;
		static const char * CONDITION_NAME[NB_CONDITION];
	};


}