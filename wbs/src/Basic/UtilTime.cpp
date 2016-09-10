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
#include <math.h>
#include <sstream>
#include <iostream>
#include <boost/date_time.hpp>

#include "Basic/UtilTime.h"
#include "Basic/UtilStd.h"
#include "Basic/Statistic.h"
#include "Basic/UtilMath.h"

#include "WeatherBasedSimulationString.h"


using namespace std;

const locale TIME_FORMATS[] =
{
	locale(locale::classic(), new boost::posix_time::time_input_facet("%Y-%m-%d %H:%M:%S")),
	locale(locale::classic(), new boost::posix_time::time_input_facet("%Y/%m/%d %H:%M:%S")),
	locale(locale::classic(), new boost::posix_time::time_input_facet("%Y-%m-%d-%H")),
	locale(locale::classic(), new boost::posix_time::time_input_facet("%Y/%m/%d/%H")),
	locale(locale::classic(), new boost::posix_time::time_input_facet("%Y-%m-%d")),
	locale(locale::classic(), new boost::posix_time::time_input_facet("%Y-%m")),
	locale(locale::classic(), new boost::posix_time::time_input_facet("%Y"))
};


namespace WBSF
{

static const char* MONTH_NAME[2][12] =
{
	{"JANUARY", "FEBRUARY", "MARCH", "APRIL", "MAY", "JUNE", "JULY", "AUGUST", "SEPTEMBER", "OCTOBER", "NOVEMBER", "DECEMBER"},
	{"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"}
};

const char* GetMonthName(size_t m, bool bFull)
{
	_ASSERTE(m>=0 && m<12);
	return MONTH_NAME[bFull?0:1][m];
}

static StringVector MONTH_TITLE_LONG;
static StringVector MONTH_TITLE_SHORT;
void ReloadString()
{
	MONTH_TITLE_LONG.LoadString(IDS_STR_MONTH_LONG, "|;");
	MONTH_TITLE_SHORT.LoadString(IDS_STR_MONTH_SHORT, "|;");
}

const char* GetMonthTitle(size_t m, bool bFull)
{
	assert(m<12);
	if (MONTH_TITLE_LONG.empty())
		ReloadString();

	assert(MONTH_TITLE_LONG.size() == 12);
	assert(MONTH_TITLE_SHORT.size() == 12);

	return bFull ? MONTH_TITLE_LONG[m].c_str() : MONTH_TITLE_SHORT[m].c_str();
}


size_t GetMonthIndex(const char* month)
{
	size_t index = UNKNOWN_POS;
	string str(month);
	transform(str.begin(), str.end(), str.begin(), ::toupper);

	for(int i=0; i<2&&index==-1; i++)
	{
		for(int j=0; j<12&&index==-1; j++)
		{
			if( str == MONTH_NAME[i][j] )
				index=j;
		}
	}

	return index;
}


size_t GetJDay(int year, size_t m, size_t d)
{
	_ASSERTE( year >= YEAR_NOT_INIT && year <= 2100);
	_ASSERTE( m < 12);
	if( d == LAST_DAY)
		d = GetNbDayPerMonth(year, m)-1;
	
	_ASSERTE(d<GetNbDayPerMonth(year, m));
	
	size_t jd = 0;

	for(size_t mm=0; mm<m; mm++)
	{
		jd += GetNbDayPerMonth(year, mm);
	}
	
	jd += d;
	
	_ASSERT(jd < 366);
	
	return jd;
}

//short GetjDay(int year, size_t m, size_t d)
//{
//	return GetJDay(year, month, day);
//}


size_t GetDayOfTheMonth(int year, size_t jd)
{
	_ASSERTE(jd < 366);
	
	size_t d = jd;

	for(size_t m=0; m<12; m++ )
	{
		if (d >= GetNbDayPerMonth(year, m))
			d -= GetNbDayPerMonth(year, m);
		else break;
	}
	
	_ASSERTE( d < 31 );

	return d;
}

size_t GetMonthIndex(int year, size_t jd)
{
	_ASSERTE(jd < 366);

	const size_t* pFirst = IsLeap(year)?FIRST_DAY_LEAP_INDEX:FIRST_DAY_INDEX;
	
	size_t m = 0;
	for(int i=11; i>=0; i--)
	{
		if (jd >= pFirst[i])
		{
			m = i;
			break;
		}
	}

	_ASSERTE(m < 12);
	return m;
}


//latDeg : latitude in degree
//d: julian day of the year
// daylength (seconds) 
//double GetDayLength(double latDeg, short d)
double GetDayLength(double latDeg, CTRef date)
{
	size_t jd = date.GetJDay();
	_ASSERTE( latDeg >=-90 && latDeg <= 90);
	_ASSERTE( jd < 366);
	static const double SECPERRAD =13750.9871;     // seconds per radian of hour angle 
	static const double RADPERDAY =0.017214;       // radians of Earth orbit per julian day 
	static const double RADPERDEG =0.01745329;     // radians per degree 
	static const double MINDECL =-0.4092797;       // minimum declination (radians) 
	static const double DAYSOFF =11.25;            // julian day offset of winter solstice 


	// precalculate the transcendentals 
	// check for (+/-) 90 degrees latitude, throws off daylength calc 
	double lat = latDeg*RADPERDEG;
	if (lat > 1.5707) lat = 1.5707;
	if (lat < -1.5707) lat = -1.5707;
	double coslat = cos(lat);
	double sinlat = sin(lat);

// calculate cos and sin of declination 
	double decl = MINDECL * cos(((double)jd + DAYSOFF) * RADPERDAY);
	double cosdecl = cos(decl);
	double sindecl = sin(decl);
	
	// calculate daylength as a function of lat and decl 
	double cosegeom = coslat * cosdecl;
	double sinegeom = sinlat * sindecl;
	double coshss = -(sinegeom) / cosegeom;
	if (coshss < -1.0) coshss = -1.0;  // 24-hr daylight 
	if (coshss > 1.0) coshss = 1.0;    // 0-hr daylight 
	double hss = acos(coshss);                // hour angle at sunset (radians) 
	// daylength (seconds) 
	double daylength = 2.0 * hss * SECPERRAD;

	return daylength;
}

//
//2.15. What is the Julian Period?
//The Julian period (and the Julian day number) must not be confused
//with the Julian calendar. 
//
//The French scholar Joseph Justus Scaliger (1540-1609) was interested
//in assigning a positive number to every year without having to worry
//about BC/AD. He invented what is today known as the "Julian Period".
//
//The Julian Period probably takes its name from the Julian calendar,
//although it has been claimed that it is named after Scaliger's father,
//the Italian scholar Julius Caesar Scaliger (1484-1558).
//
//Scaliger's Julian period starts on 1 January 4713 BC (Julian calendar)
//and lasts for 7980 years. AD 2003 is thus year 6716 in the Julian
//period. After 7980 years the number starts from 1 again.
//
//Why 4713 BC and why 7980 years? Well, in 4713 BC the Indiction (see
//section 2.14), the Golden Number (see section 2.12.3) and the Solar
//Number (see section 2.4) were all 1. The next times this happens is
//15*19*28=7980 years later, in AD 3268.
//
//Astronomers have used the Julian period to assign a unique number to
//every day since 1 January 4713 BC. This is the so-called Julian Day
//(JD). JD 0 designates the 24 hours from noon UTC on 1 January 4713 BC
//to noon UTC on 2 January 4713 BC.
//
//This means that at noon UTC on 1 January AD 2000, JD 2,451,545
//started.
//
//This can be calculated thus:
//From 4713 BC to AD 2000 there are 6712 years.
//In the Julian calendar, years have 365.25 days, so 6712 years
//correspond to 6712*365.25=2,451,558 days. Subtract from this
//the 13 days that the Gregorian calendar is ahead of the Julian
//calendar, and you get 2,451,545.
//
//Often fractions of Julian day numbers are used, so that 1 January AD
//2000 at 15:00 UTC is referred to as JD 2,451,545.125.
//
//Note that some people use the term "Julian day number" to refer to any
//numbering of days. NASA, for example, uses the term to denote the
//number of days since 1 January of the current year.


//GetRef return the number of day from the 1 december 1
//minus year 0, year aren't leap and return negative value
__int32 GetRef(int year, size_t m, size_t d)
{
	ASSERT( year>=YEAR_NOT_INIT);
	ASSERT(m < 12);
	ASSERT(d < 31);
	
	//For a date in the Gregorian calendar:
	int ref = 0;


	int _a = int((13 - m) / 12);
	int _y = year - _a;
	int _m = int(m + 12 * _a - 2);

	
	
	//For a date in the Julian calendar:
	if( year > 0)//no leap year under zero
		ref = __int32(d + (153 * _m + 2) / 5 + _y * 365 + _y / 4 - _y / 100 + _y / 400 - 306);
	else 
		ref = __int32(d + (153 * _m + 2) / 5 + _y * 365 - 306);
	//else JD = int(day + (153*m+2)/5 + y*365 + y/4 - 32083);

	return ref;

	
}

//JD is the Julian day number that starts at noon UTC on the specified
//date.
//
//The algorithm works fine for AD dates. If you want to use it for BC
//dates, you must first convert the BC year to a negative year (e.g.,
//10 BC = -9). The algorithm works correctly for all dates after 4800 BC,
//i.e. at least for all positive Julian day numbers.
//
//To convert the other way (i.e., to convert a Julian day number, JD,
//to a day, month, and year) these formulas can be used (again, the
//divisions are integer divisions):
void SetRef(__int32 ref, int& year, size_t& m, size_t& d)
{
	if( ref >= 0)
	{
		//  For the Gregorian calendar:
		int _a = ref + 306;//32044;
		int _b = (4 * _a + 3) / 146097;
		int _c = _a - (_b * 146097) / 4;

		int _d = (4 * _c + 3) / 1461;
		int _e = _c - (1461 * _d) / 4;
		int _m = (5 * _e + 2) / 153;
	  
		d = _e - (153 * _m + 2) / 5; //+ 1
		m = _m + 2 - 12 * (_m / 10);
		year = _b * 100 + _d + _m / 10;
	}
	else
	{
		//JD=-JD;
		int _y = -(ref + 1) / 365;
		int _f = ref + _y * 365;
		int _e = _f + 306 - (_f / 307) * 365;
		int _m = (5 * _e + 2) / 153;
	  
		d = _e - (153 * _m + 2) / 5;
		m = _m + 2 - 12 * (_m / 10);
		year = -_y;
	}
}



string GetTimeSpanStr(double sec)
{
	string str;

	int day = int(sec/(24*60*60));
	int hour = int((sec-day*24*60*60)/(60*60));
	int minute = int((sec-day*24*60*60-hour*60*60)/(60));
	double second = (sec-day*24*60*60-hour*60*60-minute*60);
	
	if(day>0)
	{
		str += ToString(day) + " day";
		if(day>1)
			str += "s";
	}
		
	if(hour>0||day>0)
	{
		str += ToString(hour) + " hour";
		if(hour>1)
			str += "s";
	}
		
	if(minute>0||hour>0||day>0)
	{
		str += ToString(minute) + " minute";
		if(minute>0)
			str += "s";
	}
	
	str += ToString(second,2) + " second";
	if(second>1)
		str += "s";


	return str;
}



string CTRefFormat::GetMissingString(CTM tm)const
{
	string str = GetFormat(tm);
	transform(str.begin(), str.end(), str.begin(), ::toupper);
	ReplaceString(str, "#", "");
	ReplaceString(str, "%%", "");
	
	static const char* ALL_SYMBOL[15] = {"%A","%B","%C","%D","%H","%I","%J","%M","%P","%S","%U","%W","%X","%Y","%Z"};
	for(int i=0; i<15; i++)
		ReplaceString(str, ALL_SYMBOL[i], "??");

	return str;
}


const char* CTRefFormat::DEFAULT_FORMAT[CTM::NB_REFERENCE][CTM::NB_MODE] =
{ { "%Y", " " },
{ "%Y-%m", "%m" },
{ "%Y-%m-%d", "%m-%d" },
{ "%Y-%m-%d-%H", "%m-%d-%H" },
{ " ", " " }
};
const char* CTRefFormat::DEFAULT_HEADER[CTM::NB_REFERENCE][CTM::NB_MODE] =
{ { "Year", "" },
{ "Year-Month", "Month" },
{ "Year-Month-Day", "Month-Day" },
{ "Year-Month-Day-Hour", "Month-Day-Hour" },
{ " ", " " }
};

const char* CTRefFormat::DATE_YMD = "%Y-%m-%d";
const char* CTRefFormat::DATE_YMDH = "%Y-%m-%d-%H";
const char* CTRefFormat::DATE_YMD_HMS = "%Y-%m-%d %H:%M:%S";
//const char* CTRefFormat::DATE_DMY = "%d-%m-%Y";
//const char* CTRefFormat::DATE_MD = "%m-%d";
//const char* CTRefFormat::DATE_DM = "%d-%m";

//"DateYMD", "DateDMY", "DateMD", "DateDM"
const char* CTRefFormat::FORMAT_NAME[NB_FORMAT] = { "Year", "Month", "Day", "JDay", "Hour", "Reference" };




//********************************************************************
//CTM


const char CTM::NB_REFERENCES[8][NB_MODE] = 
{ 
	{ 1, 0 },
	{ 2, 1 },
	{ 3, 2 },
	{ 4, 3 },
	{ 1, 1 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
};

const char* CTM::TYPE_NAME[8] = { "Annual", "Monthly", "Daily", "Hourly", "Atemporal", "", "", "Unknown" };
const char* CTM::MODE_NAME[NB_MODE] = { "For each year", "Overall years" };
const char* CTM::TYPE_MODE_NAME[8][NB_MODE] =
{
	{ "Annual (For each year)", "Annual (Overall years)" },
	{ "Monthly (For each year)", "Monthly (Overall years)" },
	{ "Daily (For each year)", "Daily (Overall years)" },
	{ "Hourly (For each year)", "Hourly (Overall years)" },
	{ "Atemporal(1)", "Atemporal(2)" },
	{ "", "" },
	{ "", "" },
	{ "Unknown", "Unknown" }
};

const char* CTM::GetTypeName(size_t TType)
{
	assert(TType >= 0 && TType < 8);
	return TYPE_NAME[TType];
}


const char* CTM::GetModeName(size_t TMode)
{
	assert(TMode >= 0 && TMode<NB_MODE);
	return MODE_NAME[TMode];
}

StringVector CTM::TYPE_TITLE;
StringVector CTM::MODE_TITLE;
void CTM::ReloadString() 
{
	TYPE_TITLE.clear();
	MODE_TITLE.clear();
}

const char* CTM::GetTypeTitle(size_t TType)
{ 
	ASSERT(TType >= 0 && TType<8); 
	if (TYPE_TITLE.empty())
		TYPE_TITLE.LoadString(IDS_STR_TM_TYPE, "|;");

	return TYPE_TITLE.empty() ? TYPE_NAME[TType] : TYPE_TITLE[TType].c_str();
}

const char* CTM::GetModeTitle(size_t TMode)
{ 
	ASSERT(TMode >= 0 && TMode<NB_MODE); 
	if (MODE_TITLE.empty())
		MODE_TITLE.LoadString(IDS_STR_TM_MODE, "|;");

	return MODE_TITLE.empty() ? MODE_NAME[TMode] : MODE_TITLE[TMode].c_str();
}

const bool CTM::AVAILABILITY[NB_REFERENCE*NB_MODE][NB_REFERENCE*NB_MODE] =
{//  AF MF DF HF UF AO MO DO HO UO
	{ 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 },//AF = ANNUAL FOR_EACH_YEAR
	{ 1, 0, 0, 0, 0, 1, 1, 0, 0, 0 },//MF = MONTHLY FOR_EACH_YEAR
	{ 1, 1, 0, 0, 0, 1, 1, 1, 0, 0 },//DF = DAILY FOR_EACH_YEAR
	{ 1, 1, 1, 0, 0, 1, 1, 1, 1, 0 },//HF = HOURLY FOR_EACH_YEAR
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },//UF = ATEMPORAL FOR_EACH_YEAR
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },//AO = ANNUAL OVERALL_YEARS
	{ 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 },//MO = MONTHLY OVERALL_YEARS
	{ 0, 0, 0, 0, 0, 1, 1, 0, 0, 0 },//DO = DAILY OVERALL_YEARS
	{ 0, 0, 0, 0, 0, 1, 1, 1, 0, 0 },//HO = HOURLY OVERALL_YEARS
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },//UO = ATEMPORAL OVERALL_YEARS
};

