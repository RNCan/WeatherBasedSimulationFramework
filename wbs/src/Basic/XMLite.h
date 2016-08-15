// XMLite.h: interface for the XMLite class.
//
// XMLite : XML Lite Parser Library
// by bro ( Cho,Kyung Min: bro@shinbiro.com ) 2002-10-30
// Microsoft MVP (Visual C++) bro@msmvp.com
// Updates for VS2005, additions for CCInfo, and other fixes hguy@cogniview.com
// 
// History.
// 2002-10-29 : First Coded. Parsing XMLElelement and Attributes.
//              get xml parsed string ( looks good )
// 2002-10-30 : Get Node Functions, error handling ( not completed )
// 2002-12-06 : Helper Funtion string to long
// 2002-12-12 : Entity Helper Support
// 2003-04-08 : Close, 
// 2003-07-23 : add property escape_value. (now no escape on default)
// 2003-10-24 : bugfix) attribute parsing <tag a='1' \r\n/> is now ok
// 2004-03-05 : add branch copy functions
// 2004-06-14 : add _tcseistr/_tcsenistr/_tcsenicmp functions
// 2004-06-14 : now support, XML Document and PI, Comment, CDATA node
// 2004-06-15 : add GetText()/ Find() functions
// 2004-06-15 : add force_parse : now can parse HTML (not-welformed xml)
// 2006-10-08 : GH: multiple fixes for compiling under VS2005, replaced CString with std::string
// 2006-10-18 : GH: added const functions and some helper functions
// 
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLITE_H__786258A5_8360_4AE4_BDAF_2A52F8E1B877__INCLUDED_)
#define AFX_XMLITE_H__786258A5_8360_4AE4_BDAF_2A52F8E1B877__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//#include <windows.h>
#include <vector>
#include <deque>
#include <string>
#include <tchar.h>

struct _tagXMLAttr;
typedef _tagXMLAttr XAttr, *LPXAttr;
typedef std::vector<LPXAttr> XAttrs;

//struct _tagXMLNode;
//typedef _tagXMLNode XNode, *LPXNode;
struct XNode;
typedef XNode* LPXNode;
typedef std::vector<LPXNode> XNodes, *LPXNodes;

struct _tagXMLDocument;
typedef struct _tagXMLDocument XDoc, *LPXDoc;

// Entity Encode/Decode Support
typedef struct _tagXmlEntity
{
	char entity;					// entity ( & " ' < > )
	char ref[10];					// entity reference ( &amp; &quot; etc )
	int ref_len;					// entity reference length
}XENTITY,*LPXENTITY;

typedef struct _tagXMLEntitys : public std::vector<XENTITY>
{
	LPXENTITY GetEntity( int entity );
	LPXENTITY GetEntity( LPCSTR entity );	
	int GetEntityCount( LPCSTR str );
	int Ref2Entity( LPCSTR estr, LPSTR str, int len );
	int Entity2Ref( LPCSTR str, LPSTR estr, int estrlen );
	std::string Ref2Entity( LPCSTR estr );
	std::string Entity2Ref( LPCSTR str );	
	int Ref2Entity( LPCSTR estr, std::string& str, int elen );
	int Entity2Ref( LPCSTR str, std::string& estr, int len );

	_tagXMLEntitys(){};
	_tagXMLEntitys( LPXENTITY entities, int count );
}XENTITYS,*LPXENTITYS;
extern XENTITYS entityDefault;
std::string XRef2Entity( LPCSTR estr );
std::string XEntity2Ref( LPCSTR str );	

typedef enum 
{
	PIE_PARSE_WELFORMED	= 0,
	PIE_ALONE_NOT_CLOSED,
	PIE_NOT_CLOSED,
	PIE_NOT_NESTED,
	PIE_ATTR_NO_VALUE
}PCODE;

