//***********************************************************************
#include "Geomatic/GDALBasic.h"
#include <deque>
//#include <boost\dynamic_bitset.hpp>
#include "DelivaStat.h"

namespace WBSF
{

	class CDeriveLiDARVariableOption : public CBaseOptions
	{
	public:

		enum TFrequency{ FREQUENCY = -1 };

		CDeriveLiDARVariableOption();

		virtual ERMsg ParseOption(int argc, char* argv[]);
		virtual ERMsg ProcessOption(int& i, int argc, char* argv[]);
		bool IsRemoved(const std::string& str)const;

//		std::deque<int> m_condition;
		std::string m_XHeader;
		std::string m_YHeader;
		int m_precision;

		CLiDARStatSelection m_stats;
	};


	typedef std::deque < std::vector<float> > OutputData;

	//***********************************************************************
	//									 
	//	Main                                                             
	//									 
	//***********************************************************************
	class CDeriveLiDARVariable
	{
	public:
		
		enum TFilePath { INPUT_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };

		std::string GetDescription(){ return  std::string("DeriveLiDARVariable version ") + VERSION + " (" + __DATE__ + ")"; }
		ERMsg Execute();

		ERMsg OpenAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS);
		void ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder);
		void ProcessBlock(int xBlock, int yBlock, const CBandsHolder& bandHolder,  OutputData& output);
		void WriteBlock(int xBlock, int yBlock, CGDALDatasetEx& outputDS, OutputData& output);
		void CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS);

		CDeriveLiDARVariableOption m_optionsIn;
		CDeriveLiDARVariableOption m_optionsOut;

		static const char * VERSION;
		static const int NB_THREAD_PROCESS;
	};


}