//********************************************************************
//CTRef

CTRefFormat CTRef::TREF_FORMAT;

CTRef::CTRef()
{
	Reset();
}

//mettre la valeur our year = YEAR_NOT_INIT: a voir
CTRef::CTRef(float in)
{
	Reset();
	operator =(in);
}
//CTRef::CTRef(__int32 in)
//{
//	Reset();
//	operator =(in);
//}
//
//CTRef::CTRef(__int32 in)
//{
//	Reset();
//	operator =(in);
//}
//
CTRef::CTRef(double in)
{
	operator =(in);
}
//
//CTRef::CTRef(unsigned __int64)
//{
//	ASSERT(false); //que faire de cela???
//}

CTRef::CTRef(int year)
{
	Set(year,0,0,0,CTM(CTM::ANNUAL));
}

CTRef::CTRef(int year, size_t m)
{
	Set(year, m, 0, 0, CTM(CTM::MONTHLY));
}

CTRef::CTRef(int year, size_t m, size_t d)
{
	Set(year, m, d, 0, CTM(CTM::DAILY) );
}

CTRef::CTRef(int year, size_t m, size_t d, size_t h, CTM TM)
{
	Set(year, m, d, h, TM);
}

//CTRef::CTRef(short t, short y_or_r, short m, short d, short h, short mode)
//{
//	Set(t, y_or_r, m, d, h, mode);
//}

