//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//******************************************************************************


#include "stdafx.h"
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/crc.hpp>
#include <stdio.h>
#include <stdexcept>
#include <Strsafe.h>
#include <ShlObj.h>
#include <shellapi.h>
#include <commdlg.h>
#include <filesystem>

#include "Basic/UtilStd.h"
#include "Basic/UtilMath.h"
#include "Basic/Callback.h"
#include "Basic/UtilTime.h"
#include "Basic/Rijndael.h"
#include "Basic/Registry.h"
#include "Basic/DynamicRessource.h"

#include "WeatherBasedSimulationString.h"

using namespace std;

using boost::tokenizer;
using boost::escaped_list_separator;



//template < >
//boost::filesystem::path& boost::filesystem::path::append< typename boost::filesystem::path::iterator >(typename boost::filesystem::path::iterator begin, typename boost::filesystem::path::iterator end, const codecvt_type& cvt)
//{
//	for (; begin != end; ++begin)
//		*this /= *begin;
//	return *this;
//}

// Return path when appended to a_From will resolve to same as a_To
//boost::filesystem::path make_relative(boost::filesystem::path a_From, boost::filesystem::path a_To)
//{
//	a_From = boost::filesystem::absolute(a_From); a_To = boost::filesystem::absolute(a_To);
//	boost::filesystem::path ret;
//	boost::filesystem::path::const_iterator itrFrom(a_From.begin()), itrTo(a_To.begin());
//	// Find common base
//	for (boost::filesystem::path::const_iterator toEnd(a_To.end()), fromEnd(a_From.end()); itrFrom != fromEnd && itrTo != toEnd && *itrFrom == *itrTo; ++itrFrom, ++itrTo);
//	// Navigate backwards in directory to reach previously found base
//	for (boost::filesystem::path::const_iterator fromEnd(a_From.end()); itrFrom != fromEnd; ++itrFrom)
//	{
//		if ((*itrFrom) != ".")
//			ret /= "..";
//	}
//	// Now navigate down the directory branch
//	ret.append(itrTo, a_To.end(), boost::filesystem::path::codecvt());
//	return ret;
//}

//namespace fs = boost::filesystem;
namespace fs = std::filesystem;

auto make_relative(const fs::path& base, const fs::path& path)
{
	//std::filesystem::path path, file;

	std::filesystem::path result = std::filesystem::relative(path, base);
	return result;


	//// Start at the root path and while they are the same then do nothing then when they first
	//// diverge take the entire from path, swap it with '..' segments, and then append the remainder of the to path.
	//auto fromIter = from.begin();
	//auto toIter = to.begin();
	//
	//// Loop through both while they are the same to find nearest common directory
	//while (fromIter != from.end() && toIter != to.end() && *toIter == *fromIter)
	//{
	//	++toIter;
	//	++fromIter;
	//}
	//
	//// Replace from path segments with '..' (from => nearest common directory)
	//auto finalPath = fs::path{};
	//while (fromIter != from.end())
	//{
	//	finalPath /= "..";
	//	++fromIter;
	//}
	//
	//// Append the remainder of the to path (nearest common directory => to)
	//while (toIter != to.end())
	//{
	//	finalPath /= *toIter;
	//	++toIter;
	//}
	//
	//return finalPath;
}

namespace WBSF
{

	const char STRVMISS[] = "VMiss";
	const char STRDEFAULT[] = "Default";


	ERMsg RemoveFile(const std::string& filePath)
	{
		ERMsg msg;

		if (FileExists(filePath))
		{
			remove(filePath.c_str());
			if (FileExists(filePath))
				msg.ajoute("Error deleting file: " + filePath);// GetLastErrorMessage();
		}

		return msg;
	}


	ERMsg RemoveDirectory(const std::string& pathIn)
	{
		ERMsg msg;

		std::string path(pathIn);

		if (IsPathEndOk(path))
			path = path.substr(0, path.size() - 1);

		if (DirectoryExists(path))
		{
			if (!::RemoveDirectoryW(UTF16(path).c_str()))
			{
				msg = GetLastErrorMessage();
				msg.ajoute(pathIn);
			}
		}


		return msg;
	}

	ERMsg RenameFile(const std::string& filePath1, const std::string& filePath2)
	{
		ERMsg msg;

		if (!MoveFileExW(UTF16(filePath1).c_str(), UTF16(filePath2).c_str(), MOVEFILE_COPY_ALLOWED))
		{
			msg = GetLastErrorMessage();
			msg.ajoute(FormatMsg(IDS_BSC_UNABLE_RENAME, filePath1, filePath2));
		}


		return msg;
	}

	ERMsg RenameDir(const std::string& pathIn1, const std::string& pathIn2)
	{
		ERMsg msg;


		std::string path1(pathIn1);

		if (IsPathEndOk(path1))
			path1 = path1.substr(0, path1.size() - 1);

		std::string path2(pathIn1);

		if (IsPathEndOk(path2))
			path2 = path2.substr(0, path2.size() - 1);

		if (!MoveFileExW(UTF16(path1).c_str(), UTF16(path2).c_str(), MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING))
		{
			msg = GetLastErrorMessage();
			msg.ajoute(pathIn1);
			msg.ajoute(pathIn2);
		}


		return msg;
	}

	ERMsg CopyOneFile(const std::string& filePath1, const std::string& filePath2, bool failIfExist)
	{
		ERMsg msg;

		if (!CopyFileW(UTF16(filePath1).c_str(), UTF16(filePath2).c_str(), failIfExist))
		{
			msg = GetLastErrorMessage();
			msg.ajoute(FormatMsg(IDS_BSC_UNABLE_COPY_FILE, filePath1, filePath2));
		}

		return msg;
	}

	ERMsg CopyDirectory(const std::string& pathIn1, const std::string& pathIn2)
	{
		ERMsg msg;


		std::wstring path1(UTF16(pathIn1));

		//		if (IsPathEndOk(path1))
					//path1 = path1.substr(0, path1.size() - 1);

		std::wstring path2(UTF16(pathIn1));

		//if (IsPathEndOk(path2))
			//path2 = path2.substr(0, path2.size() - 1);

		assert(false);
		SHFILEOPSTRUCTW s = { 0 };
		s.hwnd = NULL;
		s.wFunc = FO_COPY;
		s.fFlags = FOF_SILENT;
		s.pTo = path2.c_str();
		s.pFrom = path1.c_str();


		//if (SHFileOperation(&s)>0)
		//msg.ajoute("Erreur copy dir");


		return msg;
	}

	std::string GetErrorString(ERMsg msg, const char* sep)
	{
		std::string str;
		for (int i = 0; i < (int)msg.dimension(); i++)
			str += msg[i] + sep;

		return str;
	}

	ERMsg GetErrorMsgFromString(const std::string& str, const char* sep)
	{
		ERMsg msg;

		StringVector errors(str.c_str(), sep);
		for (StringVector::const_iterator it = errors.begin(); it != errors.end(); it++)
			msg.ajoute(*it);

		return msg;
	}


	std::string LoadString(UINT nId)
	{
		std::string str;

		HMODULE hModule = CDynamicResources::get();
		//HMODULE hModule = GetModuleHandleW(0);
		PCTSTR szName = MAKEINTRESOURCE((nId >> 4) + 1); // lifted 

		// No sense continuing if we can't find the resource
		HRSRC hrsrc = ::FindResource(hModule, szName, RT_STRING);
		DWORD dwSize = ::SizeofResource(hModule, hrsrc) / sizeof(TCHAR);

		if (NULL == hrsrc)
		{
			//TRACE(_T("Cannot find resource %d: 0x%X"), nId, ::GetLastError());
		}
		else if (0 == dwSize)
		{
			//TRACE(_T("Cant get size of resource %d 0x%X\n"), nId, GetLastError());
		}
		else
		{
			//std::wstring tmp;
			str.insert(str.begin(), dwSize + 1, 0);
			::LoadStringA(hModule, nId, &(str[0]), dwSize);
			str.resize(strlen(str.c_str()));

			//str = UTF8(tmp);
		}

		return str;
	}


