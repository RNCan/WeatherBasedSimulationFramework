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
#include "FileManager/InputDirectoryManager.h"




namespace WBSF
{

	const char* CInputDirectoryManager::SUB_DIR_NAME = "Input\\";

	CInputDirectoryManager::CInputDirectoryManager(const std::string& projectPath) :
		CDirectoryManager(true, false, SUB_DIR_NAME, FILE_NAME, ".csv", true)
	{
		SetLocalBasePath(projectPath);
	}


	CInputDirectoryManager::~CInputDirectoryManager(void)
	{
	}

	CInputDirectoryManager::CInputDirectoryManager(const CInputDirectoryManager& in)
	{
		operator=(in);
	}

	CInputDirectoryManager& CInputDirectoryManager::operator=(const CInputDirectoryManager& in)
	{
		CDirectoryManager::operator=(in);
		return *this;
	}
}