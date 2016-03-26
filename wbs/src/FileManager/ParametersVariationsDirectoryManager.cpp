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
#include "FileManager/ParametersVariationsDirectoryManager.h"
#include "ModelBase/ParametersVariations.h"

namespace WBSF
{

	const char* CParametersVariationsDirectoryManager::SUB_DIR_NAME = "ParametersVariations\\";

	CParametersVariationsDirectoryManager::CParametersVariationsDirectoryManager(const std::string& projectPath, const std::string& globalPaths) :
		CDirectoryManager(true, false, SUB_DIR_NAME, FILE_TITLE, CParametersVariationsDefinition::FILE_EXT)
	{

		SetLocalBasePath(projectPath);
	}


	CParametersVariationsDirectoryManager::~CParametersVariationsDirectoryManager(void)
	{
	}

	CParametersVariationsDirectoryManager::CParametersVariationsDirectoryManager(const CParametersVariationsDirectoryManager& NDM)
	{
		operator=(NDM);
	}

	CParametersVariationsDirectoryManager& CParametersVariationsDirectoryManager::operator=(const CParametersVariationsDirectoryManager& NDM)
	{
		CDirectoryManager::operator=(NDM);

		return *this;
	}

	ERMsg CParametersVariationsDirectoryManager::Get(const std::string& fileName, CParametersVariationsDefinition& PV)const
	{
		ERMsg msg;
		if (!fileName.empty())
		{
			std::string filePath;
			msg = GetFilePath(fileName, filePath);
			if (msg)
				msg = PV.Load(filePath);
		}

		return msg;

	}

	ERMsg CParametersVariationsDirectoryManager::Set(const std::string& fileName, CParametersVariationsDefinition& PV)const
	{
		ERMsg msg;

		std::string filePath = GetFilePath(GetLocalPath(), fileName);

		ASSERT(!filePath.empty());

		msg = CreateMultipleDir(GetPath(filePath));
		if (msg)
		{
			msg = PV.Save(filePath);
		}

		return msg;

	}

}