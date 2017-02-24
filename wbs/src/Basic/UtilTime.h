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

#include <crtdbg.h>
#include <vector>

#include "Basic/ERMsg.h"
#include "Basic/zenXml.h"
#include "Basic/UtilStd.h"
#include "Basic/TimeStep.h"


namespace WBSF
{

	class CJDayRef;

	enum { MAX_DAY_PER_MONTH = 31 };

	enum TMonth{ JANUARY, FEBRUARY, MARCH, APRIL, MAY, JUNE, JULY, AUGUST, SEPTEMBER, OCTOBER, NOVEMBER, DECEMBER };
	enum TDayOfMonth{ DAY_01, DAY_02, DAY_03, DAY_04, DAY_05, DAY_06, DAY_07, DAY_08, DAY_09, DAY_10, DAY_11, DAY_12, DAY_13, DAY_14, DAY_15, DAY_16, DAY_17, DAY_18, DAY_19, DAY_20, DAY_21, DAY_22, DAY_23, DAY_24, DAY_25, DAY_26, DAY_27, DAY_28, DAY_29, DAY_30, DAY_31 };
	enum TShortMonth{ JAN, FEB, MAR, APR, /*MAY,:already define*/ JUN = JUNE, JUL, AUG, SEP, OCT, NOV, DEC };
	enum { FIRST_DAY = 0, LAST_DAY = -2, DAY_NOT_INIT = -1 };
	enum { FIRST_MONTH = 0, LAST_MONTH = 11, MONTH_NOT_INIT = -1 };
	enum { FIRST_HOUR = 0, LAST_HOUR = 23, HOUR_NOT_INIT = -1 };
	enum { YEAR_NOT_INIT = -999 };

