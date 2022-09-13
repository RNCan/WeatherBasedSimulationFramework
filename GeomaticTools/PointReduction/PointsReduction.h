//***********************************************************************
#include "Geomatic/GDALBasic.h"
#include <boost\dynamic_bitset.hpp>

namespace WBSF
{
	class CApproximateNearestNeighbor;
	class CSearchResultVector;
	class CLocation;
	class CLocationVector;

	class CPointsReductionOption : public CBaseOptions
	{
	public:

		enum TFrequency{ FREQUENCY = -1 };

		CPointsReductionOption();

		virtual ERMsg ParseOption(int argc, char* argv[]);
		virtual ERMsg ProcessOption(int& i, int argc, char* argv[]);
		//bool IsRemoved(const std::string& str)const;

		double m_distanceMin;
		std::string m_XHeader;
		std::string m_YHeader;
		int m_sort;
		size_t m_maxN;
//		int m_precision;
	};



	//***********************************************************************
	//									 
	//	Main                                                             
	//									 
	//***********************************************************************
	class CPointsReduction
	{
	public:

	
		enum TFilePath { INPUT_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };

		std::string GetDescription(){ return  std::string("PointsReduction version ") + VERSION + " (" + __DATE__ + ")"; }
		ERMsg Execute();

		ERMsg OpenAll(CLocationVector& locations);
		//static bool AtLeastOnePointIn(const CGeoExtents& blockExtents, const CGeoCoordFile& inputFile);
		
		//void ProcessBlock(double d, const CLocationVector& locations, boost::dynamic_bitset<size_t>& status, WBSF::CCallback& callback);
		//void CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS);
		void SearchD(CApproximateNearestNeighbor& ann, CSearchResultVector& searchResultArray, const CLocation& location, double d);
		//ERMsg GenerateWellDistributedStation();


		CPointsReductionOption m_options;

		static const char * VERSION;
		static const int NB_THREAD_PROCESS;
		
	};


}