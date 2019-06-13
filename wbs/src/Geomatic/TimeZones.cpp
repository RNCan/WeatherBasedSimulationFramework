//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
// 22-04-2018	Rémi Saint-Amant	Remove used of cctz, always work in Standar time never in daylight time
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//******************************************************************************
#include "stdafx.h"



#include <iostream>
#include <chrono>
#include <string>

#include "TimeZones.h"
//#include "cctz/civil_time.h"
//#include "cctz/time_zone.h"
//#include "cctz/time_zone_info.h"
#include "Geomatic/ShapefileBase.h"
//using namespace cctz;

 
namespace WBSF
{
	

	static WBSF::CShapeFileBase TIME_ZONE;
	ERMsg CTimeZones::Load(std::string& filePath)
	{
		ERMsg msg;
		if (TIME_ZONE.GetCount() == 0)
		{
			msg = TIME_ZONE.Read(filePath);
			if (msg)
				TIME_ZONE.ComputeInternalElimination(100, 50);
		}

		return msg;
	}

	
	/*template <typename D>
	cctz::time_point<cctz::sys_seconds> FloorDay(cctz::time_point<D> tp, cctz::time_zone tz)
	{
		return cctz::convert(cctz::civil_day(cctz::convert(tp, tz)), tz);
	}
	

	static cctz::time_point<cctz::sys_seconds> GetTimePoint(CTRef TRef, const cctz::time_zone& zone)
	{
		ASSERT(TRef.GetTM().Type() == CTM::HOURLY);
		cctz::civil_second cs(TRef.GetYear(), int(TRef.GetMonth() + 1), int(TRef.GetDay() + 1), int(TRef.GetHour()), 0, 0);
		return cctz::convert(cs, zone);
	}

	double CTimeZones::GetDecimalHour(__int64 time)
	{
		const auto tp = std::chrono::system_clock::from_time_t(time);
		const cctz::time_zone::absolute_lookup al = cctz::utc_time_zone().lookup(tp);
		return al.cs.hour() + al.cs.minute() / 60.0 + al.cs.second()/3600.0;
	}
*/

	double CTimeZones::GetDecimalHour(__int64 time)
	{
		/*time -= GetTime0(time);
		__int64 h = __int64 (time / 3600 );
		__int64 m = __int64((time - h*3600)/ 60);
		__int64 s = time - h * 3600 - m*60;
*/
		time_t t = time;
		tm* pTm = gmtime(&t);

		return pTm->tm_hour + pTm->tm_min / 60.0 + pTm->tm_sec / 3600.0;
	}

	//delta in second
	std::string CTimeZones::GetZoneName(const CGeoPoint& pt)
	{
		ASSERT(TIME_ZONE.GetNbShape() > 0);//TimeZone must be loaded...

		std::string str_zone;
		int poly = -1;

		if (TIME_ZONE.IsInside(pt, &poly))
		{
			const CDBF3& DBF = TIME_ZONE.GetDBF();
			const CDBFRecord& record = DBF[poly];
			ASSERT(record.size() >= 14);
			str_zone = record[13];

			ASSERT(!str_zone.empty());///some zone is empty.... hummm
		}
		ASSERT(poly != -1);


		return str_zone;
	}

	//delta in second
	__int64 CTimeZones::GetTimeZone(const CGeoPoint& pt)
	{
		ASSERT(TIME_ZONE.GetNbShape() > 0);//TimeZone must be loaded...

		__int64 time_zone = NOT_INIT; // in second
		
		int poly = -1;

		if (TIME_ZONE.IsInside(pt, &poly))
		{
			const CDBF3& DBF = TIME_ZONE.GetDBF();
			const CDBFRecord& record = DBF[poly];
			ASSERT(record.size() >= 14);
			std::string str_zone = record[3];
			ASSERT(!str_zone.empty());///some zone is empty.... hummm
			if (!str_zone.empty())
			{
				float zone = ToFloat(str_zone);
				ASSERT(zone >= -12 && zone <= 12);
				time_zone = zone * 3600;
			}
			else
			{
				//take default from longitude
				time_zone = __int64(pt.m_lon / 15 * 3600);
			}
			
		}
		ASSERT(poly != -1);


		return time_zone;
	}

	/*bool CTimeZones::GetZone(const CGeoPoint& pt, cctz::time_zone& zone)
	{
		bool bRep = false;

		std::string str_zone = GetZoneName(pt);
		if (!str_zone.empty())
			bRep = cctz::load_time_zone(str_zone, &zone);

		return bRep;
	}
*/

	
	__int64 CTimeZones::TRef2Time(CTRef TRef)
	{
		//ASSERT(TRef.GetTM().Type() == CTM::HOURLY);
		ASSERT(TRef.GetYear() >= 1970 && TRef.GetYear() < 2038);
		
		
		//cctz::time_point<cctz::sys_seconds> tp = GetTimePoint(TRef, cctz::utc_time_zone());
		//return tp.time_since_epoch().count();
		
		
		tm timeinfo = { 0 };
		timeinfo.tm_sec = 0;     // seconds after the minute - [0,59] 
		timeinfo.tm_min = 0;     // minutes after the hour - [0,59] 
		timeinfo.tm_hour = int(TRef.GetHour());    // hours since midnight - [0,23] 
		timeinfo.tm_mday = int(TRef.GetDay() + 1);    // day of the month - [1,31] 
		timeinfo.tm_mon = int(TRef.GetMonth());     // months since January - [0,11] 
		timeinfo.tm_year = int(TRef.GetYear()-1900);
		timeinfo.tm_isdst = 0;

		//__int64 test1 = mktime(&timeinfo);
		//__int64 test2 = _mkgmtime(&timeinfo);
		//__int64 test3 = (test1 - test2) / 3600;


			
		return _mkgmtime(&timeinfo);

	}

