//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
#pragma once

#include "basic/ERMsg.h"
#include "Basic/WeatherDefine.h"
#include "Basic/GeoBasic.h"
#include "Geomatic/GDALBasic.h"


namespace WBSF
{
	class CWeatherStation;
	class CNormalsStation;

	//file with extension MMG for climatic change anomalies
	class CMonthlyMeanGrid
	{
	public:
		enum TMEMBER { FIRST_YEAR, LAST_YEAR, VARIABLES_USED, NB_MEMBER };
		static const char* GetMemberName(int i){ ASSERT(i >= 0 && i < NB_MEMBER); return MEMBER_NAME[i]; }
		static const char* GetXMLFlag(){ return XML_FLAG; }


		CMonthlyMeanGrid()
		{
			Reset();
		}

		void Reset()
		{
			m_firstYear = -1;
			m_lastYear = -1;

			for (int i = 0; i < NORMALS_DATA::NB_FIELDS; i++)
				m_supportedVariables[i] = false;
		}

		ERMsg Open(const std::string& filePath, CCallback& callback);
		void Close();

		
		ERMsg Load(const std::string& filePath);
		ERMsg Save(const std::string& filePath);

		std::string GetMember(size_t i)const;
		void SetMember(size_t i, const std::string& str);

		virtual std::string GetFilePath(size_t v);
		virtual bool UpdateData(int firstRefYear, int firstccYear, size_t nbNeighbor, double maxDistance, double power, CWeatherStation& station, CCallback& callback);
		virtual bool UpdateData(int firstRefYear, int firstccYear, size_t nbNeighbor, double maxDistance, double power, CNormalsStation& station, CCallback& callback);
		bool UpdateStandardDeviation(int firstRefYear, int firstccYear, size_t nbNeighbor, double maxDistance, double power, CNormalsStation& station, CCallback& callback);
		virtual ERMsg ExportMonthlyValue(int firstRefYear, int firstccYear, size_t nbNeighbor, CWeatherStation& station, const std::string& filePath, CCallback& callback);

		int m_firstYear;
		int m_lastYear;
		std::bitset<NORMALS_DATA::NB_FIELDS> m_supportedVariables;

		std::string GetVariablesUsed()const;
		void SetVariablesUsed(std::string str);

	protected:

		bool GetMonthlyMean(int year, size_t nbNeighbor, double power, const CGeoPointIndexVector& pts, const std::vector<double>& d, double monthlyMean[12][NORMALS_DATA::NB_FIELDS], CCallback& callback);
		float GetMonthlyMean(size_t v, int year, size_t m, size_t nbNeighbor, double power, const CGeoPointIndexVector& pts, const std::vector<double>& d);
		


		std::string m_filePath;
		CGDALDatasetEx m_grid[NORMALS_DATA::NB_FIELDS];

		static const char * FILE_NAME[NORMALS_DATA::NB_FIELDS];
		static const char* XML_FLAG;
		static const char* MEMBER_NAME[NB_MEMBER];

	};


	class CNormalFromDaily
	{
	public:

		CNormalFromDaily();
		void Reset();

		enum TCCPeriod { P_1961_1990, P_1971_2000, P_1981_2010, P_1991_2020, P_2001_2030, P_2011_2040, P_2021_2050, P_2031_2060, P_2041_2070, P_2051_2080, P_2061_2090, P_2071_2100, NB_CC_PERIODS };
		enum TMEMBER { INPUT_DB, FIRST_YEAR, LAST_YEAR, MINIMUM_YEARS, NB_NEIGHBOR, OUPUT_DB, APPLY_CC, INPUT_MMG, REF_PERIOD_INDEX, CCPERIOD_INDEX, NB_MEMBER };//CREATE_ALL, 
		static const char* GetMemberName(int i){ ASSERT(i >= 0 && i < NB_MEMBER); return MEMBER_NAME[i]; }
		static const char* GetXMLFlag(){ return XML_FLAG; }
		static const int FIRST_YEAR_OF_FIRST_PERIOD = 1961;
		static const int LAST_YEAR_OF_FIRST_PERIOD = 1990;
		
		typedef std::bitset<NB_CC_PERIODS> CCPeriodBitset;
		std::string GetMember(size_t i)const;
		void SetMember(size_t i, const std::string& str);



		ERMsg CreateNormalDatabase(CCallback& callback);
		ERMsg ApplyClimaticChange(CWeatherStation& dailyStation, CMonthlyMeanGrid& cc, size_t p, CCallback& callback);
		void CleanUpYears(CWeatherStation& dailyStation, short firstYear, short lastYear);
		
		size_t GetNbDBCreate();
		size_t GetCCPeriod(size_t i);
		int GetFirstYear(size_t i);
		int GetLastYear(size_t i);
		std::string GetOutputFilePath(size_t i);


		std::string m_inputDBFilePath;
		std::string m_outputDBFilePath;

		int m_firstYear;
		int m_lastYear;
		int m_nbYearMin;
		int m_nbNeighbor;
		int m_maxDistance;
		double m_power;

		//climatic change section
		bool m_bApplyCC;
		std::string m_inputMMGFilePath;
		int m_refPeriodIndex;
		CCPeriodBitset m_CCPeriodIndex;
		//bool m_bCreateAll;


	protected:

		std::string m_filePath;

		static const char* XML_FLAG;
		static const char* MEMBER_NAME[NB_MEMBER];
	};



	

}//WBSF

namespace zen
{

	template <> inline
		void writeStruc(const WBSF::CMonthlyMeanGrid& MMG, XmlElement& output)
	{
		XmlOut out(output);
		out[WBSF::CMonthlyMeanGrid::GetMemberName(WBSF::CMonthlyMeanGrid::FIRST_YEAR)](MMG.m_firstYear);
		out[WBSF::CMonthlyMeanGrid::GetMemberName(WBSF::CMonthlyMeanGrid::LAST_YEAR)](MMG.m_lastYear);
		out[WBSF::CMonthlyMeanGrid::GetMemberName(WBSF::CMonthlyMeanGrid::VARIABLES_USED)](MMG.GetVariablesUsed());

	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CMonthlyMeanGrid& MMG)
	{
		XmlIn in(input);

		in[WBSF::CMonthlyMeanGrid::GetMemberName(WBSF::CMonthlyMeanGrid::FIRST_YEAR)](MMG.m_firstYear);
		in[WBSF::CMonthlyMeanGrid::GetMemberName(WBSF::CMonthlyMeanGrid::LAST_YEAR)](MMG.m_lastYear);
		std::string str;
		in[WBSF::CMonthlyMeanGrid::GetMemberName(WBSF::CMonthlyMeanGrid::VARIABLES_USED)](str);
		MMG.SetVariablesUsed(str);

		return true;
	}
}




