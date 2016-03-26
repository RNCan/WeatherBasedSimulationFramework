//=========================================================
//	TITLE:		Array extention
//				for WinNT, MSVC 6.0, MFC 6.00
//				Copyright (C) Matrix Baltic Software
//				Vilnius, Lithuania
//	MODULE:		ArrayEx.h
//	PURPOSE:	Interface of the CArrayEx class.
//
//	AUTHOR:		Audrius Vasiliauskas
// 
//	METADATA:		
//
//=========================================================


#ifndef __ARRAY_EX_H__
#define __ARRAY_EX_H__

#if _MSC_VER < 0x0600
	#pragma warning(disable : 4786) //identifier was truncated to '255' characters in the debug information
#endif

#ifndef __AFXTEMPL_H__
	#include <afxtempl.h>
#endif

//#include "XMLite.h"
#include "UI/Common/UtilWin.h"
////////////////////////////////////////////////////////////////////////////
//
// tips & triks. Speed optimization and bug detecting compromise
//

#ifdef _DEBUG
	#define DATA_ACCESS_OPERATOR(i) ((*this)[i]) // for better bug tracking
	#define FAST_ACCESS_OPERATOR(var,i) ((var)[(i)]) // for better bug tracking
#else
	#define DATA_ACCESS_OPERATOR(i) (m_pData[i]) // 10 times faster
	#define FAST_ACCESS_OPERATOR(var,i) ((var).GetData()[(i)]) 
#endif

//
////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////
//
// Special compare functions for Sorted Array
//

/* // this compare function copy from mfc
template<class TYPE, class ARG_TYPE>
AFX_INLINE BOOL AFXAPI CompareElements(const TYPE* pElement1, const ARG_TYPE* pElement2)
{
	ASSERT(AfxIsValidAddress(pElement1, sizeof(TYPE), FALSE));
	ASSERT(AfxIsValidAddress(pElement2, sizeof(ARG_TYPE), FALSE));

	return *pElement1 == *pElement2;
}
*/

template<class TYPE, class ARG_TYPE>
AFX_INLINE BOOL AFXAPI CompareElementsLess(const TYPE* pElement1, const ARG_TYPE* pElement2)
{
	ASSERT(AfxIsValidAddress(pElement1, sizeof(TYPE), FALSE));
	ASSERT(AfxIsValidAddress(pElement2, sizeof(ARG_TYPE), FALSE));

	return ((*pElement1) < (*pElement2));
}

//
////////////////////////////////////////////////////////////////////////////



template <class TYPE, class ARG_TYPE = const TYPE&>
class CArrayEx : public CArray<TYPE, ARG_TYPE>
{
// Construction
public:
	CArrayEx ();
	CArrayEx (const CArrayEx &x);

// Assigment
public:
	CArrayEx &operator = (const CArrayEx &x);

// Comparison 
public:
	BOOL operator <  (const CArrayEx &x) const;
	BOOL operator <= (const CArrayEx &x) const;
	BOOL operator == (const CArrayEx &x) const;
	BOOL operator != (const CArrayEx &x) const;
	BOOL operator >  (const CArrayEx &x) const;
	BOOL operator >= (const CArrayEx &x) const;

// Operator
public:
	operator CArray<TYPE, ARG_TYPE>();
	INT_PTR Find(ARG_TYPE elem)const;

//serialisation
//	void GetXML(LPXNode& pRoot)const;
//	void GetXMLElement(LPXNode& pRoot)const;
//	void SetXML(const LPXNode pRoot);
//	void SetXMLElement(const LPXNode pRoot);
	//static const char* GetXMLFlag();
	
	CString ToString( const CString& sep=",")const;
	void FromString( const CString& str, const CString& sep=",");

	//CString ToXMLString( )const;
	//void FromXMLString( const CString& str);

	static const char* XML_FLAG;
	
};

template <class TYPE, class ARG_TYPE>
inline 
	CArrayEx<TYPE, ARG_TYPE>::CArrayEx ()
{
}

