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

	class CGribDirectoryManager : public CDirectoryManager
	{
	public:
		CGribDirectoryManager(const std::string& projectPath = "", const std::string& directoryListString = "");
		CGribDirectoryManager(const CGribDirectoryManager& NDM);
		virtual ~CGribDirectoryManager(void);

		CGribDirectoryManager& operator=(const CGribDirectoryManager& NDM);


	private:

		static const char* SUB_DIR_NAME;
	};

}