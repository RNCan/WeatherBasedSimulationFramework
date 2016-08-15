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
//******************************************************************************
#include "stdafx.h"
#include "FileManager/MapInputdirectoryManager.h"
#include "Geomatic/GDALBasic.h"


namespace WBSF
{

	const char* CMapInputDirectoryManager::SUB_DIR_NAME = "MapInput\\";

	CMapInputDirectoryManager::CMapInputDirectoryManager(const std::string& projectPath, const std::string& globalPaths) :
		CDirectoryManager(true, true, SUB_DIR_NAME, FILE_NAME, ".tif", false)
	{
		SetLocalBasePath(projectPath);
		SetGlobalPaths(globalPaths);
	}


	CMapInputDirectoryManager::~CMapInputDirectoryManager(void)
	{
	}

	CMapInputDirectoryManager::CMapInputDirectoryManager(const CMapInputDirectoryManager& NDM)
	{
		operator=(NDM);
	}

	CMapInputDirectoryManager& CMapInputDirectoryManager::operator=(const CMapInputDirectoryManager& NDM)
	{
		CDirectoryManager::operator=(NDM);

		return *this;
	}

	ERMsg CMapInputDirectoryManager::DeleteFile(const std::string& fileName)const
	{
		ASSERT(!GetFileExtension(fileName).empty());
		return CDirectoryManagerBase::DeleteFile(fileName, "");
	}

}