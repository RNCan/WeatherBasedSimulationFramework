﻿#pragma once


#include "Geomatic/GDALBasic.h"
#include "Geomatic/LandsatDataset.h"
#include "boost/dynamic_bitset.hpp"
#include "RangerLib/RangerLib.h"

//
namespace WBSF
{
	typedef std::vector<CLandsatPixel>CLandsatPixelVector;
	typedef std::deque< std::vector<__int16>> DebugData;
	typedef std::vector<__int16> RFCodeData;
	typedef std::vector< CLandsatPixelVector > LansatData;
	typedef std::auto_ptr<Forest> ForestPtr;
	typedef std::array<ForestPtr, 3> Forests3;
	typedef boost::dynamic_bitset<size_t> CloudBitset;

	class CCloudCleanerIOption : public CBaseOptions
	{
	public:

		enum TTrigger { T_PRIMARY, T_SECONDARY, NB_TRIGGER_TYPE };
		enum TFilePath { RF_MODEL_FILE_PATH, REF_FILE_PATH, UPDATE_FILE_PATH, NB_FILE_PATH };

		enum TDebug { D_DEBUG_ID, D_DEBUG_B1, D_DEBUG_TCB, D_DEBUG_ZSW, D_NB_SCENE, D_SCENE_USED, NB_DBUG };
		static const char* DEBUG_NAME[NB_DBUG];

		CCloudCleanerIOption();

		virtual ERMsg ParseOption(int argc, char* argv[]);
		virtual ERMsg ProcessOption(int& i, int argc, char* argv[]);

		bool IsTrigged(std::array <CLandsatPixel, 3>& p, size_t t = T_PRIMARY, size_t fm = 1)
		{
			size_t c0 = (fm == 0) ? 1 : 0;
			size_t c2 = (fm == 2) ? 1 : 2;

			if (!p[c0].IsInit() && !p[c2].IsInit())
				return false;


			bool bB1 = IsB1Trigged(p, t, fm);
			bool bTCB = IsTCBTrigged(p, t, fm);
			bool bZSW = IsZSWTrigged(p, t, fm);

			return bB1 || bTCB || bZSW;
		}

		bool IsB1Trigged(std::array <CLandsatPixel, 3>& p, size_t t = T_PRIMARY, size_t fm = 1)
		{
			size_t c0 = (fm == 0) ? 1 : 0;
			size_t c2 = (fm == 2) ? 1 : 2;

			if (!p[c0].IsInit() && !p[c2].IsInit())
				return false;

			bool t1 = p[c0].IsInit() ? ((__int32)p[c0][Landsat::B1] - p[fm][Landsat::B1] < m_B1threshold[t]) : true;
			bool t2 = p[c2].IsInit() ? ((__int32)p[c2][Landsat::B1] - p[fm][Landsat::B1] < m_B1threshold[t]) : true;

			return (t1&&t2);
		}
		
		bool DoubleCloud(std::array <CLandsatPixel, 3>& p, size_t t = T_PRIMARY, size_t fm = 1)
		{
			size_t c0 = (fm == 0) ? 1 : 0;
			size_t c2 = (fm == 2) ? 1 : 2;
			
			if (!p[c0].IsInit() && !p[c2].IsInit())
				return false;

			bool t1 = p[c0].IsInit() ? ((__int32)p[fm][Landsat::B1] - p[c0][Landsat::B1] < m_B1threshold[t]) : true;
			bool t2 = p[c2].IsInit() ? ((__int32)p[fm][Landsat::B1] - p[c2][Landsat::B1] < m_B1threshold[t]) : true;

			return t1 && t2;
		}

		bool IsTCBTrigged(std::array <CLandsatPixel, 3>& p, size_t t = T_PRIMARY, size_t fm = 1)
		{
			size_t c0 = (fm == 0) ? 1 : 0;
			size_t c2 = (fm == 2) ? 1 : 2;

			if (!p[c0].IsInit() && !p[c2].IsInit())
				return false;

			if (DoubleCloud(p, t, fm))
				return false;

			bool t3 = p[c0].IsInit() ? ((__int32)p[c0][Landsat::I_TCB] - p[fm][Landsat::I_TCB] > m_TCBthreshold[t]) : true;
			bool t4 = p[c2].IsInit() ? ((__int32)p[c2][Landsat::I_TCB] - p[fm][Landsat::I_TCB] > m_TCBthreshold[t]) : true;

			return (t3&&t4);
		}
		bool IsZSWTrigged(std::array <CLandsatPixel, 3>& p, size_t t = T_PRIMARY, size_t fm = 1)
		{
			size_t c0 = (fm == 0) ? 1 : 0;
			size_t c2 = (fm == 2) ? 1 : 2;

			if (!p[c0].IsInit() && !p[c2].IsInit())
				return false;

			if (DoubleCloud(p, t, fm))
				return false;

			bool t5 = p[c0].IsInit() ? ((__int32)p[c0][Landsat::I_ZSW] - p[fm][Landsat::I_ZSW] > m_ZSWthreshold[t]) : true;
			bool t6 = p[c2].IsInit() ? ((__int32)p[c2][Landsat::I_ZSW] - p[fm][Landsat::I_ZSW] > m_ZSWthreshold[t]) : true;

			return (t5&&t6);
		}


