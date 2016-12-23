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

		static __int64 UTCTRef2UTCTime(CTRef TRef);
		static CTRef UTCTime2UTCTRef(__int64 time);

		static __int64 LocalTRef2LocalTime(CTRef TRef, const CGeoPoint& pt);
		static CTRef LocalTime2LocalTRef(__int64 time, const CGeoPoint& pt);
		
		static double GetDecimalHour(__int64 time);
		static __int64 GetDelta(CTRef TRef, const CGeoPoint& pt);
		static __int64 GetDelta(__int64 time, const CGeoPoint& pt);

		static __int64 LocalTime2UTCTime(__int64  time, const CGeoPoint& pt);
		static __int64 UTCTime2LocalTime(__int64  time, const CGeoPoint& pt);
		static CTRef LocalTRef2UTCTRef(CTRef TRef, const CGeoPoint& pt);
		static CTRef UTCTRef2LocalTRef(CTRef TRef, const CGeoPoint& pt);
	
		static std::string CTimeZones::GetZoneName(const CGeoPoint& pt);
		static bool GetZone(const CGeoPoint& pt, cctz::time_zone& zone);
		static CTRef LocalTRef2UTCTRef(CTRef TRef, const cctz::time_zone& zone);
		static CTRef UTCTRef2LocalTRef(CTRef TRef, const cctz::time_zone& zone);

		//static cctz::time_point<cctz::sys_seconds> GetTimePoint(CTRef TRef, const cctz::time_zone& zone)

		//void Enter()
		//{
		//	m_CS.Enter();
		//}
		//void Leave()
		//{
		//	m_CS.Leave();
		//}

	protected:

		//CCriticalSection m_CS;

	};

};