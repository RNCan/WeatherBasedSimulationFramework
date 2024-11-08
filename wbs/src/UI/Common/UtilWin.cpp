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
#include "stdafx.h"  
#include <algorithm>

#include "Basic/Registry.h"
#include "Basic/UtilMath.h"

#include "UtilWin.h" 
#include "SYShowMessage.h"



#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

CString UtilWin::GetCurrentTimeString()
{
	CTime t = CTime::GetCurrentTime();
	return t.Format("%#c");
}
CString& UtilWin::InsertAt(CString& text, int pos, char car)
{
	ASSERT(pos >= 0 && pos <= text.GetLength());


	CString left = text.Left(pos);
	CString right = text.Right(text.GetLength() - pos);

	text = left + car + right;

	return text;
}

CString& UtilWin::RemoveAt(CString& text, int pos, int length)
{
	ASSERT(pos >= 0 && pos <= text.GetLength());


	//CString left  = text.Left(pos);
	//CString right = text.Right(text.GetLength() - pos); 

	//text = left + car + right;
	//length = Min( text.GetLength(), pos + length);
	text = text.Left(pos) + text.Mid(pos + length);
	return text;
}

CString& UtilWin::ReplaceComaByReturn(CString& string)
{

	for (int i = 0; i < string.GetLength(); i++)
	{
		if (string[i] == ';')
		{
			string.SetAt(i++, '\r');
			InsertAt(string, i, '\n');
		}
	}

	return string;
}

CString& UtilWin::ReplaceReturnByComa(CString& string)
{

	for (int i = 0; i < string.GetLength(); i++)
	{
		if (string[i] == '\r' || string[i] == '\n')
		{
			string.SetAt(i, ';');
			i++;
			if (string[i] == '\r' || string[i] == '\n')
			{
				RemoveAt(string, i);
				i--;
			}

		}
	}

	return string;
}

/*CString UtilWin::ToCString(float fVal, int prec)
{
	return ToString((double)fVal, prec);
}*/

CString UtilWin::RealToCString(double fVal, int pres)
{
	CString str;

	if (pres != -1)
	{
		CString format;
		format.Format(_T("%%.%dlf"), pres);
		str.Format(format, fVal);
	}
	else str.Format(_T("%g"), fVal);

	if (str.Find('.') != -1)//if it's a real;
	{
		int i = str.GetLength() - 1;
		while (i >= 0 && str[i] == '0')
			i--;

		if (i >= 0 && str[i] == '.') i--;
		str = str.Left(i + 1);
		if (str.IsEmpty())
			str = "0";
	}

	return str;
}

CString UtilWin::Int64ToCString(__int64 val, short n, char c)
{
	//char buffer[100]={0};
	//VERIFY( _i64toa_s(val, buffer, 100, 10) == 0);
	CString buffer;

	VERIFY(_i64tow_s(val, buffer.GetBufferSetLength(100), 100, 10) == 0);
	buffer.ReleaseBuffer();

	if (n >= 0)
	{
		ASSERT(buffer.GetLength() <= n);
		while (buffer.GetLength() < n)
			buffer.Insert(val < 0 ? 1 : 0, c);
	}

	return buffer;
}
//
//CString UtilWin::IntToString(int val, short n, char c)
//{
//	//char buffer[50]={0};
//	CString buffer;
//	
//	VERIFY( _itow_s(val, buffer.GetBufferSetLength(50), 50, 10) == 0);
//	buffer.ReleaseBuffer();
//
//	if(n>=0)
//	{
//		ASSERT( buffer.GetLength() <= n );
//		while( buffer.GetLength() < n )
//			buffer.Insert(val<0?1:0, c);
//	}
//	
//	return buffer;
//}

//float UtilWin::FeetToMetre (float fFeet)
//{
//	// get 2 decimal digit
//	float tmp = fFeet* 30.48f;
//	int tmp2 = (int)tmp;
//	tmp = (float)tmp2 * 0.01f;
//
//	return tmp;
//}
//
//
//float UtilWin::MetreToFeet(float fMetre)
//{
//	
//	// get 2 decimal digit
//	float tmp = fMetre* 328;
//	int tmp2 = (int)tmp;
//	tmp = (float)tmp2 * 0.01f;
//
//	return tmp;
//}

// random generation
/*void UtilWin::InitRandomize(unsigned rand)
{
	if( rand == 0 )
		srand( (unsigned)time( NULL ) );
	else
		srand( (unsigned) rand  );
}

*/

// looking for a name in a list
int UtilWin::FindStringExact(const CStringArray& nameArray, const CString& nameSearch, bool bCaseSensitive)
{
	INT_PTR rep = -1;
	INT_PTR nSize = nameArray.GetSize();

	for (INT_PTR i = 0; i < nSize; i++)
	{
		bool bRep = false;
		if (bCaseSensitive)
			bRep = nameArray[i] == nameSearch;
		else bRep = nameArray[i].CompareNoCase(nameSearch) == 0;

		if (bRep)
		{
			rep = i;
			break;
		}
	}

	return int(rep);
}

ERMsg UtilWin::CreateMultipleDir(const CString& path)
{
	ERMsg msg;

	CString tmp = path;
	if (tmp.IsEmpty())
		return msg;

	while (IsEndOk(tmp))
		tmp = tmp.Left(tmp.GetLength() - 1);



	if (tmp != "c:" && tmp != "C:")
	{
		CFileStatus status;
		if (!CFile::GetStatus(tmp, status) || !(status.m_attribute&CFile::directory))
		{
			int pos = std::max(tmp.ReverseFind('\\'), tmp.ReverseFind('/'));
			CString sousRep = tmp.Left(pos);
			msg = CreateMultipleDir(sousRep);
			if (msg)
			{
				//can't create directory when a file without extention with the same name exist!
				if (!CreateDirectory(tmp, NULL))
				{
					msg = SYGetMessage(GetLastError());
					return msg;
				}
			}
			else
			{
				return msg;
			}
		}
	}

	return msg;
}

