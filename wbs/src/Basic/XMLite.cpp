// XMLite.cpp: implementation of the XMLite class.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "XMLite.h"
#include <iostream>
#include <sstream>
#include <string>

#pragma warning (disable: 4996)

XNode GLOBAL_NULL_NODE;
LPXNode NULL_ROOT=&GLOBAL_NULL_NODE; 



static const char chXMLTagOpen		= '<';
static const char chXMLTagClose	= '>';
static const char chXMLTagPre	= '/';
static const char chXMLEscape = '\\';	// for value field escape

static const char szXMLPIOpen[] = "<?";
static const char szXMLPIClose[] = "?>";
static const char szXMLCommentOpen[] = "<!--";
static const char szXMLCommentClose[] = "-->";
static const char szXMLCDATAOpen[] = "<![CDATA[";
static const char szXMLCDATAClose[] = "]]>";

static const XENTITY x_EntityTable[] = {
		{ '&', "&amp;", 5 } ,
		{ '\"',"&quot;", 6 } ,
		{ '\'',"&apos;", 6 } ,
		{ '<', "&lt;", 4 } ,
		{ '>', "&gt;", 4 },
		{ '�', "&#176;", 6 },
		{ '�', "&#178;", 6 }
	};


PARSEINFO piDefault;
DISP_OPT optDefault;
XENTITYS entityDefault((LPXENTITY)x_EntityTable, sizeof(x_EntityTable)/sizeof(x_EntityTable[0]) );
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//========================================================
// Name   : _tcschrs
// Desc   : same with strpbrk 
// Param  :
// Return :
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
LPSTR _tcschrs( LPCSTR psz, LPCSTR pszchs )
{
	while( psz && *psz )
	{
		if( strchr( pszchs, *psz ) )
			return (LPSTR)psz;
		psz++;
	}
	return NULL;
}

//========================================================
// Name   : _tcsskip
// Desc   : skip space
// Param  : 
// Return : skiped string
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
LPSTR _tcsskip( LPCSTR psz )
{
	//while( psz && *psz == ' ' && *psz == 13 && *psz == 10 ) psz++;
	while( psz && isspace(*psz) ) psz++;
		
	return (LPSTR)psz;
}

//========================================================
// Name   : _tcsechr
// Desc   : similar with strchr with escape process
// Param  : escape - will be escape character
// Return : 
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
LPSTR _tcsechr( LPCSTR psz, int ch, int escape )
{
	LPSTR pch = (LPSTR)psz;

	while( pch && *pch )
	{
		if( escape != 0 && *pch == escape )
			pch++;
		else
		if( *pch == ch ) 
			return (LPSTR)pch;
		pch++;
	}
	return pch;
}

//========================================================
// Name   : _tcselen
// Desc   : similar with strlen with escape process
// Param  : escape - will be escape character
// Return : 
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
int _tcselen( int escape, LPSTR srt, LPSTR end = NULL ) 
{
	int len = 0;
	LPSTR pch = srt;
	if( end==NULL ) end = (LPSTR)sizeof(long);
	LPSTR prev_escape = NULL;
	while( pch && *pch && pch<end )
	{
		if( escape != 0 && *pch == escape && prev_escape == NULL )
			prev_escape = pch;
		else
		{
			prev_escape = NULL;
			len++;
		}
		pch++;
	}
	return len;
}

//========================================================
// Name   : _tcsecpy
// Desc   : similar with _tcscpy with escape process
// Param  : escape - will be escape character
// Return : 
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
void _tcsecpy( std::string* pString, int escape, LPSTR srt, LPSTR end = NULL )
{
	*pString = "";
	LPSTR pch = srt;
	if( end==NULL ) end = (LPSTR)sizeof(long);
	LPSTR prev_escape = NULL;
	while( pch && *pch && pch<end )
	{
		if( escape != 0 && *pch == escape && prev_escape == NULL )
			prev_escape = pch;
		else
		{
			prev_escape = NULL;
			*pString += *pch;
		}

		pch++;
	}
}

//========================================================
// Name   : _tcsepbrk
// Desc   : similar with strpbrk with escape process
// Param  : escape - will be escape character
// Return : 
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
LPSTR _tcsepbrk( LPCSTR psz, LPCSTR chset, int escape )
{
	LPSTR pch = (LPSTR)psz;
	LPSTR prev_escape = NULL;
	while( pch && *pch )
	{
		if( escape != 0 && *pch == escape && prev_escape == NULL )
			prev_escape = pch;
		else
		{
			prev_escape = NULL;
			if( strchr( chset, *pch ) )
				return (LPSTR)pch;		
		}
		pch++;
	}
	return pch;
}

//========================================================
// Name   : _tcsenicmp
// Desc   : similar with strnicmp with escape process
// Param  : escape - will be escape character
// Return : 
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
int _tcsenicmp( LPCSTR psz, LPCSTR str, int len, int escape )
{
	LPSTR pch = (LPSTR)psz;
	LPSTR prev_escape = NULL;
	LPSTR des = (LPSTR)str;
	int i = 0;
	
	while( pch && *pch && i < len )
	{
		if( escape != 0 && *pch == escape && prev_escape == NULL )
			prev_escape = pch;
		else
		{
			prev_escape = NULL;
			if( tolower(*pch) != tolower(des[i]) )
				break;
			i++;
		}
		pch ++;
	}
	
	// find
	if( i == len )
		return 0;
	if( psz[i] > des[i] )
		return 1;
	return -1;
}

//========================================================
// Name   : _tcsenistr
// Desc   : similar with _tcsistr with escape process
// Param  : escape - will be escape character
// Return : 
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
LPSTR _tcsenistr( LPCSTR psz, LPCSTR str, int len, int escape )
{
	LPSTR pch = (LPSTR)psz;
	LPSTR prev_escape = NULL;
	LPSTR des = (LPSTR)str;
	int i = 0;
	
	while( pch && *pch )
	{
		if( escape != 0 && *pch == escape && prev_escape == NULL )
			prev_escape = pch;
		else
		{
			prev_escape = NULL;
			if( _tcsenicmp( pch, str, len, escape ) == 0 )
				return (LPSTR)pch;
		}
		pch++;
	}
	return pch;
}

//========================================================
// Name   : _tcseistr
// Desc   : similar with _tcsistr with escape process
// Param  : escape - will be escape character
// Return : 
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
LPSTR _tcseistr( LPCSTR psz, LPCSTR str, int escape )
{
	int len = (int)strlen( str );
	return _tcsenistr( psz, str, len, escape );
}

