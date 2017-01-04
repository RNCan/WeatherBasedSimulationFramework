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

#include "FileManager/DirectoryManager.h"
#include "Basic/UtilStd.h"

#include "WeatherBasedSimulationString.h"

using namespace std;

namespace WBSF
{
	//////////////////////////////////////////////////////////////////////
	// Construction/Destruction
	//////////////////////////////////////////////////////////////////////

	CDirectoryManagerBase::CDirectoryManagerBase(bool bHaveLocalPath, bool bHaveGlobalPath, const std::string& suDirName, int nameType)
	{
		assert(m_subDirName.empty() || IsPathEndOk(m_subDirName));
		ASSERT(nameType >= 0 && m_nameType < 3);


		m_bHaveLocalPath = bHaveLocalPath;
		m_bHaveGlobalPaths = bHaveGlobalPath;
		m_subDirName = suDirName;
		m_nameType = nameType;

		//assert( !m_subDirName.empty() && !IsPathEndOk(m_subDirName))
		//	m_subDirName += "\\";

		//#ifdef _DEBUG  //test unitaire
		//	std::string test;
		//	ASSERT( test.LoadString(IDS_FM_FILE_ALREADY_EXIST));//FileManagerRes.rc must be include 
		//	ASSERT( test.LoadString(IDS_CMN_FILENOTIMPORT));	//CommunRes.rc must be include 
		//#endif 
	}

	CDirectoryManagerBase::CDirectoryManagerBase(const CDirectoryManagerBase& in)
	{
		operator=(in);
	}

	CDirectoryManagerBase::~CDirectoryManagerBase()
	{
	}

	CDirectoryManagerBase& CDirectoryManagerBase::operator=(const CDirectoryManagerBase& in)
	{

		m_localPath = in.m_localPath;
		m_globalPaths = in.m_globalPaths;
		m_bHaveLocalPath = in.m_bHaveLocalPath;
		m_bHaveGlobalPaths = in.m_bHaveGlobalPaths;
		m_subDirName = in.m_subDirName;
		m_nameType = in.m_nameType;

		return *this;
	}

	void CDirectoryManagerBase::SetLocalBasePath(const std::string& in)
	{
		if (m_bHaveLocalPath)
		{
			if (in != m_localPath)
			{
				m_localPath = TrimConst(in);
				if (!m_localPath.empty() && !IsPathEndOk(m_localPath))
					m_localPath += '\\';

				//UpdateDirectoryArray();
			}
		}
	}

	void CDirectoryManagerBase::SetGlobalPaths(const std::string& in)
	{
		if (m_bHaveGlobalPaths)
		{
			if (in != m_globalPaths)
			{
				StringVector tmp = GetDirectoriesFromString(in);
				m_globalPaths = tmp.to_string("|");
				//StringVector tmp(in, "|");
				//
				//for (StringVector::iterator it = tmp.begin(); it != tmp.end())
				//{
				//	string path = TrimConst(*it);
				//	if (!path.empty())
				//	{
				//		if(!IsPathEndOk(path))
				//			*it += '\\';
				//	}
				//	else
				//	{
				//		it = tmp.erase(it);
				//	}
				//}
				//
				//m_globalPaths = tmp.to_string("|");
			}
		}
	}

	//void CDirectoryManagerBase::SetGlobalPaths(const std::string& directoryListString)
	//{
	//	if( m_directoryListString != directoryListString)
	//	{
	//		m_directoryArray = GetDirectoryListFromString(directoryListString);
	//		m_directoryListString  = GetDirectoryStringFromList(m_directoryArray);
	//	}
	//
	//	ASSERT( m_directoryArray.size() <= m_bHaveGlobalPaths+(m_bHaveLocalPath?1:0));
	//}
	//
	//namespace
	//{
	//   boost::bind(
	//   &boost::iequals<std::string,std::string>,
	//   boost::bind(&Model::mName, _1), // 1st arg to iequals
	//   assetName,                      // 2nd arg to iequals
	//   std::locale()                   // 3rd arg to iequals
	//)
	//}   

