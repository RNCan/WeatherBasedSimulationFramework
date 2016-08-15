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
#include "Filemanager/NormalDirectoryManager.h"
#include "Basic/NormalsDatabase.h"

namespace WBSF
{

	const char* CNormalsDirectoryManager::SUB_DIR_NAME = "Weather\\";

	CNormalsDirectoryManager::CNormalsDirectoryManager(const std::string& projectPath, const std::string& globalPaths) :
		CDirectoryManager(true, true, SUB_DIR_NAME, FILE_TITLE, CNormalsDatabase::DATABASE_EXT)
	{
		SetLocalBasePath(projectPath);
		SetGlobalPaths(globalPaths);
	}


	CNormalsDirectoryManager::~CNormalsDirectoryManager(void)
	{
	}

	CNormalsDirectoryManager::CNormalsDirectoryManager(const CNormalsDirectoryManager& in)
	{
		operator=(in);
	}

	CNormalsDirectoryManager& CNormalsDirectoryManager::operator=(const CNormalsDirectoryManager& in)
	{
		CDirectoryManager::operator=(in);

		return *this;
	}

	ERMsg CNormalsDirectoryManager::OpenFile(const std::string& fileName, CNormalsDatabase& NormalDatabase, short openMode)const
	{
		ERMsg msg;

		if (NormalDatabase.IsOpen())
			NormalDatabase.Close();

		std::string filePath;
		msg = GetFilePath(fileName, filePath);
		if (msg)
			msg = NormalDatabase.Open(filePath, openMode);


		return msg;
	}

	ERMsg CNormalsDirectoryManager::CreateNewDataBase(const std::string& filePath)const
	{
		ERMsg msg = CDirectoryManager::CreateNewDataBase(filePath);

		if (msg)
			msg = CNormalsDatabase::CreateDatabase(filePath);

		return msg;
	}

	ERMsg CNormalsDirectoryManager::DeleteDatabase(const std::string& fileName, CCallback& callback)const
	{
		return CNormalsDatabase().DeleteDatabase(fileName, callback);
	}
}