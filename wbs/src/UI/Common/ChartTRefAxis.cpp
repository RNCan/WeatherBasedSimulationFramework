//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// from Cédric Moonen(cedric_moonen@hotmail.com) code
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//****************************************************************************
#include "stdafx.h"
#include <sstream>
#include <iostream>
#include <iomanip>
#include <math.h>

#include "ChartCtrl/ChartCtrl.h"
#include "Basic/UtilStd.h"
#include "UI/Common/ChartTRefAxis.h"

using namespace std;
using namespace WBSF;

CChartTRefAxis::CChartTRefAxis()
 : CChartAxis(), m_strDTTickFormat(), 
   m_bAutoTickFormat(true), m_BaseInterval(tiDay), 
   m_iDTTickIntervalMult(1), m_dFirstTickValue(0)
{
	m_ReferenceTick = CTRef(2000, 0, 0);
}

CChartTRefAxis::~CChartTRefAxis()
{
}


void CChartTRefAxis::SetTickIncrement(bool bAuto, 
										  TimeInterval Interval, 
										  int Multiplier)
{
	m_bAutoTicks = bAuto;
	if (!m_bAutoTicks)
	{
		m_BaseInterval = Interval;
		m_iDTTickIntervalMult = Multiplier;
	}
}

void CChartTRefAxis::SetTickLabelFormat(bool bAutomatic, 
											const TChartString& strFormat)
{
	m_bAutoTickFormat = bAutomatic;
	m_strDTTickFormat = WBSF::UTF8(strFormat);
	m_pParentCtrl->RefreshCtrl();
}

double CChartTRefAxis::GetFirstTickValue() const
{
	double dRetVal = m_dFirstTickValue;

	return dRetVal; 

}

bool CChartTRefAxis::GetNextTickValue(double dCurrentTick, double& dNextTick) const
{
	if (m_MinValue == m_MaxValue)
		return false;
	
	CTRef dtTick;
	dtTick.SetRef((int)dCurrentTick, m_ReferenceTick.GetTM());
	dtTick += m_iDTTickIntervalMult;

	//if (m_ReferenceTick.GetType() == CTRef::HOURLY && m_BaseInterval != tiHour)
	//{
	//	
	//	if (m_BaseInterval == tiDay)
	//	{
	//		dtTick.Transform(CTM(CTM::DAILY));
	//		dtTick += m_iDTTickIntervalMult;
	//		dtTick.Transform(CTM(CTM::HOURLY));

	//		ASSERT(dtTick.IsValid());
	//	}
	//	else if (m_BaseInterval == tiMonth)
	//	{
	//		dtTick.Transform(CTM(CTM::MONTHLY));
	//		dtTick += m_iDTTickIntervalMult;
	//		dtTick.Transform(CTM(CTM::HOURLY));
	//		ASSERT(dtTick.IsValid());
	//	}
	//	else
	//	{
	//		//annual
	//		int nbYears = int(m_iDTTickIntervalMult / 365);
	//		dtTick.m_year = dtTick.GetYear() + nbYears;
	//	}
	//}
	//else if( m_ReferenceTick.GetType()==CTRef::DAILY && m_BaseInterval != tiDay )
	//{
	//	
	//	if( m_BaseInterval == tiMonth )
	//	{
	//		int month = dtTick.GetMonth() + m_iDTTickIntervalMult/30;
	//		int year = dtTick.GetYear() + month/12;
	//		month = month%12;
	//		dtTick.m_year = year;
	//		dtTick.m_month = month;
	//		//dtTick.m_day = 0;
	//		int day = __min( dtTick.GetDay(), dtTick.GetNbDayPerMonth(year, month)-1 );
	//		dtTick.m_day = day;
	//		ASSERT( dtTick.IsValid() );
	//		
	//	}
	//	else
	//	{
	//		//annual
	//		int nbYears = int(m_iDTTickIntervalMult/365);
	//		dtTick.m_year = dtTick.GetYear()+nbYears;
	//	}
	//}
	//else
	//{
	//	dtTick += m_iDTTickIntervalMult;
	//}

	dNextTick = dtTick.GetRef();

	if (dNextTick <= m_MaxValue)
		return true;
	else
		return false;
}

TChartString CChartTRefAxis::GetTickLabel(double TickValue) const
{
	CTRef tickTime;
	tickTime.SetRef((int)TickValue, m_ReferenceTick.GetTM());
	
	std::string tmp = tickTime.GetFormatedString(m_strDTTickFormat);
	return TChartString(CString(tmp.c_str()));
	
	//return strLabel;
}

long CChartTRefAxis::ValueToScreenDiscrete(double dValue) const
{
	// In discrete mode, all values between two ticks relates
	// to the middle of the interval (there's no other values than
	// the tick values).
	double tickAfter;
	double tickBefore = GetTickBeforeVal(dValue);
	GetNextTickValue(tickBefore, tickAfter);

	long tickPosBefore = ValueToScreenStandard(tickBefore);
	long tickPosAfter = ValueToScreenStandard(tickAfter);
	return tickPosBefore + (tickPosAfter-tickPosBefore)/2;
}

long CChartTRefAxis::GetTickPos(double TickVal) const
{
	// The tick is always at the same position,
	// even if the axis is discrete.
	return ValueToScreenStandard(TickVal);
}