	//bool stringCompare( const string &left, const string &right )
	//{
	//   for( string::const_iterator lit = left.begin(), rit = right.begin(); lit != left.end() && rit != right.end(); ++lit, ++rit )
	//      if( tolower( *lit ) < tolower( *rit ) )
	//         return true;
	//      else if( tolower( *lit ) > tolower( *rit ) )
	//         return false;
	//   if( left.size() < right.size() )
	//      return true;
	//   return false;
	//}
	StringVector CDirectoryManagerBase::GetDirectoriesFromString(const std::string& directoryListString)
	{
		StringVector tmp1 = Tokenize(directoryListString, "|");
		StringVector tmp2;
		for (StringVector::iterator it = tmp1.begin(); it != tmp1.end(); it++)
		{
			Trim(*it);
			if (!it->empty())
			{
				if (!IsPathEndOk(*it))
					*it += '\\';

				*it = SimplifyFilePath(*it);
				if (tmp2.Find(*it, false) == UNKNOWN_POS)
					tmp2.push_back(*it);
			}
		}

		return tmp2;
	}
	//
	//std::string CDirectoryManagerBase::GetDirectoryStringFromList(const StringVector& directoryList, const string& sep)
	//{
	//	std::string str;
	//	//std::stringstream s(
	//	//std::copy(directoryList.begin(), directoryList.end(), std::ostream_iterator<std::string>(str, ";"));
	//
	//	for(StringVector::const_iterator it=directoryList.begin(); it!=directoryList.end(); it++)
	//		str += *it + sep;
	//
	//	return str;
	//}

	string CDirectoryManagerBase::GetCleanPath(std::string path)
	{
		Trim(path);
		if (!path.empty())
		{
			if (!IsPathEndOk(path))
				path += '\\';

			path = SimplifyFilePath(path);
		}

		return path;
	}

	void CDirectoryManagerBase::AppendToDirectoryList(std::string path)
	{
		path = GetCleanPath(path);
		if (!path.empty())
		{
			if (!m_globalPaths.empty())
				m_globalPaths += "|";

			m_globalPaths += path;
		}
	}
	//void CDirectoryManagerBase::UpdateDirectoryArray()
	//{
	//	m_directoryArray = GetDirectoryListFromString(m_directoryListString);
	//}
	//
	bool CDirectoryManagerBase::IsInDirectoryList(const std::string& pathIn)const
	{
		bool bIn = true;

		string path = GetCleanPath(pathIn);
		if (!path.empty())
		{
			StringVector directories = GetDirectoriesFromString(GetDirectories());
			bIn = directories.Find(path, false) < directories.size();
		}

		return bIn;
	}



	StringVector CDirectoryManagerBase::GetFilesList(const std::string& fileExt)const
	{
		StringVector fileArray;

		StringVector directories = GetDirectoriesFromString(GetDirectories());
		for (size_t i = 0; i < directories.size(); i++)
		{
			AppendFileArray(directories[i], fileExt, fileArray);
		}

		return fileArray;
	}


	void CDirectoryManagerBase::AppendFileArray(const std::string& path, const std::string& fileExt, StringVector& fileArray)const
	{
		std::string filter = path + "*" + fileExt;

		StringVector tmp = WBSF::GetFilesList(filter, m_nameType);
		fileArray.insert(fileArray.begin(), tmp.begin(), tmp.end());
	}

	std::string CDirectoryManagerBase::GetDirectories(bool bAddLocalPath, bool bAddGlobalPaths, const std::string& sep)const
	{
		string str;
		if (m_bHaveLocalPath && bAddLocalPath)
			str += GetLocalPath();

		if (m_bHaveGlobalPaths && bAddGlobalPaths)
		{
			if (!str.empty())
				str += '|';

			str += m_globalPaths;
		}

		//TrimRight(str, ";");
		//ReplaceString(str, ";", sep);

		//.TrimRight(";");
		//str.Replace(";", sep);

		return str;
	}

	ERMsg CDirectoryManagerBase::DeleteFile(const std::string& fileName, const std::string& fileExt)const
	{
		ERMsg msg;

		std::string filePath = GetFilePath(fileName, fileExt);
		while (FileExists(filePath))
		{
			msg += RemoveFile(filePath);

			filePath = GetFilePath(fileName, fileExt);
		}

		return msg;
	}

	ERMsg CDirectoryManagerBase::RenameFile(const std::string& fileName, const std::string& oldFileExt, const std::string& newFileName, const std::string& newFileExt)const
	{
		ASSERT(fileName != newFileName);

		ERMsg msg;

		string filePath = GetFilePath(fileName, oldFileExt);
		while (!filePath.empty() && msg)
		{
			string path = GetPath(filePath);
			std::string newFilePath = path + newFileName + newFileExt;
			//rename(filePath.c_str(), newFilePath.c_str());
			//	msg = SYGetMessage(*e);
			msg += WBSF::RenameFile(filePath, newFilePath);
			filePath = GetFilePath(fileName, oldFileExt);
		}

		return msg;
	}