CString UtilWin::GetRelativePath(const CString& basePath, const CString& filePath)
{
	bool bIsFilePath = true;
	/*relatifPath = filePath;

	bool bRep = false;
	CString filePathTmp(filePath);
	CString basePathTmp(basePath);

	filePathTmp.MakeLower();
	basePathTmp.MakeLower();
	int pos = filePathTmp.Find(basePathTmp);
	if ( pos >= 0 )
	{
		bRep = true;
		relatifPath = ".\\" + filePath.Mid(basePath.GetLength() );
	}
*/
//CString relatifPath;
//WBSF::CPath path( filePath );

//relatifPath = path.GetRelativePath( basePath );
//if( relatifPath.IsEmpty() )
//	relatifPath = filePath;

//return relatifPath;
	CStringW wFilePath = ToUTF16(filePath);
	CStringW szBaseFolder = ToUTF16(basePath);
	WCHAR	szRelativePath[_MAX_PATH] = { 0 };
	CString	sRelPath;

	PathRelativePathToW(szRelativePath, szBaseFolder, FILE_ATTRIBUTE_DIRECTORY,
		wFilePath, bIsFilePath ? 0 : FILE_ATTRIBUTE_DIRECTORY);

	sRelPath = szRelativePath;
	if (sRelPath.GetLength() > 1)
	{
		// Remove ".\" from the beginning
		if (sRelPath[0] == '\\')
			sRelPath = '.' + sRelPath;
	}

	return sRelPath;
}

bool UtilWin::GetRelativePath(const CString& basePath, const CString& filePath, CString& relatifPath)
{
	relatifPath = GetRelativePath(basePath, filePath);
	return true;
}

CString UtilWin::GetCurrentDirectory()
{
	CString currentDir;
	::GetCurrentDirectory(MAX_PATH, currentDir.GetBufferSetLength(MAX_PATH));
	currentDir.ReleaseBuffer();
	return currentDir + _T("\\");
}

CString UtilWin::GetAbsolutePath(const CString& basePathIn, const CString& relatifPathIn)
{
	CString basePath(basePathIn);
	CString relatifPath(relatifPathIn);

	//char	szAbsolutePath[_MAX_PATH] = { 0 };
	CStringW absolutePath;
	if (!relatifPath.IsEmpty())
	{
		relatifPath.Replace('/', '\\');

		if (relatifPath[0] == '\\')
		{

			//			WCHAR	szAbsolutePath[_MAX_PATH] = { 0 };
			CString sFullPath = ToUTF16(basePath);
			if (!UtilWin::IsEndOk(sFullPath))
				sFullPath += '/';

			sFullPath += ToUTF16(relatifPath);

			PathCanonicalizeW(absolutePath.GetBufferSetLength(_MAX_PATH), CStringW(sFullPath));
			absolutePath.ReleaseBuffer();

		}
		else
		{
			absolutePath = relatifPath;
		}

	}


	return CString(absolutePath);
}

bool UtilWin::GetAbsolutePath(const CString& basePath, const CString& relatifPath, CString& filePath)
{
	filePath = GetAbsolutePath(basePath, relatifPath);
	return true;
}

void UtilWin::GetLongPathName(CString& path)
{
	if (path.IsEmpty())
		return;

	//ASSERT(false);
	if (path.Find('~') != -1 && path.GetAt(0) != '/' && path.GetAt(0) != '\\')
	{
		CString tmp = path;
		path.Empty();

		tmp.Replace('/', '\\');


		if (tmp[tmp.GetLength() - 1] == '\\')
			path = '\\';

		SHFILEINFO sfi;
		SHGetFileInfo(tmp, 0, &sfi, sizeof(sfi), SHGFI_DISPLAYNAME);


		path = sfi.szDisplayName + path;
		tmp = tmp.Left(tmp.ReverseFind('\\'));

		while (tmp.GetLength() > 2)
		{
			//CString path = UtilWin::GetPath(lpszFileName);

			SHGetFileInfo(tmp, 0, &sfi, sizeof(sfi), SHGFI_DISPLAYNAME);
			CString rep = sfi.szDisplayName;


			path = rep + '\\' + path;
			tmp = tmp.Left(tmp.ReverseFind('\\'));

		}

		path = tmp + '\\' + path;
	}
	else
	{
		//ici on ne fait que changer l'extention pour avoir la bonne
		CFileFind fileList;

		BOOL working = fileList.FindFile(path, 0);

		if (working)
		{
			working = fileList.FindNextFile();
			path = fileList.GetFilePath();
		}

	}

}
void UtilWin::SimplifyPath(CString& path)
{
	const CString tmp(path);
	_wfullpath(path.GetBufferSetLength(MAX_PATH), tmp, MAX_PATH);
	path.ReleaseBuffer();
}

bool UtilWin::IsEndOk(const CString& filePath)
{
	CString C = filePath.Right(1);
	return  C == '\\' || C == '/';
}

CString UtilWin::GetFileExtension(const CString& filePath)
{
	CString ext;
	int pos = filePath.ReverseFind('.');
	if (pos != -1)
	{
		ext = filePath.Mid(pos);
	}

	//ext.MakeLower();
	return ext;
}

void UtilWin::SetFileExtension(CString& filePath, const CString& fileExtention)
{
	//    CString ext;
	//    strip(filePath);
	filePath.Trim();
	if (!filePath.IsEmpty() /*&& !fileExtention.IsEmpty()*/)
	{
		int pos = filePath.ReverseFind('.');
		if (pos != -1)
		{
			filePath = filePath.Mid(0, pos);
		}


		if (!fileExtention.IsEmpty())
		{
			if (fileExtention.Find('.') >= 0)
				filePath += fileExtention;
			else filePath += '.' + fileExtention;
		}

	}

}

CString UtilWin::GetTempPath()
{
	DWORD size = ::GetTempPath(0, NULL);

	CString tmp;
	::GetTempPath(size, tmp.GetBufferSetLength(size));
	tmp.ReleaseBuffer();

	return tmp;
}


void UtilWin::Purge(CString& cLine)
{
	int pos = cLine.Find(':');
	if (pos == -1)
		pos = cLine.Find(' ');

	cLine = cLine.Mid(pos + 1);
	cLine.TrimLeft();
	cLine.TrimRight();
}


