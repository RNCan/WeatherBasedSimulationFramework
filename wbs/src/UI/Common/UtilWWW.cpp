//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//****************************************************************************
#include "StdAfx.h"
#include <iostream>

#include "Basic/UtilStd.h"
#include "Basic/Registry.h"
#include "Basic/UtilTime.h"
#include "Basic/decode_html_entities_utf8.h"
#include "Basic/CallcURL.h"

#include "UtilWWW.h"
#include "SYShowMessage.h"
#include "UtilWin.h"

#include "WeatherBasedSimulationString.h"


using namespace std;
using namespace WBSF;

namespace UtilWWW
{
	static const long MAX_READ_SIZE = 2048;

	CString GetServerName(const CString& path)
	{
		CString serverName(path);
		serverName.MakeLower();
		serverName.Replace('\\', '/');
		serverName.Replace(_T("ftp://"), _T(""));


		int pos = serverName.FindOneOf(_T("\\/"));
		if (pos >= 0)
			serverName = serverName.Left(pos);


		return serverName;
	}

	CString GetServerPath(const CString& path)
	{
		CString serverPath(path);
		serverPath.MakeLower();
		serverPath.Replace('\\', '/');
		serverPath.Replace(_T("ftp://"), _T(""));

		int pos1 = serverPath.FindOneOf(_T("\\/"));
		if (pos1 >= 0)
		{
			serverPath = serverPath.Mid(pos1);

			if (!UtilWin::IsEndOk(path))
				serverPath += '/';
		}
		else
		{
			serverPath.Empty();
		}

		return serverPath;
	}

	ERMsg GetPageTextCurl(const std::string& URL, std::string& source)
	{
		CCallcURL cURL;
		return cURL.get_text(URL, source);
	}