CTRef::CTRef(const string& str)
{
	FromString(str);
}

CTRef::CTRef(const string& strIn, CTM TM)
{
	ASSERT(TM.Type() != UNKNOWN);

	string str(strIn);
	Trim(str);
	if (!str.empty() && str[0] == '-')
		str[0] = '*';

	StringVector tmp = Tokenize(str, "/-");
	if (!tmp.empty() && !tmp[0].empty() && tmp[0][0] == '*')
		tmp[0][0] = '-';

	int p[4] = {0,0,0,0};
	//int pos=0;
	for(size_t i=0; i<min((size_t)4,tmp.size()); i++)
		p[i] = ToValue<int>(Trim(tmp[i]));

	if (TM.Type() == ATEMPORAL || TM.Mode() == FOR_EACH_YEAR)
	{
		Set(p[0], p[1] - 1, p[2] - 1, p[3], TM);
	}
	else 
	{

		Set(0, p[0] - 1, p[1] - 1, p[2], TM);
	}
}


CTRef& CTRef::Set(int y_or_r, size_t m, size_t d, size_t h, CTM TM)
{

	ASSERT(m<12 || m==LAST_MONTH);
	ASSERT(d<31 || d==LAST_DAY || d==DAY_NOT_INIT);
	ASSERT(h==NOT_INIT || h<=24);

	bool bAddRef=false;//faudrait vérifier cela, c'est très bizard...
	if( h==24 )
	{
		h=0;
		bAddRef=true;
	}

	Reset();

	m_type = TM.Type();
	m_mode = TM.Mode();
	if (m_type == ATEMPORAL)
	{
		m_ref=y_or_r;
	}
	else
	{
		if( d == LAST_DAY)
			d = GetNbDayPerMonth(y_or_r, m)-1;

		m_year=y_or_r; 
		m_month=m;
		m_day=d;    
		m_hour=h;
	}

	if (m_year == YEAR_NOT_INIT)
		m_mode = CTM::OVERALL_YEARS;

	ResetUnused();

	if( bAddRef )
		Shift(1);

	ASSERT(IsValid());

	return *this;
}



void CTRef::SetJDay(int year, size_t jd)
{
	if (jd == LAST_DAY)
		jd = WBSF::GetNbDaysPerYear(year) - 1;

	size_t m = WBSF::GetMonthIndex(year, jd);
	size_t d = WBSF::GetDayOfTheMonth(year, jd);

	Set(year, m, d, 0, CTM(CTM::DAILY) ); 
}

void CTRef::SetJDay(size_t jd)
{
	if( m_year > YEAR_NOT_INIT)
		return SetJDay(int(m_year), jd);

	if (jd == LAST_DAY)
		jd = 364;

	size_t m = WBSF::GetMonthIndex(jd);
	size_t d = WBSF::GetDayOfTheMonth(0, jd);

	Set(YEAR_NOT_INIT, m, d, 0, CTM(CTM::DAILY, OVERALL_YEARS) );
}

void CTRef::ResetUnused()
{
	if( !IsAnnual() )
		m_year = YEAR_NOT_INIT;

	switch(m_type)
	{
	case UNKNOWN:	m_year = m_month = m_day = m_hour = 0; break;
	case ANNUAL:	m_month = m_day = m_hour = 0; break;
	case MONTHLY:	m_day = m_hour = 0; break;
	case DAILY:		m_hour = 0; break;
	case HOURLY:	break;
	case ATEMPORAL: break;
	default: ASSERT(false);
	}
}

bool CTRef::IsValid()const
{
	bool bValid=true;
	if( m_type != UNKNOWN && m_type != ATEMPORAL)
	{
		if( !(m_type>=0 && m_type<NB_REFERENCE) )
			bValid=false; 

		if( m_year<YEAR_NOT_INIT)
			bValid=false; 
		
		if( m_month>=12)
			bValid=false; 
		
		if (m_day >= WBSF::GetNbDayPerMonth(m_year, m_month))
			bValid=false; 

		if( m_hour>=24)
			bValid=false;
	}

	return bValid;
}


void CTRef::Reset()
{
	m_type=UNKNOWN;
	m_mode = FOR_EACH_YEAR;
	m_year=0; 
	m_month=0;
	m_day=0;    
	m_hour=0;
	m_ref=0;
}

CTRef& CTRef::operator =(float in)
{
	const void* pBits = this;
	*((float*)pBits) = in; 

	return *this;
}

//CTRef& CTRef::operator =(__int32 in)
//{
//	const void* pBits = this;
//	*((__int32*)pBits) = in; 
//
//	return *this;
//}

CTRef& CTRef::operator =(double in)
{
	Set__int32(__int32(in) );
	return *this;
}