CString UtilWin::GetLastDirName(CString filePath)
{
	filePath.Trim(_T(" "));
	while (filePath.Right(1) == '/' ||
		filePath.Right(1) == '\\')
	{
		filePath = filePath.Left(filePath.GetLength() - 1);
	}

	return UtilWin::GetFileTitle(filePath);
}


CString UtilWin::PurgeFileName(CString name)
{

	name.Replace('\\', '-');
	name.Replace('/', '-');
	name.Replace('\"', '-');
	name.Replace(':', '-');
	name.Replace('*', '-');
	name.Replace('?', '-');
	name.Replace('<', '-');
	name.Replace('>', '-');
	name.Replace('|', '-');
	name.Replace('\t', ' ');
	name.Replace('.', ' ');
	//name.Remove('.');

	//, is not a problem for file name but not realy usefull in CSV file
	//name.Remove(',');
	name.Replace(',', '-');
	name.Replace(';', '-');
	name.Trim();

	return name;
}

int UtilWin::GetEndNumber(CString name)
{
	int number = 0;
	if (!name.IsEmpty())
	{
		int pos = name.GetLength();
		while (pos > 0 && name[pos - 1] >= '0' && name[pos - 1] <= '9')
			pos--;
		number = ToInt(name.Mid(pos));
	}

	return number;
}

CString UtilWin::GenerateNewName(CString name)
{
	if (!name.IsEmpty())
	{
		CString oldTitle = GetFileTitle(name);

		//int no=1;
		while (FileExist(name))
		{
			CString title = GetFileTitle(name);
			int no = GetEndNumber(title);
			//no = (no==0)?2:no+1;
			if (no == 0)
			{
				title = oldTitle + _T("2");//ToString(no);
				SetFileTitle(name, title);
			}
			else
			{
				oldTitle.Replace(ToCString(no), ToCString(no + 1));
			}
		}
	}

	return name;
}

CString UtilWin::GetFileName(const CString& filePath)
{
	CString name;

	int pos = std::max(filePath.ReverseFind('\\'), filePath.ReverseFind('/')) + 1;
	name = filePath.Mid(pos);

	return name;
}

void UtilWin::SetFileName(CString& filePath, const CString fileName)
{
	filePath = GetPath(filePath) + fileName;
}

CString UtilWin::GetFileTitle(const CString& filePath)
{
	CString name;
	int posExt = filePath.ReverseFind('.');
	if (posExt == -1)
		posExt = filePath.GetLength();

	int pos = std::max(filePath.ReverseFind('\\'), filePath.ReverseFind('/')) + 1;

	name = filePath.Mid(pos, posExt - pos);

	return name;
}

void UtilWin::SetFileTitle(CString& filePath, const CString& fileTitle)
{
	//    strip(filePath);
	filePath.Trim();


	if (!filePath.IsEmpty() && !fileTitle.IsEmpty())
	{
		CString path = GetPath(filePath);
		CString ext = GetFileExtension(filePath);
		filePath = path + fileTitle + ext;
	}

}


CString UtilWin::GetPath(const CString& filePath)
{
	//if it's a path we return the path
	if (UtilWin::IsEndOk(filePath))
		return filePath;

	CString path;
	int pos1 = filePath.ReverseFind('\\');
	int pos2 = filePath.ReverseFind('/');
	int pos = pos1 > pos2 ? pos1 : pos2;

	if (pos != -1)
	{
		path = filePath.Left(pos + 1);
	}

	//path.MakeLower();
	return path;
}

void UtilWin::SetFilePath(CString& filePath, const CString& path)
{
	if (!filePath.IsEmpty() && !path.IsEmpty())
	{
		CString fileName = GetFileName(filePath);

		filePath = path;
		if (!IsEndOk(filePath))
			filePath += "\\";
		filePath += fileName;
	}
}

CString UtilWin::GetDrive(const CString& filePath)
{

	CString drive;
	int pos1 = filePath.Find('\\');
	int pos2 = filePath.Find('/');
	int pos = pos1 > pos2 ? pos1 : pos2;

	if (pos != -1)
	{
		drive = filePath.Left(pos + 1);
	}

	return drive;
}



bool UtilWin::GetIntArray(const CString& texte, CIntArray& intArray, int maxPos)
{
	bool bRep = false;

	CString param = texte;
	param.TrimLeft();
	param.TrimRight();

	intArray.RemoveAll();
	if (!param.IsEmpty())
	{
		int pos1 = 0;
		int pos2 = param.Find(',', 0);

		while (pos1 != -1)
		{
			CString element;
			if (pos2 != -1)
				element = param.Mid(pos1, pos2 - pos1);
			else element = param.Mid(pos1);

			if (element.Find('*', 0) != -1)
			{
				intArray.RemoveAll();
				intArray.SetSize(maxPos);

				for (int i = 0; i < maxPos; i++)
					intArray[i] = i;

				break;
			}

			if (element.Find('-', 0) != -1)
			{
				int posTmp = element.Find('-', 0);
				CString firstCorner = element.Mid(0, posTmp);

				double fFirstValue;
				if (StringToReal(firstCorner, fFirstValue))
				{
					if (int(fFirstValue) < 0)
						fFirstValue = 0;

					CString secondCorner = element.Mid(posTmp + 1);

					double fSecondValue;
					if (StringToReal(secondCorner, fSecondValue))
					{
						if ((int)fSecondValue >= maxPos)
							fSecondValue = maxPos - 1;

						for (int i = (int)fFirstValue; i <= (int)fSecondValue; i++)
							intArray.Add(i);
					}
					else
					{
						//      bRep = false;
							  //message.asgType(ERMsg::ERREUR);
							  //message.ajoute(CResString::GetString(IDS_COM_UNABLE_EXTRACT_ARRAY ) );
						break;
					}
				}
				else
				{
					//    bRep = false;
						//message.asgType(ERMsg::ERREUR);
						//message.ajoute(CResString::GetString(IDS_COM_UNABLE_EXTRACT_ARRAY ) );
					break;
				}


			}
			else
			{

				double value;
				if (StringToReal(element, value))
				{
					if (int(value) >= 0 && int(value) < maxPos)
						intArray.Add(int(value));
				}
				else
				{
					//bRep = false;
					//message.asgType(ERMsg::ERREUR);
					//message.ajoute(CResString::GetString(IDS_COM_UNABLE_EXTRACT_ARRAY ) );
					break;
				}
			}

			pos1 = pos2;
			if (pos1 != -1)
			{
				pos1++;//éliminer la ,
				pos2 = param.Find(',', pos1);
			}

		}

		if (intArray.GetSize() > 0)
		{
			bRep = true;
			//message.asgType(ERMsg::ERREUR);
			//message.ajoute(CResString::GetString(IDS_COM_UNABLE_EXTRACT_ARRAY ) );
		}

	}

	return bRep;
}

