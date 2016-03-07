//****************************************************************************
// Fichier: TempGenHelp.cpp
// Classe:  CTempGenHelp 
// Usage:
//  
//  To use CTempGenHelp. There are 4 steps:
//      1) initialization of the DLL with the method : initialize       
//      2) initialization of the simulator with the methods :
//              SetNormalDBFilePath
//              SetDailyDBFilePath
//              SetTarget
//              SetReplication
//              SetTGInput
//      3) execution of the simulator with the method : Generate
//		4) get results with the method: GetValue, GetAllYearStat or Save
//
//****************************************************************************
// 11-04-2005  Rémi Saint-Amant  Initial version 
// 12-04-2007  Rémi Saint-Amant  Modification for memory computation
//								 Take new TempGen	
// 30-09-2009  Rémi Saint-Amant  Updated, category added, simulation of wind spped and humidity
// 22-09-2010  Rémi Saint-Amant  Add Write stream to export as CWeather into stream
// 30-09-2010                    Add seedType and Xval on the TGInput
//****************************************************************************
#include "stdafx.h"
#include <crtdbg.h>
#include <Windows.h>

#include "ModelBase/TempGenHelp.h"

namespace WBSF
{


	//****************************************************************************
	// Summary:    Constructor
	//
	// Description: To Create and initialize an object
	//
	// Input:      
	//
	// Output:
	//
	// Note:
	//****************************************************************************
	CTempGenHelp::CTempGenHelp()
	{
		m_hDll = NULL;
		m_SetNormalDBFilePath = NULL;
		m_SetDailyDBFilePath = NULL;
		m_SetTarget = NULL;
		m_SetReplication = NULL;
		m_SetTGInput = NULL;
		m_Generate = NULL;
		m_Save = NULL;
		m_WriteStream = NULL;
		m_GetValue = NULL;
		m_GetAllYearsStat = NULL;
		m_GetGrowingSeasonStat = NULL;
	}


	//****************************************************************************
	// Summary:     Destructor
	//
	// Description:  Destroy and clean memory
	//
	// Input:      
	//
	// Output:
	//
	// Note:
	//****************************************************************************
	CTempGenHelp::~CTempGenHelp()
	{
		if (m_hDll)
		{
			FreeLibrary((HMODULE)m_hDll);
			m_hDll = NULL;
		}

		m_SetNormalDBFilePath = NULL;
		m_SetDailyDBFilePath = NULL;
		m_SetTarget = NULL;
		m_SetReplication = NULL;
		m_SetTGInput = NULL;
		m_Generate = NULL;
		m_Save = NULL;
		m_WriteStream = NULL;
		m_GetValue = NULL;
		m_GetAllYearsStat = NULL;
		m_GetGrowingSeasonStat = NULL;
	}

	ERMsg SYGetMessage(DWORD errnum)
	{
		ERMsg msg;
		char cause[1024];

		FormatMessageA(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, errnum, 0, cause, 1024, NULL);

		msg.asgType(ERMsg::ERREUR);
		msg.ajoute(cause);

		return msg;
	}

	//****************************************************************************
	// Summary:     initialize the class 
	//
	// Description:  initialize the dll with file path
	//
	// Input:      DLLFilePath : the file path of the dll (tempGenLib.dll)
	//
	// Output:     ERMsg : error msg
	//
	// Note:
	//****************************************************************************
	ERMsg CTempGenHelp::Initialize(const char* DLLFilePath)
	{
		ERMsg msg;

		if (m_hDll)
		{
			//if a dll is loaded : free
			FreeLibrary((HMODULE)m_hDll);
			m_hDll = NULL;
		}

		//load dll
		m_hDll = LoadLibraryA(DLLFilePath);

		if (m_hDll != NULL)
		{
			//load function
			m_SetNormalDBFilePath = (SetNormalDBFilePathF)GetProcAddress((HMODULE)m_hDll, "SetNormalDBFilePath");
			m_SetDailyDBFilePath = (SetDailyDBFilePathF)GetProcAddress((HMODULE)m_hDll, "SetDailyDBFilePath");
			m_SetTarget = (SetTargetF)GetProcAddress((HMODULE)m_hDll, "SetTarget");
			m_SetReplication = (SetReplicationF)GetProcAddress((HMODULE)m_hDll, "SetReplication");
			m_SetTGInput = (SetTGInputF)GetProcAddress((HMODULE)m_hDll, "SetTGInput");
			m_Generate = (GenerateF)GetProcAddress((HMODULE)m_hDll, "Generate");
			m_Save = (SaveF)GetProcAddress((HMODULE)m_hDll, "Save");
			m_WriteStream = (WriteStreamF)GetProcAddress((HMODULE)m_hDll, "WriteStream");
			m_GetValue = (GetValueF)GetProcAddress((HMODULE)m_hDll, "GetValue");
			m_GetAllYearsStat = (GetAllYearsStatF)GetProcAddress((HMODULE)m_hDll, "GetAllYearsStat");
			m_GetGrowingSeasonStat = (GetGrowingSeasonStatF)GetProcAddress((HMODULE)m_hDll, "GetGrowingSeasonStat");

			_ASSERTE(IsInit());
		}
		else
		{
			//get error msg
			msg = SYGetMessage(GetLastError());
		}

		return msg;
	}

