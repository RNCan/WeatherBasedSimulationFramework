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
#include "Filemanager/ModelInputdirectorymanager.h"
#include "ModelBase/ModelInput.h"

namespace WBSF
{
	const char* CModelInputDirectoryManager::SUB_DIR_NAME = "Model Input\\";

	CModelInputDirectoryManager::CModelInputDirectoryManager(const std::string& projectPath) :
		CDirectoryManager(true, false, SUB_DIR_NAME, FILE_TITLE, ".ids")
	{
		SetLocalBasePath(projectPath);
	}


	CModelInputDirectoryManager::~CModelInputDirectoryManager(void)
	{
	}

	CModelInputDirectoryManager::CModelInputDirectoryManager(const CModelInputDirectoryManager& DM) :
		CDirectoryManager(DM)
	{
	}

	ERMsg CModelInputDirectoryManager::Get(const std::string& fileName, CModelInput& modelInput)const
	{
		ERMsg msg;

		std::string filePath;
		msg = GetFilePath(fileName, filePath);
		if (msg)
			msg = modelInput.Load(filePath);


		return msg;

	}

	ERMsg CModelInputDirectoryManager::Set(const std::string& fileName, CModelInput& modelInput)const
	{
		ERMsg msg;

		std::string filePath = GetFilePath(GetLocalPath(), fileName);
		ASSERT(!filePath.empty());

		msg = CreateMultipleDir(WBSF::GetPath(filePath));
		if (msg)
		{
			msg = modelInput.Save(filePath);
		}

		return msg;

	}
}

