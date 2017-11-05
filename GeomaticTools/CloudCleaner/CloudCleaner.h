#pragma once


#include "Geomatic/GDALBasic.h"
#include "Geomatic/See5hooks.h"
#include "Geomatic/LandsatDataset.h"
//
namespace WBSF
{
	typedef std::vector<CLandsatPixel>CLandsatPixelVector;
	typedef std::deque< std::vector<short>> DebugData;
	typedef std::deque< std::vector<__int8>> DTCodeData;
	typedef std::vector< CLandsatPixelVector > LansatData;

	class CCloudCleanerOption : public CBaseOptions
	{
	public:

		enum TFilePath { DT_FILE_PATH, LANDSAT_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };
		enum TDebug{ D_B1_T1, D_B1_T3, D_TCB_T1, D_TCB_T3, NB_DBUG };
		static const char* DEBUG_NAME[NB_DBUG];

		CCloudCleanerOption();
		
		virtual ERMsg ParseOption(int argc, char* argv[]);
		virtual ERMsg ProcessOption(int& i, int argc, char* argv[]);
		
		bool IsTrigged(std::array <CLandsatPixel, 3>& p)
		{
			bool t1 = p[0].IsInit() ? (p[0][Landsat::B1] - p[1][Landsat::B1] < m_B1threshold) : true;
			bool t2 = p[2].IsInit() ? (p[2][Landsat::B1] - p[1][Landsat::B1] < m_B1threshold) : true;
			bool t3 = p[0].IsInit() ? (p[0][Landsat::I_TCB] - p[1][Landsat::I_TCB] > m_TCBthreshold) : true;
			bool t4 = p[2].IsInit() ? (p[2][Landsat::I_TCB] - p[1][Landsat::I_TCB] > m_TCBthreshold) : true;
			
			return (t1&&t2)||(t3&&t4);
		}
	
		double m_B1threshold;
		double m_TCBthreshold;
		bool m_bDebug;
		bool m_bOutputDT;

		__int64 m_nbPixelDT;
		__int64 m_nbPixel;
	};

	
	class CCloudCleaner
	{
	public:

		std::string GetDescription();
		ERMsg Execute();


		ERMsg OpenAll(CLandsatDataset& lansatDS, CGDALDatasetEx& maskDS, CLandsatDataset& outputDS, CGDALDatasetEx& DTCodeDS, CGDALDatasetEx& debugDS);
		void ReadBlock(int xBlock, int yBlock, CBandsHolder& bandHolder);
		void ProcessBlock(int xBlock, int yBlock, const CBandsHolder& bandHolder, CDecisionTree& DT, LansatData& data, DTCodeData& DTCode, DebugData& debug);
		void WriteBlock(int xBlock, int yBlock, const CBandsHolder& bandHolder, const LansatData& data, DTCodeData& DTCode, DebugData& debug, CGDALDatasetEx& outputDS, CGDALDatasetEx& DTCodeDS, CGDALDatasetEx& debugDS);
		void CloseAll(CGDALDatasetEx& landsatDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS, CGDALDatasetEx& DTCodeDS, CGDALDatasetEx& debugDS);

		void LoadData(const CBandsHolder& bandHolder, LansatData& data);
		ERMsg ReadRules(CDecisionTree& DT);

		CCloudCleanerOption m_options;
		
		static void LoadModel(CDecisionTreeBaseEx& DT, std::string filePath);
		
		static CDecisionTreeBlock GetDataRecord(std::array<CLandsatPixel, 3> p, CDecisionTreeBaseEx& DT);
	};

}