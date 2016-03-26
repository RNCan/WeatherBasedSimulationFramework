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
			//m_bSpecificHumidity=true;
			for (int i = 0; i < NORMALS_DATA::NB_FIELDS; i++)
				m_supportedVariables[i] = false;
		}

		ERMsg Open(const std::string filePath);
		void Close();

		//est-ce utiliser???
		//ERMsg Load(std::string filePath){ m_filePath = filePath; return XLoad(filePath.c_str(), *this); }
		//ERMsg Save(std::string filePath){ m_filePath = filePath; return XSave(filePath.c_str(), *this); }




//		void GetXML(LPXNode& pRoot)const{ XGetXML(*this, pRoot); }
	//	void SetXML(const LPXNode pRoot){ XSetXML(*this, pRoot); }
		//std::string GetMember(int i, LPXNode& pNode = NULL_ROOT)const;
		//void SetMember(int i, const std::string& str, const LPXNode pNode = NULL_ROOT);
		std::string GetMember(size_t i)const;
		void SetMember(size_t i, const std::string& str);

		virtual std::string GetFilePath(int var);
		virtual bool UpdateData(short firstRefYear, short firstccYear, short nbNeighbor, int maxDistance, double power, CWeatherStation& station);
		virtual bool UpdateData(short firstRefYear, short firstccYear, short nbNeighbor, int maxDistance, double power, CNormalsStation& station);
		bool UpdateStandardDeviation(short firstRefYear, short firstccYear, short nbNeighbor, int maxDistance, double power, CNormalsStation& station);
		virtual ERMsg ExportMonthlyValue(short firstRefYear, short firstccYear, short nbNeighbor, CWeatherStation& station, const std::string& filePath, CCallback& callback);

		int m_firstYear;
		int m_lastYear;
		bool m_supportedVariables[NORMALS_DATA::NB_FIELDS];

		//bool m_bSpecificHumidity;

	protected:

		bool GetMonthlyMean(short year, int nbNeighbor, double power, const CGeoPointIndexVector& pts, const std::vector<double>& d, double monthlyMean[12][NORMALS_DATA::NB_FIELDS]);
		float GetMonthlyMean(short v, short year, short m, int nbNeighbor, double power, const CGeoPointIndexVector& pts, const std::vector<double>& d);
		std::string GetVariablesUsed()const;
		void SetVariablesUsed(std::string str);


		std::string m_filePath;

		//CMapBilFile m_grid[NORMAL_DATA::NB_FIELDS];
		CGDALDatasetEx m_grid[NORMALS_DATA::NB_FIELDS];

		static const char * FILE_NAME[NORMALS_DATA::NB_FIELDS];
		static const char* XML_FLAG;
		static const char* MEMBER_NAME[NB_MEMBER];

	};


	//typedef std::auto_ptr<CMonthlyMeanGrid> CMonthlyMeanGridPtr;

	class CNormalFromDaily
	{
	public:

		CNormalFromDaily();
		void Reset();

		enum TMEMBER { INPUT_DB, FIRST_YEAR, LAST_YEAR, MINIMUM_YEARS, NB_NEIGHBOR, OUPUT_DB, APPLY_CC, INPUT_MMG, REF_PERIOD_INDEX, CCPERIOD_INDEX, CREATE_ALL, NB_MEMBER };
		static const char* GetMemberName(int i){ ASSERT(i >= 0 && i < NB_MEMBER); return MEMBER_NAME[i]; }
		static const char* GetXMLFlag(){ return XML_FLAG; }

//		ERMsg Load(std::string filePath){ m_filePath = filePath; return XLoad(filePath.c_str(), *this); }
		//ERMsg Save(std::string filePath){ m_filePath = filePath; return XSave(filePath.c_str(), *this); }
		//void GetXML(LPXNode& pRoot)const{ XGetXML(*this, pRoot); }
		//void SetXML(const LPXNode pRoot){ XSetXML(*this, pRoot); }
		//std::string GetMember(int i, LPXNode& pNode = NULL_ROOT)const;
		//void SetMember(int i, const std::string& str, const LPXNode pNode = NULL_ROOT);
		std::string GetMember(size_t i)const;
		void SetMember(size_t i, const std::string& str);



		ERMsg CreateNormalDatabase(CCallback& callback);
		ERMsg ApplyClimaticChange(CWeatherStation& dailyStation, CMonthlyMeanGrid& cc, CCallback& callback);
		void CleanUpYears(CWeatherStation& dailyStation, short firstYear, short lastYear);
		int GetNbDBCreate();
		int GetFirstYear(int i);
		int GetLastYear(int i);
		std::string GetOutputFilePath(int i);


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
		//int m_firstApplyedCCYear;
		int m_refPeriodIndex;
		int m_CCPeriodIndex;
		bool m_bCreateAll;


	protected:

		std::string m_filePath;

		static const char* XML_FLAG;
		static const char* MEMBER_NAME[NB_MEMBER];
	};
}