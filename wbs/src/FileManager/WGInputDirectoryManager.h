//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once


#include "FileManager/DirectoryManager.h"

namespace WBSF
{

	class CWGInput;

	class CWGInputDirectoryManager : public CDirectoryManager
	{
	public:

		static const char* SUB_DIR_NAME;


		CWGInputDirectoryManager(const std::string& projectPath = "", const std::string& directoryListString = "");
		CWGInputDirectoryManager(const CWGInputDirectoryManager& NDM);
		virtual ~CWGInputDirectoryManager(void);

		ERMsg Get(const std::string& fileName, CWGInput& WGInput)const;
		ERMsg Set(const std::string& fileName, CWGInput& WGInput)const;

		CWGInputDirectoryManager& operator=(const CWGInputDirectoryManager& NDM);


	};

}