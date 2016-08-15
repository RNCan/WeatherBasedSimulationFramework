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
#include "FileManager/FileManager.h"

#include "Basic/Registry.h"
#include "Basic/NormalsDatabase.h"
#include "Basic/DailyDatabase.h"
#include "Basic/HourlyDatabase.h"

using namespace std;

namespace WBSF
{


	//*****************************************************************************
	//CFileManager

	const char* CFileManager::DEFAULT_DIR[NB_DIRTYPE] =
	{
		"",
		CLocationDirectoryManager::SUB_DIR_NAME,
		CInputDirectoryManager::SUB_DIR_NAME,
		CModelInputDirectoryManager::SUB_DIR_NAME,
		CWGInputDirectoryManager::SUB_DIR_NAME,
		CMapInputDirectoryManager::SUB_DIR_NAME,
		CNormalsDirectoryManager::SUB_DIR_NAME,
		CWeatherUpdateDirectoryManager::SUB_DIR_NAME,
		CScriptDirectoryManager::SUB_DIR_NAME,
		CParametersVariationsDirectoryManager::SUB_DIR_NAME,
		"Output\\",
		"MapOutput\\",
		"Tmp\\"

	};


	CFileManager::CFileManager()
	{
		LoadDefaultFileManager();
	}


	void CFileManager::Update()
	{
		CRegistry registry;

		std::string sWeatherPath = registry.GetProfileString(CRegistry::WEATHER, m_appPath + "Weather\\");
		std::string sMapPath = registry.GetProfileString(CRegistry::MAPS, m_appPath + "Maps\\");
		std::string mapExtension = registry.GetProfileString(CRegistry::MAPS_EXTENSIONS, ".tif|.flt");
		std::string sScriptPath = registry.GetProfileString(CRegistry::R_SCRIPT, m_appPath + "Script\\");

		Init(sWeatherPath, sMapPath, mapExtension, sScriptPath);
	}


	void CFileManager::Init(const std::string& sWeatherPath, const std::string& sMapPath, const std::string& mapExtension, const std::string& sScriptPath)
	{

		ASSERT(m_appPath.length() >= 2);
		ASSERT(m_projectPath.length() >= 2);

		m_normalsManager.SetLocalBasePath(m_projectPath);
		m_normalsManager.SetGlobalPaths(sWeatherPath);
		m_dailyManager.SetLocalBasePath(m_projectPath);
		m_dailyManager.SetGlobalPaths(sWeatherPath);
		m_hourlyManager.SetLocalBasePath(m_projectPath);
		m_hourlyManager.SetGlobalPaths(sWeatherPath);
		m_gribsManager.SetLocalBasePath(m_projectPath);
		m_gribsManager.SetGlobalPaths(sWeatherPath);

		m_modelManager.SetLocalBasePath(m_appPath);
		m_modelInputManager.SetLocalBasePath(m_projectPath);
		m_WGInputManager.SetLocalBasePath(m_projectPath);
		m_PVDManager.SetLocalBasePath(m_projectPath);
		m_inputManager.SetLocalBasePath(m_projectPath);
		m_locManager.SetLocalBasePath(m_projectPath);

		m_mapInputManager.SetLocalBasePath(m_projectPath);
		m_mapInputManager.SetGlobalPaths(sMapPath);
		m_mapInputManager.SetExtensions(mapExtension);
		m_scripManager.SetLocalBasePath(m_projectPath);
		//m_scripManager.SetGlobalPaths(sScriptPath);
		m_weatherUpdateManager.SetLocalBasePath(m_projectPath);


	}

	//******************************************************************
	//******************************************************************
	// general section


	void CFileManager::SetAppPath(const std::string& path)
	{
		ASSERT(path.length() >= 2);

		if (m_appPath != path)
		{
			m_appPath = TrimConst(path);

			if (!IsPathEndOk(path))
				m_appPath += '\\';

			m_modelManager.SetLocalBasePath(m_appPath);

		}
	}

	void CFileManager::SetWeatherPath(const std::string& path)
	{
		m_normalsManager.SetGlobalPaths(path);
		m_dailyManager.SetGlobalPaths(path);
		m_hourlyManager.SetGlobalPaths(path);
		m_gribsManager.SetGlobalPaths(path);
	}

	void CFileManager::SetProjectPath(const std::string& path)
	{
		std::string projectPath = TrimConst(path);
		if (projectPath.empty())
			projectPath = GetUserDataPath() + "BioSIM\\";

		if (!IsPathEndOk(projectPath))
			projectPath += "\\";

		if (projectPath != m_projectPath)
		{
			m_projectPath = projectPath;

			m_normalsManager.SetLocalBasePath(m_projectPath);
			m_dailyManager.SetLocalBasePath(m_projectPath);
			m_hourlyManager.SetLocalBasePath(m_projectPath);
			m_gribsManager.SetLocalBasePath(m_projectPath);

			m_inputManager.SetLocalBasePath(m_projectPath);
			m_locManager.SetLocalBasePath(m_projectPath);
			m_modelInputManager.SetLocalBasePath(m_projectPath);
			m_WGInputManager.SetLocalBasePath(m_projectPath);
			m_PVDManager.SetLocalBasePath(m_projectPath);
			m_mapInputManager.SetLocalBasePath(m_projectPath);
			m_scripManager.SetLocalBasePath(m_projectPath);
			m_weatherUpdateManager.SetLocalBasePath(m_projectPath);
		}
	}


