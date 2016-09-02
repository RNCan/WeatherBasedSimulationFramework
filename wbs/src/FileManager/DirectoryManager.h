//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#ifndef __DIRECTORYMANAGER_H
#define __DIRECTORYMANAGER_H
#pragma once


#include "basic/ERMsg.h"
#include "Basic/UtilStd.h"

namespace WBSF
{

	class CDirectoryManagerBase
	{
	public:

		CDirectoryManagerBase(bool bHaveLocalPath, bool bHaveGlobalPath, const std::string& subDirName, int nameType = FILE_TITLE);


		CDirectoryManagerBase(const CDirectoryManagerBase& in);
		virtual ~CDirectoryManagerBase();


		CDirectoryManagerBase& operator=(const CDirectoryManagerBase& in);

		bool IsInDirectoryList(const std::string& path)const;
		void AppendToDirectoryList(std::string path);

		static std::string GetCleanPath(std::string path);
		const std::string& GetLocalBasePath()const{ return m_localPath; }
		void SetLocalBasePath(const std::string& in);

		const std::string& GetGlobalPaths()const{ return m_globalPaths; }
		void SetGlobalPaths(const std::string& in);

		const std::string& GetSubDirName()const{ return m_subDirName; }
		bool HaveProjectPath()const{ return m_bHaveLocalPath; }
		bool HaveGlobalPath()const{ return m_bHaveGlobalPaths; }

		static StringVector GetDirectoriesFromString(const std::string& in);
		std::string GetDirectories(bool bAddLocalPath = true, bool bAddGlobalPaths = true, const std::string& sep = "|")const;

		std::string GetLocalPath()const
		{
			ASSERT(!m_bHaveLocalPath || !m_localPath.empty());

			if (m_localPath.empty())
				return m_localPath;

			ASSERT(m_subDirName.length() == 0 || IsPathEndOk(m_subDirName));
			return m_localPath + m_subDirName;
		}


	protected:

		virtual StringVector GetFilesList(const std::string& fileExt)const;

		virtual ERMsg DeleteFile(const std::string& fileName, const std::string& fileExt)const;
		virtual ERMsg RenameFile(const std::string& fileName, const std::string& oldFileExt, const std::string& newFileName, const std::string& newFileExt)const;
		virtual bool GetLastUpdate(const std::string& fileName, const std::string& fileExt, __time64_t& lastUpdate, bool bVerifyAllFiles = true)const;
		virtual __time64_t  GetLastUpdate(const std::string& fileName, const std::string& fileExt, bool bVerifyAllFiles = true)const;
		virtual ERMsg Import(const std::string& filePath, const std::string& fileExt)const;


		void UpdateDirectoryArray();
		virtual void AppendFileArray(const std::string& path, const std::string& fileExt, StringVector& fileList)const;


		ERMsg GetFilePath(const std::string& fileName, const std::string& fileExt, std::string& filePath)const;
		std::string GetFilePath(const std::string& fileName, const std::string& fileExt)const;
		std::string _GetFileName(const std::string& fileTitle, const std::string& fileExt)const
		{
			std::string fileName(fileTitle);
			if (!fileTitle.empty())
			{
				if (!IsEqualNoCase(GetFileExtension(fileTitle), fileExt))
					fileName += fileExt;
			}

			return fileName;
		}

		std::string _GetFilePath(const std::string& path, const std::string& fileTitle, const std::string& fileExt)const
		{
			std::string fileName = _GetFileName(fileTitle, fileExt);
			std::string filePath;
			if (!fileName.empty())
				filePath = path + fileName;

			return filePath;
		}



		bool m_bHaveLocalPath;
		std::string m_localPath;
		std::string m_subDirName;
		int m_nameType;

		bool m_bHaveGlobalPaths;
		std::string m_globalPaths;

	};



	//****************************************
	class CDirectoryManager : public CDirectoryManagerBase
	{
	public:
		CDirectoryManager(bool bHaveProjectPath = false, bool bHaveGlobalPath = false, const std::string& suDirName = "", int nameType = FILE_TITLE, const std::string& extension = "", bool simpleExt = true);


		CDirectoryManager(const CDirectoryManager& in);
		virtual ~CDirectoryManager();

		CDirectoryManager& operator=(const CDirectoryManager& in);

		std::string GetFilePath(const std::string& fileName)const;
		ERMsg GetFilePath(const std::string& fileName, std::string& filePath)const;
		std::string GetExtensions()const{ return m_extensions; }
		void SetExtensions(const std::string& extensions);

		virtual StringVector GetFilesList()const;
		virtual ERMsg DeleteFile(const std::string& fileName)const;
		virtual ERMsg RenameFile(const std::string& fileName, const std::string& newFileName)const;
		virtual ERMsg CopyFile(const std::string& fileName, const std::string& newFileName)const;
		virtual bool GetLastUpdate(const std::string& fileName, __time64_t& lastUpdate, bool bVerifyAllFiles = true)const;

		__time64_t GetLastUpdate(const std::string& fileName, bool bVerifyAllFiles)const;
		__time64_t GetLastUpdate(const std::string& fileName)const{ return GetFileStamp(GetFilePath(fileName)); }

		virtual ERMsg Import(const std::string& filePath)const;
		virtual bool FileExists(const std::string& fileName)const;
		virtual ERMsg CreateNewDataBase(const std::string& filePath)const;

		//int GetExtensionIndex(std::string path)const;
		bool IsInExtensionList(const std::string& extIn)const;
		void AppendToExtensionList(std::string path);



	protected:

		std::string GetFileName(const std::string& fileName)const
		{
			if (m_simpleExt && !m_extensions.empty())//m_extensionList.size() == 1)
				return CDirectoryManagerBase::_GetFileName(fileName, m_extensions);

			return fileName;
		}

		std::string GetFilePath(const std::string& path, const std::string& fileName)const
		{
			if (m_simpleExt && !m_extensions.empty()) //&& m_extensionList.size() == 1)
				return CDirectoryManagerBase::_GetFilePath(path, fileName, m_extensions);

			return _GetFilePath(path, fileName, "");
		}

		std::string m_extensions;
		bool m_simpleExt;
	};

	typedef std::unique_ptr<CDirectoryManager> CDirectoryManagerPtr;

}

#endif