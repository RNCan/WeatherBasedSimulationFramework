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

#include "UtilWWW.h"
#include "SYShowMessage.h"
#include "UtilWin.h"

#include "WeatherBasedSimulationString.h"


using namespace std;
using namespace WBSF;

namespace UtilWWW
{
	static const long MAX_READ_SIZE=2048;

	CString GetServerName( const CString& path )
	{
		CString serverName(path);
		serverName.MakeLower();
		serverName.Replace('\\', '/');
		serverName.Replace(_T("ftp://"), _T(""));


		int pos = serverName.FindOneOf(_T("\\/"));
		if( pos >= 0)
			serverName = serverName.Left(pos);
		

		return serverName;
	}

	CString GetServerPath( const CString& path )
	{
		CString serverPath(path);
		serverPath.MakeLower();
		serverPath.Replace('\\', '/');
		serverPath.Replace(_T("ftp://"), _T(""));

		int pos1 = serverPath.FindOneOf(_T("\\/"));
		if( pos1 >= 0)
		{
			serverPath = serverPath.Mid(pos1);

			if( !UtilWin::IsEndOk(path) )
				serverPath += '/';
		}
		else 
		{
			serverPath.Empty();
		}
		
		return serverPath;
	}

	ERMsg GetPageText(CHttpConnectionPtr& pConnection, const std::string& URLIn, std::string& text, bool bConvert, DWORD flags)
	{
		
		ERMsg msg;

		//std::string text;
		//INTERNET_FLAG_NO_COOKIES|
		CString URL = WBSF::convert(URLIn).c_str();
		CHttpFile* pURLFile = pConnection->OpenRequest(_T("GET"), URL, NULL, 1, NULL, NULL, flags);

		bool bRep = false;

		if( pURLFile!=NULL )
		{
			int nbTry=0;
			while( !bRep && msg)
			{
				TRY
				{
					nbTry++;
					bRep = pURLFile->SendRequest()!=0;
				}
				CATCH_ALL(e)
				{
					DWORD errnum = GetLastError();
					if( errnum ==12002 ||errnum==12029)
					{
						if( nbTry >= 10)
						{
							msg = UtilWin::SYGetMessage(*e);
						}
						//try again
					}
					else if(  errnum == 12031 || errnum == 12111) 
					{
						//throw a exception: server reset
						THROW( new CInternetException(errnum));
					}
					else if( errnum ==12003)
					{
						msg = UtilWin::SYGetMessage(*e);

						DWORD size = 255;
						TCHAR cause[256]={0};
						InternetGetLastResponseInfo( &errnum, cause, &size);
						if (_tcslen(cause) > 0)
							msg.ajoute(UtilWin::ToUTF8(cause));
					}
					else
					{
						CInternetException e(errnum);
						msg += UtilWin::SYGetMessage(e);
					}
				}
				END_CATCH_ALL
			}
		}


		if( bRep )
		{
			const short MAX_READ_SIZE = 4096;
			pURLFile->SetReadBufferSize(MAX_READ_SIZE);

			std::string tmp;
			tmp.resize(MAX_READ_SIZE);
			UINT charRead=0;
			while ((charRead = pURLFile->Read(&(tmp[0]), MAX_READ_SIZE))>0)
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

			pURLFile->Close();
		}
		else
		{
			CString tmp;
			tmp.FormatMessage( IDS_CMN_UNABLE_LOAD_PAGE, URL);
			msg.ajoute(UtilWin::ToUTF8(tmp));
		}

		delete pURLFile;
		return msg;
	}