	__time64_t CFileManager::GetLastWeatherUpdate(const std::string& normalName, const std::string& dailyName, const std::string& hourlyName)const
	{
		__time64_t lastUpdate = CNormalsDatabase().GetLastUpdate(m_normalsManager.GetFilePath(normalName), true);

		if (!dailyName.empty())
			lastUpdate = max(lastUpdate, CDailyDatabase().GetLastUpdate(m_dailyManager.GetFilePath(normalName), true));

		if (!hourlyName.empty())
			lastUpdate = max(lastUpdate, CHourlyDatabase().GetLastUpdate(m_hourlyManager.GetFilePath(normalName), true));


		return lastUpdate;
	}

	__time64_t CFileManager::GetLastModelUpdate(const std::string& modelInput, const std::string& ID, const std::string& TGInput)const
	{
		//bool rep = false;
		__time64_t lastUpdate;
		//problem ici, le modèl devrais etre nommé avec son mon
		if (m_modelManager.GetLastUpdate(ID, lastUpdate))
		{
			//rep = true;

			__time64_t modifyTmp;
			if (ModelInput(ID).GetLastUpdate(modelInput, modifyTmp))
				if (modifyTmp > lastUpdate)
					lastUpdate = modifyTmp;

			if (m_WGInputManager.GetLastUpdate(TGInput, modifyTmp))
				if (modifyTmp > lastUpdate)
					lastUpdate = modifyTmp;
		}

		return lastUpdate;
	}

	void CFileManager::VerifieOrCreateDir(int type)const
	{
		ASSERT(NB_DIRTYPE == 13);
		CreateMultipleDir(GetDefaultProjectPath(type));
	}

	std::string CFileManager::GetExportFilePath(const std::string& fileName)const
	{
		std::string filePath;
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];

		_splitpath(fileName.c_str(), drive, dir, fname, ext);
		if (strlen(drive) == 0 || strlen(dir) == 0)
		{
			filePath = GetDefaultProjectPath(CD_OUTPUT);
		}
		else
		{
			filePath = drive;
			filePath += dir;
		}

		filePath += fname;

		if (strlen(ext) == 0)
			filePath += ".csv";
		else
			filePath += ext;

		return filePath;
	}

	std::string CFileManager::GetGraphFilePath(const std::string& fileName, const std::string& fileExtention)const
	{
		std::string filePath;
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];

		_splitpath(fileName.c_str(), drive, dir, fname, ext);
		if (strlen(drive) == 0 || strlen(dir) == 0)
		{
			filePath = GetDefaultProjectPath(CD_OUTPUT);
		}
		else
		{
			filePath = drive;
			filePath += dir;
		}

		filePath += fname;


		filePath += fileExtention;


		return filePath;
	}


	std::string CFileManager::GetOutputMapFilePath(const std::string& fileNameIn, const std::string& extention)const
	{
		std::string filePath;
		ASSERT(fileNameIn != extention);
		std::string fileName = PurgeFileName(fileNameIn);



		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];

		_splitpath(fileName.c_str(), drive, dir, fname, ext);
		if (strlen(drive) == 0 || strlen(dir) == 0)
		{
			filePath = GetDefaultProjectPath(CD_MAPOUTPUT);
		}
		else
		{
			filePath = drive;
			filePath += dir;
		}

		filePath += fname;
		filePath += extention;

		return filePath;
	}


	void CFileManager::LoadDefaultFileManager()
	{
		m_appPath = GetApplicationPath();
		m_projectPath = GetUserDataPath() + "BioSIM\\";

		Update();
	}

	void CFileManager::SaveDefaultFileManager()const
	{
		assert(!GetWeatherPath().empty());

		CRegistry registry;
		registry.WriteProfileString(CRegistry::WEATHER, GetWeatherPath());
		registry.WriteProfileString(CRegistry::MAPS, m_mapInputManager.GetDirectories(false, true));
		registry.WriteProfileString(CRegistry::MAPS_EXTENSIONS, m_mapInputManager.GetExtensions());
		registry.WriteProfileString(CRegistry::R_SCRIPT, m_scripManager.GetDirectories(false, true));
	}

	void CFileManager::CreateDefaultDirectories()const
	{
		for (int i = 0; i < NB_DIRTYPE; i++)
			VerifieOrCreateDir(i);
	}

}