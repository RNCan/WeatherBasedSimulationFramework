//=========================================================
//	TITLE:		Sorted array
//				for WinNT, MSVC 6.0, MFC 6.00
//				Copyright (C) Matrix Baltic Software
//				Vilnius, Lithuania
//	MODULE:		SortedArray.h
//	PURPOSE:	Interface of the CSortedArray class.
//
//	AUTHOR:		Audrius Vasiliauskas
// 
//	METADATA:		TYPE: must have copy constructor, operator = , and operator <
//				Operator must be const 
//=========================================================


#ifndef __SORTED_ARRAY_H__
#define __SORTED_ARRAY_H__

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


#if _MSC_VER < 0x0600
	#pragma warning(disable : 4786) //identifier was truncated to '255' characters in the debug information
#endif

#ifndef __AFXTEMPL_H__
	#include <afxtempl.h>
#endif

#ifndef __SWAP_H__
	#include "Swap.h"
#endif

#ifndef __ARRAY_EX_H__
	#include "ArrayEx.h"
#endif

////////////////////////////////////////////////////////////////////////////
//
// Special compare functions for Sorted Array. 
//

template<class TYPE>
AFX_INLINE INT_PTR __cdecl QSortCompareElements(const TYPE* pElement1, const TYPE* pElement2)
{
	ASSERT(AfxIsValidAddress(pElement1, sizeof(TYPE), FALSE));
	ASSERT(AfxIsValidAddress(pElement2, sizeof(TYPE), FALSE));

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

template<class TYPE>
AFX_INLINE INT_PTR __cdecl QSortCompareElementsPointers(const TYPE* pElement1, const TYPE* pElement2)
{
	ASSERT(AfxIsValidAddress(pElement1, sizeof(TYPE), FALSE));
	ASSERT(AfxIsValidAddress(pElement2, sizeof(TYPE), FALSE));

   if((**pElement1) == (**pElement2))
   {
      return 0;
   }
	
   if((**pElement1) < (**pElement2))
   {
      return -1;
   }

   return 1;
}

//
////////////////////////////////////////////////////////////////////////////

template< class TYPE, class ARG_TYPE = const TYPE&> 
class CSortedArray : public CArrayEx<TYPE, ARG_TYPE> 
{
// Types
public:
   typedef INT_PTR    (__cdecl *QSortCompareFn)(const TYPE *pElement1, const TYPE *pElement2);

// Atributes
public:
	INT_PTR m_nCutOff; // variable for sort speed tunning. Recommended range [3..128]


// Construction
public:
	CSortedArray();
	CSortedArray(const CSortedArray &x);
	virtual ~CSortedArray();

// Assigment
public:
	CSortedArray & operator = (const CSortedArray &x);

// Comparison 
public:
	BOOL operator <  (const CSortedArray &x) const;
	BOOL operator <= (const CSortedArray &x) const;
	BOOL operator == (const CSortedArray &x) const;
	BOOL operator != (const CSortedArray &x) const;
	BOOL operator >  (const CSortedArray &x) const;
	BOOL operator >= (const CSortedArray &x) const;

// Operator
public:
	operator CArrayEx<TYPE, ARG_TYPE>();

// Method
public:
	virtual INT_PTR AddSorted(ARG_TYPE tValue);      // insert new element to sorted array and return insertion index
	virtual void Sort();					            // quick sort, use operator ==, <  and template functions CompareElements,CompareElementsLess
	virtual void Reverse();

	virtual INT_PTR Lookup(ARG_TYPE tValue) const;	// binary search if fail return -1 
	virtual INT_PTR Search(ARG_TYPE tValue) const;	// binary search if fail return nearest item

   virtual void Sort(QSortCompareFn pCompareFn);
	virtual INT_PTR Lookup(ARG_TYPE tValue,QSortCompareFn pCompareFn) const;	   // binary search if fail return -1 
	virtual INT_PTR Search(ARG_TYPE tValue,QSortCompareFn pCompareFn) const;	   // binary search if fail return nearest item
   virtual INT_PTR Find(ARG_TYPE tValue,QSortCompareFn pCompareFn) const;      // linear search for given value

// Private sort method
protected:
	void InsertionSort(INT_PTR nLow, INT_PTR nHigh);
	void QuickSort(INT_PTR nLow, INT_PTR nHigh);
};

// Implementation of the CSortedArray class.

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

template <class TYPE, class ARG_TYPE>
inline 
	CSortedArray<TYPE, ARG_TYPE>::CSortedArray ()
{
	m_nCutOff = 64;
}

template <class TYPE, class ARG_TYPE>
inline 
	CSortedArray<TYPE, ARG_TYPE>::CSortedArray (const CSortedArray &x)
{
	*this = x;
}

template <class TYPE, class ARG_TYPE>
inline 
	CSortedArray<TYPE, ARG_TYPE>::~CSortedArray ()
{
}

template <class TYPE, class ARG_TYPE>
inline CSortedArray<TYPE, ARG_TYPE> &
	   CSortedArray<TYPE, ARG_TYPE>::operator = (const CSortedArray &x)
{
	if(this != &x)
	{
		m_nCutOff = x.m_nCutOff;
		Copy(x);
	}

	return *this;
}

template <class TYPE, class ARG_TYPE>
inline INT_PTR 
	CSortedArray<TYPE, ARG_TYPE>::Search(ARG_TYPE tValue) const
{	// binary search if fail return nearest item
	INT_PTR nLow = 0, nHigh = GetSize(), nMid;

	while(nLow < nHigh)
	{
		nMid = (nLow + nHigh ) / 2;
		if(CompareElementsLess(&(m_pData[nMid]),&tValue))
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

template <class TYPE, class ARG_TYPE>
inline INT_PTR 
	CSortedArray<TYPE, ARG_TYPE>::Lookup(ARG_TYPE tValue) const
{	// binary search if fail return -1
	INT_PTR nRet = Search(tValue);

	if(nRet < GetSize())
	{
		if(CompareElements(&(m_pData[nRet]),&tValue))
		{
			return nRet;
		}
	}

	return -1;
}

template <class TYPE, class ARG_TYPE>
inline void 
	CSortedArray<TYPE, ARG_TYPE>::InsertionSort(INT_PTR nLow, INT_PTR nHigh)
{
	INT_PTR i;
	INT_PTR j;
    for(i = nLow + 1;i <= nHigh; i++ )
    {
        TYPE tValue = DATA_ACCESS_OPERATOR(i);

        for(j = i;j > nLow && CompareElementsLess(&tValue,&(DATA_ACCESS_OPERATOR(j - 1)));j --)
		{
            DATA_ACCESS_OPERATOR(j) = DATA_ACCESS_OPERATOR(j - 1);
		}
        DATA_ACCESS_OPERATOR(j) = tValue;
    }
}


// Quicksort: sort first N items in array A
// TYPE: must have copy constructor, operator =, and operator <



template <class TYPE, class ARG_TYPE>
inline void 
	CSortedArray<TYPE, ARG_TYPE>::QuickSort(INT_PTR nLow, INT_PTR nHigh)
{
    if(nLow + m_nCutOff > nHigh )
	{
        InsertionSort(nLow,nHigh);
	}
    else
    {
            //Sort Low, Middle, High
        INT_PTR nMid = (nLow + nHigh) / 2;	// middle value to partition about

        if(CompareElementsLess(&(DATA_ACCESS_OPERATOR(nMid)),&(DATA_ACCESS_OPERATOR(nLow))))
		{
            TMPLEX::Swap(DATA_ACCESS_OPERATOR(nLow),DATA_ACCESS_OPERATOR(nMid));
		}
        if(CompareElementsLess(&(DATA_ACCESS_OPERATOR(nHigh)),&(DATA_ACCESS_OPERATOR(nLow))))
		{
            TMPLEX::Swap(DATA_ACCESS_OPERATOR(nLow),DATA_ACCESS_OPERATOR(nHigh));
		}
        if(CompareElementsLess(&(DATA_ACCESS_OPERATOR(nHigh)),&(DATA_ACCESS_OPERATOR(nMid))))
		{
            TMPLEX::Swap(DATA_ACCESS_OPERATOR(nMid),DATA_ACCESS_OPERATOR(nHigh));
		}

            // Place pivot at Position High-1
        TYPE tPivot = (DATA_ACCESS_OPERATOR(nMid));
        TMPLEX::Swap(DATA_ACCESS_OPERATOR(nMid),DATA_ACCESS_OPERATOR(nHigh - 1));

            // Begin partitioning
        INT_PTR i, j;
        for(i = nLow,j = nHigh - 1; ; )
        {
			// find first element too big to be in low partition
            while(CompareElementsLess(&(DATA_ACCESS_OPERATOR(++i)),&tPivot)) // Symantec may require { }
                ;                     // instead of ;

			// find first element too small to be in high partition
            while(CompareElementsLess(&tPivot,&(DATA_ACCESS_OPERATOR(--j)))) // for both these
                ;                     // loops. It should not.
            if(i < j)
			{
                TMPLEX::Swap(DATA_ACCESS_OPERATOR(i),DATA_ACCESS_OPERATOR(j));
			}
            else
			{
                break;
			}
        }

        // Restore pivot
        TMPLEX::Swap(DATA_ACCESS_OPERATOR(i),DATA_ACCESS_OPERATOR(nHigh - 1));

        QuickSort(nLow,i - 1);   // Sort small elements
        QuickSort(i + 1,nHigh);  // Sort large elements
    }
}

template <class TYPE, class ARG_TYPE>
inline void 
	CSortedArray<TYPE, ARG_TYPE>::Sort()
{
    QuickSort(0,GetSize() - 1);
}

template <class TYPE, class ARG_TYPE>
inline INT_PTR		
	CSortedArray<TYPE, ARG_TYPE>::AddSorted(ARG_TYPE tValue)
{
	INT_PTR nInsertIndex = Search(tValue);

	if(nInsertIndex < GetSize())
	{
		InsertAt(nInsertIndex,tValue);
	}
	else
	{
		nInsertIndex = Add(tValue);
	}

	return nInsertIndex;
}

template <class TYPE, class ARG_TYPE>
inline BOOL 
	CSortedArray<TYPE, ARG_TYPE>::operator <  (const CSortedArray &x) const
{
	return CArrayEx<TYPE, ARG_TYPE>::operator < (x);
}

template <class TYPE, class ARG_TYPE>
inline BOOL 
	CSortedArray<TYPE, ARG_TYPE>::operator <= (const CSortedArray &x) const
{
	return CArrayEx<TYPE, ARG_TYPE>::operator <= (x);
}

template <class TYPE, class ARG_TYPE>
inline BOOL 
	CSortedArray<TYPE, ARG_TYPE>::operator == (const CSortedArray &x) const
{
	return CArrayEx<TYPE, ARG_TYPE>::operator == (x);
}

template <class TYPE, class ARG_TYPE>
inline BOOL 
	CSortedArray<TYPE, ARG_TYPE>::operator != (const CSortedArray &x) const
{
	return CArrayEx<TYPE, ARG_TYPE>::operator != (x);
}

template <class TYPE, class ARG_TYPE>
inline BOOL 
	CSortedArray<TYPE, ARG_TYPE>::operator >  (const CSortedArray &x) const
{
	return CArrayEx<TYPE, ARG_TYPE>::operator > (x);
}

template <class TYPE, class ARG_TYPE>
inline BOOL 
	CSortedArray<TYPE, ARG_TYPE>::operator >= (const CSortedArray &x) const
{
	return CArrayEx<TYPE, ARG_TYPE>::operator >= (x);
}

template <class TYPE, class ARG_TYPE>
inline 
	CSortedArray<TYPE, ARG_TYPE>::operator CArrayEx<TYPE, ARG_TYPE>()
{
	return *this;
}

template <class TYPE, class ARG_TYPE>
inline void 
	CSortedArray<TYPE, ARG_TYPE>::Sort(QSortCompareFn pCompareFn)
{
   typedef int (__cdecl *compare )(const void *elem1,const void *elem2);

   ASSERT(pCompareFn);
   qsort(static_cast<void *>(GetData()),GetSize(),sizeof(TYPE),reinterpret_cast<compare>(pCompareFn));
}

template <class TYPE, class ARG_TYPE>
inline INT_PTR 
	CSortedArray<TYPE, ARG_TYPE>::Find(ARG_TYPE tValue,QSortCompareFn pCompareFn) const
{
   INT_PTR i;
   TYPE tItem = tValue;

   for(i = 0;i < GetSize();i ++)
   {
      if(pCompareFn(&(m_pData[i]),&tItem) == 0)
      {
         return i;
      }
   }

   return -1;
}

template <class TYPE, class ARG_TYPE>
inline INT_PTR 
	CSortedArray<TYPE, ARG_TYPE>::Search(ARG_TYPE tValue,QSortCompareFn pCompareFn) const
{	// binary search if fail return nearest item
	INT_PTR nLow = 0, nHigh = GetSize(), nMid;
   TYPE tItem = tValue;

	while(nLow < nHigh)
	{
		nMid = (nLow + nHigh ) / 2;
		if(pCompareFn(&(m_pData[nMid]),&tItem) < 0)
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

template <class TYPE, class ARG_TYPE>
inline INT_PTR 
	CSortedArray<TYPE, ARG_TYPE>::Lookup(ARG_TYPE tValue,QSortCompareFn pCompareFn) const
{	// binary search if fail return -1
	INT_PTR nRet = Search(tValue,pCompareFn);
   TYPE tItem = tValue;

	if(nRet < GetSize())
	{
		if(pCompareFn(&(m_pData[nRet]),&tItem) == 0)
		{
			return nRet;
		}
	}

	return -1;
}

template <class TYPE, class ARG_TYPE>
inline void 
	CSortedArray<TYPE, ARG_TYPE>::Reverse()
{
	for(INT_PTR i=0; i<GetSize()/2; i++)
	{
		TMPLEX::Swap(DATA_ACCESS_OPERATOR(i),DATA_ACCESS_OPERATOR(GetSize()-i-1));
	}
}

//************************************************************************

template <class T1, class T2>
class CSortedIndex
{
public:

	CSortedIndex(T1 index=0, T2 priority=0)
	{
		m_index=index;
		m_priority=priority;
	}


	T1 m_index;
	T2 m_priority;

	bool operator ==(const CSortedIndex& in)const{ return m_index==in.m_index && m_priority == in.m_priority; }
	bool operator !=(const CSortedIndex& in)const{ return !operator==(in); }
	bool operator >(const CSortedIndex& in)const{ return m_priority > in.m_priority; }
	bool operator >=(const CSortedIndex& in)const{ return m_priority >= in.m_priority; }
	bool operator <(const CSortedIndex& in)const{ return m_priority < in.m_priority; }
	bool operator <=(const CSortedIndex& in)const{ return m_priority <= in.m_priority; }
};

//template <class T1, class T2>
//class CSortedIndexArray: public CSortedArray<CSortedIndex<T1,T2> > 
//{
//};


typedef CSortedIndex<long, double> CSortedIndexD;
typedef CSortedIndex<long, long> CSortedIndexL;
typedef CSortedIndex<double, double> CSortedValue;

typedef CSortedArray<CSortedIndexL, const CSortedIndexL&> CSortedIndexLArray;
typedef CSortedArray<CSortedIndexD, const CSortedIndexD&> CSortedIndexDArray;
typedef CSortedArray<CSortedValue, const CSortedValue&> CSortedValueArray;



#endif // !defined(__SORTED_ARRAY_H__)