bool UtilWin::StringToReal(const CString& realStr, double& fValue)
{
	bool bRep = false;
	if (!realStr.IsEmpty())
	{
		TCHAR* pRest = NULL;
		fValue = _tcstod((LPCTSTR)realStr, &pRest);

		CString tmp(pRest);
		tmp.TrimLeft();

		if (tmp.GetLength() == 0)
			bRep = true;
	}

	return bRep;
}


/*bool UtilWin::StringTokenizerReal(CString& realStr, float& fValue)
{
	double value;
	bool bRep = StringTokenizerReal(realStr, value);
	fValue = (float)value;

	return bRep;
}
*/
bool UtilWin::StringTokenizerReal(CString& realStr, double& fValue)
{
	bool bRep = false;
	if (!realStr.IsEmpty())
	{
		TCHAR* pRest = NULL;
		TCHAR* pExpression = realStr.GetBuffer(50);
		fValue = _tcstod(pExpression, &pRest);

		if (fValue > -HUGE_VAL && fValue < HUGE_VAL && ((pRest - pExpression) > 0))
		{
			ASSERT(pExpression < pRest);
			ASSERT(pRest - pExpression < 50);
			realStr = realStr.Mid(int(pRest - pExpression));
			realStr.TrimLeft();
			bRep = true;
		}
	}

	return bRep;
}

bool UtilWin::StringToInt(const CString& intStr, int& nValue)
{
	bool bRep = false;
	if (!intStr.IsEmpty())
	{

		TCHAR* pRest = NULL;
		nValue = (int)_tcstod((LPCTSTR)intStr, &pRest);

		CString tmp(pRest);
		tmp.TrimLeft();

		if (tmp.GetLength() == 0)
			bRep = true;
	}

	return bRep;
}

int UtilWin::GetFilesList(CStringArray& fileNameArray, const CString& filePath, bool fullPath, bool bSubDirSearch)
{

	return GetFilesList(fileNameArray, filePath, fullPath ? FILE_PATH : FILE_TITLE, bSubDirSearch);
}

