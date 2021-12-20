//***********************************************************************
#include "Geomatic/GDALBasic.h"
#include <boost\dynamic_bitset.hpp>

namespace WBSF
{
	class CPointsExtractorLayerOption : public CBaseOptions
	{
	public:

		enum TFrequency{ FREQUENCY = -1 };

		CPointsExtractorLayerOption();

		virtual ERMsg ParseOption(int argc, char* argv[]);
		virtual ERMsg ProcessOption(int& i, int argc, char* argv[]);
		bool IsRemoved(const std::string& str)const;

		std::string m_XHeader;
		std::string m_YHeader;
//		int m_precision;
	};



	//***********************************************************************
	//									 
	//	Main                                                             
	//									 
	//***********************************************************************
	class CPointsExtractorLayer
	{
	public:

		
		enum TFilePath { LAYER_FILE_PATH, INPUT_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };

		std::string GetDescription(){ return  std::string("PointsExtractorLayer version ") + VERSION + " (" + __DATE__ + ")"; }
		ERMsg Execute();

		CPointsExtractorLayerOption m_options;

		static const char * VERSION;
		static const int NB_THREAD_PROCESS;

	};


}