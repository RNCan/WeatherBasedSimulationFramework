#include "StdAfx.h"
#include ".\TYNExtractor.h"

#include "dailyStation.h"
#include "FileManager\DailyFile.h"
#include "FileManager\FileManagerRes.h"
#include "FileManager\NormalFile.h"
#include "SYShowMessage.h"
#include "CommonRes.h"
#include "mappingRes.h"
#include "ShapefileBase.h"
#include "Resource.h"
#include ".\gridym.h"
#include "Statistic.h"
#include "AdvancedNormalStation.h"

using namespace WEATHER;
using namespace CFL;
using namespace UtilWin;

const char* CTYNExtractor::MODEL_NAME[NB_MODEL] = {"HadCM3", "PCM", "CGCM2", "CSIRO2", "ECHam4"}; 
const char* CTYNExtractor::SRES_NAME[NB_SRES] = {"A1FI", "A2", "B2", "B1"}; 
const char* CTYNExtractor::FILE_EXT[NB_EXT] = {".tmp", ".dtr", ".pre"};
const char* CTYNExtractor::PERIOD_NAME[NB_PERIOD] = {"2001-2030", "2011-2040", "2021-2050", "2031-2060", "2041-2070", "2051-2080", "2061-2090", "2071-2100"};
const char* CTYNExtractor::OBS_FILE_NAME = "obs.clim6190.globe.lan";


CString CTYNExtractor::GetFileName( short t)
{
	ASSERT( t>=0 && t<NB_EXT);

	CString fileName;
	fileName.Format("%s.%s", OBS_FILE_NAME, FILE_EXT[t]);

	return fileName;
}

CString CTYNExtractor::GetFileName( short m, short s, short t)
{
	ASSERT( m>=0 && m<NB_MODEL);
	ASSERT( s>=0 && s<NB_SRES);
	ASSERT( t>=0 && t<NB_EXT);

	CString fileName;
	fileName.Format("%s.%s.2001-2100%s", SRES_NAME[s], MODEL_NAME[m], FILE_EXT[t]);
	return fileName;
}

//*********************************************************************
const char* CTYNExtractor::ATTRIBUTE_NAME[] = { "EXTRACT_TYPE", "MODEL", "SRES", "PERIOD", "INPUT_FILEPATH", "OUT_FILEPATH", "DELETE_OLD_DB"};//"SHAPEFILE", "BOUNDING_BOX", 
const char* CTYNExtractor::CLASS_NAME = "CRUExtractor";


CTYNExtractor::CTYNExtractor(void)
{
	if( !IsRegister( GetClassID() ) )
	{
		InitClass();
	}

	Reset();

	
}

void CTYNExtractor::InitClass(const CStringArray& option)
{
	GetParamClassInfo().m_className.LoadString( IDS_SOURCENAME_TYN_EXTRACTOR);

	CToolsBase::InitClass(option);

	ASSERT( GetParameters().GetSize() <= I_NB_ATTRIBUTE);


	CStringArray array;
	LoadStringArray( array, IDS_PROPERTIES_TYN_EXTRACTOR);
	ASSERT( array.GetSize() == NB_ATTRIBUTE);

	CStringArray extractType;
	LoadStringArray( extractType, TYN_EXTRACOR_TYPE);

	CStringArray modelName;
	for(int i=0; i<NB_MODEL; i++)modelName.Add(MODEL_NAME[i]);
	CStringArray SRESName;
	for(int i=0; i<NB_SRES; i++)SRESName.Add(SRES_NAME[i]);
	CStringArray periodName;
	for(int i=0; i<NB_PERIOD; i++)periodName.Add(PERIOD_NAME[i]);

	CString filter1 = "*.normals|*.normals|*.dailyStations|*.dailyStations||";//GetString( IDS_CMN_FILTER_DAILY);
	GetParameters().Add( CParamDef(CParamDef::COMBO, CParamDef::BY_NUMBER, ATTRIBUTE_NAME[0], array[0], extractType, "0" ) );
	GetParameters().Add( CParamDef(CParamDef::COMBO, CParamDef::BY_NUMBER, ATTRIBUTE_NAME[1], array[1], modelName, "1" ) );
	GetParameters().Add( CParamDef(CParamDef::COMBO, CParamDef::BY_NUMBER, ATTRIBUTE_NAME[2], array[2], SRESName, "0" ) );
	GetParameters().Add( CParamDef(CParamDef::COMBO, CParamDef::BY_NUMBER, ATTRIBUTE_NAME[3], array[3], periodName, "0" ) );
	GetParameters().Add( CParamDef(CParamDef::PATH, ATTRIBUTE_NAME[4], array[4], ".pre|*.pre,*.dtr,*.tmp||" ) );
	GetParameters().Add( CParamDef(CParamDef::FILEPATH, ATTRIBUTE_NAME[5], array[5], filter1 ) );
	GetParameters().Add( CParamDef(CParamDef::FILEPATH, ATTRIBUTE_NAME[6], array[6], filter1 ) );
	GetParameters().Add( CParamDef(CParamDef::BOOL, ATTRIBUTE_NAME[7], array[7]) );
}

CTYNExtractor::~CTYNExtractor(void)
{
}


CTYNExtractor::CTYNExtractor(const CTYNExtractor& in)
{
	operator=(in);
}

void CTYNExtractor::Reset()
{
	m_extractType=FROM_DB;
	m_model=CGCM2;
	m_sres=A2;
	m_period=0;
	m_TYNPath.Empty();
	m_inputFilePath.Empty();
//	m_shapeFilePath.Empty();
//	m_boundingBox.SetRectEmpty();
//	m_type=0;

	m_outputFilePath.Empty();
	m_bDeleteOldDB = true;
}

CTYNExtractor& CTYNExtractor::operator =(const CTYNExtractor& in)
{
	if( &in != this)
	{
		CToolsBase::operator =(in);
		m_model=in.m_model;
		m_sres=in.m_sres;
		m_period=in.m_period;
		m_TYNPath=in.m_TYNPath;
		m_extractType = in.m_extractType;
		m_inputFilePath = in.m_inputFilePath;
//		m_shapeFilePath = in.m_shapeFilePath;
//		m_boundingBox = in.m_boundingBox;
//		m_type = in.m_type;
		m_outputFilePath = in.m_outputFilePath;
		m_bDeleteOldDB = in.m_bDeleteOldDB;
	}

	return *this;
}

bool CTYNExtractor::operator ==(const CTYNExtractor& in)const
{
	bool bEqual = true;

	if( CToolsBase::operator !=(in))bEqual = false;
	if( m_extractType != in.m_extractType )bEqual = false;
	if( m_model!=in.m_model )bEqual = false;
	if(	m_sres!=in.m_sres )bEqual = false;
	if(	m_period!=in.m_period )bEqual = false;
	if(	m_TYNPath!=in.m_TYNPath)bEqual = false;
	if( m_inputFilePath != in.m_inputFilePath)bEqual = false;
//	if( m_shapeFilePath != in.m_shapeFilePath)bEqual = false;
//	if( m_boundingBox != in.m_boundingBox)bEqual = false;
	if( m_outputFilePath != in.m_outputFilePath)bEqual = false;
	if( m_bDeleteOldDB != in.m_bDeleteOldDB)bEqual = false;

	return bEqual;
}

bool CTYNExtractor::operator !=(const CTYNExtractor& in)const
{
	return !operator ==(in);
}