//========================================================
// Name   : _SetString
// Desc   : put string of (psz~end) on ps string
// Param  : trim - will be trim?
// Return : 
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
void _SetString( LPSTR psz, LPSTR end, std::string* ps, bool trim = FALSE, int escape = 0 )
{
	//trim
	if( trim )
	{
		while( psz && psz < end && _istspace(*psz) ) psz++;
		while( (end-1) && psz < (end-1) && _istspace(*(end-1)) ) end--;
	}
	int len = (int)(end - psz);
	if( len <= 0 ) return;
	if( escape )
	{
		len = _tcselen( escape, psz, end );
		_tcsecpy( ps, escape, psz, end );
	}
	else
	{
		ps->assign(psz, len);
	}

    // [BUGFIX] newline bug fix on GetXML()
    if(XIsEmptyString(*ps))
		ps->erase();

}
/*
void _SetString( LPSTR psz, LPSTR end, CString* ps, bool trim = FALSE, int escape = 0 )
{
    //trim
    if( trim )
    {
        while( psz && psz < end && _istspace(*psz) ) psz++;
        while( (end-1) && psz < (end-1) && _istspace(*(end-1)) ) end--;
    }
    int len = end - psz;
    if( len <= 0 ) return;
    if( escape )
    {
        len = _tcselen( escape, psz, end );
        LPSTR pss = ps->GetBufferSetLength( len );
        _tcsecpy( pss, escape, psz, end );
    }
    else
    {
        LPSTR pss = ps->GetBufferSetLength(len + 1 );
        memcpy( pss, psz, len );
        pss[len] = '\0';
    }

        // [BUGFIX] newline bug fix on GetXML()
    if(XIsEmptyString(*ps))
        ps->Empty();
} 
*/

XNode::~XNode()
{
	Close();
}

void XNode::Close()
{
	for( XNodes::size_type iChild = 0 ; iChild < childs.size(); iChild++)
	{
		LPXNode p = childs[iChild];
		if( p )
		{
			delete p; childs[iChild] = NULL;
		}
	}
	childs.clear();
	
	for( XAttrs::size_type iAttr = 0 ; iAttr < attrs.size(); iAttr++)
	{
		LPXAttr p = attrs[iAttr];
		if( p )
		{
			delete p; attrs[iAttr] = NULL;
		}
	}
	attrs.clear();
}
	
// attr1="value1" attr2='value2' attr3=value3 />
//                                            ^- return pointer
//========================================================
// Name   : LoadAttributes
// Desc   : loading attribute plain xml text
// Param  : pszAttrs - xml of attributes
//          pi = parser information
// Return : advanced string pointer. (error return NULL)
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
LPSTR XNode::LoadAttributes( LPCSTR pszAttrs , LPPARSEINFO pi /*= &piDefault*/)
{
	LPSTR xml = (LPSTR)pszAttrs;

	while( xml && *xml )
	{
		if( xml = _tcsskip( xml ) )
		{
			// close tag
			if( *xml == chXMLTagClose || *xml == chXMLTagPre )
				// wel-formed tag
				return xml;

			// XML Attr Name
			char* pEnd = strpbrk( xml, " =" );
			if( pEnd == NULL ) 
			{
				// error
				if( pi->error_occur == false ) 
				{
					pi->error_occur = true;
					pi->error_pointer = xml;
					pi->error_code = PIE_ATTR_NO_VALUE;
					char err[1024];
					sprintf(err, "<%.768s> attribute has error ", name.c_str() );
					pi->error_string = err;
				}
				return NULL;
			}
			
			LPXAttr attr = new XAttr;
			attr->parent = this;

			// XML Attr Name
			_SetString( xml, pEnd, &attr->name );
			
			// add new attribute
			attrs.push_back( attr );
			xml = pEnd;
			
			// XML Attr Value
			if( xml = _tcsskip( xml ) )
			{
				//if( xml = strchr( xml, '=' ) )
				if( *xml == '=' )
				{
					if( xml = _tcsskip( ++xml ) )
					{
						// if " or '
						// or none quote
						int quote = *xml;
						if( quote == '"' || quote == '\'' )
							pEnd = _tcsechr( ++xml, quote, chXMLEscape );
						else
						{
							//attr= value> 
							// none quote mode
							//pEnd = _tcsechr( xml, ' ', '\\' );
							pEnd = _tcsepbrk( xml, " >", chXMLEscape );
						}

						bool trim = pi->trim_value;
						char escape = pi->escape_value;
						//_SetString( xml, pEnd, &attr->value, trim, chXMLEscape );	
						_SetString( xml, pEnd, &attr->value, trim, escape );
						xml = pEnd;
						// ATTRVALUE 
						if( pi->entity_value && pi->entitys )
							attr->value = pi->entitys->Ref2Entity(attr->value.c_str());

						if( quote == '"' || quote == '\'' )
							xml++;
					}
				}
			}
		}
	}

	// not wel-formed tag
	return NULL;
}

// attr1="value1" attr2='value2' attr3=value3 />
//                                            ^- return pointer
//========================================================
// Name   : LoadAttributes
// Desc   : loading attribute plain xml text
// Param  : pszAttrs - xml of attributes
//          pszEnd - last string
//          pi = parser information
// Return : advanced string pointer. (error return NULL)
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2004-06-14
//========================================================
LPSTR XNode::LoadAttributes( LPCSTR pszAttrs, LPCSTR pszEnd, LPPARSEINFO pi /*= &piDefault*/ )
{
	LPSTR xml = (LPSTR)pszAttrs;

	while( xml && *xml )
	{
		if( xml = _tcsskip( xml ) )
		{
			// close tag
			if( xml >= pszEnd )
				// wel-formed tag
				return xml;

			// XML Attr Name
			char* pEnd = strpbrk( xml, " =" );
			if( pEnd == NULL ) 
			{
				// error
				if( pi->error_occur == false ) 
				{
					pi->error_occur = true;
					pi->error_pointer = xml;
					pi->error_code = PIE_ATTR_NO_VALUE;
					char err[1024];
					sprintf(err, "<%.768s> attribute has error ", name.c_str() );
					pi->error_string = err;
				}
				return NULL;
			}
			
			LPXAttr attr = new XAttr;
			attr->parent = this;

			// XML Attr Name
			_SetString( xml, pEnd, &attr->name );
			
			// add new attribute
			attrs.push_back( attr );
			xml = pEnd;
			
			// XML Attr Value
			if( xml = _tcsskip( xml ) )
			{
				//if( xml = strchr( xml, '=' ) )
				if( *xml == '=' )
				{
					if( xml = _tcsskip( ++xml ) )
					{
						// if " or '
						// or none quote
						int quote = *xml;
						if( quote == '"' || quote == '\'' )
							pEnd = _tcsechr( ++xml, quote, chXMLEscape );
						else
						{
							//attr= value> 
							// none quote mode
							//pEnd = _tcsechr( xml, ' ', '\\' );
							pEnd = _tcsepbrk( xml, " >", chXMLEscape );
						}

						bool trim = pi->trim_value;
						char escape = pi->escape_value;
						//_SetString( xml, pEnd, &attr->value, trim, chXMLEscape );	
						_SetString( xml, pEnd, &attr->value, trim, escape );
						xml = pEnd;
						// ATTRVALUE 
						if( pi->entity_value && pi->entitys )
							attr->value = pi->entitys->Ref2Entity(attr->value.c_str());

						if( quote == '"' || quote == '\'' )
							xml++;
					}
				}
			}
		}
	}

	// not wel-formed tag
	return NULL;
}

