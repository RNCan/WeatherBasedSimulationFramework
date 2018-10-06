#pragma once


#include "Geomatic/GDALBasic.h"
#include "Geomatic/LandsatDataset.h"
#include "boost/dynamic_bitset.hpp"
#include "RangerLib/RangerLib.h"

//
namespace WBSF
{
	enum TSuspicious { CLOUDS, SHADOW, NB_SUSPICIOUS };
	typedef std::vector<CLandsatPixel>CLandsatPixelVector;
	typedef std::deque< std::vector<__int16>> DebugData;
	typedef std::deque< std::vector<__int16>> RFCodeData;
	typedef std::vector< CLandsatPixelVector > LansatData;
	typedef std::auto_ptr<Forest> ForestPtr;
	typedef std::array<ForestPtr, 3> Forests3;
	typedef std::vector < Forests3> Forests3MT;
	typedef std::vector < std::array<boost::dynamic_bitset<size_t>, NB_SUSPICIOUS>> SuspectBitset;
	typedef SuspectBitset CloudBitset;
	


	class CCloudCleanerOption : public CBaseOptions
	{
	public:

		
		enum TTrigger { T_PRIMARY, T_SECONDARY, NB_TRIGGER_TYPE };
		enum TFilePath { RF_MODEL_FILE_PATH, LANDSAT_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };

		enum TDebug { D_DEBUG_FLAG, D_NB_SCENE, D_SCENE_USED, D_MODEL, D_DELTA_B1, D_DELTA_TCB/*, D_DELTA_B1_REF, D_DELTA_TCB_REF*/, NB_DBUG };
		static const char* DEBUG_NAME[NB_DBUG];

		CCloudCleanerOption();

		virtual ERMsg ParseOption(int argc, char* argv[]);
		virtual ERMsg ProcessOption(int& i, int argc, char* argv[]);


		bool IsB1Trigged(std::array <CLandsatPixel, 4>& p, size_t t, size_t fm)
		{
			size_t c0 = (fm == 0) ? 1 : 0;
			size_t c2 = (fm == 2) ? 1 : 2;

			if (!p[fm].IsInit())
				return false;

			if (!p[c0].IsInit() && !p[c2].IsInit() && !p[3].IsInit())
				return false;

			bool t1 = p[c0].IsInit() ? ((__int32)p[c0][Landsat::B1] - p[fm][Landsat::B1] < m_B1threshold[t]) : true;
			bool t2 = p[c2].IsInit() ? ((__int32)p[c2][Landsat::B1] - p[fm][Landsat::B1] < m_B1threshold[t]) : true;
			bool t3 = p[3].IsInit() ? ((__int32)p[3][Landsat::B1] - p[fm][Landsat::B1] < m_B1threshold[t]) : true;

			bool t4 = false;
			bool t5 = false;

			if (p[3].IsInit())
			{
				t4 = p[c0].IsInit() ? ((__int32)p[c0][Landsat::B1] - p[3][Landsat::B1] < m_B1threshold[t]) : true;
				t5 = p[c2].IsInit() ? ((__int32)p[c2][Landsat::B1] - p[3][Landsat::B1] < m_B1threshold[t]) : true;
			}

			bool t6 = (t1 && t2) != (t4 && t5);
			return t6/* || t3*/;


			//return (t1 && t2) || t3;
		}




		bool IsTCBTrigged(std::array <CLandsatPixel, 4>& p, size_t t, size_t fm)
		{
			size_t c0 = (fm == 0) ? 1 : 0;
			size_t c2 = (fm == 2) ? 1 : 2;

			if (!p[fm].IsInit())
				return false;

			if (!p[c0].IsInit() && !p[c2].IsInit() && !p[3].IsInit())
				return false;

			bool t1 = p[c0].IsInit() ? ((__int32)p[c0][Landsat::I_TCB] - p[fm][Landsat::I_TCB] > m_TCBthreshold[t]) : true;
			bool t2 = p[c2].IsInit() ? ((__int32)p[c2][Landsat::I_TCB] - p[fm][Landsat::I_TCB] > m_TCBthreshold[t]) : true;
			bool t3 = p[3].IsInit() ? ((__int32)p[3][Landsat::I_TCB] - p[fm][Landsat::I_TCB] > m_TCBthreshold[t]) : true;
			//bool t4 = (p[3].IsInit() && t == T_PRIMARY )? ((__int32)p[fm][Landsat::I_TCB] < m_TCBthreshold[2]) : false;

			bool t4 = false;
			bool t5 = false;

			if (p[3].IsInit())
			{
				t4 = p[c0].IsInit() ? ((__int32)p[c0][Landsat::I_TCB] - p[3][Landsat::I_TCB] > m_TCBthreshold[t]) : true;
				t5 = p[c2].IsInit() ? ((__int32)p[c2][Landsat::I_TCB] - p[3][Landsat::I_TCB] > m_TCBthreshold[t]) : true;
			}

			bool t6 = (t1 && t2) != (t4 && t5);
			return t6 /*|| t3*/;


			//return (t1 && t2) ||t3;
		}


