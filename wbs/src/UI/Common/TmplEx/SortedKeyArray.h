//=========================================================
//	TITLE:		Sorted array
//				for WinNT, MSVC 6.0, MFC 6.00
//				Copyright (C) Matrix Baltic Software
//				Vilnius, Lithuania
//	MODULE:	SortedKeyArray.h
//	PURPOSE:	Interface of the CSortedKeyArray class.
//
//	AUTHOR:		Audrius Vasiliauskas
// 
//	METADATA:		TYPE: must have copy constructor, operator =, and operator <
//
//=========================================================


#ifndef __SORTED_KEY_ARRAY_H__
#define __SORTED_KEY_ARRAY_H__

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


#if _MSC_VER < 0x0600
	#pragma warning(disable : 4786) //identifier was truncated to '255' characters in the debug information
#endif

#ifndef __AFXTEMPL_H__
	#include <afxtempl.h>
#endif

#ifndef __SORTED_ARRAY_H__
	#include "SortedArray.h"
#endif

////////////////////////////////////////////////////////////////////////////
//
// Special compare functions for Sorted Key Array
//


template<class KEY, class ARG_KEY, class TYPE, class ARG_TYPE>
AFX_INLINE BOOL AFXAPI CompareElementsKey(const TYPE* pElement1, const ARG_KEY* pElement2)
{
	ASSERT(AfxIsValidAddress(pElement1, sizeof(TYPE), FALSE));
	ASSERT(AfxIsValidAddress(pElement2, sizeof(ARG_KEY), FALSE));

	return *pElement1 == *pElement2;
}


template<class KEY, class ARG_KEY, class TYPE, class ARG_TYPE>
AFX_INLINE BOOL AFXAPI CompareElementsLessKey(const TYPE* pElement1, const ARG_KEY* pElement2)
{
	ASSERT(AfxIsValidAddress(pElement1, sizeof(TYPE), FALSE));
	ASSERT(AfxIsValidAddress(pElement2, sizeof(ARG_KEY), FALSE));

	return ((*pElement1) < (*pElement2));
}

template<class TYPE,class ARG_KEY>
AFX_INLINE INT_PTR __cdecl QSortCompareElementsKey(const TYPE* pElement1, const ARG_KEY* pElement2)
{
	ASSERT(AfxIsValidAddress(pElement1, sizeof(TYPE), FALSE));
	ASSERT(AfxIsValidAddress(pElement2, sizeof(ARG_KEY), FALSE));

   if((*pElement1) == (*pElement2))
   {
      return 0;
   }
	
   if((*pElement1) < (*pElement2))
   {
      return -1;
   }

   return 1;
}

template<class TYPE,class ARG_KEY>
AFX_INLINE INT_PTR __cdecl QSortCompareElementsPointersKey(const TYPE* pElement1, const ARG_KEY* pElement2)
{
	ASSERT(AfxIsValidAddress(pElement1, sizeof(TYPE), FALSE));
	ASSERT(AfxIsValidAddress(pElement2, sizeof(ARG_KEY), FALSE));

   if((**pElement1) == (*pElement2))
   {
      return 0;
   }
	
   if((**pElement1) < (*pElement2))
   {
      return -1;
   }

   return 1;
}
//
////////////////////////////////////////////////////////////////////////////

template<class KEY, class ARG_KEY, class TYPE, class ARG_TYPE>
class CSortedKeyArray : public CSortedArray<TYPE, ARG_TYPE> 
{
// Types
public:
   typedef INT_PTR    (__cdecl *QSortCompareKeyFn)(const TYPE *pElement1, const ARG_KEY *pElement2);

// Atributes
public:

// Construction
public:
	CSortedKeyArray();
	CSortedKeyArray(const CSortedKeyArray &x);
	virtual ~CSortedKeyArray();

// Assigment
public:
	CSortedKeyArray & operator = (const CSortedKeyArray &x);

// Comparison 
public:
	BOOL operator <  (const CSortedKeyArray &x) const;
	BOOL operator <= (const CSortedKeyArray &x) const;
	BOOL operator == (const CSortedKeyArray &x) const;
	BOOL operator != (const CSortedKeyArray &x) const;
	BOOL operator >  (const CSortedKeyArray &x) const;
	BOOL operator >= (const CSortedKeyArray &x) const;

// Operator
public:
	operator CSortedArray<TYPE, ARG_TYPE>();

// Method
public:
	virtual INT_PTR LookupKey(ARG_KEY tValue) const;	// binary search if fail return -1
	virtual INT_PTR SearchKey(ARG_KEY tValue) const;	// binary search if fail return nearest item