//COleDateTime CChartTRefAxis::AddMonthToDate(const COleDateTime& Date, 
//												int iMonthsToAdd) const
//{
//	COleDateTime newDate;
//	int nMonths = Date.GetMonth()-1 + iMonthsToAdd;
//	int nYear = Date.GetYear() + nMonths/12;;
//	// We can 'add' a negative number of months
//	if (nMonths<0)
//	{
//		nYear = Date.GetYear() - (-nMonths)/12;
//		nMonths += (-nMonths)/12 * 12;
//	}
//
//	newDate.SetDateTime(nYear,nMonths%12+1,Date.GetDay(),Date.GetHour(),
//		Date.GetMinute(),Date.GetSecond());
//	return newDate;
//}

void CChartTRefAxis::RefreshTickIncrement()
{
	if (!m_bAutoTicks)
		return;

	if (m_MaxValue == m_MinValue)
	{
		m_iDTTickIntervalMult = 1;
		return;
	}

	int PixelSpace;
	if (m_bIsHorizontal)
		PixelSpace = 120;
	else
		PixelSpace = 20;

	int MaxTickNumber = max(1, (int)fabs((m_EndPos-m_StartPos)/PixelSpace * 1.0));
	
	CTRef StartDate;
	StartDate.SetRef((int)m_MinValue, m_ReferenceTick.GetTM());

	CTRef EndDate;
	EndDate.SetRef((int)m_MaxValue, m_ReferenceTick.GetTM());

	ASSERT( StartDate.GetTM() == EndDate.GetTM() );

	int minTickInterval = max(1, (EndDate - StartDate) / MaxTickNumber);
	
	if( StartDate.GetType() == CTRef::HOURLY )
	{
		m_BaseInterval = tiHour;

		if (minTickInterval > 6*30*24)
		{
			//m_BaseInterval = tiYear;
			m_iDTTickIntervalMult = (6 * 30 * 24)*((int)minTickInterval / (6 * 30 * 24) + 1);
		}
		else if (minTickInterval > 15*24)
		{
			//m_BaseInterval = tiMonth;
			m_iDTTickIntervalMult = (30 * 24)*((int)minTickInterval / (30 * 24) + 1);
		}
		else if (minTickInterval > 3.5 * 24)
		{
			m_iDTTickIntervalMult = (7 * 24)*((int)minTickInterval / (7 * 24) + 1);
		}
		else if(minTickInterval > 12)
		{
			//m_BaseInterval = tiDay;
			m_iDTTickIntervalMult = 24*((int)minTickInterval/24 + 1);
		}
		else
		{
			//m_BaseInterval = tiHour;
			m_iDTTickIntervalMult = minTickInterval;
		}
	}
	else if( StartDate.GetType() == CTRef::DAILY )
	{
		if (minTickInterval > 6 * 30)
		{
			m_BaseInterval = tiYear;
			m_iDTTickIntervalMult = 365*(int(minTickInterval/365) + 1);
		}
		else if (minTickInterval > 15)
		{
			m_BaseInterval = tiMonth;
			m_iDTTickIntervalMult = 30*(int(minTickInterval/30) + 1);
		}
		else
		{
			m_BaseInterval = tiDay;
			m_iDTTickIntervalMult = minTickInterval+1;
		}
	}
	else if( StartDate.GetType() == CTRef::MONTHLY )
	{
		if (minTickInterval > 12)
		{
			m_BaseInterval = tiYear;
			m_iDTTickIntervalMult = (int)minTickInterval/12 + 1;
		}
		else
		{
			m_BaseInterval = tiMonth;
			m_iDTTickIntervalMult = minTickInterval+1;
		}

	}
	else //if( StartDate.GetType() == CTRef::ANNUAL )
	{
		m_BaseInterval = tiYear;
		m_iDTTickIntervalMult = minTickInterval + 1;
	}
}

void CChartTRefAxis::RefreshFirstTick()
{
	m_dFirstTickValue = GetTickBeforeVal(m_MinValue);
	if (m_bAutoTickFormat)
		RefreshDTTickFormat();
}

void CChartTRefAxis::SetReferenceTick(CTRef referenceTick)
{
	m_ReferenceTick = referenceTick;
	m_pParentCtrl->RefreshCtrl();
}

void CChartTRefAxis::RefreshDTTickFormat()
{
	CTRefFormat format = CTRef::GetFormat();
	int type = CTM::ATEMPORAL;
	switch (m_BaseInterval)
	{
	case tiHour:type = CTM::HOURLY; break;
	case tiDay:type = CTM::DAILY; break;
	case tiMonth:type = CTM::MONTHLY; break;
	case tiYear:type = CTM::ANNUAL; break;
	default: ASSERT(false);
	}

	int mode = m_ReferenceTick.GetMode();
	//m_strDTTickFormat = stdString::UTF16(format.GetFormat(CTM(type, mode)));
	m_strDTTickFormat = format.GetFormat(CTM(type, mode));
}

double CChartTRefAxis::GetTickBeforeVal(double dValue) const
{
	CTRef tickBefore = m_ReferenceTick;
	CTRef valueTime;
	valueTime.SetRef((int)dValue, m_ReferenceTick.GetTM());
	long dtSpan = valueTime - m_ReferenceTick;
	long nbRef = int(((dtSpan)/m_iDTTickIntervalMult) * m_iDTTickIntervalMult);
	tickBefore = valueTime - nbRef;

	return tickBefore.GetRef();
}