CTRef& CTRef::operator =(const CTRef& in)
{
	if( &in != this)
	{
		m_type=in.m_type;   
		m_mode=in.m_mode;
		m_year=in.m_year;
		m_month=in.m_month;
		m_day=in.m_day;
		m_hour=in.m_hour;
		m_ref=in.m_ref;
	}

	return *this;
}
bool CTRef::operator == (const CTRef& in)const
{
	bool bEqual = true;

	if(m_type!=in.m_type)bEqual = false; 
	if(m_mode!=in.m_mode)bEqual = false; 
	if(m_year!=in.m_year)bEqual = false; 
	if(m_month!=in.m_month)bEqual = false; 
	if(m_day!=in.m_day)bEqual = false; 
	if(m_hour!=in.m_hour)bEqual = false; 
	if(m_ref!=in.m_ref)bEqual = false; 

	return bEqual;
}

__int32 CTRef::operator-(const CTRef& in)const
{
	return GetRef()-in.GetRef();
}

CTRef CTRef::operator-(__int32 nbRef)const
{
	CTRef TRef;
	TRef.SetRef(GetRef() - nbRef, GetTM());
	return TRef;
}

CTRef CTRef::operator+(__int32 nbRef)const
{
	CTRef ref(*this);
	ref.Shift(nbRef);
	return ref;
}

CTRef& CTRef::Shift(__int32 nbRef)
{
	if( IsInit() )
	{
		__int32 ref = GetRef();
		ref+=nbRef;
		SetRef(ref, GetTM());
		ASSERT( IsValid() );
	}

	return *this;
}


__int32 CTRef::GetRef()const
{
	__int32 ref=0;
	switch(m_type)
	{
	case ANNUAL:	ref = __int32(m_year); break;
	case MONTHLY:	ref = __int32((m_year)*12+m_month); break;
	case DAILY:		ref = WBSF::GetRef(short(IsAnnual() ? m_year : 1), m_month, m_day); break;
	case HOURLY:	ref = WBSF::GetRef(short(IsAnnual() ? m_year : 1), short(m_month), short(m_day)) * 24 + __int32(m_hour); break;
	case ATEMPORAL: ref = m_ref; break;
	case UNKNOWN:	ref = 0; break;
	default: ASSERT(false);
	}

	return ref;
}

void CTRef::SetRef(__int32 ref, CTM TM)
{
	ASSERT(TM.Type() != UNKNOWN);
	SetTM(TM);
	
	short neg=ref<0?1:0;
	int year=0;
	size_t m=0;
	size_t d=0;

	if( m_type==DAILY)
		WBSF::SetRef(ref, year, m, d);
	else if( m_type==HOURLY)
		WBSF::SetRef((ref + neg) / 24 - neg, year, m, d);
	
	switch(m_type)
	{
	case ANNUAL:	m_year = ref; break;
	case MONTHLY:	m_year = short((ref+neg)/12-neg); m_month=(12+(ref%12))%12; break;
	case DAILY:		m_year=year; m_month=m; m_day=d; break;
	case HOURLY:	m_year=year; m_month=m; m_day=d;m_hour=(24+(ref%24))%24; break;
	case ATEMPORAL: m_ref = ref; break;
	default: ASSERT(false);
	}
	
	//if( m_year != 0)
	//	m_mode = ANNUAL;

	ResetUnused();
	ASSERT(IsValid());
}


CTRef& CTRef::Transform(const CTM& TM, size_t initType)
{ 
	if (IsInit())
	{ 
		if (TM.Type() > GetTM().Type())
		{
			switch (initType)
			{
			case FIRST_TREF: break; //*do nothing
			case MID_TREF: SetRef((Transform(*this, TM, FIRST_TREF).GetRef() + Transform(*this, TM, LAST_TREF).GetRef()) / 2, TM); break;
			case LAST_TREF:	Shift(1);break;//add one ref her
			default: ASSERT(false);
			}

			SetTM(TM);

			switch (initType)
			{
			case FIRST_TREF: /*do nothing*/ break;
			case MID_TREF: break;
			case LAST_TREF:	Shift(-1); break;//remove one ref her
			default: ASSERT(false);
			}

			ResetUnused();
		}
		else
		{
			SetTM(TM);
			ResetUnused();
		}
	}
	
	return *this; 
}


#include "time.h"

//%a	Abbreviated weekday name *	Thu
//%A	Full weekday name *	Thursday
//%b	Abbreviated month name *	Aug
//%B	Full month name *	August
//%c	Date and time representation *	Thu Aug 23 14:55:02 2001
//%d	Day of the month (01-31)	23
//%H	Hour in 24h format (00-23)	14
//%I	Hour in 12h format (01-12)	02
//%j	Day of the year (001-366)	235
//%m	Month as a decimal number (01-12)	08
//%M	Minute (00-59)	55
//%p	AM or PM designation	PM
//%S	Second (00-61)	02
//%U	Week number with the first Sunday as the first day of week one (00-53)	33
//%w	Weekday as a decimal number with Sunday as 0 (0-6)	4
//%W	Week number with the first Monday as the first day of week one (00-53)	34
//%x	Date representation *	08/23/01
//%X	Time representation *	14:55:02
//%y	Year, last two digits (00-99)	01
//%Y	Year	2001
//%Z	Timezone name or abbreviation	CDT
//%%	A % sign	%
//#		remove zero before number

string CTRef::GetMissingString(CTM tm)
{
	return TREF_FORMAT.GetMissingString(tm);
}

string CTRef::ToString()const
{
	string str;
	str.resize(100);

	short t = short(m_type);
	short y = IsAnnual()?short(m_year):YEAR_NOT_INIT;
	__int32  r = IsAnnual()?short(m_ref):YEAR_NOT_INIT;
	short m = short(m_month+1);
	short d = short(m_day+1);
	short h = short(m_hour);

	switch(m_type)
	{
	case HOURLY:	_snprintf(&(str[0]), 100, "%02d%04d%02d%02d%02d", t, y, m, d, h); break;
	case DAILY:		_snprintf(&(str[0]), 100, "%02d%04d%02d%02d", t, y, m, d); break;
	case MONTHLY:	_snprintf(&(str[0]), 100, "%02d%04d%02d", t, y, m); break;
	case ANNUAL:	_snprintf(&(str[0]), 100, "%02d%04d", t, y); break;
	case ATEMPORAL: _snprintf(&(str[0]), 100, "%02d%10d", t, r); break;
	case UNKNOWN:	_snprintf(&(str[0]), 100, "%02d", t); break;
	default: ASSERT(false);
	}

	str.resize(strlen(str.c_str()));
	str.shrink_to_fit();

	return str;
}

void CTRef::FromString(const string& str)
{
	ASSERT( str.length() >= 2);

	short type = UNKNOWN;
	short mode = FOR_EACH_YEAR;
	short y_or_r = YEAR_NOT_INIT;
	short m = 0;
	short d = 0;
	short h = 0;
	

	if( str.length() >= 2)
		type = ToValue<short>(str.substr(0, 2));

	if( type!=UNKNOWN )
	{
		if( type != ATEMPORAL )
		{
			if( str.length() >= 6)
				y_or_r = ToValue<short>(str.substr(2, 4));

			if( str.length() >= 8)
				m = ToValue<short>(str.substr(6, 2)) - 1;
			if( str.length() >= 10)
				d = ToValue<short>(str.substr(8, 2)) - 1;
			if( str.length() >= 12)
				h = ToValue<short>(str.substr(10, 2)) % 24;

		}
		else
		{
			y_or_r = ToValue<short>(str.substr(2));
		}
	
		if( y_or_r == YEAR_NOT_INIT )
			mode = OVERALL_YEARS;
	}

	Set(y_or_r, m, d, h, CTM(type, mode) );
}


