//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//******************************************************************************
#include "stdafx.h"



#include <iostream>
#include <chrono>
#include <string>

#include "TimeZones.h"
#include "cctz/civil_time.h"
#include "cctz/time_zone.h"
#include "cctz/time_zone_info.h"
#include "Geomatic/ShapefileBase.h"
//using namespace cctz;


namespace WBSF
{
	

	static WBSF::CShapeFileBase TIME_ZONE;
	ERMsg CTimeZones::Load(std::string& filePath)
	{
		ERMsg msg = TIME_ZONE.Read(filePath);
		if (msg)
			TIME_ZONE.ComputeInternalElimination(100,50);

		return msg;
	}

	
	template <typename D>
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

	//delta in second
	bool CTimeZones::GetZone(const CGeoPoint& pt, cctz::time_zone& zone)//, __int64* pDelta
	{
		ASSERT(TIME_ZONE.GetNbShape() > 0);//TimeZone must be loaded...

		//cctz::time_zone zone = cctz::utc_time_zone();//just in case...
		bool bRep = false;
		//if (pDelta)
			//*pDelta = __int64(pt.m_lon / 15 * 3600);

		int poly = -1;

		if (TIME_ZONE.IsInside(pt, &poly))
		{
			const CDBF3& DBF = TIME_ZONE.GetDBF();
			const CDBFRecord& record = DBF[poly];
			ASSERT(record.size() >= 14);
			std::string str_zone = record[13];

			ASSERT(!str_zone.empty());///some zone is empty.... hummm

			if (!str_zone.empty() )
				bRep = cctz::load_time_zone(str_zone, &zone);
			/*else if (pDelta)
			{
				cctz::time_zone::Impl zone2();
				std::string str_zone = record[7];
				int z = WBSF::ToInt(str_zone);
				ASSERT(z >= -12 && z <= 12);

				*pDelta = z * 3600;
			}*/

		}
		ASSERT(poly != -1);
		ASSERT(bRep);//we assume we foun the zone. Update the Shapefile
		
		return bRep;
	}
	//cctz::time_zone GetZone(const CGeoPoint& pt);
	//static cctz::time_point<cctz::sys_seconds> GetTimePoint(CTRef TRef, const cctz::time_zone& zone);


	__int64 CTimeZones::UTCTRef2UTCTime(CTRef TRef)
	{
		ASSERT(TRef.GetTM().Type() == CTM::HOURLY);

		cctz::time_point<cctz::sys_seconds> tp = GetTimePoint(TRef, cctz::utc_time_zone());
		return tp.time_since_epoch().count();
	}

	CTRef CTimeZones::UTCTime2UTCTRef(__int64 time)
	{
		const auto tp = std::chrono::system_clock::from_time_t(time);
		const cctz::time_zone::absolute_lookup al = cctz::utc_time_zone().lookup(tp);
		return CTRef (al.cs.year(), al.cs.month()-1, al.cs.day()-1, al.cs.hour());
	}

	__int64 CTimeZones::LocalTRef2LocalTime(CTRef TRef, const CGeoPoint& pt)
	{
		ASSERT(TRef.GetTM().Type() == CTM::HOURLY);

		CTRef UTCTRef = LocalTRef2UTCTRef(TRef, pt);
		__int64 UTCTime = UTCTRef2UTCTime(UTCTRef);
		__int64 t = UTCTime2LocalTime(UTCTime, pt);
		
		/*cctz::time_zone zone = GetZone(pt);
		cctz::time_point<cctz::sys_seconds> tp = GetTimePoint(TRef, zone);
		__int64 tt = tp.time_since_epoch().count();
		ASSERT(t==tt);*/

		return t;
	}
	
	CTRef CTimeZones::LocalTime2LocalTRef(__int64 time, const CGeoPoint& pt)
	{
		__int64 UTCTime = LocalTime2UTCTime(time, pt);
		CTRef UTCTRef = UTCTime2UTCTRef(UTCTime);
		CTRef TRef = UTCTRef2LocalTRef(UTCTRef, pt);

		return TRef;
		
	}