	CTRef CTimeZones::Time2TRef(__int64 time)
	{
		//const char time_as_str[] = "1296575549:573352";
		//time_t t = atoi(time_as_str); // convert to time_t, ignores msec
		//tm* pTm = localtime(&t);

		//const auto tp = std::chrono::system_clock::from_time_t(time);
		//const cctz::time_zone::absolute_lookup al = cctz::utc_time_zone().lookup(tp);
		//return CTRef (al.cs.year(), al.cs.month()-1, al.cs.day()-1, al.cs.hour());

		time_t t = time;
		tm* pTm = gmtime(&t);
		ASSERT(pTm->tm_mon>=0 && pTm->tm_mon<=12);
		ASSERT(pTm->tm_mday >= 1 && pTm->tm_mday <= GetNbDayPerMonth(pTm->tm_year, pTm->tm_mon));

		//__int64 test1 = mktime(pTm);
		//__int64 test2 = _mkgmtime(pTm);
		//__int64 test3 = (test1 - test2) / 3600;


		return CTRef(pTm->tm_year+1900, pTm->tm_mon, pTm->tm_mday - 1, pTm->tm_hour);
		
	}
	
	__int64 CTimeZones::GetTime0(__int64 time)
	{
		//return the UTC time of the begginning of the day (h=0)
		time_t t = time;
		//struct tm timeinfo = { 0 };
		struct tm timeinfo = *gmtime(&t);
		timeinfo.tm_sec = 0;     // seconds after the minute - [0,59] 
		timeinfo.tm_min = 0;     // minutes after the hour - [0,59] 
		timeinfo.tm_hour = 0;    // hours since midnight - [0,23] 

		return _mkgmtime(&timeinfo);
	}

	//__int64 CTimeZones::GetDelta(CTRef TRef, const CGeoPoint& pt)
	//{
	//	__int64 delta = __int64(pt.m_lon / 15 * 3600);

	//	

	//	cctz::time_zone zone;
	//	if (GetZone(pt, zone))
	//	{
	//		const auto tp = cctz::convert(cctz::civil_second(TRef.GetYear(), int(TRef.GetMonth() + 1), int(TRef.GetDay() + 1), int(TRef.GetHour()), 0, 0), zone);
	//		const cctz::time_zone::absolute_lookup al = zone.lookup(tp);
	//		delta = al.offset;
	//	}

	//	__int64 time_zone = GetTimeZone(pt);


	//	return delta;
	//}

	//__int64 CTimeZones::GetDelta(__int64 time, const CGeoPoint& pt)
	//{
	//	__int64 delta = __int64(pt.m_lon / 15 * 3600);
	//	cctz::time_zone zone;
	//	if (GetZone(pt, zone))
	//	{
	//		const auto tp = std::chrono::system_clock::from_time_t(time);
	//		const cctz::time_zone::absolute_lookup al = zone.lookup(tp);

	//		delta = al.offset;
	//	}


	//	return delta;
	//}
	
	__int64 CTimeZones::LocalTime2UTCTime(__int64 time, const CGeoPoint& pt)
	{
		//__int64 delta = GetDelta(time, pt);
		__int64 time_zone = GetTimeZone(pt);
		return time - time_zone;
	}
	__int64 CTimeZones::UTCTime2LocalTime(__int64 time, const CGeoPoint& pt)
	{
		//__int64 delta = GetDelta(time, pt);
		__int64 time_zone = GetTimeZone(pt);
		return time + time_zone;
	}

	CTRef CTimeZones::LocalTRef2UTCTRef(CTRef TRef, const CGeoPoint& pt)
	{
		//__int64 delta = GetDelta(TRef, pt);
		__int64 time_zone = GetTimeZone(pt);
		return TRef - int(time_zone / 3600);//can get problems with timezone at mid hour
	}

	CTRef CTimeZones::UTCTRef2LocalTRef(CTRef TRef, const CGeoPoint& pt)
	{
		//__int64 delta = GetDelta(TRef, pt);
		__int64 time_zone = GetTimeZone(pt);
		return TRef + int(time_zone /3600);
		
	}

	/*CTRef CTimeZones::LocalTRef2UTCTRef(CTRef TRef, const cctz::time_zone& zone)
	{
		const auto tp = cctz::convert(cctz::civil_second(TRef.GetYear(), int(TRef.GetMonth() + 1), int(TRef.GetDay() + 1), int(TRef.GetHour()), 0, 0), zone);
		const cctz::time_zone::absolute_lookup al = zone.lookup(tp);
		
		return TRef - int(al.offset / 3600);
	}

	CTRef CTimeZones::UTCTRef2LocalTRef(CTRef TRef, const cctz::time_zone& zone)
	{
		const auto tp = cctz::convert(cctz::civil_second(TRef.GetYear(), int(TRef.GetMonth() + 1), int(TRef.GetDay() + 1), int(TRef.GetHour()), 0, 0), zone);
		const cctz::time_zone::absolute_lookup al = zone.lookup(tp);

		return TRef + int(al.offset/3600);

	}*/

	
	

}