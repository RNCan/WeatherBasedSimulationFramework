//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include "Basic/Callback.h"
#include "FileManager/DirectoryManager.h"


namespace WBSF
{
	class CHourlyDatabase;

	class CHourlyDirectoryManager : public CDirectoryManager
	{
	public:

		static const char* SUB_DIR_NAME;

		CHourlyDirectoryManager(const std::string& projectPath = "", const std::string& directoryListString = "");
		CHourlyDirectoryManager(const CHourlyDirectoryManager& NDM);
		virtual ~CHourlyDirectoryManager(void);

		ERMsg OpenFile(const std::string& fileName, CHourlyDatabase& dailyFile, short openMode = 0, CCallback& callback = DEFAULT_CALLBACK)const;
		//ERMsg CreateNewDataBase(int directoryIndex, const std::string& fileName)const;
		virtual ERMsg CreateNewDataBase(const std::string& filePath)const;

		CHourlyDirectoryManager& operator=(const CHourlyDirectoryManager& NDM);

		ERMsg DeleteDatabase(const std::string& fileName, CCallback& callback = DEFAULT_CALLBACK)const;

	};

}