// <?xml version="1.0"?>
//                      ^- return pointer
//========================================================
// Name   : LoadProcessingInstrunction
// Desc   : loading processing instruction
// Param  : pszXml - PI string
//          pi - parser information
// Return : advanced string pointer. (error return NULL)
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2004-06-14
//========================================================
LPSTR XNode::LoadProcessingInstrunction( LPCSTR pszXml, LPPARSEINFO pi /*= &piDefault*/ )
{
	// find the end of pi
	LPSTR end = _tcsenistr( pszXml, szXMLPIClose, sizeof(szXMLPIClose)-1, pi ? pi->escape_value : 0 );
	if( end == NULL )
		return NULL;

	// process pi
	if( doc )
	{
		LPSTR xml = (LPSTR)pszXml;

		LPXNode node = new XNode;
		node->parent = this;
		node->doc = doc;
		node->type = XNODE_PI;
		
		xml += sizeof(szXMLPIOpen)-1;
		char* pTagEnd = strpbrk( xml, " ?>" );
		_SetString( xml, pTagEnd, &node->name );
		xml = pTagEnd;
		
		node->LoadAttributes( xml, end, pi );

		doc->childs.push_back( node );
	}

	end += sizeof(szXMLPIClose)-1;
	return end;
}

// <!-- comment -->
//                 ^- return pointer
//========================================================
// Name   : LoadComment
// Desc   : loading comment
// Param  : pszXml - comment string
//          pi - parser information
// Return : advanced string pointer. (error return NULL)
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2004-06-14
//========================================================
LPSTR XNode::LoadComment( LPCSTR pszXml, LPPARSEINFO pi /*= &piDefault*/ )
{
	// find the end of comment
	LPSTR end = _tcsenistr( pszXml, szXMLCommentClose, sizeof(szXMLCommentClose)-1, pi ? pi->escape_value : 0 );
	if( end == NULL )
		return NULL;

	// process comment
	LPXNode par = parent;
	if( parent == NULL && doc )
		par = (LPXNode)&doc;
	if( par )
	{
		LPSTR xml = (LPSTR)pszXml;
		xml += sizeof(szXMLCommentOpen)-1;
		
		LPXNode node = new XNode;
		node->parent = this;
		node->doc = doc;
		node->type = XNODE_COMMENT;
		node->name = "#COMMENT";
		_SetString( xml, end, &node->value, FALSE );

		par->childs.push_back( node );
	}

	end += sizeof(szXMLCommentClose)-1;
	return end;
}

// <![CDATA[ cdata ]]>
//                    ^- return pointer
//========================================================
// Name   : LoadCDATA
// Desc   : loading CDATA
// Param  : pszXml - CDATA string
//          pi - parser information
// Return : advanced string pointer. (error return NULL)
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2004-06-14
//========================================================
LPSTR XNode::LoadCDATA( LPCSTR pszXml, LPPARSEINFO pi /*= &piDefault*/ )
{
	// find the end of CDATA
	LPSTR end = _tcsenistr( pszXml, szXMLCDATAClose, sizeof(szXMLCDATAClose)-1, pi ? pi->escape_value : 0 );
	if( end == NULL )
		return NULL;

	// process CDATA
	LPXNode par = parent;
	if( parent == NULL && doc )
		par = (LPXNode)&doc;
	if( par )
	{
		LPSTR xml = (LPSTR)pszXml;
		xml += sizeof(szXMLCDATAOpen)-1;
		
		LPXNode node = new XNode;
		node->parent = this;
		node->doc = doc;
		node->type = XNODE_CDATA;
		node->name = "#CDATA";
		_SetString( xml, end, &node->value, FALSE );

		par->childs.push_back( node );
	}

	end += sizeof(szXMLCDATAClose)-1;
	return end;
}

//========================================================
// Name   : LoadOtherNodes
// Desc   : internal function for loading PI/CDATA/Comment
// Param  : node - current xml node
//          pbRet - error occur
//          pszXml - CDATA string
//          pi - parser information
// Return : advanced string pointer. (error return NULL)
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2004-06-14
//========================================================
LPSTR LoadOtherNodes( LPXNode node, bool* pbRet, LPCSTR pszXml, LPPARSEINFO pi /*= &piDefault*/ )
{
	LPSTR xml = (LPSTR)pszXml;
	bool do_other_type = true;
	*pbRet = false;

	while( xml && do_other_type )
	{
		do_other_type = false;

		xml = _tcsskip( xml );
		LPSTR prev = xml;
		// is PI( Processing Instruction ) Node?
		if( strnicmp( xml, szXMLPIOpen, sizeof(szXMLPIOpen)-1 ) == 0 )
		{
			// processing instrunction parse
			// return pointer is next node of pi
			xml = node->LoadProcessingInstrunction( xml, pi );
			//if( xml == NULL )
			//	return NULL;
			// restart xml parse
		}

		if( xml != prev )
			do_other_type = true;
		xml = _tcsskip( xml );
		prev = xml;

		// is comment Node?
		if( strnicmp( xml, szXMLCommentOpen, sizeof(szXMLCommentOpen)-1 ) == 0 )
		{
			// processing comment parse
			// return pointer is next node of comment
			xml = node->LoadComment( xml, pi );
			// comment node is terminal node
			if( node->parent && node->parent->type != XNODE_DOC 
				&& xml != prev )
			{
				*pbRet = true;
				return xml;
			}
			// restart xml parse when this node is root doc node
		}

		if( xml != prev )
			do_other_type = true;

		xml = _tcsskip( xml );
		prev = xml;
		// is CDATA Node?
		if( strnicmp( xml, szXMLCDATAOpen, sizeof(szXMLCDATAOpen)-1 ) == 0 )
		{
			// processing CDATA parse
			// return pointer is next node of CDATA
			xml = node->LoadCDATA( xml, pi );
			// CDATA node is terminal node
			if( node->parent && node->parent->type != XNODE_DOC 
				&& xml != prev )
			{
				*pbRet = true;
				return xml;
			}
			// restart xml parse when this node is root doc node
		}

		if( xml != prev )
			do_other_type = true;
	}

	return xml;
}

