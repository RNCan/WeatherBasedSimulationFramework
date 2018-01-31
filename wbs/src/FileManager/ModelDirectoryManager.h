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

	class CModel;


	class CModelDirectoryManager : public CDirectoryManager
	{
	public:

		static const char* SUB_DIR_NAME;

		CModelDirectoryManager(const std::string& projectPath = "");
		CModelDirectoryManager(const CModelDirectoryManager& NDM);
		virtual ~CModelDirectoryManager(void);


		template<class T>
		ERMsg Get(const std::string& fileName, T& object)const
		{
			ERMsg msg;

			std::string filePath;
			msg = GetFilePath(fileName, filePath);

			if (msg)
				msg = object.Load(filePath);


			return msg;

		}

		template<class T>
		ERMsg Set(const std::string& fileName, T& object)const
		{
			ERMsg msg;

			std::string filePath;
			filePath = GetFilePath(fileName);

			if (filePath.empty())
				filePath = CDirectoryManager::GetFilePath(GetLocalPath(), fileName);

			ASSERT(!filePath.empty());

			msg = CreateMultipleDir(WBSF::GetPath(filePath));
			if (msg)
				msg = object.Save(filePath);

			return msg;

		}

		virtual ERMsg CopyFile(const std::string& fileName, const std::string& newFileName)const
		{
			return CDirectoryManager::CopyFile(fileName, newFileName);
		}

		virtual ERMsg RenameFile(const std::string& fileName, const std::string& newFileName)const
		{
			return CDirectoryManager::RenameFile(fileName, newFileName);
		}

		virtual bool GetLastUpdate(const std::string& fileName, __time64_t& lastUpdate, bool bVerifyAllFiles = true)const;

		bool FileExists(const std::string& filePath)const;

		std::string GetHelpFilePath(const std::string& fileName)const;
		

	};


}