//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once 

#include <afxtempl.h>
#include <math.h>
#include <WinUser.h>
#include "basic/ERMsg.h"

#pragma warning( disable : 4800)


namespace UtilWin
{
	typedef CArray <bool > CBoolArray;
	typedef CArray < int > CIntArray;
	typedef CArray < float > CFloatArray;
	typedef CArray < double > CDoubleArray;


	int FindStringExact(const CStringArray& nameArray, const CString& nameSearch, bool bCaseSensitive = true);

	inline CString GetCString(UINT nId)
	{
		CString str;
		VERIFY(str.LoadString(nId));
		return str;
	}

	CString& InsertAt(CString& text, int pos, char car);
	CString& RemoveAt(CString& text, int pos, int length = 1);
	CString& ReplaceReturnByComa(CString& string);
	CString& ReplaceComaByReturn(CString& string);
	void Purge(CString& cLine);


	bool IsEndOk(const CString& filePath);

	CString GetTempPath();
	CString GetFileTitle(const CString& filePath);
	void SetFileTitle(CString& filePath, const CString& fileTitle);
	CString GetFileExtension(const CString& filePath);
	void SetFileExtension(CString& filePath, const CString& fileExtention);
	CString GetFileName(const CString& filePath);
	void SetFileName(CString& filePath, const CString fileName);
	CString GetPath(const CString& filePath);
	void SetFilePath(CString& filePath, const CString& path);
	CString GetDrive(const CString& filePath);
	CString PurgeFileName(CString name);
	int GetEndNumber(CString name);
	CString GenerateNewName(CString name);


	CString GetCurrentDirectory();
	CString GetRelativePath(const CString& basePath, const CString& filePath);
	CString GetAbsolutePath(const CString& basePath, const CString& relativePath);
	bool GetRelativePath(const CString& basePath, const CString& filePath, CString& relativePath);
	bool GetAbsolutePath(const CString& basePath, const CString& relativePath, CString& filePath);

	bool FileExist(const CString& fileName);
	bool DirExist(const CString& path);
	ERMsg CreateMultipleDir(const CString& path);
	void GetLongPathName(CString& path);
	void SimplifyPath(CString& path);

	enum TFileName { FILE_TITLE, FILE_NAME, FILE_PATH };
	int GetFilesList(CStringArray& fileNameArray, const CString& filePath, bool fullPath = false, bool bSubDirSearch = false);
	int GetFilesList(CStringArray& fileNameArray, const CString& filePath, int fullPath, bool bSubDirSearch = false);
	int GetDirList(CStringArray& fileNameArray, const CString& filePath, bool fullPath = false, bool bSubDirSearch = false);


	template<class T, class U>
	int FindInArray(const T& theArray, const U& value)
	{
		int nRep = -1;
		INT_PTR nSize = theArray.GetSize();

		for (INT_PTR i = 0; i < nSize&&nRep == -1; i++)
			if (theArray[i] == value)
				nRep = (int)i;

		return nRep;
	}

	template<class T>
	bool IsExactlyEgal(const T& array1, const T& array2)
	{
		bool bRep = true;

		if (array1.GetSize() == array2.GetSize())
		{
			INT_PTR nSize = array1.GetSize();
			for (INT_PTR i = 0; i < nSize; i++)
				if (array1[i] != array2[i])
				{
					bRep = false;
					break;
				}
		}
		else bRep = false;

		return bRep;
	}

	template<class T>
	void ToSomething(const char* pChar, T& idIn)
	{
		T idOut(0);
		unsigned char* pIn = (unsigned char*)pChar;
		unsigned char* pOut = (unsigned char*)&idOut;

		for (int i = 0; i < sizeof(T); i++)
			pOut[i] = pIn[i];

		idIn = idOut;
	}

	inline char ToChar(char* pChar)
	{
		char v = 0;
		ToSomething(pChar, v);
		return v;
	}
	inline short ToShort(char* pChar)
	{
		short v = 0;
		ToSomething(pChar, v);
		return v;
	}
	inline long ToLong(char* pChar)
	{
		long v = 0;
		ToSomething(pChar, v);
		return v;
	}
	inline float ToFloat(char* pChar)
	{
		float v = 0;
		ToSomething(pChar, v);
		return v;
	}
	inline double ToDouble(char* pChar)
	{
		double v = 0;
		ToSomething(pChar, v);
		return v;
	}
	template<class T>
	void ToRawData(T idIn, char* pChar)
	{
		CString tmp;
		//char* pChar = tmp.GetBufferSetLength(sizeof(T));
		char* pIn = (char*)&idIn;

		for (int i = 0; i < sizeof(T); i++)
			pChar[i] = pIn[i];

	}


