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

#include "Simulation/ExportDefine.h"

namespace WBSF
{

	class CExport
	{
	public:

		enum TMember { NAME, SCRIPT_NAME, AUTO_EXPORT, VARIABLES, STATISTIC, NB_MEMBER };

		static const char* GetMemberName(int i){ ASSERT(i >= 0 && i < NB_MEMBER); return MEMBER_NAME[i]; }

		CExport();
		CExport(const CExport& in);
		virtual ~CExport();

		void Reset();
		CExport& operator=(const CExport& in);
		bool operator==(const CExport& in)const;
		bool operator!=(const CExport& in)const{ return !operator==(in); }

		std::string m_fileName;
		std::string m_scriptName;
		bool m_bAutoExport;

		CVariableDefineVector	m_variables;
		std::string				m_statistic;


	protected:


		static const char* MEMBER_NAME[NB_MEMBER];
	};

}

namespace zen
{

	template <> inline
	void writeStruc(const WBSF::CExport& in, XmlElement& output)
	{
		XmlOut out(output);
		out[WBSF::CExport::GetMemberName(WBSF::CExport::NAME)](in.m_fileName);
		out[WBSF::CExport::GetMemberName(WBSF::CExport::SCRIPT_NAME)](in.m_scriptName);
		out[WBSF::CExport::GetMemberName(WBSF::CExport::AUTO_EXPORT)](in.m_bAutoExport);
		out[WBSF::CExport::GetMemberName(WBSF::CExport::VARIABLES)](in.m_variables);
		out[WBSF::CExport::GetMemberName(WBSF::CExport::STATISTIC)](in.m_statistic);
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CExport& out)
	{
		XmlIn in(input);
		
		in[WBSF::CExport::GetMemberName(WBSF::CExport::NAME)](out.m_fileName);
		in[WBSF::CExport::GetMemberName(WBSF::CExport::SCRIPT_NAME)](out.m_scriptName);
		in[WBSF::CExport::GetMemberName(WBSF::CExport::AUTO_EXPORT)](out.m_bAutoExport);
		in[WBSF::CExport::GetMemberName(WBSF::CExport::VARIABLES)](out.m_variables);
		in[WBSF::CExport::GetMemberName(WBSF::CExport::STATISTIC)](out.m_statistic);

		return true;
	}
}