	bool CTempGenHelp::IsInit()const
	{
		return  m_hDll != NULL &&
			m_SetNormalDBFilePath != NULL &&
			m_SetDailyDBFilePath != NULL &&
			m_SetTarget != NULL &&
			m_SetReplication != NULL &&
			m_SetTGInput != NULL &&
			m_Generate != NULL &&
			m_Save != NULL &&
			m_WriteStream != NULL &&
			m_GetValue != NULL &&
			m_GetAllYearsStat != NULL;
	}


	//****************************************************************************
	// Summary:     Initialize normals file path 
	//
	// Description:  set the normal file path (.normals)
	//
	// Input:      filePath : the file path of the normals file
	//
	// Output:     ERMsg : error msg
	//
	// Note:
	//****************************************************************************
	ERMsg CTempGenHelp::SetNormalDBFilePath(const char* filePath)
	{
		_ASSERTE(m_SetNormalDBFilePath != NULL);

		ERMsg msg;

		char messageOut[1024] = { 0 };
		if (!m_SetNormalDBFilePath(filePath, messageOut))
		{
			msg.asgType(ERMsg::ERREUR);
			msg.ajoute(messageOut);
		}

		return msg;
	}


	//****************************************************************************
	// Summary: Initialize daily file path 
	//
	// Description: set the daily file path. 
	//				This is optionnal if you use only normals generation (ie year==0)
	//
	// Input:   filePath : the file path of the daily file (.dailyStations)
	//
	// Output:  ERMsg : error msg.
	//
	// Note:    To use daily simulation, year must be different from zero in the SetTGInput function.
	//****************************************************************************
	ERMsg CTempGenHelp::SetDailyDBFilePath(const char* filePath)
	{
		_ASSERTE(m_SetDailyDBFilePath != NULL);

		ERMsg msg;

		char messageOut[1024] = { 0 };
		if (!m_SetDailyDBFilePath(filePath, messageOut))
		{
			msg.asgType(ERMsg::ERREUR);
			msg.ajoute(messageOut);
		}


		return msg;
	}

	//****************************************************************************
	// Summary:     Initialize target localisation(simulation point )
	//
	// Description:  set the target location. lat,lonelev,slope and aspect
	//
	// Input:   name: name of the simulation point 
	//          lat: latitude in decimal degree of the point. negative lat for southern hemisphere.
	//          lon: longitude in decimal degree of the point. negative lon for western hemisphere.
	//          elev: elevation in meters.
	//          slope: slope in %. 100% equals a slope of 45 degres.
	//          aspect: aspect in degrees. 0 = north, 90 = east, 180 = south, 270 = west.
	//
	// Output:     
	//
	// Note:       
	//****************************************************************************
	void CTempGenHelp::SetTarget(const char* name, const char* ID, const double& lat, const double& lon, float elev, float slope, float aspect)
	{
		_ASSERTE(m_SetTarget != NULL);

		m_SetTarget(name, ID, lat, lon, elev, slope, aspect);
	}

	//****************************************************************************
	// Summary: Set replication
	//
	// Description: Set the number of replication.
	//
	// Input:   nbRep:  The number of replications. For normals simulations, this should be > 10.
	//                  See BioSIM documentation for more details.
	// Output:  
	//
	// Note:    After simulation( method Generate ), one result is create 
	//          for each replication.
	//****************************************************************************
	void CTempGenHelp::SetNbReplication(short nbRep)
	{
		_ASSERTE(m_SetReplication != NULL);

		return m_SetReplication(nbRep);
	}


	//****************************************************************************
	// Summary: Set parameters
	//
	// Description:  Set temporal and simulation parameters.
	//
	// Input:   year: the last year of the simulation. For normals simulations use 0.
	//          nbYear: The number of years to simulate. 
	//          nbNormalStation: The number of normal stations to find around a simulation point. 
	//          nbDailyStation: The number of daily stations to find around a simulation point. 
	//          albedoType:     Exposition correction. Only used if slope and aspect are not NULL.
	//          bSimPpt: true to simulate precipitation, false otherwise.
	//
	// Output:  ERMsg : error msg
	//
	// Note:    To use daily simulation, year and daily file path must be supplied 
	//          variable            min value   max value		default     Note
	//          firstYear:          -998        2100			0			for normals simulation, firstYear must be <= 0
	//          lastYear:           -998        2100			0			for normals simulation, lastYear must be <= 0
	//          nbNormalStation:    1           20				8           
	//          nbDailyStation:     1           20				8           Not used if lastYear = 0
	//          AlbedoType:       NONE(0)  CONIFER_CANOPY(1) CONIFER_CANOPY(1)
	//          category:           one or combinaison of "T P H WS"		T=temperatrue, P=precipitation, H=humidity and WS = windSpeed
	//			seedType: the type of seed. FIXED_SEED or RANDOM_SEED
	//			XValidation: 0 = false, 1 = true
	//****************************************************************************
	void CTempGenHelp::SetTGInput(short firstYear, short lastYear,
		short nbNormalStation, short nbDailyStation,
		short albedoType, const char* category, short seedType, short Xval)
	{
		_ASSERTE(m_SetTGInput != NULL);
		_ASSERTE(firstYear <= lastYear);

		return m_SetTGInput(firstYear, lastYear, nbNormalStation, nbDailyStation, albedoType, category, seedType, Xval);
	}

