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



namespace WBSF
{


	class CInputDirectoryManager : public CDirectoryManager
	{
	public:

		static const char* SUB_DIR_NAME;

		CInputDirectoryManager(const std::string& projectPath = "");
		CInputDirectoryManager(const CInputDirectoryManager& DM);
		virtual ~CInputDirectoryManager(void);
		CInputDirectoryManager& operator=(const CInputDirectoryManager& NDM);


	};

}