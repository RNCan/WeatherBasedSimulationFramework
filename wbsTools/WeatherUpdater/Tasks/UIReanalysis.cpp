//Integrated Surface Data - “Lite”
#include "stdafx.h"
#include "UIReanalysis.h"

#include <boost\filesystem.hpp>
#include "Basic/FileStamp.h"
#include "Basic/DailyDatabase.h"
#include "Geomatic/TimeZones.h"
#include "Geomatic/GDALBasic.h"
#include "UI/Common/SYShowMessage.h"
#include "TaskFactory.h"
#include "../Resource.h"
#include "ERA5.h"
#include "20CRv3.h"




using namespace std; 
using namespace WBSF::HOURLY_DATA;
using namespace UtilWWW;

#pragma warning(disable: 4275 4251)
#include "gdal_priv.h"
#include "cpl_conv.h"
#include "ogr_srs_api.h"



namespace WBSF
{


	const char* CUIReanalysis::SERVER_NAME = "";
	const char* CUIReanalysis::SERVER_PATH = "";
	

	//*********************************************************************
	const char* CUIReanalysis::ATTRIBUTE_NAME[NB_ATTRIBUTES] = { "WorkingDir", "Product", "Frequency", "FirstYear", "LastYear", "BoundingBox", "ShowDownload" };
	const size_t CUIReanalysis::ATTRIBUTE_TYPE[NB_ATTRIBUTES] = { T_PATH, T_COMBO_INDEX, T_COMBO_INDEX, T_STRING, T_STRING, T_GEORECT, T_BOOL };
	const UINT CUIReanalysis::ATTRIBUTE_TITLE_ID = IDS_UPDATER_ERA5_P;
	const UINT CUIReanalysis::DESCRIPTION_TITLE_ID = ID_TASK_ERA5;

	const char* CUIReanalysis::CLASS_NAME(){ static const char* THE_CLASS_NAME = "Reanalysis";  return THE_CLASS_NAME; }
	CTaskBase::TType CUIReanalysis::ClassType()const { return CTaskBase::UPDATER; }
	static size_t CLASS_ID = CTaskFactory::RegisterTask(CUIReanalysis::CLASS_NAME(), (createF)CUIReanalysis::create);
	

	static const CGeoRect DEFAULT_BOUDINGBOX(-180, -90, 180, 90, PRJ_WGS_84);

	CUIReanalysis::CUIReanalysis(void)
	{}

	CUIReanalysis::~CUIReanalysis(void)
	{
	}


	std::string CUIReanalysis::Option(size_t i)const
	{
		string str;
		switch (i)
		{
		case PRODUCT: str = "ERA5|20CRv3"; break;
		case FREQUENCY:	str = "Hourly|Daily"; break;
		};
		return str;
	}

	std::string CUIReanalysis::Default(size_t i)const
	{
		string str;

		switch (i)
		{
		case WORKING_DIR: str = m_pProject->GetFilePaht().empty() ? "" : GetPath(m_pProject->GetFilePaht()) + "Reanalysis\\"; break;
		case PRODUCT: str = "0"; break;
		case FREQUENCY: str = "1"; break;
		case FIRST_YEAR:  str = "1950"; break;
		case LAST_YEAR:	str = "2020"; break;
		case BOUNDING_BOX: str = ToString(DEFAULT_BOUDINGBOX); break;
		case SHOW_DONWLOAD:str = "0"; break;
		};

		return str;
	}

	//*****************************************************************************



