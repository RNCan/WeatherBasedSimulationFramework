#include "StdAfx.h"
#include "Resource.h"
#include "FileManagerRes.h"
#include "CreateOURANOSNormal.h"
#include "OURANOSData.h"

using namespace UtilWin;
//*********************************************************************
const char* CCreateOURANOSNormal::ATTRIBUTE_NAME[] = { "INPUT_PATH", "OUTPUT_FILE_PATH", "PERIOD"};
const char* CCreateOURANOSNormal::CLASS_NAME = "OURANOS_NORMAL";

CCreateOURANOSNormal::CCreateOURANOSNormal(void)
{
	if( !IsRegister( GetClassID() ) )
	{
		InitClass();
	}

	Reset();

}

void CCreateOURANOSNormal::InitClass(const CStringArray& option)
{
	GetParamClassInfo().m_className.LoadString( IDS_SOURCENAME_CREATE_OURANOS_NORMAL );

	CToolsBase::InitClass(option);
	ASSERT( GetParameters().GetSize() < I_NB_ATTRIBUTE);

	CStringArrayEx str;
	str.LoadString( IDS_PROPERTIES_CREATE_OURANOS_NORMAL);
	ASSERT( str.GetSize() == NB_ATTRIBUTE);

//		CString filter1 = "*.loc|*.loc||";
	CString filter2 = GetString( IDS_CMN_FILTER_NORMALS);
	GetParameters().Add( CParamDef(CParamDef::PATH, ATTRIBUTE_NAME[0], str[0], "" ) );
	GetParameters().Add( CParamDef(CParamDef::FILEPATH, ATTRIBUTE_NAME[1], str[1], filter2 ) );

	CParamDef p(CParamDef::COMBO, ATTRIBUTE_NAME[2], str[2], "");
	p.m_subType = CParamDef::BY_NUMBER;
	for(int i=0; i<12; i++)
	{
		CString tmp;
		tmp.Format("%d-%d", 1961+10*i, 1990+10*i);
		p.m_listValue.Add(tmp);
	}
	GetParameters().Add(p);
}

CCreateOURANOSNormal::~CCreateOURANOSNormal(void)
{
}


CCreateOURANOSNormal::CCreateOURANOSNormal(const CCreateOURANOSNormal& in)
{
	operator=(in);
}

void CCreateOURANOSNormal::Reset()
{
	m_inputPath.Empty();
	m_outputFilePath.Empty();
	m_period=0;
}

CCreateOURANOSNormal& CCreateOURANOSNormal::operator =(const CCreateOURANOSNormal& in)
{
	if( &in != this)
	{
		CToolsBase::operator =(in);
		m_inputPath = in.m_inputPath;
		m_outputFilePath = in.m_outputFilePath;
		m_period=in.m_period;
	}

	return *this;
}

bool CCreateOURANOSNormal::operator ==(const CCreateOURANOSNormal& in)const
{
	bool bEqual = true;

	if( CToolsBase::operator !=(in))bEqual = false;
	if( m_inputPath != in.m_inputPath)bEqual = false;
	if( m_outputFilePath != in.m_outputFilePath)bEqual = false;
	if( m_period!=in.m_period)bEqual = false;
	
	return bEqual;
}

bool CCreateOURANOSNormal::operator !=(const CCreateOURANOSNormal& in)const
{
	return !operator ==(in);
}


CString CCreateOURANOSNormal::GetValue(short type)const
{
	CString str;
	

	ASSERT( NB_ATTRIBUTE == 3); 
	switch(type)
	{
	case I_INPUT_PATH: str = m_inputPath;break;
	case I_OUTPUT_FILE_PATH: str = m_outputFilePath; break;
	case I_PERIOD: str = ToString(m_period); break;
	default: str = CToolsBase::GetValue(type); break;
	}

	return str;
}

void CCreateOURANOSNormal::SetValue(short type, const CString& str)
{
	ASSERT( NB_ATTRIBUTE == 3);
	switch(type)
	{
	case I_INPUT_PATH: m_inputPath = str; break;
	case I_OUTPUT_FILE_PATH: m_outputFilePath=str; break;
	case I_PERIOD: m_period = ToInt(str); break;
	default: CToolsBase::SetValue(type, str); break;
	}

}

bool CCreateOURANOSNormal::Compare(const CParameterBase& in)const
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CCreateOURANOSNormal& info = dynamic_cast<const CCreateOURANOSNormal&>(in);
	return operator==(info);
}

CParameterBase& CCreateOURANOSNormal::Assign(const CParameterBase& in)
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CCreateOURANOSNormal& info = dynamic_cast<const CCreateOURANOSNormal&>(in);
	return operator=(info);
}

ERMsg CCreateOURANOSNormal::Execute(CFL::CCallback& callback)
{
	ERMsg msg;

	COuranosDatabase ouranosDatabase(m_inputPath);

//	CStringArray dirList;
	//UtilWin::GetDirList(dirList,m_inputPath+"*");

	//for(int i=0; i<dirList.GetSize(); i++)
	//{
	//	UtilWin::CreateMultipleDir(m_outputFilePath+dirList[i]+"\\");
	//	msg += ouranosDatabase.CreateMMG( m_inputPath+dirList[i]+"\\",m_outputFilePath+dirList[i]+"\\"+dirList[i]+".mmg", callback);
	//}

	UtilWin::CreateMultipleDir(m_outputFilePath+"CRCM423_aev-sresa2-run\\");
	msg += ouranosDatabase.CreateMMG( m_inputPath+"CRCM423_aev-sresa2-run\\",m_outputFilePath+"CRCM423_aev-sresa2-run\\CRCM423_aev-sresa2-run.mmg", callback);

	
	return msg;
}


