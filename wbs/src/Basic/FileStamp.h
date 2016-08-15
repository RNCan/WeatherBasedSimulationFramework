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

#include <string>
#include <wtypes.h>
#include "basic/ERMsg.h"
#include "basic/zenXml.h"

#include "Basic/UtilStd.h"
#include "Basic/UtilZen.h"

namespace WBSF
{

	class CFileStamp : public CFileInfo
	{
	public:

		enum TMember { NAME, TIME, SIZE, NB_MEMBER };
		static const char* GetMemberName(int i){ _ASSERTE(i >= 0 && i < NB_MEMBER); return MEMBER_NAME[i]; }
		static const char* GetXMLFlag(){ return XML_FLAG; }



		CFileStamp(const std::string& filePath = "", bool bFullName = false);

		ERMsg SetFileStamp(const std::string& filePath, bool bFullName = false);

		std::string Format()const;

		ERMsg LoadXML(std::string filePath){ return zen::LoadXML(filePath, GetXMLFlag(), "1", *this); }
		ERMsg SaveXML(std::string filePath){ return zen::SaveXML(filePath, GetXMLFlag(), "1", *this); }

	protected:

		//std::string m_filePath;

		static const char* XML_FLAG;
		static const char* MEMBER_NAME[NB_MEMBER];
	};


	typedef std::vector<CFileStamp, const CFileStamp&> CFileStampVector;

}



namespace zen
{
	template <> inline
		void writeStruc(const WBSF::CFileStamp& in, XmlElement& output)
	{
		XmlOut out(output);
		out[WBSF::CFileStamp::GetMemberName(WBSF::CFileStamp::NAME)](in.m_filePath);
		out[WBSF::CFileStamp::GetMemberName(WBSF::CFileStamp::TIME)](in.m_time);
		out[WBSF::CFileStamp::GetMemberName(WBSF::CFileStamp::SIZE)](in.m_size);
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CFileStamp& out)
	{
		XmlIn in(input);

		in[WBSF::CFileStamp::GetMemberName(WBSF::CFileStamp::NAME)](out.m_filePath);
		in[WBSF::CFileStamp::GetMemberName(WBSF::CFileStamp::TIME)](out.m_time);
		in[WBSF::CFileStamp::GetMemberName(WBSF::CFileStamp::SIZE)](out.m_size);

		return true;
	}
}