// <TAG attr1="value1" attr2='value2' attr3=value3 >
// </TAG>
// or
// <TAG />
//        ^- return pointer
//========================================================
// Name   : Load
// Desc   : load xml plain text
// Param  : pszXml - plain xml text
//          pi = parser information
// Return : advanced string pointer  (error return NULL)
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
LPSTR XNode::Load( LPCSTR pszXml, LPPARSEINFO pi /*= &piDefault*/ )
{
	// Close it
	Close();

	LPSTR xml = (LPSTR)pszXml;

	xml = strchr( xml, chXMLTagOpen );
	if( xml == NULL )
		return NULL;

	// Close Tag
	if( *(xml+1) == chXMLTagPre ) // </Close
		return xml;

	// Load Other Node before <Tag>(pi, comment, CDATA etc)
	bool bRet = false;
	LPSTR ret = NULL;
	ret = LoadOtherNodes( this, &bRet, xml, pi );
	if( ret != NULL ) 
		xml = ret;
	if( bRet ) 
		return xml;

	// XML Node Tag Name Open
	xml++;
	char* pTagEnd = strpbrk( xml, " />\t\r\n" );
	_SetString( xml, pTagEnd, &name );
	xml = pTagEnd;
	// Generate XML Attributte List
	if( xml = LoadAttributes( xml, pi ) )
	{
		// alone tag <TAG ... />
		if( *xml == chXMLTagPre )
		{
			xml++;
			if( *xml == chXMLTagClose )
				// wel-formed tag
				return ++xml;
			else
			{
				// error: <TAG ... / >
				if( pi->error_occur == false ) 
				{
					pi->error_occur = true;
					pi->error_pointer = xml;
					pi->error_code = PIE_ALONE_NOT_CLOSED;
					pi->error_string = "Element must be closed.";
				}
				// not wel-formed tag
				return NULL;
			}
		}
		else
		// open/close tag <TAG ..> ... </TAG>
		//                        ^- current pointer
		{
			// if text value is not exist, then assign value
			//if( this->value.IsEmpty() || this->value == _T("") )
			if( XIsEmptyString( value ) )
			{
				// Text Value 
				char* pEnd = _tcsechr( ++xml, chXMLTagOpen, chXMLEscape );
				if( pEnd == NULL ) 
				{
					if( pi->error_occur == false ) 
					{
						pi->error_occur = true;
						pi->error_pointer = xml;
						pi->error_code = PIE_NOT_CLOSED;
						char err[1024];
						sprintf(err, "%.256s must be closed with </%.256s>", name.c_str(), name.c_str() );
						pi->error_string = err;
					}
					// error cos not exist CloseTag </TAG>
					return NULL;
				}
				
				bool trim = pi->trim_value;
				char escape = pi->escape_value;
				//_SetString( xml, pEnd, &value, trim, chXMLEscape );
				_SetString( xml, pEnd, &value, trim, escape );

				xml = pEnd;
				// TEXTVALUE reference
				if( pi->entity_value && pi->entitys )
					value = pi->entitys->Ref2Entity(value.c_str());
			}

			// generate child nodes
			while( xml && *xml )
			{
				LPXNode node = new XNode;
				node->parent = this;
				node->doc = doc;
				node->type = type;
				
				xml = node->Load( xml,pi );
				if(!node->name.empty())
				{
					childs.push_back( node );
				}
				else
				{
					delete node;
				}

				// open/close tag <TAG ..> ... </TAG>
				//                             ^- current pointer
				// CloseTag case
				if( xml && *xml && *(xml+1) && *xml == chXMLTagOpen && *(xml+1) == chXMLTagPre )
				{
					// </Close>
					xml+=2; // C
					
					if( xml = _tcsskip( xml ) )
					{
						std::string closename;
						char* pEnd = strpbrk( xml, " >" );
						if( pEnd == NULL ) 
						{
							if( pi->error_occur == false ) 
							{
								pi->error_occur = true;
								pi->error_pointer = xml;
								pi->error_code = PIE_NOT_CLOSED;
								char err[1024];
								sprintf(err, "it must be closed with </%.768s>", name.c_str() );
								pi->error_string = err;
							}
							// error
							return NULL;
						}
						_SetString( xml, pEnd, &closename );
						if( closename == this->name )
						{
							// wel-formed open/close
							xml = pEnd+1;
							// return '>' or ' ' after pointer
							return xml;
						}
						else
						{
							xml = pEnd+1;
							// 2004.6.15 - example <B> alone tag
							// now it can parse with attribute 'force_arse'
							if( pi->force_parse == false )
							{
								// not welformed open/close
								if( pi->error_occur == false ) 
								{
									pi->error_occur = true;
									pi->error_pointer = xml;
									pi->error_code = PIE_NOT_NESTED;
									char err[1024];
									sprintf(err, "'<%.256s> ... </%.256s>' is not wel-formed.", name.c_str(), closename.c_str());
									pi->error_string = err;
								}
								return NULL;
							}
						}
					}
				}
				else	// Alone child Tag Loaded
						// else 해야하는지 말아야하는지 의심간다.
				{
					
					//if( xml && this->value.IsEmpty() && *xml !=chXMLTagOpen )
					if( xml && XIsEmptyString( value ) && *xml !=chXMLTagOpen )
					{
						// Text Value 
						char* pEnd = _tcsechr( xml, chXMLTagOpen, chXMLEscape );
						if( pEnd == NULL ) 
						{
							// error cos not exist CloseTag </TAG>
							if( pi->error_occur == false )  
							{
								pi->error_occur = true;
								pi->error_pointer = xml;
								pi->error_code = PIE_NOT_CLOSED;
								char err[1024];
								sprintf(err, "it must be closed with </%.768s>", name.c_str() );
								pi->error_string = err;
							}
							return NULL;
						}
						
						bool trim = pi->trim_value;
						char escape = pi->escape_value;
						//_SetString( xml, pEnd, &value, trim, chXMLEscape );
						_SetString( xml, pEnd, &value, trim, escape );

						xml = pEnd;
						//TEXTVALUE
						if( pi->entity_value && pi->entitys )
							value = pi->entitys->Ref2Entity(value.c_str());
					}
				}
			}
		}
	}

	return xml;
}