	std::string GetOutputString(ERMsg msg, CCallback& callback, bool bAllMessage, const char* sep)
	{


		string strError = GetString(IDS_STR_ERROR);
		string strSucces = GetString(IDS_STR_SUCCESS);
		string strLastMessage = GetString(IDS_STR_LAST_ERROR);
		string strLastComment = GetString(IDS_STR_LAST_COMMENT);

		std::string tmp;
		if (msg)
			tmp = strLastMessage + strSucces + "\n\n";
		else
			tmp = strLastMessage + strError + ":" + GetErrorString(msg, sep).c_str() + "\n\n";

		tmp += strLastComment + "\n\n";
		tmp += bAllMessage ? callback.GetDlgMessageText().data() : callback.GetCurrentTaskMessageText().data();
		tmp += "\n";

		return tmp;
	}



	/**
	 * https://svn.boost.org/trac/boost/ticket/1976#comment:2
	 *
	 * "The idea: uncomplete(/foo/new, /foo/bar) => ../new
	 *  The use case for this is any time you get a full path (from an open dialog, perhaps)
	 *  and want to store a relative path so that the group of files can be moved to a different
	 *  directory without breaking the paths. An IDE would be a simple example, so that the
	 *  project file could be safely checked out of subversion."
	 *
	 * ALGORITHM:
	 *  iterate path and base
	 * compare all elements so far of path and base
	 * whilst they are the same, no write to output
	 * when they change, or one runs out:
	 *   write to output, ../ times the number of remaining elements in base
	 *   write to output, the remaining elements in path
	 */
	 /*boost::filesystem::path naive_uncomplete(boost::filesystem::path const p, boost::filesystem::path const base)
	 {

		 using boost::filesystem::path;
		 using boost::filesystem::dot;
		 using boost::filesystem::slash;

		 if (p == base)
			 return "./";
			 //!! this breaks stuff if path is a filename rather than a directory,
				 // which it most likely is... but then base shouldn't be a filename so...

		 boost::filesystem::path from_path, from_base, output;

		 boost::filesystem::path::iterator path_it = p.begin(),    path_end = p.end();
		 boost::filesystem::path::iterator base_it = base.begin(), base_end = base.end();

		 // check for emptiness
		 if ((path_it == path_end) || (base_it == base_end))
			 throw std::runtime_error("path or base was empty; couldn't generate relative path");

	 #ifdef WIN32
		 // drive letters are different; don't generate a relative path
		 if (*path_it != *base_it)
			 return p;

		 // now advance past drive letters; relative paths should only go up
		 // to the root of the drive and not past it
		 ++path_it, ++base_it;
	 #endif

		 // Cache system-dependent dot, double-dot and slash strings
		 const std::string _dot  = std::string(1, dot<path>::value);
		 const std::string _dots = std::string(2, dot<path>::value);
		 const std::string _sep = std::string(1, slash<path>::value);

		 // iterate over path and base
		 while (true) {

			 // compare all elements so far of path and base to find greatest common root;
			 // when elements of path and base differ, or run out:
			 if ((path_it == path_end) || (base_it == base_end) || (*path_it != *base_it)) {

				 // write to output, ../ times the number of remaining elements in base;
				 // this is how far we've had to come down the tree from base to get to the common root
				 for (; base_it != base_end; ++base_it) {
					 if (*base_it == _dot)
						 continue;
					 else if (*base_it == _sep)
						 continue;

					 output /= "../";
				 }

				 // write to output, the remaining elements in path;
				 // this is the path relative from the common root
				 boost::filesystem::path::iterator path_it_start = path_it;
				 for (; path_it != path_end; ++path_it) {

					 if (path_it != path_it_start)
						 output /= "/";

					 if (*path_it == _dot)
						 continue;
					 if (*path_it == _sep)
						 continue;

					 output /= *path_it;
				 }

				 break;
			 }

			 // add directory level to both paths and continue iteration
			 from_path /= path(*path_it);
			 from_base /= path(*base_it);

			 ++path_it, ++base_it;
		 }

		 return output;
	 }
	 */

	std::string GetRelativePath(const std::string& sBasePath, const std::string& sFilePath)
	{
		std::string path;
		if (!sFilePath.empty())
		{
			std::wstring wBasePath(UTF16(sBasePath));
			std::wstring wFilePath(UTF16(sFilePath));
			std::filesystem::path basePath(wBasePath);
			std::filesystem::path filePath(wFilePath);
			if (basePath.root_name() == filePath.root_name())
			{
				boost::filesystem::path relPath = make_relative(basePath, filePath);

				std::wstring wpath = relPath.wstring();
				path = UTF8(wpath);
				if (!path.empty() && path.back() == '.')
				{
					//remove dot at the end
					path.pop_back();
				}

				std::replace(path.begin(), path.end(), '/', '\\');

				if (path.empty())
					path = ".\\";
			}
			else
			{
				path = sFilePath;
			}
		}

		return path;
	}

	std::string SimplifyFilePath(const std::string& filePath)
	{

		std::string tmp;
		tmp.resize(MAX_PATH);
		_fullpath(&(tmp[0]), filePath.c_str(), MAX_PATH);
		tmp.resize(strlen(tmp.c_str()));


		return tmp;
	}

	std::string GetAbsolutePath(const std::string& sBasePath, const std::string& sFilePath)
	{
		std::string path;
		if (!sFilePath.empty())
		{
			std::wstring wBasePath(UTF16(sBasePath));
			std::wstring wFilePath(UTF16(sFilePath));
			boost::filesystem::path basePath(wBasePath);
			boost::filesystem::path filePath(wFilePath);
			if (filePath.is_relative())
			{
				boost::system::error_code ec;
				//boost::filesystem::path absPath = boost::filesystem::canonical(filePath, basePath, ec);

				boost::filesystem::path absPath = boost::filesystem::absolute(filePath, basePath);
				//if (absPath.is_complete())
				//{
				//	boost::system::error_code ec;
				//	absPath = boost::filesystem::canonical(absPath, ec);
				//	
				//}

				path = SimplifyFilePath(UTF8(absPath.wstring()));
				//path = UTF8(absPath.wstring());
				std::replace(path.begin(), path.end(), '/', '\\');
				assert(!path.empty());


				//if (path.empty())
				//path = sFilePath;
			}
			else
			{
				path = sFilePath;
			}

		}

		return path;
	}


	/*bool IsPathEndOk(const std::string& filePath)
	{
		bool bRep = false;
		int pos = int(filePath.length())-1;
		if( pos>=0 )
			bRep = filePath.at(pos) == _T('\\') || filePath.at(pos) == _T('/');

		return bRep;
	}*/

	std::string upperCase(std::string input) {
		for (std::string::iterator it = input.begin(); it != input.end(); ++it)
			*it = toupper(*it);
		return input;
	}
	

	string FilePath2SpecialPath(const std::string& filePath, const string& appPath, const string& projectPath)
	{
		string special_path = filePath;
		if (!filePath.empty())
		{
			string::size_type pos = upperCase(filePath).find(upperCase(projectPath));
			if (pos != string::npos)
			{
				special_path.replace(pos, projectPath.length(), "[Project Path]\\");
			}
			else
			{
				//pos = upperCase(filePath).find(upperCase(appPath + "Models\\"));
				//if (pos != -1)
				//{
				//	special_path.replace(pos, projectPath.length(), (appPath + "Models\\").c_str(), "[Models Path]\\");
				//}
				//else
				//{
				pos = upperCase(filePath).find(upperCase(appPath));
				if (pos != -1)
				{
					special_path.replace(pos, appPath.length(), "[BioSIM Path]\\");
				}
				//}
			}

		}

		return special_path;
	}

	string SpecialPath2FilePath(const string& specialPath, const string& appPath, const string& projectPath)
	{
		string filePath = specialPath;

		if (!filePath.empty())
		{
			string::size_type pos = filePath.find("[Project Path]\\");
			if (pos != string::npos)
			{
				ReplaceString(filePath, "[Project Path]\\", projectPath.c_str());
			}

			pos = filePath.find("[BioSIM Path]\\");
			if (pos != -1)
			{
				ReplaceString(filePath, "[BioSIM Path]\\", appPath.c_str());
			}

			pos = filePath.find("[Models Path]\\");
			if (pos != -1)
			{
				ReplaceString(filePath, "[Models Path]\\", (appPath + "Models\\").c_str());
			}
		}

		return filePath;
	}

	bool FileExists(const std::string& filePath)
	{
		DWORD ftyp = GetFileAttributesA(filePath.c_str());
		if (ftyp == INVALID_FILE_ATTRIBUTES)
			return false;  //something is wrong with your path!

		if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
			return false;   // this is a directory, not a file

		//bool bExists = !(INVALID_FILE_ATTRIBUTES == GetFileAttributesA(filePath.c_str()) && GetLastError() == ERROR_FILE_NOT_FOUND);
		return true;
	}