CString CTYNExtractor::GetValue(short type)const
{
	CString value;
	
	ASSERT( NB_ATTRIBUTE == 8); 
	switch(type)
	{
	case I_EXTRACT_TYPE: value.Format("%d", m_extractType);break;
	case I_MODEL: value.Format("%d", m_model);break;	
	case I_SRES: value.Format("%d", m_sres);break;
	case I_PERIOD: value.Format("%d", m_period);break;
	case I_TYN_PATH: value = m_TYNPath;break;
	case I_INPUT_DB: value = m_inputFilePath;break;
	//case I_SHAPEFILE: value = m_shapeFilePath;break;
	//case I_BOUNDINGBOX: value = m_boundingBox.ToString(m_type); break;
	case I_OUT_FILEPATH: value = m_outputFilePath; break;
	case I_DELETE_OLD_DB: value = m_bDeleteOldDB?"1":"0"; break;
	default: value = CToolsBase::GetValue(type); break;
	}

	return value;
}

void CTYNExtractor::SetValue(short type, const CString& value)
{
	ASSERT( NB_ATTRIBUTE == 8); 
	switch(type)
	{
	case I_EXTRACT_TYPE: m_extractType = atoi(value); break;
	case I_MODEL: m_model = atoi(value); break;
	case I_SRES: m_sres = atoi(value); break;
	case I_PERIOD: m_period = atoi(value); break;
	case I_TYN_PATH: m_TYNPath=value;break;
	case I_INPUT_DB: m_inputFilePath = value;break;
	//case I_SHAPEFILE: m_shapeFilePath = value;break;
	//case I_BOUNDINGBOX: m_type = m_boundingBox.FromString(value); break;
	case I_OUT_FILEPATH: m_outputFilePath=value; break;
	case I_DELETE_OLD_DB: m_bDeleteOldDB=atoi(value); break;
	default: CToolsBase::SetValue(type, value); break;
	}

}

void CTYNExtractor::GetSelection(short param, CSelectionDlgItemVector& items)const
{}
void CTYNExtractor::SetSelection(short param, const CSelectionDlgItemVector& items)
{}

bool CTYNExtractor::Compare(const CParameterBase& in)const
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CTYNExtractor& info = dynamic_cast<const CTYNExtractor&>(in);
	return operator==(info);
}

CParameterBase& CTYNExtractor::Assign(const CParameterBase& in)
{
	ASSERT(in.GetClassID() == GetClassID() );
	const CTYNExtractor& info = dynamic_cast<const CTYNExtractor&>(in);
	return operator=(info);
}

/*
#include "Statistic.h"
bool ReadData(CString filePath, CNormalsStation& normalStationT)
{
	//Get the data for each station
	CStdioFile file( filePath, CFile::modeRead);
	
	CFL::CStatistic stat[3][12];

	CNormalsData data;
	normalStationT.GetData(data);

	int section = -1;
	CString line;
	while( file.ReadString(line) )
	{
		if( section == -1)
		{
			//find minimum temperature
			if( line.Find("TEMP MINIMA PROM") != -1)
				section = 0;
				
			//find maximum temperature
			if( line.Find("TEMP MAXIMA PROM") != -1)
				section = 1;
		
			//Find precipitation
			if( line.Find("LLUVIA TOTAL MEN") != -1)
				section = 2;
		}
		else 
		{
			if( line.Find("MINIMA")==-1 )
			{
				if( !line.IsEmpty() )
				{
					int year=atoi(line.Mid(25,4));
					ASSERT( year > 1800 && year < 2100);

					if( year >= 1961 && year <= 1990)
					{
						for(int m=0; m<12; m++)
						{
							CString elem = line.Mid(29+7*m, 7);
							elem.Trim();
							if( !elem.IsEmpty() )
							{
								stat[section][m]+=atof(elem);
							}
						}
					}
				}
			}
			else section = -1;
		}
	}

		
	bool bRep[2]={true, true};
	for(int m=0; m<12; m++)
	{
		if( stat[0][m][CFL::NB_VALUE] < 10 && stat[1][m][CFL::NB_VALUE] < 10)
			bRep[0]=false;
		if( stat[2][m][CFL::NB_VALUE] < 10)
			bRep[1]=false;
	}

	for(int m=0; m<12; m++)
	{
		if( bRep[0] )
		{
			data[m][NORMAL_DATA::TMIN_MN] = (float)stat[0][m][CFL::MEAN];
			data[m][NORMAL_DATA::TMAX_MN] = (float)stat[1][m][CFL::MEAN];
		}
		else
		{
			for(int i=NORMAL_DATA::TMIN_MN; i<=NORMAL_DATA::TACF_B2; i++)
				data[m][i] = -999;
		}
		
		
		if( bRep[1] )
		{
			data[m][NORMAL_DATA::PRCP_TT] = (float)stat[2][m][CFL::MEAN];
			data[m][NORMAL_DATA::PRCP_SD] = (float)stat[2][m][CFL::COEF_VAR];
		}
		else
		{
			for(int i=NORMAL_DATA::PRCP_TT; i<=NORMAL_DATA::PRCP_SD; i++)
				data[m][i] = -999;
		}
	}

	if( bRep[0] || bRep[1] )
		normalStationT.SetData(data);

	return bRep[0] || bRep[1];
}
ERMsg CTYNExtractor::Execute(CFL::CCallback& callback)
{
	ERMsg msg;

	//Comparer les données normales
	CString outputFilePath(GetAbsoluteFilePath(m_outputFilePath));
	UtilWin::SetFileExtension( outputFilePath, ".Normals");

	callback.AddMessage( GetString(IDS_CREATE_DATABASE) );
	callback.AddMessage(outputFilePath, 1);
	callback.AddMessage("");

	CLocArray stationsList;
	stationsList.Load("D:\\travail\\Cuauhtémoc\\Data Weather\\CoordenateStations.loc");
	CLocArray stationsList2;
	stationsList2.Load("D:\\travail\\Cuauhtémoc\\LOC\\CoordenateStations.loc");
	


	CNormalsDatabase inputDB;
	CNormalsDatabase outputDB;

	msg += inputDB.Open( GetAbsoluteFilePath(m_inputFilePath), CDailyDatabase::modeReadOnly);

	CNormalsDatabase::DeleteDatabase(GetAbsoluteFilePath(m_outputFilePath));
	msg += outputDB.Open( GetAbsoluteFilePath(m_outputFilePath), CDailyDatabase::modeEdit);

	
	CStringArray filesList;
	UtilWin::GetFileList(filesList, "D:\\travail\\Cuauhtémoc\\Data Weather\\*.txt", true, true);

	callback.SetNbStep(0, filesList.GetSize(), 1);
	

	for(int i=0; i<filesList.GetSize()&&msg; i++)
	{
		//find the station coord
		int index = stationsList.FindByName(UtilWin::GetFileTitle(filesList[i]));
		if( index == -1)
		{
			callback.AddMessage("station introuvable : " + UtilWin::GetFileTitle(filesList[i]) );
			continue;
		}

		ASSERT(index >= 0);
		CStation station = stationsList[index];
		station.SetName( stationsList2[index].GetName() + "-" + UtilWin::GetFileTitle(filesList[i]));


		CSearchResultVector resultT;
//		CSearchResultVector resultP;
		VERIFY(inputDB.Match(station, 1, resultT, HAVE_TEMPERATURE));
//		VERIFY(inputDB1.Match(station, 1, resultP, HAVE_PRECIPITATION));

		if( resultT[0].m_distance/1000 < 600)
		{
			CNormalsStation normalStationT;
			VERIFY(inputDB.GetAt(resultT[0].m_index, normalStationT));

			((CStation&)normalStationT)= station;
			if( ReadData(filesList[i], normalStationT) )
			{
				msg = outputDB.Add( normalStationT);
			}
		}
		else
		{
			callback.AddMessage("la station a été éliminé : " + station.GetName() );
		}

		msg += callback.StepIt();
	}
	
	outputDB.Close();
	if( msg)
	{
		msg = outputDB.Open(outputFilePath);
	}
	
	return msg;
}
*/