	//don't use these variable directly : use interface
	static const size_t _DAYS_IN_MONTH[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	static const size_t FIRST_DAY_INDEX[12] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
	static const size_t FIRST_DAY_LEAP_INDEX[12] = { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 };
	static const double _MID_DAY_IN_MONTH[12] = { 15.5, 45.0, 74.5, 105.0, 135.5, 166.0, 196.5, 227.5, 258.0, 288.5, 319.0, 349.5 };

	const char* GetMonthName(size_t m, bool bFull = true);
	const char* GetMonthTitle(size_t m, bool bFull = true);


	inline double MidDayInMonth(size_t m)
	{
		_ASSERTE(m >= 0 && m < 12);
		return _MID_DAY_IN_MONTH[m];
	}

	inline static bool IsLeap(int year)
	{
		return (year > 0) && !(year % 4 || (!(year % 100) && year % 400));
	}

	inline static size_t GetNbDaysPerYear(int year)
	{
		return 365 + IsLeap(year);
	}

	inline static size_t GetNbDayPerMonth(int year, size_t month)
	{
		_ASSERTE(year >= YEAR_NOT_INIT && year <= 2101);
		_ASSERTE(month >= 0 && month < 12);

		size_t nbDay = _DAYS_IN_MONTH[month];
		if (month == 1 && IsLeap(year))
			nbDay++;

		return nbDay;
	}
	inline size_t GetNbDayPerMonth(size_t m)
	{
		_ASSERTE(m >= 0 && m < 12);
		return _DAYS_IN_MONTH[m];
	}

	inline size_t GetRealDay(int year, size_t m, size_t d){ return d == LAST_DAY ? GetNbDayPerMonth(year, m) : d; }
	inline size_t GetRealDay(size_t m, size_t d){ return d == LAST_DAY ? GetNbDayPerMonth(m) : d; }

	inline size_t GetFirstJdayForMonth(int year, size_t m)
	{
		return IsLeap(year) ? FIRST_DAY_LEAP_INDEX[m] : FIRST_DAY_INDEX[m];
	}

	inline size_t GetFirstJdayForMonth(size_t m)
	{
		return GetFirstJdayForMonth(0, m);
	}

	size_t GetJDay(int year, size_t m, size_t d);
	size_t GetDayOfTheMonth(int year, size_t jd);
	inline size_t GetDayOfTheMonth(size_t jd){ return GetDayOfTheMonth(0, jd); }
	size_t GetMonthIndex(int year, size_t jd);
	size_t GetMonthIndex(const char* month);
	inline size_t GetMonthIndex(size_t jd){ return GetMonthIndex(0, jd); }

	__int32 GetRef(int year, size_t m, size_t d);
	void SetRef(__int32 jd, int& year, size_t& month, size_t& day);

	std::string GetTimeSpanStr(double sec);



	class CTM
	{
	public:

		enum TType{ UNKNOWN = 7, ANNUAL = 0, MONTHLY, DAILY, HOURLY, ATEMPORAL, NB_REFERENCE };
		enum TMode{ FOR_EACH_YEAR, OVERALL_YEARS, NB_MODE };

		const char* GetTypeModeName()const{ return TYPE_MODE_NAME[m_type][m_mode]; }

		static void ReloadString();
		static const char* GetTypeName(size_t TType);
		static const char* GetModeName(size_t TMode);
		static const char* GetTypeTitle(size_t TType);
		static const char* GetModeTitle(size_t TMode);
		static bool IsTypeAvailable(CTM TM1, size_t t){ return AVAILABILITY[TM1.i()][CTM(t, FOR_EACH_YEAR).i()] || AVAILABILITY[TM1.i()][CTM(t, OVERALL_YEARS).i()]; }
		static bool IsModeAvailable(CTM TM1, CTM TM2){ return AVAILABILITY[TM1.i()][TM2.i()]; }

		bool operator==(CTM TM)const{ return m_type == TM.m_type && m_mode == TM.m_mode; }
		bool operator!=(CTM TM)const{ return !operator==(TM); }

		std::string GetTypeName()const{ return GetTypeName(Type()); }
		std::string GetModeName()const{ return GetModeName(Mode()); }
		bool IsTypeAvailable(short t)const{ return IsTypeAvailable(*this, t); }
		bool IsModeAvailable(CTM TM)const{ return IsModeAvailable(*this, TM); }
		size_t GetNbTimeReferences()const{ ASSERT(IsValid()); return NB_REFERENCES[m_type][m_mode]; }

		CTM(size_t t = UNKNOWN, size_t m = FOR_EACH_YEAR)
		{
			m_type = (short)t;
			m_mode = (short)m;
			assert(IsValid());
		}

		CTM(const CTM& in)
		{
			m_type = in.m_type;
			m_mode = in.m_mode;
			assert(IsValid());
		}

		void clear() { m_type = UNKNOWN;	m_mode = FOR_EACH_YEAR; }
		bool IsInit()const { _ASSERTE(IsValid()); return m_type != UNKNOWN; }
		bool IsValid()const { return ((m_type == UNKNOWN || m_type == NOT_INIT) || (Type() >= ANNUAL&&Type() < NB_REFERENCE)) && (Mode() == UNKNOWN || Mode() == NOT_INIT || (Mode() >= FOR_EACH_YEAR&&Mode() < NB_MODE)); }
		bool IsHourly()const { return m_type == HOURLY; }
		bool IsDaily()const { return m_type == DAILY;  }
		size_t Type()const{ return (size_t)m_type; }
		size_t Mode()const{ return (size_t)m_mode; }

		std::istream& operator << (std::istream& io){ return io >> m_type >> m_mode; }
		std::ostream& operator >> (std::ostream& io)const{ return io << m_type << " " << m_mode; }
		friend std::ostream& operator<<(std::ostream &s, const CTM& TM){ TM >> s; return s; }
		friend std::istream& operator>>(std::istream &s, CTM& TM){ TM << s;	return s; }

		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & m_type & m_mode;
		}

	protected:

		short i()const{ return (m_type != UNKNOWN) ? (m_type + (m_mode*NB_REFERENCE)) : -1; }
		//operator short()const{ return (m_type != UNKNOWN) ? (m_type + (m_mode*NB_REFERENCE)) : -1; }

		short m_type;
		short m_mode;


		static const char* TYPE_NAME[8];
		static const char* MODE_NAME[NB_MODE];
		static const bool AVAILABILITY[NB_REFERENCE*NB_MODE][NB_REFERENCE*NB_MODE];
		static const char* TYPE_MODE_NAME[8][NB_MODE];
		static const char NB_REFERENCES[8][NB_MODE];

		static StringVector TYPE_TITLE;
		static StringVector MODE_TITLE;
	};



	class CTRefFormat
	{
	public:

		enum TFormat{ YEAR, MONTH, DAY, JDAY, HOUR, REFERENCE, NB_FORMAT };
		static const char* GetFormatName(size_t i){ _ASSERTE(i < NB_FORMAT); return FORMAT_NAME[i]; }


		static const char* DATE_YMD;
		static const char* DATE_YMDH;
		static const char* DATE_YMD_HMS;

		static const char* GetDefaultHeader(CTM tm){ return DEFAULT_HEADER[tm.Type()][tm.Mode()]; }
		static const char* GetDefaultFormat(CTM tm){ return DEFAULT_FORMAT[tm.Type()][tm.Mode()]; }


