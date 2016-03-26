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

	//static const TCHAR STRDEFAULT[]= _T("Default");
	//static const char DIR_DELIMITER= '\\';
	//static const float VMISS= (float)WBSF::VMISS;
	//static const char* STRVMISS= WBSF::STRVMISS;


	//static const double EPSILON	= 1.0e-10;
	//static const double FEET2METER = 0.3048;
	//static const double METER2FEET = 1/FEET2METER;

    typedef CArray <bool > CBoolArray;
    typedef CArray < int > CIntArray; 
    typedef CArray < float > CFloatArray;
    typedef CArray < double > CDoubleArray;
	
//	typedef CArrayEx <bool, bool > CBoolArrayEx;
  //  typedef CArrayEx < int, int > CIntArrayEx; 
    //typedef CArrayEx < float, float > CFloatArrayEx;
    //typedef CArrayEx < double, double > CDoubleArrayEx;
    //typedef CArray<CFloatArray, CFloatArray&> CFloatMatrix;
    //typedef CArray<CFloatArray*, CFloatArray*> CFloatMatrix2;
//#ifdef CRect
  //  typedef CArray<CRect, CRect&> CRectArray;
//#endif

    //char* strip(char *line);
    //CString& strip(CString& line);
    int FindStringExact(const CStringArray& nameArray, const CString& nameSearch, bool bCaseSensitive=true);
