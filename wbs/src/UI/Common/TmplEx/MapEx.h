//=========================================================
//	TITLE:		Map extention
//				for WinNT, MSVC 6.0, MFC 6.00
//				Copyright (C) Matrix Baltic Software
//				Vilnius, Lithuania
//	MODULE:		MapEx.h
//	PURPOSE:	Interface of the CMapEx class.
//
//	AUTHOR:		Audrius Vasiliauskas
// 
//	METADATA:		Added special hashkey for string maps
//
//=========================================================

#ifndef __MAP_EX_H__
#define __MAP_EX_H__

#if _MSC_VER < 0x0600
	#pragma warning(disable : 4786) //identifier was truncated to '255' characters in the debug information
#endif

#ifndef __AFXTEMPL_H__
	#include <afxtempl.h>
#endif


////////////////////////////////////////////////////////////////////////////
//
// Special Hashkey for String Maps
//

template<> 
inline UINT AFXAPI HashKey<CString> (CString strKey)
{
	LPCTSTR key = strKey;
	UINT nHash = 0;
	while (*key)
	{
		nHash = (nHash<<5) + nHash + *key++;
	}
	return nHash;
}

template<> 
inline UINT AFXAPI HashKey<CString&> (CString& strKey)
{
	LPCTSTR key = strKey;
	UINT nHash = 0;
	while (*key)
	{
		nHash = (nHash<<5) + nHash + *key++;
	}
	return nHash;
}

template<> 
inline UINT AFXAPI HashKey<const CString&> (const CString& strKey)
{
	LPCTSTR key = strKey;
	UINT nHash = 0;
	while (*key)
	{
		nHash = (nHash<<5) + nHash + *key++;
	}
	return nHash;
}

//
////////////////////////////////////////////////////////////////////////////


template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
class CMapEx : public CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>
{

public:
// Construction
	CMapEx(int nBlockSize = 10);
	CMapEx(const CMapEx &x);

public:
// Assigment
	CMapEx &operator = (const CMapEx& x);

// Accessing elements
	VALUE GetAt(int nIndex) const;
	KEY GetKeyAt(int nIndex) const;

// overwrite CArray methods
	int GetSize( ) const;
};


template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
inline CMapEx<KEY, ARG_KEY, VALUE, ARG_VALUE>::CMapEx(int nBlockSize)
	:CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>(nBlockSize)
{
}


template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
inline CMapEx<KEY, ARG_KEY, VALUE, ARG_VALUE>::CMapEx(const CMapEx &x)
{
	*this = x;
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
inline CMapEx<KEY, ARG_KEY, VALUE, ARG_VALUE> &
	   CMapEx<KEY, ARG_KEY, VALUE, ARG_VALUE>::operator = (const CMapEx& x)
{
	KEY rKey;
	VALUE rValue;
	POSITION pos = x.GetStartPosition();

	RemoveAll();
	while(pos != NULL)
	{
		x.GetNextAssoc(pos,rKey,rValue);
		SetAt(rKey,rValue);
	}
	return *this;
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
inline VALUE CMapEx<KEY, ARG_KEY, VALUE, ARG_VALUE>::GetAt(int nIndex) const
{
	ASSERT(nIndex >= 0 && nIndex < GetCount());
	
	POSITION pos = GetStartPosition();
	KEY rKey;
	VALUE rValue;

	do	
	{
		GetNextAssoc(pos,rKey,rValue);
	}while(nIndex --);

	return rValue;
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
inline KEY CMapEx<KEY, ARG_KEY, VALUE, ARG_VALUE>::GetKeyAt(int nIndex) const
{
	ASSERT(nIndex >= 0 && nIndex < GetCount());
	
	POSITION pos = GetStartPosition();
	KEY rKey;
	VALUE rValue;

	do	
	{
		GetNextAssoc(pos,rKey,rValue);
	}while(nIndex --);

	return rKey;
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
inline int CMapEx<KEY, ARG_KEY, VALUE, ARG_VALUE>::GetSize( ) const
{
	return GetCount();
}


#endif