		CTRefFormat()
		{
			for (int t = 0; t < CTM::NB_REFERENCE; t++)
			{
				for (int m = 0; m < CTM::NB_MODE; m++)
				{
					m_header[t][m] = DEFAULT_HEADER[t][m];
					m_format[t][m] = DEFAULT_FORMAT[t][m];
				}
			}
		}

		const char* GetHeader(CTM tm)const{ ASSERT(tm.IsInit());  return m_header[tm.Type()][tm.Mode()].c_str(); }
		void SetHeader(CTM tm, const char* format){ m_header[tm.Type()][tm.Mode()] = format; }

		const char* GetFormat(CTM tm)const{ ASSERT(tm.IsInit()); return m_format[tm.Type()][tm.Mode()].c_str(); }
		void SetFormat(CTM tm, const char* format){ m_format[tm.Type()][tm.Mode()] = format; }

		std::string GetMissingString(CTM tm)const;

	protected:

		std::string m_format[CTM::NB_REFERENCE][CTM::NB_MODE];
		std::string m_header[CTM::NB_REFERENCE][CTM::NB_MODE];
		static const char* DEFAULT_FORMAT[CTM::NB_REFERENCE][CTM::NB_MODE];
		static const char* DEFAULT_HEADER[CTM::NB_REFERENCE][CTM::NB_MODE];

		static const char* FORMAT_NAME[NB_FORMAT];
	};


	//Temporal reference
	class CTRef
	{
	public:


		

		enum TType{ UNKNOWN = 7, ANNUAL = 0, MONTHLY, DAILY, HOURLY, ATEMPORAL, NB_REFERENCE };
		enum TMode{ FOR_EACH_YEAR, OVERALL_YEARS, NB_MODE };

		static const CTRefFormat& GetFormat(){ return TREF_FORMAT; }
		static void SetFormat(const CTRefFormat& format){ TREF_FORMAT = format; }

		static CTRef GetCurrentTRef(CTM TM = CTM(DAILY));

		CTM GetTM()const{ return CTM(m_type, m_mode); }
		void SetTM(const CTM& TM){ m_type = TM.Type(); m_mode = TM.Mode(); }

		operator const float()const
		{
			const void* pBits = this;
			return *((float*)pBits);
		}

		operator const double()const
		{
			double d = double(Get__int32());
			const void* pBits = &d;
			return *((double*)pBits);
		}


		CTRef();
		explicit CTRef(float in);
		explicit CTRef(double in);

		__int32 Get__int32()const
		{
			const void* pBits = this;
			return *((__int32*)pBits);
		}

		void Set__int32(__int32 in)
		{
			const void* pBits = this;
			*((__int32*)pBits) = in;

		}

		//explicit CTRef(int y);

		CTRef(int year);
		CTRef(int year, size_t m);
		CTRef(int year, size_t m, size_t d);
		CTRef(int year, size_t m, size_t d, size_t h, CTM TM = CTM(CTM::HOURLY));

		CTRef(const std::string& str);
		CTRef(const std::string& str, CTM TM);


		CTRef& Set(int y_or_r, size_t m, size_t d, size_t h, CTM TM);

		enum TTranformInitType { FIRST_TREF, MID_TREF, LAST_TREF, NB_TRANSFORM_INIT };
		CTRef& Transform(const CTM& TM, size_t initType = FIRST_TREF);
		CTRef& Transform(const CTRef& t, size_t initType = FIRST_TREF){ return Transform(t.GetTM(), initType); }//{ m_type = t.m_type; m_mode = t.m_mode; ResetUnused(); return *this; }
		static CTRef Transform(CTRef t, CTM TM, size_t initType = FIRST_TREF){ CTRef tt(t); tt.Transform(TM, initType); return tt; }
		CTRef as(const CTM& TM, size_t initType = FIRST_TREF)const{ CTRef tt(*this); tt.Transform(TM, initType); return tt; }


		CTRef Disaggregate(const CTM& TM)const;
		bool IsInit()const{ return m_type != UNKNOWN; }

		void Reset();
		void clear(){ Reset(); }