		bool DoubleCloud(std::array <CLandsatPixel, 4>& p, size_t t, size_t fm)
		{
			return false;


			size_t c0 = (fm == 0) ? 1 : 0;
			size_t c2 = (fm == 2) ? 1 : 2;

			if (!p[3].IsInit())
				return false;

			if (!p[c0].IsInit() || !p[c2].IsInit())
				return false;

			bool t1 = p[c0].IsInit() ? ((__int32)p[3][Landsat::B1] - p[c0][Landsat::B1] < m_B1threshold[t]) : true;
			bool t2 = p[c2].IsInit() ? ((__int32)p[3][Landsat::B1] - p[c2][Landsat::B1] < m_B1threshold[t]) : true;

			bool t3 = p[c0].IsInit() ? ((__int32)p[3][Landsat::I_TCB] - p[c0][Landsat::I_TCB] > m_TCBthreshold[t]) : true;
			bool t4 = p[c2].IsInit() ? ((__int32)p[3][Landsat::I_TCB] - p[c2][Landsat::I_TCB] > m_TCBthreshold[t]) : true;

			return (t1 && t2) || (t3 && t4);
		}

		bool IsB1TriggedRef(const CLandsatPixel& p, std::vector <CLandsatPixel>& r, size_t t = T_PRIMARY)
		{
			if (!p.IsInit())
				return false;

			bool t3 = false;
			for (size_t i = 0; i < r.size(); i++)
				t3 |= r[i].IsInit() ? ((__int32)r[i][Landsat::B1] - p[Landsat::B1] < m_B1threshold[t]) : true;

			return t3;
		}

		bool IsTCBTriggedRef(const CLandsatPixel& p, std::vector <CLandsatPixel>& r, size_t t = T_PRIMARY)
		{
			if (!p.IsInit())
				return false;

			bool t3 = false;
			for (size_t i = 0; i < r.size(); i++)
				t3 |= r[i].IsInit() ? ((__int32)r[i][Landsat::I_TCB] - p[Landsat::I_TCB] > m_TCBthreshold[t]) : true;

			return t3;
		}
		
		__int32 GetB1Trigger(std::array <CLandsatPixel, 4>& p, size_t fm);
		__int32 GetTCBTrigger(std::array <CLandsatPixel, 4>& p, size_t fm);
		//__int32 GetB1TriggerRef(const CLandsatPixel& p, std::vector <CLandsatPixel>& r);
		//__int32 GetTCBTriggerRef(const CLandsatPixel& p, std::vector <CLandsatPixel>& r);

		int GetDebugFlag(std::array <CLandsatPixel, 4>& p, size_t t = T_PRIMARY, size_t fm = 1)
		{
			size_t c0 = (fm == 0) ? 1 : 0;
			size_t c2 = (fm == 2) ? 1 : 2;

			bool bB1 = IsB1Trigged(p, t, fm);
			bool bTCB = IsTCBTrigged(p, t, fm);
			int nB1 = (bB1) ? 1 : 0;
			int nTCB = (bTCB) ? 2 : 0;

			int nt = (t == T_SECONDARY && (nB1 + nTCB) > 0) ? 4 : 0;

			return nt + nB1 + nTCB;
		}


		std::array<__int32, NB_TRIGGER_TYPE> m_B1threshold;
		std::array<__int32, NB_TRIGGER_TYPE> m_TCBthreshold;
		
		size_t m_Dmax;
		size_t m_buffer;
		std::array < size_t, 2> m_bufferEx;
		size_t m_sieve;

		bool m_bDebug;
		bool m_bOutputCode;
		//bool m_bOutputJD;
		bool m_bFillClouds;
		bool m_bFillMissing;
		bool m_bUseMedian;
		bool m_bVerifyDoubleCloud;
		CTPeriod m_periodTreated;
		std::string m_rename;

		std::array<size_t, 2> m_scenesTreated;
		std::array<size_t, 2> m_scenesLoaded;
		std::string m_medianFilePath;


