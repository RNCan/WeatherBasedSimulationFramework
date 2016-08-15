//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
#ifndef __FILEMANAGER_H
#define __FILEMANAGER_H

#pragma once


#include "basic/ERMsg.h"

#include "FileManager/ModelDirectoryManager.h"
#include "FileManager/NormalDirectoryManager.h"
#include "FileManager/DailyDirectoryManager.h"
#include "FileManager/HourlyDirectoryManager.h"
#include "FileManager/GribDirectoryManager.h"
#include "FileManager/InputDirectoryManager.h"
#include "FileManager/LocationDirectoryManager.h"
#include "FileManager/ModelInputDirectoryManager.h"
#include "FileManager/WGInputDirectoryManager.h"
#include "FileManager/ParametersVariationsDirectoryManager.h"
#include "FileManager/MapInputDirectoryManager.h"
#include "FileManager/ScriptDirectoryManager.h"

namespace WBSF
{

	class CFileManager
	{
	public:

		enum TDirType{ CD_PROJECT, CD_LOC, CD_INPUT, CD_MODELINPUT, CD_WGINPUT, CD_MAPINPUT, CD_WEATHER, CD_PVD, CD_WEATHER_UPDATE, CD_SCRIPT, CD_OUTPUT, CD_MAPOUTPUT, CD_TMP, NB_DIRTYPE };
		static const char* DEFAULT_DIR[NB_DIRTYPE];

		static CFileManager& GetInstance()
		{
			static CFileManager INSTANCE; // Guaranteed to be destroyed.
			// Instantiated on first use.
			return INSTANCE;
		}

		std::string GetDefaultProjectPath(int type)const{ assert(!m_projectPath.empty()); return m_projectPath + DEFAULT_DIR[type]; }

		void LoadDefaultFileManager();
		void SaveDefaultFileManager()const;
		void CreateDefaultDirectories()const;

		const std::string& GetAppPath()const{ return m_appPath; }
		void SetAppPath(const std::string& path);

		bool IsInWeatherPath(const std::string& path)const{ return m_normalsManager.IsInDirectoryList(path); }
		std::string GetWeatherPath(std::string sep = "|")const{ return m_normalsManager.GetDirectories(false, true, sep); }
		void SetWeatherPath(const std::string& path);

		const std::string& GetProjectPath()const { return m_projectPath; }
		void SetProjectPath(const std::string& path);


		std::string GetOutputPath()const{ VerifieOrCreateDir(CD_OUTPUT); return GetDefaultProjectPath(CD_OUTPUT); }
		std::string GetOutputMapPath()const{ VerifieOrCreateDir(CD_MAPOUTPUT); return GetDefaultProjectPath(CD_MAPOUTPUT); }
		std::string GetTmpPath()const{ VerifieOrCreateDir(CD_TMP); return GetDefaultProjectPath(CD_TMP); }

		void Reset(void);

		void Update();
		__time64_t GetLastWeatherUpdate(const std::string& normalName, const std::string& dailyName, const std::string& hourlyName)const;
		__time64_t GetLastModelUpdate(const std::string& modelInput, const std::string& ID, const std::string& WGInput)const;
		void VerifieOrCreateDir(int type)const;


		std::string GetExportFilePath(const std::string& fileName)const;
		std::string GetOutputMapFilePath(const std::string& fileName, const std::string& ext)const;
		std::string GetGraphFilePath(const std::string& fileName, const std::string& ext)const;

		//******************************************************************
		// Normal manager
		const CNormalsDirectoryManager& Normals()const{ return m_normalsManager; }
		CNormalsDirectoryManager& Normals(){ return m_normalsManager; }

		//******************************************************************
		// Daily manager
		const CDailyDirectoryManager& Daily()const{ return m_dailyManager; }
		CDailyDirectoryManager& Daily(){ return m_dailyManager; }

		//******************************************************************
		// Hourly manager
		const CHourlyDirectoryManager& Hourly()const{ return m_hourlyManager; }
		CHourlyDirectoryManager& Hourly(){ return m_hourlyManager; }

		//******************************************************************
		// gribs manager
		const CGribDirectoryManager& Gribs()const{ return m_gribsManager; }
		CGribDirectoryManager& Gribs(){ return m_gribsManager; }

		//******************************************************************
		// model manager
		const CModelDirectoryManager& Model()const{ return m_modelManager; }

		//******************************************************************
		// Input manager
		const CInputDirectoryManager& Input()const{ return m_inputManager; }
		CInputDirectoryManager& Input(){ return m_inputManager; }

		//******************************************************************
		// Locations manager
		const CLocationDirectoryManager& Loc()const{ return m_locManager; }
		CLocationDirectoryManager& Loc(){ return m_locManager; }

		//******************************************************************
		// modelInput manager
		const CModelInputDirectoryManager& ModelInput(const std::string& modelID = "")const
		{
			if (!modelID.empty())
			{
				CModelInputDirectoryManager& modelInputManager = const_cast<CModelInputDirectoryManager&>(m_modelInputManager);
				modelInputManager.SetExtensions(modelID);
			}

			return m_modelInputManager;
		}

		//******************************************************************
		// WGInput manager
		const CWGInputDirectoryManager& WGInput(const std::string& modelID = "")const
		{
			return m_WGInputManager;
		}

		//******************************************************************
		// WGInput manager
		const CParametersVariationsDirectoryManager& PVD(const std::string& modelID = "")const
		{
			if (!modelID.empty())
			{
				CParametersVariationsDirectoryManager& PVDManager = const_cast<CParametersVariationsDirectoryManager&>(m_PVDManager);
				PVDManager.SetExtensions(modelID);
			}

			return m_PVDManager;
		}

		//***********************************************************************
		// maps manager
		const CMapInputDirectoryManager& MapInput()const{ return m_mapInputManager; }
		CMapInputDirectoryManager& MapInput(){ return m_mapInputManager; }

		//***********************************************************************
		// script manager
		const CScriptDirectoryManager & Script()const{ return m_scripManager; }
		CScriptDirectoryManager & Script(){ return m_scripManager; }

		//***********************************************************************
		// script manager
		const CWeatherUpdateDirectoryManager & WeatherUpdate()const{ return m_weatherUpdateManager; }
		CWeatherUpdateDirectoryManager & WeatherUpdate(){ return m_weatherUpdateManager; }

	protected:

		CFileManager();
		CFileManager(const CFileManager& in);				// Don't Implement
		CFileManager& operator=(const CFileManager& in);	// Don't Implement

		void Init(const std::string& sWeatherPath, const std::string& sMapPath, const std::string& sMapExtension, const std::string& sScriptPath);

		std::string m_appPath;
		std::string m_projectPath;

		CModelDirectoryManager m_modelManager;
		CNormalsDirectoryManager m_normalsManager;
		CDailyDirectoryManager m_dailyManager;
		CHourlyDirectoryManager m_hourlyManager;
		CGribDirectoryManager m_gribsManager;
		CInputDirectoryManager m_inputManager;
		CLocationDirectoryManager m_locManager;
		CModelInputDirectoryManager m_modelInputManager;
		CWGInputDirectoryManager m_WGInputManager;
		CParametersVariationsDirectoryManager m_PVDManager;
		CMapInputDirectoryManager m_mapInputManager;
		CScriptDirectoryManager m_scripManager;
		CWeatherUpdateDirectoryManager m_weatherUpdateManager;

	};


	inline CFileManager& GetFileManager(){ return CFileManager::GetInstance(); }
	inline CFileManager& GetFM(){ return CFileManager::GetInstance(); }

}
#endif