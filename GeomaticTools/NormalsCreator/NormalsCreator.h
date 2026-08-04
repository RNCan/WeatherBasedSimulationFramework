//***********************************************************************
#include <bitset>
#include <boost/dynamic_bitset.hpp>

#include "Basic/DailyDatabase.h"
#include "Basic/NormalsDatabase.h"
#include "Basic/AdvancedNormalStation.h"
#include "Simulation/MonthlyMeanGrid.h"
#include "Geomatic/GDALBasic.h"

namespace WBSF
{

	class CNormalsCreatorOption : public CBaseOptions
	{
	public:

		enum TPeriod {P1961_1990, P1971_2000, P1981_2010, P1991_2020, P2001_2030, P2011_2040,
			P2021_2050, P2031_2060, P2041_2070, P2051_2080, P2061_2090, P2071_2100, NB_PERIODS };
		enum TFrequency{ FREQUENCY = -1 };

		CNormalsCreatorOption();
		//bool ApplyCC()const {
		//	return !m_inputMMGFilePath.empty();
		//}



		virtual ERMsg ParseOption(int argc, char* argv[]);
		virtual ERMsg ProcessOption(int& i, int argc, char* argv[]);
		std::string GetOutputFilePath(size_t p);

		int m_firstYear;
		int m_lastYear;
		int m_nbYearMin;
		

		//climatic change section
		//bool m_bApplyCC;
		//std::string m_inputMMGFilePath;

		//int m_firstRefYear;
		size_t m_reference_period;
		std::bitset<NB_PERIODS> m_CC_period;
		int m_nbNeighbor;
		int m_maxDistance;
		double m_power;
		
	};

	typedef std::unique_ptr<CBandsHolder>CBandsHolderPtr;

	//***********************************************************************
	//									 
	//	Main                                                             
	//									 
	//***********************************************************************
	class CNormalsCreator
	{
	public:

		//enum TCondition{ ALL_VALID, AT_LEAST_ONE_VALID, AT_LEAST_ONE_MISSING, ALL_MISSING, NB_CONDITION };
		enum TFilePath { INPUT_FILE_PATH, MMG_FILE_PATH, OUTPUT_FILE_PATH, NB_FILE_PATH };

		std::string GetDescription(){ return  std::string("NormalsCreator version ") + VERSION + " (" + __DATE__ + ")"; }
		ERMsg Execute();

		
		ERMsg OpenAll(CDailyDatabase& inputDB, CMonthlyMeanGrid& MMG, std::array<CNormalsDatabasePtr, CNormalsCreatorOption::NB_PERIODS>& normalsDB);
		static size_t GetNbStationIn(const CGeoExtents& blockExtents, const CDailyDatabase& inputDB);
		void ReadBlock(int xBlock, int yBlock, std::array< CBandsHolderPtr, NORMALS_DATA::NB_FIELDS>& bandHolder);
		void ProcessBlock(int xBlock, int yBlock, std::array< CBandsHolderPtr, NORMALS_DATA::NB_FIELDS>& bandHolder, CDailyDatabase& inputDB, std::array<CNormalsDatabasePtr, CNormalsCreatorOption::NB_PERIODS>& normalsDB, ERMsg& warnings);
		void CloseAll(CDailyDatabase& inputDB, CMonthlyMeanGrid& MMG, std::array<CNormalsDatabasePtr, CNormalsCreatorOption::NB_PERIODS>& normalsDB);

		bool UpdateData(size_t p, CGeoExtents blockExtents, std::array< CBandsHolderPtr, NORMALS_DATA::NB_FIELDS>& bandHolder, CWeatherStation& stationIn);
		bool UpdateStandardDeviation(size_t p, CGeoExtents blockExtents, std::array< CBandsHolderPtr, NORMALS_DATA::NB_FIELDS>& bandHolder, CNormalsStation& station);
		bool GetMonthlyMean(int firstYear, size_t nbYears, size_t nbNeighbor, double power, const CGeoPointIndexVector& pts, const std::vector<double>& d, double monthlyMean[12][NORMALS_DATA::NB_FIELDS], std::array< CBandsHolderPtr, NORMALS_DATA::NB_FIELDS>& bandHolder)const;
		float GetMonthlyMean(size_t v, int year, size_t m, size_t nbNeighbor, double power, const CGeoPointIndexVector& pts, const std::vector<double>& d, std::array< CBandsHolderPtr, NORMALS_DATA::NB_FIELDS>& bandHolder)const;
		bool GetMonthlyValues(int firstYear, size_t nbYears, size_t nbNeighbor, double maxDistance, double power, const CGeoPoint& ptIn, CGeoExtents blockExtents, std::vector< std::array<std::array<float, NORMALS_DATA::NB_FIELDS>, 12>>& values, std::array< CBandsHolderPtr, NORMALS_DATA::NB_FIELDS>& bandHolder)const;
		bool GetNearestPoints(size_t nbNeighbor, double maxDistance, double power, const CGeoPoint& ptIn, CGeoExtents blockExtents, CGeoPointIndexVector& pts, std::vector<double>& d, std::array< CBandsHolderPtr, NORMALS_DATA::NB_FIELDS>& bandHolder)const;

		CNormalsCreatorOption m_options;
		CMonthlyMeanGrid m_MMG;

		static const char * VERSION;
		static const int NB_THREAD_PROCESS;

	};


}