int UtilWin::GetFilesList(CStringArray& fileNameArray, const CString& filePath, int fullPath, bool bSubDirSearch)
{
	ASSERT(!bSubDirSearch || fullPath == FILE_PATH); //if bSubDirSearch -> fullPath;

	if (!bSubDirSearch)
		fileNameArray.RemoveAll();

	bool bAddEmptyExtension = filePath.Right(1) == ".";


	// CAdvancedFileFind: je ne me raplele plus pourquoi j'uttilisais cela????????
	//CAdvancedFileFind fileList;
	//BOOL bWorking = fileList.FindFile(filePath,  FIND_FIRST_EX_DO_EXACT_MATCH);

	CFileFind fileList;
	BOOL bWorking = fileList.FindFile(filePath);

	while (bWorking)
	{
		bWorking = fileList.FindNextFile();
		//if (!bWorking && !fileList.is.IsLastFile())
		if (!bWorking)
			break;

		bool bAddDirectory = fileList.IsDirectory() && bAddEmptyExtension;

		if (!fileList.IsDirectory() || bAddDirectory)
		{
			switch (fullPath)
			{
			case FILE_TITLE:fileNameArray.Add(GetFileTitle(fileList.GetFilePath())); break;
			case FILE_NAME: fileNameArray.Add(GetFileName(fileList.GetFilePath())); break;
			case FILE_PATH: fileNameArray.Add(fileList.GetFilePath()); break;
			default: ASSERT(false);
			}

		}
	}

	if (bSubDirSearch)
	{
		//add directory to the list
		CString filePathTmp(filePath);
		UtilWin::SetFileName(filePathTmp, _T("*.*"));
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

	return int(fileNameArray.GetSize());
}

int UtilWin::GetDirList(CStringArray& fileNameArray, const CString& filePath, bool fullPath, bool bSubDirSearch)
{

	if (!bSubDirSearch)
		fileNameArray.RemoveAll();


	CFileFind fileList;

	BOOL working = fileList.FindFile(filePath, 0);

	while (working)
	{
		working = fileList.FindNextFile();
		if (fileList.IsDirectory())
		{
			if (!fileList.IsDots())
			{
				if (fullPath)
					fileNameArray.Add(fileList.GetFilePath() + _T("\\"));
				else fileNameArray.Add(fileList.GetFileTitle());

				if (bSubDirSearch)
				{
					CString newPath = fileList.GetFilePath() + _T("\\");
					GetDirList(fileNameArray, newPath, fullPath, bSubDirSearch);
				}
			}
		}
	}

	return int(fileNameArray.GetSize());
}
//
//ERMsg UtilWin::WinExecWait(const CString& command, CString dir, UINT uCmdShow, LPDWORD pExitCode )
//{
//	ERMsg msg;
//
//	while( IsEndOk(dir) )
//		dir = dir.Left(dir.GetLength()-1);
//
//	STARTUPINFO si;
//	::ZeroMemory(&si, sizeof(STARTUPINFO) );
//	si.cb = sizeof(STARTUPINFO);
//    si.dwFlags = STARTF_USESHOWWINDOW;
//    si.wShowWindow = uCmdShow;
//
//	PROCESS_INFORMATION pi;
//	
//	CStringW wCommand = ToUTF16(command);
//	CStringW wDir = ToUTF16(dir);
//	const WCHAR* pDir = wDir.IsEmpty() ? (WCHAR*)NULL : (LPCWSTR)wDir;
//	if (::CreateProcessW(NULL, wCommand.GetBuffer(), NULL, NULL, FALSE, NULL, NULL, pDir, &si, &pi))
//	{
//		::CloseHandle( pi.hThread);
//		::WaitForSingleObject ( pi.hProcess, INFINITE);
//		if( pExitCode!=NULL )
//			::GetExitCodeProcess(pi.hProcess,pExitCode);
//		
//	}
//	else
//	{
//		CString error;
//		error.FormatMessage(IDS_BSC_APP_NOTEXEC, command );
//		msg.ajoute( ToUTF8(error) );
//	}
//
//	return msg;
//}
//
//
//ERMsg UtilWin::CallApplication( CString appType, CString argument, CWnd* pCaller, int showMode, bool bAddCote, bool bWait )
//{
//	ERMsg msg;
//
//   	WBSF::CRegistry registry;
//
//	CString appFilePath = Convert(registry.GetAppFilePath(ToUTF8(appType)));
//
//	if( bAddCote )
//		argument = _T("\"") + argument + _T('\"');
//
//    CString command = _T("\"") + appFilePath  + _T("\" ") + argument;
//	
//	if( bWait )
//	{
//		msg = UtilWin::WinExecWait(command, _T(""), showMode);
//	}
//	else  
//	{
//		if (WinExec(ToUTF8(command).c_str(), showMode) < 31)
//		{
//			CString error;
//			error.FormatMessage(IDS_BSC_APP_NOTEXEC, appFilePath );
//			msg.ajoute( ToUTF8(error) );
//		}
//	}
//
//	if( !msg )
//	{
//		msg = AskToFindApplication(appType, appFilePath, pCaller);
//		if( msg )
//		{
//			command = _T("\"") + appFilePath + _T("\" ") + argument;
//
//			if( bWait )
//			{
//				msg = UtilWin::WinExecWait(command, _T(""), showMode);
//			}
//			else
//			{
//				if( WinExec(ToUTF8(command).c_str(), showMode) < 31)
//				{
//					msg.ajoute(WBSF::FormatMsg(IDS_BSC_APP_NOTEXEC, ToUTF8(appFilePath) ));
//				}
//			}
//		}
//	}
//
//    return msg;
//}
//
//
//
//ERMsg UtilWin::AskToFindApplication(CString appType, CString& appFilePath, CWnd* pCaller)
//{
//	ERMsg msg;
//
//    ASSERT(pCaller != NULL);
//	
//	CString appName = GetFileName(appFilePath);
//
//	CString error;
//	error.FormatMessage(IDS_BSC_APP_NOTFOUND, appName );
//
//	if( pCaller->MessageBox( error, AfxGetAppName(), MB_YESNO) == IDYES)
//	{
//		CFileDialog browse(true, _T("*.exe"), appName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("Program (*.exe)|*.exe||"), pCaller);
//		if( browse.DoModal() == IDOK )
//		{
//			appFilePath = browse.GetPathName();
//			WBSF::CRegistry registry;
//			registry.SetAppFilePath( ToUTF8(appType), ToUTF8(appFilePath) );
//		}
//		else 
//		{
//			CString error;
//			error.FormatMessage(IDS_BSC_APP_NOTEXEC, appFilePath );
//			msg.ajoute(ToUTF8(error));
//		}
//	}
//	else 
//	{
//		CString error;
//		error.FormatMessage(IDS_BSC_APP_NOTEXEC, appFilePath );
//		msg.ajoute(ToUTF8(error));
//	}
//
//	return msg;
//}
bool UtilWin::DirExist(const CString& path)
{
	ASSERT(IsEndOk(_T("e:\\temp\\")) == true);
	ASSERT(IsEndOk(_T("e:/temp/")) == true);
	ASSERT(IsEndOk(_T("e:\\temp")) == false);
	ASSERT(IsEndOk(_T("e:/temp")) == false);

	CString tmp(path);
	tmp.Trim();
	//    strip(tmp);

	while (IsEndOk(tmp))
		tmp = tmp.Left(tmp.GetLength() - 1);

	CFileStatus status;
	//Ne focntionne pas quand on est à la racine: f:
	return CFile::GetStatus(tmp, status) != 0 && (status.m_attribute&CFile::directory);
}

bool UtilWin::FileExist(const CString& fileName)
{
	bool bExist = false;
	HANDLE hFile;

	if (!fileName.IsEmpty())
	{
		// Use the preferred Win32 API call and not the older OpenFile.
		hFile = CreateFile(
			fileName,
			GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			0,
			0);

		if (hFile != INVALID_HANDLE_VALUE)
		{
			// If the handle is valid then the file exists.
			CloseHandle(hFile);
			bExist = true;
		}
	}

	return (bExist);

}

void UtilWin::EnsurePointOnDisplay(CPoint& pt)
{
	MONITORINFO mi;
	memset(&mi, 0, sizeof(mi));
	mi.cbSize = sizeof(mi);

	HMONITOR hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
	if (hMonitor)
		GetMonitorInfo(hMonitor, &mi);

	// Now we have a clipping rectangle,
	// bring our input point into sight.
	if (pt.x > mi.rcWork.right)
	{
		pt.x = mi.rcWork.right;
	}
	if (pt.y > mi.rcWork.bottom)
	{
		pt.y = mi.rcWork.bottom;
	}
	if (pt.x < mi.rcWork.left)
	{
		pt.x = mi.rcWork.left;
	}
	if (pt.y < mi.rcWork.top)
	{
		pt.y = mi.rcWork.top;
	}
}

// Take coords, and shift them so all corners
// (if possible) appear on the nearest screen.
void UtilWin::EnsureRectangleOnDisplay(CRect& rect)
{

	MONITORINFO mi;
	memset(&mi, 0, sizeof(mi));
	mi.cbSize = sizeof(mi);

	HMONITOR hMonitor = MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST);
	if (hMonitor)
		GetMonitorInfo(hMonitor, &mi);

	// Now we have a clipping rectangle,
	// bring our input rectangle into sight.
	if (rect.right > mi.rcWork.right)
	{
		rect.left -= rect.right - mi.rcWork.right;
		rect.right = mi.rcWork.right;
	}
	if (rect.bottom > mi.rcWork.bottom)
	{
		rect.top -= rect.bottom - mi.rcWork.bottom;
		rect.bottom = mi.rcWork.bottom;
	}
	if (rect.left < mi.rcWork.left)
	{
		rect.right += mi.rcWork.left - rect.left;
		rect.left = mi.rcWork.left;
	}
	if (rect.top < mi.rcWork.top)
	{
		rect.bottom += mi.rcWork.top - rect.top;
		rect.top = mi.rcWork.top;
	}
}