// <?xml version='1.0'?>
// <TAG attr1="value1" attr2='value2' attr3=value3 >
// </TAG>
// or
// <TAG />
//        ^- return pointer
//========================================================
// Name   : Load
// Desc   : load xml plain text for xml document
// Param  : pszXml - plain xml text
//          pi = parser information
// Return : advanced string pointer  (error return NULL)
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
LPSTR _tagXMLDocument::Load( LPCSTR pszXml, LPPARSEINFO pi /*= NULL*/ )
{
	LPXNode node = new XNode;
	node->parent = (LPXNode)this;
	node->type = XNODE_ELEMENT;
	node->doc = this;
	LPSTR end;
	
	if( pi == NULL )
		pi = &parse_info;

	if( (end = node->Load( pszXml, pi )) == NULL )
	{
		delete node;
		return NULL;
	}

	childs.push_back( node );

	// Load Other Node after </Tag>(pi, comment, CDATA etc)
	LPSTR ret;
	bool bRet = false;
	ret = LoadOtherNodes( node, &bRet, end, pi );
	if( ret != NULL ) 
		end = ret;

	return end;
}


const XDoc& _tagXMLDocument::operator = ( const XDoc& doc )
{
	Close();
	_CopyBranch( (const LPXNode)&doc, this );
	return *this;
}


LPXNode	_tagXMLDocument::GetRoot()
{
	XNodes::iterator it = childs.begin();
	for( ; it != childs.end() ; ++(it) )
	{
		LPXNode node = *it;
		if( node->type == XNODE_ELEMENT )
			return node;
	}
	return NULL;
}

const LPXNode	_tagXMLDocument::GetRoot() const
{
	XNodes::const_iterator it = childs.begin();
	for( ; it != childs.end() ; ++(it) )
	{
		const LPXNode node = *it;
		if( node->type == XNODE_ELEMENT )
			return node;
	}
	return NULL;
}

//========================================================
// Name   : GetXML
// Desc   : convert plain xml text from parsed xml attirbute
// Param  :
// Return : converted plain string
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
std::string _tagXMLAttr::GetXML( LPDISP_OPT opt /*= &optDefault*/ ) const
{
	std::ostringstream os;
	//os << (LPCSTR)name << "='" << (LPCSTR)value << "' ";
	
	os << name.c_str() << "=" << (char)opt->value_quotation_mark 
		<< ((opt->reference_value&&opt->entitys) ? opt->entitys->Entity2Ref(value.c_str()).c_str() : value.c_str()) 
		<< (char)opt->value_quotation_mark << " ";
	return os.str();
}

//========================================================
// Name   : GetXML
// Desc   : convert plain xml text from parsed xml node
// Param  :
// Return : converted plain string
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
std::string XNode::GetXML( LPDISP_OPT opt /*= &optDefault*/ ) const
{
	std::ostringstream os;

	//add header
	
	// new line and tab
	if( opt && opt->newline && type == XNODE_ELEMENT)
	{
		//if(opt->bAddHeader && parent == NULL )
		//{
			//os << "<?xml version='1.0' encoding=\"utf-8\" ?>\r\n";
		//	os << "\r\n";
		//}
		//else
		//{
			if( opt && opt->newline )
				os << "\r\n";

			for( int i = 0 ; i < opt->tab_base ; i++)
				os << '\t';
		//}
	}

	if( type == XNODE_DOC )
	{
		for( XNodes::size_type i = 0 ; i < childs.size(); i++ )
			os << childs[i]->GetXML( opt ).c_str();
		return os.str();
	}
	else
	if( type == XNODE_PI )
	{
		// <?TAG
		os << szXMLPIOpen << name.c_str();
		// <?TAG Attr1="Value1"
		if( !attrs.empty() ) os << ' ';
		for( XAttrs::size_type i = 0 ; i < attrs.size(); i++ )
		{
			os << attrs[i]->GetXML(opt).c_str();
		}
		//?>
		os << szXMLPIClose;	
		return os.str();
	}
	else
	if( type == XNODE_COMMENT )
	{
		// <--comment
		os << szXMLCommentOpen << value.c_str();
		//-->
		os << szXMLCommentClose;	
		return os.str();
	}
	else
	if( type == XNODE_CDATA )
	{
		// <--comment
		os << szXMLCDATAOpen << value.c_str();
		//-->
		os << szXMLCDATAClose;	
		return os.str();
	}

	// <TAG
	os << '<' << name.c_str();

	// <TAG Attr1="Val1" 
	if( attrs.empty() == false ) os << ' ';
	for( XAttrs::size_type i = 0 ; i < attrs.size(); i++ )
	{
		os << attrs[i]->GetXML(opt).c_str();
	}
	
	if( childs.empty() && value.empty() )
	{
		// <TAG Attr1="Val1"/> alone tag 
		os << "/>";	
	}
	else
	{
		// <TAG Attr1="Val1"> and get child
		os << '>';
		if( opt && opt->newline && !childs.empty() )
		{
			opt->tab_base++;
		}

		for( XNodes::size_type i = 0 ; i < childs.size(); i++ )
			os << childs[i]->GetXML( opt ).c_str();
		
		// Text Value
		if( value != "" )
		{
			if( opt && opt->newline && !childs.empty() )
			{
				if( opt && opt->newline )
					os << "\r\n";
				for( int i = 0 ; i < opt->tab_base ; i++)
					os << '\t';
			}
			os << ((opt->reference_value&&opt->entitys) ? opt->entitys->Entity2Ref(value.c_str()).c_str() : value.c_str());
		}

		// </TAG> CloseTag
		if( opt && opt->newline && !childs.empty() )
		{
			os << "\r\n";
			for( int i = 0 ; i < opt->tab_base-1 ; i++)
				os << '\t';
		}
		os << "</" << name.c_str() << '>';

		if( opt && opt->newline )
		{
			if( !childs.empty() )
				opt->tab_base--;
		}
	}
	
	return os.str();
}
/*
void XNode::Write( std::ostream& os)const
{
	DISP_OPT opt;
	opt.newline=false;
	
	//Get info string
	std::string buffer = GetXML(&opt);
	os.write( buffer.c_str(), buffer.length() );
}*/

