#pragma once


#include "Geomatic/GDALBasic.h"
//#include "Geomatic/See5hooks.h"
#include "Geomatic/LandsatDataset.h"
#include "boost/dynamic_bitset.hpp"
#include "RangerLib/RangerLib.h"

//
namespace WBSF
{
	typedef std::vector<CLandsatPixel>CLandsatPixelVector;
	typedef std::deque< std::vector<__int16>> DebugData;
	typedef std::deque< std::vector<__int16>> RFCodeData;
	typedef std::vector< CLandsatPixelVector > LansatData;
	typedef std::auto_ptr<Forest> ForestPtr;
	typedef std::vector<boost::dynamic_bitset<size_t>> CloudBitset;

	class CCloudCleanerOption : public CBaseOptions
	{
	public:

		enum TTrigger { T_PRIMARY, T_SECONDARY, NB_TRIGGER_TYPE };
		//enum TFilePath { RF_BEG_FILE_PATH, RF_MID_FILE_PATH, RF_END_FILE_PATH, LANDSAT_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };
		enum TFilePath { RF_MODEL_FILE_PATH, LANDSAT_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };

		enum TDebug{ D_DEBUG_ID, D_DEBUG_B1, D_DEBUG_TCB, D_DEBUG_ZSW, D_NB_SCENE, D_SCENE_USED, NB_DBUG };
		static const char* DEBUG_NAME[NB_DBUG];

		CCloudCleanerOption();
		
		virtual ERMsg ParseOption(int argc, char* argv[]);
		virtual ERMsg ProcessOption(int& i, int argc, char* argv[]);
		
		bool IsTrigged(std::array <CLandsatPixel, 3>& p, size_t t = T_PRIMARY)
		{
			if (!p[0].IsInit() && !p[2].IsInit())
				return false;
			

			bool bB1 = IsB1Trigged(p,t);
			bool bTCB = IsTCBTrigged(p,t);
			bool bZSW = IsZSWTrigged(p,t);
			
			return bB1 || bTCB || bZSW;
		}

		bool IsB1Trigged(std::array <CLandsatPixel, 3>& p, size_t t = T_PRIMARY)
		{
			if (!p[0].IsInit() && !p[2].IsInit())
				return false;

			bool t1 = p[0].IsInit() ? ((__int32)p[0][Landsat::B1] - p[1][Landsat::B1] < m_B1threshold[t]) : true;
			bool t2 = p[2].IsInit() ? ((__int32)p[2][Landsat::B1] - p[1][Landsat::B1] < m_B1threshold[t]) : true;

			return (t1&&t2);
		}

		bool IsTCBTrigged(std::array <CLandsatPixel, 3>& p, size_t t = T_PRIMARY)
		{
			if (!p[0].IsInit() && !p[2].IsInit())
				return false;

			bool t3 = p[0].IsInit() ? ((__int32)p[0][Landsat::I_TCB] - p[1][Landsat::I_TCB] > m_TCBthreshold[t]) : true;
			bool t4 = p[2].IsInit() ? ((__int32)p[2][Landsat::I_TCB] - p[1][Landsat::I_TCB] > m_TCBthreshold[t]) : true;

			return (t3&&t4);
		}
		bool IsZSWTrigged(std::array <CLandsatPixel, 3>& p, size_t t = T_PRIMARY)
		{
			if (!p[0].IsInit() && !p[2].IsInit())
				return false;

			bool t5 = p[0].IsInit() ? ((__int32)p[0][Landsat::I_ZSW] - p[1][Landsat::I_ZSW] > m_ZSWthreshold[t]) : true;
			bool t6 = p[2].IsInit() ? ((__int32)p[2][Landsat::I_ZSW] - p[1][Landsat::I_ZSW] > m_ZSWthreshold[t]) : true;

			return (t5&&t6);
		}