double UtilWin::CStringToCoord(const CString& coordStr)
{
	double coord = 0;
	double a = 0, b = 0, c = 0;
	int nbVal = _stscanf_s(coordStr, _T("%lf %lf %lf"), &a, &b, &c);
	if (nbVal == 1)
	{
		//decimal degree 
		coord = a;
	}
	else if (nbVal >= 2 && nbVal <= 3)
	{
		//degree minute
		coord = WBSF::GetDecimalDegree((int)a, (int)b, c);
	}


	//CString tmp;
	//tmp.Format( "%.12lf", coord);
	return coord;
}

CString UtilWin::CoordToCString(double coord, bool bWithFraction)
{
	int prec = 0;
	if (bWithFraction)
		prec = 2;

	return CString(WBSF::CoordToString(coord, prec).c_str());
	/*CString deg;
	CString min;
	CString sec;

	CString str;

	int prec = 0;
	if( bWithFraction )
		prec = 2;

	double mult = pow(10.0, prec);
	deg = ToCString(WBSF::GetDegrees(coord, mult));
	min = ToCString(WBSF::GetMinutes(coord, mult));
	sec = ToCString(WBSF::GetSeconds(coord, mult), prec);

	if( sec == "0" || sec == "-0")
		sec.Empty();
	if( sec.IsEmpty() && (min == "0"||min == "-0") )
		min.Empty();

	str.Format( _T("%s %s %s"), deg, min, sec );
	str.Trim();*/
	//return str;
}

CString UtilWin::ToCString(CRect rect)
{
	CString str;
	str.Format(_T("%d %d %d %d"), rect.top, rect.left, rect.bottom, rect.right);

	return str;
}

CRect UtilWin::CRectFromCString(const CString& str)
{
	CRect rect;

	_stscanf_s(str, _T("%d %d %d %d"), &rect.top, &rect.left, &rect.bottom, &rect.right);

	return rect;
}

COLORREF UtilWin::ToCOLORREF(const CString& str)
{
	int r = 0;
	int g = 0;
	int b = 0;
	_stscanf_s(str, _T("%d %d %d"), &r, &g, &b);

	return RGB(r, g, b);
}


bool UtilWin::SetClipboardText(const CString& str)
{
	bool bRep = false;

	if (AfxGetMainWnd()->OpenClipboard())
	{
		EmptyClipboard();

		// Allouer de la mémoire relocalisable globale
		//-------------------------------------------
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, str.GetLength() + 1);

		// Verrouiller le bloc afin d'obtenir un pointeur éloigné
		// sur cette mémoire
		TCHAR* pBuffer = (TCHAR*)GlobalLock(hMem);

		// Copier la chaîne dans cette mémoire
		_tcscpy_s(pBuffer, str.GetLength() + 1, (LPCTSTR)str);

		// Relâche la mémoire et copier sur le clipboard
		GlobalUnlock(hMem);

		SetClipboardData(CF_TEXT, hMem);


		bRep = CloseClipboard();
	}

	return bRep;
}

CString UtilWin::GetClipboardText()
{
	CString str;

	if (AfxGetMainWnd()->OpenClipboard())
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

CString UtilWin::GetVersionString(const CString& filerPath)
{
	CString version;

	DWORD dwDummy;
	DWORD dwFVISize = GetFileVersionInfoSize(filerPath, &dwDummy);
	if (dwFVISize > 0)
	{
		LPBYTE lpVersionInfo = new BYTE[dwFVISize];
		VERIFY(GetFileVersionInfo(filerPath, 0, dwFVISize, lpVersionInfo));
		UINT uLen;
		VS_FIXEDFILEINFO *lpFfi;
		VerQueryValue(lpVersionInfo, _T("\\"), (LPVOID *)&lpFfi, &uLen);
		DWORD dwFileVersionMS = lpFfi->dwFileVersionMS;
		DWORD dwFileVersionLS = lpFfi->dwFileVersionLS;
		delete[] lpVersionInfo;

		DWORD dwLeftMost = HIWORD(dwFileVersionMS);
		DWORD dwSecondLeft = LOWORD(dwFileVersionMS);
		DWORD dwSecondRight = HIWORD(dwFileVersionLS);
		DWORD dwRightMost = LOWORD(dwFileVersionLS);


		version.Format(_T("%d.%d.%d.%d"), dwLeftMost, dwSecondLeft, dwSecondRight, dwRightMost);
	}

	return version;
}

CString UtilWin::GetCompilationDateString(char *compilation_date)
{
	CString str;

	//char *compilation_date = __DATE__;
	//char *compilation_time = __TIME__;
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

		strncpy(year, compilation_date + 7, 4);
		strncpy(day, compilation_date + 4, 2);

		CTime time(ToInt(CString(year)), month + 1, ToInt(CString(day)), 12, 0, 0);
		str = time.Format(_T("%x"));
	}

	return str;
}

CString UtilWin::GetImportImageFilter()
{
	CString fileFilter;
	CSimpleArray<GUID> aguidFileTypes;
	HRESULT hResult = CImage::GetImporterFilterString(fileFilter, aguidFileTypes);
	if (FAILED(hResult))
	{
		CString fmt;
		fmt.Format(_T("GetExporterFilter failed:\n%x - %s"), hResult, _com_error(hResult).ErrorMessage());
		::AfxMessageBox(fmt);

		return _T("bimtap (*.bmp)|*.bmp||");
	}

	return fileFilter;
}