	virtual INT_PTR LookupKey(ARG_KEY tValue,QSortCompareKeyFn pCompareFn) const;	   // binary search if fail return -1 
	virtual INT_PTR SearchKey(ARG_KEY tValue,QSortCompareKeyFn pCompareFn) const;	   // binary search if fail return nearest item
   virtual INT_PTR FindKey(ARG_KEY tValue,QSortCompareKeyFn pCompareFn) const;      // linear search for given value

};

// Implementation of the CSortedKeyArray class.

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

template<class KEY, class ARG_KEY, class TYPE, class ARG_TYPE>
inline 
	CSortedKeyArray<KEY, ARG_KEY, TYPE, ARG_TYPE>::CSortedKeyArray ()
{
}

template<class KEY, class ARG_KEY, class TYPE, class ARG_TYPE>
inline 
	CSortedKeyArray<KEY, ARG_KEY, TYPE, ARG_TYPE>::CSortedKeyArray (const CSortedKeyArray &x)
{
	(*this) = x;
}

template<class KEY, class ARG_KEY, class TYPE, class ARG_TYPE>
inline 
	CSortedKeyArray<KEY, ARG_KEY, TYPE, ARG_TYPE>::~CSortedKeyArray ()
{
}

template<class KEY, class ARG_KEY, class TYPE, class ARG_TYPE>
inline CSortedKeyArray<KEY, ARG_KEY, TYPE, ARG_TYPE> &
	   CSortedKeyArray<KEY, ARG_KEY, TYPE, ARG_TYPE>::operator = (const CSortedKeyArray &x)
{
	CSortedArray<TYPE, ARG_TYPE>::operator = (x);

	return (*this);
}

template<class KEY, class ARG_KEY, class TYPE, class ARG_TYPE>
inline INT_PTR 
	CSortedKeyArray<KEY, ARG_KEY, TYPE, ARG_TYPE>::SearchKey(ARG_KEY tValue) const
{	// binary search if fail return nearest item
	INT_PTR nLow = 0, nHigh = GetSize(), nMid;

	while(nLow < nHigh)
	{
		nMid = (nLow + nHigh ) / 2;
		if(CompareElementsLessKey<KEY, ARG_KEY, TYPE, ARG_TYPE>(&(m_pData[nMid]),&tValue))
		{
			nLow = nMid + 1;
		}
		else
		{
			nHigh = nMid;
		}
	}
	return nLow;
}

template<class KEY, class ARG_KEY, class TYPE, class ARG_TYPE>
inline INT_PTR 
	CSortedKeyArray<KEY, ARG_KEY, TYPE, ARG_TYPE>::LookupKey(ARG_KEY tValue) const
{	// binary search if fail return -1
	INT_PTR nRet = SearchKey(tValue);

	if(nRet < GetSize())
	{
		if(CompareElementsKey<KEY, ARG_KEY, TYPE, ARG_TYPE>(&(m_pData[nRet]),&tValue))
		{
			return nRet;
		}
	}

	return -1;
}


template<class KEY, class ARG_KEY, class TYPE, class ARG_TYPE>
inline BOOL 
	CSortedKeyArray<KEY, ARG_KEY, TYPE, ARG_TYPE>::operator <  (const CSortedKeyArray &x) const
{
	return CSortedArray<TYPE, ARG_TYPE>::operator < (x);
}

template<class KEY, class ARG_KEY, class TYPE, class ARG_TYPE>
inline BOOL 
	CSortedKeyArray<KEY, ARG_KEY, TYPE, ARG_TYPE>::operator <= (const CSortedKeyArray &x) const
{
	return CSortedArray<TYPE, ARG_TYPE>::operator <= (x);
}

template<class KEY, class ARG_KEY, class TYPE, class ARG_TYPE>
inline BOOL 
	CSortedKeyArray<KEY, ARG_KEY, TYPE, ARG_TYPE>::operator == (const CSortedKeyArray &x) const
{
	return CSortedArray<TYPE, ARG_TYPE>::operator == (x);
}

