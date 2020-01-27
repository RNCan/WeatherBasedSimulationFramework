//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include <memory>
#include <unordered_map>
#include "basic/ERMsg.h"
#include "Basic/UtilStd.h"
#include "Basic/UtilTime.h"
#include "Basic/GeoBasic.h"
#include "Geomatic/SpatialRegression.h"

namespace cctz{ class time_zone; }


namespace WBSF
{
	class CTimeZones
	{
	public:

		CTimeZones()
		{}


		~CTimeZones()
		{}

		static ERMsg Load(std::string& filePath);

		static __int64 TRef2Time(CTRef TRef);
		static CTRef Time2TRef(__int64 time);
		static __int64 GetTime0(__int64 time);

		static double GetDecimalHour(__int64 time);
		static __int64 UTCTRef2UTCTime(CTRef TRef) { return TRef2Time(TRef); }
		static CTRef UTCTime2UTCTRef(__int64 time) { return Time2TRef(time); }

		static __int64 LocalTRef2LocalTime(CTRef TRef) { return TRef2Time(TRef); }
		static CTRef LocalTime2LocalTRef(__int64 time) { return Time2TRef(time); }
		
//		static double GetDecimalHour(__int64 time);
		//static __int64 GetDelta(CTRef TRef, const CGeoPoint& pt);
		//static __int64 GetDelta(__int64 time, const CGeoPoint& pt);

		static __int64 LocalTime2UTCTime(__int64  time, const CGeoPoint& pt);
		static __int64 UTCTime2LocalTime(__int64  time, const CGeoPoint& pt);
		static CTRef LocalTRef2UTCTRef(CTRef TRef, const CGeoPoint& pt);
		static CTRef UTCTRef2LocalTRef(CTRef TRef, const CGeoPoint& pt);
	
		static std::string GetZoneName(const CGeoPoint& pt);
		static __int64 GetTimeZone(const CGeoPoint& pt);
		//static bool GetZone(const CGeoPoint& pt, cctz::time_zone& zone);
		//static CTRef LocalTRef2UTCTRef(CTRef TRef, const cctz::time_zone& zone);
		//static CTRef UTCTRef2LocalTRef(CTRef TRef, const cctz::time_zone& zone);


	};

};