//*********************************************************************
// File: InputParam.h
//
// Class:	CParameter: hold input and output parameters
//          CLocation: hold simulation point
//			CMTGInput: hold Temperature generator parameter
//			CCounter: hold information about stage process
//
// Description: These class helper facilite transfer of input parameters.
//
// Note :
//*********************************************************************
#pragma once




#include <map>
//#include <math.h>
#include "basic/ERMsg.h"
#include "Basic/UtilMath.h"
#include "Basic/WeatherDefine.h"
//#include "XMLite.h"
#include "Basic/UtilTime.h"
#include "Basic/UtilStd.h"
#include "basic/zenXml.h"
#include "Basic/UtilZen.h"





namespace WBSF
{

	//****************************************************************
	// CParameter

	class CParameter
	{
	public:

		std::string m_name;
		std::string m_value;
		bool		m_bIsVariable;


		CParameter(const std::string& name = "", const std::string& value = "", bool bVar = false);
		void clear();

		bool operator ==(const CParameter& in)const;
		bool operator !=(const CParameter& in)const{ return !operator==(in); }

		friend std::istream& operator >> (std::istream& io, CParameter& parameter);
		std::istream& operator << (std::istream& io);

		const std::string& GetName()const{ return m_name; }
		void SetName(const std::string& name){ m_name = name; }

		size_t GetSizeT()const{ return ToSizeT(m_value); }
		int GetInt()const{ return ToInt(m_value); }
		bool GetBool()const{ return GetInt() != 0; }
		double GetReal()const{ return ToDouble(m_value); }
		float GetFloat()const{ return ToFloat(m_value); }
		const std::string& GetString()const{ return m_value; }
		CTRef GetTRef()const;

		void SetString(const std::string& value){ m_value = value; }
		void Set(float value){ m_value = ToString(value); }

		bool IsVariable()const{ return m_bIsVariable; }
	};


	typedef std::vector<CParameter> CParameterVectorBase;
	class CParameterVector : public CParameterVectorBase
	{
	public:

		CParameterVector(size_t size = 0) :CParameterVectorBase(size){}
	};


	

	//****************************************************************
	// CCounter

	class CCounter
	{
	public:

		CCounter(size_t no = 0, size_t total = 0);
		void Reset();
		void clear(){ Reset(); }

		bool operator ==(const CCounter& in)const;
		bool operator !=(const CCounter& in)const{ return !operator==(in); }
		friend std::istream& operator >> (std::istream& io, CCounter& counter);
		std::istream& operator << (std::istream& io);

		bool IsFirst()const{ return m_no == 0; }
		bool IsLast()const{ _ASSERTE(m_no >= 0 && m_no < m_total); return m_no == m_total - 1; }
		size_t GetNo()const{ _ASSERTE(m_no >= 0 && m_no < m_total); return m_no; }
		void SetNo(size_t no){ m_no = no; }
		void SetTotal(size_t total){ m_total = total; }
		size_t GetTotal()const{ return m_total; }

		std::string to_string()const{ return std::to_string(m_no) + "/" + std::to_string(m_total); }
		void from_string(const std::string& in)
		{
			StringVector counter(in, "/");
			if (counter.size() == 2)
			{
				m_no = (size_t)std::stoull(counter[0]);
				m_total = (size_t)std::stoull(counter[1]);
			}
		}

	private:

		size_t m_no;
		size_t m_total;
	};

	
	class CMonthDay
	{
	public:

		size_t m_month;
		size_t m_day;

		CMonthDay(size_t m = UNKNOWN_POS, size_t d = UNKNOWN_POS);
		CMonthDay(const std::string& date, const std::string& separator = "-/\\")
		{
			Set(date, separator);
		}

		void Set(const std::string& date = "", const std::string& delimiter = "-/\\");
		std::string Get(const std::string& delimiter = "/")const;

		bool IsValid()const;
		
		CTRef GetTRef(int year)const{ return CTRef(year, m_month, m_day); }
		CTRef GetTRef(const CTRef& TRef)const{ return GetTRef(TRef.GetYear()).Transform(TRef); }
		bool operator > (const CTRef& TRef)const{ return GetTRef(TRef) > TRef; }
		bool operator >= (const CTRef& TRef)const{ return GetTRef(TRef) >= TRef; }
		bool operator < (const CTRef& TRef)const{ return GetTRef(TRef) < TRef; }
		bool operator <= (const CTRef& TRef)const{ return GetTRef(TRef) <= TRef; }
		friend bool operator > (const CTRef& TRef, const CMonthDay& MD){ return MD < TRef; }
		friend bool operator >= (const CTRef& TRef, const CMonthDay& MD){ return MD <= TRef; }
		friend bool operator < (const CTRef& TRef, const CMonthDay& MD){ return MD > TRef; }
		friend bool operator <= (const CTRef& TRef, const CMonthDay& MD){ return MD >= TRef; }
	};

}


namespace zen
{
	template <> inline
		void writeStruc(const WBSF::CParameter& in, XmlElement& output)
	{
		output.setAttribute("Name", in.m_name);
		output.setAttribute("IsVariable", in.m_bIsVariable);
		output.setValue(in.m_value);
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CParameter& out)
	{
		input.getAttribute("Name", out.m_name);
		input.getAttribute("IsVariable", out.m_bIsVariable);
		input.getValue(out.m_value);
		return true;
	}


	template <> inline
		void writeStruc(const WBSF::CParameterVector& in, XmlElement& output)
	{
		zen::writeStruc3(in, output, "Parameter");
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CParameterVector& out)
	{
		return zen::readStruc3(input, out, "Parameter");
	}

	template <> inline
		void writeStruc(const WBSF::CCounter& in, XmlElement& output)
	{
		output.setValue(in.to_string());
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CCounter& out)
	{
		std::string value;
		if (input.getValue(value))
			out.from_string(value);

		return true;
	}
}
