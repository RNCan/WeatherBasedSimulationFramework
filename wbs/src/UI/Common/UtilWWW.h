//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     R�mi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include <memory>
#include <afxinet.h>

#include "basic/ERMsg.h"
#include "Basic/UtilStd.h"
#include "Basic/Callback.h"


namespace UtilWWW
{
	typedef std::unique_ptr<CInternetSession> CInternetSessionPtr;
	typedef std::unique_ptr<CFtpConnection> CFtpConnectionPtr;
	typedef std::unique_ptr<CHttpConnection> CHttpConnectionPtr;



	CString GetServerName( const CString& path);
	CString GetServerPath( const CString& path);
	ERMsg GetPageText(CHttpConnectionPtr& pConnection, const std::string& page, std::string& text, bool replaceAccent = false, DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE);
	ERMsg GetPageTextCurl(const std::string& page, std::string& text);
	bool IsFileUpToDate(const WBSF::CFileInfo& ftpFile, const CString& localFile, bool bLookFileSize = true, bool bLookFileTime = true);
	bool IsFileUpToDate(CFtpConnectionPtr& pConnection, const CString& ftpFile, const CString& localFilePath, bool bLookFileSize=true, bool bLookFileTime=true);
	bool GetFileInfo(CFtpConnectionPtr& pConnect, const CString& filePath, WBSF::CFileInfo& info);
	void GetFileInfo(const CFtpFileFind& finder, WBSF::CFileInfo& info, bool bHaveWildcard);
	
	ERMsg GetHttpConnection(const CString& serverName, CHttpConnectionPtr& pConnection, CInternetSessionPtr& pSession, DWORD flags = PRE_CONFIG_INTERNET_ACCESS, const CString& userName = _T(""), const CString& password = _T(""), bool bHttps=false, size_t maxTry = 5, WBSF::CCallback& callback = WBSF::CCallback::DEFAULT_CALLBACK);
	ERMsg GetFtpConnection(const CString& serverName, CFtpConnectionPtr& pConnection, CInternetSessionPtr& pSession, DWORD flags=PRE_CONFIG_INTERNET_ACCESS, LPCTSTR userName=NULL, LPCTSTR password=NULL, BOOL bPassif=TRUE, size_t maxTry = 5, WBSF::CCallback& callback = WBSF::CCallback::DEFAULT_CALLBACK);
	ERMsg CopyFile(CHttpConnectionPtr& pConnection, const CString& URL, const CString& outputFilePath, DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE, BOOL bThrow = TRUE, WBSF::CCallback& callback = WBSF::CCallback::DEFAULT_CALLBACK);
	ERMsg CopyFile(CFtpConnectionPtr& pConnection, const CString& InputFilePath, const CString& outputFilePath, DWORD flags=INTERNET_FLAG_RELOAD|INTERNET_FLAG_DONT_CACHE, BOOL bThrow = TRUE);
	ERMsg CopyFileToFTP(CFtpConnectionPtr& pConnection, const CString& InputFilePath, const CString& outputFilePath);
	
	ERMsg FindFiles(CHttpConnectionPtr& pConnect, const CString& URL, WBSF::CFileInfoVector& fileList, BOOL bThrow=FALSE);
	ERMsg FindFiles(CFtpConnectionPtr& pConnect, const CString& URL, WBSF::CFileInfoVector& fileList, BOOL bThrow=FALSE, WBSF::CCallback& callback = WBSF::CCallback::DEFAULT_CALLBACK);
	ERMsg FindDirectories(CHttpConnectionPtr& pConnect, const CString& URL, WBSF::CFileInfoVector& fileList, BOOL bThrow=FALSE, WBSF::CCallback& callback = WBSF::CCallback::DEFAULT_CALLBACK);
	ERMsg FindDirectories(CFtpConnectionPtr& pConnect, const CString& URL, WBSF::CFileInfoVector& fileList);
	void ConvertString2FindFileInfo(CFtpConnectionPtr& pConnect, const CStringArray& fileListIn, WBSF::CFileInfoVector& fileListOut, BOOL bThrow = FALSE, WBSF::CCallback& callback = WBSF::CCallback::DEFAULT_CALLBACK);
	ERMsg WaitServer(size_t nbSecond, WBSF::CCallback& callback = WBSF::CCallback::DEFAULT_CALLBACK);



	//***********************************************************************************
	//stl adaptor
	ERMsg GetHttpConnection(const std::string& serverName, CHttpConnectionPtr& pConnection, CInternetSessionPtr& pSession, DWORD flags = PRE_CONFIG_INTERNET_ACCESS, const std::string& userName = "", const std::string& password = "", bool bHttps = false, size_t maxTry =5 , WBSF::CCallback& callback = WBSF::CCallback::DEFAULT_CALLBACK);
	ERMsg GetFtpConnection(const std::string& serverName, CFtpConnectionPtr& pConnection, CInternetSessionPtr& pSession, DWORD flags = PRE_CONFIG_INTERNET_ACCESS, const std::string& userName = "", const std::string& password = "", BOOL bPassif = TRUE, size_t maxTry = 5, WBSF::CCallback& callback = WBSF::CCallback::DEFAULT_CALLBACK);
	ERMsg CopyFile(CHttpConnectionPtr& pConnection, const std::string& URL, const std::string& outputFilePath, DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE, BOOL bThrow = TRUE, WBSF::CCallback& callback = WBSF::CCallback::DEFAULT_CALLBACK);
	ERMsg CopyFile(CFtpConnectionPtr& pConnection, const std::string& URL, const std::string& outputFilePath, DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE, BOOL bThrow = TRUE);

	ERMsg FindFiles(CHttpConnectionPtr& pConnect, const std::string& URL, WBSF::CFileInfoVector& fileList, BOOL bThrow = FALSE);
	ERMsg FindFiles(CFtpConnectionPtr& pConnect, const std::string& URL, WBSF::CFileInfoVector& fileList, BOOL bThrow = FALSE, WBSF::CCallback& callback = WBSF::CCallback::DEFAULT_CALLBACK);
	ERMsg FindDirectories(CHttpConnectionPtr& pConnect, const std::string& URL, WBSF::CFileInfoVector& fileList, BOOL bThrow=false, WBSF::CCallback& callback = WBSF::CCallback::DEFAULT_CALLBACK);
	ERMsg FindDirectories(CFtpConnectionPtr& pConnect, const std::string& URL, WBSF::CFileInfoVector& fileList);
	bool IsFileUpToDate(const WBSF::CFileInfo& info, const std::string& localFile, bool bLookFileSize = true, bool bLookFileTime = true);
	bool IsFileUpToDate(CFtpConnectionPtr& pConnection, const std::string& URL, const std::string& localFilePath, bool bLookFileSize = true, bool bLookFileTime = true);
	//ERMsg GetPageText(CHttpConnectionPtr& pConnection, const std::string& page, std::string& text, bool replaceAccent = false, DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE);

	//***
	//cURL
	ERMsg FindFilesCurl(const std::string& URL, WBSF::CFileInfoVector& fileList);
	ERMsg FindDirectoriesCurl(const std::string& _URL, WBSF::CFileInfoVector& fileList, WBSF::CCallback& callback);
	ERMsg CopyFileCurl(const std::string& URL, std::string& outputFilePath, bool bShowCurl = false);;

}

