//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     R�mi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
// 01-01-2016	R�mi Saint-Amant	Include into Weather-based simulation framework
//******************************************************************************
#include "stdafx.h"
#include "FileManager/GribDirectoryManager.h"

namespace WBSF
{


	const char* CGribDirectoryManager::SUB_DIR_NAME = "Weather\\";

	CGribDirectoryManager::CGribDirectoryManager(const std::string& projectPath, const std::string& globalPaths) :
		CDirectoryManager(true, true, SUB_DIR_NAME, FILE_NAME, ".gribs")
	{
		SetLocalBasePath(projectPath);
		SetGlobalPaths(globalPaths);
	}


	CGribDirectoryManager::~CGribDirectoryManager(void)
	{
	}

	CGribDirectoryManager::CGribDirectoryManager(const CGribDirectoryManager& NDM)
	{
		operator=(NDM);
	}

	CGribDirectoryManager& CGribDirectoryManager::operator=(const CGribDirectoryManager& NDM)
	{
		CDirectoryManager::operator=(NDM);

		return *this;
	}

}