	//****************************************************************************
	// Summary: Create results
	//
	// Description:  this function runs the simulator and creates results.
	//
	// Input:   
	//
	// Output:  ERMsg : error msg
	//
	// Note:    For each replication defined, Generate creates a results
	//****************************************************************************
	ERMsg CTempGenHelp::Generate()
	{
		_ASSERTE(m_Generate != NULL);

		ERMsg msg;

		char messageOut[1024] = { 0 };
		if (!m_Generate(messageOut))
		{
			msg.asgType(ERMsg::ERREUR);
			msg.ajoute(messageOut);
		}


		return msg;

	}

	//****************************************************************************
	// Summary: Save results to output file
	//
	// Description:  this function save the results create in Generate to output files.
	//
	// Input:   outputFilePathVector: one file per replication.
	//
	// Output:  ERMsg : error msg
	//
	// Note:    The size of outputFilePathVector must be the same as the number of replication
	//****************************************************************************
	ERMsg CTempGenHelp::Save(const StringVector& outputFilePathVector)const
	{
		_ASSERTE(m_Save != NULL);

		ERMsg msg;

		char** pOutputFile = new char*[outputFilePathVector.size()];

		for (int i = 0; i < (int)outputFilePathVector.size(); i++)
		{
			pOutputFile[i] = new char[MAX_PATH];
			strcpy(pOutputFile[i], outputFilePathVector[i].c_str());
		}

		char messageOut[1024] = { 0 };
		if (!m_Save(pOutputFile, messageOut))
		{
			msg.asgType(ERMsg::ERREUR);
			msg.ajoute(messageOut);
		}

		for (int i = 0; i < (int)outputFilePathVector.size(); i++)
			delete pOutputFile[i];

		delete pOutputFile;

		return msg;

	}

	//****************************************************************************
	// Summary: Save one replication into an ouput stream
	//
	// Description: this function save the results of one replication (CWeather) create in Generate function
	//				into an stream. The stream can be read direcly by the CWeather class
	//
	// Input:   r: the replication number (0.. n-1).
	//			io: the stream.
	//
	// Output:  void
	//
	// Note:    
	//****************************************************************************
	void CTempGenHelp::WriteStream(short r, std::ostream& io)const
	{
		_ASSERTE(m_WriteStream != NULL);

		ERMsg msg;

		m_WriteStream(r, io);
	}

	//****************************************************************************
	// Summary: Get one value in the results.
	//
	// Description:  this function return one value from the results.
	//
	// Input:   r:	The replication. From 0 to nbReplication-1.
	//			y:	The yearIndex. From 0 to nbYear-1.
	//			jd:	The julianDay. From 0 to 365. 365 can be missing (ie -9999)
	//			v:	the variable. See TVariable in the the header file.
	//
	// Output:  float : the value. Can be misiing value(ie -9999)
	//
	// Note:    Generate must be call before
	//****************************************************************************
	float CTempGenHelp::GetValue(short r, short y, short jd, short v)const
	{
		_ASSERTE(m_GetValue != NULL);
		return m_GetValue(r, y, jd, v);
	}

	//****************************************************************************
	// Summary: Get The mean of all years for a variable statitstic
	//
	// Description:  this function return a statistc from the results.
	//
	// Input:   var: the variable. See TVariable in the the header file.
	//			dailyStatType: the type of the daily statistic(ie MEAN, SUM). See TStat in the header file.
	//			annualStatType: the type of the daily statistic(ie MEAN, SUM). See TStat in the header file.
	//
	// Output:  double: the statistic
	//
	// Note:    Generate must be call before
	//			Example of statistic:
	//			The annual mean of minimum mean temperature: GetAllYearsStat(TMIN, MEAN, MEAN);
	//			The annual standard deviation of maximum mean temperature: GetAllYearsStat(TMAX, MEAN, STD_DEV);
	//			The annual mean of total precipitation: GetAllYearsStat(PRCP, SUM, MEAN);
	//			The annual coeficiant of variation of total precipitation: GetAllYearsStat(PRCP, SUM, COEF_VAR);
	//****************************************************************************
	double CTempGenHelp::GetAllYearsStat(short var, short dailyStatType, short annualStatType)const
	{
		_ASSERTE(m_GetAllYearsStat != NULL);
		return m_GetAllYearsStat(var, dailyStatType, annualStatType);
	}

	double CTempGenHelp::GetGrowingSeasonStat(short var, short dailyStatType, short annualStatType)const
	{
		_ASSERTE(m_GetAllYearsStat != NULL);
		return m_GetGrowingSeasonStat(var, dailyStatType, annualStatType);
	}

}