/*
void ReadData(CString filePath, float data[300][4][12], bool dataAvalable[300]  )
{
	//Get the data for each station
	CStdioFile file( filePath, CFile::modeRead);
	
	//CFL::CStatistic stat[3][12];


	int section = -1;
	CString line;
	while( file.ReadString(line) )
	{
		if( section == -1)
		{
			//find minimum temperature
			if( line.Find("TEMP MINIMA PROM") != -1)
				section = 0;

			if( line.Find("TEMP MEDIA MENS") != -1)
				section = 1;

			//find maximum temperature
			if( line.Find("TEMP MAXIMA PROM") != -1)
				section = 2;
		
			//Find precipitation
			if( line.Find("LLUVIA TOTAL MEN") != -1)
				section = 3;
		}
		else 
		{
			if( line.Find("MINIMA")==-1 )
			{
				if( !line.IsEmpty() )
				{
					int year=atoi(line.Mid(25,4));
					ASSERT( year > 1800 && year < 2100);

					//if( year >= 1960 && year <= 1990)
					{
						for(int m=0; m<12; m++)
						{
							CString elem = line.Mid(29+7*m, 7);
							elem.Trim();
							if( !elem.IsEmpty() )
							{
								data[year-1800][section][m]=(float)atof(elem);
								dataAvalable[year-1800]=true;
							}
						}
					}
				}
			}
			else section = -1;
		}
	}
}

ERMsg CTYNExtractor::Execute(CFL::CCallback& callback)
{
	ERMsg msg;

	//Comparer les données normales
	CString outputFilePath(GetAbsoluteFilePath(m_outputFilePath));
	UtilWin::SetFileExtension( outputFilePath, ".csv");

	callback.AddMessage( GetString(IDS_CREATE_DATABASE) );
	callback.AddMessage(outputFilePath, 1);
	callback.AddMessage("");

	CLocArray stationsList;
	stationsList.Load("D:\\travail\\Cuauhtémoc\\Data Weather\\CoordenateStations.loc");
//	CLocArray stationsList2;
//	stationsList2.Load("D:\\travail\\Cuauhtémoc\\LOC\\CoordenateStations.loc");
	
	CStdioFile outputFile( GetAbsoluteFilePath(m_outputFilePath), CFile::modeCreate|CFile::modeWrite);
	outputFile.WriteString("StationID,Year,Variable,January,February,March,April,May,June,July,August,September,October,November,December\n");
	
	CStringArray filesList;
	UtilWin::GetFileList(filesList, "D:\\travail\\Cuauhtémoc\\Data Weather\\*.txt", true, true);

	callback.SetNbStep(0, filesList.GetSize(), 1);
	

	for(int i=0; i<filesList.GetSize()&&msg; i++)
	{
		//find the station coord
		int index = stationsList.FindByName(UtilWin::GetFileTitle(filesList[i]));
		if( index == -1)
		{
			callback.AddMessage("station introuvable : " + UtilWin::GetFileTitle(filesList[i]) );
			continue;
		}

		ASSERT(index >= 0);
		float data[300][4][12] = {0};
		bool dataAvalable[300] = {0};
		for(int x=0; x<300; x++)
			for(int y=0; y<4; y++)
				for(int z=0; z<12; z++)
					data[x][y][z] = -999;

		ReadData(filesList[i], data, dataAvalable);
		CString fileTitle = UtilWin::GetFileTitle(filesList[i]);

		for(int x=0; x<300; x++)
		{
			if( dataAvalable[x] )
			{
				for(int y=0; y<4; y++)
				{
					static const char* VAR_NAME[4] = {"TMIN","TAVG", "TMAX", "PRCP"};

					CString line;
					line.Format("%s, %4d,%4s", fileTitle, 1800+x, VAR_NAME[y]);
					for(int z=0; z<12; z++)
					{
						CString tmp;
						if( data[x][y][z] > -999)
							tmp.Format( ",%.4f", data[x][y][z] );
						else tmp = ",.";

						line += tmp;
					}

					outputFile.WriteString(line + "\n");
				}
			}
		}

		msg += callback.StepIt();
	}
	
	
	return msg;
}
*/
/*
#include "mapbilfile.h"
ERMsg CTYNExtractor::Execute(CFL::CCallback& callback)
{
	ERMsg msg;

	char* NAME[8] = 
{	"HadCM2_IS92a_map_SAT_SON_19601990_20702100.cdl",
	"HadCM2_IS92a_map_SAT_MAM_19601990_20702100.cdl",
	"HadCM2_IS92a_map_SAT_JJA_19601990_20702100.cdl",
	"HadCM2_IS92a_map_SAT_DJF_19601990_20702100.cdl",
	"HadCM2_IS92a_map_P_SON_19601990_20702100.cdl",
	"HadCM2_IS92a_map_P_MAM_19601990_20702100.cdl",
	"HadCM2_IS92a_map_P_JJA_19601990_20702100.cdl",
	"HadCM2_IS92a_map_P_DJF_19601990_20702100.cdl"
};

	const short NB_COL = 96;
	const short NB_ROW = 73;
	for(int i=0; i<8; i++)
	{
		float data[NB_ROW][NB_COL] = {0};

		CString path = "D:\\travail\\Cuauhtémoc\\Climatic Change\\";
		CString filePath1 = path + NAME[i];
		CString filePath2 = path + NAME[i];
		UtilWin::SetFileExtension( filePath2, ".bil");

		CStdioFile file(filePath1, CFile::modeRead);

		CString line;
		for(int j=0; j<89; j++)
			file.ReadString(line);


		
		callback.SetNbStep(0, NB_COL*NB_ROW, 1);
		

		int p=0;
		while( file.ReadString(line) )
		{
			line.Replace(',',' ');
			double value;
			while( UtilWin::StringTokenizerReal(line, value) )
			{
				int x = int(p/NB_COL);
				int y = int(p%NB_COL);

				data[x][y] = (float)value;
				p++;
			}
		}

		file.Close();

		



		CMapBilFile map;
			
		//map.CreateMap(filePath2);
		map.SetCellType(CMapBinaryFile::FLOAT);
		map.SetCellSizeX(3.75);
		map.SetCellSizeY(2.5);
		map.SetNbCols(NB_COL);
		map.SetNbRows(NB_ROW);
		map.SetNoData(-9999);
		map.SetProjection( CProjection(CProjection::GEO, CProjParam(), CSpheroids(), CProjectionBase::UT_DEGREES) );
		//map.SetBoundingBox( CGeoRect(-1.875, -91.25, 358.125, 91.25, CProjection::GEO) );
		map.SetBoundingBox( CGeoRect(-181.875, -91.25, 178.125, 91.25, CProjection::GEO) );
		
		
		msg += map.Open( filePath2, CGeoFileInterface::modeWrite );

		if( !msg)
			return msg;

		for(int x=0; x<NB_ROW; x++)
		{
			for(int y=0; y<NB_COL; y++)	
			{
				VERIFY( map.WriteCell(data[x][(y+NB_COL/2)%NB_COL]) );
			}
		}
		map.Close(true);
	}
}
*/
/*
#include "mapbilfile.h"
#include "UtilTime.h"
ERMsg CTYNExtractor::Execute(CFL::CCallback& callback)
{
	ERMsg msg;

	char* NAME[8] = 
{	"HadCM2_IS92a_map_SAT_DJF_19601990_20702100.cdl",
	"HadCM2_IS92a_map_SAT_MAM_19601990_20702100.cdl",	
	"HadCM2_IS92a_map_SAT_JJA_19601990_20702100.cdl",
	"HadCM2_IS92a_map_SAT_SON_19601990_20702100.cdl",
	"HadCM2_IS92a_map_P_DJF_19601990_20702100.cdl",
	"HadCM2_IS92a_map_P_MAM_19601990_20702100.cdl",
	"HadCM2_IS92a_map_P_JJA_19601990_20702100.cdl",
	"HadCM2_IS92a_map_P_SON_19601990_20702100.cdl"
};

	CMapBilFile maps[8];
	for(int i=0; i<8; i++)
	{
		CString path = "D:\\travail\\Cuauhtémoc\\Climatic Change\\";
		CString filePath = path + NAME[i];

		VERIFY(maps[i].Open( filePath, CGeoFileInterface::modeDirectAccess|CGeoFileInterface::modeRead ));
	}
	
	//apply climatic change on normal database
	CString outputFilePath(GetAbsoluteFilePath(m_outputFilePath));
	UtilWin::SetFileExtension( outputFilePath, ".Normals");

	callback.AddMessage( GetString(IDS_CREATE_DATABASE) );
	callback.AddMessage(outputFilePath, 1);
	callback.AddMessage("");

//	CLocArray stationsList;
//	stationsList.Load("D:\\travail\\Cuauhtémoc\\Data Weather\\CoordenateStations.loc");
//	CLocArray stationsList2;
//	stationsList2.Load("D:\\travail\\Cuauhtémoc\\LOC\\CoordenateStations.loc");
	


	CNormalsDatabase inputDB;
	CNormalsDatabase outputDB;

	msg += inputDB.Open( GetAbsoluteFilePath(m_inputFilePath), CDailyDatabase::modeReadOnly);

	CNormalsDatabase::DeleteDatabase(GetAbsoluteFilePath(m_outputFilePath));
	msg += outputDB.Open( GetAbsoluteFilePath(m_outputFilePath), CDailyDatabase::modeEdit);

	
//	CStringArray filesList;
//	UtilWin::GetFileList(filesList, "D:\\travail\\Cuauhtémoc\\Data Weather\\*.txt", true, true);

	callback.SetNbStep(0, inputDB.GetSize(), 1);
	

	for(int i=0; i<inputDB.GetSize()&&msg; i++)
	{
		//find the station coord
		CNormalsStation station;
		inputDB.GetAt(i, station);

		bool bValid = true;
		float values[8]={0}; 
		for(int i=0; i<8; i++)
		{
			values[i]=maps[i].GetNearestCell(station.GetCoord());
			if( values[i] <= maps[i].GetNoData() )
				bValid=false;
		}

		if( bValid )
		{
			CNormalsData data = station.GetData();
			for(int m=11; m<23; m++)
			{
				int index = ((m-11)/3);
				int month = m%12;
				if( data.HaveTemperature() )
				{
					data[month][NORMAL_DATA::TMIN_MN] += values[index];
					data[month][NORMAL_DATA::TMAX_MN] += values[index];
				}
				if( data.HavePrecipitation() )
				{
					data[month][NORMAL_DATA::PRCP_TT] += (values[index]*CFL::GetNbDayPerMonth(month));
					if( data[month][NORMAL_DATA::PRCP_TT] < 0)
						data[month][NORMAL_DATA::PRCP_TT] = 0;
				}
			}
	
			station.SetData(data);
			msg = outputDB.Add( station);
		}
		else
		{
			callback.AddMessage("la station a été éliminé : " + station.GetName() );
		}

		msg += callback.StepIt();
	}
	
	outputDB.Close();
	if( msg)
	{
		msg = outputDB.Open(outputFilePath);
	}

	return msg;
}
*/