		CTRef& operator =(float in);
		CTRef& operator =(double in);
		CTRef& operator =(const CTRef& in);
		bool operator == (const CTRef& in)const;
		bool operator != (const CTRef& in)const{ return !operator==(in); }
		bool operator > (const CTRef& in)const{ return ((IsInit() && in.IsInit()) ? GetRef() > in.GetRef() :false); }
		bool operator < (const CTRef& in)const{ return ((IsInit() && in.IsInit()) ? GetRef() < in.GetRef() : false); }
		bool operator >= (const CTRef& in)const{ return ((IsInit() && in.IsInit()) ? GetRef() >= in.GetRef() : false); }
		bool operator <= (const CTRef& in)const{ return ((IsInit() && in.IsInit()) ? GetRef() <= in.GetRef() : false); }
		CTRef& operator+=(__int32 nbRef){ return Shift(nbRef); }
		CTRef& operator-=(__int32 nbRef){ return Shift(-nbRef); }
		__int32 operator-(const CTRef& in)const;
		CTRef operator+(__int32 nbRef)const;
		CTRef operator-(__int32 nbRef)const;
		CTRef operator+(long nbRef)const{ return operator+(__int32(nbRef)); }
		CTRef operator-(long nbRef)const{ return operator+(__int32(-nbRef)); }
		CTRef operator+(size_t nbRef)const{ return operator+(__int32(nbRef)); }
		CTRef operator-(size_t nbRef)const{ return operator-(__int32(nbRef)); }
		CTRef& operator++(int){ return Shift(1); }
		CTRef& operator--(int){ return Shift(-1); }
		CTRef& Shift(__int32 nbRef);

		__int32 GetRef()const;
		void SetRef(__int32 ref, CTM TM);


		bool IsInversed(CTRef in)const
		{
			bool bRep = false;
			if (m_mode == FOR_EACH_YEAR)
			{
				in.m_year = m_year;//remove year and look if it's greater
				bRep = *this > in;
			}

			return bRep;
		}

		bool IsAnnual()const{ return m_type != UNKNOWN&&m_mode == FOR_EACH_YEAR; }
		bool HaveYear()const{ return IsAnnual() && m_type != ATEMPORAL; }
		bool HaveMonth()const{ return m_type >= MONTHLY&&m_type != ATEMPORAL; }
		bool HaveDay()const{ return m_type >= DAILY&&m_type != ATEMPORAL; }
		bool HaveHour()const{ return m_type >= HOURLY&&m_type != ATEMPORAL; }
		bool IsTemporal()const{ return m_type != ATEMPORAL; }
		bool IsReference()const{ return m_type == ATEMPORAL; }

		std::string ToString()const;
		void FromString(const std::string& str);


		static std::string GetMissingString(CTM tm);

		std::string GetFormatedString(std::string format = "")const;
		void FromFormatedString(std::string str, std::string format = "", const char* sep = "-", int base = 1);
		bool IsValid()const;

		short GetType()const{ return m_type; }
		short GetMode()const{ return m_mode; }
		size_t GetHour()const { return IsInit() ? size_t(m_hour) : 0; }
		size_t GetDay()const { return IsInit() ? size_t(m_day) : 0; }
		size_t GetMonth()const { return IsInit() ? size_t(m_month) : 0; }
		int GetYear()const { return IsInit() ? int(m_year) : YEAR_NOT_INIT; }
		size_t GetJDay()const { return (m_type == HOURLY || m_type == DAILY) ? WBSF::GetJDay(m_year, m_month, m_day) : 0; }
		void SetJDay(int year, size_t jd);
		void SetJDay(size_t jd);
		void ResetUnused();

		union
		{
			struct
			{
				unsigned __int32 m_hour : 5;    // 0..32 (5 bits)
				unsigned __int32 m_day : 5;    // 0..32 (5 bits)
				unsigned __int32 m_month : 4;    // 0..16 (4 bits)
				signed   __int32 m_year : 14;   // -8191..8192  (14 bits)
				unsigned __int32 m_type : 3;    // 0..7 (3 bits)
				unsigned __int32 m_mode : 1;    // 0..1 (1 bits)
			};

			struct
			{
				signed   __int32 m_ref : 28;	//-134,217,727..134,217,728 (28 bits)
				unsigned __int32 m_type : 3;    // type
				unsigned __int32 m_mode : 1;	// mode
			};
		};

		size_t GetNbDaysPerYear()const{ return WBSF::GetNbDaysPerYear(m_year); }
		size_t GetNbDayPerMonth()const{ return GetNbDayPerMonth(m_year, m_month); }
		short IsLeap()const{ return WBSF::IsLeap(m_year); }

