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
//****************************************************************************
#include "stdafx.h"
#include <boost/algorithm/string.hpp>

#include "Basic/FileStamp.h"
#include "Basic/UtilTime.h"

using namespace std;


namespace WBSF
{

	const char* CFileStamp::XML_FLAG = "FileStamp";
	const char* CFileStamp::MEMBER_NAME[NB_MEMBER] = { "Name", "Time", "Size" };


	CFileStamp::CFileStamp(const string& filePath, bool bFullName)
	{
		clear();

		if (!filePath.empty())
			SetFileStamp(filePath, bFullName);
	}

	string CFileStamp::Format()const
	{
		string timeStr = FormatTime("%d %B %y(%H:%M:%S)", m_time);

		string tmp = m_filePath + ", " + timeStr + ", " + ToString(m_size);
		return tmp;
	}
	
	ERMsg CFileStamp::SetFileStamp(const string& filePath, bool bFullName)
	{
		ERMsg msg;

		//CFileInfo info;
		if (GetFileInfo(filePath, *this))
		{
			if (!bFullName)
				m_filePath = GetFileName(m_filePath);
		}
		else
		{
			msg.asgType(ERMsg::ERREUR);
			//msg.ajoute ( UtilWin::GetCString( IDS_CMN_UNABLE_OPEN_READ));
		}

		return msg;
	}

}