/*
#include ".\gridym.h"
#include "mapbilfile.h"
ERMsg CTYNExtractor::Execute(CFL::CCallback& callback)
{
	ERMsg msg;

	CGridYM test;
	test.Open("D:\\travail\\Cuauhtémoc\\Climatic Change\\TYN\\A2.CGCM2.2001-2030.pre", CFile::modeRead);
	//test.Open("D:\\travail\\Cuauhtémoc\\Climatic Change\\TYN\\hd.had3.a2m.2080s.pre", CFile::modeRead);
	//test.Open("D:\\travail\\Cuauhtémoc\\Climatic Change\\TYN\\halfdeg.elv", CFile::modeRead);
	

	CMapBilFile map;
		
	//map.CreateMap(filePath2);
	map.SetCellType(CMapBinaryFile::FLOAT);
	map.SetCellSizeX(0.5);
	map.SetCellSizeY(0.5);
	map.SetNbCols(test.GetSizeX());
	map.SetNbRows(test.GetSizeY());
	map.SetNoData(test.GetNoData());
	map.SetProjection( CProjection(CProjection::GEO, CProjParam(), CSpheroids(), CProjectionBase::UT_DEGREES) );
	map.SetBoundingBox( test.GetBoundingBox() );
	
	
	msg += map.Open( "D:\\travail\\Cuauhtémoc\\Climatic Change\\TYN\\test2.bil", CGeoFileInterface::modeWrite );
	if( !msg)
		return msg;

	CBoxYM box;
	test.ReadCell(CGeoPoint(1.31666, 52.6333, CProjection::GEO), box);
	test.ReadCell(CPoint(362, 285), box);
	float v1 = test.ReadCell(CGeoPoint(1.31666, 52.6333, CProjection::GEO), 0, 6);
	float v2 = test.ReadCell(CPoint(362, 285), 0, 6);

	callback.SetNbStep(0, test.GetSizeX()*test.GetSizeY(), 1);
	

	for(int y=test.GetSizeY()-1; y>=0&&msg; y--)
	{
		for(int x=0; x<test.GetSizeX()&&msg; x++)	
		{
			CFL::CStatistic stat;
			for(int year=0; year<test.GetNbYears(); year++)
			{
				for(int m=0; m<12; m++)
				{
					float v = test.ReadCell(CPoint(x,y), year, m);
					if( v > test.GetNoData() )
						stat+=v;
				}
			}

			if( stat[CFL::NB_VALUE] > 0)
				VERIFY( map.WriteCell((float)stat[CFL::MEAN]) );
			else VERIFY( map.WriteCell(map.GetNoData()) );

			msg += callback.StepIt();
		}
	}

	test.Close();
	map.Close(true);
	
	//test.Close();
	
		//Creation d'un fihcier de match station pour un loc en particulier

	return msg;
}
*/