template<class KEY, class ARG_KEY, class TYPE, class ARG_TYPE>
inline BOOL 
	CSortedKeyArray<KEY, ARG_KEY, TYPE, ARG_TYPE>::operator != (const CSortedKeyArray &x) const
{
	return CSortedArray<TYPE, ARG_TYPE>::operator != (x);
}

template<class KEY, class ARG_KEY, class TYPE, class ARG_TYPE>
inline BOOL 
	CSortedKeyArray<KEY, ARG_KEY, TYPE, ARG_TYPE>::operator >  (const CSortedKeyArray &x) const
{
	return CSortedArray<TYPE, ARG_TYPE>::operator > (x);
}

template<class KEY, class ARG_KEY, class TYPE, class ARG_TYPE>
inline BOOL 
	CSortedKeyArray<KEY, ARG_KEY, TYPE, ARG_TYPE>::operator >= (const CSortedKeyArray &x) const
{
	return CSortedArray<TYPE, ARG_TYPE>::operator >= (x);
}

template<class KEY, class ARG_KEY, class TYPE, class ARG_TYPE>
inline 
	CSortedKeyArray<KEY, ARG_KEY, TYPE, ARG_TYPE>::operator CSortedArray<TYPE, ARG_TYPE>()
{
	return *this;
}

template<class KEY, class ARG_KEY, class TYPE, class ARG_TYPE>
inline INT_PTR 
	CSortedKeyArray<KEY, ARG_KEY, TYPE, ARG_TYPE>::FindKey(ARG_KEY tValue,QSortCompareKeyFn pCompareFn) const
{
   INT_PTR i;

   for(i = 0;i < GetSize();i ++)
   {
      if(pCompareFn(&(m_pData[i]),&tValue) == 0)
      {
         return i;
      }
   }

   return -1;
}

template<class KEY, class ARG_KEY, class TYPE, class ARG_TYPE>
inline INT_PTR 
	CSortedKeyArray<KEY, ARG_KEY, TYPE, ARG_TYPE>::SearchKey(ARG_KEY tValue,QSortCompareKeyFn pCompareFn) const
{	// binary search if fail return nearest item
	INT_PTR nLow = 0, nHigh = GetSize(), nMid;

	while(nLow < nHigh)
	{
		nMid = (nLow + nHigh ) / 2;
		if(pCompareFn(&(m_pData[nMid]),&tValue) < 0)
		{
			nLow = nMid + 1;
		}
		else
		{
			nHigh = nMid;
		}
	}
	return nLow;
}

template<class KEY, class ARG_KEY, class TYPE, class ARG_TYPE>
inline INT_PTR 
	CSortedKeyArray<KEY, ARG_KEY, TYPE, ARG_TYPE>::LookupKey(ARG_KEY tValue,QSortCompareKeyFn pCompareFn) const
{	// binary search if fail return -1
	INT_PTR nRet = SearchKey(tValue,pCompareFn);

	if(nRet < GetSize())
	{
		if(pCompareFn(&(m_pData[nRet]),&tValue) == 0)
		{
			return nRet;
		}
	}

	return -1;
}

//************************************************************************


template <class T1, class T2>
class CSortedKeyIndex
{
public:

	CSortedKeyIndex(T1 key=-1, T2 index=-1)
	{
		m_key=key;
		m_index=index;
	}


	T1 m_key;
	T2 m_index;
	
	bool operator ==(const CSortedKeyIndex& in)const{ return m_key==in.m_key&&m_index==in.m_index;}
	bool operator <(const CSortedKeyIndex& in)const{ return m_key<in.m_key;}
	//bool operator >(T1 key)const{ return m_key>key;}
	bool operator ==(T1 key)const{ return m_key==key;}
	bool operator <(T1 key)const{ return m_key<key;}
};

typedef CSortedKeyIndex<long, long> CSortedKeyIndexL;

typedef CSortedKeyArray<long, long, CSortedKeyIndexL, const CSortedKeyIndexL&> CSortedKeyIndexLArray;

#endif // !defined(__SORTED_ARRAY_H__)
