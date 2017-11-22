#include "StdAfx.h"
#include "CompareDailyData.h"

//#include "normalStation.h"
//#include "normalDatabase.h"
#include "DailyDatabase.h"
#include "DailyStation.h"
#include "Statistic.h"
#include "FrequencyTable.h"

#include "SYShowMessage.h"
#include "CommonRes.h"
#include "FileManager\FileManagerRes.h"
#include "Task.h"
#include "Resource.h"
//#include "AdvancedNormalStation.h"

//#include <shlwapi.h>
//#include <atlpath.h>

using namespace DAILY_DATA;
using namespace WEATHER;
using namespace UtilWin;
//*********************************************************************

const char* CCompareDailyData::ATTRIBUTE_NAME[NB_ATTRIBUTE]={ "Input1", "Input2", "Output"};
const char* CCompareDailyData::CLASS_NAME = "COMPARE DAILY DATABASE";

CCompareDailyData::CCompareDailyData(void)
{
	if( !IsRegister( GetClassID() ) )
	{
		InitClass();
	}

	Reset();
}

void CCompareDailyData::InitClass(const CStringArray& option)
{
	//GetParamClassInfo().m_className.LoadString( IDS_SOURCENAME_EXPROTFROMDAILY );
	GetParamClassInfo().m_className = "Compare daily database";

	CToolsBase::InitClass(option);
	//init static 
	ASSERT( GetParameters().GetSize() < I_NB_ATTRIBUTE);

	//CStringArray array;
	//LoadStringArray( array, IDS_PROPERTIES_NORMAL2);
	//ASSERT( array.GetSize() == NB_ATTRIBUTE+6);

	CString filter1 = GetString( IDS_CMN_FILTER_DAILY);
	CString filter2 = "*.txt|*.txt||";
	GetParameters().Add( CParamDef(CParamDef::FILEPATH, ATTRIBUTE_NAME[0], "Input1", filter1) );
	GetParameters().Add( CParamDef(CParamDef::FILEPATH, ATTRIBUTE_NAME[1], "Input2", filter1) );
	GetParameters().Add( CParamDef(CParamDef::FILEPATH, ATTRIBUTE_NAME[2], "output", filter2) );
}

CCompareDailyData::~CCompareDailyData(void)
{
}


CCompareDailyData::CCompareDailyData(const CCompareDailyData& in)
{
	operator=(in);
}

void CCompareDailyData::Reset()
{
	CToolsBase::Reset();

	m_inputFilePath1.Empty();
	m_inputFilePath2.Empty();
	m_ouputFilePath.Empty();
	

}

CCompareDailyData& CCompareDailyData::operator =(const CCompareDailyData& in)
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

bool CCompareDailyData::operator ==(const CCompareDailyData& in)const
{
	bool bEqual = true;

	if( CToolsBase::operator !=(in) )bEqual = false;
	if( m_ouputFilePath != in.m_ouputFilePath)bEqual = false;
	if( m_inputFilePath1 != in.m_inputFilePath1)bEqual = false;
	if( m_inputFilePath2 != in.m_inputFilePath2)bEqual = false;
	
	
	return bEqual;
}

bool CCompareDailyData::operator !=(const CCompareDailyData& in)const
{
	return !operator ==(in);
}


CString CCompareDailyData::GetValue(short type)const
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

void CCompareDailyData::SetValue(short type, const CString& value)
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


ERMsg CCompareDailyData::Execute(CFL::CCallback& callback)
{
	ERMsg message;

	
	//Get the data for each station
	CDailyDatabase dailyDB1;
	CDailyDatabase dailyDB2;
	

	message += dailyDB1.Open( GetAbsoluteFilePath(m_inputFilePath1));
	message += dailyDB2.Open( GetAbsoluteFilePath(m_inputFilePath2));
	CStdioFile outputFile( GetAbsoluteFilePath(m_ouputFilePath), CFile::modeCreate|CFile::modeWrite );
	
	outputFile.WriteString( "nom\tTMin1\tSDMin1\tTmin2\tSDTmin2\tTMax1\tSDMax1\tTmax2\tSDTmax2\n");
	if( !message)
		return message;

	
	callback.SetCurrentDescription("Compare database");
	callback.SetNbStep(0, dailyDB1.GetSize(), 1);
	

	for(int i=0; i<dailyDB1.GetSize(); i++)
	{
		CDailyStation dailyStation1;
		CDailyStation dailyStation2;

		ERMsg messageTmp = 	dailyDB1.GetAt(i, dailyStation1);

		if( messageTmp)
		{
			CIntArray years;
			dailyStation1.GetYearList(years);
			ASSERT( years.GetSize() == 1);
			int year = years[0];

			CSearchResultVector result;
			dailyDB2.Match(dailyStation1, 1, result, TEMPERATURE, year );
			dailyDB2.GetAt( result[0].m_index, dailyStation2, year);

			CFL::CStatistic stat[4];

			const CDailyYear& data1 = dailyStation1.GetYear( year );
			const CDailyYear& data2 = dailyStation2.GetYear( year );

			
			for(int jd=0; jd<366; jd++)
			{
				if( data1[jd][TMIN]!=-999 && data2[jd][TMIN]!=-999 )
				{
					stat[0] += data1[jd][TMIN];
					stat[1] += data2[jd][TMIN];
				}
				
				if( data1[jd][TMAX]!=-999 && data2[jd][TMAX]!=-999 )
				{
					stat[2] += data1[jd][TMAX];
					stat[3] += data2[jd][TMAX];
				}
			}

			CString line;
			line.Format( "%s\t", dailyStation1.GetName() );
			if( stat[0][CFL::NB_VALUE] >= 0 && stat[2][CFL::NB_VALUE] >= 0 )
			{
				for(int x=0; x<4; x++)
				{
					CString tmp;
					tmp.Format( "%lf\t%lf\t", stat[x][CFL::MEAN], stat[x][CFL::STD_DEV] );
					line += tmp;
				}
			}
			
			outputFile.WriteString(line+"\n");
		}


		if(! messageTmp )
			callback.AddMessage( messageTmp, 1);

		message += callback.StepIt();
		
	}

	dailyDB1.Close();
	dailyDB2.Close();


	return message;
}

bool CCompareDailyData::Compare(const CParameterBase& in)const
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CCompareDailyData& info = dynamic_cast<const CCompareDailyData&>(in);
	return operator==(info);
}

CParameterBase& CCompareDailyData::Assign(const CParameterBase& in)
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CCompareDailyData& info = dynamic_cast<const CCompareDailyData&>(in);
	return operator=(info);
}