	CString GetCurrentTimeString();

	CString CoordToCString(double coord, bool bWithFraction = false);
	double CStringToCoord(const CString& coordStr);


	template<class T>
	void Inverce(T& x, T& y)
	{
		T tmp = x;
		x = y;
		y = tmp;
	}

	void EnsurePointOnDisplay(CPoint& pt);
	void EnsureRectangleOnDisplay(CRect& rect);

	bool GetIntArray(const CString& texte, CIntArray& intArray, int maxPos);

	CString RealToCString(double val, int pres = 4);
	CString Int64ToCString(__int64 val, short n = -1, char c = ' ');


	inline CString ToCString(char val) { return Int64ToCString((__int64)val); }
	inline CString ToCString(short val) { return Int64ToCString((__int64)val); }
	inline CString ToCString(int val) { return Int64ToCString((__int64)val); }
	inline CString ToCString(long val) { return Int64ToCString((__int64)val); }
	inline CString ToCString(size_t val) { return Int64ToCString((__int64)val); }
	inline CString ToCString(__int64 val) { return Int64ToCString(val); }
	inline CString ToCString(COLORREF c) { return Int64ToCString(GetRValue(c)) + _T(" ") + Int64ToCString(GetGValue(c)) + _T(" ") + Int64ToCString(GetBValue(c)); }
	inline CString ToCString(float val, int pres = 4) { return RealToCString((double)val, pres); }
	inline CString ToCString(double val, int pres = 4) { return RealToCString(val, pres); }
	inline CString ToCString(CTime val, bool bNumeric = true) { return bNumeric ? ToCString(val.GetTime()) : val.Format("%c"); }

	template <class TYPE, class ARG_TYPE>
	inline  CString ToCString(const CArray<TYPE, ARG_TYPE>& val, const CString& sep = ",")
	{
		CString str;
		for (int i = 0; i < val.GetSize(); i++)
		{
			str += i == 0 ? "" : sep;
			str += ToString(val[i]);
		}

		return str;
	}

	CString ToCString(CRect rect);
	CRect CRectFromCString(const CString& str);

	inline bool ToBool(const CString& str) { return _tstoi(str) != 0; }
	inline char ToChar(const CString& str) { return (char)_tstoi(str); }
	inline short ToShort(const CString& str) { return (short)_tstoi(str); }
	inline int ToInt(const CString& str) { return _tstoi(str); }
	inline __int64 ToInt64(const CString& str) { return _tstoi64(str); }
	inline long ToLong(const CString& str) { return _tstoi(str); }
	inline float ToFloat(const CString& str) { return (float)_tstof(str); }
	inline double ToDouble(const CString& str) { return _tstof(str); }
	inline CTime ToTime(const CString& str) { return CTime(ToInt64(str)); }
	COLORREF ToCOLORREF(const CString& str);


	CString GetExportImageFilter();
	CString GetImportImageFilter();
	CString GetDefaultFilterExt(CString filter, int nFilterIndex);

	CString GetVersionString(const CString& filerPath);
	CString GetCompilationDateString(char* compilation_date);

	inline int StringToInt(const CString& str) { return _tstoi(str); }
	bool StringToReal(const CString& realStr, double& fValue);
	inline bool StringToFloat(const CString& realStr, float& fValue)
	{
		double tmp;
		bool bRep = StringToReal(realStr, tmp);
		fValue = (float)tmp;

		return bRep;
	}
	inline float StringToFloat(const CString& realStr)
	{
		double tmp = 0;
		StringToReal(realStr, tmp);
		return (float)tmp;
	}


	bool StringTokenizerReal(CString& realStr, double& fValue);

	CString GetLastDirName(CString filePath);

	template <class T>
	bool StringTokenizer(CString& realStr, T& value)
	{
		double tmp;
		bool bRep = StringTokenizerReal(realStr, tmp);
		value = (T)tmp;

		return bRep;
	}

	bool StringToInt(const CString& intStr, int& nValue);


	CString GetClipboardText();
	bool SetClipboardText(const CString& str);

	class CStringException : public CException
	{
	public:

		CStringException(CString msg);
		virtual ~CStringException();
	
		virtual BOOL GetErrorMessage(LPTSTR lpszError, UINT nMaxError,PUINT pnHelpContext = NULL) const;
		virtual BOOL GetErrorMessage(LPTSTR lpszError, UINT nMaxError,PUINT pnHelpContext = NULL);
		