	__int64 CTimeZones::GetDelta(CTRef TRef, const CGeoPoint& pt)
	{
		__int64 delta = __int64(pt.m_lon / 15 * 3600);

		cctz::time_zone zone;
		if (GetZone(pt, zone))
		{
			const auto tp = cctz::convert(cctz::civil_second(TRef.GetYear(), int(TRef.GetMonth() + 1), int(TRef.GetDay() + 1), int(TRef.GetHour()), 0, 0), zone);
			const cctz::time_zone::absolute_lookup al = zone.lookup(tp);
			delta = al.offset;
		}

			//	{
			//		std::string str_zone = record[7];
			//		delta = WBSF::ToInt(str_zone);
			//		ASSERT(delta >= -12 && delta<=12);
			//	}


		//int poly = -1;
		//if (TIME_ZONE.IsInside(pt, &poly))
		//{
		//	const CDBF3& DBF = TIME_ZONE.GetDBF();
		//	const CDBFRecord& record = DBF[poly];
		//	ASSERT(record.size() >= 14);
		//	std::string str_zone = record[13];

		//	cctz::time_zone zone;
		//	if (!str_zone.empty() && cctz::load_time_zone(str_zone, &zone))
		//	{
		//		
		//		const auto tp = cctz::convert(cctz::civil_second(TRef.GetYear(), int(TRef.GetMonth() + 1), int(TRef.GetDay() + 1), int(TRef.GetHour()), 0, 0), zone);
		//		const cctz::time_zone::absolute_lookup al = zone.lookup(tp);

		//		__int64 delta_s = al.offset;
		//		delta = Round(delta_s / 3600);
		//	}
		//	else
		//	{
		//		std::string str_zone = record[7];
		//		delta = WBSF::ToInt(str_zone);
		//		ASSERT(delta >= -12 && delta<=12);
		//	}
		//}

		return delta;
	}

	__int64 CTimeZones::GetDelta(__int64 time, const CGeoPoint& pt)
	{
		__int64 delta = __int64(pt.m_lon / 15 * 3600);
		cctz::time_zone zone;
		if (GetZone(pt, zone))
		{
			const auto tp = std::chrono::system_clock::from_time_t(time);
			const cctz::time_zone::absolute_lookup al = zone.lookup(tp);

			delta = al.offset;
		}

		/*int poly = -1;
		if (TIME_ZONE.IsInside(pt, &poly))
		{
			const CDBF3& DBF = TIME_ZONE.GetDBF();
			const CDBFRecord& record = DBF[poly];
			ASSERT(record.size() >= 14);
			std::string str_zone = record[13];


			cctz::time_zone zone;
			if (!str_zone.empty() && cctz::load_time_zone(str_zone, &zone, delta))
			{
				const auto tp = std::chrono::system_clock::from_time_t(time);
				const cctz::time_zone::absolute_lookup al = zone.lookup(tp);

				delta = al.offset;
			}
			else
			{
				std::string str_zone = record[7];
				delta = WBSF::ToInt(str_zone);
				ASSERT(delta >= -12 && delta <= 12);
			}
		}*/

		return delta;
	}
	
	__int64 CTimeZones::LocalTime2UTCTime(__int64 time, const CGeoPoint& pt)
	{
		__int64 delta = GetDelta(time, pt);
		return time - delta;
	}
	__int64 CTimeZones::UTCTime2LocalTime(__int64 time, const CGeoPoint& pt)
	{
		__int64 delta = GetDelta(time, pt);
		return time + delta;
	}

	CTRef CTimeZones::LocalTRef2UTCTRef(CTRef TRef, const CGeoPoint& pt)
	{
		__int64 delta = GetDelta(TRef, pt);
		return TRef - int(delta / 3600);
	}

	CTRef CTimeZones::UTCTRef2LocalTRef(CTRef TRef, const CGeoPoint& pt)
	{
		__int64 delta = GetDelta(TRef, pt);
		return TRef + int(delta/3600);
		
	}

	CTRef CTimeZones::LocalTRef2UTCTRef(CTRef TRef, const cctz::time_zone& zone)
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

	}

	
	

}