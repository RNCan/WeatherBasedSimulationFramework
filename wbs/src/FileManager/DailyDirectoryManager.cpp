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
#include "FileManager/DailyDirectoryManager.h"
#include "Basic/DailyDatabase.h"


namespace WBSF
{

	const char* CDailyDirectoryManager::SUB_DIR_NAME = "Weather\\";

	CDailyDirectoryManager::CDailyDirectoryManager(const std::string& projectPath, const std::string& globalPaths) :
		CDirectoryManager(true, true, SUB_DIR_NAME, FILE_TITLE, CDailyDatabase::DATABASE_EXT)
	{
		SetLocalBasePath(projectPath);
		SetGlobalPaths(globalPaths);
	}


	CDailyDirectoryManager::~CDailyDirectoryManager(void)
	{
	}

	CDailyDirectoryManager::CDailyDirectoryManager(const CDailyDirectoryManager& in)
	{
		operator=(in);
	}

	CDailyDirectoryManager& CDailyDirectoryManager::operator=(const CDailyDirectoryManager& in)
	{
		CDirectoryManager::operator=(in);

		return *this;
	}

	ERMsg CDailyDirectoryManager::OpenFile(const std::string& fileName, CDailyDatabase& dailyFile, short openMode, CCallback& callback)const
	{
		ERMsg msg;

		if (dailyFile.IsOpen())
			dailyFile.Close();

		std::string filePath;
		msg = GetFilePath(fileName, filePath);
		if (msg)
			msg = dailyFile.Open(filePath, openMode, callback);

		return msg;
	}

	ERMsg CDailyDirectoryManager::CreateNewDataBase(const std::string& filePath)const
	{
		ERMsg msg = CDirectoryManager::CreateNewDataBase(filePath);

		if (msg)
			msg = CDailyDatabase::CreateDatabase(filePath);


		return msg;
	}

	ERMsg CDailyDirectoryManager::DeleteDatabase(const std::string& fileName, CCallback& callback)const
	{
		return CDailyDatabase::DeleteDatabase(fileName, callback);
	}

}
