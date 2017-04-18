//***********************************************************************
#include "Geomatic/GDALBasic.h"
#include <boost\dynamic_bitset.hpp>

namespace WBSF
{
	enum TCondition{ ALL_VALID, AT_LEAST_ONE_VALID, AT_LEAST_ONE_MISSING, ALL_MISSING, NB_CONDITION };
	static const char * CONDITION_NAME[NB_CONDITION] = { "AllValid", "AtLeastOneValid", "AtLeastOneMissing", "AllMissing" };

	class CPointsExtractorOption : public CBaseOptions
	{
	public:

		enum TFrequency{ FREQUENCY = -1 };

		CPointsExtractorOption();

		virtual ERMsg ParseOption(int argc, char* argv[]);
		virtual ERMsg ProcessOption(int& i, int argc, char* argv[]);
		bool IsRemoved(const std::string& str)const;

		std::deque<int> m_condition;
		std::string m_XHeader;
		std::string m_YHeader;
		int m_precision;
	};



	//***********************************************************************
	//									 
	//	Main                                                             
	//									 
	//***********************************************************************
	class CPointsExtractor
	{
	public:

		enum TCondition{ ALL_VALID, AT_LEAST_ONE_VALID, AT_LEAST_ONE_MISSING, ALL_MISSING, NB_CONDITION };
		enum TFilePath { IMAGE_FILE_PATH, INPUT_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };

		std::string GetDescription(){ return  std::string("PointsExtractor version ") + VERSION + " (" + __DATE__ + ")"; }
		ERMsg Execute();

		ERMsg OpenAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGeoCoordFile& inputFile);
		static bool AtLeastOnePointIn(const CGeoExtents& blockExtents, const CGeoCoordFile& inputFile);
		void ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder);
		void ProcessBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, CGeoCoordFile& inputFile, boost::dynamic_bitset<size_t>& treated);
		void CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS);

		CPointsExtractorOption m_options;

		static const char * VERSION;
		static const int NB_THREAD_PROCESS;
		static const char * CONDITION_NAME[NB_CONDITION];
	};


}