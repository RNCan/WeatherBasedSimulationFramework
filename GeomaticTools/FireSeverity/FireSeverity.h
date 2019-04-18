#pragma once


#include "Geomatic/GDALBasic.h"
#include "Geomatic/LandsatDataset.h"
#include "boost/dynamic_bitset.hpp"
#include "RangerLib/RangerLib.h"

//
namespace WBSF
{
	
	enum TFireBitset1 {T_Z, T_JD, NB_FB_TYPE1};
	enum TFireBitset2 { T_FIRE, T_BUFFER, NB_FB_TYPE2 };
	enum TOutput {O_DNBR, O_ZSCORE1, O_ZSCORE2, O_FIRE_SEV, NB_OUTPUTS};
	enum TDebug { D_NB_MISSING, D_DNBR0, D_OFFSET, D_T1_B3, D_T1_B4, D_T1_B5, D_T1_B7, D_T3_B3, D_T3_B4, D_T3_B5, D_T3_B7, NB_DEBUGS };

	typedef std::deque< std::array< std::vector<__int16>, NB_OUTPUTS>> OutputData;
	typedef std::deque< std::array< std::vector<__int16>, NB_DEBUGS>> DebugData;
	
	typedef std::vector< CLandsatPixelVector > LansatData;
	typedef std::vector < std::map<std::pair<__int16, __int16>, CStatistic>> BufferStat;
	typedef std::vector < std::pair<std::array<size_t, NB_FB_TYPE1>, std::array<boost::dynamic_bitset<size_t>, NB_FB_TYPE2>>> FireBitset;
	

	class CFireSeverityOption : public CBaseOptions
	{
	public:

		
		//enum TMethod { M_SIMPLE, M_BUFFER, NB_METHODS };
		enum TFilePath { FIRE_FILE_PATH, LANDSAT_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };
		

		//static const char* DEBUG_NAME[NB_DBUG];

		CFireSeverityOption();

		virtual ERMsg ParseOption(int argc, char* argv[]);
		virtual ERMsg ProcessOption(int& i, int argc, char* argv[]);

		bool m_bDebug;
		size_t m_buffer;
		std::array<size_t, 2> m_scenesTreated;
		std::array<size_t, 2> m_scenesLoaded;

		CTPeriod m_periodTreated;
	};


	class CFireSeverity
	{
	public:

		std::string GetDescription();
		ERMsg Execute();

		
		ERMsg OpenAll(CLandsatDataset& lansatDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& fireDS, CGDALDatasetEx& outputDS, CGDALDatasetEx& debugDS);
		ERMsg LoadFires(CLandsatDataset& lansatDS, CGDALDatasetEx& fireDS, FireBitset& fires, std::vector <bool>& bHaveFire);
		void ReadBlock(size_t xBlock, size_t yBlock, CBandsHolder& bandHolder);
		void ComputeBufferStat(size_t xBlock, size_t yBlock, const CBandsHolder& bandHolder, const FireBitset& fires, BufferStat& bufferStat);
		void ProcessBlock(size_t xBlock, size_t yBlock, const CBandsHolder& bandHolder, const FireBitset& fire, const BufferStat& bufferStat, OutputData& output, DebugData& debug);
		void WriteBlock(size_t xBlock, size_t yBlock, OutputData& output, DebugData& debug, CGDALDatasetEx& outputDS, CGDALDatasetEx& debugDS);
		void CloseAll(CGDALDatasetEx& landsatDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& fireDS, CGDALDatasetEx& outputDS, CGDALDatasetEx& debugDS);
		void LoadData(const CBandsHolder& bandHolder, LansatData& data);
		
		CFireSeverityOption m_options;
		
		static size_t GetPrevious(size_t z, size_t x, size_t y, const CLandsatWindow& window);
		static size_t GetNext(size_t z, size_t x, size_t y, const CLandsatWindow& window);
		
		static std::string GetScenesDateFormat(const std::vector<CTPeriod>& p);
		static __int16 GetNbMissing(__int16 JD_base, std::array <CLandsatPixel, 2> p);
		static __int16 GetDeltaNBR(__int16 JD_base, std::array <CLandsatPixel, 2> p);
		static std::array<__int16, 2> GetZscore(const std::array <CLandsatPixel, 2>& p);
		static __int16 GetFireSeverity(__int16 dNBR);
		static int GetYear(const std::string& name);
		static size_t FindLayerIndex(CLandsatDataset& lansatDS, int year);
	};

}