CString UtilWin::GetExportImageFilter()
{
	CString fileFilter;
	CSimpleArray<GUID> aguidFileTypes;
	HRESULT hResult = CImage::GetExporterFilterString(fileFilter, aguidFileTypes);
	if (FAILED(hResult))
	{
		CString fmt;
		fmt.Format(_T("GetExporterFilter failed:\n%x - %s"), hResult, _com_error(hResult).ErrorMessage());
		::AfxMessageBox(fmt);

		return _T("bimtap (*.bmp)|*.bmp||");
	}

	return fileFilter;
}

CString UtilWin::GetDefaultFilterExt(CString filter, int nFilterIndex)
{
	ASSERT(nFilterIndex >= 0);//warning, COpenDlg take and return inde in 1 base
	CString ext;
	CStringArrayEx filters(filter, _T("|"));
	if (nFilterIndex * 2 + 1 < filters.GetSize())
		ext = filters[nFilterIndex * 2 + 1];
	int pos = ext.FindOneOf(_T(";"));
	if (pos > -1)
		ext = ext.Left(pos);
	ext.Remove('*');

	return ext;
}

namespace UtilWin
{


	CStringException::CStringException(CString msg) :m_msg(msg)
	{}

	CStringException::~CStringException()
	{}

	BOOL CStringException::GetErrorMessage(LPTSTR lpszError, UINT nMaxError, PUINT pnHelpContext) const
	{
		return const_cast<CStringException*>(this)->GetErrorMessage(lpszError, nMaxError, pnHelpContext);
	}

	BOOL CStringException::GetErrorMessage(LPTSTR lpszError, UINT nMaxError, PUINT pnHelpContext)
	{
		if (m_msg.GetLength() >= (int)nMaxError)
			return FALSE;


		_tcscpy(lpszError, m_msg.GetString());
		return TRUE;
	}




#ifdef _DEBUG
#define DATA_ACCESS_OPERATOR(i) ((*this)[i]) // for better bug tracking
#define FAST_ACCESS_OPERATOR(var,i) ((var)[(i)]) // for better bug tracking
#else
#define DATA_ACCESS_OPERATOR(i) (m_pData[i]) // 10 times faster
#define FAST_ACCESS_OPERATOR(var,i) ((var).GetData()[(i)]) 
#endif

	//

	//**************************
	ERMsg CStdioFileEx::Open(LPCTSTR lpszFileName, UINT nOpenFlags)
	{
		ERMsg msg;

		CFileException e;
		if (!CStdioFile::Open(lpszFileName, nOpenFlags, &e))
		{
			msg = SYGetMessage(e);
		}

		return msg;
	}
	//return CStdioFile::Open(lpszFileName, m_bGoodSeek?nOpenFlags|CFile::typeBinary:nOpenFlags, pError);


	BOOL CStdioFileEx::ReadString(CString& rString)
	{
		BOOL bRep = CStdioFile::ReadString(rString);
		//because we work in binary file, we have to remove \r char
		if (m_bGoodSeek)
			rString.Remove('\r');

		return bRep;
	}

	BOOL CStdioFileEx::ReadText(CString& rString)
	{
		CString tmp;
		while (ReadString(tmp))
			rString += tmp + _T("\n");

		return !rString.IsEmpty();
	}


	CStringA UTF16toUTF8(const CStringW& utf16)
	{
		return CStringA((LPCWSTR)utf16);
		//CStringA utf8 = CW2A(utf16, CP_UTF8);
		//return utf8;

		/*CStringA utf8;
		int len = utf16.GetLength() *4;
		char *ptr = utf8.GetBuffer(len);
		if (ptr)
			WideCharToMultiByte(CP_UTF8, 0, utf16, -1, ptr, len, 0, 0);
		utf8.ReleaseBuffer();
		return utf8;*/
	}

	CStringW UTF8toUTF16(const CStringA& utf8)
	{
		return CStringW((LPCSTR)utf8);

		/*CStringW utf16;
		int len = utf8.GetLength();
		WCHAR *ptr = utf16.GetBuffer(len);
		if (ptr) MultiByteToWideChar(CP_UTF8, 0, utf8, -1, ptr, len);
		utf16.ReleaseBuffer();
		return utf16;*/
	}
	/*

	CString Crypt(const CString& str)
	{
		CString strOut;
		for(int i=0; i<str.GetLength(); i++)
		{
			int n = (i%2)?i:str.GetLength()-i;
			unsigned char c = unsigned char (str[i]);
			c+=(0xCA+n);
			if( c!=0 )
				strOut.AppendChar( c );
			else strOut.Append("z°ÜÆ");
		}

		//ASSERT( strOut.GetLength() == str.GetLength() );
		return strOut;
	}

	CString Decrypt(const CString& str)
	{

		CArray<unsigned char> str2;
		bool bHaveZero = str.Find( "z°ÜÆ" ) >= 0;

		for(int i=0; i<str.GetLength(); i++)
		{
			if( bHaveZero && str.Find( "z°ÜÆ", i ) == i)
			{
				str2.Add( 0 );
				i+=3;
			}
			else
			{
				str2.Add( str[i] );
			}
		}


		CString strOut;
		for(INT_PTR i=0; i<str2.GetSize(); i++)
		{
			INT_PTR n = (i%2)?i:str2.GetSize()-i;
			unsigned char c = str2[i];
			c-=unsigned char(0xCA+n);

			strOut.AppendChar( c );
		}

		ASSERT( strOut.GetLength() == str2.GetSize() );
		return strOut;
	}
	*/

	// initialisation de CDeLiVaApp
	bool SetIconApp(CString szAppName, CString szPathIcon)
	{
		/*		HKEY hKey=NULL;
				long ret;
			   // Création de la racine szAppName
				 if(RegCreateKeyEx(HKEY_CLASSES_ROOT, szAppName, &hKey)!=ERROR_SUCCESS) return false;
						ret = RegSetValueEx(hKey, "DefaultIcon", REG_SZ,szPathIcon,MAX_PATH);

				 if(ret==ERROR_SUCCESS) RegCloseKey(hKey);
				 */
		HKEY hKey = NULL;
		DWORD dw;
		long ret = RegOpenKeyExW(HKEY_CLASSES_ROOT, ToUTF16(szAppName), 0, KEY_QUERY_VALUE, &hKey);
		if (ret == ERROR_SUCCESS)
			ret = RegCreateKeyExW(hKey, L"DefaultIcon", 0L, NULL,
				REG_OPTION_NON_VOLATILE,
				KEY_ALL_ACCESS,
				NULL, &hKey, &dw);

		if (ret == ERROR_SUCCESS)
			ret = RegSetValueExW(hKey, L"", 0, REG_SZ, (LPBYTE)(LPCTSTR)szPathIcon, szPathIcon.GetLength());

		if (ret == ERROR_SUCCESS)
			ret = RegCloseKey(hKey);

		return (ret == ERROR_SUCCESS);
	}