	ERMsg GetPageText(CHttpConnectionPtr& pConnection, const std::string& URLIn, std::string& text, bool bConvert, DWORD flags)
	{

		ERMsg msg;

		//std::string text;
		//INTERNET_FLAG_NO_COOKIES|
		CString URL = WBSF::convert(URLIn).c_str();
		//CHttpFile* pURLFile = pConnection->OpenRequest(_T("GET"), URL, NULL, 1, NULL, NULL, flags);
		CHttpFile* pURLFile = pConnection->OpenRequest(CHttpConnection::HTTP_VERB_GET, URL, NULL, 1, NULL, NULL, flags);


		//bool bRep = false;

		if (pURLFile != NULL)
		{
			//			DWORD dwStatus;

			////int nbTry = 0;
			////while (!bRep && msg)
			//if(!bRep && msg)
			//{
				////try
				////{
					////nbTry++;
			bool bRep = pURLFile->SendRequest() != 0;
			//}
			//catch (CException* e)
			//{
			//	DWORD errnum = GetLastError();
			//	if (errnum == 12002 || errnum == 12029)
			//	{
			//		if (nbTry >= 2)
			//		{
			//			msg = UtilWin::SYGetMessage(*e);
			//		}
			//		//try again
			//	}
			//	else if (errnum == 12031 || errnum == 12111)
			//	{
			//		//throw a exception: server reset
			//		THROW(new CInternetException(errnum));
			//	}
			//	else if (errnum == 12003)
			//	{
			//		msg = UtilWin::SYGetMessage(*e);

			//		DWORD size = 255;
			//		TCHAR cause[256] = { 0 };
			//		InternetGetLastResponseInfo(&errnum, cause, &size);
			//		if (_tcslen(cause) > 0)
			//			msg.ajoute(UtilWin::ToUTF8(cause));
			//	}
			//	else
			//	{
			//		CInternetException e(errnum);
			//		msg += UtilWin::SYGetMessage(e);
			//	}
			//}

		//}
	//}


			if (bRep)
			{
				const short MAX_READ_SIZE = 4096;
				pURLFile->SetReadBufferSize(MAX_READ_SIZE);

				std::string tmp;
				tmp.resize(MAX_READ_SIZE);
				UINT charRead = 0;
				while ((charRead = pURLFile->Read(&(tmp[0]), MAX_READ_SIZE)) > 0)
				{
					//tmp.ReleaseBuffer();
					text.append(tmp.c_str(), charRead);
				}


				//convert UTF8 to UTF16
				//size_t test1 = text.size();
				//size_t test2 = text.length();
				//int len = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, NULL, 0);

				//std::wstring out;
				//out.resize(len);
				////WCHAR *ptr = textOut.GetBufferSetLength(len); ASSERT(ptr);
				//
				//	MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, &(out[0]), len);
				//
				////Convert UTF16 to ANSI
				//int newLen = WideCharToMultiByte(CP_THREAD_ACP, 0, out.c_str(), -1, NULL, 0, 0, 0);
				//text.resize(newLen);
				//WideCharwToMultiByte(CP_THREAD_ACP, 0, out.c_str(), -1, NULL, 0, 0, 0);

				//Convert UTF16 to ANSI
				//text = WBSF::UTF8(out);

				//decode HTML code
				if (bConvert)
				{
					decode_html_entities_utf8(&(text[0]), NULL);

					//Convert UTF8 to UTF16 
					int len = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, NULL, 0);

					std::wstring w_text;
					w_text.resize(len);
					MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, &(w_text[0]), len);

					//Convert UTF16 to Windows-1252 encoding
					int newLen = WideCharToMultiByte(CP_ACP, 0, w_text.c_str(), -1, NULL, 0, 0, 0);

					text.resize(newLen);
					WideCharToMultiByte(CP_ACP, 0, w_text.c_str(), -1, &(text[0]), newLen, 0, 0);
					text.resize(strlen(text.c_str()));
				}

				//textOut.ReleaseBuffer();

				//text.Replace(_T("&nbsp;"), _T(""));
				////text.Remove('\r');
				////text.Remove('\n');
				//if( replaceAccent )
				//{

				//	//lowercase
				//	text.Replace(_T("&egrave;"), _T("è"));
				//	text.Replace(_T("&eacute;"), _T("é"));
				//	text.Replace(_T("&ocirc;"), _T("ô"));
				//	text.Replace(_T("&icirc;"), _T("î"));
				//	text.Replace(_T("&ecirc;"), _T("ê"));
				//	text.Replace(_T("&euml;"), _T("ë"));
				//	text.Replace(_T("&ccedil;"), _T("ç"));
				//	

				//	//uppercase 
				//	text.Replace(_T("&Egrave;"), _T("È"));
				//	text.Replace(_T("&Eacute;"), _T("É"));
				//	text.Replace(_T("&Ocirc;"), _T("Ô"));
				//	text.Replace(_T("&Icirc;"), _T("Î"));
				//	text.Replace(_T("&Ecirc;"), _T("Ê"));
				//	text.Replace(_T("&Euml;"), _T("Ë"));
				//	text.Replace(_T("&Ccedil;"), _T("Ç"));
				//}
			}
			pURLFile->Close();
			delete pURLFile;
		}
		else
		{
			CString tmp;
			tmp.FormatMessage(IDS_CMN_UNABLE_LOAD_PAGE, URL);
			msg.ajoute(UtilWin::ToUTF8(tmp));
		}


		return msg;
	}
	
	ERMsg CopyFileCurl(const std::string& URL, std::string& outputFilePath, bool bShowCurl)
	{
		CCallcURL cURL;
		return cURL.copy_file(URL, outputFilePath, bShowCurl);
	}

	ERMsg CopyFile(CHttpConnectionPtr& pConnection, const CString& URL, const CString& outputFilePath, DWORD flags, BOOL bThrow, WBSF::CCallback& callback)
	{
		ASSERT(pConnection.get() != NULL);

		ERMsg msg;

		try
		{

			CHttpFile* pURLFile = pConnection->OpenRequest(_T("GET"), URL, NULL, 1, NULL, NULL, flags);

			if (pURLFile != NULL && pURLFile->SendRequest())
			{
				UtilWin::CStdioFileEx file;

				BOOL bBinary = flags & INTERNET_FLAG_TRANSFER_BINARY;

				UINT type = bBinary ? CFile::modeWrite | CFile::modeCreate | CFile::typeBinary : CFile::modeWrite | CFile::modeCreate;
				msg = file.Open(outputFilePath, type);
				if (msg)
				{
					const short MAX_READ_SIZE = 4096;
					pURLFile->SetReadBufferSize(MAX_READ_SIZE);

					bool bFirst = true;
					bool bEmptyFile = true;
					string source;
					std::string tmp;
					tmp.resize(MAX_READ_SIZE);
					UINT charRead = 0;
					while (((charRead = pURLFile->Read(&(tmp[0]), MAX_READ_SIZE)) > 0) && msg)
					{
						if (bFirst)
						{
							CString str;
							pURLFile->QueryInfo(HTTP_QUERY_CONTENT_LENGTH, str);
							int LengthFile = ::atoi((LPCSTR)CStringA(str));
							callback.PushTask((LPCSTR)CStringA(URL), (double)LengthFile, MAX_READ_SIZE);
							bFirst = false;
						}

						if (charRead > 0)
						{
							file.Write(tmp.c_str(), charRead);
							bEmptyFile = false;
						}

						msg += callback.StepIt();
					}

					file.Close();

					if (!msg || bEmptyFile)
						CFile::Remove(outputFilePath);

					callback.PopTask();
				}



				pURLFile->Close();
			}
			else
			{
				std::string errorURL = CStringA(URL);
				std::string error = FormatMsg(IDS_CMN_UNABLE_LOAD_PAGE, errorURL);
				msg.ajoute(error);
			}

			delete pURLFile;
		}
		catch (CException* e)
		{
			if (bThrow)
			{
				throw;
				//	THROW(e)
			}
			else
			{
				msg = UtilWin::SYGetMessage(*e);
			}
		}


		return msg;
	}



	bool IsFileUpToDate(CFtpConnectionPtr& pConnection, const CString& ftpFilePath, const CString& localFilePath, bool bLookFileSize, bool bLookFileTime)
	{
		bool bRep = false;

		CFileInfo ftpFileInfo;
		if (GetFileInfo(pConnection, ftpFilePath, ftpFileInfo))
			bRep = IsFileUpToDate(ftpFileInfo, localFilePath, bLookFileSize, bLookFileTime);

		return bRep;
	}

	bool IsFileUpToDate(const CFileInfo& ftpFile, const CString& localFilePath, bool bLookFileSize, bool bLookFileTime)
	{
		bool bRep = false;

		CFileStatus s;
		if (CFile::GetStatus(localFilePath, s))
		{
			bRep = true;
			if (bLookFileSize && s.m_size != ftpFile.m_size)
				bRep = false;
			if (bLookFileTime && s.m_mtime < ftpFile.m_time)
				bRep = false;

		}

		return bRep;
	}

	ERMsg CopyFile(CFtpConnectionPtr& pConnection, const CString& InputFilePath, const CString& outputFilePath, DWORD flags, BOOL bThrow)
	{
		ASSERT(pConnection.get() != NULL);

		//DWORD flag = INTERNET_FLAG_TRANSFER_BINARY|INTERNET_FLAG_RELOAD|INTERNET_FLAG_EXISTING_CONNECT|INTERNET_FLAG_DONT_CACHE;
		//INTERNET_FLAG_RELOAD;
		ERMsg msg;
		BOOL bRep = pConnection->GetFile(InputFilePath, outputFilePath, FALSE, FILE_ATTRIBUTE_NORMAL, flags);

		if (!bRep)
		{
			DWORD errnum = GetLastError();

			//DWORD size = 255;
			//TCHAR cause[256]={0};
			//InternetGetLastResponseInfo( &errnum, cause, &size);
			if (bThrow)
			{
				THROW(new CInternetException(errnum));
			}
			else
			{
				CInternetException e(errnum);
				msg += UtilWin::SYGetMessage(e);
			}

			//msg.asgType( ERMsg::ERREUR);
			//CString error;
			//error.FormatMessage( IDS_CMN_UNABLE_COPY_FILE, InputFilePath);
			//msg.ajoute(error);
			//msg.ajoute("Unable to copy file: " + InputFilePath);




			/*if( errnum == 12031 || errnum == 12111)
			{
				//throw a exception: server reset
				THROW( new CInternetException(errnum));
			}
			else if( errnum ==12003)
			{
				DWORD size = 255;
				TCHAR cause[256]={0};
				InternetGetLastResponseInfo( &errnum, cause, &size);
				if( strlen(cause) > 0 )
					msg.ajoute(cause);
			}
			else
			{
				CInternetException e(errnum);
				msg += UtilWin::SYGetMessage(e);
			}
			*/
		}

		return msg;
	}

	ERMsg CopyFileToFTP(CFtpConnectionPtr& pConnection, const CString& InputFilePath, const CString& outputFilePath)
	{
		ASSERT(pConnection.get() != NULL);
		ERMsg msg;

		BOOL bRep = pConnection->PutFile(InputFilePath, outputFilePath);

		if (!bRep)
		{
			DWORD errnum = GetLastError();
			CInternetException e(errnum);
			msg += UtilWin::SYGetMessage(e);
		}

		return msg;
	}

	void ConvertString2FindFileInfo(CFtpConnectionPtr& pConnect, const CStringArray& fileListIn, CFileInfoVector& fileListOut, BOOL bThrow, WBSF::CCallback& callback)
	{
		for (int i = 0; i < fileListIn.GetSize(); i++)
		{
			CFileInfoVector fileList;
			FindFiles(pConnect, fileListIn[i], fileList, bThrow, callback);
			ASSERT(fileList.size() == 1);
			fileListOut.insert(fileListOut.begin(), fileList.begin(), fileList.end());
		}
	}

	bool GetFileInfo(CFtpConnectionPtr& pConnect, const CString& filePath, CFileInfo& info)
	{
		bool bRep = false;
		CFtpFileFind finder(pConnect.get());

		// start looping
		if (finder.FindFile(filePath))
		{
			finder.FindNextFile();

			ASSERT(!finder.IsDots() && !finder.IsDirectory());
			CString test = finder.GetFileURL();

			GetFileInfo(finder, info, filePath.FindOneOf(_T("*?")) != -1 && !UtilWin::GetPath(filePath).IsEmpty());
			bRep = true;
		}

		return bRep;
	}

	void GetFileInfo(const CFtpFileFind& finder, CFileInfo& info, bool bHavePath)
	{
		ASSERT(!finder.GetFileURL().IsEmpty());

		info.m_size = finder.GetLength();

		CString test1 = finder.GetFilePath();
		CString test2 = finder.GetFileName();
		if (bHavePath)
		{
			if (!UtilWin::GetPath(test2).IsEmpty())
				info.m_filePath = UtilWin::ToUTF8(test2);
			else if (!UtilWin::GetPath(test1).IsEmpty())
				info.m_filePath = UtilWin::ToUTF8(test1);
			else
				info.m_filePath = UtilWin::ToUTF8(test1);
		}
		else
		{
			info.m_filePath = UtilWin::ToUTF8(test2);
		}
		//if (bHaveWildcard )
			//info.m_filePath = UtilWin::ToUTF8(finder.GetFilePath());
		//else 
			//info.m_filePath = UtilWin::ToUTF8(finder.GetFileName());


		info.m_attribute = 0;


		if (finder.IsReadOnly())
			info.m_attribute |= FILE_ATTRIBUTE_READONLY;
		if (finder.IsDirectory())
			info.m_attribute |= FILE_ATTRIBUTE_DIRECTORY;
		if (finder.IsCompressed())
			info.m_attribute |= FILE_ATTRIBUTE_COMPRESSED;
		if (finder.IsSystem())
			info.m_attribute |= FILE_ATTRIBUTE_SYSTEM;
		if (finder.IsHidden())
			info.m_attribute |= FILE_ATTRIBUTE_HIDDEN;
		if (finder.IsTemporary())
			info.m_attribute |= FILE_ATTRIBUTE_TEMPORARY;
		if (finder.IsNormal())
			info.m_attribute |= FILE_ATTRIBUTE_NORMAL;
		if (finder.IsArchived())
			info.m_attribute |= FILE_ATTRIBUTE_ARCHIVE;

		try
		{
			CTime time;
			finder.GetLastWriteTime(time);
			info.m_time = time.GetTime();
		}
		catch (CException*)
		{
		}


	}

	ERMsg FindFiles(CFtpConnectionPtr& pConnect, const CString& URL, CFileInfoVector& fileList, BOOL bThrow, WBSF::CCallback& callback)
	{
		ERMsg msg;

		callback.PushTask(WBSF::FormatMsg(IDS_MSG_SEARCH_FILE, std::string(CStringA(URL))), NOT_INIT);


		// use a file find object to enumerate files
		CFtpFileFind finder(pConnect.get());

		// start looping
		BOOL bWorking = finder.FindFile(URL, INTERNET_FLAG_EXISTING_CONNECT);//, INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_RELOAD
		DWORD errNum = GetLastError();

		while (bWorking && msg)
		{
			bWorking = finder.FindNextFile();
			errNum = GetLastError();

			if (finder.IsDots() || finder.IsDirectory())
				continue;

			CString test = finder.GetFileURL();

			CFileInfo info;

			GetFileInfo(finder, info, !UtilWin::GetPath(URL).IsEmpty());//URL.FindOneOf(_T("*?")) != -1 && 
			fileList.push_back(info);

			msg += callback.StepIt(0);
		}

		if (errNum != ERROR_NO_MORE_FILES)
		{
			if (bThrow)
			{
				throw(new CInternetException(errNum));
			}
			else
			{
				CInternetException e(errNum);
				msg = UtilWin::SYGetMessage(e);
			}
		}

		callback.PopTask();

		return msg;
	}

	static __time64_t GetTime(string date, string time)
	{
		CTime Time = CTime::GetCurrentTime();

		string::size_type pos = 0;
		string day = WBSF::Tokenize(date, "-", pos);
		if (pos != string::npos)
		{
			string month = WBSF::Tokenize(date, "-", pos);
			if (pos != string::npos)
			{
				string year = WBSF::Tokenize(date, "-", pos);
				string hour = WBSF::Tokenize(time, ":", pos = 0);
				if (pos != string::npos)
				{
					string min = WBSF::Tokenize(time, ":", pos);
					int i_year = WBSF::ToInt(year);
					int i_month = int(WBSF::GetMonthIndex(month.c_str()) + 1);
					if (i_month == 0)
						i_month = WBSF::ToInt(month);
					int i_day = WBSF::ToInt(day);
					int i_hour = WBSF::ToInt(hour);
					int i_min = WBSF::ToInt(min);

					if (i_day > 1900 && i_day < 2100)//year are first and day at end
						std::swap(i_day, i_year);

					Time = CTime(i_year, i_month, i_day, i_hour, i_min, 0);
				}
			}
		}

		return Time.GetTime();

	}

	ERMsg FindDirectories(CFtpConnectionPtr& pConnect, const CString& page, CFileInfoVector& fileList)
	{
		ERMsg msg;

		CFtpFileFind finder(pConnect.get());

		// start looping
		BOOL bWorking = finder.FindFile(page);
		DWORD errNum = GetLastError();

		while (bWorking)
		{
			bWorking = finder.FindNextFile();
			errNum = GetLastError();

			if (finder.IsDots() || !finder.IsDirectory())
				continue;

			CString path = finder.GetFilePath() + '/';

			CFileInfo info;
			memset(&info, 0, sizeof(info));
			info.m_filePath = UtilWin::ToUTF8(path);
			//_tcsncpy_s(info.m_filePath, _MAX_PATH, (LPCTSTR)path, _MAX_PATH);
			info.m_size = finder.GetLength();



			if (finder.IsReadOnly())
				info.m_attribute |= FILE_ATTRIBUTE_READONLY;
			if (finder.IsDirectory())
				info.m_attribute |= FILE_ATTRIBUTE_DIRECTORY;
			if (finder.IsCompressed())
				info.m_attribute |= FILE_ATTRIBUTE_COMPRESSED;
			if (finder.IsSystem())
				info.m_attribute |= FILE_ATTRIBUTE_SYSTEM;
			if (finder.IsHidden())
				info.m_attribute |= FILE_ATTRIBUTE_HIDDEN;
			if (finder.IsTemporary())
				info.m_attribute |= FILE_ATTRIBUTE_TEMPORARY;
			if (finder.IsNormal())
				info.m_attribute |= FILE_ATTRIBUTE_NORMAL;
			if (finder.IsArchived())
				info.m_attribute |= FILE_ATTRIBUTE_ARCHIVE;

			fileList.push_back(info);
		}


		//fileList.IsEmpty() && 
		if (errNum != ERROR_NO_MORE_FILES)
		{
			CInternetException e(errNum);
			msg = UtilWin::SYGetMessage(e);
		}

		return msg;
	}

	ERMsg FindFilesCurl(const string& URL, CFileInfoVector& fileList)
	{
		ASSERT(URL.find("://") != string::npos);


		ERMsg msg;

		std::string path = WBSF::GetPath(URL);
		std::string filterName = WBSF::GetFileName(URL);


		string argument = "-s -k \"" + path;
		string exe = GetApplicationPath() + "External\\curl.exe";
		CCallcURL cURL(exe);

		string source;
		msg = cURL.get_text(argument, source);


		if (msg)
		{

			std::string::size_type posBegin = source.find("<a href=");
			while (posBegin != std::string::npos)
			{
				std::string fileName = FindString(source, "<a href=\"", "\">", posBegin);
				if (WBSF::Match(filterName.c_str(), fileName.c_str()))
				{

					std::string filePath = path + fileName;

					CFileInfo info;
					//memset( &info, 0, sizeof(info) );
					info.m_filePath = filePath;

					std::string str = FindString(source, "</a>", "\n", posBegin);
					WBSF::Trim(str);
					std::string::size_type pos = 0;
					std::string date = WBSF::Tokenize(str, " ", pos, true);
					if (pos != std::string::npos)
					{
						std::string time = WBSF::Tokenize(str, " ", pos, true);
						if (pos != std::string::npos)
						{
							std::string size = WBSF::Tokenize(str, " ", pos, true);
							
							size.erase(std::remove(size.begin(), size.end(), 'M'), size.end());
							size.erase(std::remove(size.begin(), size.end(), 'K'), size.end());
							info.m_time = GetTime(date, time);
							info.m_size = WBSF::ToInt(size);
						}
					}

					fileList.push_back(info);
				}

				posBegin = source.find("<a href=", posBegin);
			}
		}


		return msg;
	}


	ERMsg FindFiles(CHttpConnectionPtr& pConnect, const CString& URL, CFileInfoVector& fileList, BOOL bThrow)
	{
		ERMsg msg;
		std::string path = WBSF::GetPath(WBSF::UTF8((LPCTSTR)URL));
		std::string filterName = WBSF::GetFileName(WBSF::UTF8((LPCTSTR)URL));

		try
		{
			std::string source;
			msg = GetPageText(pConnect, path, source);
			if (msg)
			{

				std::string::size_type posBegin = source.find("<a href=");
				while (posBegin != std::string::npos)
				{
					std::string fileName = FindString(source, "<a href=\"", "\">", posBegin);
					if (WBSF::Match(filterName.c_str(), fileName.c_str()))
					{

						std::string filePath = path + fileName;

						CFileInfo info;
						//memset( &info, 0, sizeof(info) );
						info.m_filePath = filePath;

						std::string str = FindString(source, "</a>", "\n", posBegin);
						WBSF::Trim(str);
						std::string::size_type pos = 0;
						std::string date = WBSF::Tokenize(str, " ", pos, true);
						if (pos != std::string::npos)
						{
							std::string time = WBSF::Tokenize(str, " ", pos, true);
							if (pos != std::string::npos)
							{
								std::string size = WBSF::Tokenize(str, " ", pos, true);
								size.erase(std::remove(size.begin(), size.end(), 'M'), size.end());
								size.erase(std::remove(size.begin(), size.end(), 'K'), size.end());
								info.m_time = GetTime(date, time);
								info.m_size = WBSF::ToInt(size);
							}
						}

						fileList.push_back(info);
					}

					posBegin = source.find("<a href=", posBegin);
				}
			}
		}
		catch (CException* e)
		{
			if (bThrow)
			{
				throw;
				//	THROW(e)
			}
			else
			{
				msg = UtilWin::SYGetMessage(*e);
			}
		}


		return msg;
	}


	ERMsg FindDirectoriesCurl(const string& URL, CFileInfoVector& fileList, WBSF::CCallback& callback)
	{
		ASSERT(URL.find("://")!=string::npos);//URL begin with http:// or https://

		ERMsg msg;

		//string URL = WBSF::UTF8((LPCTSTR)_URL);
		//string URL = _URL;
		//if (!URL.empty() && URL.front() != '/')
			//URL.insert(URL.begin(), '/');


		string argument = "-s -k \"" + URL;
		string exe = GetApplicationPath() + "External\\curl.exe";
		CCallcURL cURL(exe);

		string source;
		msg = cURL.get_text(argument, source);

		if (msg)
		{
			std::string::size_type posBegin = source.find("<a href=");
			while (posBegin != std::string::npos && msg)
			{
				string fileName = FindString(source, "<a href=\"", "\">", posBegin);
				size_t count = std::count_if(fileName.begin(), fileName.end(), [](char c) {return c == '/'; });

				if (WBSF::Match("*/", fileName.c_str()) && count==1)
				{
					string relPath = WBSF::GetRelativePath(URL, fileName);
					//bool bParent = WBSF::Find(URL, relPath);


					if (fileName != "./" && fileName != "../" &&
						fileName != ".\\" && fileName != "..\\" &&
						relPath != "..\\")
					{
						std::string filePath = URL + fileName;

						CFileInfo info;
						info.m_filePath = UtilWin::ToUTF8(filePath);

						std::string str = FindString(source, "</a>", "\n", posBegin);
						string::size_type pos = 0;
						string date = WBSF::Tokenize(str, " ", pos);
						if (pos != string::npos)
						{
							string time = WBSF::Tokenize(str, " ", pos);
							info.m_time = GetTime(date, time);
						}

						fileList.push_back(info);
					}
				}

				posBegin = source.find("<a href=", posBegin);
				msg += callback.StepIt(0);
			}
		}

		return msg;
	}

	ERMsg FindDirectories(CHttpConnectionPtr& pConnect, const CString& _URL, CFileInfoVector& fileList, BOOL bThrow, WBSF::CCallback& callback)
	{
		ERMsg msg;



		string URL = WBSF::UTF8((LPCTSTR)_URL);
		if (!URL.empty() && URL.front() != '/')
			URL.insert(URL.begin(), '/');

		try
		{

			std::string source;
			msg = GetPageText(pConnect, URL, source);
			if (msg)
			{
				std::string::size_type posBegin = source.find("<a href=");
				while (posBegin != std::string::npos && msg)
				{
					string fileName = FindString(source, "<a href=\"", "\">", posBegin);

					if (WBSF::Match("*/", fileName.c_str()))
					{
						string relPath = WBSF::GetRelativePath(URL, fileName);


						if (fileName != "./" && fileName != "../" &&
							fileName != ".\\" && fileName != "..\\" &&
							relPath != "..\\")
						{
							std::string filePath = URL + fileName;

							CFileInfo info;
							info.m_filePath = UtilWin::ToUTF8(filePath);

							std::string str = FindString(source, "</a>", "\n", posBegin);
							string::size_type pos = 0;
							string date = WBSF::Tokenize(str, " ", pos);
							if (pos != string::npos)
							{
								string time = WBSF::Tokenize(str, " ", pos);
								info.m_time = GetTime(date, time);
							}

							fileList.push_back(info);
						}
					}

					posBegin = source.find("<a href=", posBegin);
					msg += callback.StepIt(0);
				}
			}
		}
		catch (CException* e)
		{
			if (bThrow)
			{
				throw;
			}
			else
			{
				msg = UtilWin::SYGetMessage(*e);
			}
		}

		return msg;
	}

	ERMsg GetHttpConnection(const CString& serverName, CHttpConnectionPtr& pConnection, CInternetSessionPtr& pSession, DWORD flags, const CString& userName, const CString& password, bool bHttps, size_t maxTry, WBSF::CCallback& callback)
	{
		ERMsg msg;

		pConnection.reset();
		pSession.reset();

		size_t nbTry = 0;
		while (!pConnection.get() && msg)
		{
			nbTry++;
			try
			{

				pSession.reset(new CInternetSession(NULL, 1, flags));
				INTERNET_PORT nPort = bHttps ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;
				pConnection.reset(pSession->GetHttpConnection(serverName, nPort, userName, password));
				ASSERT(pConnection.get());

				//if (!pConnection.get())
				//{
					//pSession->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 60000);
				//}
				//else
				//{
				//	//msg.asgType(ERMsg::ERREUR);
				//	pSession.reset();
				//	CInternetException e(GetLastError());
				//	msg = UtilWin::SYGetMessage(e);
				//}
			}
			catch (CException* e)
			{
				//pSession->Close();
				pSession.reset();
				pConnection.reset();

				//CString error;
				//e->GetErrorMessage(error.GetBufferSetLength(255), 255);
				//msg.ajoute(CStringA(error));
				if (nbTry < maxTry)
				{
					callback.AddMessage(UtilWin::SYGetMessage(*e));
					msg = WaitServer(5, callback);
				}
				else
				{
					msg = UtilWin::SYGetMessage(*e);
				}

			}
		}

		return msg;
	}

	ERMsg GetFtpConnection(const CString& serverName, CFtpConnectionPtr& pConnection, CInternetSessionPtr& pSession, DWORD flags, LPCTSTR userName, LPCTSTR password, BOOL bPassif, size_t maxTry, WBSF::CCallback& callback)
	{
		ERMsg msg;

		pConnection.reset();
		pSession.reset();
		size_t nbTry = 0;

		while (!pConnection.get() && msg)
		{
			nbTry++;

			try
			{
				pSession.reset(new CInternetSession(NULL, 1, flags));
				pSession->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 120000);
				pSession->SetOption(INTERNET_OPTION_CONNECT_TIMEOUT, 10000);
				pSession->SetOption(INTERNET_OPTION_DATA_RECEIVE_TIMEOUT, 120000);
				pSession->SetOption(INTERNET_OPTION_KEEP_CONNECTION, INTERNET_KEEP_ALIVE_ENABLED);
				pSession->SetOption(INTERNET_OPTION_RESET_URLCACHE_SESSION, 0);
				pSession->SetOption(INTERNET_OPTION_SETTINGS_CHANGED, 0);
				pSession->SetOption(INTERNET_OPTION_REFRESH, 0);

				pConnection.reset(pSession->GetFtpConnection(serverName, userName, password, INTERNET_DEFAULT_FTP_PORT, bPassif));
				ASSERT(pConnection.get());

			}
			catch (CException* e)
			{
				//pSession->Close();
				pSession.reset();
				pConnection.reset();

				//		CString error;
					//	e->GetErrorMessage(error.GetBufferSetLength(255), 255);
						//msg.ajoute(CStringA(error));
				if (nbTry < maxTry)
				{
					callback.AddMessage(UtilWin::SYGetMessage(*e));
					msg = WaitServer(5, callback);
				}
				else
				{
					msg = UtilWin::SYGetMessage(*e);
				}
			}
		}

		return msg;
	}



	ERMsg GetHttpConnection(const std::string& serverName, CHttpConnectionPtr& pConnection, CInternetSessionPtr& pSession, DWORD flags, const std::string& userName, const std::string& password, bool bHttps, size_t maxTry, WBSF::CCallback& callback)
	{
		return GetHttpConnection(UtilWin::Convert(serverName), pConnection, pSession, flags, UtilWin::Convert(userName), UtilWin::Convert(password), bHttps, maxTry, callback);
	}

	ERMsg GetFtpConnection(const std::string& serverName, CFtpConnectionPtr& pConnection, CInternetSessionPtr& pSession, DWORD flags, const std::string& userName, const std::string& password, BOOL bPassif, size_t maxTry, WBSF::CCallback& callback)
	{
		return GetFtpConnection(UtilWin::Convert(serverName), pConnection, pSession, flags, UtilWin::Convert(userName), UtilWin::Convert(password), bPassif, maxTry, callback);
	}
	ERMsg CopyFile(CHttpConnectionPtr& pConnection, const std::string& URL, const std::string& outputFilePath, DWORD flags, BOOL bThrow, WBSF::CCallback& callback)
	{
		return CopyFile(pConnection, UtilWin::Convert(URL), UtilWin::Convert(outputFilePath), flags, bThrow, callback);
	}
	


	ERMsg CopyFile(CFtpConnectionPtr& pConnection, const std::string& URL, const std::string& outputFilePath, DWORD flags, BOOL bThrow)
	{
		return CopyFile(pConnection, UtilWin::Convert(URL), UtilWin::Convert(outputFilePath), flags, bThrow);
	}
	ERMsg FindFiles(CHttpConnectionPtr& pConnect, const std::string& URL, CFileInfoVector& fileList, BOOL bThrow)
	{
		return FindFiles(pConnect, UtilWin::Convert(URL), fileList, bThrow);
	}

	ERMsg FindFiles(CFtpConnectionPtr& pConnect, const std::string& URL, CFileInfoVector& fileList, BOOL bThrow, WBSF::CCallback& callback)
	{
		return FindFiles(pConnect, UtilWin::Convert(URL), fileList, bThrow, callback);
	}

	ERMsg FindDirectories(CHttpConnectionPtr& pConnect, const std::string& URL, CFileInfoVector& fileList, BOOL bThrow, WBSF::CCallback& callback)
	{
		return FindDirectories(pConnect, UtilWin::Convert(URL), fileList, bThrow, callback);
	}

	ERMsg FindDirectories(CFtpConnectionPtr& pConnect, const std::string& URL, CFileInfoVector& fileList)
	{
		return FindDirectories(pConnect, UtilWin::Convert(URL), fileList);
	}

	bool IsFileUpToDate(const CFileInfo& info, const std::string& localFile, bool bLookFileSize, bool bLookFileTime)
	{
		return IsFileUpToDate(info, UtilWin::Convert(localFile), bLookFileSize, bLookFileTime);
	}
	bool IsFileUpToDate(CFtpConnectionPtr& pConnection, const std::string& URL, const std::string& localFilePath, bool bLookFileSize, bool bLookFileTime)
	{
		return IsFileUpToDate(pConnection, UtilWin::Convert(URL), UtilWin::Convert(localFilePath), bLookFileSize, bLookFileTime);
	}

	ERMsg WaitServer(size_t nbSec, WBSF::CCallback& callback)
	{
		ERMsg msg;
		callback.PushTask(FormatMsg(IDS_BSC_WAIT_30_SECONDS, std::to_string(nbSec)), nbSec * 20);
		for (size_t i = 0; i < nbSec * 20 && msg; i++)
		{
			Sleep(50);//wait 50 milisec
			msg += callback.StepIt();
		}
		callback.PopTask();
		return msg;
	}

}