	bool DirectoryExists(std::string path)
	{
		//std::string tmp(path);
		while (IsPathEndOk(path))
			path = path.substr(0, path.length() - 1);

		DWORD ftyp = GetFileAttributesA(path.c_str());
		if (ftyp == INVALID_FILE_ATTRIBUTES)
			return false;  //something is wrong with your path!

		if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
			return true;   // this is a directory!

		return false;    // this is not a directory!
	}

	std::string GetApplicationPath()
	{
		//Get full path with decoration
		std::wstring appPath;
		appPath.resize(MAX_PATH);
		GetModuleFileNameW(NULL, &(appPath[0]), MAX_PATH);
		appPath.resize(wcslen(appPath.c_str()));
		appPath.shrink_to_fit();

		//remove inutile decoration (ie .\)
		//char	szAbsolutePath[_MAX_PATH]={0};
		//if( PathCanonicalize(szAbsolutePath, appPath) )
			//appPath = szAbsolutePath;
		std::string appPath2 = SimplifyFilePath(UTF8(appPath));

		ASSERT(!appPath2.empty());
		return appPath2.substr(0, appPath2.find_last_of("\\/") + 1);
	}

	ERMsg GetFileInfo(const std::string& filePath, CFileInfo& info)
	{
		ERMsg msg;
		//memset(&info, 0, sizeof(info) );
		struct _stat64 stat;
		int rc = _wstat64(UTF16(filePath).c_str(), &stat);
		if (rc == 0)
		{
			info.m_filePath = filePath;
			info.m_time = stat.st_mtime;
			info.m_size = stat.st_size;
		}
		else
		{
			msg.ajoute("Unable to get file information for: " + filePath);
		}

		//return rc==0;
		return msg;
	}

	CFileInfo GetFileInfo(const std::string& filePath)
	{
		CFileInfo info;
		GetFileInfo(filePath, info);
		return info;
	}

	__time64_t GetFileStamp(const std::string& filePath)
	{
		__time64_t fileStamp = 0;

		CFileInfo info;
		if (GetFileInfo(filePath, info))
			fileStamp = info.m_time;

		return fileStamp;
	}

	/*ERMsg GetFilesInfo(const StringVector& filesList, CFileInfoVector& filesInfo)
	{
		ERMsg msg;
		typedef std::pair<StringVector::const_iterator, CFileInfoVector::iterator > SVFIVIterator;

		filesInfo.resize(filesList.size());
		for(SVFIVIterator it(filesList.begin(), filesInfo.begin()); it.first!=filesList.end(); it.first++, it.second++)
			msg+=GetFileInfo(*it.first, *it.second);

		return msg;
	}
*/

	__time64_t FILETIME_to_time64(FILETIME const& ft)
	{
		ULARGE_INTEGER ull;
		ull.LowPart = ft.dwLowDateTime;
		ull.HighPart = ft.dwHighDateTime;
		return ull.QuadPart / 10000000ULL - 11644473600ULL;
	}

	void GetFilesInfo(const std::string& filter, bool bSubDirSearch, CFileInfoVector& filesInfo)
	{

		bool bAddEmptyExtension = filter.substr(filter.length() - 1) == ".";
		std::string path = GetPath(filter);



		std::string ext = GetFileExtension(filter);
		if (!ext.empty() && ext.find('*') != string::npos)
			ext.clear();

		// Find the first file in the directory.
		WIN32_FIND_DATA ffd;

		HANDLE hFind = FindFirstFileExW(UTF16(filter).c_str(), FindExInfoBasic, &ffd, FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH);

		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				bool bDirectory = (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
				bool bAddDirectory = bDirectory && bAddEmptyExtension;
				bool bSameExt = ext.empty() || IsEqual(ext, GetFileExtension(UTF8(ffd.cFileName)));

				if ((!bDirectory && bSameExt) || (bDirectory&&bAddDirectory))
				{
					CFileInfo info;
					info.m_filePath = path + UTF8(ffd.cFileName);
					info.m_time = FILETIME_to_time64(ffd.ftCreationTime);
					info.m_size = (ffd.nFileSizeHigh * ((__int64)MAXDWORD + 1)) + ffd.nFileSizeLow;

					filesInfo.push_back(info);
				}
			} while (FindNextFile(hFind, &ffd) != 0);

			FindClose(hFind);
		}

		if (bSubDirSearch)
		{
			std::string filePathTmp(filter);
			SetFileName(filePathTmp, "*.*");

			WIN32_FIND_DATA ffd;
			HANDLE hFind = FindFirstFileExW(UTF16(path + "*").c_str(), FindExInfoBasic, &ffd, FindExSearchLimitToDirectories, NULL, FIND_FIRST_EX_LARGE_FETCH);

			if (hFind != INVALID_HANDLE_VALUE)
			{
				do
				{
					bool bDirectory = (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
					bool bAddDirectory = bDirectory && bAddEmptyExtension;
					if (bDirectory)
					{
						std::string fileName = UTF8(ffd.cFileName);
						//ASSERT(findDir.IsDots() == (findDir.GetFileName() == "." || findDir.GetFileName() == ".."));
						if (!(fileName == "." || fileName == ".."))
						{
							//std::string newFilePath = path + "\\" + fileName + "\\" + GetFileName(filter);
							std::string newFilePath = path + fileName + "\\" + GetFileName(filter);
							GetFilesInfo(newFilePath, bSubDirSearch, filesInfo);
						}

					}
				} while (FindNextFile(hFind, &ffd) != 0);


				FindClose(hFind);
			}
		}
	}

	void GetFilesList(const CFileInfoVector& filesInfo, int type, StringVector& filesList)
	{
		typedef std::pair<CFileInfoVector::const_iterator, StringVector::iterator > SVFIVIterator;

		filesList.resize(filesInfo.size());
		for (SVFIVIterator it(filesInfo.begin(), filesList.begin()); it.first != filesInfo.end(); it.first++, it.second++)
		{
			switch (type)
			{
			case FILE_TITLE: *it.second = GetFileTitle(it.first->m_filePath); break;
			case FILE_NAME:  *it.second = GetFileName(it.first->m_filePath); break;
			case FILE_PATH:  *it.second = it.first->m_filePath; break;
			default: ASSERT(false);
			}
		}
	}

	StringVector GetFilesList(const std::string& filter, int type, bool bSubDirSearch)
	{
		CFileInfoVector filesInfo;
		GetFilesInfo(filter, bSubDirSearch, filesInfo);

		StringVector filesList;
		GetFilesList(filesInfo, type, filesList);
		return filesList;

		/*bool bAddEmptyExtension = filter.substr(filter.length()-1) == ".";

		// Find the first file in the directory.
		WIN32_FIND_DATA ffd;
		HANDLE hFind = FindFirstFileExW(convert(filter).c_str(), FindExInfoBasic, &ffd, FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH);

		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				bool bDirectory = (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)!=0;
				bool bAddDirectory = bDirectory&&bAddEmptyExtension;

				if( !bDirectory || bAddDirectory)
				{
					std::string str = UTF8(ffd.cFileName);

					switch( type )
					{
					case FILE_TITLE: filesList.push_back(GetFileTitle(str)); break;
					case FILE_NAME:  filesList.push_back(GetFileName(str)); break;
					case FILE_PATH:  filesList.push_back(GetPath(filter)+str.c_str()); break;
					default: ASSERT(false);
					}
				}
			}
			while (FindNextFile(hFind, &ffd) != 0);

			//DWORD dwError = GetLastError();
			//if (dwError == ERROR_NO_MORE_FILES)
			//{
			//}

			FindClose(hFind);


			if (bSubDirSearch)
			{
				string filePathTmp(filePath);
				SetFileName(filePathTmp, "*.*");

				WIN32_FIND_DATA ffd;
				HANDLE hFind = FindFirstFileExW(convert(filter).c_str(), FindExInfoBasic, &ffd, FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH);

				if (hFind != INVALID_HANDLE_VALUE)
				{
					do
					{
						bool bDirectory = (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
						bool bAddDirectory = bDirectory&&bAddEmptyExtension;

						if (!bDirectory || bAddDirectory)
						{
							std::string str = UTF8(ffd.cFileName);

							switch (type)
							{
							case FILE_TITLE: filesList.push_back(GetFileTitle(str)); break;
							case FILE_NAME:  filesList.push_back(GetFileName(str)); break;
							case FILE_PATH:  filesList.push_back(GetPath(filter) + str.c_str()); break;
							default: ASSERT(false);
							}
						}
					} while (FindNextFile(hFind, &ffd) != 0);

					//DWORD dwError = GetLastError();
					//if (dwError == ERROR_NO_MORE_FILES)
					//{
					//}

					FindClose(hFind);
				//add directory to the list

				CFileFind findDir;
				BOOL workingTmp = findDir.FindFile(filePathTmp, 0);
				while (workingTmp)
				{
					workingTmp = findDir.FindNextFile();
					if (findDir.IsDirectory())
					{
						ASSERT(findDir.IsDots() == (findDir.GetFileName() == "." || findDir.GetFileName() == ".."));
						if (!findDir.IsDots())
						{
							CString newPath = findDir.GetFilePath() + _T("\\") + GetFileName(filePath);
							GetFilesList(fileNameArray, newPath, fullPath, bSubDirSearch);
						}
					}
				}
			}
		}
		*/

	}


