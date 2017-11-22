#include "StdAfx.h"
#include "ClipLOC.h"
#include "SYShowMessage.h"

#include "ShapefileBase.h" 
#include "BasicRes.h"
#include "CommonRes.h"
#include "mappingRes.h"
#include "Resource.h"

//using namespace UtilWin; 
using namespace std;
using namespace stdString;
using namespace CFL;
using namespace GeoBasic;

//*********************************************************************
const char* CClipLOC::ATTRIBUTE_NAME[] = { "INPUT_FILEPATH", "SHAPEFILE", "BOUNDING_BOX", "OUT_FILEPATH"};
const char* CClipLOC::CLASS_NAME = "ClipLocation";

CClipLOC::CClipLOC(void)
{
	if( !IsRegister( GetClassID() ) )
	{
		InitClass();
	}

	Reset();

}

void CClipLOC::InitClass(const StringVector& option)
{
	GetParamClassInfo().m_className = GetString( IDS_SOURCENAME_CLIP_LOC );

	CToolsBase::InitClass(option);

	if( GetParameters().size() < I_NB_ATTRIBUTE)
	{
		StringVector header(IDS_PROPERTIES_CLIP_LOC, "|;");
		ASSERT(header.size() == NB_ATTRIBUTE);

		string filter1 = GetString(IDS_STR_FILTER_LOC);
		string filter2 = GetString( IDS_STR_FILTER_SHAPEFILE);
		GetParameters().push_back( CParamDef(CParamDef::FILEPATH, ATTRIBUTE_NAME[0], header[0], filter1 ) );
		GetParameters().push_back( CParamDef(CParamDef::FILEPATH, ATTRIBUTE_NAME[1], header[1], filter2 ) );
		GetParameters().push_back( CParamDef(CParamDef::COORD_RECT, ATTRIBUTE_NAME[2], header[2]) );
		GetParameters().push_back( CParamDef(CParamDef::FILEPATH, ATTRIBUTE_NAME[3], header[3], filter1 ) );
	}
}

CClipLOC::~CClipLOC(void)
{
}


CClipLOC::CClipLOC(const CClipLOC& in)
{
	operator=(in);
}

void CClipLOC::Reset()
{
	m_inputFilePath.clear();
	m_shapeFilePath.clear();
	m_boundingBox.SetRectEmpty();

	m_outputFilePath.clear();
	
}

CClipLOC& CClipLOC::operator =(const CClipLOC& in)
{
	if( &in != this)
	{
		CToolsBase::operator =(in);
		m_inputFilePath = in.m_inputFilePath;
		m_shapeFilePath = in.m_shapeFilePath;
		m_boundingBox = in.m_boundingBox;
//		m_type = in.m_type;
		m_outputFilePath = in.m_outputFilePath;
	}

	return *this;
}

bool CClipLOC::operator ==(const CClipLOC& in)const
{
	bool bEqual = true;

	if( CToolsBase::operator !=(in))bEqual = false;
	if( m_inputFilePath != in.m_inputFilePath)bEqual = false;
	if( m_shapeFilePath != in.m_shapeFilePath)bEqual = false;
	if( m_boundingBox != in.m_boundingBox)bEqual = false;
	if( m_outputFilePath != in.m_outputFilePath)bEqual = false;
	
	return bEqual;
}

bool CClipLOC::operator !=(const CClipLOC& in)const
{
	return !operator ==(in);
}


string CClipLOC::GetValue(size_t type)const
{
	string value;
	

	ASSERT( NB_ATTRIBUTE == 4); 
	switch(type)
	{
	case I_INPUT_DB: value = m_inputFilePath;break;
	case I_SHAPEFILE: value = m_shapeFilePath;break;
	case I_BOUNDINGBOX: value = stdString::ToString(m_boundingBox).c_str(); break;
	case I_OUT_FILEPATH: value = m_outputFilePath; break;
	default: value = CToolsBase::GetValue(type); break;
	}

	return value;
}

void CClipLOC::SetValue(size_t type, const string& value)
{
	ASSERT( NB_ATTRIBUTE == 4); 
	switch(type)
	{
	case I_INPUT_DB: m_inputFilePath = value;break;
	case I_SHAPEFILE: m_shapeFilePath = value;break;
	case I_BOUNDINGBOX: m_boundingBox = stdString::ToValue<GeoBasic::CGeoRect>(value); break;
	case I_OUT_FILEPATH: m_outputFilePath=value; break;
	default: CToolsBase::SetValue(type, value); break;
	}

}

void CClipLOC::GetSelection(short param, CSelectionDlgItemVector& items)const
{}
void CClipLOC::SetSelection(short param, const CSelectionDlgItemVector& items)
{}

bool CClipLOC::Compare(const CParameterBase& in)const
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CClipLOC& info = dynamic_cast<const CClipLOC&>(in);
	return operator==(info);
}

CParameterBase& CClipLOC::Assign(const CParameterBase& in)
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CClipLOC& info = dynamic_cast<const CClipLOC&>(in);
	return operator=(info);
}

ERMsg CClipLOC::Execute(CFL::CCallback& callback)
{
	ERMsg msg;

	string extention = GetFileExtension(m_inputFilePath);

	msg = ExecuteLOC(callback);

	return msg;
}


//***********************************************************************************
ERMsg CClipLOC::ExecuteLOC(CFL::CCallback& callback)
{
	ERMsg msg;

	//load the WeatherUpdater
	TRY
	{
		Trim(m_inputFilePath);
		Trim(m_shapeFilePath);
		Trim(m_outputFilePath);

		string outputFilePath(GetAbsoluteFilePath(m_outputFilePath));
		SetFileExtension(outputFilePath, ".csv");
		
		string inputFilePath(GetAbsoluteFilePath(m_inputFilePath));

		if (IsEqualNoCase(outputFilePath, inputFilePath))
		{
			msg.ajoute(GetString(IDS_BSC_SAME_NAMES));
			return msg;
		}


		callback.AddMessage( GetString(IDS_CREATE_DATABASE) );
		callback.AddMessage(outputFilePath, 1);
		callback.AddMessage("");

		
		//Get the data for each station
		CLocationVector inputLOC;
		CLocationVector outputLOC;
		CShapeFileBase shapefile;
		

		if( !m_shapeFilePath.empty() )
		{
			msg += shapefile.Read( GetAbsoluteFilePath(m_shapeFilePath) );
		}

		msg += inputLOC.Load(GetAbsoluteFilePath(m_inputFilePath));
		
		if( msg)
		{
			callback.SetNbStep(inputLOC.size());
			

			for(size_t i=0; i<inputLOC.size()&&msg; i++)
			{
				CGeoPoint pt(inputLOC[i].m_lon, inputLOC[i].m_lat, PRJ_WGS_84);
				bool bRect = m_boundingBox.IsRectNull() || m_boundingBox.PtInRect( inputLOC[i] );
				bool bShape = shapefile.GetNbShape()==0 || shapefile.IsInside(pt);
				if( bRect&&bShape )
				{
					outputLOC.push_back( inputLOC[i]);
				}
				
				msg += callback.StepIt();
			}

			msg += outputLOC.Save(outputFilePath);
		}
	}
	CATCH_ALL(e)
	{
		msg = SYGetMessage(*e);
		msg.ajoute( GetString( IDS_INVALID_INPUT_TASK) );
	}
	END_CATCH_ALL


	return msg;
}