		//static method
		static const char* GetMonthName(size_t m, bool bFull = true){ return WBSF::GetMonthName(m, bFull); }
		static size_t GetNbDayPerMonth(size_t m){ return WBSF::GetNbDayPerMonth(m); }
		static double MidDayInMonth(size_t m){ return WBSF::MidDayInMonth(m); }
		static short IsLeap(int year){ return WBSF::IsLeap(year); }
		static size_t GetNbDaysPerYear(int year){ return WBSF::GetNbDaysPerYear(year); }
		static size_t GetNbDayPerMonth(int year, size_t m){ return WBSF::GetNbDayPerMonth(year, m); }
		static size_t GetFirstJdayForMonth(int year, size_t m){ return WBSF::GetFirstJdayForMonth(year, m); }
		static size_t GetFirstJdayForMonth(size_t m){ return WBSF::GetFirstJdayForMonth(m); }
		static size_t GetJDay(int year, size_t m, size_t d){ return WBSF::GetJDay(year, m, d); }
		static size_t GetDayOfTheMonth(int year, size_t jd){ return WBSF::GetDayOfTheMonth(year, jd); }
		static size_t GetDayOfTheMonth(size_t jd){ return WBSF::GetDayOfTheMonth(jd); }
		static size_t GetMonthIndex(int year, size_t jd){ return WBSF::GetMonthIndex(year, jd); }
		static size_t GetMonthIndex(const char* month){ return WBSF::GetMonthIndex(month); }
		static size_t GetMonthIndex(size_t jd){ return WBSF::GetMonthIndex(jd); }

		__int32 GetRef(int year, size_t m, size_t d);
		void SetRef(__int32 ref, int& year, size_t& m, size_t& d);

		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			__int32 me = Get__int32();
			ar & me;
			Set__int32(me);
		}

		std::istream& operator << (std::istream& s)
		{
			std::string tmp;
			s >> tmp;
			FromString(tmp);

			return s;
		}
		std::ostream& operator >> (std::ostream& s)const
		{
			std::string tmp = ToString();
			s << tmp;
			return s;
		}
		friend std::istream& operator >>(std::istream& s, CTRef& Tref){ Tref << s; return s; }
		friend std::ostream& operator <<(std::ostream& s, const CTRef& Tref){ Tref >> s; return s; }


	protected:

		//static short TM(short t, short m){ ASSERT(t>=0&&t<NB_REFERENCE);ASSERT(m>=0&&m<NB_MODE);return t+(m*NB_REFERENCE);}
		//CTM TM()const{ ASSERT(IsInit());return CTM(m_type, m_mode);}