#include "oldUtilTime.h"
#include "MapBinaryFile.h"
#include "WeatherGenerator.h"
ERMsg CTYNExtractor::Execute(CFL::CCallback& callback)
{
	ERMsg msg;

	const char * EXT_NAME[2] = {"mly", "dly"};
	const char * VAR_NAME[3] = {"tmp", "tmn","pre"};

	callback.SetCurrentDescription("Extraction");
	callback.SetNbStep(0,31143,1);
	
	
	m_inputFilePath.Trim(" ");
	m_outputFilePath.Trim(" ");

	CString outputFilePath(GetAbsoluteFilePath(m_outputFilePath));
	//UtilWin::SetFileExtension( outputFilePath, ".DailyStations");
	UtilWin::SetFileExtension( outputFilePath, ".Normals");
	
	CString inputFilePath(GetAbsoluteFilePath(m_inputFilePath));
	
	
	CTGInput TGInput;
	TGInput.SetNbNormalStation(8);
	TGInput.SetCategory("T,P,H,WS");

	CWeatherGenerator tempGen;
	tempGen.SetTGInput(TGInput);

	msg += tempGen.SetNormalDBFilePath( "C:\\Travail\\IsabelChuine\\weather\\Europe 1971-2000 (CRU TS12).Normals" );

	
	CMapBinaryFile elev;
	msg+=elev.Open("C:\\ClimaticChange\\10min-elev.flt", CGeoFileInterface::modeRead|CGeoFileInterface::modeDirectAccess);
	if(!msg)
		return msg;


	/*CMapBinaryFile grid;
	grid.SetProjection( CProjection (CProjection::GEO) );
	grid.SetNbCols(TYN_SC1.GetSizeX());
	grid.SetNbRows(TYN_SC1.GetSizeY());
	grid.SetBoundingBox(TYN_SC1.GetBoundingBox());
	grid.SetNoData(TYN_SC1.GetNoData());
	grid.SetCellSizeX( TYN_SC1.GetCellSizeX() );
	grid.SetCellSizeY( TYN_SC1.GetCellSizeY() );
	msg += grid.Open(m_inputFilePath+"10min-elev.flt", CGeoFileInterface::modeRead|CGeoFileInterface::modeDirectAccess);
	*/
	//msg += grid.Open(m_inputFilePath+"10min-elev.flt", CGeoFileInterface::modeWrite);

/*	int nbCell=0;
	if( msg)
	{
		int curN=0;
		for(int y=0; y<TYN_SC1.GetSizeY()&&msg; y++)
		{
			for(int x=0; x<TYN_SC1.GetSizeX()&&msg; x++)
			{
				CPoint pt(x,TYN_SC1.GetSizeY()-y-1);
				if( TYN_SC1.HaveValue(pt) )
				{
					float value = TYN_SC1.ReadCell(pt,0,0)*1000;
					nbCell++;
					
					if (value > TYN_SC1.GetNoData())
						msg+=grid.WriteCell(value);
					else  msg+=grid.WriteCell(0);
				}
				else
				{
					msg+=grid.WriteCell(TYN_SC1.GetNoData());
				}
			}
		}
	}

	grid.Close(true);
*/
	
/*	for(int y=0; y<6; y++)
	{
		int year=1995+y;
		for(int i=0; i<3; i++)
		{
			
			CString filePath1;
			CString filePath2;
			filePath1.Format("%sold\\obs.1901-2000_%s_%d_%s.fit", m_inputFilePath, VAR_NAME[i], year, EXT_NAME[i==2?0:1]);
			filePath2.Format("%sNew\\obs.1901-2000_%s_%d_%s.fit", m_inputFilePath, VAR_NAME[i], year, EXT_NAME[i==2?0:1]);
			CStdioFile file1(filePath1, CFile::modeRead);
			CStdioFile file2(filePath2, CFile::modeWrite|CFile::modeCreate);

			CString line;
			for(int i=0; i<4; i++)
			{
				file1.ReadString(line);
				file2.WriteString(line+"\n");
			}
			//file2.WriteString("Phenofit soil input file created on 2008.1.25 at 15:02:16\n");
			//file2.WriteString("Variable = water holding capacity  (mm)\n");
			//file2.WriteString("Origin   =  water-holding capacity in the GISS GCM (Model II) from RW Webb, CE Rosenzweig, ER Levine SPECIFYING LAND SURFACE CHARACTERISTICS IN GENERAL CIRCULATION MODELS: SOIL PROFILE DATA SET AND DERIVED WATER-HOLDING CAPACITIES \n");
			//file2.WriteString("Columns  = lat(decimal degree), lon (decimal degree), water holding capacity (mm)\n");


			while( file1.ReadString(line) )
			{
				int pos = 0;
				double lat = ToDouble( line.Tokenize("\t", pos) );
				double lon = ToDouble( line.Tokenize("\t", pos) );

				CGeoPoint pt(lon-TYN_SC1.GetCellSizeX(),lat,CProjection::GEO);
				CPoint xy = TYN_SC1.CGeoPoint2CPoint( pt );
				pt.m_x += xy.x*TYN_SC1.GetCellSizeX()/TYN_SC1.GetSizeX();
				pt.m_y -= (TYN_SC1.GetSizeY()-xy.y-1)*TYN_SC1.GetCellSizeY()/TYN_SC1.GetSizeY();
				xy = TYN_SC1.CGeoPoint2CPoint( pt );
				pt = TYN_SC1.CPoint2CGeoPoint( xy );
			
				//float value = grid.ReadCell(xy.x, TYN_SC1.GetSizeY()-xy.y-1);

				line.Format("%.2lf\t%.2lf\t%s\n",pt.m_lat(),pt.m_lon(),line.Mid(pos));
				file2.WriteString(line);
			}

			file1.Close();
			file2.Close();
		}
	}

	return msg;
*/
/*	CStdioFile fileTest;

	CString filePath;
//	filePath.Format("%sobs.1901-2000_%s_%d_%s.fit", m_inputFilePath, VAR_NAME[0], 1995, EXT_NAME[0]);
	filePath.Format("%sobs.1901-2000_WHC.fit",m_inputFilePath);
	VERIFY( fileTest.Open(filePath, CFile::modeRead) );
	CString tmp;
	for(int j=0; j<4; j++)
		fileTest.ReadString(tmp);

	CFloatMatrix matrix;
	matrix.SetSize(TYN_SC1.GetSizeX(), TYN_SC1.GetSizeY());
	for(int x=0;x<TYN_SC1.GetSizeX(); x++)
		for(int y=0; y<TYN_SC1.GetSizeY(); y++)
			matrix[x][y] = TYN_SC1.GetNoData();

	if( msg)
	{
		for(int n=0; n<31143; n++)
		{
			CString line;
			
			if( n==31142)
			{
				int i;
				i=0;
			}
			fileTest.ReadString(line);
				
			int pos=0;
			double lat = ToDouble( line.Tokenize("\t", pos) );
			double lon = ToDouble( line.Tokenize("\t", pos) )-TYN_SC1.GetCellSizeX();
			float value = ToFloat( line.Tokenize("\t", pos) );

			CGeoPoint pt(lon,lat,CProjection::GEO);
			CPoint xy = TYN_SC1.CGeoPoint2CPoint( pt );
			pt.m_x += xy.x*TYN_SC1.GetCellSizeX()/TYN_SC1.GetSizeX();
			pt.m_y -= (TYN_SC1.GetSizeY()-xy.y-1)*TYN_SC1.GetCellSizeY()/TYN_SC1.GetSizeY();
			xy = TYN_SC1.CGeoPoint2CPoint( pt );

			matrix[xy.x][xy.y] = value;
		}
	}

	msg += grid.Open(m_inputFilePath+"10min-WHC.flt", CGeoFileInterface::modeWrite);

	nbCell=0;
	if( msg)
	{
		int curN=0;
		for(int y=0; y<TYN_SC1.GetSizeY()&&msg; y++)
		{
			for(int x=0; x<TYN_SC1.GetSizeX()&&msg; x++)
			{
				if( TYN_SC1.ReadCell(CPoint(x,TYN_SC1.GetSizeY()-y-1),0,0) > -999)
					nbCell++;

				//if( matrix[x][TYN_SC1.GetSizeY()-y-1] != 0)
				msg+=grid.WriteCell(matrix[x][TYN_SC1.GetSizeY()-y-1]);
				//else msg+=grid.WriteCell(grid.GetNoData());
			}
		}
	}

	grid.Close();
*/	

	/*if( m_bDeleteOldDB )
	{
		CString tmp;
		tmp.LoadString(IDS_DELETE_FILE);
		callback.AddMessage(tmp);
		callback.AddMessage(outputFilePath, 1);
		msg = CDailyDatabase::DeleteDatabase(outputFilePath);
		if(!msg)return msg;
	}

	CDailyDatabase dailyDB;
	dailyDB.Open(outputFilePath, CDailyDatabase::modeWrite);
*/
	if( m_bDeleteOldDB )
		msg = CNormalsDatabase::DeleteDatabase(outputFilePath);

	CNormalsDatabase normalDB;
	normalDB.SetBeginYear(1971);
	normalDB.SetEndYear(2000);
	normalDB.Open(outputFilePath, CDailyDatabase::modeEdit);

	CStdioFile file[30][3];

	for(int y=0; y<30; y++)
	{
		int year=1971+y;
		for(int i=0; i<3; i++)
		{
			CString filePath;
			//if (i==0 || i==1)
			//	filePath.Format("%sobs.1901-2000_%s_%d_%s.fit", m_inputFilePath, VAR_NAME[i], year, EXT_NAME[i==2?0:1]);
			//else 
			filePath.Format("%sobs.1901-2000_%s_%d_%s.fit", m_inputFilePath, VAR_NAME[i], year, EXT_NAME[0]);

			VERIFY( file[y][i].Open(filePath, CFile::modeRead) );
			CString tmp;
			for(int j=0; j<4; j++)
				file[y][i].ReadString(tmp);
		}
	}
	
	
	for(int n=0; n<31143&&msg; n++)
	{
		CNormalsStation station;
		//CYearPackage data;

		CStatistic stats[12][3];
		for(int y=0; y<30&&msg; y++)
		{
			int year=1971+y;
			CString line[3];
			for(int i=0; i<3; i++)
				file[y][i].ReadString(line[i]);
			
			int pos[3]={0};
			double lat[3] = {0};
			double lon[3] = {0};

			for(int i=0; i<3; i++)
			{
				lat[i] = ToDouble( line[i].Tokenize("\t", pos[i]) );
				lon[i] = ToDouble( line[i].Tokenize("\t", pos[i]) );
				ASSERT( lat[i] == lat[0] );
				ASSERT( lon[i] == lon[0] );
			}

			for(int m=0; m<12; m++)
			{
				for(int i=0; i<3; i++)
					stats[m][i] += ToFloat( line[i].Tokenize("\t", pos[i]) );
					
			}

				//dailyData[jd][DAILY_DATA::TMIN] = value[1];
				//dailyData[jd][DAILY_DATA::TMAX] = 2*value[0]-value[1];
				//dailyData[jd][DAILY_DATA::PRCP] = value[2];
			//}

			//data.AddYear(year, dailyData);
			if( y==0 )
			{
				station.SetLat(lat[0]);
				station.SetLon(lon[0]);
			}
			else
			{
				ASSERT( station.GetLat() == lat[0]);
				ASSERT( station.GetLon() == lon[0]);
			}
		}

		CGeoPoint pt(station.GetLon()-elev.GetCellSizeX(), station.GetLat()-elev.GetCellSizeY(), CProjection::GEO);
		CPoint xy;
		elev.CartoCoordToColRow( pt, xy );
		pt.m_x += xy.x*elev.GetCellSizeX()/elev.GetNbCols();
		pt.m_y += (elev.GetNbRows()-xy.y-1)*elev.GetCellSizeY()/elev.GetNbRows();
		//xy = elev.CGeoPoint2CPoint( pt );
		elev.CartoCoordToColRow( pt, xy );

		//CGeoPoint pt(station.GetLon(), station.GetLat(), CProjection::GEO);
		//CPoint xy = TYN_SC1.CGeoPoint2CPoint( pt );

		//CGeoPoint pt(station.GetCoord());
		//CPoint xy;
		
		//pt.m_x += xy.x*TYN_SC1.GetCellSize
		
		elev.ColRowToCartoCoord( xy, pt );// get the center of the cell now
		float e = elev.ReadCell(xy.x,xy.y);
		ASSERT( e != -999);
			

		
		station.SetLat( pt.m_y );
		station.SetLon( pt.m_x );
		station.SetElev(int(e));
		CString name;
		name.Format("%03d-%03d", xy.x+1, xy.y+1);
		station.SetName( name );
		name.Format("lat=%.3lf lon=%.3lf", pt.m_y, pt.m_x);
		station.SetID(name);
		//station.AddPakage(data);

		//dailyDB.Add(station);
		//CAdvancedNormalStation normal;
		//normal.FromDaily(station, 10);

		CLocStation target(station);
		tempGen.SetTarget(target );

		//replace some varaible
		CNormalsData Ndata;
		tempGen.GetNormalData(Ndata, CFL::CCallback() );
		for(int m=0; m<12; m++)
		{
			Ndata[m][NORMAL_DATA::TMIN_MN] = (float)stats[m][1][CFL::MEAN];
			Ndata[m][NORMAL_DATA::TMAX_MN] = (float)(2*stats[m][0][MEAN]-stats[m][1][MEAN]);
			Ndata[m][NORMAL_DATA::PRCP_TT] = (float)stats[m][2][MEAN];
			Ndata[m][NORMAL_DATA::PRCP_SD] = (float)stats[m][2][COEF_VAR];
		}
		
		//CNormalsStation normal;
		station.SetData(Ndata);

		normalDB.Add(station);

		msg+=callback.StepIt();
	}
			

	//dailyDB.Close();

	//dailyDB.Open(outputFilePath, CDailyDatabase::modeRead, callback);
	
	normalDB.Close();
	normalDB.Open(outputFilePath, CNormalsDatabase::modeReadOnly);

	return msg;

}

