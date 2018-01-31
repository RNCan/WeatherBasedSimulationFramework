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
#include "ModelDirectoryManager.h"
#include "ModelBase/Model.h"



namespace WBSF
{

	const char* CModelDirectoryManager::SUB_DIR_NAME = "Models\\";


	CModelDirectoryManager::CModelDirectoryManager(const std::string& path) :
		CDirectoryManager(true, false, SUB_DIR_NAME, FILE_TITLE, CModel::FILE_EXT)
	{
		SetLocalBasePath(path);
	}


	CModelDirectoryManager::~CModelDirectoryManager(void)
	{
	}

	CModelDirectoryManager::CModelDirectoryManager(const CModelDirectoryManager& in) :
		CDirectoryManager(in)
	{
	}
	
	bool CModelDirectoryManager::GetLastUpdate(const std::string& fileName, __time64_t& lastUpdate, bool bVerifyAllFiles)const
	{
		return CDirectoryManager::GetLastUpdate(fileName, lastUpdate, bVerifyAllFiles);
	}

	bool CModelDirectoryManager::FileExists(const std::string& fileName)const
	{
		std::string tmp = GetFilePath(fileName);
		return !tmp.empty();
	}

	std::string CModelDirectoryManager::GetHelpFilePath(const std::string& fileName)const
	{
		//use extension of the file name instead of .mdl
		return CDirectoryManagerBase::GetFilePath(fileName, "");
	}
}