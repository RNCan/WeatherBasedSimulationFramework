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


#include "Basic/UtilZen.h"

namespace WBSF
{
	class CVariableDefine
	{
	public:



		static const char* GetXMLFlag(){ return XML_FLAG; }
		static const char* XML_FLAG;

		CVariableDefine(int dimension = -1, int field = -1/*,const std::string name = ""*/);
		void Reset();
		CVariableDefine& operator=(const CVariableDefine& in);
		bool operator==(const CVariableDefine& in)const;
		bool operator!=(const CVariableDefine& in)const{ return !operator==(in); }


		int m_dimension;
		int m_field;

	};

	typedef std::vector<WBSF::CVariableDefine> CVariableDefineVector;
}

namespace zen
{

	template <> inline
		void writeStruc(const WBSF::CVariableDefine& in, XmlElement& output)
	{
		output.setAttribute("Dimension",in.m_dimension);
		output.setAttribute("Field",in.m_field);
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CVariableDefine& out)
	{
		input.getAttribute("Dimension",out.m_dimension);
		input.getAttribute("Field",out.m_field);

		return true;
	}

	template <> inline
		void writeStruc(const WBSF::CVariableDefineVector& in, XmlElement& output)
	{
		writeStruc2<WBSF::CVariableDefine>(in, output);
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CVariableDefineVector& out)
	{
		return readStruc2<WBSF::CVariableDefine>(input, out);
	}
}