	bool CDirectoryManagerBase::GetLastUpdate(const std::string& fileName, const std::string& fileExt, __time64_t& lastUpdate, bool bVerifyAllFiles)const
	{
		//bool bRep = false;

		std::string filePath = GetFilePath(fileName, fileExt);
		lastUpdate = GetFileStamp(fileName);
		return lastUpdate >= 0;

		//CFileStatus status;
		//   if( CFile::GetStatus( filePath, status ) )   // static function
		//   {
		//       lastUpdate = status.m_mtime;
		//       bRep = true;
		//}

		//return bRep;
	}

	__time64_t CDirectoryManagerBase::GetLastUpdate(const std::string& fileName, const std::string& fileExt, bool bVerifyAllFiles)const
	{
		std::string filePath = GetFilePath(fileName, fileExt);
		return GetFileStamp(fileName);
	}


	ERMsg CDirectoryManagerBase::Import(const std::string& filePath, const std::string& fileExt)const
	{
		ASSERT(!filePath.empty());
		ASSERT(m_bHaveLocalPath);
		ASSERT(!m_localPath.empty());

		ERMsg msg;

		if (!GetLocalPath().empty())
		{
			ASSERT(false);
			//std::string newName = GetLocalPath() + UtilWin::GetFileTitle( filePath ) + fileExt;

			//if( !DirExist(GetLocalPath()) )
			//{
			//	if( !CreateMultipleDir( GetLocalPath() ) )
			//	{
			//		msg.asgType(ERMsg::ERREUR);
			//		return msg;
			//	}
			//}

			//if( !::CopyFile( filePath, newName, true) )
			//      {
			//	msg = SYGetMessage( GetLastError() );
			//          std::string erreur;
			//          erreur.FormatMsg(IDS_CMN_FILENOTIMPORT, (LPCTSTR)filePath, (LPCTSTR)GetLocalPath() );
			//          msg.ajoute(erreur);
			//      }
		}


		return msg;
	}


	std::string CDirectoryManagerBase::GetFilePath(const std::string& fileName, const std::string& fileExt)const
	{
		std::string filePath;

		StringVector directories = GetDirectoriesFromString(GetDirectories());
		for (size_t i = 0; i < directories.size() && filePath.empty(); i++)
		{
			std::string tmp = _GetFilePath(directories[i], fileName, m_nameType == FILE_TITLE ? fileExt : "");
			if (FileExists(tmp))
				filePath = tmp;
		}

		return filePath;
	}

	ERMsg CDirectoryManagerBase::GetFilePath(const std::string& fileName, const std::string& fileExt, std::string& filePath)const
	{
		ERMsg msg;

		filePath = GetFilePath(fileName, fileExt);
		if (filePath.empty())
		{
			string directoryList = "\t" + GetDirectories();
			ReplaceString(directoryList, "|", "\r\n\t");

			string error = FormatMsg(IDS_WG_FILE_NOT_FOUND, _GetFileName(fileName, fileExt), directoryList.c_str());
			msg.ajoute(error);
		}

		return msg;
	}









	//////////////////////////////////////////////////////////////////////
	// Construction/Destruction
	//////////////////////////////////////////////////////////////////////


	CDirectoryManager::CDirectoryManager(bool bHaveProjectPath, bool bHaveGlobalPath, const std::string& suDirName, int nameType, const std::string& extensions, bool simpleExt) :
		CDirectoryManagerBase(bHaveProjectPath, bHaveGlobalPath, suDirName, nameType)
	{
		m_simpleExt = simpleExt;
		SetExtensions(extensions);
	}

	CDirectoryManager::CDirectoryManager(const CDirectoryManager& in) :
		CDirectoryManagerBase(in)
	{
		operator=(in);
	}

	CDirectoryManager::~CDirectoryManager()
	{
	}

	CDirectoryManager& CDirectoryManager::operator=(const CDirectoryManager& in)
	{
		if (&in != this)
		{
			CDirectoryManagerBase::operator =(in);
			m_simpleExt = in.m_simpleExt;
			m_extensions = in.m_extensions;
		}

		return *this;
	}
	//
	//std::string CDirectoryManager::GetExtension(std::string sep)const
	//{
	//	ASSERT( !m_simpleExt || m_extensionList.size() == 1);
	//
	//	std::string str;
	//	for(StringVector::const_iterator it=m_extensionList.begin(); it!=m_extensionList.end(); it++)
	//		str += *it + sep;
	//	return str;
	//}

	void CDirectoryManager::SetExtensions(const std::string& extension)
	{
		StringVector tmp = Tokenize(extension, "|", true);
		assert(!m_simpleExt || tmp.size() == 1);


		m_extensions = tmp.to_string("|");
		//m_extensionList  = Tokenize(extension, ";|\t\r\n");

	}

