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
#include "FileManager/ScriptDirectoryManager.h"


namespace WBSF
{

	const char* CScriptDirectoryManager::SUB_DIR_NAME = "Script\\";

	CScriptDirectoryManager::CScriptDirectoryManager(const std::string& projectPath, const std::string& globalPaths) :
		CDirectoryManager(true, true, SUB_DIR_NAME, FILE_NAME, ".R", true)
	{
		SetLocalBasePath(projectPath);
		SetGlobalPaths(globalPaths);
	}


	CScriptDirectoryManager::~CScriptDirectoryManager(void)
	{
	}

	CScriptDirectoryManager::CScriptDirectoryManager(const CScriptDirectoryManager& in)
	{
		operator=(in);
	}

	CScriptDirectoryManager& CScriptDirectoryManager::operator=(const CScriptDirectoryManager& in)
	{
		CDirectoryManager::operator=(in);
		return *this;
	}



	//*************************************************************************************
	const char* CWeatherUpdateDirectoryManager::SUB_DIR_NAME = "Update\\";

	CWeatherUpdateDirectoryManager::CWeatherUpdateDirectoryManager(const std::string& projectPath, const std::string& globalPaths) :
		CDirectoryManager(true, true, SUB_DIR_NAME, FILE_TITLE, ".Update", true)
	{

		SetLocalBasePath(projectPath);
		SetGlobalPaths(globalPaths);
	}


	CWeatherUpdateDirectoryManager::~CWeatherUpdateDirectoryManager(void)
	{
	}

	CWeatherUpdateDirectoryManager::CWeatherUpdateDirectoryManager(const CWeatherUpdateDirectoryManager& in)
	{
		operator=(in);
	}

	CWeatherUpdateDirectoryManager& CWeatherUpdateDirectoryManager::operator=(const CWeatherUpdateDirectoryManager& in)
	{
		CDirectoryManager::operator=(in);
		return *this;
	}
}