string CTRef::GetFormatedString(string format)const
{
	ASSERT(IsInit());

	string str;

	//time_t rawtime;


	if (IsTemporal())
	{
		if (format.empty())
			format = TREF_FORMAT.GetFormat(CTM(m_type, m_mode));

		// get current timeinfo and modify it to the user's choice
		//time ( &rawtime );
		//timeinfo = gmtime ( &rawtime );
		struct tm timeinfo = { 0 };
		timeinfo.tm_sec = 0;     // seconds after the minute - [0,59] 
		timeinfo.tm_min = 0;     // minutes after the hour - [0,59] 
		timeinfo.tm_hour = m_hour;    // hours since midnight - [0,23] 
		timeinfo.tm_mday = m_day + 1;    // day of the month - [1,31] 
		timeinfo.tm_wday = (GetRef() + 1) % 7;//day of the week, range 0 to 6, o=sunday
		timeinfo.tm_yday = (int)GetJDay(m_year, m_month, m_day);			  //Day of year (0 - 365)
		timeinfo.tm_mon = m_month;     // months since January - [0,11] 
		timeinfo.tm_isdst = 0;

		if (m_year >= 1970 && m_year < 2038)
		{
			timeinfo.tm_year = m_year - 1900;
		}
		else
		{
			timeinfo.tm_year = WBSF::IsLeap(m_year) ? 96 : 95;//leap/non leap year

			string year = WBSF::ToString(GetYear());

			ReplaceString(format, "%y", "%Y");
			ReplaceString(format, "%#Y", "%Y");
			ReplaceString(format, "%Y", year);
		}

		try
		{
			char buffer[256] = { 0 };
			strftime(buffer, 256, format.c_str(), &timeinfo);
			str = buffer;
		}
		catch (...)
		{
		}
	}
	else
	{
		str = WBSF::ToString(__int32(m_ref));
	}
	return str;
}

const size_t NB_TIME_FORMATS = sizeof(TIME_FORMATS) / sizeof(TIME_FORMATS[0]);

time_t pt_to_time_t(const boost::posix_time::ptime& pt)
{
	boost::posix_time::ptime timet_start(boost::gregorian::date(1970, 1, 1));
	boost::posix_time::time_duration diff = pt - timet_start;
	return diff.ticks() / boost::posix_time::time_duration::rep_type::ticks_per_second;

}
void seconds_from_epoch(const string& s)
{
	boost::posix_time::ptime pt;
	for (size_t i = 0; i<NB_TIME_FORMATS; ++i)
	{
		istringstream is(s);
		is.imbue(TIME_FORMATS[i]);
		is >> pt;
		if (pt != boost::posix_time::ptime())
			break;
	}
}

void CTRef::FromFormatedString(string str, string format, const char* sep, int base )
{
	ASSERT( str.length() >= 2);

	Reset();

	if (str.empty())
		return;

	
	if (str[0] == '-')
		str[0] = '*';
	


	StringVector elems(str.c_str(), sep);
	if (elems[0][0] == '*')
		elems[0][0] = '-';

	short type = UNKNOWN;
	short mode = FOR_EACH_YEAR;
	short y_or_r = YEAR_NOT_INIT;
	short m = 0;
	short d = 0;
	short h = 0;

	if (format.empty())
	{
		//try to find format
		if (elems.size() == 1)
		{
			//annual or reference??
			type = ANNUAL;
			y_or_r = ToInt(elems[0]);
		}
		else if (elems.size() == 2)
		{
			//monthly or daily
			type = MONTHLY;
			y_or_r = ToInt(elems[0]);
			m = ToInt(elems[1]);
		}
		else if (elems.size() == 3)
		{
			//daily
			type = DAILY;
			y_or_r = ToInt(elems[0]);
			m = ToInt(elems[1]);
			d = ToInt(elems[2]);
		}
		else //if (elems.size() >= 4)
		{
			//hourly
			type = HOURLY;
			y_or_r = ToInt(elems[0]);
			m = ToInt(elems[1]);
			d = ToInt(elems[2]);
			h = ToInt(elems[3]);
		}

	}
	else
	{
		if (!str.empty())
		{

			if (format == CTRefFormat::DATE_YMD)
			{

				type = DAILY;
				y_or_r = ToInt(elems[0]);
				m = ToInt(elems[1]);
				d = ToInt(elems[2]);
			}
			else if (format == CTRefFormat::DATE_YMDH || format == CTRefFormat::DATE_YMD_HMS)
			{
				type = DAILY;
				y_or_r = ToInt(elems[0]);
				m = ToInt(elems[1]);
				d = ToInt(elems[2]);
			}
			else
			{
				auto localImbu = locale(std::locale::classic(), new boost::posix_time::time_input_facet(format));
				boost::posix_time::ptime ptime;

				istringstream is(str);
				is.imbue(localImbu);
				is >> ptime;
				if (ptime != boost::posix_time::ptime())
				{
					//second since 1970
					//ptime.date
					assert(false);
				}

			}
			//else if ()
			//{
				//CTRefFormat::
				//type = DAILY;
				//d = ToInt(elems[0]);// str.Tokenize(sep, pos).ToInt();
				//m = ToInt(elems[1]); //str.Tokenize(sep, pos).ToInt();
				//y_or_r = ToInt(elems[2]); //str.Tokenize(sep, pos).ToInt();
				//break;

				//case DATE_MD:
				//	type = MONTHLY;
				//	m = ToInt(elems[0]); //str.Tokenize(sep, pos).ToInt();
				//	d = ToInt(elems[1]); //str.Tokenize(sep, pos).ToInt();
				//	break;

				//case DATE_DM:
				//	type = MONTHLY;
				//	d = ToInt(elems[0]); //str.Tokenize(sep, pos).ToInt();
				//	m = ToInt(elems[1]); //str.Tokenize(sep, pos).ToInt();
				//	break;

		
		}
	}

	if( y_or_r == YEAR_NOT_INIT && type != UNKNOWN)
		mode = OVERALL_YEARS;
	
	Set(y_or_r, m-base, d-base, h, CTM(type,mode) );
}

CTRef CTRef::GetCurrentTRef(CTM TM)
{
	time_t ltime;
	tm today = { 0 };

	_tzset();
	time(&ltime);
	_localtime64_s(&today, &ltime);

	//month is un zero base and day is in 1 base
	return CTRef(1900 + today.tm_year, today.tm_mon, today.tm_mday - 1, today.tm_hour, TM);
}

CTRef CTRef::Disaggregate( const CTM& TM )const
{
	ASSERT( GetMode() < TM.Mode() );
	ASSERT( GetType() < TM.Type() );

	CTRef copy(*this);
	//if( copy.GetType() < TM.Type() )
	//{
	//	switch(TM.Type())
	//	{
	//	case HOURLY:	copy.m_hour = 12; 
	//	case DAILY:		copy.m_day = 15; 
	//	case MONTHLY:	copy.m_month = 5;break;
	//	case ANNUAL:	//trange transformation
	//	case ATEMPORAL: 
	//	case UNKNOWN:	
	//	default: ASSERT(false);
	//	}
	//}
	//else
	//{
	copy.SetTM(TM);
	copy.ResetUnused();

	return copy;
}

//*************************************************************
//CTPeriod

CTPeriod::CTPeriod(const CTRef& begin, const CTRef& end, short type)
{
	ASSERT( type == CONTINUOUS || type == YEAR_BY_YEAR );
	ASSERT(begin.GetTM() == end.GetTM() );

	m_begin=begin;
	m_end=end;
	m_type=type;
}

CTPeriod::CTPeriod(int y1, int y2, int type)
{
	ASSERT( type == CONTINUOUS || type == YEAR_BY_YEAR );
	m_begin = CTRef(y1);
	m_end = CTRef(y2);
	m_type=type;
}

