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
	class CNormalsDatabase;

	class CNormalsDirectoryManager : public CDirectoryManager
	{
	public:

		static const char* SUB_DIR_NAME;

		CNormalsDirectoryManager(const std::string& projectPath = "", const std::string& directoryListString = "");
		CNormalsDirectoryManager(const CNormalsDirectoryManager& in);
		virtual ~CNormalsDirectoryManager(void);

		ERMsg OpenFile(const std::string& fileName, CNormalsDatabase& NormalDatabase, short openMode = 0)const;
		//ERMsg CreateNewDataBase(int directoryIndex, const std::string& fileName)const;

		virtual ERMsg CreateNewDataBase(const std::string& filePath)const;
		virtual ERMsg DeleteDatabase(const std::string& fileName, CCallback& callback = DEFAULT_CALLBACK)const;


		CNormalsDirectoryManager& operator=(const CNormalsDirectoryManager& in);

	};

}