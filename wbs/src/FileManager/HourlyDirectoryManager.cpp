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
#include "FileManager/HourlyDirectoryManager.h"
#include "Basic/HourlyDatabase.h"



namespace WBSF
{
	const char* CHourlyDirectoryManager::SUB_DIR_NAME = "Weather\\";

	CHourlyDirectoryManager::CHourlyDirectoryManager(const std::string& projectPath, const std::string& globalPaths) :
		CDirectoryManager(true, true, SUB_DIR_NAME, FILE_NAME, CHourlyDatabase::DATABASE_EXT)
	{
		SetLocalBasePath(projectPath);
		SetGlobalPaths(globalPaths);
	}


	CHourlyDirectoryManager::~CHourlyDirectoryManager(void)
	{
	}

	CHourlyDirectoryManager::CHourlyDirectoryManager(const CHourlyDirectoryManager& in)
	{
		operator=(in);
	}

	CHourlyDirectoryManager& CHourlyDirectoryManager::operator=(const CHourlyDirectoryManager& in)
	{
		CDirectoryManager::operator=(in);

		return *this;
	}

	ERMsg CHourlyDirectoryManager::OpenFile(const std::string& fileName, CHourlyDatabase& hourlyFile, short openMode, CCallback& callback)const
	{
		ERMsg msg;

		if (hourlyFile.IsOpen())
			hourlyFile.Close();

		std::string filePath;
		msg = GetFilePath(fileName, filePath);
		if (msg)
			msg = hourlyFile.Open(filePath, openMode, callback);


		return msg;
	}
	//
	//ERMsg CHourlyDirectoryManager::CreateNewDataBase(int directoryIndex, const std::string& fileName)const
	//{
	//	ASSERT( directoryIndex>= 0 && directoryIndex<m_directoryArray.size());
	//	std::string filePath = GetFilePath( m_directoryArray[directoryIndex], fileName );
	//	return CreateNewDataBase(filePath );
	//}

	ERMsg CHourlyDirectoryManager::CreateNewDataBase(const std::string& filePath)const
	{
		ERMsg msg = CDirectoryManager::CreateNewDataBase(filePath);

		if (msg)
			msg = CHourlyDatabase().CreateDatabase(filePath);


		return msg;
	}

	ERMsg CHourlyDirectoryManager::DeleteDatabase(const std::string& fileName, CCallback& callback)const
	{
		return CHourlyDatabase().DeleteDatabase(fileName, callback);
	}
}