CTPeriod::CTPeriod(int y1, size_t m1, int y2, size_t m2, int type)
{
	ASSERT( type == CONTINUOUS || type == YEAR_BY_YEAR );
	m_begin = CTRef(y1, m1);
	m_end = CTRef(y2, m2);
	m_type=type;
}
CTPeriod::CTPeriod(int y1, size_t m1, size_t d1, int y2, size_t m2, size_t d2, int type)
{
	ASSERT( type == CONTINUOUS || type == YEAR_BY_YEAR );
	m_begin = CTRef(y1, m1, d1);
	m_end = CTRef(y2, m2, d2);
	m_type=type;
}

void CTPeriod::Reset()
{
	m_type=CONTINUOUS;
	m_begin.Reset();
	m_end.Reset();
}

CTPeriod& CTPeriod::operator = (const CTPeriod& in)
{
	if( &in != this)
	{
		m_type=in.m_type;
		m_begin=in.m_begin;
		m_end=in.m_end;
	}

	ASSERT( operator==(in) );
	return *this;
}

bool CTPeriod::operator == (const CTPeriod& in)const
{
	ASSERT( m_begin.m_type == m_end.m_type);

	bool bEqual = true;

	if(m_type!=in.m_type) bEqual = false;
	if(m_begin!=in.m_begin) bEqual = false;
	if(m_end!=in.m_end) bEqual = false;

	return bEqual;

}

CTPeriod& CTPeriod::Inflate(const CTRef& in)
{
	ASSERT( in.IsInit() );

	if( IsInit() )
	{
		if( in < m_begin)
			m_begin = in;
		if( in > m_end)
			m_end = in;
	}
	else
	{
		m_begin = m_end = in;
	}

	return *this;
}

CTPeriod& CTPeriod::Inflate(const CTPeriod& in)
{
//	ASSERT( in.IsInit() );

	if( IsInit() )
	{
		if( in.m_begin < m_begin)
			m_begin = in.m_begin;
		if( in.m_end > m_end)
			m_end = in.m_end;
	}
	else
	{ 
		m_begin = in.m_begin;
		m_end = in.m_end;
	}

	return *this;
}

bool CTPeriod::IsInside(CTRef ref)const
{
	if( !IsInit() )
		return true;

	ASSERT( m_begin.m_type == m_end.m_type);
	ASSERT( ref.m_type == m_end.m_type);

	bool bRep=ref>=m_begin && ref<=m_end;
	if( bRep && m_type == YEAR_BY_YEAR && IsAnnual())
	{
		CTPeriod p = GetAnnualPeriod(ref.m_year);
		bRep = IsInversed()?p.IsOutsideInclusive(ref):p.IsInside(ref);
	}

	return bRep;
}

bool CTPeriod::IsOutsideInclusive(CTRef ref)const
{
	return ref<=m_begin || ref>=m_end;
}

bool CTPeriod::IsInside(const CTPeriod& p)const 
{
	_ASSERTE( p.GetTM() == GetTM() );
	bool bRep = p.Begin() >= m_begin && p.End() <= m_end;

	if( bRep && m_type == YEAR_BY_YEAR)
	{
		//_ASSERTE( p.GetType() == CONTINUOUS );

		bRep=false;
		size_t nbSegment = GetNbSegments();
		for (size_t i = 0; i<nbSegment&&!bRep; i++)
			bRep = GetSegment(i).IsInside(p);
	}

	return bRep;
}


bool CTPeriod::IsIntersect(const CTPeriod& p)const 
{
	bool bRep = false;
	if( m_type == CONTINUOUS)
		bRep = IsInside(p.Begin()) || IsInside(p.End()) || p.IsInside(Begin()) || p.IsInside(End());
	else 
	{
		_ASSERTE( p.GetType() == CONTINUOUS );

		bRep=false;
		size_t nbSegment = GetNbSegments();
		for (size_t i = 0; i<nbSegment&&!bRep; i++)
			bRep = GetSegment(i).IsIntersect(p);
	}

	return bRep;
}


CTPeriod CTPeriod::Intersect(const CTPeriod& p)const
{
	_ASSERTE( p.m_type == CONTINUOUS );
	CTPeriod _p( *this );

	CTPeriod p2;
	if( m_type == YEAR_BY_YEAR )
	{
		//ASSERT( p.GetNbYears() == 1);
		//ASSERT( !IsInversed() );
		_p.Begin().m_year = p.Begin().m_year;
		_p.End().m_year = p.End().m_year;
		
		p2 = CTPeriod( max( _p.m_begin, p.Begin() ), min( _p.m_end, p.End() ), m_type);
	}
	else
	{
		p2 = CTPeriod( max( _p.m_begin, p.Begin() ), min( _p.m_end, p.End() ), m_type);
		if( p2.Begin() > p2.End() )
			p2.Reset();
	}
	return p2;
}

size_t CTPeriod::GetNbSegments()const
{
	if(!IsInit() )
		return 0;

	if( GetType() == CONTINUOUS || GetTMode() == OVERALL_YEARS)
		return 1;
	
	//return IsInversed()?GetNbYears()-1:GetNbYears();
	return IsInversed()?GetNbYears()+1:GetNbYears();
}

int CTPeriod::GetSegmentIndex(CTRef ref)const
{
	if( !IsInit() || !IsInside(ref) )
		return -1;

	if( GetType() == CONTINUOUS || GetTMode() == OVERALL_YEARS)
		return 0;
	
	int index = ref.GetYear()-m_begin.GetYear();
	
	if( IsInversed() )
	{
		if(ref.GetJDay() > m_begin.GetJDay())
			index++;
	}

	return index;
	//return ref.GetYear()-m_begin.GetYear();
	//return ref.GetYear()-m_begin.GetYear();
}

CTPeriod CTPeriod::GetSegment(size_t i)const
{
	ASSERT( i>= 0 && i<GetNbSegments() );

	CTPeriod p(*this);
	
	if( GetType() == CONTINUOUS || GetTMode() == OVERALL_YEARS)
		return p;

	if( IsInversed() )
	{
		if(i==0)
		{
			p.SetType(CONTINUOUS);
			p.Begin() = CTRef(Begin().m_year, JANUARY, FIRST_DAY);
			p.End().m_year = Begin().m_year;
		}
		else if( i== GetNbSegments()-1)
		{
			p.SetType(CONTINUOUS);
			p.Begin().m_year = End().m_year;
			p.End() = CTRef(End().m_year, DECEMBER, LAST_DAY);
		}
		else
		{
			p.SetType(CONTINUOUS);
			p.Begin().m_year = Begin().m_year+i;
			p.End().m_year = Begin().m_year+i+1;
		}
	}
	else
	{
		p.SetType(CONTINUOUS);
		p.Begin().m_year = Begin().m_year+i;
		p.End().m_year = Begin().m_year+i;
	}


	return p;
}

string CTPeriod::GetFormatedString(string periodformat, string TRefFormat)const
{
	return FormatMsg(periodformat.c_str(), m_begin.GetFormatedString(TRefFormat), m_end.GetFormatedString(TRefFormat));
}

void CTPeriod::FromFormatedString(string str, string periodformat, string TRefFormat, const char* sep, int base)
{
	ReplaceString(periodformat, "%1", "%s");
	ReplaceString(periodformat, "%2", "%s");

	const char* str1 = NULL;
	const char* str2 = NULL;
	if (sscanf(str.c_str(), periodformat.c_str(), &str1, &str2) == 2)
	{
		CTRef TRef1, TRef2;
		TRef1.FromFormatedString(str1, TRefFormat, sep, base);
		TRef2.FromFormatedString(str2, TRefFormat, sep, base);
		if (TRef1.IsInit() && TRef2.IsInit())
		{
			m_type = CONTINUOUS;
			m_begin = TRef1;
			m_end = TRef2;
		}
	}
}

string CTPeriod::ToString()const
{ 
	return "(" + WBSF::ToString(m_type) + " " + m_begin.ToString() + " " + m_end.ToString() + ")";
}