		static CTRefFormat TREF_FORMAT;

	};

	inline int GetCurrentYear(){ return CTRef::GetCurrentTRef().GetYear(); }
	inline CTRef FromFormatedString(std::string str, std::string format = "", const char* sep = "-", int base = 1){ CTRef TRef; TRef.FromFormatedString(str, format, sep, base); return TRef; }
	

	class CJDayRef : public CTRef
	{
	public:

		CJDayRef(size_t jd)
		{
			SetJDay(jd);
		}

		CJDayRef(int year, size_t jd)
		{
			SetJDay(year, jd);
		}
	};


	class CTimeRef : public CTRef
	{
	public:

		CTimeRef(__time64_t t, CTM TM = CTM(CTM::DAILY))
		{
			m_time = t;
			if (t > 0)
			{
				tm theTime = { 0 };
				_localtime64_s(&theTime, &t);
				Set(1900 + theTime.tm_year, theTime.tm_mon, theTime.tm_mday - 1, theTime.tm_hour, TM);
			}
		}


		__time64_t m_time;
	};

	
	typedef std::vector<CTRef>CTRefVector;

	//*************************************************************
	//CTPeriod

	class CTPeriod
	{
	public:

		enum TType{ CONTINUOUS, YEAR_BY_YEAR };
		enum TTType{
			UNKNOWN = CTRef::UNKNOWN,
			ANNUAL = CTRef::ANNUAL,
			MONTHLY = CTRef::MONTHLY,
			DAILY = CTRef::DAILY,
			HOURLY = CTRef::HOURLY,
			ATEMPORAL = CTRef::ATEMPORAL,
			NB_REFERENCE
		};

		enum TMode
		{
			FOR_EACH_YEAR = CTRef::FOR_EACH_YEAR,
			OVERALL_YEARS = CTRef::OVERALL_YEARS,
			NB_MODE
		};

		CTPeriod(const CTRef& begin = CTRef(), const CTRef& end = CTRef(), short m_type = CONTINUOUS);
		CTPeriod(int y1, int y2, int type = CONTINUOUS);
		CTPeriod(int y1, size_t  m1, int y2, size_t m2, int type);
		CTPeriod(int y1, size_t  m1, size_t  d1, int y2, size_t m2, size_t d2, int type = CONTINUOUS);

		void Reset();
		void clear(){ Reset(); }

		CTPeriod& operator = (const CTPeriod& in);
		CTPeriod& Inflate(const CTRef& in);
		CTPeriod& Inflate(const CTPeriod& in);

		bool operator == (const CTPeriod& in)const;
		bool operator != (const CTPeriod& in)const{ return !operator==(in); }
		CTPeriod& operator += (const CTRef& in){ return Inflate(in); }
		CTPeriod& operator += (const CTPeriod& in){ return Inflate(in); }
		CTRef operator[](size_t i)const{ return m_begin + (int)i; }

		const CTRef& Begin()const { return m_begin; }
		CTRef& Begin(){ return m_begin; }
		const CTRef& End()const { return m_end; }
		CTRef& End(){ return m_end; }
		__int32 GetLength()const{ return IsInit() ? (m_end - m_begin) + 1 : 0; }

		short GetType()const{ return m_type; }
		void SetType(short type){ m_type = type; }
		short GetTType()const{ ASSERT(m_begin.m_type == m_end.m_type); return m_begin.m_type; }
		short GetTMode()const{ ASSERT(m_begin.m_mode == m_end.m_mode); return m_begin.m_mode; }

		int GetFirstYear()const{ return m_begin.m_year; }
		int GetLastYear()const{ return m_end.m_year; }
		size_t GetNbYears()const{ return GetLastYear() - GetFirstYear() + 1; }
		__int32 GetNbMonth()const{ return __int32(GetNbYears()) * 12; }
		__int32 GetNbDay()const{ ASSERT(GetTType() == DAILY); return GetNbRef(); }
		__int32 GetNbDay2()const;
		__int32 GetNbHour()const{ return GetNbDay2() * 24; }
		__int32 GetNbRef()const{ return IsInit() ? (m_end.GetRef() - m_begin.GetRef() + 1) : 0; }
		__int32 GetRefPos(const CTRef& t)const{ ASSERT(t.GetTM() == GetTM()); return t.GetRef() - m_begin.GetRef(); }
		bool IsInit()const{ return m_begin.IsInit() && m_end.IsInit(); }
		CTRef GetFirstAnnualTRef(int y)const;
		CTRef GetLastAnnualTRef(int y)const;
		int GetAnnualSize(int y)const{ return (GetLastAnnualTRef(y) - GetFirstAnnualTRef(y)) + 1; }
		CTPeriod GetAnnualPeriodByIndex(int y)const{ return CTPeriod(GetFirstAnnualTRef(y), GetLastAnnualTRef(y)); }
		CTPeriod GetAnnualPeriod(int year)const;
		size_t size()const{ return GetNbRef(); }

		CTM GetTM()const{ return m_begin.GetTM(); }
		void SetTM(const CTM& TM){ m_begin.SetTM(TM); m_end.SetTM(TM); }

		CTPeriod Get(const CTM& TM);
		CTPeriod& Transform(const CTM& TM);
		CTPeriod& Transform(const CTPeriod& p){ return Transform(p.GetTM()); }

		void FromFormatedString(std::string str, std::string periodformat = "[%1|%2]", std::string TRefFormat = "", const char* sep = "-", int base = 1);
		std::string GetFormatedString(std::string periodformat = "[%1|%2]", std::string TRefFormat = "")const;
		std::string ToString()const;
		void FromString(const std::string& str);

		bool IsInside(CTRef ref)const;
		bool IsInside(const CTPeriod& p)const;
		bool IsIntersect(const CTPeriod& p)const;

		CTPeriod Intersect(const CTPeriod& p)const;
		size_t GetNbSegments()const;
		int GetSegmentIndex(CTRef ref)const;
		CTPeriod GetSegment(size_t i)const;

		bool IsAnnual()const{ ASSERT(m_begin.m_type == m_end.m_type); return m_begin.IsAnnual(); }
		bool IsInversed()const
		{
			_ASSERTE((m_type == CONTINUOUS) || (m_begin.m_month != m_end.m_month) || m_begin.m_day <= m_end.m_day);
			return  (m_type == YEAR_BY_YEAR) && m_begin.IsInversed(m_end);
		}

		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & m_type & m_begin & m_end;
		}

		std::istream& operator << (std::istream& s){ s >> m_type >> m_begin >> m_end;	return s; }
		std::ostream& operator >> (std::ostream& s)const	{ s << m_type << " " << m_begin << " " << m_end; return s; }
		friend std::istream& operator >>(std::istream& s, CTPeriod& p){ p << s; return s; }
		friend std::ostream& operator <<(std::ostream& s, const CTPeriod& p){ p >> s; return s; }