//	CString FindString(const CString& source, const char* strBegin, const char* strEnd, int& posBegin, int& posEnd);
    
	inline CString GetCString(UINT nId)
	{
		CString str;
		VERIFY( str.LoadString(nId) );
		return str;
	}

    CString& InsertAt(CString& text, int pos, char car);
    CString& RemoveAt(CString& text, int pos, int length=1);
    CString& ReplaceReturnByComa(CString& string);
    CString& ReplaceComaByReturn(CString& string);
    void Purge(CString& cLine);
	//CString GetApplicationPath();
	//CString GetUserDataPath();
	

    
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

	enum TFileName { FILE_TITLE, FILE_NAME, FILE_PATH};
    int GetFilesList(CStringArray& fileNameArray, const CString& filePath, bool fullPath=false, bool bSubDirSearch=false);
	int GetFilesList(CStringArray& fileNameArray, const CString& filePath, int fullPath, bool bSubDirSearch=false);
	int GetDirList(CStringArray& fileNameArray, const CString& filePath, bool fullPath=false, bool bSubDirSearch=false);

	//ERMsg WinExecWait(const CString& command, CString dir=_T(""), UINT uCmdShow = SW_HIDE, LPDWORD pExitCode=NULL );
    //ERMsg CallApplication( CString appType, CString argument, CWnd* pCaller, int showMode=SW_HIDE, bool bAddCote = true, bool bWait=false);
    //ERMsg AskToFindApplication(CString appType, CString& strNamePath, CWnd* pCaller);

    //void SortArray(CStringArray& array);

 //   template<class T, class U>
 //   int GetMinPos(T& array)
 //   {
 //       int pos = -1;
 //       int size = array.GetSize();
 //       if( size > 0)
 //       {
 //           pos = 0;
 //           U min = array[0];
 //       
 //           for(int i=1; i<size; i++)
 //           {
 //               if( array[i] < min)
 //               {
 //                   pos = i;
 //                   min = array[i];
 //               }
 //           }
 //       }

 //       return pos;
 //   }
	//
    template<class T, class U>
    int FindInArray( const T& theArray, const U& value)
    {
	    int nRep = -1;
	    INT_PTR nSize = theArray.GetSize();

	    for (INT_PTR i=0; i<nSize&&nRep==-1; i++)
		    if( theArray[i] == value)
			    nRep = (int)i;

	    return nRep;
    }

 //   template<class T>
 //   void order(T* p, T* q, bool asd)
 //   {
	//	if(asd?(*p > *q):(*p < *q))
	//    {
	//	    T temp = *p;
	//	    *p = *q;
	//	    *q = temp;
	//    }
 //   }

 //   template<class T>
 //   void SortArray(T& theArray, bool asd=true)
 //   {
	//    INT_PTR size = theArray.GetSize();
 //       
	//    for(INT_PTR i=0; i<size-1; i++)
	//		for(INT_PTR j=size-1; i<j; j--)
	//			order( &theArray[j-1], &theArray[j], asd);

 //   }

 //   template<class T>
 //   bool IsEqual(const T& array1, const T& array2)
 //   {
 //       bool bRep = true;
	//    
 //       if( array1.GetSize() == array2.GetSize() )
 //       {
 //           INT_PTR nSize = array1.GetSize();
 //           for(INT_PTR i=0; i<nSize&&bRep; i++)
 //               if( fabs(double(array1[i]) - double(array2[i])) > EPSILON )
 //                   bRep = false;
 //       }
 //       else bRep = false;

 //       return bRep;
 //   }

    template<class T>
    bool IsExactlyEgal(const T& array1, const T& array2)
    {
        bool bRep = true;
	    
        if( array1.GetSize() == array2.GetSize() )
        {
            INT_PTR nSize = array1.GetSize();
            for(INT_PTR i=0; i<nSize; i++)
                if( array1[i] != array2[i] )
                {
                    bRep = false;
                    break;
                }
        }
        else bRep = false;

        return bRep;
    }

 //   template<class T>
 //   void SwitchEndien(T& idIn)
 //   {
 //       T idOut(0);
 //       unsigned char* pIn = (unsigned char*)&idIn;
 //       unsigned char* pOut = (unsigned char*)&idOut;
 //   
 //       for(int i=0; i<sizeof(T); i++) 
 //           pOut[sizeof(T)-i-1] = pIn[i];

 //       idIn = idOut;
 //   }

	//inline void SwitchEndien(CString& str, int nbBits)
 //   {
	//	ASSERT( str.GetLength()%nbBits==0);

	//	CString tmp(str);
	//	unsigned char* pIn = (unsigned char*)str.GetBuffer();
	//	unsigned char* pOut = (unsigned char*)tmp.GetBuffer();

	//	int nbElem = int(str.GetLength()/nbBits);
	//	for(int i=0; i<nbElem; i++)
	//	{
	//		int base = i*nbBits;
 //   
	//        for(int j=0; j<nbBits; j++) 
	//	        pOut[base+nbBits-j-1] = pIn[base+j];

	//		//idIn = idOut;
	//	}

	//	str = tmp;
 //   }
 // 
	template<class T>
	void ToSomething(const char* pChar, T& idIn)
	{
        T idOut(0);
        unsigned char* pIn = (unsigned char*)pChar;
        unsigned char* pOut = (unsigned char*)&idOut;
    
        for(int i=0; i<sizeof(T); i++) 
            pOut[i] = pIn[i];

        idIn = idOut;
	}

	inline char ToChar(char* pChar)
	{
		char v=0;
		ToSomething(pChar, v);
		return v;
	}
	inline short ToShort(char* pChar)
	{
		short v=0;
		ToSomething(pChar, v);
		return v;
	}
	inline long ToLong(char* pChar)
	{
		long v=0;
		ToSomething(pChar, v);
		return v;
	}
	inline float ToFloat(char* pChar)
	{
		float v=0;
		ToSomething(pChar, v);
		return v;
	}
	inline double ToDouble(char* pChar)
	{
		double v=0;
		ToSomething(pChar, v);
		return v;
	}
	template<class T>
	void ToRawData(T idIn, char* pChar)
	{
		CString tmp;
		//char* pChar = tmp.GetBufferSetLength(sizeof(T));
        char* pIn = (char*)&idIn;
    
        for(int i=0; i<sizeof(T); i++) 
            pChar[i] = pIn[i];

	}

    //void SortArray(CIntArray& array);
    //bool SaveArray(CFloatArray& array, const CString& fileName);

    //float FeetToMetre(float fFeet);
    //inline int FeetToMetre(int nFeet);
    //float MetreToFeet(float fMetre);
    //inline int MetreToFeet(int nMetre);

    //void InitRandomize(unsigned rand=0);
    //double GetRandom(const double& min, const double& max);

    //bool IsLeap(int year);
	//int GetNbDay(int year);
	//int GetFirstIndexPerMonth( int month, int year=1999  );
    //int GetNbDayPerMonth(int month, int year=1999);
    
    //int GetJDay(int day, int month, int year=1999);
    //int GetCurrentJulianDay();
	//int GetCurrentYear();
    //void GetDayAndMonth(int julienneDay, int& day, int& month);
    //void GetDayAndMonth(int julienneDay, int year, int& day, int& month);
    //int GetMonth(int juliennDay, int year=1999);
	//bool VerifyValidDay(int day, int month, int year);
	CString GetCurrentTimeString();
	

    //double GetDistance(const double& lat1, const double& lon1, const double& lat2, const double& lon2) ;
    //double GetDistanceTotal(double lat1, double lon1, double lat2, double lon2);

    //struct GlobalVar
    //{
	   // static short DayPerMonth[12];
    //};


 //   inline int FeetToMetre (int nFeet)
 //   {
	//    return (int)FeetToMetre ( (float)nFeet );
 //   }

 //   inline int MetreToFeet(int nMetre)
 //   {
	//    return (int)MetreToFeet( (float)nMetre );
 //   }
 //
 //   inline double GetDegDec(long degree, long minute, double second=0)
 //   {
 //       ASSERT(degree >= -360 && degree <= 360 );
	//    ASSERT(minute >= 0 && minute <= 60);
 //       ASSERT(second >= 0 && second <= 60);
	//	
 //       
 //       int signe = degree<0?-1:1;
	//    
	//    double degDec = degree + (minute*signe)/60.0 + (second*signe)/3600.0;
	//    
	//    return degDec;
 //   }

	//
	////template<class T>
	////int _Signe(const T& a)
	////{
	////	return a>= 0?1:-1;
	////}

	////swes formule sont complexe mais
	////essais de maintenir le bon min
	////meme qnad on a des 4.99999
	//inline long GetDegree(double dec, double mult=1)
	//{
	//	long nbSec = long(dec*3600*mult+WBSF::Signe(dec)*0.5);
	//	return long( nbSec/(3600*mult));
	//}

	//inline long GetMinute(double dec, double mult=1)
	//{
	//	long nbSec = abs(long(dec*3600*mult+WBSF::Signe(dec)*0.5));
	//	return long( (nbSec-abs(GetDegree(dec, mult))*3600*mult)/(60*mult));
	//}
	//inline long GetSecond(double dec)
	//{
	//	long nbSec = abs(long(dec*3600+WBSF::Signe(dec)*0.5));
	//	return long( nbSec-(abs(GetDegree(dec))*3600+GetMinute(dec)*60));
	//}

	//inline double GetSecond(double dec, double mult)
	//{
	//	long nbSec = abs(long(dec*3600*mult+WBSF::Signe(dec)*0.5));
	//	return (nbSec-(abs(GetDegree(dec, mult))*3600*mult+GetMinute(dec, mult)*60*mult))/mult;
	//}

	CString CoordToCString(double coord, bool bWithFraction=false);
	double CStringToCoord(const CString& coordStr);

   // bool IsEntier( double value );

    template<class T>
    void Inverce(T& x, T& y)
    {
        T tmp = x;
        x = y;
        y = tmp;
    }

	void EnsurePointOnDisplay (CPoint& pt);
	void EnsureRectangleOnDisplay (CRect& rect);

    bool GetIntArray(const CString& texte, CIntArray& intArray, int maxPos );
	
	CString RealToCString(double val, int pres=4);
	//CString IntToCString(int val, short n=-1, char c=' ');
	CString Int64ToCString(__int64 val, short n=-1, char c=' ');
	
	
	inline CString ToCString(char val){ return Int64ToCString((__int64)val); }
	inline CString ToCString(short val){ return Int64ToCString((__int64)val); }
	inline CString ToCString(int val){ return Int64ToCString((__int64)val); }
	inline CString ToCString(long val){ return Int64ToCString((__int64)val); }
	inline CString ToCString(size_t val){ return Int64ToCString((__int64)val); }
	inline CString ToCString(__int64 val){ return Int64ToCString(val); }
	inline CString ToCString(COLORREF c){ return Int64ToCString(GetRValue(c)) + _T(" ") + Int64ToCString(GetGValue(c)) + _T(" ") + Int64ToCString(GetBValue(c)); }
	inline CString ToCString(float val, int pres=4){ return RealToCString((double)val, pres);}
	inline CString ToCString(double val, int pres=4){ return RealToCString(val, pres);}
	inline CString ToCString(CTime val, bool bNumeric=true){return bNumeric?ToCString(val.GetTime()):val.Format("%c");}

	template <class TYPE, class ARG_TYPE>
	inline  CString ToCString( const CArray<TYPE, ARG_TYPE>& val, const CString& sep=",")
	{
		CString str;
		for(int i=0; i<val.GetSize(); i++)
		{
			str += i==0?"":sep;
			str += ToString(val[i]);
		}

		return str;
	}
	
	CString ToCString(CRect rect);
	CRect CRectFromCString(const CString& str);

	inline bool ToBool(const CString& str){ return _tstoi(str) != 0; }
	inline char ToChar(const CString& str){ return (char)_tstoi(str); }
	inline short ToShort(const CString& str){ return (short)_tstoi(str); }
	inline int ToInt(const CString& str){ return _tstoi(str); }
	inline __int64 ToInt64(const CString& str){ return _tstoi64(str); }
	inline long ToLong(const CString& str){ return _tstoi(str); }
	inline float ToFloat(const CString& str){ return (float)_tstof(str); }
	inline double ToDouble(const CString& str){ return _tstof(str); }
	inline CTime ToTime(const CString& str){ return CTime(ToInt64(str)); }
	COLORREF ToCOLORREF(const CString& str);
	
	//template <class TYPE, class ARG_TYPE>
	//inline void ToArray( const CString& str, CArray<TYPE, ARG_TYPE>& val, const CString& sep=",")
	//{

	//	#pragma warning(disable : 4244)
	//	val.RemoveAll();

	//	const type_info& info = typeid(TYPE);
	//	//if( info == typeid(char) || info == typeid(short) || info == typeid(long) )
	//	//	tmp = ToString((int)val);
	//	CString elem;
	//	int pos = 0;
	//	while( elem = str.Tokenize(sep, pos) )
	//	{
	//		if( info==typeid(char))
	//			val.Add(ToChar(elem)); 
	//		else if( info==typeid(short))
	//			val.Add(ToShort(elem)); 
	//		else if( info==typeid(int))
	//			val.Add(ToInt(elem)); 
	//		else if( info==typeid(long))
	//			val.Add(ToLong(elem));
	//		else if( info==typeid(__int64))
	//			val.Add(ToInt64(elem));
	//		else if( info==typeid(float))
	//			val.Add(ToFloat(elem));
	//		else if( info==typeid(double))
	//			val.Add(ToDouble(elem)); 
	//		else ASSERT(false);
	//	}
	//}

	CString GetExportImageFilter();
	CString GetImportImageFilter();
	CString GetDefaultFilterExt(CString filter, int nFilterIndex);
	/*template <class T>
    CString ToString(T val, short prec=4)
    {
		CString tmp;

		const type_info& info = typeid(T);
		if( info == typeid(char) || info == typeid(short) || 
			info == typeid(long) )
			tmp = ToString((int)val);
		
        return tmp;
    }*/
    
	

	
	/*template <class T>
	CString ToString(const T& val, int pres=4)
	{
		switch( type_of(T) )
		{
		}

	}*/
    
	CString GetVersionString(const CString& filerPath);
	CString GetCompilationDateString(char* compilation_date);
	
	inline int StringToInt(const CString& str){return _tstoi(str);}
    bool StringToReal(const CString& realStr, double& fValue);
	inline bool StringToFloat(const CString& realStr, float& fValue)
	{
		double tmp;
		bool bRep = StringToReal(realStr, tmp);
		fValue = (float) tmp;

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

    //double ComputeExposition(double lat, float slopePourcent, float aspect);
    //void ComputeSlopeAndAspect(double lat, double exposition, float& slopePourcent, float& aspect);


	//ERMsg OpenFile(CFile& file, LPCTSTR filePath, UINT flag);

	CString GetClipboardText();
	bool SetClipboardText(const CString& str);

	//class CFileEx: public CFile
	//{

	//public:
	//	CFileEx()
	//	{}

	//	CFileEx(LPCTSTR lpszFileName, UINT nOpenFlags):CFile(lpszFileName, nOpenFlags)
	//	{}

	//	ERMsg Open(LPCTSTR lpszFileName, UINT nOpenFlags)
	//	{
	//		return UtilWin::OpenFile(*this, lpszFileName, nOpenFlags);
	//	}
	//};

	
	//use bGoodSeek to avoid problem when using ReadString, GetPosition and Seek
	class CStdioFileEx: public CStdioFile
	{
	public:
		CStdioFileEx(bool bGoodSeek=false)
		{
			m_bGoodSeek=bGoodSeek;
		}

		CStdioFileEx(LPCTSTR lpszFileName, UINT nOpenFlags, bool bGoodSeek=false):CStdioFile(lpszFileName, bGoodSeek?nOpenFlags|CFile::typeBinary:nOpenFlags)
		{			m_bGoodSeek=bGoodSeek;
		}

		ERMsg Open(LPCTSTR lpszFileName, UINT nOpenFlags);
		BOOL ReadString(CString& rString);
		BOOL ReadText(CString& rString);

		bool GetGookSeek()const {return m_bGoodSeek; }
		void SetGookSeek(bool bGoodSeek){ m_bGoodSeek=bGoodSeek; }
	private:
		bool m_bGoodSeek;
	};


	CStringA UTF16toUTF8(const CStringW& utf16);
	CStringW UTF8toUTF16(const CStringA& utf8);
	//CString UTF8toASCII(const CStringA& utf8);
	//CStringA ASCIIToUTF8(const CString& str);
#ifdef _UNICODE
	
	inline std::string ToUTF8(const CStringW& utf16){ return std::string((LPCSTR)UtilWin::UTF16toUTF8(utf16)); }
	inline std::string ToUTF8(const std::wstring& utf16){ return std::string(UtilWin::UTF16toUTF8(utf16.c_str())); }
	inline std::string ToUTF8(LPCTSTR utf16){ return std::string((LPCSTR)UtilWin::UTF16toUTF8(utf16)); }
	inline std::string ToUTF8(const CStringA& utf8){ return std::string((LPCSTR)utf8); }
	inline std::string ToUTF8(const std::string& utf8){ return utf8; }
	
	
	inline CStringW ToUTF16(const CStringW& utf16){ return utf16; }
	inline CStringW ToUTF16(const CStringA& utf8){ return UtilWin::UTF8toUTF16(utf8); }
	inline CStringW ToUTF16(const std::string& utf8){ return UtilWin::UTF8toUTF16(utf8.c_str()); }

	
	inline CString Convert(const CStringW& utf16){ return utf16; }
	inline CString Convert(const CStringA& utf8){ return UTF8toUTF16(utf8); }
	inline CString Convert(const std::string& utf8){ return UTF8toUTF16(utf8.c_str()); }
	inline CString Convert(const char* utf8){ return UTF8toUTF16(utf8); }
#else
	
	inline CString Convert(const CStringA& utf8){ return utf8; }
	inline CString Convert(const CStringW& utf16){ return UTF16toUTF8(utf16); }
	inline CString Convert(const std::string& utf8){ return UTF8toUTF16(utf8.c_str()); }


	inline std::string ToUTF8(const CStringW& utf16){ return UtilWin::UTF16toUTF8(utf16); }
	inline std::string ToUTF8(const CStringA& str){ return utf8 }
	inline std::string ToUTF8(const std::string& utf8){ return utf8; }
#endif


	//CString Crypt(const CString& str);
	//CString Decrypt(const CString& str);

	//BOOL RegisterExtension(CString sExt, CString sDescription);
	inline DWORD GetWindowStyle(HWND hwnd){return (DWORD)GetWindowLong(hwnd, GWL_STYLE);}
}


class CStringArrayEx : public CStringArray
{
public:

	CStringArrayEx(UINT nId=0, const CString& token=_T("\r\n\t,;|"))
	{
		if( nId!= 0)
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
	bool operator != (const CStringArray& in) const{return !operator==(in);}

	CString ToString(const CString& sep = _T(","), bool bAddBraquet = true)const;
	void FromString(const CString& str, const CString& sep = _T(","));

	bool LoadString(UINT nId, const CString& token = _T("\r\n\t,;|"));
	void LoadString(const CString& str, const CString& token = _T("\r\n\t,;|") );

	int Find(const CString in, bool bCaseSensitive=true)const;
};

