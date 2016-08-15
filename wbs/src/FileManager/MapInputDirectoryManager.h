//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include "DirectoryManager.h"
#include "FileManager/DirectoryManager.h"


namespace WBSF
{

	class CGeoFileInfo;
	class CNewGeoFileInfo;

	class CMapInputDirectoryManager : public CDirectoryManager
	{
	public:

		static const char* SUB_DIR_NAME;

		CMapInputDirectoryManager(const std::string& projectPath = "", const std::string& directoryListString = "");
		CMapInputDirectoryManager(const CMapInputDirectoryManager& NDM);
		virtual ~CMapInputDirectoryManager(void);

		virtual ERMsg DeleteFile(const std::string& fileName)const;
		CMapInputDirectoryManager& operator=(const CMapInputDirectoryManager& NDM);


	};

}