// Parse info.
typedef struct _tagParseInfo
{
	bool		trim_value;			// [set] do trim when parse?
	bool		entity_value;		// [set] do convert from reference to entity? ( &lt; -> < )
	LPXENTITYS	entitys;			// [set] entity table for entity decode
	char		escape_value;		// [set] escape value (default '~')
	bool		force_parse;		// [set] force parse even if xml is not welformed

	LPSTR		xml;				// [get] xml source
	bool		error_occur;		// [get] is occurance of error?
	LPSTR		error_pointer;		// [get] error position of xml source
	PCODE		error_code;			// [get] error code
	std::string error_string;		// [get] error string

	LPXDoc		doc;
	_tagParseInfo() { trim_value = false; entity_value = true; force_parse = false; entitys = &entityDefault; xml = NULL; error_occur = false; error_pointer = NULL; error_code = PIE_PARSE_WELFORMED; escape_value = '~'; }
}PARSEINFO,*LPPARSEINFO;
extern PARSEINFO piDefault;

// display optional environment
typedef struct _tagDispOption
{
	bool bAddHeader;		//add <!xml... header
	bool newline;			// newline when new tag
	bool reference_value;	// do convert from entity to reference ( < -> &lt; )
	char value_quotation_mark;	// val="" (default value quotation mark "
	LPXENTITYS	entitys;	// entity table for entity encode

	int tab_base;			// internal usage
	_tagDispOption() { bAddHeader=true; newline = true; reference_value = true; entitys = &entityDefault; tab_base = 0; value_quotation_mark = '"'; }
}DISP_OPT, *LPDISP_OPT;
extern DISP_OPT optDefault;

// XAttr : Attribute Implementation
typedef struct _tagXMLAttr
{
	std::string name;
	std::string	value;
	
	//_tagXMLNode*	parent;
	XNode*	parent;

	std::string GetXML( LPDISP_OPT opt = &optDefault ) const;
	std::string GetValue( LPDISP_OPT opt = &optDefault ) const;
}XAttr, *LPXAttr;

typedef enum
{
	XNODE_ELEMENT,				// general node '<element>...</element>' or <element/>
	XNODE_PI,					// <?xml version="1.0" ?>
	XNODE_COMMENT,				// <!-- comment -->
	XNODE_CDATA,				// <![CDATA[ cdata ]]>
	XNODE_DOC,					// internal virtual root
}NODE_TYPE;

// XMLNode structure
//typedef struct _tagXMLNode
struct XNode
{
	// name and value
	std::string name;
	std::string	value;

	// internal variables
	LPXNode	parent;		// parent node
	XNodes	childs;		// child node
	XAttrs	attrs;		// attributes
	NODE_TYPE type;		// node type 
	LPXDoc	doc;		// document

	// Load/Save XML
	LPSTR	Load( LPCSTR pszXml, LPPARSEINFO pi = &piDefault );
	std::string GetXML( LPDISP_OPT opt = &optDefault ) const;
	std::string GetText( LPDISP_OPT opt = &optDefault ) const;

	// internal load functions
	LPSTR	LoadAttributes( LPCSTR pszAttrs, LPPARSEINFO pi = &piDefault );
	LPSTR	LoadAttributes( LPCSTR pszAttrs, LPCSTR pszEnd, LPPARSEINFO pi = &piDefault );
	LPSTR	LoadProcessingInstrunction( const char* pszXml= "<?xml version=\"1.0\" encoding=\"utf-8\" ?>", LPPARSEINFO pi = &piDefault );
	LPSTR	LoadComment( LPCSTR pszXml, LPPARSEINFO pi = &piDefault ); 
	LPSTR	LoadCDATA( LPCSTR pszXml, LPPARSEINFO pi = &piDefault ); 

	// in own attribute list
	LPXAttr	GetAttr( LPCSTR attrname ); 
	LPCSTR	GetAttrValue( LPCSTR attrname ); 
	XAttrs	GetAttrs( LPCSTR name ); 