	ERMsg CopyFile(CHttpConnectionPtr& pConnection, const CString& URL, const CString& outputFilePath, DWORD flags, LPCTSTR userName, LPCTSTR password, WBSF::CCallback& callback )
	{
		ASSERT(	pConnection.get() != NULL);

		ERMsg msg;

		CHttpFile* pURLFile = pConnection->OpenRequest(_T("GET"), URL, NULL, 1, NULL, NULL, flags );
		
		if( pURLFile != NULL && pURLFile->SendRequest() )
		{
			UtilWin::CStdioFileEx file;

			BOOL bBinary = flags & INTERNET_FLAG_TRANSFER_BINARY;

			UINT type = bBinary ? CFile::modeWrite | CFile::modeCreate | CFile::typeBinary : CFile::modeWrite | CFile::modeCreate;
			msg = file.Open(outputFilePath, type);
			if (msg)
			{
				const short MAX_READ_SIZE = 4096;
				pURLFile->SetReadBufferSize(MAX_READ_SIZE);
				 
				bool bEmptyFile = true;
				string source;
				std::string tmp;
				tmp.resize(MAX_READ_SIZE);
				UINT charRead = 0;
				while (((charRead = pURLFile->Read(&(tmp[0]), MAX_READ_SIZE))>0) && msg)
				{
					//source.append(tmp.c_str(), charRead);
					
					if (charRead > 0)
					{
						file.Write(tmp.c_str(), charRead);
						bEmptyFile = false;
					}
						
						//file.Write(source.data(), (UINT)source.size());

					msg += callback.StepIt(0);
				}
			
				file.Close();

				if (!msg || bEmptyFile)
					CFile::Remove(outputFilePath);

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

		return msg;
	}

	
	
	bool IsFileUpToDate(CFtpConnectionPtr& pConnection, const CString& ftpFilePath, const CString& localFilePath, bool bLookFileSize, bool bLookFileTime)
	{
		bool bRep = false;

		CFileInfo ftpFileInfo;
		if( GetFileInfo(pConnection, ftpFilePath, ftpFileInfo) )
			bRep = IsFileUpToDate(ftpFileInfo, localFilePath, bLookFileSize, bLookFileTime);

		return bRep;
	}
	
	bool IsFileUpToDate(const CFileInfo& ftpFile, const CString& localFilePath, bool bLookFileSize, bool bLookFileTime)
	{
		bool bRep = false;

		CFileStatus s;
		if( CFile::GetStatus(localFilePath, s) )
		{
			bRep = true;
			if( bLookFileSize && s.m_size!=ftpFile.m_size)
				bRep = false;
			if( bLookFileTime && s.m_mtime < ftpFile.m_time)
				bRep = false;
				
		}

		return bRep;
	}

	ERMsg CopyFile(CFtpConnectionPtr& pConnection, const CString& InputFilePath, const CString& outputFilePath, DWORD flags, BOOL bFailIfExists )
	{
		ASSERT(	pConnection.get() != NULL);
	
		//DWORD flag = INTERNET_FLAG_TRANSFER_BINARY|INTERNET_FLAG_RELOAD|INTERNET_FLAG_EXISTING_CONNECT|INTERNET_FLAG_DONT_CACHE;
		//INTERNET_FLAG_RELOAD;
		ERMsg msg;
		BOOL bRep = pConnection->GetFile(InputFilePath, outputFilePath, bFailIfExists, FILE_ATTRIBUTE_NORMAL, flags);

		if( !bRep )
		{
			DWORD errnum = GetLastError();
			CInternetException e(errnum);
			msg += UtilWin::SYGetMessage(e);
			
			//DWORD size = 255;
			//TCHAR cause[256]={0};
			//InternetGetLastResponseInfo( &errnum, cause, &size);
			
			//THROW( new CInternetException(errnum));

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
		ASSERT(	pConnection.get() != NULL);
		ERMsg msg;

		BOOL bRep = pConnection->PutFile(InputFilePath, outputFilePath);

		if( !bRep )
		{
			DWORD errnum = GetLastError();
			CInternetException e(errnum);
			msg += UtilWin::SYGetMessage(e);
		}

		return msg;
	}

	void ConvertString2FindFileInfo(CFtpConnectionPtr& pConnect, const CStringArray& fileListIn, CFileInfoVector& fileListOut, WBSF::CCallback& callback)
	{
		for(int i=0; i<fileListIn.GetSize(); i++)
		{
			CFileInfoVector fileList;
			FindFiles(pConnect, fileListIn[i], fileList, callback);
			ASSERT( fileList.size() == 1);
			fileListOut.insert(fileListOut.begin(), fileList.begin(), fileList.end());
		}
	}

	bool GetFileInfo(CFtpConnectionPtr& pConnect, const CString& filePath, CFileInfo& info)
	{
		bool bRep = false;
		CFtpFileFind finder(pConnect.get());

		// start looping
		if( finder.FindFile(filePath) )
		{
			finder.FindNextFile();

			ASSERT( !finder.IsDots() && !finder.IsDirectory() );
			CString test = finder.GetFileURL();
			
			GetFileInfo(finder, info, filePath.FindOneOf(_T("*?")) != -1 && !UtilWin::GetPath(filePath).IsEmpty() ); 
			bRep = true;
		}

		return bRep;
	}

	void GetFileInfo(const CFtpFileFind& finder, CFileInfo& info, bool bHaveWildcard)
	{
		ASSERT( !finder.GetFileURL().IsEmpty() );

		info.m_size = finder.GetLength();

		//CString test1 = finder.GetFilePath();
		//CString test2 = finder.GetFileName();
		if (bHaveWildcard )
			info.m_filePath = UtilWin::ToUTF8(finder.GetFilePath());
		else 
			info.m_filePath = UtilWin::ToUTF8(finder.GetFileName());


		info.m_attribute = 0;
			
			
		if( finder.IsReadOnly() )
			info.m_attribute |= FILE_ATTRIBUTE_READONLY;
		if( finder.IsDirectory())
			info.m_attribute |= FILE_ATTRIBUTE_DIRECTORY;
		if( finder.IsCompressed())
			info.m_attribute |= FILE_ATTRIBUTE_COMPRESSED;
		if( finder.IsSystem())
			info.m_attribute |= FILE_ATTRIBUTE_SYSTEM;
		if( finder.IsHidden())
			info.m_attribute |= FILE_ATTRIBUTE_HIDDEN;
		if( finder.IsTemporary())
			info.m_attribute |= FILE_ATTRIBUTE_TEMPORARY;
		if( finder.IsNormal())
			info.m_attribute |= FILE_ATTRIBUTE_NORMAL;
		if( finder.IsArchived())
			info.m_attribute |= FILE_ATTRIBUTE_ARCHIVE;
			
		TRY
		{
			CTime time;
			finder.GetLastWriteTime(time);
			info.m_time = time.GetTime();
		}
		CATCH_ALL(e)
		{}
		END_CATCH_ALL

	}

	ERMsg FindFiles(CFtpConnectionPtr& pConnect, const CString& URL, CFileInfoVector& fileList, WBSF::CCallback& callback)
	{
		ERMsg msg;

		callback.PushTask(WBSF::FormatMsg(IDS_MSG_SEARCH_FILE, std::string(CStringA(URL))), NOT_INIT);
		

		// use a file find object to enumerate files
		CFtpFileFind finder(pConnect.get());

		// start looping
		BOOL bWorking = finder.FindFile(URL, INTERNET_FLAG_EXISTING_CONNECT);//, INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_RELOAD
		DWORD errNum = GetLastError();

		while (bWorking&&msg)
		{
			bWorking = finder.FindNextFile();
			errNum = GetLastError();

			if( finder.IsDots() || finder.IsDirectory() )
				continue;

			CString test = finder.GetFileURL();

			CFileInfo info;
			
			GetFileInfo(finder, info, URL.FindOneOf(_T("*?")) != -1 && !UtilWin::GetPath(URL).IsEmpty());
			fileList.push_back(info);

			msg += callback.StepIt(0);
		}
		
		if( errNum != ERROR_NO_MORE_FILES)
		{
			CInternetException e(errNum);
			msg = UtilWin::SYGetMessage(e);
		}
		
		callback.PopTask();

		return msg;
	}

	static __time64_t GetTime(string date, string time)
	{
		CTime Time = CTime::GetCurrentTime();

		string::size_type pos=0;
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
					Time = CTime(WBSF::ToInt(year), int(WBSF::GetMonthIndex(month.c_str()) + 1), WBSF::ToInt(day), WBSF::ToInt(hour), WBSF::ToInt(min), 0);
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

			if( finder.IsDots() || !finder.IsDirectory() )
				continue;
	        
			CString path = finder.GetFilePath() + '/';
			
			CFileInfo info;
			memset( &info, 0, sizeof(info) );
			info.m_filePath = UtilWin::ToUTF8(path);
			//_tcsncpy_s(info.m_filePath, _MAX_PATH, (LPCTSTR)path, _MAX_PATH);
			info.m_size = finder.GetLength();
			
			
			
			if( finder.IsReadOnly() )
				info.m_attribute |= FILE_ATTRIBUTE_READONLY;
			if( finder.IsDirectory())
				info.m_attribute |= FILE_ATTRIBUTE_DIRECTORY;
			if( finder.IsCompressed())
				info.m_attribute |= FILE_ATTRIBUTE_COMPRESSED;
			if( finder.IsSystem())
				info.m_attribute |= FILE_ATTRIBUTE_SYSTEM;
			if( finder.IsHidden())
				info.m_attribute |= FILE_ATTRIBUTE_HIDDEN;
			if( finder.IsTemporary())
				info.m_attribute |= FILE_ATTRIBUTE_TEMPORARY;
			if( finder.IsNormal())
				info.m_attribute |= FILE_ATTRIBUTE_NORMAL;
			if( finder.IsArchived())
				info.m_attribute |= FILE_ATTRIBUTE_ARCHIVE;
			
			fileList.push_back(info);
		}

		
		//fileList.IsEmpty() && 
		if( errNum !=  ERROR_NO_MORE_FILES)
		{
			CInternetException e(errNum);
			msg = UtilWin::SYGetMessage(e);
		}
		
		return msg;
	}

	
	ERMsg FindFiles(CHttpConnectionPtr& pConnect, const CString& URL, CFileInfoVector& fileList)
	{
		ERMsg msg;
		std::string path = WBSF::GetPath(WBSF::UTF8((LPCTSTR)URL));
		std::string filterName = WBSF::GetFileName(WBSF::UTF8((LPCTSTR)URL));

		std::string source;
		msg = GetPageText(pConnect, path, source );
		if( msg )
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
							std::remove(size.begin(), size.end(), 'M');  
							std::remove(size.begin(), size.end(), 'K');
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

	ERMsg FindDirectories(CHttpConnectionPtr& pConnect, const CString& _URL, CFileInfoVector& fileList)
	{

		ERMsg msg;

		string URL = WBSF::UTF8((LPCTSTR)_URL);
		//CString path = UtilWin::GetPath(URL);
		//CString filterName = UtilWin::GetFileName(URL);
		

		std::string source;
		msg = GetPageText(pConnect, URL, source);
		if( msg )
		{
			std::string::size_type posBegin = source.find("<a href=");
			while (posBegin != std::string::npos)
			{
				string fileName = FindString(source, "<a href=\"", "\">", posBegin);

				if (WBSF::Match("*/", fileName.c_str() ))
				{
					if (fileName != "./" && fileName != "../" &&
						fileName != ".\\" && fileName != "..\\")
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
			}
		}

		return msg;
	}

	ERMsg GetHttpConnection(const CString& serverName, CHttpConnectionPtr& pConnection, CInternetSessionPtr& pSession, DWORD flags, const CString& userName, const CString& password)
	{
		ERMsg msg;

		//DWORD flags = INTERNET_FLAG_RELOAD|INTERNET_FLAG_DONT_CACHE|INTERNET_FLAG_KEEP_CONNECTION;
		pSession.reset( new CInternetSession );
		pConnection.reset( pSession->GetHttpConnection(serverName, flags, 0, userName, password ) );
		if (pConnection.get())
		{
			pSession->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 60000);
		}
		else
		{
			msg.asgType( ERMsg::ERREUR);
			CInternetException e(GetLastError() );
			msg = UtilWin::SYGetMessage(e);
		}

		return msg;
	}

	ERMsg GetFtpConnection(const CString& serverName, CFtpConnectionPtr& pConnection, CInternetSessionPtr& pSession, DWORD flags, LPCTSTR userName, LPCTSTR password, BOOL bPassif )
	{
		ERMsg msg;
		
		pConnection.reset();
		pSession.reset();
		
	
		pSession.reset( new CInternetSession(NULL, 1, flags) );

		TRY
		{
			pSession->SetOption(INTERNET_OPTION_RECEIVE_TIMEOUT, 120000);
			//pSession->SetOption(INTERNET_OPTION_CONNECT_TIMEOUT, 10000);
			pSession->SetOption(INTERNET_OPTION_DATA_RECEIVE_TIMEOUT, 120000);
			pSession->SetOption(INTERNET_OPTION_KEEP_CONNECTION, INTERNET_KEEP_ALIVE_ENABLED);
			pSession->SetOption(INTERNET_OPTION_RESET_URLCACHE_SESSION, 0);
			pSession->SetOption(INTERNET_OPTION_SETTINGS_CHANGED, 0);
			pSession->SetOption(INTERNET_OPTION_REFRESH, 0);

			pConnection.reset( pSession->GetFtpConnection(serverName, userName, password, INTERNET_DEFAULT_FTP_PORT, bPassif) );
				
		}
		CATCH_ALL(e)
		{
			pSession->Close();
			pSession.reset();

			CString error;
			e->GetErrorMessage(error.GetBufferSetLength(255), 255);
			msg.ajoute(CStringA(error));
		}
		END_CATCH_ALL
		
		return msg;
	}



	ERMsg GetHttpConnection(const std::string& serverName, CHttpConnectionPtr& pConnection, CInternetSessionPtr& pSession, DWORD flags, const std::string& userName, const std::string& password)
	{
		return GetHttpConnection(UtilWin::Convert(serverName), pConnection, pSession, flags, UtilWin::Convert(userName), UtilWin::Convert(password));
	}
	
	ERMsg GetFtpConnection(const std::string& serverName, CFtpConnectionPtr& pConnection, CInternetSessionPtr& pSession, DWORD flags, const std::string& userName, const std::string& password, BOOL bPassif)
	{
		return GetFtpConnection(UtilWin::Convert(serverName), pConnection, pSession, flags, UtilWin::Convert(userName), UtilWin::Convert(password), bPassif);
	}
	ERMsg CopyFile(CHttpConnectionPtr& pConnection, const std::string& URL, const std::string& outputFilePath, DWORD flags, const std::string& userName, const std::string& password, WBSF::CCallback& callback)
	{
		return CopyFile(pConnection, UtilWin::Convert(URL), UtilWin::Convert(outputFilePath), flags, UtilWin::Convert(userName), UtilWin::Convert(password), callback);
	}
	ERMsg CopyFile(CFtpConnectionPtr& pConnection, const std::string& URL, const std::string& outputFilePath, DWORD flags, BOOL bFailIfExists)
	{
		return CopyFile(pConnection, UtilWin::Convert(URL), UtilWin::Convert(outputFilePath), flags);
	}
	ERMsg FindFiles(CHttpConnectionPtr& pConnect, const std::string& URL, CFileInfoVector& fileList)
	{
		return FindFiles(pConnect, UtilWin::Convert(URL), fileList);
	}
	ERMsg FindFiles(CFtpConnectionPtr& pConnect, const std::string& URL, CFileInfoVector& fileList, WBSF::CCallback& callback)
	{
		return FindFiles(pConnect, UtilWin::Convert(URL), fileList, callback);
	}

	ERMsg FindDirectories(CHttpConnectionPtr& pConnect, const std::string& URL, CFileInfoVector& fileList)
	{
		return FindDirectories(pConnect, UtilWin::Convert(URL), fileList);
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
	


}
