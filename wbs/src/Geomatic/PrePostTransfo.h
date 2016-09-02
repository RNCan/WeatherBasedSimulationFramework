//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once


#include "Basic/UtilStd.h"

namespace WBSF
{
	//************************************************************************
	class CPrePostTransfo
	{
	public:

		enum TMember { TYPE, LOG_TYPE, USE_X, NUMBER, IN_PERCENT, NB_MEMBER };
		static const char* GetMemberName(int i){ ASSERT(i >= 0 && i < NB_MEMBER); return MEMBER_NAME[i]; }
		static const char* GetXMLFlag(){ return XML_FLAG; }


		enum TType{ NO_TRANFO, LOG_TRANFO, LOGIT_TRANSFO, NB_TRANSFO };
		static const char* TypeName[];

		enum TLogType{ LOG_X, LOG_X1, NB_LOG };
		static const char* LogTypeName[];

		static const char* LogitTypeName[];

		CPrePostTransfo();
		CPrePostTransfo(const CPrePostTransfo& in);
		void Reset();

		CPrePostTransfo& operator=(const CPrePostTransfo& in);
		bool operator == (const CPrePostTransfo& in)const;
		bool operator != (const CPrePostTransfo& in)const{ return !operator ==(in); }

		double Transform(double value)const;
		double InvertTransform(double value, double noData)const;
		std::string GetDescription()const;
		bool HaveTransformation()const{ return m_type != NO_TRANFO; }

		size_t m_type;

		//log
		size_t m_logType;

		//logit
		bool m_bDataInPercent;
		bool m_bUseXPrime;
		int m_n;

	protected:


		void UnitTest();


		double TransformLog(double value)const;
		double InvertTransformLog(double value)const;
		double TransformLogit(double value)const;
		double InvertTransformLogit(double value)const;


		static const char* XML_FLAG;
		static const char* MEMBER_NAME[NB_MEMBER];

	};

	class CPrePostTransfoPtr : public std::unique_ptr < CPrePostTransfo >
	{
	public:

		CPrePostTransfoPtr()
		{
			reset(new CPrePostTransfo);
		}
	};

}

namespace zen
{
	template <> inline
		void writeStruc(const WBSF::CPrePostTransfo& in, XmlElement& output)
	{
		XmlOut out(output);

		out[WBSF::CPrePostTransfo::GetMemberName(WBSF::CPrePostTransfo::TYPE)](in.m_type);
		out[WBSF::CPrePostTransfo::GetMemberName(WBSF::CPrePostTransfo::LOG_TYPE)](in.m_logType);
		out[WBSF::CPrePostTransfo::GetMemberName(WBSF::CPrePostTransfo::USE_X)](in.m_bUseXPrime);
		out[WBSF::CPrePostTransfo::GetMemberName(WBSF::CPrePostTransfo::NUMBER)](in.m_n);
		out[WBSF::CPrePostTransfo::GetMemberName(WBSF::CPrePostTransfo::IN_PERCENT)](in.m_bDataInPercent);
		
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CPrePostTransfo& out)
	{
		XmlIn in(input);
		in[WBSF::CPrePostTransfo::GetMemberName(WBSF::CPrePostTransfo::TYPE)](out.m_type);
		in[WBSF::CPrePostTransfo::GetMemberName(WBSF::CPrePostTransfo::LOG_TYPE)](out.m_logType);
		in[WBSF::CPrePostTransfo::GetMemberName(WBSF::CPrePostTransfo::USE_X)](out.m_bUseXPrime);
		in[WBSF::CPrePostTransfo::GetMemberName(WBSF::CPrePostTransfo::NUMBER)](out.m_n);
		in[WBSF::CPrePostTransfo::GetMemberName(WBSF::CPrePostTransfo::IN_PERCENT)](out.m_bDataInPercent);

		return true;
	}
}