	// in one level child nodes
	LPXNode	GetChild( LPCSTR name ); 
	const LPXNode GetChild( LPCSTR name )const;
	//LPCSTR	GetChildValue( LPCSTR name ); 
	const LPCSTR	GetChildValue( LPCSTR name )const;
	std::string	GetChildText( LPCSTR name, LPDISP_OPT opt = &optDefault );
	XNodes	GetChilds( LPCSTR name )const; 
	XNodes	GetChilds(){return childs;}
	const XNodes& GetChilds()const{return childs;}

	LPXAttr GetChildAttr( LPCSTR name, LPCSTR attrname );
	LPCSTR GetChildAttrValue( LPCSTR name, LPCSTR attrname );
	
	LPXNode Select(LPCSTR name );
	const LPXNode Select(LPCSTR name )const;

	// search node
	LPXNode	Find( LPCSTR name );
	const LPXNode Find ( LPCSTR name ) const;
	LPXNode	Find( LPCSTR name, LPCSTR id );
	const LPXNode Find ( LPCSTR name, LPCSTR id ) const;

	// modify DOM 
	int		GetChildCount();
	LPXNode GetChild( XNodes::size_type i );
	XNodes::iterator GetChildIterator( LPXNode node );
	static LPXNode CreateNode( LPCSTR name = NULL, LPCSTR value = NULL );
	static void DeleteNode( LPXNode& pNode );
	LPXNode	AppendChild( LPCSTR name = NULL, LPCSTR value = NULL );
	LPXNode	AppendChild( LPXNode node );
	bool	RemoveChild( LPXNode node );
	LPXNode DetachChild( LPXNode node );

	// node/branch copy
	void	CopyNode( LPXNode node );
	void	CopyBranch( const LPXNode branch );
	void	_CopyBranch( const LPXNode node, LPXDoc	docNew = NULL );
	LPXNode	AppendChildBranch( LPXNode node );

	// modify attribute
	LPXAttr GetAttr( XAttrs::size_type i );
	XAttrs::iterator GetAttrIterator( LPXAttr node );
	LPXAttr CreateAttr( LPCSTR anem = NULL, LPCSTR value = NULL );
	LPXAttr AppendAttr( LPCSTR name, std::string value  ){ return AppendAttr( name , value.c_str()  );}
	LPXAttr AppendAttr( LPCSTR name = NULL, LPCSTR value = NULL );
	LPXAttr	AppendAttr( LPXAttr attr );
	bool	RemoveAttr( LPXAttr attr );
	LPXAttr DetachAttr( LPXAttr attr );

	// operator overloads
	LPXNode operator [] ( int i ) { return GetChild(i); }
	XNode& operator = ( const XNode& node ) { CopyBranch((const LPXNode)&node); return *this; }

	//_tagXMLNode() { parent = NULL; doc = NULL; type = XNODE_ELEMENT; }
	//~_tagXMLNode();
	XNode() { parent = NULL; doc = NULL; type = XNODE_ELEMENT; }
	~XNode();

	void Close();

	
};//XNode, *LPXNode;

// XMLDocument structure
typedef struct _tagXMLDocument : public XNode
{
	PARSEINFO	parse_info;

	_tagXMLDocument() { parent = NULL; doc = this; type = XNODE_DOC; }
	
	LPSTR	Load( LPCSTR pszXml, LPPARSEINFO pi = NULL );
	
	LPXNode	GetRoot();
	const LPXNode GetRoot() const;

	const XDoc& operator = ( const XDoc& doc );

}XDoc, *LPXDoc;

// Helper Funtion
inline long XStr2Int( LPCSTR str, long default_value = 0 )
{
	return ( str && *str ) ? atol(str) : default_value;
}

inline bool XIsEmptyString( const std::string& s )
{
	std::string::const_iterator i = s.begin();
	//while ((i != s.end()) && (_tcschr(" \n\r\t", *s.begin()) != NULL))
	//[Bug fix by RSA 27/06/2012]
	//while ((i != s.end()) && (_tcschr(" \n\r\t", *i) != NULL))
	while ((i != s.end()) && (strchr(" \n\r\t", *i) != NULL))
		i++;

	return (i == s.end());
}