	BOOL SetRunAppWithExtension(CString szAppName, CString szCommandLine, CString szExtName)
	{
		HKEY hkey;
		long ret = RegOpenKeyExW(HKEY_CLASSES_ROOT, L"", 0, KEY_QUERY_VALUE, &hkey);

		TCHAR errmsg[1024];
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, ret, 0, errmsg, 1024, NULL);


		DWORD dw;

		HKEY hkey2;
		ret = RegCreateKeyEx(hkey, (LPCTSTR)szExtName, 0L, NULL,
			REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS,
			NULL, &hkey2, &dw);
		//CString key=sDescription;
		RegSetValueExW(hkey2, L"", 0, REG_SZ, (LPBYTE)(LPCWSTR)ToUTF16(szAppName), szAppName.GetLength());
		RegCloseKey(hkey);
		RegCloseKey(hkey2);

		RegOpenKeyExW(HKEY_CLASSES_ROOT, L"", 0, KEY_QUERY_VALUE, &hkey);
		RegCreateKeyExW(hkey, szAppName, 0L, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, &dw);
		RegCreateKeyExW(hkey, L"shell\\open\\command", 0L, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, &dw);
		RegSetValueExW(hkey, L"", 0, REG_SZ, (LPBYTE)(LPCTSTR)szCommandLine, szCommandLine.GetLength());
		RegCloseKey(hkey);

		return TRUE;
	}

	//BOOL RegisterExtension(CString sExt, CString sDescription)
	//{
	//	// sDescription is the description key for you application

	//	CString name = AfxGetAppName();
	//	
	//	CString path=UtilWin::GetApplicationPath()+name+ _T(".exe"); // Getting the application name
	//	if( !SetRunAppWithExtension(name, path + _T(" \"%1\""), sExt))
	//		return false;

	//	if( !SetIconApp(name, path + _T(",0") ) )
	//		return false;

	//	return TRUE;
	//}

	typedef HRESULT(WINAPI *PGetDpiForMonitor)(HMONITOR hmonitor, int dpiType, UINT* dpiX, UINT* dpiY);

	WORD GetWindowDPI(HWND hWnd)
	{
		// Try to get the DPI setting for the monitor where the given window is located.
		// This API is Windows 8.1+.
		HMODULE hShcore = LoadLibraryW(L"shcore");
		if (hShcore)
		{
			PGetDpiForMonitor pGetDpiForMonitor =
				reinterpret_cast<PGetDpiForMonitor>(GetProcAddress(hShcore, "GetDpiForMonitor"));
			if (pGetDpiForMonitor)
			{
				HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY);
				UINT uiDpiX;
				UINT uiDpiY;
				HRESULT hr = pGetDpiForMonitor(hMonitor, 0, &uiDpiX, &uiDpiY);
				if (SUCCEEDED(hr))
				{
					return static_cast<WORD>(uiDpiX);
				}
			}
		}

		// We couldn't get the window's DPI above, so get the DPI of the primary monitor
		// using an API that is available in all Windows versions.
		HDC hScreenDC = GetDC(0);
		int iDpiX = GetDeviceCaps(hScreenDC, LOGPIXELSX);
		ReleaseDC(0, hScreenDC);

		return static_cast<WORD>(iDpiX);
	}
}

CStringArrayEx::CStringArrayEx(const CStringArray& in)
{
	operator=(in);
}

bool CStringArrayEx::LoadString(UINT nID, const CString& token)
{
	bool bRep = false;

	CString str;
	if (str.LoadString(nID))
	{
		bRep = true;
		LoadString(str, token);
	}

	ASSERT(bRep);
	return bRep;
}

void CStringArrayEx::LoadString(const CString& str, const CString& token)
{
	RemoveAll();

	int pos = 0;
	CString item = str.Tokenize(token, pos);
	while (pos != -1)
	{
		Add(item);
		item = str.Tokenize(token, pos);
	}

}


CStringArrayEx& CStringArrayEx::operator = (const CStringArray& in)
{
	if (this != &in)
	{
		Copy(in);
	}

	return *this;
}

CStringArrayEx& CStringArrayEx::operator = (const CStringArrayEx& in)
{
	if (this != &in)
	{
		Copy(in);
	}

	return *this;
}


bool CStringArrayEx::operator == (const CStringArray& in) const
{
	register INT_PTR i;
	register INT_PTR nSize(GetSize());

	if (nSize != in.GetSize())
	{
		return FALSE;
	}

	for (i = 0; i < nSize; i++)
	{
		if (!(DATA_ACCESS_OPERATOR(i) == FAST_ACCESS_OPERATOR(in, i)))
		{
			return FALSE;
		}
	}

	return TRUE;
}


CString CStringArrayEx::ToString(const CString& sep, bool bAddBraquet)const
{
	CString str;
	if (GetSize() > 0)
	{
		if (bAddBraquet)
			str += _T("[");
		for (int i = 0; i < GetSize(); i++)
		{
			str += i == 0 ? _T("") : sep;
			str += DATA_ACCESS_OPERATOR(i);
		}
		if (bAddBraquet)
			str += _T("]");
	}

	return str;
}


void CStringArrayEx::FromString(const CString& str, const CString& sep)
{
	RemoveAll();

	CString elem;
	int pos = 0;

	while (pos != -1)
	{
		elem = str.Tokenize(sep + _T("[]"), pos);

		if (!elem.IsEmpty())
			Add(elem);
	}
}

int CStringArrayEx::Find(const CString in, bool bCaseSensitive)const
{
	return UtilWin::FindStringExact(*this, in, bCaseSensitive);
}