	ERMsg CUIReanalysis::Execute(CCallback& callback)
	{
		ERMsg msg;
		size_t product = as<size_t>(PRODUCT);
		if (product == DATA_ERA5)
		{
			CERA5 ERA5;
			ERA5.m_workingDir = GetDir(WORKING_DIR);
			ERA5.m_first_year = as<int>(FIRST_YEAR);
			ERA5.m_last_year = as<int>(LAST_YEAR);
			ERA5.m_frequency = as<size_t>(FREQUENCY);
			ERA5.m_show_download = as<bool>(SHOW_DONWLOAD);
			ERA5.m_bounding_box = as<CGeoRect>(BOUNDING_BOX);
			ERA5.m_bounding_box.SetPrjID(PRJ_WGS_84);
			msg = ERA5.Execute(callback);
		}
		else if (product == DATA_20CRV3)
		{
			C20CRv3 i20CRv3;
			i20CRv3.m_workingDir = GetDir(WORKING_DIR);
			i20CRv3.m_first_year = as<int>(FIRST_YEAR);
			i20CRv3.m_last_year = as<int>(LAST_YEAR);
			i20CRv3.m_frequency = as<size_t>(FREQUENCY);
			i20CRv3.m_show_download = as<bool>(SHOW_DONWLOAD);
			i20CRv3.m_bounding_box = as<CGeoRect>(BOUNDING_BOX);
			i20CRv3.m_bounding_box.SetPrjID(PRJ_WGS_84);

			if (i20CRv3.m_frequency == DATA_DAILY)
				msg = i20CRv3.Execute(callback);
			else 
				msg.ajoute("Hourly not supported for 20CRv3");
		}
		

		return msg;
	}

	ERMsg CUIReanalysis::GetGribsList(CTPeriod p, CGribsMap& gribsList, CCallback& callback)
	{
		ERMsg msg;

		CERA5 ERA5;
		ERA5.m_workingDir = GetDir(WORKING_DIR);
		ERA5.m_first_year = as<int>(FIRST_YEAR);
		ERA5.m_last_year = as<int>(LAST_YEAR);
		ERA5.m_frequency = as<size_t>(FREQUENCY);

		return ERA5.GetGribsList(p, gribsList, callback);
	}

	ERMsg CUIReanalysis::GetStationList(StringVector& stationList, CCallback& callback)
	{
		ERMsg msg;

		CERA5 ERA5;
		ERA5.m_workingDir = GetDir(WORKING_DIR);
		ERA5.m_first_year = as<int>(FIRST_YEAR);
		ERA5.m_last_year = as<int>(LAST_YEAR);
		ERA5.m_frequency = as<size_t>(FREQUENCY);

		return ERA5.GetStationList(stationList, callback);
	}
	
