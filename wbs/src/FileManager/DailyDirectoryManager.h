//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
#ifndef __DAILYDIRECTORYMANAGER_H
#define __DAILYDIRECTORYMANAGER_H
#pragma once

#include "Basic/Callback.h"
#include "Filemanager/DirectoryManager.h"

namespace WBSF
{
	class CDailyDatabase;

	class CDailyDirectoryManager : public CDirectoryManager
	{
	public:
		CDailyDirectoryManager(const std::string& projectPath = "", const std::string& globalPaths = "");
		CDailyDirectoryManager(const CDailyDirectoryManager& in);
		virtual ~CDailyDirectoryManager(void);

		ERMsg OpenFile(const std::string& fileName, CDailyDatabase& dailyFile, short openMode = 0, CCallback& callback = DEFAULT_CALLBACK)const;
		//ERMsg CreateNewDataBase(int directoryIndex, const std::string& fileName)const;
		virtual ERMsg CreateNewDataBase(const std::string& filePath)const;

		CDailyDirectoryManager& operator=(const CDailyDirectoryManager& in);

		ERMsg DeleteDatabase(const std::string& fileName, CCallback& callback = DEFAULT_CALLBACK)const;
	
	protected:

		//static const char* EXTENSION;
		static const char* SUB_DIR_NAME;
	};

}
#endif