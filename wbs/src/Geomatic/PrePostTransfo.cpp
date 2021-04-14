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
#include "Basic/UtilMath.h"
#include "Basic/UtilStd.h"

#include "Geomatic/PrePostTransfo.h"

using namespace std;


namespace WBSF
{

	const char* CPrePostTransfo::TypeName[] = { "-----", "log", "logit" };
	const char* CPrePostTransfo::LogTypeName[] = { "x", "x+1" };
	const char* CPrePostTransfo::LogitTypeName[] = { "x", "(%dx+1)/(%d+2)" };

	const char* CPrePostTransfo::XML_FLAG = "PrePostTransfo";
	const char* CPrePostTransfo::MEMBER_NAME[NB_MEMBER] = { "Type", "LogType", "UseXprime", "n", "InPercent"};


	CPrePostTransfo::CPrePostTransfo()
	{
		Reset();
	}

	void CPrePostTransfo::Reset()
	{
		m_type = NO_TRANFO;
		m_logType = LOG_X;
		m_bUseXPrime = true;
		m_bDataInPercent = false;
		m_n = 300;
	}

	CPrePostTransfo::CPrePostTransfo(const CPrePostTransfo& in)
	{
		operator=(in);
	}


	CPrePostTransfo& CPrePostTransfo::operator=(const CPrePostTransfo& in)
	{
		if (&in != this)
		{
			m_type = in.m_type;
			m_logType = in.m_logType;
			m_bUseXPrime = in.m_bUseXPrime;
			m_bDataInPercent = in.m_bDataInPercent;
			m_n = in.m_n;
		}

		ASSERT(*this == in);
		return *this;
	}

	bool CPrePostTransfo::operator == (const CPrePostTransfo& in)const
	{
		bool bEqual = true;

		if (m_type != in.m_type)bEqual = false;
		if (m_logType != in.m_logType)bEqual = false;
		if (m_bUseXPrime != in.m_bUseXPrime)bEqual = false;
		if (m_bDataInPercent != in.m_bDataInPercent)bEqual = false;
		if (m_n != in.m_n)bEqual = false;

		return bEqual;
	}
	double CPrePostTransfo::Transform(double x)const
	{
		double y = -FLT_MAX;
		
		ASSERT(NB_TRANSFO == 3);
		switch (m_type)
		{
		case NO_TRANFO: y = x; break;
		case LOG_TRANFO: y = TransformLog(x); break;
		case LOGIT_TRANSFO: y = TransformLogit(x); break;
		default: ASSERT(false);
		}

		return y;
	}

	double CPrePostTransfo::InvertTransform(double y, double noData)const
	{
		double x = noData;

		
		if ( _finite(y))
		{
			ASSERT(NB_TRANSFO == 3);
			switch (m_type)
			{
			case NO_TRANFO:		x = y; break;
			case LOG_TRANFO:	x = InvertTransformLog(y); break;
			case LOGIT_TRANSFO: x = InvertTransformLogit(y); break;
			default: ASSERT(false);
			}
		}
		

		return x;

	}

	double CPrePostTransfo::TransformLog(double x)const
	{
		double y = 0;

		ASSERT(NB_LOG == 2);
		switch (m_logType)
		{
		case LOG_X: y = log10(x); break;
		case LOG_X1: y = log10(x + 1); break;
		default: ASSERT(false);
		}

		return y;
	}

	double CPrePostTransfo::InvertTransformLog(double y)const
	{

		double x = 0;

		ASSERT(NB_LOG == 2);
		switch (m_logType)
		{
		case LOG_X: x = pow(10.0, y); break;
		case LOG_X1: x = pow(10.0, y) - 1; break;
		default: ASSERT(false);
		}

		return x;
	}




	double CPrePostTransfo::TransformLogit(double x)const
	{
		if (m_bDataInPercent)
			x /= 100;

		if (m_bUseXPrime)
			x = (m_n*x + 1) / (m_n + 2);
	
		ASSERT(x>0 && x<1);
		double y = log(x / (1 - x));

		return y;
	}

	double CPrePostTransfo::InvertTransformLogit(double y)const
	{

		double x = exp(y);

		x = x / (1 + x);

		if (m_bUseXPrime)
			x = (((m_n + 2)*x) - 1) / m_n;

		if (m_bDataInPercent)
			x *= 100;

		ASSERT(x >= 0 && x <= 1);
		return x;
	}


	std::string CPrePostTransfo::GetDescription()const
	{
		ASSERT(NB_TRANSFO == 3);

		string str;
		switch (m_type)
		{
		case NO_TRANFO:  str = TypeName[m_type];  break;
		case LOG_TRANFO: str = string(TypeName[m_type]) + "(" + LogTypeName[m_logType] + ")";   break;
		case LOGIT_TRANSFO:
		{
			string tmp = LogitTypeName[0];

			if (m_bUseXPrime)
			{
				tmp = LogitTypeName[1];
				ReplaceString(tmp, "%d", ToString(m_n));
			}

			str = string(TypeName[m_type]) + "(" + tmp + ")";
			if (m_bDataInPercent)
				str += " %";
		}
		break;
		default: ASSERT(false);
		}

		return str;
	}

}