/*class CXMLSerialization
{
public:

	virtual static const char* GetXMLFlag()=0;
	virtual static int GetNbMember()=0;
	virtual static const char* GetMemberName(int i)=0;
	virtual CString GetString(i)const=0;
	virtual void SetString(int i, const CString& str)const=0;

	virtual void GetXML(XNode& root)const;
	virtual void SetXML(XNode& root);
};
*/

template <class T>
LPXNode XAppendChild( T& x, LPXNode& pRoot)
{
	LPXNode pNode = NULL;
	if( pRoot == NULL )
	{
		pNode = pRoot = XNode::CreateNode(x.GetXMLFlag());
	}
	else
	{
		pNode = pRoot->AppendChild( x.GetXMLFlag() );
	}
	
	_ASSERTE(pNode);
	return pNode;
}


template <class T>
LPXNode XGetXML(const T& x, LPXNode& pRoot, bool byAtt=false)
{
	XNode* pNode = XAppendChild(x, pRoot);
	
	for(int i=0; i<T::NB_MEMBER; i++)
	{
		LPXNode pChildNode=NULL;
		std::string memberName(x.GetMemberName(i));
		std::string str = x.GetMember(i, pChildNode);
		if( pChildNode )
		{
			_ASSERTE(!byAtt);
			//pChildNode->name = memberName;//Force name to the name of the member class
			pNode->AppendChild(pChildNode);
		}
		else 
		{
			if( byAtt )
				pNode->AppendAttr(memberName.c_str(), str.c_str() );
			else pNode->AppendChild(memberName.c_str(), str.c_str() );
		}
	}

	return pNode;
}



template <class T>
LPXNode XSetXML(T& x, const LPXNode pRoot, bool byAtt=false)
{
	_ASSERTE(pRoot);
	LPXNode pNode = pRoot->Select(x.GetXMLFlag());
	if( pNode )
	{
		for(int i=0; i<T::NB_MEMBER; i++)
		{
			std::string memberName(x.GetMemberName(i));
			
			if( byAtt )
			{
				LPXAttr pChildAttr = pNode->GetAttr(memberName.c_str());
				if( pChildAttr )
					x.SetMember( i, pChildAttr->value.c_str(), NULL); 
			}
			else
			{
				LPXNode pChildNode = pNode->GetChild(memberName.c_str());

				if( pChildNode )
				{
					x.SetMember( i, pChildNode->value.c_str(), pChildNode); 
				}
			}
		}
	}

	return pNode;
}

extern LPXNode NULL_ROOT;

#include "UtilStd.h"

template <class T>
ERMsg XLoad( const std::string&  filePath, T& x )
{
	ERMsg msg;

	ifStream file;
	
	msg = file.open(filePath);
	if( msg)
	{
		std::string str = file.GetText();
		//file.Read(str.GetBufferSetLength((int)file.GetLength()), (int)file.GetLength());
		//str.ReleaseBuffer();

		XDoc doc;
		doc.Load(str.c_str());
		x.SetXML(&doc);
	}

	return msg;
}

template <class T>
ERMsg XSave( const std::string&  filePath, const T& x, const std::string&  version=""  )
{
	ERMsg msg;

	ofStream file;
	
	msg = file.open(filePath);

	if(msg)
	{
		XDoc doc;
		
		doc.LoadProcessingInstrunction("<?xml version=1.0 encoding=\"utf-8\" ?>");
		LPXNode pRoot = &doc;
		x.GetXML(pRoot);
		if( !version.empty())
			if( pRoot->GetChildCount() == 2)
			pRoot->GetChild(1)->AppendAttr("version", version);
		
		std::string str = doc.GetXML();
		file.write( str.c_str(), (UINT)str.length() );
		
		doc.Close();
	}

	return msg;
}	

#endif // !defined(AFX_XMLITE_H__786258A5_8360_4AE4_BDAF_2A52F8E1B877__INCLUDED_)
