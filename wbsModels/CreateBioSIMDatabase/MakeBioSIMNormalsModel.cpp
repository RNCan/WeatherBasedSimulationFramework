//*********************************************************************
// File: MakeBioSIMNormalsModel.cpp
//
// Class: CCreateBioSIMDatabase
//
//************** MODIFICATIONS  LOG ********************
// 31/10/2012   2.2	Rémi Saint-Amant    Update with BioSIM 10.2.4
// 01/09/2003   1.0	Rémi Saint-Amant    Creation
//*********************************************************************
#include "stdafx.h"
#include "MakeBioSIMNormalsModel.h"
#include "DailyStation.h"
#include "AdvancedNormalStation.h"
#include "EntryPoint.h"


#include <fstream>
#include <stdio.h>
#include <math.h>
#include <crtdbg.h>



using namespace DAILY_DATA;
using namespace UtilWin;
//**************************
//put the number of input parameters
//NB_INPUT_PARAMETER is used to determine if the dll
//uses the same number of parameters than the model interface

static const int IDS_BAD_PARAMETER = ERROR_USER_BASE+1;

//**************************
//these two lines define the main class 
//create the main object of your class 
//very important: don't change the name of pGlobalModel
static const bool bRegistred = 
	CModelFactory::RegisterModel( CCreateBioSIMDatabase::CreateObject );

//CNormalFile* CCreateBioSIMDatabase::m_pNormalDB=NULL;
//CDailyFile* CCreateBioSIMDatabase::m_pDailyDB=NULL;

//C2DimArray<UtilWin::CStdioFileEx> CCreateBioSIMDatabase::m_file;
//UtilWin::CStdioFileEx CCreateBioSIMDatabase::m_fileElev;

CNormalFile* m_pNormalDB=NULL;
CDailyFile* m_pDailyDB=NULL;
C2DimArray<UtilWin::CStdioFileEx> m_file;
UtilWin::CStdioFileEx m_fileElev;


CCreateBioSIMDatabase::CCreateBioSIMDatabase()
{
	//m_pNormalDB=NULL;
	//m_pDailyDB=NULL;

	NB_INPUT_PARAMETER=2;
	VERSION="2.2(2012)";

    // initialise your variables here (optionnal)
	//m_bExportFile = true;
}

CCreateBioSIMDatabase::~CCreateBioSIMDatabase()
{
	//if( m_pDailyDB )
		//delete m_pDailyDB;

	//m_pDailyDB=NULL;
}

void CCreateBioSIMDatabase::GetDailyStation(CDailyStation& station)
{
	
	station.SetName( (LPCTSTR)m_info.m_loc.GetName());
	station.SetID( (LPCTSTR)m_info.m_loc.GetID() );
	station.SetLat(m_info.m_loc.GetLat());
	station.SetLon(m_info.m_loc.GetLon());
	station.SetElev((int)m_info.m_loc.GetElev());

	CYearPackage package;
	for(int y=0; y<m_weather.GetNbYear(); y++)
	{
		CDailyData data;
		for(int jd=0; jd<m_weather[y].GetNbDay(); jd++)
		{
			data[jd][TMIN] = (float)m_weather[y].GetDay(jd).GetTMin();
			data[jd][TMAX] = (float)m_weather[y].GetDay(jd).GetTMax();
			data[jd][PRCP] = (float)m_weather[y].GetDay(jd).GetPpt();
			data[jd][TDEW] = (float)m_weather[y].GetDay(jd)[TDEW];
			data[jd][RELH] = (float)m_weather[y].GetDay(jd)[RELH];
			data[jd][WNDS] = (float)m_weather[y].GetDay(jd)[WNDS];
			data[jd][SRAD] = (float)m_weather[y].GetDay(jd)[SRAD];
		}

		CDailyYear dailyYear;
		dailyYear.SetYear(m_weather[y].GetYear());
		dailyYear.SetData(data);
		
		package.SetYear(dailyYear);
	}

	station.AddPakage(package);
}