#ifdef _MFC_VER
		CArchive& operator << ( CArchive& ar)
		{
			ar >> m_type;
			__int32 tmp;
			ar >> tmp; m_begin.Set__int32(tmp);
			ar >> tmp; m_end.Set__int32(tmp);
			return ar;
		}
		CArchive& operator >> ( CArchive& ar)const
		{
			ar << m_type;
			ar << m_begin.Get__int32();
			ar << m_begin.Get__int32();

			return ar;
		}
		friend CArchive& operator >>(CArchive& ar, CTPeriod& p){ p<<ar; return ar;}
		friend CArchive& operator <<(CArchive& ar, const CTPeriod& p){ p>>ar; return ar;}

#endif

		void WriteStream(std::ostream& stream)const;
		void ReadStream(std::istream& stream);

	protected:


		bool IsOutsideInclusive(CTRef ref)const;

		short m_type;//YEAR_BY_YEAR, CONTINUOUS
		CTRef m_begin;
		CTRef m_end;

	};




	class CJDayPeriod : public CTPeriod
	{
	public:
		CJDayPeriod(int year, short Jday1, short Jday2);
		CJDayPeriod(short firtYear, short Jday1, short lastYear, short Jday2, int type = CONTINUOUS);
	};



	template <class T>
	class CTReferencedVectorIterator
	{
	public:

		CTReferencedVectorIterator(T& vector, CTRef Tref)
		{
			m_vector = vector;
			m_Tref = Tref;//actual lkevel of the reference
		}

		CTReferencedVectorIterator operator[](size_t i)
		{
			assert(m_Tref.GetTM() != m_vector.GetTM());//to musch level of indirection

			CTM TM(m_Tref.GetTM().Type() + 1, m_Tref.GetTM().Mode());
			return CTReferencedVectorIterator(m_vector, CTRef(m_Tref).Transform(TM));
		}

		operator T&(){ return m_vector[m_Tref]; }

	protected:

		T& m_vector;
		CTRef m_Tref;
	};

	template <class T>
	class CTReferencedVectorConstIterator
	{
	public:

		CTReferencedVectorConstIterator(const T& vector, CTRef Tref)
		{
			m_vector = vector;
			m_Tref = Tref;//actual lkevel of the reference
		}

		CTReferencedVectorConstIterator operator[](size_t i)const
		{
			assert(m_Tref.GetTM() != m_vector.GetTM());//to musch level of indirection

			CTM TM(m_Tref.GetTM().Type() + 1, m_Tref.GetTM().Mode());
			return CTReferencedVectorConstIterator(m_vector, CTRef(m_Tref).Transform(TM));
		}

		operator const T&()const{ return m_vector[m_Tref]; }

	protected:

		const T& m_vector;
		CTRef m_Tref;
	};

	template <class T>
	class CTReferencedVector : public std::vector < T >
	{
	public:

		CTPeriod m_period;

		CTReferencedVector(CTPeriod period = CTPeriod())//, size_t timeStep = 1)//timeStep
		{
			Init();
		}

		void Init(CTPeriod period = CTPeriod())
		{
			m_period = period;
			resize(m_period.GetNbRef());
		}

		CTReferencedVector(const CTReferencedVector& in){ operator=(in); }

		CTReferencedVector& operator=(const CTReferencedVector& in)
		{
			if (&in != this)
			{
				std::vector<T>::operator =(in);
				m_period = in.m_period;
			}

			return *this;
		}

		CTRef GetFirstTRef()const{ return m_period.Begin(); }
		CTM GetTM()const{ return m_period.GetTM(); }
		bool IsInit()const{ return m_period.IsInit(); }

		using std::vector<T>::at;
		using std::vector<T>::operator[];
		const T& operator[](const CTRef& TRef)const{ return at(TRef); }
		T& operator[](const CTRef& TRef){ return at(TRef); }
		const T& at(const CTRef& TRef)const{ return std::vector<T>::operator[]((TRef - m_period.Begin()) /* m_timeStep*/); }
		T& at(const CTRef& TRef){ return std::vector<T>::operator[]((TRef - m_period.Begin()) /* m_timeStep*/); }

		CTRef GetTRef(size_t i)const{ return (m_period.Begin() + i)/*m_timeStep*/; }

		//protected:


	};

	//*************************************************************
	//CTTransformation

	class CTTransformation
	{
	public:

		CTTransformation(const CTPeriod& p, CTM TM);
		CTTransformation(const CTPeriod& pIn, const CTPeriod& pOut);

		void ComputeCluster(const CTPeriod& p, CTM TM);
		void ComputeCluster(const CTPeriod& p, const CTPeriod& pOut);

		size_t GetNbCluster()const{ return m_clusters.size(); }
		CTRef GetClusterTRef(size_t c)const{ return (c < m_clusters.size() ? m_clusters[c] : CTRef()); }
		size_t GetCluster(CTRef t)const;

		CTM GetTMIn()const{ return m_pIn.GetTM(); }
		CTM GetTMOut()const{ return m_pOut.GetTM(); }
		
		bool IsInit()const{ return m_pIn.IsInit() && m_pOut.IsInit() && m_pIn!=m_pOut; }
		bool IsInside(CTRef TRef)const;
		CTPeriod GetPeriodIn()const{ return m_pIn; }
		CTPeriod GetPeriodOut()const{ return m_pOut; }

	protected:

		CTRefVector m_clusters;
		//full output period
		CTPeriod m_pIn;
		CTPeriod m_pOut;
	};



	class CSun
	{
	public:

		CSun(double lat, double lon, double zone = -999);

		double GetSunrise(CTRef Tref)const{ Compute(Tref.GetYear(), Tref.GetMonth(), Tref.GetDay()); return m_sunrise; }
		double GetSunset(CTRef Tref)const{ Compute(Tref.GetYear(), Tref.GetMonth(), Tref.GetDay()); return m_sunset; }
		double GetSolarNoon(CTRef Tref)const{ Compute(Tref.GetYear(), Tref.GetMonth(), Tref.GetDay()); return m_solarNoon; }
		double GetDayLength(CTRef Tref)const{ Compute(Tref.GetYear(), Tref.GetMonth(), Tref.GetDay()); return m_daylength; }


		double GetSunrise(int year, size_t m, size_t d)const{ Compute(year, m, d); return m_sunrise; }
		double GetSunset(int year, size_t m, size_t d)const{ Compute(year, m, d); return m_sunset; }
		double GetSolarNoon(int year, size_t m, size_t d)const{ Compute(year, m, d); return m_solarNoon; }
		double GetDayLength(int year, size_t m, size_t d)const{ Compute(year, m, d); return m_daylength; }


		double GetLat()const{ return m_lat; }
		double GetLon()const{ return m_lon; }
		double GetZone()const{ return m_zone; }

	private:


		//input 
		double m_lat;
		double m_lon;
		double m_zone;

		//output
		short m_year;
		short m_month;
		short m_day;
		double m_sunrise;
		double m_sunset;
		double m_daylength;
		double m_solarNoon;


		void Compute(int year, size_t m, size_t d)const;
		static void Compute(double lat, double lon, double zone, int year, size_t m, size_t d, double& sunrise, double& sunset, double& solarNoon, double& daylength);

		// Get the days to J2000
		// h is UT in decimal hours
		// FNday only works between 1901 to 2099 - see Meeus chapter 7
		static double FNday(int year, int month, int day, float h);

		// the function below returns an angle in the range ( 0 to 2*pi )
		static double FNrange(double x);

		// Calculating the hourangle
		static double f0(double lat, double declin);

		// Calculating the hourangle for twilight times
		static double f1(double lat, double declin);

		// Find the ecliptic __int32itude of the Sun
		static double FNsun(double d);

		static const double pi;
		static const double tpi;
		static const double degs;
		static const double rads;

	};

	double GetDayLength(double latDeg, CTRef d);
	inline double GetDayLength(double latDeg, size_t jd){ return GetDayLength(latDeg, CJDayRef(2000, jd)); }
	inline double GetDayLength(double latDeg, size_t m, size_t d){ return GetDayLength(latDeg, CTRef(2000, m, d)); }

	std::string FormatTime(const std::string& format, __time64_t t);
	std::string FormatTime(const std::string& format, int year, size_t m, size_t d, size_t h = 12, size_t min = 0, size_t sec = 0);

	std::string GetCurrentTimeString(const std::string format = "%#c");

}//namespace WBSF



namespace std
{
	inline string to_string(const WBSF::CTM& in){ return WBSF::ToString(in); }
	inline WBSF::CTM to_CTM(const string& str){ return WBSF::ToObject<WBSF::CTM>(str); }
}

namespace zen
{
	
	template <> inline
		void writeStruc(const WBSF::CTRef& in, XmlElement& output)
	{
		output.setValue(in.ToString());
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CTRef& out)
	{
		std::string str;
		input.getValue(str);
		out.FromString(str);

		return true;
	}
	
	template <> inline
		void writeStruc(const WBSF::CTM& TM, XmlElement& output)
	{
		output.setValue(std::to_string(TM));
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CTM& TM)
	{
		std::string str;
		input.getValue(str);
		TM = std::to_CTM(str);

		return true;
	}


	template <> inline
		void writeStruc(const WBSF::CTPeriod& in, XmlElement& output)
	{
		output.setValue(in.ToString());
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CTPeriod& out)
	{
		std::string str;
		input.getValue(str);
		out.FromString(str);

		return true;
	}

}