	StringVector GetDirectoriesList(const std::string& filter)
	{

		//std::string filter = filterIn;
		//while (IsPathEndOk(filter))
			//filter = filter.substr(0, filter.length() - 1);

		StringVector dirList;

		WIN32_FIND_DATA ffd;
		HANDLE hFind = FindFirstFileExW(UTF16(filter).c_str(), FindExInfoBasic, &ffd, FindExSearchLimitToDirectories, NULL, FIND_FIRST_EX_LARGE_FETCH);

		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				string tmp = UTF8(ffd.cFileName);
				bool bDirectory = (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

				if (bDirectory && tmp != "." && tmp != "..")
					dirList.push_back(tmp);

			} while (FindNextFile(hFind, &ffd) != 0);


			FindClose(hFind);
		}


		return dirList;
	}

	std::string OpenFileName(const char *filter, HWND owner = NULL)
	{
		OPENFILENAMEW ofn;

		std::wstring wfilter = UTF16(filter);
		std::replace(wfilter.begin(), wfilter.end(), '|', '\0');
		std::wstring wfileName; wfileName.resize(MAX_PATH);

		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = owner;
		ofn.lpstrFilter = &(wfilter[0]);
		ofn.lpstrFile = &(wfileName[0]);
		ofn.nMaxFile = MAX_PATH;
		ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
		ofn.lpstrDefExt = L"";
		ofn.lpstrInitialDir = L"";

		std::string fileNameStr;
		if (GetOpenFileNameW(&ofn))
			fileNameStr = UTF8(wfileName);

		return fileNameStr;
	}

	ERMsg AskToFindApplication(std::string appType, std::string appFilePath, HWND hCaller)
	{
		ERMsg msg;

		//ASSERT(hCaller != NULL);

		std::string title = GetFileName(GetApplicationPath());
		std::string appName = GetFileName(appFilePath);
		std::string question = FormatMsg(IDS_BSC_APP_NOTFOUND, appName);

		if (MessageBoxA(hCaller, question.c_str(), title.c_str(), MB_YESNO) == IDYES)
		{
			std::string appFilePath = OpenFileName(GetString(IDS_STR_FILTER_EXECUTABLE).c_str(), hCaller);

			if (!appFilePath.empty())
			{
				CRegistry registry;
				registry.SetAppFilePath(appType, appFilePath);
			}
			else
			{
				msg.ajoute(FormatMsg(IDS_BSC_APP_NOTEXEC, appFilePath));
			}
		}
		else
		{
			msg.ajoute(FormatMsg(IDS_BSC_APP_NOTEXEC, appFilePath));
		}

		return msg;
	}

	ERMsg CallApplication(std::string appType, std::string argument, HWND hCaller, int showMode, bool bAddCote, bool bWait)
	{
		ERMsg msg;

		CRegistry registry;

		std::string appFilePath = registry.GetAppFilePath(appType);

		if (GetPath(appFilePath).empty())//if the path is not specified, take the current application path.
			appFilePath = GetApplicationPath() + appFilePath;

		if (bAddCote)
			argument = '\"' + argument + '\"';

		std::string command = '\"' + appFilePath + "\" " + argument;

		if (bWait)
		{
			msg = WinExecWait(command.c_str(), "", showMode);
		}
		else
		{
			if (WinExec(command.c_str(), showMode) < 31)
			{
				msg.ajoute(FormatMsg(IDS_BSC_APP_NOTEXEC, appFilePath));
			}
		}

		if (!msg)
		{
			msg = AskToFindApplication(appType, appFilePath, hCaller);
			if (msg)
			{
				appFilePath = registry.GetAppFilePath(appType);

				if (GetPath(appFilePath).empty())//if the path is not specified, take the current application path.
					appFilePath = GetApplicationPath() + appFilePath;

				command = '\"' + appFilePath + "\" " + argument;

				if (bWait)
				{
					msg = WinExecWait(command, "", showMode);
				}
				else
				{
					if (WinExec(command.c_str(), showMode) < 31)
					{
						msg.ajoute(FormatMsg(IDS_BSC_APP_NOTEXEC, appFilePath));
					}
				}
			}
		}

		return msg;
	}


	ERMsg WinExecWait(const std::string& command, std::string inputDir, UINT uCmdShow, LPDWORD pExitCode)
	{
		ERMsg msg;

		while (IsPathEndOk(inputDir))
			inputDir = inputDir.substr(0, inputDir.length() - 1);

		STARTUPINFO si = { 0 };
		si.cb = sizeof(si);
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = uCmdShow;

		PROCESS_INFORMATION pi = { 0 };

		std::wstring wdir(UTF16(inputDir));
		std::wstring wcommand = UTF16(command);
		LPCWSTR pDir = wdir.empty() ? NULL : wdir.c_str();

		if (::CreateProcessW(NULL, &(wcommand[0]), NULL, NULL, FALSE, NULL, NULL, pDir, &si, &pi))
		{
			::CloseHandle(pi.hThread);
			::WaitForSingleObject(pi.hProcess, INFINITE);

			if (pExitCode != NULL)
				::GetExitCodeProcess(pi.hProcess, pExitCode);
		}
		else
		{
			msg = GetLastErrorMessage();
			msg.ajoute(std::string("Unable to execute command: ") + command);
		}

		return msg;
	}

	HWND FindMyTopMostWindow()
	{
		DWORD dwProcID = GetCurrentProcessId();
		HWND hWnd = GetTopWindow(GetDesktopWindow());
		while (hWnd)
		{
			DWORD dwWndProcID = 0;
			GetWindowThreadProcessId(hWnd, &dwWndProcID);
			if (dwWndProcID == dwProcID)
				return hWnd;
			hWnd = GetNextWindow(hWnd, GW_HWNDNEXT);
		}
		return NULL;
	}

	bool SetClipboardText(const std::string& str)
	{
		bool bRep = false;

		HWND hWnd = FindMyTopMostWindow();
		if (OpenClipboard(hWnd))
		{
			EmptyClipboard();

			// Allouer de la mémoire relocalisable globale
			//-------------------------------------------
			HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, str.length() + 1);

			// Verrouiller le bloc afin d'obtenir un pointeur éloigné
			// sur cette mémoire
			char* pBuffer = (char*)GlobalLock(hMem);

			// Copier la chaîne dans cette mémoire
			strcpy_s(pBuffer, str.length() + 1, str.c_str());

			// Relâche la mémoire et copier sur le clipboard
			GlobalUnlock(hMem);

			SetClipboardData(CF_TEXT, hMem);


			bRep = CloseClipboard() != FALSE;
		}

		return bRep;
	}

	std::string GetClipboardText()
	{
		std::string str;

		HWND hWnd = FindMyTopMostWindow();
		if (OpenClipboard(hWnd))
		{
			// Mettre la main sur le bloc de mémoire
			// référé par le texte
			HGLOBAL hMem = GetClipboardData(CF_TEXT);

			char* pBuffer = (char*)GlobalLock(hMem);
			if (pBuffer)
			{
				// Verrouiller la mémoiure du Clipboard qu'on puisse reférer
				// à la chaîne actuelle
				str = pBuffer;
				GlobalUnlock(hMem);
			}
		}

		CloseClipboard();

		return str;
	}

	int GetCrc32(const std::string& str, ULONGLONG begin, ULONGLONG end)
	{
		ASSERT(begin <= end);
		boost::crc_32_type result;

		if (begin >= 0 && begin < str.size() && end <= 0 && end < str.size() && begin < end)
		{
			result.process_bytes(&(str.at((size_t)begin)), size_t(end - begin));
		}
		else
		{
			result.process_bytes(str.data(), str.length());
		}

		return result.checksum();
	}

	int GetEndNumber(std::string name)
	{
		int number = 0;
		if (!name.empty())
		{
			std::string::size_type posBeg = name.find_last_not_of("0123456789");
			std::string::size_type posEnd = name.find_last_of("0123456789");
			if (posBeg != std::string::npos && posEnd != std::string::npos && name[posBeg] == ' ' && posEnd == name.length() - 1)
			{
				number = ToValue<int>(name.substr(posBeg + 1, posEnd - posBeg));
			}
		}

		return number;
	}

	std::string GenerateNewName(std::string name)
	{
		if (!name.empty())
		{
			int no = GetEndNumber(name);
			if (no == 0)
			{
				name += " 2";
			}
			else
			{
				std::string noStr = ToString(no);
				std::string::size_type pos = name.rfind(noStr);
				ASSERT(pos != std::string::npos);
				name = name.substr(0, pos) + ToString(no + 1);
			}
		}

		return name;
	}

	std::string GenerateNewFileName(std::string name)
	{
		if (!name.empty())
		{
			while (FileExists(name))
			{
				std::string title = GetFileTitle(name);
				title = GenerateNewName(title);
				SetFileTitle(name, title);
			}

		}

		return name;
	}

	ERMsg CreateMultipleDir(const std::string& path)
	{
		ERMsg msg;

		if (path.empty())
			return msg;


		std::string tmp(path);
		while (IsPathEndOk(tmp))//remove \ and //
			tmp = tmp.substr(0, tmp.length() - 1);

		if (tmp != "c:" && tmp != "C:" && tmp.size() > 2)
		{
			if (!DirectoryExists(path))
			{
				std::string::size_type pos = tmp.find_last_of("\\/");
				if (pos != std::string::npos)
				{
					std::string subDir = tmp.substr(0, pos);
					msg = CreateMultipleDir(subDir);
					if (msg)
					{
						//can't create directory when a file without extension with the same name exist!
						if (!CreateDirectoryW(UTF16(tmp).c_str(), NULL))
						{
							DWORD dw = GetLastError();
							if (dw != ERROR_ALREADY_EXISTS)
							{
								msg = GetLastErrorMessage();
								return msg;
							}

						}
					}
				}
			}
		}

		return msg;
	}

	size_t GetTotalSystemMemory()
	{
		MEMORYSTATUSEX status;
		status.dwLength = sizeof(status);
		GlobalMemoryStatusEx(&status);
		return status.ullTotalPhys;
	}

	ERMsg GetLastErrorMessage()
	{
		ERMsg msg;
		LPSTR lpMsgBuf = NULL;

		DWORD dw = GetLastError();

		FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dw,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPSTR)&lpMsgBuf,
			0, NULL);

		// Display the error message and clean up
		std::string str = "Failed with error " + std::to_string(dw) + ": " + lpMsgBuf;
		msg.ajoute(str);

		LocalFree(lpMsgBuf);

		return msg;
	}



	std::string SecondToDHMS(double time)
	{
		int d = int(time / (24 * 60 * 60)); time -= d * 24 * 60 * 60;
		int h = int(time / (60 * 60));  time -= h * 60 * 60;
		int m = int(time / 60);  time -= m * 60;
		double s = time;

		std::string str;
		str.resize(100);

		if (d > 0)
			_snprintf(&(str[0]), 100, "%d, %02d:%02d:%02.3lf", d, h, m, s);
		else if (h > 0 || d > 0)
			_snprintf(&(str[0]), 100, "%02d:%02d:%02.3lf", h, m, s);
		else if (m > 0 || h > 0 || d > 0)
			_snprintf(&(str[0]), 100, "%02d:%02.3lf", m, s);
		else _snprintf(&(str[0]), 100, "%02.3lf", s);
		str.resize(strlen(str.c_str()));
		return str;
	}


	std::string GetTempPath()
	{
		DWORD size = ::GetTempPathW(0, NULL);

		std::wstring tmp;
		tmp.resize(size);
		::GetTempPathW(size, &(tmp[0]));
		//tmp.ReleaseBuffer();

		return UTF8(tmp);
	}

	std::string GetUserDataPath()
	{
		std::wstring path;
		path.insert(path.begin(), MAX_PATH, 0);
		HRESULT result = SHGetFolderPathW(0, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, &(path[0]));
		path.resize(wcslen(path.c_str()));
		path.shrink_to_fit();

		if (result == S_OK)
			path += L"\\NRCan\\";
		else
			path = UTF16(GetTempPath());

		return UTF8(path);
	}




	std::string GetVersionString(const std::string& filerPath)
	{
		std::string version;

		DWORD dwDummy;
		DWORD dwFVISize = GetFileVersionInfoSizeW(UTF16(filerPath).c_str(), &dwDummy);
		if (dwFVISize > 0)
		{
			//LPBYTE lpVersionInfo = new BYTE[dwFVISize]; 
			std::wstring tmp;
			tmp.resize(dwFVISize);
			VERIFY(GetFileVersionInfoW(UTF16(filerPath).c_str(), 0, dwFVISize, &(tmp[0])));

			UINT uLen = 0;
			VS_FIXEDFILEINFO *lpFfi = NULL;
			VerQueryValueW(tmp.data(), L"\\", (LPVOID *)&lpFfi, &uLen);
			DWORD dwFileVersionMS = lpFfi->dwFileVersionMS;
			DWORD dwFileVersionLS = lpFfi->dwFileVersionLS;
			//delete [] lpVersionInfo; 

			DWORD dwLeftMost = HIWORD(dwFileVersionMS);
			DWORD dwSecondLeft = LOWORD(dwFileVersionMS);
			DWORD dwSecondRight = HIWORD(dwFileVersionLS);
			DWORD dwRightMost = LOWORD(dwFileVersionLS);

			version = ToString(dwLeftMost) + "." + ToString(dwSecondLeft) + "." + ToString(dwSecondRight) + "." + ToString(dwRightMost);
		}

		return version;
	}

	std::string GetCompilationDateString(char *compilation_date)
	{
		std::string str;

		char *months[12] = { "Jan","Feb","Mar","Apr","May","Jun",
							"Jul","Aug","Sep","Oct","Nov","Dec" };


		int month = -1;
		for (int i = 0; i < 12; i++)
		{
			if (memcmp(compilation_date, months[i], 3) == 0)
			{
				month = i;
				break;
			}
		}

		if (month != -1)
		{
			char year[5] = { 0 };
			char day[3] = { 0 };
			char hour[3] = { 0 };
			char minute[3] = { 0 };
			char second[3] = { 0 };

			memcpy(year, compilation_date + 7, 4);
			memcpy(day, compilation_date + 4, 2);

			//CTime time(ToInt(year), month+1, ToInt(day), 12,0,0 );
			str = FormatTime("%x", ToInt(year), month, ToSizeT(day) - 1);
		}

		return str;
	}

	struct StringComparator
	{
		StringComparator(const std::string &nameToFind) : m_str(nameToFind) {}
		~StringComparator() {}

		bool operator () (const std::string & str) const { return boost::iequals(str, m_str); }

		const std::string & m_str;
	};

	StringVector::const_iterator FindStringExact(const StringVector& list, const std::string& value, bool bCaseSensitive)
	{
		if (!bCaseSensitive)
			return find_if(list.begin(), list.end(), StringComparator(value));

		return find(list.begin(), list.end(), value);
	}



	std::string GetText(ERMsg msg)
	{
		std::string messStr;

		static CCriticalSection CS;

		CS.Enter();
		for (unsigned int i = 0; i < msg.dimension(); i++)
		{
			if (!messStr.empty())
				messStr += "\n";

			messStr += msg[i].data();
		}
		CS.Leave();

		return messStr;
	}



	std::string& ReplaceString(std::string& str, const std::string& oldStr, const std::string& newStr)
	{
		//int nbReplace = 0;
		size_t pos = 0;
		while ((pos = str.find(oldStr, pos)) != std::string::npos)
		{
			str.replace(pos, oldStr.length(), newStr);
			pos += newStr.length();
			//nbReplace++;
		}

		return str;
	}

	std::string RemoveAccented(std::string str)
	{
		for (string::iterator it = str.begin(); it != str.end(); it++)
		{
			static const char*
				//   "ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõö÷øùúûüýþÿ"
				//tr = "AAAAAAECEEEEIIIIDNOOOOOx0UUUUYPsaaaaaaeceeeeiiiiOnooooo/0uuuuypy";
				tr = "AAAAAAECEEEEIIIIDNOOOOOx0UUUUYPsaaaaaaeceeeeiiiiOnoooooouuuuypy";
			unsigned char ch = *it;
			if (ch >= 192)
				*it = tr[ch - 192];

			//++p; // http://stackoverflow.com/questions/14094621/
		}
		//ÿ
		return str;
	}

	std::string PurgeFileName(std::string name)
	{
		std::replace(name.begin(), name.end(), '\\', '-');
		std::replace(name.begin(), name.end(), '/', '-');
		std::replace(name.begin(), name.end(), '\"', '-');
		std::replace(name.begin(), name.end(), ':', '-');
		std::replace(name.begin(), name.end(), '*', '-');
		std::replace(name.begin(), name.end(), '?', '-');
		std::replace(name.begin(), name.end(), '<', '-');
		std::replace(name.begin(), name.end(), '>', '-');
		std::replace(name.begin(), name.end(), '|', '-');
		std::replace(name.begin(), name.end(), '\t', ' ');
		std::replace(name.begin(), name.end(), '.', ' ');
		//, is not a problem for file name but is problem in CSV file
		std::replace(name.begin(), name.end(), ',', '-');
		std::replace(name.begin(), name.end(), ';', '-');
		std::replace(name.begin(), name.end(), '\"', ' ');
		std::replace(name.begin(), name.end(), '\'', ' ');
		ReplaceString(name, "œ", "oe");

		//std::replace(name.begin(), name.end(), 'œ', 'oe');

		Trim(name);

		return name;
	}

	string ANSI_2_ASCII(std::string str)
	{
		int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);

		std::wstring w_text;
		w_text.resize(len);
		MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &(w_text[0]), len);

		//Convert UTF16 to ASCII encoding
		static const UINT US_ASCII = 20127;
		int newLen = WideCharToMultiByte(US_ASCII, 0, w_text.c_str(), -1, NULL, 0, 0, 0);

		str.resize(newLen);
		WideCharToMultiByte(US_ASCII, 0, w_text.c_str(), -1, &(str[0]), newLen, 0, 0);
		str.resize(strlen(str.c_str()));

		return str;
	}

	//typedef std::wstring_convert<std::codecvt<wchar_t, char, std::mbstate_t>, wchar_t> CONVERT_STRING_DEF;
	//static CONVERT_STRING_DEF CONVERT_STRING;

	std::wstring UTF8_UTF16(const std::string& str)
	{
		std::wstring out;

		try
		{
			//typedef std::codecvt_utf8<wchar_t> convert_typeX;
			//std::wstring_convert<convert_typeX, wchar_t> converterX;

			//out = converterX.from_bytes(str);

			typedef std::wstring_convert<std::codecvt<wchar_t, char, std::mbstate_t>, wchar_t> CONVERT_STRING_DEF;
			CONVERT_STRING_DEF CONVERT_STRING;
			out = CONVERT_STRING.from_bytes(str);
			//out.resize(strlen(out.length()));
			out.resize(wcslen(out.c_str()));
			//out.shrink_to_fit();
		}
		catch (...)
		{

		}

		return out;
	}

	std::string UTF16_UTF8(const std::wstring& str)
	{


		std::string out;
		try
		{
			//typedef std::codecvt_utf8<wchar_t> convert_typeX;
			//std::wstring_convert<convert_typeX, wchar_t> converterX;

			//out = converterX.to_bytes(str);

			typedef std::wstring_convert<std::codecvt<wchar_t, char, std::mbstate_t>, wchar_t> CONVERT_STRING_DEF;
			CONVERT_STRING_DEF CONVERT_STRING;
			out = CONVERT_STRING.to_bytes(str);
			//out.resize(out.length());
			out.resize(strlen(out.c_str()));
			//out.shrink_to_fit();
		}
		catch (...)
		{
			//int i;
			//i = 0;
			out = std::string(str.begin(), str.end());
		}

		return out;
	}



	// change a char's encoding from UTF8 to ANSI
	std::string UTF8_ANSI(const std::string& u8)
	{
		int wcsLen = ::MultiByteToWideChar(CP_UTF8, NULL, u8.c_str(), (int)u8.length(), NULL, 0);
		std::wstring wszString;	wszString.resize(wcsLen);
		::MultiByteToWideChar(CP_UTF8, NULL, u8.c_str(), (int)u8.length(), ((wchar_t*)(&wszString[0])), wcsLen);

		int ansiLen = ::WideCharToMultiByte(CP_ACP, NULL, wszString.c_str(), (int)wszString.length(), NULL, 0, NULL, NULL);
		std::string szAnsi;	szAnsi.resize(ansiLen);
		::WideCharToMultiByte(CP_ACP, NULL, wszString.c_str(), (int)wszString.length(), ((char*)(&szAnsi[0])), ansiLen, NULL, NULL);

		return szAnsi;
	}

	// change a char's encoding from ANSI to UTF8
	std::string ANSI_UTF8(const std::string& szAnsi)
	{
		int wcsLen = ::MultiByteToWideChar(CP_ACP, NULL, szAnsi.c_str(), (int)szAnsi.length(), NULL, 0);
		std::wstring wszString;	wszString.resize(wcsLen);
		::MultiByteToWideChar(CP_ACP, NULL, szAnsi.c_str(), (int)szAnsi.length(), ((wchar_t*)(&wszString[0])), wcsLen);

		int u8Len = ::WideCharToMultiByte(CP_UTF8, NULL, wszString.c_str(), (int)wszString.length(), NULL, 0, NULL, NULL);
		std::string szU8; szU8.resize(u8Len);
		::WideCharToMultiByte(CP_UTF8, NULL, wszString.c_str(), (int)wszString.length(), ((char*)(&szU8[0])), u8Len, NULL, NULL);

		return szU8;
	}

	std::string Encrypt(const std::string& str, const std::string& key)
	{
		std::string strOut;
		if (!str.empty())
		{
			try
			{
				int l = int(ceil((double)str.length() / key.length())*key.length() + 1);

				std::string init;
				for (int i = 0; i < l; i++)
					init += '\0';


				std::CRijndael oRijndael;
				oRijndael.MakeKey(key.c_str(), init.c_str(), int(key.length()), int(key.length()));

				strOut.resize(l);
				oRijndael.EncryptBlock(str.c_str(), &(strOut[0]));
				ASSERT(strOut.size() == l);
				strOut.resize(strlen(strOut.c_str()));
			}
			catch (...)
			{

			}
		}

		return strOut;
	}



	std::string Decrypt(const std::string& str, const std::string& key)
	{
		std::string strOut;
		if (!str.empty())
		{
			try
			{
				int l = int(ceil((double)str.length() / key.length())*key.length() + 1);

				std::string init;
				for (int i = 0; i < l; i++)
					init += '\0';

				std::CRijndael oRijndael;
				oRijndael.MakeKey(key.c_str(), init.c_str(), int(key.length()), int(key.length()));
				strOut.resize(l);
				oRijndael.DecryptBlock(str.c_str(), &(strOut[0]));
				strOut.resize(strlen(strOut.c_str()));
			}
			catch (...)
			{

			}
			//strOut.ReleaseBuffer();
		}

		return strOut;
	}


	std::string& UppercaseFirstLetter(std::string& str)
	{

		std::transform(str.begin(), str.end(), str.begin(), ::tolower);

		bool cap_it = true;
		for (size_t i = 0; i < str.length(); i++)
		{
			if (str[i] < 0 || isalpha(str[i]) || isdigit(str[i]))
			{
				if (cap_it == true)
				{
					str[i] = toupper(str[i]);
					cap_it = false;
				}
			}
			else cap_it = true;
			//if (isspace(str[i]))
		}

		return str;

	}



	/*	const char *reg_esp = "[ ,.\\t\\n;:]";  // List of separator characters.

	// this can be done using raw string literals:
	// const char *reg_esp = R"([ ,.\t\n;:])";

	std::regex rgx(reg_esp); // 'regex' is an instance of the template class
	// 'basic_regex' with argument of type 'char'.
	std::cmatch match; // 'cmatch' is an instance of the template class
	// 'match_results' with argument of type 'const char *'.
	const char *target = "Unseen University - Ankh-Morpork";

	// Identifies all words of 'target' separated by characters of 'reg_esp'.
	if (std::regex_search(target, match, rgx)) {
	// If words separated by specified characters are present.

	const size_t n = match.size();
	for (size_t a = 0; a < n; a++) {
	std::string str (match[a].first, match[a].second);
	std::cout << str << "\n";
	}
	}

	*/
	//StringVector Tokenize(const std::string& str, const std::string& delimiters, bool bEliminateDuplicateSep, std::string::size_type begin, std::string::size_type end) 
	//{
	//	ASSERT(begin<=end);
	//	//ASSERT(begin<str.size());

	//	StringVector tokens;
	//	if( end==std::string::npos )
	//		end=str.size();

	//	std::string::size_type pos = begin;


	//	if(str.length()<1)  
	//		return tokens;

	//	while(pos<end)
	//	{
	//		std::string::size_type delimPos = std::min(end, str.find_first_of(delimiters, pos));
	//		std::string::size_type tokenPos = std::min(end, str.find_first_not_of(delimiters, pos));

	//		//if(end != delimPos)
	//		//{
	//		if(end != tokenPos)
	//		{
	//			if(tokenPos<delimPos)
	//			{
	//				tokens.push_back(str.substr(pos,delimPos-pos));
	//			}
	//			else
	//			{
	//				tokens.push_back("");
	//			}
	//		}
	//		else
	//		{
	//			tokens.push_back("");
	//		}

	//		pos = delimPos+1;
	//			
	//		if( bEliminateDuplicateSep )
	//		{
	//			ASSERT(str.length()==str.size());
	//			while( pos<str.length() && delimiters.find(str[pos])!=std::string::npos )
	//				pos++;
	//		}
	//	//} 
	//	//else 
	//	//{
	//	//	if(end != tokenPos)
	//	//	{
	//	//		if(tokenPos<delimPos)
	//	//		{
	//	//			tokens.push_back(str.substr(pos, delimPos-pos));
	//	//		}
	//	//	} 
	//	//	else 
	//	//	{
	//	//		//tokens.push_back("");
	//	//	}
	//	//	break;
	//	//}
	//	}

	//	return tokens;
	//}

	std::string GetString(UINT ID) { return LoadString(ID); }

	std::string ToStringDMS(double coord, bool bWithFraction)
	{
		std::string deg;
		std::string min;
		std::string sec;

		std::string str;

		int prec = 0;
		if (bWithFraction)
			prec = 2;

		double mult = pow(10.0, prec);
		deg = ToString(GetDegrees(coord, mult));
		min = ToString(GetMinutes(coord, mult));
		sec = ToString(GetSeconds(coord, mult), prec);

		if (sec == "0" || sec == "-0")
			sec.clear();
		if (sec.empty() && (min == "0" || min == "-0"))
			min.clear();


		return deg + min + sec;
	}

	//std::string ToString(const CGeoRectIndex& rect )
	//{
	//	
	//	return ToString(rect.top) + " " + ToString(rect.left) + " " + ToString( rect.bottom) + " " + ToString(rect.right);
	//}

	//CGeoRectIndex ToCRect(const std::string& str)
	//{
	//	CGeoRectIndex rect;

	//	sscanf_s( str.c_str(), "%d %d %d %d", &rect.top, &rect.left, &rect.bottom, &rect.right);

	//	return rect;
	//}

	COLORREF ToCOLORREF(const std::string& str)
	{
		int r = 0;
		int g = 0;
		int b = 0;
		sscanf_s(str.c_str(), "%d %d %d", &r, &g, &b);

		return RGB(r, g, b);
	}


	void ToVector(const std::string& str, std::vector<std::string>& val, const std::string& sep)
	{
		val = Tokenize(str, sep);
	}

	typedef boost::iterator_range<std::string::const_iterator> StringRange;
	/*class StringRange: public StringRangeBase
	{
	public:

		StringRange( std::string::const_iterator Begin, std::string::const_iterator End ) :
		StringRangeBase(Begin, End )
		{}

		StringRange(const StringRange& in):
		StringRangeBase(in.begin(), in.end() )
		{}

		template< class Iterator >
			StringRange( Iterator Begin, Iterator End ) :
				StringRangeBase(Begin, End )
			{}

		//StringRange(StringRangeBase::iterator it)
		//{
		//}

		//using boost::iterator_range<std::string::const_iterator>::iterator_range<std::string::const_iterator>;
	};
	*/


	std::string FindString(const std::string& source, const std::string& strBegin, const std::string& strEnd, std::string::size_type& posBegin, std::string::size_type& posEnd)
	{
		std::string text;
		posBegin = source.find(strBegin, posEnd);

		if (posBegin != std::string::npos)
		{
			posBegin += strBegin.length();
			posEnd = source.find(strEnd, posBegin);
			ASSERT(posEnd != std::string::npos);
			if (posEnd != std::string::npos)
				text = source.substr(posBegin, posEnd - posBegin);
		}

		return text;
	}

	std::string FindString(const std::string& source, const std::string& strBegin, const std::string& strEnd, std::string::size_type& posBegin)
	{
		ASSERT(posBegin != std::string::npos);

		std::string text;
		posBegin = source.find(strBegin, posBegin);

		if (posBegin != std::string::npos)
		{
			posBegin += strBegin.length();
			std::string::size_type posEnd = source.find(strEnd, posBegin);
			ASSERT(posEnd != std::string::npos);
			if (posEnd != std::string::npos)
			{
				text = source.substr(posBegin, posEnd - posBegin);
				posBegin = posEnd;
			}
		}

		return text;
	}

	std::string FindString(const std::string& source, const std::string& strBegin, const std::string& strEnd)
	{
		std::string::size_type posBegin = 0;
		return FindString(source, strBegin, strEnd, posBegin);
	}

	static bool FindSensitive(const std::string& str1, const std::string& str2)
	{
		return boost::find_first(StringRange(str1.begin(), str1.end()), StringRange(str2.begin(), str2.end())).begin() != str1.end();
	}

	static bool FindInsensitive(const std::string& str1, const std::string& str2)
	{
		return boost::ifind_first(StringRange(str1.begin(), str1.end()), StringRange(str2.begin(), str2.end())).begin() != str1.end();
	}

	bool Find(const std::string& str1, const std::string& str2, bool bCaseSensitive)
	{
		return bCaseSensitive ? FindSensitive(str1, str2) : FindInsensitive(str1, str2);
	}

	bool IsEqualNoCase(const std::string& in1, const std::string& in2)
	{
		return boost::iequals(in1, in2);
	}

	std::string::size_type GetNextLinePos(const std::string& str, std::string::size_type begin)
	{
		std::string::size_type end = std::min(str.size(), str.find_first_of("\r\n", begin));
		while (end < str.size() && (str[end] == '\r' || str[end] == '\n'))
			end++;

		return end;
	}

	std::string Tokenize(const std::string& str, const std::string& delimiters, std::string::size_type& pos, bool bRemoveDuplicate, std::string::size_type posEnd)
	{
		ASSERT(pos != std::string::npos);

		if (posEnd == std::string::npos)
			posEnd = str.length();

		if (bRemoveDuplicate)
			pos = str.find_first_not_of(delimiters, pos);

		std::string tmp;
		if (pos < posEnd)//str.length())
		{
			char const* p = str.c_str() + pos;

			char const* q = strpbrk(p, delimiters.c_str());
			if (q != NULL)
			{

				tmp = StringVector::value_type(p, q);
				p = q + 1;
				pos = p - str.c_str();
			}
			else
			{
				tmp = str.substr(pos);
				pos = std::string::npos;
			}

			if (bRemoveDuplicate && pos != std::string::npos)
				pos = str.find_first_not_of(delimiters, pos);
		}
		else
		{
			pos = std::string::npos;
		}

		return tmp;
	}

	StringVector Tokenize(const std::string& str, const std::string& delimiter, bool bRemoveDuplicate, std::string::size_type pos, std::string::size_type posEnd)
	{
		return StringVector().Tokenize(str, delimiter, bRemoveDuplicate, pos, posEnd);
	}


	//est-ce qu'il y a une différence de performence entre les deux
	typedef tokenizer<escaped_list_separator<char> > so_tokenizer;
	bool TokenizeWithQuote(const std::string& str, char sep, StringVector& out)
	{
		try
		{
			out.clear();
			so_tokenizer tok(str, escaped_list_separator<char>('\\', sep, '\"'));
			for (so_tokenizer::iterator beg = tok.begin(); beg != tok.end(); ++beg)
				out.push_back(*beg);
		}
		catch (...)
		{
			int i;
			i = 0;
		}

		return !out.empty();
	}

	bool TokenizeWithQuote(const std::string& str, char* sep, StringVector& out)
	{

		try
		{
			out.clear();
			so_tokenizer tok(str, escaped_list_separator<char>("\\", sep, "\""));
			for (so_tokenizer::iterator beg = tok.begin(); beg != tok.end(); ++beg)
				out.push_back(*beg);
		}
		catch (...)
		{
			int i;
			i = 0;
		}

		return !out.empty();
	}

	std::string FormatA(PCSTR szFormat, ...)
	{
		va_list argList;
		va_start(argList, szFormat);
		std::string strOut = FormatV(szFormat, argList);
		va_end(argList);
		return strOut;
	}

	std::string FormatV(const char* szFormat, va_list argList)
	{
		char* pBuf = NULL;
		int nChars = 1;
		int nUsed = 0;
		std::string::size_type nActual = 0;
		int nTry = 0;

		do
		{
			// Grow more than linearly (e.g. 512, 1536, 3072, etc)

			nChars += ((nTry + 1) * 2048);
			pBuf = reinterpret_cast<char*>(_alloca(sizeof(char)*nChars));
			nUsed = _vsnprintf(pBuf, nChars - 1, szFormat, argList);

			// Ensure proper NULL termination.

			nActual = nUsed == -1 ? nChars - 1 : std::min(nUsed, nChars - 1);
			pBuf[nActual + 1] = '\0';


		} while (nUsed < 0 && nTry++ < 5);

		// assign whatever we managed to format
		return std::string(pBuf, nActual);
	}

	std::string FormatMsgA(PCSTR szFormat, ...) //throw(std::exception)
	{
		std::string str;
		va_list argList;
		va_start(argList, szFormat);
		PSTR szTemp = NULL;
		if (FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
			szFormat, 0, 0,
			reinterpret_cast<LPSTR>(&szTemp), 0, &argList) > 0 &&
			szTemp != NULL)
		{
			//throw std::runtime_error("out of memory");
			//}
			str = szTemp;
			LocalFree(szTemp);
			va_end(argList);
		}

		return str;
	}

	//*************************************************************************

	template<typename T, size_t N>
	T * my_end(T(&ra)[N]) {
		return ra + N;
	}

	StringVector::StringVector(const char* str_array[])
	{
		
		size_t size = sizeof(str_array) / sizeof(const char*);
		resize(size);
		for (size_t i = 0; i < size; i++)
			at(i) = str_array[i];
		//std::vector<std::string> tmp( str_array );
		//std::vector<std::string> tmp(str_array, my_end(str_array));
		//operator=();
	}

	void StringVector::LoadString(UINT ID, const std::string& delimiters)
	{
		std::string str = GetString(ID);
		Tokenize(str, delimiters, false);
	}


	size_t StringVector::Find(const std::string& str, bool bCaseSensitive, bool bExact)const
	{
		std::set<size_t> cols = FindAll(str, bCaseSensitive, bExact);

		return cols.empty() ? UNKNOWN_POS : *cols.begin();
	}

	std::set<size_t> StringVector::FindAll(const std::string& str, bool bCaseSensitive, bool bExact)const
	{
		std::set<size_t> posVector;

		for (size_t i = 0; i < size(); i++)
		{
			if (bExact)
			{
				if (IsEqual(str, at(i), bCaseSensitive))
					posVector.insert(i);
			}
			else
			{
				if (WBSF::Find(str, at(i), bCaseSensitive))
					posVector.insert(i);
			}
		}


		return posVector;
	}

	std::ostream& StringVector::operator >> (std::ostream& stream)const
	{
		stream << "{";
		for (StringVector::const_iterator it = begin(); it != end(); it++)
		{
			stream << *it;
			stream << "|";
		}

		stream << "}";

		return stream;
	}

	std::istream& StringVector::operator << (std::istream& stream)
	{
		std::string s;
		getline(stream, s, '}');

		Tokenize(s, "{|}");

		return stream;
	}



	StringVector& StringVector::Tokenize(const std::string& str, const std::string& delimiters, bool bRemoveDuplicate, std::string::size_type pos, std::string::size_type posEnd)
	{

		clear();

		if (!str.empty())
		{
			while (pos != std::string::npos)
			{
				std::string str2 = WBSF::Tokenize(str, delimiters, pos, bRemoveDuplicate, posEnd);
				if (!bRemoveDuplicate || !str2.empty())
					push_back(str2);
			}
		}

		return *this;

	}

	StringVector& StringVector::TokenizeQuoted(std::string command, const std::string& delimiters)
	{
		//static const char REPLACEMENT[14] = { 'Ò', 'Ó', 'Õ', 'Ö', 'Ý', 'Ÿ', 'Å', 'ò', 'ó', 'õ', 'ö', 'ý', 'ÿ', 'å' };

		//std::string         copy;
		//
		//str.erase(std::remove(str.begin(), str.end(), '\r'), str.end());
		//
		//ASSERT(delimiters.length() < 14);

		////replace separator inside double cote by temparary separator
		//bool bOpen = false;
		//for (std::string::iterator it = str.begin(); it != str.end(); ++it)
		//{
		//	if (*it == '\"')
		//	{
		//		bOpen = !bOpen;
		//	}
		//	else if (bOpen)
		//	{
		//		char const* q = delimiters.c_str();
		//		while (*q != NULL)
		//		{
		//			if (*it == *q)
		//			{
		//				if (copy.empty())
		//				{
		//					copy = str;
		//				}

		//				ASSERT((q - delimiters.c_str()) >= 0 && (q - delimiters.c_str()) < 14);
		//				*it = REPLACEMENT[q - delimiters.c_str()];
		//				break;
		//			}
		//			q++;
		//		}
		//	}
		//}


		//std::stringstream   lineStream(str);
		//std::string         cell;



		//if (delimiters.length() == 1)
		//{
		//	while (std::getline(lineStream, cell, m_pD[0]))
		//	{
		//		push_back(cell);
		//	}
		//}
		//else
		//{
		//	Tokenize(m_line, m_pD, m_bDQ);
		//}

		//if (m_bDQ)
		//{
		//	if (!copy.empty())
		//	{
		//		m_line = copy;
		//		for (std::vector<std::string>::iterator it2 = begin(); it2 != end(); ++it2)
		//		{
		//			bool bOpen = false;
		//			for (std::string::iterator it = it2->begin(); it != it2->end(); ++it)
		//			{
		//				if (*it == '\"')
		//				{
		//					bOpen = !bOpen;
		//				}
		//				else if (bOpen)
		//				{
		//					char const* q = m_pD;
		//					while (*q != NULL)
		//					{
		//						if (*it == REPLACEMENT[q - m_pD])
		//						{
		//							*it = *q;
		//							break;
		//						}
		//						q++;
		//					}
		//				}
		//			}
		//		}
		//	}

		//	//remove "
		//	for (std::vector<std::string>::iterator it = begin(); it != end(); it++)
		//		it->erase(std::remove(it->begin(), it->end(), '"'), it->end());
		//}


		int len = int(command.length());
		bool qot = false, sqot = false;
		int arglen;
		for (int i = 0; i < len; i++)
		{
			int start = i;
			if (command[i] == '\"')
				qot = true;
			else if (command[i] == '\'')
				sqot = true;

			if (qot)
			{
				i++;
				start++;
				while (i < len && command[i] != '\"')
					i++;
				if (i < len)
					qot = false;
				arglen = i - start;
				i++;
			}
			else if (sqot)
			{
				i++;
				while (i < len && command[i] != '\'')
					i++;
				if (i < len)
					sqot = false;
				arglen = i - start;
				i++;
			}
			else
			{
				while (i < len && delimiters.find(command[i]) == string::npos)//command[i] != ' ' && command[i] != '-')
					i++;

				arglen = i - start;
			}

			push_back(command.substr(start, arglen));
		}

		//if (qot || sqot) std::cout << "One of the quotes is open\n";
		ASSERT(!qot && !sqot);

		return *this;
	}

	std::string StringVector::to_string(const char* sep)const
	{
		std::string str;
		for (const_iterator it = begin(); it != end(); it++)
		{
			if (it != begin())
				str += sep;

			str += ToString(*it);
		}

		return str;
	}


}//namespace WBSF