		__int32 GetTrigger(std::array <CLandsatPixel, 3>& p, size_t t = T_PRIMARY)
		{
			return GetB1Trigger(p, t) + GetTCBTrigger(p, t) + GetZSWTrigger(p, t);
		}
		__int32 GetB1Trigger(std::array <CLandsatPixel, 3>& p, size_t t = T_PRIMARY);
		__int32 GetTCBTrigger(std::array <CLandsatPixel, 3>& p, size_t t = T_PRIMARY);
		__int32 GetZSWTrigger(std::array <CLandsatPixel, 3>& p, size_t t = T_PRIMARY);

		int GetDebugID(std::array <CLandsatPixel, 3>& p, size_t t = T_PRIMARY)
		{
			//int nbImages = (p[0].IsInit() ? 1 : 0) + (p[1].IsInit() ? 1 : 0) + (p[2].IsInit() ? 1 : 0);

			int nB1 = IsB1Trigged(p, t) ? 1 : 0;
			int nTCB = IsTCBTrigged(p, t) ? 2 : 0;
			int nZSW = IsZSWTrigged(p, t) ? 4 : 0;
			int nt = (t == T_SECONDARY && (nB1 + nTCB + nZSW)>0) ? 8 : 0;

			return nt + nB1 + nTCB + nZSW;
		}
	
		
		

		
		std::array<__int32, 2> m_B1threshold;
		std::array<__int32, 2> m_TCBthreshold;
		std::array<__int32, 2> m_ZSWthreshold;
		size_t m_buffer;

		bool m_bDebug;
		bool m_bOutputDT;
		bool m_bFillCloud;
		//size_t m_maxScene;
		std::array<size_t, 2> m_scenes;
		size_t m_doubleTrigger;
		
		//bool m_bNoTrigger;

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
		void Preprocess(int xBlock, int yBlock, const CBandsHolder& bandHolder, const ForestPtr& forest, CloudBitset& suspects1, CloudBitset& suspects2);
		void ProcessBlock1(int xBlock, int yBlock, const CBandsHolder& bandHolder, const ForestPtr& forest, RFCodeData& DTCode, CloudBitset& suspects1, CloudBitset& suspects2, CloudBitset& clouds);
		void WriteBlock1(int xBlock, int yBlock, const CBandsHolder& bandHolder, RFCodeData& DTCode, CGDALDatasetEx& DTCodeDS);
		void ProcessBlock2(int xBlock, int yBlock, const CBandsHolder& bandHolder, LansatData& data, DebugData& debug, CloudBitset& suspects1, CloudBitset& suspects2, CloudBitset& clouds);
		void WriteBlock2(int xBlock, int yBlock, const CBandsHolder& bandHolder, const LansatData& data, DebugData& debug, CGDALDatasetEx& outputDS, CGDALDatasetEx& debugDS);
		void CloseAll(CGDALDatasetEx& landsatDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS, CGDALDatasetEx& DTCodeDS, CGDALDatasetEx& debugDS);

		static bool TouchSuspect1(size_t level, const CGeoExtents& extents, CGeoPointIndex xy, const boost::dynamic_bitset<size_t>& suspects1, boost::dynamic_bitset<size_t>& suspects2, boost::dynamic_bitset<size_t>& treated);
		static void CleanSuspect2(const CGeoExtents& extents, const boost::dynamic_bitset<size_t>& suspects1, boost::dynamic_bitset<size_t>& suspects2);
		void LoadData(const CBandsHolder& bandHolder, LansatData& data);
		//ERMsg ReadRules(CDecisionTree& DT);

		CCloudCleanerOption m_options;
		
		//static void LoadModel(CDecisionTreeBaseEx& DT, std::string filePath);
		static ERMsg ReadModel(std::string filePath, int CPU, ForestPtr& forest);
		ERMsg ReadModel(ForestPtr& forest);
		
		//static CDecisionTreeBlock GetDataRecord(std::array<CLandsatPixel, 3> p, CDecisionTreeBaseEx& DT);
	};

}