	//************************************************************
	// GetFileArray : obtain a list of file by serching in all directory
	//************************************************************
	StringVector CDirectoryManager::GetFilesList()const
	{
		StringVector files;
		if (m_simpleExt)
		{
			files = CDirectoryManagerBase::GetFilesList(m_extensions);
		}
		else
		{
			StringVector extensions(m_extensions, "|");
			for (size_t i = 0; i < extensions.size(); i++)
			{
				StringVector tmp = CDirectoryManagerBase::GetFilesList(extensions[i]);
				files.insert(files.end(), tmp.begin(), tmp.end());
			}
		}


		return files;
	}


	ERMsg CDirectoryManager::DeleteFile(const std::string& fileName)const
	{
		if (m_simpleExt)
			return CDirectoryManagerBase::DeleteFile(fileName, m_extensions);

		return CDirectoryManagerBase::DeleteFile(fileName, "");
	}

	//this default implementation rename all file in the same directory
	//that have the same file title
	ERMsg CDirectoryManager::RenameFile(const std::string& oldFileName, const std::string& newFileName)const
	{
		ASSERT(oldFileName != newFileName);

		ERMsg msg;

		bool bHaveExt = !GetFileExtension(oldFileName).empty();

		string oldFilePath = GetFilePath(oldFileName);
		string newFilePath(oldFilePath);
		if (bHaveExt)
			SetFileName(newFilePath, newFileName);
		else
			SetFileTitle(newFilePath, newFileName);

		msg = CDirectoryManagerBase::RenameFile(GetFileTitle(oldFilePath), GetFileExtension(oldFilePath), GetFileTitle(newFilePath), GetFileExtension(newFilePath));

		//	
		//	bool bChangeExt = GetFileExtension(fileName).empty();
		//	
		//
		//	std::string filePath = GetFilePath(fileName);
		//	ASSERT( !filePath.empty() );
		////	while( !filePath.empty() )
		////	{
		//	try
		//	{
		//		std::string strSrh = filePath;
		//		if( bChangeExt )
		//			SetFileExtension(strSrh, ".*");
		//
		//		StringVector filePathList;
		//		UtilWin::GetFilesList(filePathList, strSrh, true);
		//		for(int i=0; i<filePathListsize(); i++)
		//		{
		//			std::string newFilePath = filePathList[i];
		//			if( bChangeExt )
		//				UtilWin::SetFileTitle(newFilePath, newFileName);
		//			else UtilWin::SetFileName(newFilePath, newFileName);
		//
		//			CFile::Rename(filePathList[i], newFilePath);
		//		}
		//	}
		//	catch(CFileException, e)
		//	{
		//		msg = SYGetMessage(*e);
		////			break;
		//	}
		//		
		//

		//		filePath = GetFilePath(fileName);
		//	}


		return msg;
	}

	ERMsg CDirectoryManager::CopyFile(const std::string& oldFileName, const std::string& newFileName)const
	{
		ASSERT(oldFileName != newFileName);

		ERMsg msg;
		bool bHaveExt = !GetFileExtension(oldFileName).empty();

		string oldFilePath = GetFilePath(oldFileName);
		string newFilePath(oldFilePath);
		if (bHaveExt)
			SetFileName(newFilePath, newFileName);
		else
			SetFileTitle(newFilePath, newFileName);

		msg = CopyOneFile(oldFilePath, newFilePath);

		/*
		bool bChangeExt = GetFileExtension(fileName).empty();


		std::string filePath = GetFilePath(fileName);
		ASSERT( !filePath.empty() );
		TRY
		{
		std::string strSrh = filePath;
		if( bChangeExt )
		UtilWin::SetFileExtension(strSrh, ".*");

		StringVector filePathList;
		UtilWin::GetFilesList(filePathList, strSrh, true);
		for(int i=0; i<filePathListsize(); i++)
		{
		std::string newFilePath = filePathList[i];
		if( bChangeExt )
		UtilWin::SetFileTitle(newFilePath, newFileName);
		else UtilWin::SetFileName(newFilePath, newFileName);

		if( !::CopyFile(filePathList[i], newFilePath, true) )
		{
		msg = SYGetMessage( GetLastError() );
		std::string erreur;
		erreur.FormatMsg(IDS_CMN_UNABLE_COPY_FILE, (LPCTSTR)filePathList[i]);
		msg.ajoute(erreur);
		}
		}
		}
		CATCH(CFileException, e)
		{
		msg = SYGetMessage(*e);
		}
		END_CATCH
		*/
		return msg;
	}