	ERMsg CUIReanalysis::GetWeatherStation(const string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
	{
		ERMsg msg;

		
		CERA5 ERA5;
		ERA5.m_workingDir = GetDir(WORKING_DIR);
		ERA5.m_first_year = as<int>(FIRST_YEAR);
		ERA5.m_last_year = as<int>(LAST_YEAR);
		ERA5.m_frequency = as<size_t>(FREQUENCY);

		return ERA5.GetWeatherStation(ID, TM, station, callback);
		
	}

//	ERMsg CUIReanalysis::GetStationListD(StringVector& stationList, CCallback& callback)
//	{
//		ERMsg msg;
//
//
//		//get all file in the directory 
//		StringVector fileList;
//		int firstYear = as<int>(FIRST_YEAR);
//		int lastYear = as<int>(LAST_YEAR);
//		size_t nbYears = lastYear - firstYear + 1;
//		string workingDir = GetDir(WORKING_DIR);
//		CGeoRect rect = as<CGeoRect>(BOUNDING_BOX);
//		rect.SetPrjID(PRJ_WGS_84);
//
//		string strSearch = workingDir + "Daily\\Reanalysis_1dd_v1.2_p1d.*";
//		StringVector fileListTmp = GetFilesList(strSearch);
//		sort(fileListTmp.begin(), fileListTmp.end());
//
//		
//		for (size_t i = 0; i < fileListTmp.size() && msg; i++)
//		{
//			string ext = fileListTmp[i].substr(fileListTmp[i].size() - 6);
//			ASSERT(ext.length() == 6);
//			int year = ToInt(ext.substr(0, 4));
//			size_t m = ToSizeT(ext.substr(4, 2)) - 1;
//			if (year >= firstYear && year <= lastYear)
//			{
//				fileList.push_back(fileListTmp[i]);
//				if (!m_firstTRef.IsInit())
//					m_firstTRef = CTRef(year, m);
//
//				ASSERT((m_firstTRef + fileList.size() - 1).GetYear() == year && (m_firstTRef + fileList.size() - 1).GetMonth() == m);
//			}
//		}
//
//		if (msg && !fileList.empty())
//		{
//			_setmaxstdio(1024);
//			m_files.resize(fileList.size());
//
//			//remove file 
//			for (size_t i = 0; i < fileList.size() && msg; i++)
//				msg += m_files[i].open(fileList[i], ios::in | ios::binary);
//			
//			stationList.reserve(180 * 360);
//			for (size_t i = 0; i < 180; i++)
//			{
//				for (size_t j = 0; j < 360; j++)
//				{
//					double lat = 89.5 - i;
//					double lon = 0.5 + j;
//					if (lon > 180)
//						lon -= 360;
//
//					CGeoPoint pt(lon, lat, PRJ_WGS_84);
//
//					if (rect.IsEmpty() || rect.IsInside(pt))
//					{
//						string title = FormatA("%03d_%03d", i + 1, j + 1);
//						stationList.push_back(title);
//					}
//
//					msg += callback.StepIt(0);
//				}
//			}
//		}
//
//
//
//		return msg;
//	}
//
//
//
//
//	ERMsg CUIReanalysis::GetWeatherStationD(const string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
//	{
//		ASSERT(ID.length()==7);
//		ERMsg msg;
//
//		size_t i = WBSF::as<size_t>(ID.substr(0,3))-1;
//		size_t j = WBSF::as<size_t>(ID.substr(4, 3))-1;
//		double lat = 89.5 - i;
//		double lon = 0.5 + j;
//		if (lon > 180)
//			lon -= 360;
//
//		((CLocation&)station) = CLocation(ID,ID,lat,lon,0);
//
//
//		int firstYear = m_firstTRef.GetYear();
//		int lastYear = (m_firstTRef + m_files.size()).GetYear();
//		size_t nbYears = lastYear - firstYear + 1;
//		station.CreateYears(firstYear, nbYears);
//		
//		for (size_t mm = 0; mm < m_files.size() && msg; mm++)
//		{
//			CTRef TRef = m_firstTRef + mm;
//			TRef.Transform(CTM(CTM::DAILY));
//			size_t nbDays = TRef.GetNbDayPerMonth();
//			for (size_t d = 0; d < nbDays&& msg; d++, TRef++)
//			{
//				m_files[mm].seekg(1440 + (d*180*360+(i*360+j))*4);
//				
//				float prcp = 0;
//				m_files[mm].read((char*)(&prcp), sizeof(prcp));
//				//prcp = ReverseFloat(prcp);
//
//				if (isnan(prcp) || !isfinite(prcp))
//					prcp = -999;
//
//				if (prcp > 0 && prcp < 0.00001f)
//					prcp = 0;
//
//				if (prcp < -999)
//					prcp = -999;
//			
//				ASSERT(prcp == -999 || prcp >= 0);
//
//				station[TRef].SetStat(H_PRCP, prcp);
//			}
//		}
//
//		
//		if (msg && station.HaveData())
//			msg = station.IsValid();
//		
//		return msg;
//	}
//
//
//ERMsg CUIReanalysis::GetStationListM(StringVector& stationList, CCallback& callback)
//{
//	ERMsg msg;
//
//
//	//get all file in the directory 
//	StringVector fileList;
//	int firstYear = as<int>(FIRST_YEAR);
//	int lastYear = as<int>(LAST_YEAR);
//	size_t nbYears = lastYear - firstYear + 1;
//	string workingDir = GetDir(WORKING_DIR);
//	CGeoRect rect = as<CGeoRect>(BOUNDING_BOX); 
//	rect.SetPrjID(PRJ_WGS_84);
//	//CGeoRect(-80, 43, -50, 63, PRJ_WGS_84);
//
//	string strSearch = workingDir + "Monthly\\Reanalysis_v2.2_psg.*";
//	StringVector fileListTmp = GetFilesList(strSearch);
//	sort(fileListTmp.begin(), fileListTmp.end());
//
//	for (size_t i = 0; i < fileListTmp.size() && msg; i++)
//	{
//		string ext = fileListTmp[i].substr(fileListTmp[i].size() - 4);
//		ASSERT(ext.length() == 4);
//		int year = ToInt(ext);
//
//		if (year >= firstYear && year <= lastYear)
//		{
//			fileList.push_back(fileListTmp[i]);
//			if (!m_firstTRef.IsInit())
//				m_firstTRef = CTRef(year);
//
//
//			ASSERT((m_firstTRef.GetYear() + fileList.size() - 1) == year);
//		}
//
//	}
//
//	if (msg && !fileList.empty())
//	{
//		_setmaxstdio(1024);
//		m_files.resize(fileList.size());
//
//		//remove file 
//		for (size_t i = 0; i < fileList.size() && msg; i++)
//			msg += m_files[i].open(fileList[i], ios::in | ios::binary);
//
//
//		stationList.reserve(144 * 72);
//		for (size_t i = 0; i < 72; i++)
//		{
//			for (size_t j = 0; j < 144; j++)
//			{
//				double lat = 88.75 - 2.5*i;
//				double lon = 1.25 + 2.5*j;
//				if (lon > 180)
//					lon -= 360;
//
//				CGeoPoint pt(lon, lat, PRJ_WGS_84);
//
//				if (rect.IsEmpty() || rect.IsInside(pt))
//				{
//					string title = FormatA("%03d_%03d", i + 1, j + 1);
//					stationList.push_back(title);
//				}
//
//				msg += callback.StepIt(0);
//			}
//		}
//	}
//
//
//
//
//	return msg;
//}
//
//	ERMsg CUIReanalysis::GetWeatherStationM(const string& ID, CTM TM, CWeatherStation& station, CCallback& callback)
//	{
//		ASSERT(ID.length() == 7);
//		ERMsg msg;
//
//		size_t i = WBSF::as<size_t>(ID.substr(0, 3)) - 1;
//		size_t j = WBSF::as<size_t>(ID.substr(4, 3)) - 1;
//		double lat = 88.75 - 2.5*i;
//		double lon = 1.25 + 2.5*j;
//		if (lon > 180)
//			lon -= 360;
//
//		((CLocation&)station) = CLocation(ID, ID, lat, lon, 0);
//
//
//		int firstYear = m_firstTRef.GetYear();
//		int lastYear = (m_firstTRef + m_files.size()).GetYear();
//		size_t nbYears = lastYear - firstYear + 1;
//		station.CreateYears(firstYear, nbYears);
//
//		for (size_t y = 0; y < m_files.size() && msg; y++)
//		{
//			int year = m_firstTRef.GetYear() + int(y);
//			for (size_t m= 0; m < 12&& msg; m++)
//			{
//				m_files[y].seekg(576 + (m * 72 * 144 + (i * 144 + j)) * 4);
//
//				float prcp = 0;
//				m_files[y].read((char*)(&prcp), sizeof(prcp));
//				//prcp = ReverseFloat(prcp);
//
//				if (isnan(prcp) || !isfinite(prcp))
//					prcp = -999;
//
//				if (prcp > 0 && prcp < 0.00001f)
//					prcp = 0;
//
//				if (prcp < -999)
//					prcp = -999;
//
//				ASSERT(prcp == -999 || (prcp >= 0 && prcp < 316));
//
//				for (size_t d = 0; d < WBSF::GetNbDayPerMonth(year, m) && msg; d++)
//				{
//					CTRef TRef(year, m, d);
//					station[TRef].SetStat(H_PRCP, prcp);//prcp is in mm/day
//				}
//			}
//		}
//
//
//		if (msg && station.HaveData())
//			msg = station.IsValid();
//
//		return msg;
//	}


}