typedef CModelStatVectorTemplate<NORMAL_DATA::NB_FIELDS> CNormalStatVector;
//**************************
//This method is called to compute the solution
ERMsg CCreateBioSIMDatabase::OnExecuteMonthly()
{
	_ASSERTE( m_weather.GetNbYear() >= 10);

	ERMsg msg;
    
	CDailyStation station;
	GetDailyStation(station);

	CAdvancedNormalStation normalStation;
	msg = normalStation.FromDaily( station, 2 );


	if( msg )
	{
		CNormalStatVector output(12,CTRef(CTRef::MONTHLY, 0,0,0,0, CTRef::OVERALL_YEARS));
		for(int m=0; m<12; m++)
		{
			//m_outputFile << 0<< m+1;
			for(int j=0; j< NORMAL_DATA::NB_FIELDS; j++)
				output[m][j] = normalStation[m][j];

			//m_outputFile.EndLine();
		}
				
		SetOutput(output);

		//if( m_bExportFile )
		{
			if( m_info.m_paramCounter.GetTotal() != 1 ||
				m_info.m_repCounter.GetTotal() != 1 )
			{
				msg.ajoute( GetErrorMessage(IDS_BAD_PARAMETER ) );
			}

			//CStdioFile file;
			//CFileException e;
			if( m_info.m_locCounter.IsFirst() && 
				m_info.m_paramCounter.IsFirst()&& 
				m_info.m_repCounter.IsFirst())
			{
				m_filePath.SetFileExtension(".Normals");
				if( m_bDeleteOldDB )
					CNormalFile::DeleteDatabase( (LPCTSTR)m_filePath);
				//file.Open( m_normalFilePath, CFile::modeCreate|CFile::modeWrite, &e);

				m_pNormalDB=new CNormalFile;
				m_pNormalDB->SetBeginYear(m_weather.GetFirstYear());
				m_pNormalDB->SetEndYear(m_weather.GetLastYear());
				msg += m_pNormalDB->Open( (LPCTSTR)m_filePath, CNormalFile::modeEdit);
				//file.Open( m_normalFilePath, CFile::modeCreate|CFile::modeWrite, &e);

				//char tmp[NORMAL_DATA::LINE_LENGTH1] = {0};//"2003 11 18 1971 2000 4";
				//sprintf(tmp, "%4d %2d %2d %4d %4d %1d", 2005, 1, 1, m_weather.GetFirstYear(), m_weather.GetLastYear(), 5);
				
				//file.Write(tmp, NORMAL_DATA::LINE_LENGTH1);
				//file.WriteString("\n");
			}
			//else 
			//{
				//|CFile::modeNoTruncate
				//file.Open( m_normalFilePath, CFile::modeWrite, &e);
				//file.SeekToEnd();
			//}


			if(msg)
			{
				msg += m_pNormalDB->Add(normalStation);
			}

			
			if( m_info.m_locCounter.IsLast() && 
				m_info.m_paramCounter.IsLast()&& 
				m_info.m_repCounter.IsLast())
			{
				m_pNormalDB->Close();
				delete m_pNormalDB;
				m_pNormalDB=NULL;
			}

			/*if( e.m_cause == CFileException::none )
			{
				// save result to disk
				normal.Write(file);

				file.Close();
			}
			else 
			{
				char txt[255]={0};
				e.GetErrorMessage(txt, 255);
				msg.asgType(ERMsg::ERREUR);
				msg.ajoute(txt);
			}*/
		}
	}

    

    return msg;
}

ERMsg CCreateBioSIMDatabase::OnExecuteDaily()
{
	 

	if( m_info.m_paramCounter.GetTotal() != 1 ||
		m_info.m_repCounter.GetTotal() != 1 )
	{
		return GetErrorMessage(IDS_BAD_PARAMETER);
	}

	
	

	ERMsg msg;
    
	CDailyStation station;
	GetDailyStation(station);

//	m_CS.Enter();
	
	if(	m_info.m_locCounter.IsFirst() && 
		m_info.m_paramCounter.IsFirst()&& 
		m_info.m_repCounter.IsFirst())
	{
		m_filePath.SetFileExtension(".DailyStations");
		if( m_bDeleteOldDB )
			CDailyFile::DeleteDatabase( (LPCTSTR)m_filePath);
		
		m_pDailyDB=new CDailyFile;
		msg +=	m_pDailyDB->Open( (LPCTSTR)m_filePath, CDailyFile::modeEdit);
	}
	

	if(msg)
	{
		msg += m_pDailyDB->Add(station);
	}

			
	if( m_info.m_locCounter.IsLast() && 
		m_info.m_paramCounter.IsLast()&& 
		m_info.m_repCounter.IsLast())
	{
		m_pDailyDB->Close();
		delete m_pDailyDB;
		m_pDailyDB=NULL;
	}
	
	//m_CS.Leave();

	CModelStatVectorTemplate<1> junk(1,CTRef(0,0,0), -9999);
	SetOutput(junk);

	//m_outputFile << 0<<1<<1<<-9999;
	//m_outputFile.EndLine();
	
    return msg;
}