	protected:

		CString m_msg;
	};

	//use bGoodSeek to avoid problem when using ReadString, GetPosition and Seek
	class CStdioFileEx : public CStdioFile
	{
	public:
		CStdioFileEx(bool bGoodSeek = false)
		{
			m_bGoodSeek = bGoodSeek;
		}

		CStdioFileEx(LPCTSTR lpszFileName, UINT nOpenFlags, bool bGoodSeek = false) :CStdioFile(lpszFileName, bGoodSeek ? nOpenFlags | CFile::typeBinary : nOpenFlags)
		{
			m_bGoodSeek = bGoodSeek;
		}

		ERMsg Open(LPCTSTR lpszFileName, UINT nOpenFlags);
		BOOL ReadString(CString& rString);
		BOOL ReadText(CString& rString);

		bool GetGookSeek()const { return m_bGoodSeek; }
		void SetGookSeek(bool bGoodSeek) { m_bGoodSeek = bGoodSeek; }
	private:
		bool m_bGoodSeek;
	};


	CStringA UTF16toUTF8(const CStringW& utf16);
	CStringW UTF8toUTF16(const CStringA& utf8);
	//CString UTF8toASCII(const CStringA& utf8);
	//CStringA ASCIIToUTF8(const CString& str);
#ifdef _UNICODE

	inline std::string ToUTF8(const CStringW& utf16) { return std::string((LPCSTR)UtilWin::UTF16toUTF8(utf16)); }
	inline std::string ToUTF8(const std::wstring& utf16) { return std::string(UtilWin::UTF16toUTF8(utf16.c_str())); }
	inline std::string ToUTF8(LPCTSTR utf16) { return std::string((LPCSTR)UtilWin::UTF16toUTF8(utf16)); }
	inline std::string ToUTF8(const CStringA& utf8) { return std::string((LPCSTR)utf8); }
	inline std::string ToUTF8(const std::string& utf8) { return utf8; }


	inline CStringW ToUTF16(const CStringW& utf16) { return utf16; }
	inline CStringW ToUTF16(const CStringA& utf8) { return UtilWin::UTF8toUTF16(utf8); }
	inline CStringW ToUTF16(const std::string& utf8) { return UtilWin::UTF8toUTF16(utf8.c_str()); }


	inline CString Convert(const CStringW& utf16) { return utf16; }
	inline CString Convert(const CStringA& utf8) { return UTF8toUTF16(utf8); }
	inline CString Convert(const std::string& utf8) { return UTF8toUTF16(utf8.c_str()); }
	inline CString Convert(const char* utf8) { return UTF8toUTF16(utf8); }
#else

	inline CString Convert(const CStringA& utf8) { return utf8; }
	inline CString Convert(const CStringW& utf16) { return UTF16toUTF8(utf16); }
	inline CString Convert(const std::string& utf8) { return UTF8toUTF16(utf8.c_str()); }


	inline std::string ToUTF8(const CStringW& utf16) { return UtilWin::UTF16toUTF8(utf16); }
	inline std::string ToUTF8(const CStringA& str) { return utf8 }
	inline std::string ToUTF8(const std::string& utf8) { return utf8; }
#endif


	//CString Crypt(const CString& str);
	//CString Decrypt(const CString& str);

	//BOOL RegisterExtension(CString sExt, CString sDescription);
	inline DWORD GetWindowStyle(HWND hwnd) { return (DWORD)GetWindowLong(hwnd, GWL_STYLE); }
}


class CStringArrayEx : public CStringArray
{
public:

	CStringArrayEx(UINT nId = 0, const CString& token = _T("\r\n\t,;|"))
	{
		if (nId != 0)
			LoadString(nId, token);
	}

	CStringArrayEx(const CString& str, const CString& token = _T("\r\n\t,;|"))
	{
		LoadString(str, token);
	}

	CStringArrayEx(const CStringArray& in);
	CStringArrayEx& operator = (const CStringArray& in);
	CStringArrayEx& operator = (const CStringArrayEx& in);
	bool operator == (const CStringArray& in) const;
	bool operator != (const CStringArray& in) const { return !operator==(in); }

	CString ToString(const CString& sep = _T(","), bool bAddBraquet = true)const;
	void FromString(const CString& str, const CString& sep = _T(","));

	bool LoadString(UINT nId, const CString& token = _T("\r\n\t,;|"));
	void LoadString(const CString& str, const CString& token = _T("\r\n\t,;|"));

	int Find(const CString in, bool bCaseSensitive = true)const;
};