		__int32 GetTrigger(std::array <CLandsatPixel, 3>& p, size_t t = T_PRIMARY, size_t fm = 1)
		{
			return GetB1Trigger(p, t, fm) + GetTCBTrigger(p, t, fm) + GetZSWTrigger(p, t, fm);
		}

		__int32 GetB1Trigger(std::array <CLandsatPixel, 3>& p, size_t t = T_PRIMARY, size_t fm = 1);
		__int32 GetTCBTrigger(std::array <CLandsatPixel, 3>& p, size_t t = T_PRIMARY, size_t fm = 1);
		__int32 GetZSWTrigger(std::array <CLandsatPixel, 3>& p, size_t t = T_PRIMARY, size_t fm = 1);

		int GetDebugID(std::array <CLandsatPixel, 3>& p, size_t t = T_PRIMARY, size_t fm = 1)
		{
			//int nbImages = (p[0].IsInit() ? 1 : 0) + (p[1].IsInit() ? 1 : 0) + (p[2].IsInit() ? 1 : 0);

			int nB1 = IsB1Trigged(p, t, fm) ? 1 : 0;
			int nTCB = IsTCBTrigged(p, t, fm) ? 2 : 0;
			int nZSW = IsZSWTrigged(p, t, fm) ? 4 : 0;
			int nt = (t == T_SECONDARY && (nB1 + nTCB + nZSW) > 0) ? 8 : 0;

			return nt + nB1 + nTCB + nZSW;
		}

		std::array<__int32, 2> m_B1threshold;
		std::array<__int32, 2> m_TCBthreshold;
		std::array<__int32, 2> m_ZSWthreshold;
		size_t m_buffer;
		size_t m_bufferEx;

		bool m_bDebug;
		bool m_bOutputDT;
		bool m_bBackup;
		
		size_t m_maxScene;
		size_t m_doubleTrigger;
		//bool m_bSuspectAsCloud;
		

		__int64 m_nbPixelDT;
		__int64 m_nbPixel;
	};


	class CCloudCleanerI
	{
	public:

		std::string GetDescription();
		ERMsg Execute();


		ERMsg OpenAll(CLandsatDataset& inputDS, CLandsatDataset& refDS, CGDALDatasetEx& maskDS/*, CGDALDatasetEx& DTCodeDS, CGDALDatasetEx& debugDS*/);
		void ReadBlock(CGeoExtents extents, CTPeriod p, CBandsHolder& bandHolder1, CBandsHolder& bandHolder2);
		void GetSuspicious(const CLandsatPixelVector& data1, const LansatData& data2, const Forests3& forest, CloudBitset& suspects1, CloudBitset& suspects2);
		void GetClouds(const CLandsatPixelVector& data1, const LansatData& data2, const Forests3& forest, CloudBitset& suspects1, CloudBitset& suspects2, CloudBitset& clouds);
		void SetBuffer(const CGeoExtents& extents, CloudBitset& suspects1, CloudBitset& suspects2, CloudBitset& clouds);
		//void WriteBlock1(int xBlock, int yBlock, const CBandsHolder& bandHolder, RFCodeData& DTCode, CGDALDatasetEx& DTCodeDS);
		//void ProcessBlock2(int xBlock, int yBlock, const CBandsHolder& bandHolder1, const CBandsHolder& bandHolder2, LansatData& data, DebugData& debug, CloudBitset& suspects1, CloudBitset& suspects2, CloudBitset& clouds);
		//void WriteBlock2(int xBlock, int yBlock, const CBandsHolder& bandHolder, const LansatData& data, DebugData& debug, CGDALDatasetEx& outputDS, CGDALDatasetEx& debugDS);
		void CloseAll(CGDALDatasetEx& inputDS, CGDALDatasetEx& landsatDS, CGDALDatasetEx& maskDS);

		static bool TouchSuspect1(size_t level, const CGeoExtents& extents, CGeoPointIndex xy, const CloudBitset& suspects1, CloudBitset& suspects2, CloudBitset& treated);
		static void CleanSuspect2(const CGeoExtents& extents, const CloudBitset& suspects1, CloudBitset& suspects2);

		
		void LoadData(const CBandsHolder& bandHolder, LansatData& data);
		void LoadData(size_t z, const CBandsHolder& bandHolder, CLandsatPixelVector& data);

		CCloudCleanerIOption m_options;

		static ERMsg ReadModel(std::string filePath, int CPU, ForestPtr& forest);
		ERMsg ReadModel(Forests3& forest);
	};

}