template <class TYPE, class ARG_TYPE>
inline 
	CArrayEx<TYPE, ARG_TYPE>::CArrayEx (const CArrayEx &x)
{
	*this = x;
}

template <class TYPE, class ARG_TYPE>
inline CArrayEx<TYPE, ARG_TYPE> &
	   CArrayEx<TYPE, ARG_TYPE>::operator = (const CArrayEx &x)
{
	if(this != &x)
	{
		Copy(x);
	}

	return *this;
}

template <class TYPE, class ARG_TYPE>
inline BOOL 
	   CArrayEx<TYPE, ARG_TYPE>::operator <  (const CArrayEx &x) const
{
	register INT_PTR i;
	register INT_PTR nSize(GetSize());

	if(nSize != x.GetSize())
	{
		return FALSE;
	}

	for(i = 0;i < nSize;i ++)
	{
		if(!(DATA_ACCESS_OPERATOR(i) < FAST_ACCESS_OPERATOR(x,i)))
		{
			return FALSE;
		}
	}
	
	return TRUE;
}

template <class TYPE, class ARG_TYPE>
inline BOOL 
	   CArrayEx<TYPE, ARG_TYPE>::operator <= (const CArrayEx &x) const
{
	register INT_PTR i;
	register INT_PTR nSize(GetSize());

	if(nSize != x.GetSize())
	{
		return FALSE;
	}

	for(i = 0;i < nSize;i ++)
	{
		if(!(DATA_ACCESS_OPERATOR(i) <= FAST_ACCESS_OPERATOR(x,i)))
		{
			return FALSE;
		}
	}
	
	return TRUE;
}

template <class TYPE, class ARG_TYPE>
inline BOOL 
	   CArrayEx<TYPE, ARG_TYPE>::operator == (const CArrayEx &x) const
{
	register INT_PTR i;
	register INT_PTR nSize(GetSize());

	if(nSize != x.GetSize())
	{
		return FALSE;
	}

	for(i = 0;i < nSize;i ++)
	{
		if(!(DATA_ACCESS_OPERATOR(i) == FAST_ACCESS_OPERATOR(x,i)))
		{
			return FALSE;
		}
	}
	
	return TRUE;
}

template <class TYPE, class ARG_TYPE>
inline BOOL 
	   CArrayEx<TYPE, ARG_TYPE>::operator != (const CArrayEx &x) const
{
	register INT_PTR i;
	register INT_PTR nSize(GetSize());

	if(nSize != x.GetSize())
		return TRUE;


	
	for(i = 0;i < nSize;i ++)
	{
		if((DATA_ACCESS_OPERATOR(i) != FAST_ACCESS_OPERATOR(x,i)))
		{
			return TRUE;
		}
	}

	return FALSE;
}

template <class TYPE, class ARG_TYPE>
inline BOOL 
	   CArrayEx<TYPE, ARG_TYPE>::operator >  (const CArrayEx &x) const
{
	register INT_PTR i;
	register INT_PTR nSize(GetSize());

	if(nSize != x.GetSize())
	{
		return FALSE;
	}

	for(i = 0;i < nSize;i ++)
	{
		if(!(DATA_ACCESS_OPERATOR(i) > FAST_ACCESS_OPERATOR(x,i)))
		{
			return FALSE;
		}
	}
	
	return TRUE;
}

template <class TYPE, class ARG_TYPE>
inline BOOL 
	   CArrayEx<TYPE, ARG_TYPE>::operator >= (const CArrayEx &x) const
{
	register INT_PTR i;
	register INT_PTR nSize(GetSize());

	if(nSize != x.GetSize())
	{
		return FALSE;
	}

	for(i = 0;i < nSize;i ++)
	{
		if(!(DATA_ACCESS_OPERATOR(i) >= FAST_ACCESS_OPERATOR(x,i)))
		{
			return FALSE;
		}
	}
	
	return TRUE;
}

template <class TYPE, class ARG_TYPE>
inline 
	   CArrayEx<TYPE, ARG_TYPE>::operator CArray<TYPE, ARG_TYPE>()
{
	return *this;
}