std::string XNode::GetText( LPDISP_OPT opt /*= &optDefault*/ ) const
{
	std::ostringstream os;

	if( type == XNODE_DOC )
	{
		for( XNodes::size_type i = 0 ; i < childs.size(); i++ )
			os << childs[i]->GetText( opt ).c_str();
	}
	else
	if( type == XNODE_PI )
	{
		// no text
	}
	else
	if( type == XNODE_COMMENT )
	{
		// no text
	}
	else
	if( type == XNODE_CDATA )
	{
		os << value.c_str();
	}
	else
	if( type == XNODE_ELEMENT )
	{
		if( childs.empty() && value.empty() )
		{
			// no text
		}
		else
		{
			// childs text
			for( XNodes::size_type i = 0 ; i < childs.size(); i++ )
				os << childs[i]->GetText().c_str();
			
			// Text Value
			os << ((opt->reference_value&&opt->entitys) ? opt->entitys->Entity2Ref(value.c_str()) : value.c_str());
		}
	}
	
	return os.str();
}

std::string _tagXMLAttr::GetValue( LPDISP_OPT opt /*= &optDefault*/ ) const
{
	std::ostringstream os;

	// Text Value
	os << ((opt->reference_value&&opt->entitys) ? opt->entitys->Entity2Ref(value.c_str()) : value.c_str());
	
	return os.str();
}

//========================================================
// Name   : GetAttr
// Desc   : get attribute with attribute name
// Param  :
// Return : 
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
LPXAttr	XNode::GetAttr( LPCSTR attrname )
{
	for( XAttrs::size_type i = 0 ; i < attrs.size(); i++ )
	{
		LPXAttr attr = attrs[i];
		if( attr )
		{
			if( attr->name == attrname )
				return attr;
		}
	}
	return NULL;
}

//========================================================
// Name   : GetAttrs
// Desc   : find attributes with attribute name, return its list
// Param  :
// Return : 
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
XAttrs XNode::GetAttrs( LPCSTR name )
{
	XAttrs attrs;
	for( XAttrs::size_type i = 0 ; i < attrs.size(); i++ )
	{
		LPXAttr attr = attrs[i];
		if( attr )
		{
			if( attr->name == name )
				attrs.push_back( attr );
		}
	}
	return attrs;
}

//========================================================
// Name   : GetAttrValue
// Desc   : get attribute with attribute name, return its value
// Param  :
// Return : 
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
LPCSTR	XNode::GetAttrValue( LPCSTR attrname )
{
	LPXAttr attr = GetAttr( attrname );
	return attr ? attr->value.c_str() : NULL;
}



//========================================================
// Name   : GetChilds
// Desc   : Find childs with name and return childs list
// Param  :
// Return : 
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
XNodes XNode::GetChilds( LPCSTR name )const
{
	XNodes nodes;
	for( XNodes::size_type i = 0 ; i < childs.size(); i++ )
	{
		LPXNode node = childs[i];
		if( node )
		{
			if( node->name == name )
				nodes.push_back( node );
		}
	}
	return nodes;	
}

//========================================================
// Name   : GetChild
// Desc   : get child node with index
// Param  :
// Return : NULL return if no child.
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
LPXNode XNode::GetChild( XNodes::size_type i )
{
	if( i >= 0 && i < childs.size() )
		return childs[i];
	return NULL;
}

//========================================================
// Name   : GetChildCount
// Desc   : get child node count
// Param  :
// Return : 0 return if no child
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-12-26
//========================================================
int	XNode::GetChildCount()
{
	return (int)childs.size();
}

//========================================================
// Name   : GetChild
// Desc   : Find child with name and return child
// Param  :
// Return : NULL return if no child.
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
LPXNode	XNode::GetChild( LPCSTR name )
{
	for( XNodes::size_type i = 0 ; i < childs.size(); i++ )
	{
		LPXNode node = childs[i];
		if( node )
		{
			if( node->name == name )
				return node;
		}
	}
	return NULL;
}

const LPXNode	XNode::GetChild( LPCSTR name )const 
{
	for( XNodes::size_type i = 0 ; i < childs.size(); i++ )
	{
		LPXNode node = childs[i];
		if( node )
		{
			if( node->name == name )
				return node;
		}
	}
	return NULL;
}

//========================================================
// Name   : GetChildValue
// Desc   : Find child with name and return child's value
// Param  :
// Return : NULL return if no child.
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
const LPCSTR	XNode::GetChildValue( LPCSTR name )const
{
	LPXNode node = GetChild( name );
	return (node != NULL)? node->value.c_str() : NULL;
}

std::string	XNode::GetChildText( LPCSTR name, LPDISP_OPT opt /*= &optDefault*/ )
{
	LPXNode node = GetChild( name );
	return (node != NULL)? node->GetText(opt).c_str() : "";
}

LPXAttr XNode::GetChildAttr( LPCSTR name, LPCSTR attrname )
{
	LPXNode node = GetChild(name);
	return node ? node->GetAttr(attrname) : NULL;
}

LPCSTR XNode::GetChildAttrValue( LPCSTR name, LPCSTR attrname )
{
	LPXAttr attr = GetChildAttr( name, attrname );
	return attr ? attr->value.c_str() : NULL;
}

//========================================================
// Name   : Find
// Desc   : find node with tag name from it's all childs
// Param  :
// Return : NULL return if no found node.
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
LPXNode	XNode::Find( LPCSTR name )
{
	XNodes::iterator it = childs.begin();
	for( ; it != childs.end(); ++(it))
	{
		LPXNode child = *it;
		if( child->name == name )
			return child;

		XNodes::iterator it = child->childs.begin();
		for( ; it != child->childs.end(); ++(it))
		{
			LPXNode find = child->Find( name );
			if( find != NULL )
				return find;
		}
	}

	return NULL;
}

//========================================================
// Name   : Find
// Desc   : find node with tag name from it's all childs
// Param  :
// Return : NULL return if no found node.
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
const LPXNode	XNode::Find( LPCSTR name ) const
{
	XNodes::const_iterator it = childs.begin();
	for( ; it != childs.end(); ++(it))
	{
		LPXNode child = *it;
		if( child->name == name )
			return child;

		XNodes::const_iterator it = child->childs.begin();
		for( ; it != child->childs.end(); ++(it))
		{
			const LPXNode find = child->Find( name );
			if( find != NULL )
				return find;
		}
	}

	return NULL;
}