/*
//je n'ai pas les données mensuelles pour toute la période
#include "UtilTime.h"
ERMsg CTYNExtractor::Execute(CFL::CCallback& callback)
{
	ERMsg msg;

	const char * EXT_NAME[2] = {"mly", "dly"};
	const char * VAR_NAME[3] = {"tmp", "tmn","pre"};

	callback.SetCurrentDescription("Extraction");
	callback.SetNbStep(0,31143,1);
	
	
	m_inputFilePath.Trim(" ");
	m_outputFilePath.Trim(" ");

	CString outputFilePath(GetAbsoluteFilePath(m_outputFilePath));
	UtilWin::SetFileExtension( outputFilePath, ".Normals");
	
	CString inputFilePath(GetAbsoluteFilePath(m_inputFilePath));
	
	if( m_bDeleteOldDB )
	{
		CString tmp;
		tmp.LoadString(IDS_DELETE_FILE);
		callback.AddMessage(tmp);
		callback.AddMessage(outputFilePath, 1);
		msg = CNormalsDatabase::DeleteDatabase(outputFilePath);
		if(!msg)return msg;
	}

	CNormalsDatabase normalDB;
	normalDB.Open(outputFilePath, CDailyDatabase::modeWrite);

	
	CGridYM TYN_SC1;
	TYN_SC1.Open(m_inputFilePath+"10min-elv.txt", CFile::modeRead);

	CStdioFile file[6][3];

	for(int y=0; y<6; y++)
	{
		int year=1995+y;
		for(int i=0; i<3; i++)
		{
			CString filePath;
			filePath.Format("%sobs.1901-2000_%s_%d_%s.fit", m_inputFilePath, VAR_NAME[i], year, EXT_NAME[0]);
			VERIFY( file[y][i].Open(filePath, CFile::modeRead) );
			CString tmp;
			for(int j=0; j<4; j++)
				file[y][i].ReadString(tmp);
		}
	}
	
	
	for(int n=0; n<31143; n++)
	{
		CNormalsStation station;
		
		for(int y=0; y<6; y++)
		{
			int year=1995+y;
			CString line[3];
			for(int i=0; i<3; i++)
				file[y][i].ReadString(line[i]);
			
			int pos[3]={0};
			double lat[3] = {0};
			double lon[3] = {0};

			for(int i=0; i<3; i++)
			{
				lat[i] = ToDouble( line[i].Tokenize("\t", pos[i]) );
				lon[i] = ToDouble( line[i].Tokenize("\t", pos[i]) );
				ASSERT( lat[i] == lat[0] );
				ASSERT( lon[i] == lon[0] );
			}

			CNormalsData data;
			
			float value[3]={0};
			for(int m=0; m<12; m++)
			{
				for(int i=0; i<3; i++)
					value[i] = ToFloat( line[i].Tokenize("\t", pos[i]) );


				data[m][NORMAL_DATA::TMIN_MN] = value[0]-value[1]/2.0;
				data[m][NORMAL_DATA::TMAX_MN] = value[0]+value[1]/2.0;
				data[m][NORMAL_DATA::PRCP_TT] = value[2];
			}

			data.AddYear(year, dailyData);
			if( y==0 )
			{
				station.SetLat(lat[0]);
				station.SetLon(lon[0]);
			}
			else
			{
				ASSERT( station.GetLat() == lat[0]);
				ASSERT( station.GetLon() == lon[0]);
			}
		}

		CGeoPoint pt(station.GetLon()-TYN_SC1.GetCellSizeX(), station.GetLat(), CProjection::GEO);
		CPoint xy = TYN_SC1.CGeoPoint2CPoint( pt );
		pt.m_x += xy.x*TYN_SC1.GetCellSizeX()/TYN_SC1.GetSizeX();
		pt.m_y -= (TYN_SC1.GetSizeY()-xy.y-1)*TYN_SC1.GetCellSizeY()/TYN_SC1.GetSizeY();
		xy = TYN_SC1.CGeoPoint2CPoint( pt );

		float elev = TYN_SC1.ReadCell(pt, 0, 0);
		if( elev == -999)
			elev = 0;

		pt = TYN_SC1.CPoint2CGeoPoint( xy );// get the center of the cell now
		
		station.SetLat( pt.m_y );
		station.SetLon( pt.m_x );
		station.SetElev(int(elev*1000));
		CString name;
		name.Format("%03d-%03d", xy.x, xy.y);
		station.SetName( name );
		name.Format("lat=%.2lf lon=%.2lf", pt.m_y, pt.m_x);
		station.SetID(name);
		station.AddPakage(data);

		dailyDB.Add(station);
		callback.StepIt();
	}
			

	dailyDB.Close();

	dailyDB.Open(outputFilePath, CDailyDatabase::modeRead, callback);

	return msg;

}
*/
/*ERMsg CTYNExtractor::Execute(CFL::CCallback& callback)
{
	ERMsg msg;
	
	m_inputFilePath.Trim(" ");
	m_outputFilePath.Trim(" ");

	CString outputFilePath(GetAbsoluteFilePath(m_outputFilePath));
	UtilWin::SetFileExtension( outputFilePath, ".Normals");
	
	CString inputFilePath(GetAbsoluteFilePath(m_inputFilePath));

	if( outputFilePath.CompareNoCase( inputFilePath )==0 )
	{
		msg.ajoute(GetString(IDS_CMN_SAME_NAMES));
		return msg;
	}

	if( m_bDeleteOldDB )
	{
		CString tmp;
		tmp.LoadString(IDS_DELETE_FILE);
		callback.AddMessage(tmp);
		callback.AddMessage(outputFilePath, 1);
		msg = CNormalsDatabase::DeleteDatabase(outputFilePath,  callback);
		if(!msg)return msg;
	}

	if( m_extractType == FROM_DB)
	{
		CString extention = UtilWin::GetFileExtension(m_inputFilePath);

		if( extention.CompareNoCase(".normals") ==0)
		{
			msg = ExecuteFromNormal(callback);
		}
		else
		{
			msg = ExecuteFromDaily(callback);
		}
	}
	else
	{
		msg = ExecuteFromGrid(callback);
	}
	return msg;
}
*/
ERMsg CTYNExtractor::ExecuteFromDaily(CFL::CCallback& callback)
{
	ERMsg msg;
/*
	CString outputFilePath(GetAbsoluteFilePath(m_outputFilePath));
	UtilWin::SetFileExtension( outputFilePath, ".DailyStations");

	callback.AddMessage( GetString(IDS_CREATE_DATABASE) );
	callback.AddMessage(outputFilePath, 1);
	callback.AddMessage("");

	
	//Get the data for each station
	CDailyDatabase inputDailyDB;
	CDailyDatabase outputDailyDB;
	CShapeFileBase shapefile;
	

	if( !m_shapeFilePath.IsEmpty() )
	{
		msg += shapefile.Read( GetAbsoluteFilePath(m_shapeFilePath) );
	}

	msg += inputDailyDB.Open( GetAbsoluteFilePath(m_inputFilePath), CDailyDatabase::modeReadOnly);
	msg += outputDailyDB.Open( outputFilePath, CDailyDatabase::modeEdit, false);
	if( !msg)
		return msg;

	callback.SetNbStep(0, inputDailyDB.GetSize(), 1);
	

	for(int i=0; i<inputDailyDB.GetSize()&&msg; i++)
	{
		CDailyStation station;
		inputDailyDB.GetAt(i, station);

		
		bool bRect = m_boundingBox.IsRectNull() || m_boundingBox.PtInRect( station.GetCoord() );
		bool bShape = shapefile.GetNbShape()==0 || shapefile.IsInside(station.GetCoord());
		if( bRect&&bShape )
		{
			station.SetDataModified(true);

			msg = outputDailyDB.Add( station);
		}
		
		msg += callback.StepIt();
	}

	outputDailyDB.Close();
	if( msg)
	{
		msg = outputDailyDB.Open(outputFilePath);
	}
*/	
	return msg;
}