template <class TYPE, class ARG_TYPE>
inline INT_PTR 
	   CArrayEx<TYPE, ARG_TYPE>::Find(ARG_TYPE elem)const
{
	register INT_PTR pos=-1;
	register INT_PTR nSize(GetSize());

	for (INT_PTR i=0; i<nSize && pos==-1; i++)
	{
		if(DATA_ACCESS_OPERATOR(i) == elem)
			pos = i;
	}

	return pos;
}
//
//template <class TYPE, class ARG_TYPE>
//inline const char* CArrayEx<TYPE, ARG_TYPE>::GetXMLFlag()
//{
//	return XML_FLAG;
//}

//
//template <class TYPE, class ARG_TYPE>
//inline void CArrayEx<TYPE, ARG_TYPE>::GetXML(LPXNode& pRoot)const
//{
//	XNode* pNode = XAppendChild(*this, pRoot);
//	GetXMLElement(pNode);
//}
//
//template <class TYPE, class ARG_TYPE>
//inline void CArrayEx<TYPE, ARG_TYPE>::GetXMLElement(LPXNode& pRoot)const
//{
//	for(INT_PTR i=0; i<GetSize(); i++)
//	{
//		DATA_ACCESS_OPERATOR(i).GetXML(pRoot);
//	}
//}
//
//template <class TYPE, class ARG_TYPE>
//inline void CArrayEx<TYPE, ARG_TYPE>::SetXML(const LPXNode pRoot)
//{
//	ASSERT(pRoot);
//	RemoveAll();
//
//	const LPXNode pNode = pRoot->Select(GetXMLFlag());
//	if( pNode )
//	{
//		SetXMLElement(pNode);
//	}
//}
//
//template <class TYPE, class ARG_TYPE>
//inline void CArrayEx<TYPE, ARG_TYPE>::SetXMLElement(const LPXNode pRoot)
//{
//	ASSERT( pRoot );
//
//	const XNodes nodes = pRoot->GetChilds(TYPE::GetXMLFlag());
//	for(INT_PTR i=0; i<(INT_PTR)nodes.size(); i++)
//	{
//		TYPE elem;
//		elem.SetXML(nodes[i]);
//		Add(elem);
//	}
//}

template <class TYPE, class ARG_TYPE>
inline CString CArrayEx<TYPE, ARG_TYPE>::ToString( const CString& sep)const
{
	CString str;
	if( GetSize() > 0)
	{
		str+="[";
		for(int i=0; i<GetSize(); i++)
		{
			str += i==0?"":sep;
			str += UtilWin::ToCString(GetAt(i));
		}
		str+="]";
	}

	return str;
}

template <class TYPE, class ARG_TYPE>
inline void CArrayEx<TYPE, ARG_TYPE>::FromString( const CString& str, const CString& sep)
{

	#pragma warning(disable : 4244)
	RemoveAll();

	const type_info& info = typeid(TYPE);

	CString elem;
	int pos = 0;

	while( pos!=-1 )
	{
		elem = str.Tokenize(sep+"[]", pos);

		if( !elem.IsEmpty() )
		{
			if( info==typeid(char))
				Add((char)atoi(elem)); 
			else if(info==typeid(bool))
				Add(atoi(elem)!=0);
			else if( info==typeid(short))
				Add((short)atoi(elem)); 
			else if( info==typeid(int))
				Add(atoi(elem)); 
			else if( info==typeid(long))
				Add(atoi(elem));
			else if( info==typeid(__int64))
				Add(_atoi64(elem));
			else if( info==typeid(float))
				Add((float)atof(elem));
			else if( info==typeid(double))
				Add(atof(elem)); 
			else _ASSERTE(false);//Add(elem.FromString(str));
		}
	}
}

typedef CArrayEx<bool, bool>CBoolArrayEx;
typedef CArrayEx<int, int>CIntArrayEx;
typedef CArrayEx<long, long>CLongArrayEx;
typedef CArrayEx<float, float>CFloatArrayEx;
typedef CArrayEx<double, double>CDoubleArrayEx;

	

#endif 