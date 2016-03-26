#include "StdAfx.h"
#include ".\CompareNormalData.h"

#include "normalStation.h"
#include "FileManager\normalFile.h"
#include "FileManager\DailyFile.h"
#include "dailyStation.h"
#include "Statistic.h"
#include "FrequencyTable.h"

#include "SYShowMessage.h"
#include "CommonRes.h"
#include "FileManagerRes.h"
#include ".\task.h"
#include "Resource.h"
#include "AdvancedNormalStation.h"

#include <shlwapi.h>
#include <atlpath.h>

using namespace DAILY_DATA;
using namespace WEATHER;
using namespace UtilWin;
//*********************************************************************

const char* CCompareNormalData::ATTRIBUTE_NAME[NB_ATTRIBUTE] = { "Input1", "Input2", "Output" };
const char* CCompareNormalData::CLASS_NAME = "COMPARE NORMAL DATABASE";

CCompareNormalData::CCompareNormalData(void)
{
	if( !IsRegister( GetClassID() ) )
	{
		InitClass();
	}

	Reset();
}

void CCompareNormalData::InitClass(const CStringArray& option)
{
	//GetParamClassInfo().m_className.LoadString( IDS_SOURCENAME_EXPROTFROMDAILY );
	GetParamClassInfo().m_className = "Compare normal database";

	CToolsBase::InitClass(option);
	//init static 
	ASSERT( GetParameters().GetSize() < I_NB_ATTRIBUTE);

	//CStringArray array;
	//LoadStringArray( array, IDS_PROPERTIES_NORMAL2);
	//ASSERT( array.GetSize() == NB_ATTRIBUTE+4);

	CString filter1 = GetString( IDS_CMN_FILTER_NORMALS);
	CString filter2 = "*.cvs|*.cvs||";
	GetParameters().Add( CParamDef(CParamDef::FILEPATH, ATTRIBUTE_NAME[0], "Input1", filter1) );
	GetParameters().Add( CParamDef(CParamDef::FILEPATH, ATTRIBUTE_NAME[1], "Input2", filter1) );
	GetParameters().Add( CParamDef(CParamDef::FILEPATH, ATTRIBUTE_NAME[2], "output", filter2) );
}

CCompareNormalData::~CCompareNormalData(void)
{
}


CCompareNormalData::CCompareNormalData(const CCompareNormalData& in)
{
	operator=(in);
}

void CCompareNormalData::Reset()
{
	CToolsBase::Reset();

	m_inputFilePath1.Empty();
	m_inputFilePath2.Empty();
	m_ouputFilePath.Empty();
	

}

CCompareNormalData& CCompareNormalData::operator =(const CCompareNormalData& in)
{
	if( &in != this)
	{
		CToolsBase::operator =(in);
		m_ouputFilePath = in.m_ouputFilePath;
		m_inputFilePath1 = in.m_inputFilePath1;
		m_inputFilePath2 = in.m_inputFilePath2;
		
	}

	return *this;
}

bool CCompareNormalData::operator ==(const CCompareNormalData& in)const
{
	bool bEqual = true;

	if( CToolsBase::operator !=(in) )bEqual = false;
	if( m_ouputFilePath != in.m_ouputFilePath)bEqual = false;
	if( m_inputFilePath1 != in.m_inputFilePath1)bEqual = false;
	if( m_inputFilePath2 != in.m_inputFilePath2)bEqual = false;
	
	
	return bEqual;
}

bool CCompareNormalData::operator !=(const CCompareNormalData& in)const
{
	return !operator ==(in);
}


CString CCompareNormalData::GetValue(short type)const
{
	CString value;
	
	ASSERT( NB_ATTRIBUTE == 3); 
	switch(type)
	{
	case I_INPUT1: value = m_inputFilePath1; break;
	case I_INPUT2: value = m_inputFilePath2; break;
	case I_OUTPUT: value = m_ouputFilePath; break;
	default: value = CToolsBase::GetValue(type); break;
	}
 
	return value;
}

void CCompareNormalData::SetValue(short type, const CString& value)
{
	ASSERT( NB_ATTRIBUTE == 3); 
	switch(type)
	{
	case I_INPUT1: m_inputFilePath1= value; break;
	case I_INPUT2: m_inputFilePath2= value; break;
	case I_OUTPUT: m_ouputFilePath=value; break;
	default: CToolsBase::SetValue(type, value ); break;
	}

}