//***********************************************************************************
ERMsg CTYNExtractor::ExecuteFromNormal(CFL::CCallback& callback)
{
	ERMsg msg;

	CString outputFilePath(GetAbsoluteFilePath(m_outputFilePath));
	UtilWin::SetFileExtension( outputFilePath, ".Normals");

	callback.AddMessage( GetString(IDS_CREATE_DATABASE) );
	callback.AddMessage(outputFilePath, 1);
	callback.AddMessage("");

	
	//Get the data for each station
	CNormalsDatabase inputDB;
	CNormalsDatabase outputDB;
	CGridYM TYN_SC2[3];
	
	for(int t=0; t<NB_EXT; t++)
	{
		CString filePath = m_TYNPath + GetFileName(m_model, m_sres, t);
		msg += TYN_SC2[t].Open( GetAbsoluteFilePath(filePath), CFile::modeRead);
	}

	msg += inputDB.Open( GetAbsoluteFilePath(m_inputFilePath));
	msg += outputDB.Open( outputFilePath, CNormalsDatabase::modeEdit);
	if( !msg)
		return msg;

	callback.SetNbStep(0, inputDB.GetSize(), 1);
	

	int firstYear = FISRT_PERIOD+m_period*PERIOD_SIZE;
	int nbYear = PERIOD_SIZE;

	for(int i=0; i<inputDB.GetSize()&&msg; i++)
	{
		CNormalsStation station;
		inputDB.GetAt(i, station);
		CNormalsData data = station.GetData();

		bool bValidData=true;
		for(int m=0; m<12&&msg&&bValidData; m++)	
		{
			CFL::CStatistic stat[3];
			for(int y=0; y<nbYear&&msg&&bValidData; y++)
			{
				int year = firstYear+y;
				float delta[3] = {0};
				bool bValidCell[3]={0};
				for(int t=0; t<NB_EXT; t++)
				{
					short yi = year-TYN_SC2[t].GetFirstYear();
					ASSERT( yi>=0 && yi<TYN_SC2[t].GetNbYears());
					delta[t] = TYN_SC2[t].ReadCell(station.GetCoord(), yi, m);
					bValidCell[t]= (delta[t]>TYN_SC2[t].GetNoData());
				}

				if( bValidCell[0]&&bValidCell[1]&&data.HaveCat(TEMPERATURE) )
				{
					stat[0] += data[m][NORMAL_DATA::TMIN_MN] + delta[0]-delta[1]/2;
					stat[1] += data[m][NORMAL_DATA::TMAX_MN] + delta[0]+delta[1]/2;
				}
				if( bValidCell[2]&&data.HaveCat(PRECIPITATION) )
				{
					float ppt = data[m][NORMAL_DATA::PRCP_TT] + delta[2];
					stat[2] += ppt>=0?ppt:0;
				}
			}

			if( data.HaveCat(TEMPERATURE) )
			{
				ASSERT( stat[0][CFL::NB_VALUE]==stat[1][CFL::NB_VALUE]);
				if( stat[0][CFL::NB_VALUE] >= 10)
				{
					data[m][NORMAL_DATA::TMIN_MN] = (float)stat[0][CFL::MEAN];
					data[m][NORMAL_DATA::TMAX_MN] = (float)stat[1][CFL::MEAN];
				}
				else
				{
					bValidData=false;
				}
			}
			if( data.HaveCat(PRECIPITATION) )
			{
				if( stat[2][CFL::NB_VALUE] >= 10)
				{
					data[m][NORMAL_DATA::PRCP_TT] = (float)stat[2][CFL::MEAN];
					data[m][NORMAL_DATA::PRCP_SD] = (float)stat[2][CFL::COEF_VAR];
				}
				else
				{
					bValidData=false;
				}
			}
		}

		if( bValidData )
		{
			station.SetData(data);
			ASSERT( station.IsValid() );

			msg = outputDB.Add( station);
		}
		else
		{
			CString err; 
			err.FormatMessage( IDS_NO_CC_ENTRY, station.GetName() );
			callback.AddMessage(err);
		}

		msg += callback.StepIt();
	}

	outputDB.Close();
	if( msg)
	{
		msg = outputDB.Open(outputFilePath);
	}
	
	return msg;
}


