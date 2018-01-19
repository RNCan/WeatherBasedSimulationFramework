#pragma once


#include "Geomatic/GDALBasic.h"
#include "Geomatic/See5hooks.h"
#include "Geomatic/LandsatDataset.h"
#include "boost/dynamic_bitset.hpp"
//
namespace WBSF
{
	typedef std::vector<CLandsatPixel>CLandsatPixelVector;
	typedef std::deque< std::vector<__int16>> DebugData;
	typedef std::vector<__int16> DTCodeData;
	typedef std::vector< CLandsatPixelVector > LansatData;

	class CCloudCleanerOption : public CBaseOptions
	{
	public:

		enum TFilePath { DT_FILE_PATH, LANDSAT_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };
		enum TDebug{ D_DEBUG_ID, D_DEBUG_B1, D_DEBUG_TCB, D_DEBUG_ZSW, D_NB_SCENE, D_SCENE_USED, NB_DBUG };
		static const char* DEBUG_NAME[NB_DBUG];

		CCloudCleanerOption();
		
		virtual ERMsg ParseOption(int argc, char* argv[]);
		virtual ERMsg ProcessOption(int& i, int argc, char* argv[]);
		
		bool IsTrigged(std::array <CLandsatPixel, 3>& p)
		{
			if (!p[0].IsInit() && !p[2].IsInit())
				return false;


			bool t1 = p[0].IsInit() ? (p[0][Landsat::B1] - p[1][Landsat::B1] < m_B1threshold) : true;
			bool t2 = p[2].IsInit() ? (p[2][Landsat::B1] - p[1][Landsat::B1] < m_B1threshold) : true;
			bool t3 = p[0].IsInit() ? (p[0][Landsat::I_TCB] - p[1][Landsat::I_TCB] > m_TCBthreshold) : true;
			bool t4 = p[2].IsInit() ? (p[2][Landsat::I_TCB] - p[1][Landsat::I_TCB] > m_TCBthreshold) : true;
			bool t5 = p[0].IsInit() ? (p[0][Landsat::I_ZSW] - p[1][Landsat::I_ZSW] > m_ZSWthreshold) : true;
			bool t6 = p[2].IsInit() ? (p[2][Landsat::I_ZSW] - p[1][Landsat::I_ZSW] > m_ZSWthreshold) : true;
			
			return (t1&&t2)||((t3&&t4)||(t5&&t6));
		}

		bool IsB1Trigged(std::array <CLandsatPixel, 3>& p)
		{
			if (!p[0].IsInit() && !p[2].IsInit())
				return false;

			bool t1 = p[0].IsInit() ? (p[0][Landsat::B1] - p[1][Landsat::B1] < m_B1threshold) : true;
			bool t2 = p[2].IsInit() ? (p[2][Landsat::B1] - p[1][Landsat::B1] < m_B1threshold) : true;

			return (t1&&t2);
		}

		bool IsTCBTrigged(std::array <CLandsatPixel, 3>& p)
		{
			if (!p[0].IsInit() && !p[2].IsInit())
				return false;

			bool t3 = p[0].IsInit() ? (p[0][Landsat::I_TCB] - p[1][Landsat::I_TCB] > m_TCBthreshold) : true;
			bool t4 = p[2].IsInit() ? (p[2][Landsat::I_TCB] - p[1][Landsat::I_TCB] > m_TCBthreshold) : true;

			return (t3&&t4);
		}
		bool IsZSWTrigged(std::array <CLandsatPixel, 3>& p)
		{
			if (!p[0].IsInit() && !p[2].IsInit())
				return false;

			bool t5 = p[0].IsInit() ? (p[0][Landsat::I_ZSW] - p[1][Landsat::I_ZSW] > m_ZSWthreshold) : true;
			bool t6 = p[2].IsInit() ? (p[2][Landsat::I_ZSW] - p[1][Landsat::I_ZSW] > m_ZSWthreshold) : true;

			return (t5&&t6);
		}


		int GetDebugID(std::array <CLandsatPixel, 3>& p)
		{
			int t1 = p[0].IsInit() ? (p[0][Landsat::B1] - p[1][Landsat::B1] < m_B1threshold) ? 1 : 0 : 0;
			int t2 = p[2].IsInit() ? (p[2][Landsat::B1] - p[1][Landsat::B1] < m_B1threshold) ? 1 : 0 : 0;
			//int t3 = p[0].IsInit() && !p[2].IsInit() ? (p[0][Landsat::B1] - p[1][Landsat::B1] < m_B1threshold) ? 10 : 0 : 0;
			//t t4 = p[2].IsInit() && !p[0].IsInit() ? (p[2][Landsat::B1] - p[1][Landsat::B1] < m_B1threshold) ? 10 : 0 : 0;
			int t5 = p[0].IsInit() && p[2].IsInit() ? (t1+t2)>0 ? 10 : 0 : 0;

			int t6 = p[0].IsInit() ? (p[0][Landsat::I_TCB] - p[1][Landsat::I_TCB] > m_TCBthreshold) ? 1 : 0 : 0;
			int t7 = p[2].IsInit() ? (p[2][Landsat::I_TCB] - p[1][Landsat::I_TCB] > m_TCBthreshold) ? 1 : 0 : 0;
			int t8 = p[0].IsInit() ? (p[0][Landsat::I_TCB] - p[1][Landsat::I_TCB] > m_ZSWthreshold) ? 1 : 0 : 0;
			int t9 = p[2].IsInit() ? (p[2][Landsat::I_TCB] - p[1][Landsat::I_TCB] > m_ZSWthreshold) ? 1 : 0 : 0;
			
			int t10 = p[0].IsInit() && p[2].IsInit() ? (t6 + t7 + t8 + t9)>0 ? 110 : 0 : 0;

			return t1 + t2 + t5 + t6 + t7 + t8 + t9 + t10;
		}
	
		
		

		double m_B1threshold;
		double m_TCBthreshold;
		double m_ZSWthreshold;
		bool m_bDebug;
		bool m_bOutputDT;
		bool m_bFillCloud;
		size_t m_maxScene;
		size_t m_scene;
		size_t m_buffer;

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
		void ProcessBlock1(int xBlock, int yBlock, const CBandsHolder& bandHolder, CDecisionTree& DT, DTCodeData& DTCode, boost::dynamic_bitset<size_t>& clouds);
		void WriteBlock1(int xBlock, int yBlock, const CBandsHolder& bandHolder, DTCodeData& DTCode, CGDALDatasetEx& DTCodeDS);
		void ProcessBlock2(int xBlock, int yBlock, const CBandsHolder& bandHolder, LansatData& data, DebugData& debug, const boost::dynamic_bitset<size_t>& clouds);
		void WriteBlock2(int xBlock, int yBlock, const CBandsHolder& bandHolder, const LansatData& data, DebugData& debug, CGDALDatasetEx& outputDS, CGDALDatasetEx& debugDS);
		void CloseAll(CGDALDatasetEx& landsatDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS, CGDALDatasetEx& DTCodeDS, CGDALDatasetEx& debugDS);

		void LoadData(const CBandsHolder& bandHolder, LansatData& data);
		ERMsg ReadRules(CDecisionTree& DT);

		CCloudCleanerOption m_options;
		
		static void LoadModel(CDecisionTreeBaseEx& DT, std::string filePath);
		
		static CDecisionTreeBlock GetDataRecord(std::array<CLandsatPixel, 3> p, CDecisionTreeBaseEx& DT);
	};

}