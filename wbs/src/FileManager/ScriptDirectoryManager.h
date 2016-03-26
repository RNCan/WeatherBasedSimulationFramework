//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once


#include "FileManager/DirectoryManager.h"

namespace WBSF
{


	class CScriptDirectoryManager : public CDirectoryManager
	{
	public:

		static const char* SUB_DIR_NAME;

		CScriptDirectoryManager(const std::string& projectPath = "", const std::string& directoryListString = "");
		CScriptDirectoryManager(const CScriptDirectoryManager& in);
		virtual ~CScriptDirectoryManager(void);

		CScriptDirectoryManager& operator=(const CScriptDirectoryManager& in);

	};



	class CWeatherUpdateDirectoryManager : public CDirectoryManager
	{
	public:

		static const char* SUB_DIR_NAME;

		CWeatherUpdateDirectoryManager(const std::string& projectPath = "", const std::string& directoryListString = "");
		CWeatherUpdateDirectoryManager(const CWeatherUpdateDirectoryManager& in);
		virtual ~CWeatherUpdateDirectoryManager(void);

		CWeatherUpdateDirectoryManager& operator=(const CWeatherUpdateDirectoryManager& in);

	};
}