ERMsg CTYNExtractor::ExecuteFromGrid(CFL::CCallback& callback)
{
	ERMsg msg;

	CString outputFilePath(GetAbsoluteFilePath(m_outputFilePath));
	UtilWin::SetFileExtension(outputFilePath, CNormalsDatabase::GetDatabaseExtension());

	callback.AddMessage( GetString(IDS_CREATE_DATABASE) );
	callback.AddMessage(outputFilePath, 1);
	callback.AddMessage("");
	
	CNormalsDatabase inputDB;
	CGridYM TYN_SC2[3];
	CGridYM TYN_obs[3];
	CGridYM TYN_elev;
	CNormalsDatabase outputDB;

	CString filePath = m_TYNPath + "halfdeg.elv";
	msg += TYN_elev.Open(GetAbsoluteFilePath(filePath), CFile::modeRead);

	for(int t=0; t<NB_EXT; t++)
	{
		CString filePath = m_TYNPath + GetFileName(m_model, m_sres, t);
		TYN_SC2[t].Open( GetAbsoluteFilePath(filePath), CFile::modeRead);

		filePath = m_TYNPath + GetFileName(t);
		TYN_obs[t].Open( GetAbsoluteFilePath(filePath), CFile::modeRead);

		ASSERT( TYN_SC2[t].GetSizeX() == TYN_obs[t].GetSizeX());
		ASSERT( TYN_SC2[t].GetSizeY() == TYN_obs[t].GetSizeY());
		ASSERT( TYN_SC2[t].GetSizeX() == TYN_elev.GetSizeX());
		ASSERT( TYN_SC2[t].GetSizeY() == TYN_elev.GetSizeY());
	}

	msg += inputDB.Open( GetAbsoluteFilePath(m_inputFilePath));
	msg += outputDB.Open( outputFilePath, CNormalsDatabase::modeEdit);
	if( !msg)
		return msg;

	//ASSERT( TYN_SC2[t].GetSizeX() == TYN_obs[t].GetSizeX());
	//ASSERT( TYN_SC2[t].GetSizeY() == TYN_obs[t].GetSizeY());

	
	callback.SetNbStep(0, TYN_elev.GetSizeX()*TYN_elev.GetSizeY(), 1);
	

	int firstYear = FISRT_PERIOD+m_period*PERIOD_SIZE;
	int nbYear = PERIOD_SIZE;

	const CGeoPoint& refP = TYN_elev.GetBoundingBox().LowerLeft();
	for(int y=0; y<TYN_elev.GetSizeY()&&msg; y++)
	{
		for(int x=0; x<TYN_elev.GetSizeX()&&msg; x++)	
		{

			CNormalsStation station;
			CString name;
			name.Format( "%d %d", x, y);
			station.SetName(name);
			station.SetLat( refP.m_lat() + (y*0.5 + 0.25));
			station.SetLon( refP.m_lon() + (x*0.5 + 0.25));
			station.SetElev( (int)TYN_elev.ReadCell(CPoint(x,y), 0, 0) );

			//TODO: Get the nearest station to obtain missing field
			//inputDB.Match...

			CNormalsData data;

			for(int m=0; m<12&&msg; m++)	
			{
				CFL::CStatistic stat[3];
				for(int y=0; y<nbYear; y++)
				{
					int year = firstYear+y;
					float obs[3] = {0};
					float delta[3] = {0};
					for(int t=0; t<NB_EXT; t++)
					{
						short yi = year-TYN_SC2[t].GetFirstYear();
						ASSERT( yi>=0 && yi<TYN_SC2[t].GetNbYears());
						obs[t] = TYN_obs[t].ReadCell(station.GetCoord(), yi, m);
						delta[t] = TYN_SC2[t].ReadCell(station.GetCoord(), yi, m);
					}


					stat[0] += obs[0] + delta[0]-delta[1]/2;
					stat[1] += obs[0] + delta[0]+delta[1]/2;
					float ppt = obs[2] + delta[2];
					stat[2] += ppt>=0?ppt:0;
				}

				data[m][NORMAL_DATA::TMIN_MN] = (float)stat[0][CFL::MEAN];
				data[m][NORMAL_DATA::TMAX_MN] = (float)stat[1][CFL::MEAN];
				data[m][NORMAL_DATA::PRCP_TT] = (float)stat[2][CFL::MEAN];
				data[m][NORMAL_DATA::PRCP_SD] = (float)stat[2][CFL::COEF_VAR];
			}

			
			station.SetData(data);
			ASSERT( station.IsValid() );

			msg = outputDB.Add( station);
			msg += callback.StepIt();
		}
	}
	
	
	outputDB.Close();
	if( msg)
	{
		msg = outputDB.Open(outputFilePath);
	}
	
	return msg;
}
	