//========================================================
// Name   : Find
// Desc   : find node with tag name from AND id recursively
// Param  :
// Return : NULL return if no found node.
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
LPXNode	XNode::Find( LPCSTR name, LPCSTR id )
{
	XNodes::iterator it = childs.begin();
	for( ; it != childs.end(); ++(it))
	{
		LPXNode child = *it;
		if( child->name == name )
		{
			LPXAttr attr = child->GetAttr("id");
			if ((attr != NULL) && (attr->value == id))
				return child;
		}

		XNodes::iterator it = child->childs.begin();
		for( ; it != child->childs.end(); ++(it))
		{
			LPXNode find = child->Find( name, id );
			if( find != NULL )
				return find;
		}
	}

	return NULL;
}

//========================================================
// Name   : Find
// Desc   : find node with tag name from AND id recursively
// Param  :
// Return : NULL return if no found node.
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
const LPXNode	XNode::Find( LPCSTR name, LPCSTR id ) const
{
	XNodes::const_iterator it = childs.begin();
	for( ; it != childs.end(); ++(it))
	{
		LPXNode child = *it;
		if( child->name == name )
		{
			const LPXAttr attr = child->GetAttr("id");
			if ((attr != NULL) && (attr->value == id))
				return child;
		}

		XNodes::const_iterator it = child->childs.begin();
		for( ; it != child->childs.end(); ++(it))
		{
			const LPXNode find = child->Find( name, id );
			if( find != NULL )
				return find;
		}
	}

	return NULL;
}

//========================================================
// Name   : GetChildIterator
// Desc   : get child nodes iterator
// Param  :
// Return : NULL return if no childs.
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
XNodes::iterator XNode::GetChildIterator( LPXNode node )
{
	XNodes::iterator it = childs.begin();
	for( ; it != childs.end() ; ++(it) )
	{
		if( *it == node )
			return it;
	}
	return childs.end();
}

//========================================================
// Name   : AppendChild
// Desc   : add node
// Param  :
// Return : 
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
LPXNode	XNode::AppendChild( LPCSTR name /*= NULL*/, LPCSTR value /*= NULL*/ )
{
	return AppendChild( CreateNode( name, value ) );
}

//========================================================
// Name   : AppendChild
// Desc   : add node
// Param  :
// Return : 
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
LPXNode XNode::AppendChild( LPXNode node )
{
	node->parent = this;
	node->doc = doc;
	childs.push_back( node );
	return node;
}

//========================================================
// Name   : RemoveChild
// Desc   : detach node and delete object
// Param  :
// Return : 
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
bool XNode::RemoveChild( LPXNode node )
{
	XNodes::iterator it = GetChildIterator( node );
	if( it != childs.end() )
	{
		delete *it;
		childs.erase( it );
		return true;
	}
	return false;
}

//========================================================
// Name   : GetAttr
// Desc   : get attribute with index in attribute list
// Param  :
// Return : 
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
LPXAttr XNode::GetAttr( XAttrs::size_type i )
{
	if( i >= 0 && i < attrs.size() )
		return attrs[i];
	return NULL;
}

//========================================================
// Name   : GetAttrIterator
// Desc   : get attribute iterator
// Param  : 
// Return : std::vector<LPXAttr>::iterator
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
XAttrs::iterator XNode::GetAttrIterator( LPXAttr attr )
{
	XAttrs::iterator it = attrs.begin();
	for( ; it != attrs.end() ; ++(it) )
	{
		if( *it == attr )
			return it;
	}
	return attrs.end();
}

//========================================================
// Name   : AppendAttr
// Desc   : add attribute
// Param  :
// Return : 
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
LPXAttr XNode::AppendAttr( LPXAttr attr )
{
	attr->parent = this;
	attrs.push_back( attr );
	return attr;
}

//========================================================
// Name   : RemoveAttr
// Desc   : detach attribute and delete object
// Param  :
// Return : 
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
bool XNode::RemoveAttr( LPXAttr attr )
{
	XAttrs::iterator it = GetAttrIterator( attr );
	if( it != attrs.end() )
	{
		delete *it;
		attrs.erase( it );
		return true;
	}
	return false;
}

//========================================================
// Name   : CreateNode
// Desc   : Create node object and return it
// Param  :
// Return : 
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
LPXNode XNode::CreateNode( LPCSTR name /*= NULL*/, LPCSTR value /*= NULL*/ )
{
	LPXNode node = new XNode;
	node->name = name == NULL ? "" : name;
	node->value = value == NULL ? "" : value;
	return node;
}

void XNode::DeleteNode( LPXNode& pNode )
{
	if( pNode )
	{
		pNode->Close();
		delete pNode;
		pNode=NULL;
	}
}
//========================================================
// Name   : CreateAttr
// Desc   : create Attribute object and return it
// Param  :
// Return : 
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
LPXAttr XNode::CreateAttr( LPCSTR name /*= NULL*/, LPCSTR value /*= NULL*/ )
{
	LPXAttr attr = new XAttr;
	attr->name = name == NULL ? "" : name;
	attr->value = value == NULL ? "" : value;
	return attr;
}

//========================================================
// Name   : AppendAttr
// Desc   : add attribute
// Param  :
// Return : 
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
LPXAttr XNode::AppendAttr( LPCSTR name /*= NULL*/, LPCSTR value /*= NULL*/ )
{
	return AppendAttr( CreateAttr( name, value ) );
}

//========================================================
// Name   : DetachChild
// Desc   : no delete object, just detach in list
// Param  :
// Return : 
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
LPXNode XNode::DetachChild( LPXNode node )
{
	XNodes::iterator it = GetChildIterator( node );
	if( it != childs.end() )
	{
		childs.erase( it );
		return node;
	}
	return NULL;
}

//========================================================
// Name   : DetachAttr
// Desc   : no delete object, just detach in list
// Param  :
// Return : 
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
LPXAttr XNode::DetachAttr( LPXAttr attr )
{
	XAttrs::iterator it = GetAttrIterator( attr );
	if( it != attrs.end() )
	{
		attrs.erase( it );
		return attr;
	}
	return NULL;
}

//========================================================
// Name   : CopyNode
// Desc   : copy current level node with own attributes
// Param  :
// Return : 
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
void XNode::CopyNode( LPXNode node )
{
	Close();

	doc = node->doc;
	parent = node->parent;
	name = node->name;
	value = node->value;
	type = node->type;

	// copy attributes
	for( XAttrs::size_type i = 0 ; i < node->attrs.size(); i++)
	{
		LPXAttr attr = node->attrs[i];
		if( attr )
			AppendAttr( attr->name.c_str(), attr->value.c_str() );
	}
}

