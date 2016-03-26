//=========================================================
//	TITLE:		List extention
//				for WinNT, MSVC 6.0, MFC 6.00
//				Copyright (C) Matrix Baltic Software
//				Vilnius, Lithuania
//	MODULE:		ListEx.h
//	PURPOSE:	Interface of the CListEx class.
//
//	AUTHOR:		Audrius Vasiliauskas
// 
//	METADATA:		
//
//=========================================================


#ifndef __LIST_EX_H__
#define __LIST_EX_H__

#ifndef __AFXTEMPL_H__
	#include <afxtempl.h>
#endif

////////////////////////////////////////////////////////////////////////////
//
// Special copy functions for list
//
/*
template<class TYPE>
AFX_INLINE void AFXAPI CopyElements(TYPE* pDest, const TYPE* pSrc, int nCount)
{
	ASSERT(nCount == 0 ||
		AfxIsValidAddress(pDest, nCount * sizeof(TYPE)));
	ASSERT(nCount == 0 ||
		AfxIsValidAddress(pSrc, nCount * sizeof(TYPE)));

	// default is element-copy using assignment
	while (nCount--)
		*pDest++ = *pSrc++;
}
*/

//
////////////////////////////////////////////////////////////////////////////

template< class TYPE, class ARG_TYPE>
class CListEx : public CList<TYPE, ARG_TYPE>
{
// Construction
public:
	CListEx (int nBlockSize = 10);
	CListEx (const CListEx &x);

// Assigment
public:
	CListEx &operator = (const CListEx &x);

};

template< class TYPE, class ARG_TYPE>
inline
	CListEx<TYPE,ARG_TYPE>::CListEx(int nBlockSize)
	:	CList<TYPE,ARG_TYPE>(nBlockSize)
{
}

template< class TYPE, class ARG_TYPE>
inline
	CListEx<TYPE,ARG_TYPE>::CListEx(const CListEx &x)
{
	*this = x;
}

template< class TYPE, class ARG_TYPE>
inline CListEx<TYPE,ARG_TYPE> &
	CListEx<TYPE,ARG_TYPE>::operator = (const CListEx &x)
{
	if(this != &x)
	{
		TYPE tDst;
		TYPE tSrc;
		POSITION pos;

		RemoveAll();
		pos = x.GetHeadPosition();
		while(pos != NULL)
		{
			tSrc = x.GetNext(pos);
			CopyElements<TYPE>(&tDst,&tSrc,1);
			AddTail(tDst);
		}
	}

	return *this;
}

#endif 

