//***********************************************************************

#include <bitset>
#include "Basic/UtilTime.h"
#include "Geomatic/GDALBasic.h"
#include "Geomatic/LandsatDataset.h"

namespace WBSF
{
	class CDecisionTree;
	class CDecisionTreeBaseEx;
	typedef CDecisionTreeBaseEx CSee5Tree;
	typedef CDecisionTree CSee5TreeMT;
	
	enum TOutputs { O_DT_CODE, O_T1, O_DELTA_NBR, NB_OUTPUTS};
	enum TBreaksOutput { O_NBR, O_T, NB_BREAKS_OUTPUT };

	typedef std::deque < std::array < std::vector<__int16>, NB_OUTPUTS>> OutputData;
	typedef std::deque< std::deque < std::vector<__int16>>> SegmentData;
	typedef std::deque< std::array < std::vector<__int16>, NB_BREAKS_OUTPUT>> BreakData;

	class CDisturbanceAnalyserIIOption : public CBaseOptions
	{
	public:

		enum TFilePath		{ MODEL_FILE_PATH, INPUT_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };


		CDisturbanceAnalyserIIOption();
		virtual ERMsg ParseOption(int argc, char* argv[]);
		virtual ERMsg ProcessOption(int& i, int argc, char* argv[]);

		CIndiciesVector m_despike;
		
		Landsat::TIndices m_indice;
		std::array<size_t, 2> m_scenes;
	
		size_t m_rings;
		std::vector<double> m_weight;
		std::string m_weight_str;
		double m_RMSEThreshold;
		size_t m_maxBreaks;
		bool m_bYear;
		int m_firstYear;
		bool m_bExportBrks;
		bool m_bExportAll;
		double m_dNBRThreshold;
		//CTPeriod m_outputPeriod;
	};


	class CDisturbanceAnalyserII
	{
	public:

		ERMsg Execute();

		enum TModel { MODEL_3BRK, MODEL_2BRK, NB_MODELS };
		typedef std::array<CSee5Tree&, NB_MODELS> CSee5Trees;
		std::string GetDescription() { return  std::string("DisturbanceAnalyserII version ") + VERSION + " (" + __DATE__ + ")"; }

		ERMsg OpenAll(CLandsatDataset& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS, CGDALDatasetEx& breaksDS);

		void ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder);
		void ProcessBlock(int xBlock, int yBlock, CBandsHolder& bandHolder, CSee5Tree& DT123, CSee5Tree& DT12, OutputData& outputData, BreakData& breaksData);
		void WriteBlock(int xBlock, int yBlock, CGDALDatasetEx& outputDS, CGDALDatasetEx& breaksDS, OutputData& outputData, BreakData& breaksData);
		void CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS, CGDALDatasetEx& breaksDS);
		ERMsg LoadModel(std::string filePath, CSee5TreeMT& DT);

		CDisturbanceAnalyserIIOption m_options;

		static const char* VERSION;
		static const int NB_THREAD_PROCESS;
	};
}