//========================================================
// Name   : _CopyBranch
// Desc   : recursive internal copy branch 
// Param  :
// Return : 
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
void XNode::_CopyBranch( const LPXNode node, LPXDoc docNew /* = NULL */ )
{
	CopyNode( node );
	if (docNew != NULL)
		doc = docNew;

	for( XNodes::size_type i = 0 ; i < node->childs.size(); i++)
	{
		LPXNode child = node->childs[i];
		if( child )
		{
			LPXNode mychild = new XNode;
			mychild->CopyNode( child );
			AppendChild( mychild );

			mychild->_CopyBranch( child, docNew );
		}
	}
}

//========================================================
// Name   : AppendChildBranch
// Desc   : add child branch ( deep-copy )
// Param  :
// Return : 
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
LPXNode	XNode::AppendChildBranch( LPXNode node )
{
	LPXNode child = new XNode;
	child->CopyBranch( node );

	return AppendChild( child );
}

//========================================================
// Name   : CopyBranch
// Desc   : copy branch ( deep-copy )
// Param  :
// Return : 
//--------------------------------------------------------
// Coder    Date                      Desc
// bro      2002-10-29
//========================================================
void XNode::CopyBranch( const LPXNode branch )
{
	Close();
	
	_CopyBranch( branch );
}


_tagXMLEntitys::_tagXMLEntitys( LPXENTITY entities, int count )
{
	for( int i = 0; i < count; i++)
		push_back( entities[i] );
}

LPXENTITY _tagXMLEntitys::GetEntity( int entity )
{
	for( size_type i = 0 ; i < size(); i ++ )
	{
		if( at(i).entity == entity )
			return LPXENTITY(&at(i));
	}
	return NULL;
}

LPXENTITY _tagXMLEntitys::GetEntity( LPCSTR entity )
{
	for( size_type i = 0 ; i < size(); i ++ )
	{
		LPCSTR ref = (LPSTR)at(i).ref;
		LPCSTR ps = entity;
		while( ref && *ref )
			if( *ref++ != *ps++ )
				break;
		if( ref && !*ref )	// found!
			return LPXENTITY(&at(i));
	}
	return NULL;
}

int _tagXMLEntitys::GetEntityCount( LPCSTR str )
{
	int nCount = 0;
	LPSTR ps = (LPSTR)str;
	while( ps && *ps )
		if( GetEntity( *ps++ ) ) nCount ++;
	return nCount;
}

int _tagXMLEntitys::Ref2Entity( LPCSTR estr, std::string& str, int elen )
{
	std::string::size_type nBefore = str.size();
	LPCSTR estr_end = estr+elen;
	while( estr && *estr && (estr < estr_end) )
	{
		LPXENTITY ent = GetEntity( estr );
		if( ent )
		{
			// copy entity meanning char
			str += ent->entity;
			estr += ent->ref_len;
		}
		else
			str += *estr++;	// default character copy
	}
	
	// total copied characters
	return (int)(str.size() - nBefore);
}

int _tagXMLEntitys::Ref2Entity( LPCSTR estr, LPSTR str, int len )
{
	LPSTR ps = str;
	LPSTR ps_end = ps+len;
	while( estr && *estr && (ps < ps_end) )
	{
		LPXENTITY ent = GetEntity( estr );
		if( ent )
		{
			// copy entity meanning char
			*ps = ent->entity;
			estr += ent->ref_len;
		}
		else
			*ps = *estr++;	// default character copy
		ps++;
	}
	*ps = '\0';
	
	// total copied characters
	return (int)(ps-str);	
}

int _tagXMLEntitys::Entity2Ref( LPCSTR str, std::string& estr, int len )
{
	std::string::size_type nBefore = estr.size();
	LPCSTR str_end = str+len;
	while( str && *str && (str < str_end) )
	{
		LPXENTITY ent = GetEntity( *str );
		if( ent )
		{
			// copy entity string
			if (ent->ref)
				estr += ent->ref;
		}
		else
			estr += *str;	// default character copy
		str++;
	}
	
	// total copied characters
	return int(estr.size() - nBefore);
}

int _tagXMLEntitys::Entity2Ref( LPCSTR str, LPSTR estr, int estrlen )
{
	LPSTR pes = (LPSTR)estr;
	LPSTR pes_end = pes+estrlen;
	while( str && *str && (pes < pes_end) )
	{
		LPXENTITY ent = GetEntity( *str );
		if( ent )
		{
			// copy entity string
			LPCSTR ref = ent->ref;
			while( ref && *ref )
				*pes++ = *ref++;
		}
		else
			*pes++ = *str;	// default character copy
		str++;
	}
	*pes = '\0';
	
	// total copied characters
	return int(pes-estr);
}

std::string _tagXMLEntitys::Ref2Entity( LPCSTR estr )
{
	std::string es;
	if( estr )
	{
		int len = (int)strlen(estr);
		Ref2Entity(estr, es, len);
	}
	return es;
}

std::string _tagXMLEntitys::Entity2Ref( LPCSTR str )
{
	std::string s;
	if( str )
	{
		int nEntityCount = GetEntityCount(str);
		if( nEntityCount == 0 )
			return str;
		Entity2Ref( str, s, (int)strlen(str) );
	}
	return s;
}

std::string XRef2Entity( LPCSTR estr )
{
	return entityDefault.Ref2Entity( estr );
}

std::string XEntity2Ref( LPCSTR str )
{
	return entityDefault.Entity2Ref( str );
}

//*****************************************************************
//CXMLSerialization
/*

void CXMLSerialization::GetXML(XNode& root)const
{
	LPXNode pNode = root.AppendChild(GetXMLFlag());
	_ASSERTE(pNode);
	
	if( pNode )
	{
		pNode->name = XMLFlag;
		for(int i=0; i<GetNbMember(); i++)
		{
			xml.AppendChild(GetMemberName(i), GetString(i) );
		}
	}	
}

void CXMLSerialization::SetXML(XNode& root)
{
	ERMsg msg;

	LPXNode pNode = root.Find(GetXMLFlag());
	if( pNode )
	{
		for(int i=0; i<GetNbMember(); i++)
		{
			SetString( i, pNode->GetChildValue(GetMemberName(i)) ); 
		}
	}
}

*/

LPXNode XNode::Select(LPCSTR name )
{
	if( name == this->name)
		return (XNode*)this;

	return GetChild(name);
}

const LPXNode XNode::Select(LPCSTR name )const
{
	if( name == this->name)
		return (XNode*)this;

	return GetChild(name);
}