ERMsg CCompareNormalData::Execute(CFL::CCallback& callback)
{
	ERMsg msg;

	//Get the data for each station
	CNormalsDatabase normalDB1;
	CNormalsDatabase normalDB2;
	

	msg += normalDB1.Open( GetAbsoluteFilePath(m_inputFilePath1));
	msg += normalDB2.Open( GetAbsoluteFilePath(m_inputFilePath2));
	CStdioFile outputFile( GetAbsoluteFilePath(m_ouputFilePath), CFile::modeCreate|CFile::modeWrite );

	CString filePath2(GetAbsoluteFilePath(m_ouputFilePath));
	UtilWin::SetFileTitle( filePath2, UtilWin::GetFileTitle(m_ouputFilePath)+"_ppt");
	CStdioFile outputFile2( filePath2, CFile::modeCreate|CFile::modeWrite );
	
	outputFile.WriteString( "name source,name matched,distance, diff elev, diff TminTmax\n");
	outputFile2.WriteString( "name source,name matched,distance, diff elev, diff Pppt\n");
	if( !msg)
		return msg;

	
	callback.SetCurrentDescription("Compare database");
	callback.SetNbStep(0, normalDB1.GetSize(), 1);
	

	for(int i=0; i<normalDB1.GetSize(); i++)
	{
		CNormalsStation normalStation1;
		ERMsg messageTmp = 	normalDB1.GetAt(i, normalStation1);

		CString stnName( normalStation1.GetName() );
		int pos = stnName.Find("(");
		if( pos > 0)
			stnName.Truncate(pos);
		stnName.Trim(" ");

		if( messageTmp && normalStation1.HaveCat(TEMPERATURE))
		{
			CNormalsStation normalStation2;
			
			CSearchResultVector result;
			VERIFY( normalDB2.Match(normalStation1, 2, result, TEMPERATURE ));
			
			if( normalDB2.GetStationHead(result[0].m_index).GetName().Find(stnName) == -1 &&
				normalDB2.GetStationHead(result[1].m_index).GetName().Find(stnName) != -1 )
				normalDB2.GetAt( result[1].m_index, normalStation2);//take the soncond
			else normalDB2.GetAt( result[0].m_index, normalStation2);

			CFL::CStatistic stat;

			const CNormalsData& data1 = normalStation1.GetData();
			const CNormalsData& data2 = normalStation2.GetData();
			
			for(int m=0; m<12; m++)
			{
				stat += fabs(data1[m][NORMAL_DATA::TMIN_MN]-data2[m][NORMAL_DATA::TMIN_MN]);
				stat += fabs(data1[m][NORMAL_DATA::TMAX_MN]-data2[m][NORMAL_DATA::TMAX_MN]);
			}

			int dElev = abs(normalStation1.GetElev()-normalStation2.GetElev());
			CString line;
			line.Format( "%s,%s,%lf,%d,", normalStation1.GetName(), normalStation2.GetName(), normalStation2.GetDistance( normalStation1),  dElev );
			
			CString tmp;
			tmp.Format( "%lf", stat[CFL::MEAN] );
			line += tmp;
		
		
			outputFile.WriteString(line+"\n");
		}

		if( messageTmp && normalStation1.HaveCat(PRECIPITATION))
		{
			CNormalsStation normalStation2;

			CSearchResultVector result;
			VERIFY( normalDB2.Match(normalStation1, 2, result, PRECIPITATION ));

			if( normalDB2.GetStationHead(result[0].m_index).GetName().Find(stnName) == -1 &&
				normalDB2.GetStationHead(result[1].m_index).GetName().Find(stnName) != -1 )
				normalDB2.GetAt( result[1].m_index, normalStation2);//take the soncond
			else normalDB2.GetAt( result[0].m_index, normalStation2);

			

			CFL::CStatistic stat;

			const CNormalsData& data1 = normalStation1.GetData();
			const CNormalsData& data2 = normalStation2.GetData();
			
			for(int m=0; m<12; m++)
			{
				stat += fabs(data1[m][NORMAL_DATA::PRCP_TT]-data2[m][NORMAL_DATA::PRCP_TT]);
			}

			CString line;
			int diffElev = abs(normalStation1.GetElev()-normalStation2.GetElev());
			line.Format( "%s,%s,%lf,%d,", normalStation1.GetName(), normalStation2.GetName(), normalStation2.GetDistance( normalStation1), diffElev   );
			
			CString tmp;
			tmp.Format( "%lf", stat[CFL::SUM] );
			line += tmp;
		
		
			outputFile2.WriteString(line+"\n");
		}

		if(! messageTmp )
			callback.AddMessage( messageTmp, 1);

		msg += callback.StepIt();
		
	}

	normalDB1.Close();
	normalDB2.Close();


	return msg;
}

bool CCompareNormalData::Compare(const CParameterBase& in)const
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CCompareNormalData& info = dynamic_cast<const CCompareNormalData&>(in);
	return operator==(info);
}

CParameterBase& CCompareNormalData::Assign(const CParameterBase& in)
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CCompareNormalData& info = dynamic_cast<const CCompareNormalData&>(in);
	return operator=(info);
}