	bool CDirectoryManager::GetLastUpdate(const std::string& fileName, __time64_t& lastUpdate, bool bVerifyAllFiles)const
	{
		StringVector extensions(m_extensions, "|");

		bool bRep = false;
		for (size_t i = 0; i < extensions.size(); i++)
			bRep = bRep || CDirectoryManagerBase::GetLastUpdate(fileName, extensions[i], lastUpdate, bVerifyAllFiles);

		return bRep;
	}

	__time64_t CDirectoryManager::GetLastUpdate(const std::string& fileName, bool bVerifyAllFiles)const
	{
		StringVector extensions(m_extensions, "|");

		__time64_t lastUpdate = -1;
		for (size_t i = 0; i < extensions.size(); i++)
			lastUpdate = std::max(lastUpdate, CDirectoryManagerBase::GetLastUpdate(fileName, extensions[i], bVerifyAllFiles));

		return lastUpdate;
	}

	//this default implementation import all file in the same directory
	//that have the same file title
	ERMsg CDirectoryManager::Import(const std::string& filePath)const
	{
		ERMsg msg;

		if (!GetLocalPath().empty())
		{
			if (!DirectoryExists(GetLocalPath()))
			{
				msg = CreateMultipleDir(GetLocalPath());
			}

			if (msg)
			{
				std::string newFilePath = GetLocalPath() + GetFileName(filePath);
				msg = CopyFile(filePath, newFilePath);

				/*std::string strSrh = filePath;
				UtilWin::SetFileExtension(strSrh, ".*");

				StringVector filePathList;
				UtilWin::GetFilesList(filePathList, strSrh, true);
				for(int i=0; i<filePathListsize(); i++)
				{
				std::string newFilePath = GetLocalPath() + UtilWin::GetFileName( filePathList[i] );

				if( !::CopyFile( filePathList[i], newFilePath, true) )
				{
				msg = SYGetMessage( GetLastError() );
				std::string erreur;
				erreur.FormatMsg(IDS_CMN_FILENOTIMPORT, (LPCTSTR)filePath, (LPCTSTR)GetLocalPath() );
				msg.ajoute(erreur);
				}
				}*/
			}


		}
		return msg;

	}

	std::string CDirectoryManager::GetFilePath(const std::string& fileName)const
	{


		//if( m_simpleExt && m_extensionList.size() == 1)
		if (m_simpleExt)
			return CDirectoryManagerBase::GetFilePath(fileName, m_extensions);

		//StringVector extensions(m_extensions, "|");
		return CDirectoryManagerBase::GetFilePath(fileName, "");
	}

	ERMsg CDirectoryManager::GetFilePath(const std::string& fileName, std::string& filePath)const
	{
		if (m_simpleExt)// && m_extensionList.size() == 1)
		{
			return CDirectoryManagerBase::GetFilePath(fileName, m_extensions, filePath);
		}

		return CDirectoryManagerBase::GetFilePath(fileName, "", filePath);
	}

	bool CDirectoryManager::FileExists(const std::string& fileName)const
	{
		std::string tmp = GetFilePath(fileName);
		return !tmp.empty();
	}

	ERMsg CDirectoryManager::CreateNewDataBase(const std::string& filePath)const
	{
		ERMsg msg;

		std::string fileName = GetFileName(filePath);
		//verify that the file does'nt exist in all other directories
		if (!GetFilePath(GetFileName(fileName)).empty())
		{
			string error = FormatMsg(IDS_WG_FILE_ALREADY_EXIST, fileName, WBSF::GetPath(filePath));
			msg.ajoute(error);

			return msg;
		}

		ASSERT(!FileExists(filePath));
		msg += CreateMultipleDir(WBSF::GetPath(filePath));

		return msg;//do nothing
	}

	bool CDirectoryManager::IsInExtensionList(const std::string& extIn)const
	{
		int index = -1;

		string ext(extIn);
		Trim(ext);
		if (ext.empty() || ext[0] != '.')
			ext.insert(ext.begin(), '.');

		StringVector extensions(m_extensions, "|");
		return extensions.Find(ext, false) < extensions.size();
		//for(int i=0; i<extensions.size()&&index==-1; i++)
		//if (IsEqualNoCase(ext, extensions[i]) == 0)
		//	index = i;

		//return index;
	}

	void CDirectoryManager::AppendToExtensionList(std::string ext)
	{
		Trim(ext);
		if (ext.empty() || ext[0] != '.')
			ext.insert(ext.begin(), '.');

		if (!m_extensions.empty())
			m_extensions += "|";

		m_extensions += ext;
	}
}