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
	enum TOutput {O_DNBR, O_ZSCORE, O_FIRE_SEV, NB_OUTPUTS};
	typedef std::deque< std::array< std::vector<__int16>, NB_OUTPUTS>> OutputData;
	typedef std::vector< CLandsatPixelVector > LansatData;
	typedef std::vector < std::map<std::pair<__int16, __int16>, CStatistic>> BufferStat;
	typedef std::vector < std::pair<std::array<size_t, NB_FB_TYPE1>, std::array<boost::dynamic_bitset<size_t>, NB_FB_TYPE2>>> FireBitset;

	class CFireSeverityOption : public CBaseOptions
	{
	public:

		
		//enum TMethod { M_SIMPLE, M_BUFFER, NB_METHODS };
		enum TFilePath { FIRE_FILE_PATH, LANDSAT_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };
		

		//enum TDebug { D_DEBUG_FLAG, D_NB_SCENE, D_SCENE_USED, D_MODEL, D_DELTA_B1, D_DELTA_TCB/*, D_DELTA_B1_REF, D_DELTA_TCB_REF*/, NB_DBUG };
		//static const char* DEBUG_NAME[NB_DBUG];

		CFireSeverityOption();

		virtual ERMsg ParseOption(int argc, char* argv[]);
		virtual ERMsg ProcessOption(int& i, int argc, char* argv[]);

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

		
		ERMsg OpenAll(CLandsatDataset& lansatDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& fireDS, CGDALDatasetEx& outputDS);
		ERMsg LoadFires(CLandsatDataset& lansatDS, CGDALDatasetEx& fireDS, FireBitset& fires, std::vector <bool>& bHaveFire);
		void ReadBlock(size_t xBlock, size_t yBlock, CBandsHolder& bandHolder);
		void ComputeBufferStat(size_t xBlock, size_t yBlock, const CBandsHolder& bandHolder, const FireBitset& fires, BufferStat& bufferStat);
		void ProcessBlock(size_t xBlock, size_t yBlock, const CBandsHolder& bandHolder, const FireBitset& fire, const BufferStat& bufferStat, OutputData& output);
		void WriteBlock(size_t xBlock, size_t yBlock, OutputData& output, CGDALDatasetEx& outputDS);
		void CloseAll(CGDALDatasetEx& landsatDS, CGDALDatasetEx& maskDS, CGDALDatasetEx& fireDS, CGDALDatasetEx& outputDS);
		void LoadData(const CBandsHolder& bandHolder, LansatData& data);
		
		CFireSeverityOption m_options;
		
		static size_t GetPrevious(size_t z, size_t x, size_t y, const CLandsatWindow& window);
		static size_t GetNext(size_t z, size_t x, size_t y, const CLandsatWindow& window);
		
		static std::string GetScenesDateFormat(const std::vector<CTPeriod>& p);
		static __int16 GetDeltaNBR(__int16 JD_base, std::array <CLandsatPixel, 2> p);
		static __int16 GetZscore(const std::array <CLandsatPixel, 2>& p);
		static __int16 GetFireSeverity(__int16 dNBR);
		static int GetYear(const std::string& name);
		static size_t FindLayerIndex(CLandsatDataset& lansatDS, int year);
	};

}