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
#include "FileManager/WGInputdirectoryManager.h"
#include "ModelBase/WGInput.h"



namespace WBSF
{

	const char* CWGInputDirectoryManager::SUB_DIR_NAME = "WGInput\\";

	CWGInputDirectoryManager::CWGInputDirectoryManager(const std::string& localPath, const std::string& globalPaths) :
		CDirectoryManager(true, false, SUB_DIR_NAME, FILE_TITLE, CWGInput::FILE_EXT)
	{
		SetLocalBasePath(localPath);
		SetGlobalPaths(globalPaths);
	}


	CWGInputDirectoryManager::~CWGInputDirectoryManager(void)
	{
	}

	CWGInputDirectoryManager::CWGInputDirectoryManager(const CWGInputDirectoryManager& NDM)
	{
		operator=(NDM);
	}

	CWGInputDirectoryManager& CWGInputDirectoryManager::operator=(const CWGInputDirectoryManager& NDM)
	{
		CDirectoryManager::operator=(NDM);

		return *this;
	}

	ERMsg CWGInputDirectoryManager::Get(const std::string& fileName, CWGInput& TGInput)const
	{
		ERMsg msg;

		std::string filePath;
		msg = GetFilePath(fileName, filePath);
		if (msg)
			msg = TGInput.Load(filePath);


		return msg;

	}

	ERMsg CWGInputDirectoryManager::Set(const std::string& fileName, CWGInput& TGInput)const
	{
		ERMsg msg;

		std::string filePath = GetFilePath(GetLocalPath(), fileName);

		ASSERT(!filePath.empty());

		msg = CreateMultipleDir(WBSF::GetPath(filePath));
		if (msg)
		{
			msg = TGInput.Save(filePath);
		}

		return msg;

	}
}