void CTPeriod::FromString(const string& strIn)
{
	StringVector str(strIn.c_str(), "( );");
	if (str.size() == 3)
	{
		int pos = 0;
		m_type = ToInt(str[0]);
		m_begin.FromString(str[1]);
		m_end.FromString(str[2]);
	}
	//int pos=0;
	//m_type = atoi(str.Tokenize("( );", pos));
	//m_begin.FromString(str.Tokenize("( );", pos));
	//if (pos >= 0)
	//	m_end.FromString(str.Tokenize("( );", pos));
}

void CTPeriod::ReadStream(istream& stream)
{
	__int32 tmp=0;
	stream.read( (char*)(&m_type), sizeof(m_type));
	stream.read( (char*)(&tmp), sizeof(tmp));m_begin.Set__int32(tmp);
	stream.read((char*)(&tmp), sizeof(tmp)); m_end.Set__int32(tmp);
}

void CTPeriod::WriteStream(ostream& stream)const
{
	__int32 tmp=0;
	stream.write( (char*)(&m_type), sizeof(m_type));
	tmp = m_begin.Get__int32(); stream.write( (char*)(&tmp), sizeof(tmp));
	tmp = m_end.Get__int32(); stream.write((char*)(&tmp), sizeof(tmp));
}

CTPeriod CTPeriod::GetAnnualPeriod(int year)const
{
	ASSERT( year != YEAR_NOT_INIT );
	ASSERT( m_type != CONTINUOUS);

	CTPeriod p(*this);
	p.SetType(CONTINUOUS);
	
	if( IsInversed() )
	{
		//exclusion period: must decrement the reference

		p.m_begin = m_end;
		p.m_end= m_begin ;
	}

	p.m_begin.m_year = year;
	p.m_end.m_year = year;
	
	return p;
}

CTRef CTPeriod::GetFirstAnnualTRef(int y)const
{
	ASSERT( y>=0 && y<GetNbYears() );
		
	CTRef ref = m_begin;
	if( y!=0)
	{
		ref.m_year = m_begin.m_year+y;
		if( m_type == CONTINUOUS)
		{
			ref.m_month = 0; 
			ref.m_day = 0;
			ref.m_hour = 0;
		}
	}
	
	ref.ResetUnused();

	return ref;
}

CTRef CTPeriod::GetLastAnnualTRef(int y)const
{
	ASSERT( y>=0 && y<GetNbYears() );
		
	CTRef ref = m_end;
	if( y!=GetNbYears()-1)
	{
		ref.m_year = m_begin.m_year+y;
		if( m_type == CONTINUOUS)
		{
			ref.m_month = LAST_MONTH; 
			ref.m_day = LAST_DAY;
			ref.m_hour = LAST_HOUR;
		}
	}

	ref.ResetUnused();

	return ref;
}

__int32 CTPeriod::GetNbDay2()const
{
	CTPeriod p(*this);
	p.Transform(CTM(CTM::DAILY, p.GetTMode()));
	return p.GetNbRef();
}

CTPeriod CTPeriod::Get(const CTM& TM)
{
	CTPeriod tmp = *this;
	tmp.Transform(TM);
	return tmp;
}


CTPeriod& CTPeriod::Transform(const CTM& TM)
{
	CTM oldTM = GetTM();
	m_begin.Transform(TM); 
	m_end.Transform(TM);

	if (oldTM.IsInit() && TM.IsInit() && TM.Type() > oldTM.Type())
	{
		bool bResetMonth = oldTM.Type() < CTM::MONTHLY&&TM.Type() >= CTM::MONTHLY;
		bool bResetDay = oldTM.Type() < CTM::DAILY&&TM.Type() >= CTM::DAILY;
		bool bResetHour = oldTM.Type() < CTM::HOURLY&&TM.Type() >= CTM::HOURLY;
		
		if (bResetMonth)
			m_end.m_month = LAST_MONTH;
		if (bResetDay)
			m_end.m_day = WBSF::GetNbDayPerMonth(m_end.GetYear(), m_end.GetMonth());
		if (bResetHour)
			m_end.m_hour = LAST_HOUR;
	}
	
	return *this;
}
//**************************************************************
//CJDayPeriod

CJDayPeriod::CJDayPeriod(int year, short Jday1, short Jday2)
{
	m_begin.SetJDay(year, Jday1);
	m_end.SetJDay(year, Jday2);
	m_type=CONTINUOUS;
}

CJDayPeriod::CJDayPeriod(short firtYear, short Jday1, short lastYear, short Jday2, int type)
{
	ASSERT( type == CONTINUOUS || type == YEAR_BY_YEAR );
	m_begin.SetJDay(firtYear, Jday1);
	m_end.SetJDay(lastYear, Jday2);
	m_type=type;
}

//**************************************************************
//CTTransformation

CTTransformation::CTTransformation(const CTPeriod& p, CTM TM)
{
	ComputeCluster(p, TM);
}

CTTransformation::CTTransformation(const CTPeriod& pIn, const CTPeriod& pOut)
{
	ComputeCluster(pIn, pOut);
}


void CTTransformation::ComputeCluster(const CTPeriod& pIn, const CTPeriod& pOut)
{
	m_pIn = pIn;
	m_pOut = pOut;

	size_t nbCluster = m_pOut.GetNbRef();
	m_clusters.resize(nbCluster);

	CTRef ref = m_pOut.Begin();
	for (size_t c = 0; c<nbCluster; c++, ref++)
	{
		m_clusters[c] = ref;
	}
}


void CTTransformation::ComputeCluster(const CTPeriod& p, CTM TM)
{
	CTPeriod pOut = p;
	pOut.Transform(TM);

	ComputeCluster(p, pOut);
}

bool CTTransformation::IsInside(CTRef TRef)const
{
	bool bInside = m_pIn.IsInside(TRef);

	TRef.Transform(m_pOut.Begin());
	return bInside && m_pOut.IsInside(TRef);
}

size_t CTTransformation::GetCluster(CTRef TRef)const
{
	bool bInside = m_pIn.IsInside(TRef);
	TRef.Transform(m_pOut.Begin());
	
	return (bInside&&m_pOut.IsInside(TRef)) ? m_pOut.GetRefPos(TRef) : UNKNOWN_POS;
}

//**************************************************************
//CSun

// Revised on January 6th, 2000 
// A simple C++ program calculating the sunrise and sunset for
// the current date and a set location (latitude,__int32itude)
// Jarmo Lammi © 1999 - 2000
// jjlammi@netti.fi


const double CSun::pi = 3.141592653589793238;
const double CSun::tpi = 2 * pi;
const double CSun::degs = 180.0/pi;
const double CSun::rads = pi/180.0;

// Get the days to J2000 
// h is UT in decimal hours
// FNday only works between 1901 to 2099 - see Meeus chapter 7
double CSun::FNday (int year, int month, int day, float h) 
{
	__int64 luku = - 7 * (year + (month + 9)/12)/4 + 275*month/9 + day;

	// Typecasting needed for TClite on PC DOS at least, to avoid product overflow
	luku+= (__int64)year*367;

	return (double)luku - 730531.5 + h/24.0;
}

// the function below returns an angle in the range 0 to 2*pi
double CSun::FNrange (double x) 
{
    double b = x / tpi;
    double a = tpi * (b - (__int32)(b));
    if (a < 0) a = tpi + a;

	return a;
};

// Calculating the hourangle
double CSun::f0(double lat, double declin) 
{
	static const double  SunDia = 0.53;  // Sunradius degrees
	static const double  AirRefr = 34.0/60.0; // athmospheric refraction degrees

// Correction: different sign at S HS
	double dfo = rads*(0.5*SunDia + AirRefr); if (lat < 0.0) dfo = -dfo;
	double fo = tan(declin + dfo) * tan(lat*rads);

	if (fo > 0.99999) fo=1.0; // to avoid overflow //
	if (fo < -0.99999) fo=-1.0; // to avoid overflow //
	fo = asin(fo) + pi/2.0;

	return fo;
};