/*
//Code for PHENOFIT weather file
ERMsg CCreateBioSIMDatabase::OnExecuteUntemporal()
{
	ERMsg msg; 

	if( m_info.m_paramCounter.GetTotal() != 1 ||
		m_info.m_repCounter.GetTotal() != 1 )
	{
		msg.ajoute( GetErrorMessage(IDS_BAD_PARAMETER ) );
	}

	m_outputFile << 1<<-9999;
	m_outputFile.EndLine();


	int firstYear=m_weather.GetFirstYear();
	int lastYear = m_weather.GetLastYear();
	int nbYear=lastYear-firstYear+1;
	
	
	if( m_info.m_locCounter.IsFirst() && 
		m_info.m_paramCounter.IsFirst()&& 
		m_info.m_repCounter.IsFirst())
	{
		CString path = GetPath( (LPCTSTR)m_filePath);
		CString name = GetFileTitle( (LPCTSTR)m_filePath);
		name.MakeLower();
		name.Replace("_altitude","");
		
		static const char* FILE_NAME[6] = 
		{	"_tmn_", "_tmx_", "_pre_","_RH_", "_wnd_", "_glo_"
		};

		
		//Comparer les données normales

	
		
		msg += m_fileElev.Open((LPCTSTR)m_filePath, CFile::modeWrite|CFile::modeCreate);
		
		if(!msg)
			return msg;

		
		CString tmp;
		for(int j=0; j<4; j++)
			m_fileElev.WriteString(tmp+"\n");

		m_file.SetSize(nbYear, 6);
		for(int yr=0; yr<nbYear&&msg; yr++)
		{
			int year = firstYear+yr;
			
			for(int i=0; i<6; i++)		
			{
				CString filePath;
				filePath.Format("%s%s%s%4d_dly.fit",path,name,FILE_NAME[i], year);
				ERMsg msgTmp = m_file[yr][i].Open(filePath, CFile::modeWrite|CFile::modeCreate);
				if( msgTmp)
				{

	//"Fichier de forçage climatique pour PHENOFIT, créé le 2008-09-18 à 14:48:04 (zone:+0200)."
	//"Variable = Température minimalle quotidienne à 2 m [T_i, °C] pour 1970"
	//"Origin   = BioSIM"
	//"Lignes: 9892 points, Colonnes: latitude (degrés décimaux), longitude (degrés décimaux), 365 valeurs quotidiennes"


					//a faire le sstring
					CString tmp;
					for(int j=0; j<4; j++)
						m_file[yr][i].WriteString(tmp+"\n");
				}
				else return msgTmp;
				
			}
		}
	}
		
	
	//write loc
	CString line;
	line.Format("%9.5lf %9.4lf %5.0f\n", m_info.m_loc.GetLat(), m_info.m_loc.GetLon(),m_info.m_loc.GetElev());
	m_fileElev.WriteString(line+"\n");		


	ASSERT( msg);
	
	for(int yr=0; yr<nbYear&&msg; yr++)
	{
		int year = firstYear+yr;
	
		static const int POS[6] = {TMIN,TMAX,PRCP,RELH,WNDS,SRAD};
		int pos[6]={0};
		CString line[6];
		for(int i=0; i<6; i++)
		{
			CString line;
			line.Format("%9.4lf %9.4lf", m_info.m_loc.GetLat(), m_info.m_loc.GetLon());
			for(int jd=0; jd<365; jd++)
			{
				double value= m_weather[yr].GetDay(jd)[POS[i]];
				if(POS[i]==WNDS)//transform  km/h to m/s
					value *= 1000.0/3600.0;


				line += " ";
				CString tmp;
				tmp.Format("%9.4lf",value);
				line += tmp;
			}
			m_file[yr][i].WriteString(line+"\n");
		}	
	}//all year	

	if( m_info.m_locCounter.IsLast() && 
		m_info.m_paramCounter.IsLast()&& 
		m_info.m_repCounter.IsLast())
	{
		m_fileElev.Close();
		for(int yr=0; yr<nbYear&&msg; yr++)
		{
			for(int i=0; i<6; i++)		
			{
				m_file[yr][i].Close();
			}
		}
	}
	
	
	return msg;
}
*/

//**************************
//this method is called to load parameters in your variables
ERMsg CCreateBioSIMDatabase::ProcessParameter(const CParameterVector& parameters)
{
    ERMsg msg;

    //transfer your parameter here
	int cur = 0;
	
	m_bDeleteOldDB= parameters[cur++].GetBool();
	m_filePath = parameters[cur++].GetString();

    return msg;
}


//**************************
//this method is called to get error msg strings
ERMsg CCreateBioSIMDatabase::GetErrorMessage(int errorID)
{
	ERMsg msg;

	switch(errorID)
	{
	case IDS_BAD_PARAMETER: msg.ajoute((m_info.m_language==FRENCH)?"Le nombre de replications doit être 1 pour l'export de la BD.":"Replication must be 1 when export DB.");break; 
	default: msg = CBioSIMModelBase::GetErrorMessage(errorID);
	}
	return msg;
}

//*******************************************
/*BEGIN_MESSAGE_MAP(CbidonDLLApp, CWinApp)
END_MESSAGE_MAP()

CbidonDLLApp::CbidonDLLApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CbidonDLLApp object

CbidonDLLApp theApp;


// CbidonDLLApp initialization

BOOL CbidonDLLApp::InitInstance()
{
	CWinApp::InitInstance();

	return TRUE;
}
*/