		__int64 m_nbPixelDT;
		__int64 m_nbPixel;

	};


	class CCloudCleaner
	{
	public:

		static size_t GetPrevious(const CLandsatPixelVector& landsat, size_t z);
		static size_t GetNext(const CLandsatPixelVector& landsat, size_t z);
		static size_t get_m(size_t z1, const CLandsatPixelVector& data);
		static std::array <CLandsatPixel, 4> GetP(size_t z1, CLandsatPixelVector& data, const CLandsatPixel& median);
		static std::vector <CLandsatPixel> GetR(CLandsatWindow& windowRef, size_t x, size_t y);


		std::string GetDescription();
		ERMsg Execute();


		ERMsg OpenAll(CLandsatDataset& lansatDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& medianDS, CLandsatDataset& outputDS, CGDALDatasetEx& DTCodeDS, CGDALDatasetEx& debugDS/*, CGDALDatasetEx& JDDS*/);
		void ReadBlock(size_t xBlock, size_t yBlock, CBandsHolder& bandHolder);
		void ReadBlockM(size_t xBlock, size_t yBlock, CBandsHolder& bandHolderM);
		void FindSuspicious(size_t xBlock, size_t yBlock, const CBandsHolder& bandHolder, const CBandsHolder& bandHolderM, SuspectBitset& suspects1, SuspectBitset& suspects2);
		void FindClouds(size_t xBlock, size_t yBlock, const CBandsHolder& bandHolder, const CBandsHolder& bandHolderM, const Forests3& forest, RFCodeData& DTCode, SuspectBitset& suspects1, SuspectBitset& suspects2, CloudBitset& clouds);
		void WriteBlock1(size_t xBlock, size_t yBlock, RFCodeData& DTCode, CGDALDatasetEx& DTCodeDS);
		void ResetReplaceClouds(size_t xBlock, size_t yBlock, const CBandsHolder& bandHolder, const CBandsHolder& bandHolderM, LansatData& data, DebugData& debug, SuspectBitset& suspects1, SuspectBitset& suspects2, CloudBitset& clouds);
		void WriteBlock2(size_t xBlock, size_t yBlock, const LansatData& data, DebugData& debug, CGDALDatasetEx& outputDS, CGDALDatasetEx& debugDS/*, CGDALDatasetEx& JDDS*/);
		void CloseAll(CGDALDatasetEx& landsatDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& outputDS, CGDALDatasetEx& DTCodeDS, CGDALDatasetEx& debugDS);

		void LoadData(const CBandsHolder& bandHolder, const CBandsHolder& bandHolderM, LansatData& data, CLandsatPixelVector& median);

		
		bool SieveSuspect1(size_t level, size_t nbSieve, const CGeoExtents& extents, CGeoPointIndex xy, boost::dynamic_bitset<size_t>& suspects1, boost::dynamic_bitset<size_t>& treated, std::vector<size_t>& pixels);
		//void SieveSuspect1(size_t level, size_t nbSieve, const CGeoExtents& extents, CGeoPointIndex xy, boost::dynamic_bitset<size_t>& suspects1, boost::dynamic_bitset<size_t>& treated, size_t& nbPixels);
		void SieveSuspect1(size_t nbSieve, const CGeoExtents& extents, boost::dynamic_bitset<size_t>& suspects1);
		//void FastSieveSuspect1(size_t nbSieve, const CGeoExtents& extents, boost::dynamic_bitset<size_t>& suspects1);

		static void TouchSuspect1(size_t level, const CGeoExtents& extents, CGeoPointIndex xy, const boost::dynamic_bitset<size_t>& suspects1, const boost::dynamic_bitset<size_t>& suspects2, boost::dynamic_bitset<size_t>& treated, boost::dynamic_bitset<size_t>& touch);
		void CleanSuspect2(const CGeoExtents& extents, const boost::dynamic_bitset<size_t>& suspects1, boost::dynamic_bitset<size_t>& suspects2);
		
		void SetSuspiciousBuffer(const CGeoExtents& extents, boost::dynamic_bitset<size_t>& suspects1, boost::dynamic_bitset<size_t>& suspects2);
		void SetCloudBuffer(const CGeoExtents& extents, SuspectBitset& suspects1, SuspectBitset& suspects2, CloudBitset& clouds);

		CCloudCleanerOption m_options;

		static ERMsg ReadModel(std::string filePath, int CPU, ForestPtr& forest, bool bQuit);
		ERMsg ReadModel(Forests3MT& forest);
		static std::string GetScenesDateFormat(const std::vector<CTPeriod>& p);
	};

}