// Calculating the hourangle for twilight times
double CSun::f1(double lat, double declin) 
{
// Correction: different sign at S HS
	double df1 = rads * 6.0; if (lat < 0.0) df1 = -df1;
	double fi = tan(declin + df1) * tan(lat*rads);

	if (fi > 0.99999) fi=1.0; // to avoid overflow //
	if (fi < -0.99999) fi=-1.0; // to avoid overflow //
	fi = asin(fi) + pi/2.0;

	return fi;
};

// Find the ecliptic __int32itude of the Sun
double CSun::FNsun (double d) 
{
	// mean __int32itude of the Sun
	double L = FNrange(280.461 * rads + .9856474 * rads * d);

	// mean anomaly of the Sun
	double g = FNrange(357.528 * rads + .9856003 * rads * d);

	// Ecliptic __int32itude of the Sun
	return FNrange(L + 1.915 * rads * sin(g) + .02 * rads * sin(2 * g));
};
 
CSun::CSun(double lat, double lon, double zone)
{
	m_lat = lat;
	m_lon = lon;
	m_zone=zone;
}


void CSun::Compute(int year, size_t m, size_t d)const
{
	ASSERT(m < 12);
	assert(d < WBSF::GetNbDayPerMonth(year, m));

	if( year!=m_year||m!=m_month||d!=m_day)
	{
		CSun& me = const_cast<CSun&>(*this);
		me.m_year=year;
		me.m_month=(short)m;
		me.m_day=(short)d;
		me.Compute(m_lat, m_lon, m_zone, m_year, m_month, m_day, me.m_sunrise, me.m_sunset, me.m_solarNoon, me.m_daylength );
	}
}

void CSun::Compute(double lat, double lon, double tzone, int year, size_t m, size_t d, double& sunrise, double& sunset, double& solarNoon, double& daylength)
{
	ASSERT( lat >= -90 && lat <= 90);
	ASSERT( lon >= -180 && lon <= 180);
	ASSERT( m < 12);
	ASSERT( d < 31);

	//_tzset();
	if( tzone == -999)
		tzone = lon/15;

// Timezone hours
	double jd = FNday(year, int(m)+1, int(d)+1, 12);

	// Use FNsun to find the ecliptic __int32itude of the Sun
	double lambda = FNsun(jd);

	// Obliquity of the ecliptic
	double obliq = 23.439 * rads - .0000004 * rads * jd;

	// Find the RA and DEC of the Sun
	double alpha = atan2(cos(obliq) * sin(lambda), cos(lambda));
	double delta = asin(sin(obliq) * sin(lambda));


	// Find the Equation of Time in minutes
	// Correction suggested by David Smith

	// mean __int32itude of the Sun
	double L = FNrange(280.461 * rads + .9856474 * rads * jd);
	double LL = L - alpha;
	if (L < pi) LL += tpi;
	double equation = 1440.0 * (1.0 - LL / tpi);


	double ha = f0(lat,delta);
	double hb = f1(lat,delta);
	double twx = hb - ha;   // length of twilight in radians
	twx = 12.0*twx/pi;      // length of twilight in degrees
	// Conversion of angle to hours and minutes 
	double daylen = degs * ha / 7.5;
	if (daylen<0.0001) {daylen = 0.0;}
	if( daylen>24.0 ){daylen=24.0;}
	// arctic winter   

	double riset = 12.0 - 12.0 * ha/pi + tzone - lon/15.0 + equation/60.0;
	double settm = 12.0 + 12.0 * ha/pi + tzone - lon/15.0 + equation/60.0;
	double noont = riset + 12.0 * ha/pi;
	double altmax = 90.0 + delta * degs - lat;
	// Correction suggested by David Smith
	// to express as degrees from the N horizon

	if (delta * degs > lat ) altmax = 90.0 + lat - delta * degs;
	

	double twam = riset - twx;      // morning twilight begin
	double twpm = settm + twx;      // evening twilight end
	if (riset > 24.0) riset-= 24.0;
	if (settm > 24.0) settm-= 24.0;
	if (noont>24.0)   noont-= 24.0;
	if( daylen>=24){riset=0;settm=24.0;}
	//set output
	sunrise = riset;
	sunset = settm;
	solarNoon = noont;
	daylength = daylen;

	ASSERT( !_isnan(sunrise) && _finite(sunrise) );
	ASSERT( !_isnan(sunset) && _finite(sunset) );
	ASSERT( sunrise>=0 && sunrise<=24 );
	ASSERT( sunset>=0 && sunset<=24 );
	ASSERT( solarNoon>=0 && solarNoon<=24 );
	ASSERT( daylength>=0 && daylength<=24 );

}

//%a	Abbreviated weekday name					Thu
//%A	Full weekday name							Thursday
//%b	Abbreviated month name						Aug
//%B	Full month name								August
//%c	Date and time representation				Thu Aug 23 14:55:02 2001
//%d	Day of the month				(01-31)		23
//%H	Hour in 24h format				(00-23)		14
//%I	Hour in 12h format				(01-12)		02
//%j	Day of the year					(001-366)	235
//%m	Month as a decimal number		(01-12)		08
//%M	Minute							(00-59)		55
//%p	AM or PM designation						PM
//%S	Second							(00-61)		02
//%U	Week number with the first Sunday as the first day of week one	(00-53)		33
//%w	Weekday as a decimal number with Sunday as 0					(0-6)		4
//%W	Week number with the first Monday as the first day of week one	(00-53)		34
//%x	Date representation							08/23/01
//%X	Time representation							14:55:02
//%y	Year, last two digits			(00-99)		01
//%Y	Year										2001
//%Z	Timezone name or abbreviation				CDT
//%%	A % sign	%



std::string GetCurrentTimeString(const std::string format)
{

	time_t rawtime;
	time (&rawtime);
	struct tm * timeinfo = localtime (&rawtime);

	string str;
	str.resize(150);
	strftime(&(str[0]), 150, format.c_str(), timeinfo);
	str.resize(strlen(str.c_str()));
	str.shrink_to_fit();

	return str;
}



std::string FormatTime(const std::string& format, __time64_t t)
{
	struct tm *newtime = _localtime64(&t);
	newtime->tm_isdst=0;//don't use 
	
	string str;
	
	str.resize(150);
	strftime(&(str[0]), 150, format.c_str(), newtime);

	//strftime(str.GetBufferSetLength(150), 150, format.c_str(), newtime);
	//str.ReleaseBuffer();
	return str;
}

std::string FormatTime(const std::string& format, int year, size_t m, size_t d, size_t h, size_t minute, size_t second)
{
	assert(m < 12);
	assert(d < GetNbDayPerMonth(year, m));

	struct tm timeinfo = {0};
	timeinfo.tm_isdst = 0;
	timeinfo.tm_year = year - 1900;
	timeinfo.tm_mon = int(m);//month in base zero 
	timeinfo.tm_mday = int(d+1);//day in base one
	timeinfo.tm_hour = int(h);
	timeinfo.tm_min = int(minute);
	timeinfo.tm_sec = int(second);
	timeinfo.tm_yday = (int)WBSF::GetJDay(year, m, d);

	string str;
	str.resize(150);
	strftime(&(str[0]), 150, format.c_str(), &timeinfo);

	//strftime(str.GetBufferSetLength(150), 150, format.c_str(), &timeinfo);
	//str.ReleaseBuffer();
	
	return str;
}

}//namespace WBSF


