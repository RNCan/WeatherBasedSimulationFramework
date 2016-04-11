//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//******************************************************************************
#include "stdafx.h"
#include <sstream>
#include <WTypes.h>
#include <psapi.h>
#include <boost/algorithm/string.hpp>
#include "KMeanLocal/KMlocal.h" 

#include "Basic/OpenMP.h"
#include "Basic/WeatherStation.h"
#include "Basic/ModelStat.h"
#include "Basic/GeoBasic.h"
#include "Basic/WeatherDatabaseCreator.h"
#include "Basic/Timer.h"
#include "Basic/hxGrid.h"
#include "ModelBase/Model.h"
#include "ModelBase/WGInput.h"
#include "FileManager/FileManager.h"
#include "Simulation/WeatherGeneration.h"
#include "Simulation/ExecutableFactory.h"
#include "WeatherBasedSimulationString.h"


using namespace std;
using namespace VITALENGINE;
using namespace WBSF::WEATHER;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::DIMENSION;


namespace WBSF
{

static const char mutexName[] = "hxGridMutex"; 
static const __int8 DIRECT_ACCESS=0;



//**************************************************************
//CWeatherGeneration


const char* CWeatherGeneration::XML_FLAG = "WeatherGeneration";
const char* CWeatherGeneration::MEMBERS_NAME[NB_MEMBERS_EX] = { "WGInputName", "LocationsName", "ParametersVariationsName", "NbReplications",  "UseHxGrid" };//"LocationsReplacedByWeatherStations", "XValidation",
const int CWeatherGeneration::CLASS_NUMBER = CExecutableFactory::RegisterClass(CWeatherGeneration::GetXMLFlag(), &CWeatherGeneration::CreateObject );
    

CWeatherGeneration::CWeatherGeneration()
{
	Reset();
}

CWeatherGeneration::CWeatherGeneration(const CWeatherGeneration& in)
{
	operator=(in);
}


CWeatherGeneration::~CWeatherGeneration()
{}

void CWeatherGeneration::Reset()
{
	CExecutable::Reset();

	m_name = "WeatherGeneration";

	m_WGInputName = STRDEFAULT;
	m_locationsName.clear();
	m_parametersVariationsName.clear();
	m_nbReplications = 1;
	m_bUseHxGrid=false;

}


CWeatherGeneration& CWeatherGeneration::operator =(const CWeatherGeneration& in)
{
    if( &in != this )
    {
		CExecutable::operator =(in);

	    m_WGInputName = in.m_WGInputName;
		m_parametersVariationsName = in.m_parametersVariationsName;
	    m_locationsName = in.m_locationsName;
	    m_nbReplications = in.m_nbReplications;
		m_bUseHxGrid = in.m_bUseHxGrid;
    }

	return *this;
}

bool CWeatherGeneration::operator == (const CWeatherGeneration& in)const
{
	bool bEqual = true;

	if( CExecutable::operator!=(in) )bEqual = false;
	if( m_WGInputName != in.m_WGInputName)bEqual = false;
	if (m_parametersVariationsName != in.m_parametersVariationsName)bEqual = false;
	if( m_locationsName != in.m_locationsName)bEqual = false;
	if( m_nbReplications != in.m_nbReplications)bEqual = false;
	if( m_bUseHxGrid != in.m_bUseHxGrid)bEqual = false;

	return bEqual;
}

ERMsg CWeatherGeneration::GenerateLocationListFromWeatherStation(const CFileManager& fileManager, const CWGInput& WGInput, const CLocationVector& locations, CLocationVector& stations)
{
	ERMsg msg;

	string DBFilePath;
	size_t nbStations=0;
	bool bNormal = WGInput.m_sourceType == CWGInput::FROM_DISAGGREGATIONS;
	bool bHourly = WGInput.m_generationType == CWGInput::GENERATE_HOURLY;

	if (bNormal)
	{
		DBFilePath = fileManager.Normals().GetFilePath(WGInput.m_normalsDBName);
		nbStations = WGInput.m_nbNormalsStations;
	}
	else
	{
		if (bHourly)
		{
			DBFilePath = fileManager.Hourly().GetFilePath(WGInput.m_hourlyDBName);
			nbStations = WGInput.m_nbHourlyStations;
		}
		else
		{
			DBFilePath = fileManager.Daily().GetFilePath(WGInput.m_dailyDBName);
			nbStations = WGInput.m_nbDailyStations;
		}
	}

	CWeatherDatabasePtr pDB = CreateWeatherDatabase(DBFilePath);
	msg = pDB->Open(DBFilePath);
		
	if (msg)
	{
		CSearchResultVector finalResult;
		for (size_t i = 0; i < locations.size() && msg; i++)
		{
			size_t nbYears = bNormal ? 1 : WGInput.GetNbYears();
			for (size_t y = 0; y < nbYears && msg; y++)
			{
				for (TVarH v = H_TAIR; v < NB_VAR_H && msg; v++)
				{
					if (WGInput.m_variables[v])
					{
						CSearchResultVector result;
						pDB->Search(result, locations[i], nbStations, v, WGInput.GetFirstYear() + int(y));
						finalResult.insert(finalResult.end(), result.begin(), result.end());
					}
				}
			}
		}

		sort(finalResult.begin(), finalResult.end());
		finalResult.erase(unique(finalResult.begin(), finalResult.end()), finalResult.end());

		stations = pDB->GetLocations(finalResult);
	}
		

	return msg;
}


//mettre dans les DB
ERMsg CWeatherGeneration::CheckLocationsInDatabase(CNormalsDatabasePtr& pNormalDB, CDailyDatabasePtr& pDailyDB, CHourlyDatabasePtr& pHourlyDB, const CLocationVector& locations, const CWGInput& WGInput, CCallback& callback)
{
    ERMsg msg;

	CWVariables variables = WGInput.m_variables;
	CWVariables derivedVars = WGInput.m_allowedDerivedVariables;

	size_t nbFilter = variables.count();
	size_t nbYears = 0; 

	if (pNormalDB && pNormalDB->IsOpen())
		nbYears += 1;

	if (pDailyDB && pDailyDB->IsOpen())
		nbYears += WGInput.GetNbYears();
	
	if (pHourlyDB && pHourlyDB->IsOpen())
		nbYears += WGInput.GetNbYears();

	size_t nbGetDistance = nbFilter*nbYears;

	callback.PushTask(GetString(IDS_SIM_VERIFY_DISTANCE), nbGetDistance, 1);
	
	int nested = omp_get_nested();
	omp_set_nested(1);
	

	for (size_t i = 0; i < 3&&msg; i++)
	{
		CWeatherDatabasePtr pDB = NULL;
		switch (i)
		{
		case 0: pDB = pNormalDB; break;
		case 1: pDB = pDailyDB; break;
		case 2: pDB = pHourlyDB; break;
		default: assert(false);
		}

		if (pDB && pDB->IsOpen())
		{
			int currentYear = CTRef::GetCurrentTRef().GetYear();
			size_t nbYears = i == 0 ? 1 : WGInput.GetNbYears();

			int nbThreadsYear = min(CTRL.m_nbMaxThreads, (int)nbYears);//priority over years
			int nbThreadsLoc = min(CTRL.m_nbMaxThreads/nbThreadsYear, (int)locations.size());//priority over location


#pragma omp parallel for num_threads(nbThreadsYear) shared(msg) 
			for (int y = 0; y < (int)nbYears; y++)
			{
#pragma omp flush(msg)
				if (msg)
				{
					int year = WGInput.GetFirstYear() + int(y);
					for (TVarH v = H_TAIR; v < NB_VAR_H&&msg; v++)
					{
						if (variables[v])
						{
							double Dmin = DBL_MAX;
							double Dmax = DBL_MIN;
							int maxIndex = 0;
							ERMsg messageTmp;

#pragma omp parallel for num_threads(nbThreadsLoc) shared(msg, messageTmp) 
							for (__int64 l = 0; l < (__int64)locations.size(); l++)
							{
#pragma omp flush(messageTmp)
								if (messageTmp)
								{
									CSearchResultVector results;
									ERMsg msgTmp = pDB->Search(results, locations[l], 1, v, year);
									if (messageTmp && !msgTmp)
									{
										if (callback.GetUserCancel() || WGInput.m_allowedDerivedVariables[v] || (i == 2 && v == H_TRNG) || (i == 0 && v == H_WNDD) || v == H_PRES)
										{
											Dmin = 0;
											Dmax = 0;
										}
										else
										{
											messageTmp += msgTmp;
										}
#pragma omp flush(messageTmp)
									}

									if (msgTmp)
									{
#pragma omp critical(test)
{

	Dmin = min(Dmin, results[0].m_distance / 1000);
	if (results[0].m_distance / 1000 > Dmax)
	{
		maxIndex = l;
		Dmax = results[0].m_distance / 1000;
	}
}


#pragma omp flush(messageTmp)
if (messageTmp)
{
	messageTmp += callback.StepIt(0);
#pragma omp flush(messageTmp)
}

#pragma omp flush(messageTmp)
									}
								}
							}//for all locations

							if (messageTmp)
							{
								if (locations.size() > 1)
								{
									if (Dmin > CTRL.m_maxDistFromLOC)
									{
										if (CTRL.m_bRunEvenFar)
										{
											string warning = FormatMsg(IDS_SIM_BAD_NORMAL_REGION, ToString(CTRL.m_maxDistFromLOC), ToString(Dmin), GetVariableName(v));
											callback.AddMessage(GetString(IDS_STR_WARNING) + " : " + warning);
										}
										else
										{
											messageTmp.ajoute(FormatMsg(IDS_SIM_BAD_NORMAL_REGION, ToString(CTRL.m_maxDistFromLOC), ToString(Dmin), GetVariableName(v)));
										}
									}
								}


								if (messageTmp)
								{
									if (Dmax > CTRL.m_maxDistFromPoint)
									{
										string warning = FormatMsg(IDS_SIM_FAR_NORMAL_STATION, locations[maxIndex].m_name, ToString(Dmax), ToString(CTRL.m_maxDistFromPoint), GetVariableName(v));
										callback.AddMessage(GetString(IDS_STR_WARNING) + " : " + warning, 1);
									}
								}
							}
							else
							{
								//this variable can be founb by virtual variable
								
								//	messageTmp = ERMsg();
							}

							if (!messageTmp)
								msg.ajoute(messageTmp);

							msg += callback.StepIt();
						}//if variable
					}//for all variables
				}//if msg
			}//for all years
		}//is database open
	}//for all database


	omp_set_nested(nested);

	callback.PopTask();

    return msg;
}

std::string CWeatherGeneration::GetPath(const CFileManager& fileManager)const
{
	if( m_pParent==NULL)
		return fileManager.GetTmpPath()+ m_internalName +"\\";
	
	return m_pParent->GetPath(fileManager)+ m_internalName +"\\";
}

void CWeatherGeneration::InitValidationOptimisation()
{
   // m_lastWeatherTime = 0;
}


	
ERMsg CWeatherGeneration::Validation(CFileManager& fileManager)const
{
    ERMsg msg;
	msg = ValidationDB(fileManager);
	if( msg )
	{
		msg = ValidationOutputHomeDB(fileManager);
	}

	return msg;
}

ERMsg  CWeatherGeneration::ValidationDB(const CFileManager& fileManager)const
{
    ASSERT(!m_WGInputName.empty());
    ASSERT(!m_parametersVariationsName.empty());
    ASSERT(!m_locationsName.empty());
    
    ERMsg msg;
	
	__time64_t lastSimModif = GetFileStamp(GetPath(fileManager));
	__time64_t locTime= fileManager.Loc().GetLastUpdate(m_locationsName);

	if(locTime>lastSimModif )
    {
		msg.ajoute(FormatMsg(IDS_SIM_VALID_LOC_NEWER, m_locationsName));
    }

	return msg;
}
/*
ERMsg  CWeatherGeneration::ValidationOutput(const CFileManager& fileManager)const
{
	int nbVariation = GetNbParVariation();
	short nbIter = GetNbIteration();
	int nbLoc = GetNbLocStation();
	

    ERMsg msg;
	CTime lastSimModif = GetLastModificationDate();
    CTime lastTime = 0;

    if( m_lastWeatherTime == 0)
    {
		CWGInput WGInput;
		if( GetTGInParameter(fileManager, WGInput) )
		{
			if( fileManager.GetLastWeatherUpdate(WGInput.m_normalsDBName, WGInput.m_dailyDBName, lastTime) )
			{
				m_lastWeatherTime = lastTime;
			}
		}
    }        

    
    lastTime = m_lastWeatherTime;
    

    if(lastTime > lastSimModif )
    {
        msg.asgType(ERMsg::ERREUR);            
        msg.ajoute(UtilWin::GetCString(IDS_SIM_VALID_WEATHER_NEWER ));
    }
	
    if( msg )
    {
	    //std::string ouputFileName;
	    CFileStatus status;

        bool bRep = true;
    

	    for(int noLoc=0; (noLoc<nbLoc)&& bRep; noLoc++)
		    for(int noParam=0; (noParam<nbVariation)&&bRep; noParam++)
			    for(short iter=0; (iter< nbIter)&&bRep; iter++)
			    {
				    std::string ouputFileName = GetModelOutputFileName(GetPath(fileManager), noLoc, noParam, iter);
				    if( CFile::GetStatus( ouputFileName, status ) )   // static function
				    {
					    if( (status.m_size == 0) )
					    {
						    TRACE("file not valid in validation\n");
						    TRACE("simul no, %d, noLoc %d, noParam %d, iter %d\n",noLoc, noParam, (int)iter);
						    TRACE("status.m_size %d\n", status.m_size);
						    TRACE("lastSimModif %d\n", lastSimModif);
						    TRACE("status.m_mtime %d\n", status.m_mtime);
						    std::string messagetmp;
						    messagetmp.Format("file not valid in validation\n"
										    "simul no, %d, noLoc %d, noParam %d, iter %d\n"
										    "status.m_size %d\n"
										    "lastSimModif %d\n" 
										    "status.m_mtime %d\n", 
										    noLoc, noParam, iter,
										    status.m_size, lastSimModif, status.m_mtime);

                            std::string error;
                            msg.asgType(ERMsg::ERREUR);            
                            error.FormatMsg(IDS_SIM_VALID_OUT_EMPTY, ouputFileName);
                            msg.ajoute(error);
                            bRep = false;
						    break;
					    }
					    else if( (lastSimModif > status.m_mtime) )
					    {
						    TRACE("file not valid in validation\n");
						    TRACE("simul no, %d, noLoc %d, noParam %d, iter %d\n",noLoc, noParam, iter);
						    TRACE("status.m_size %d\n", status.m_size);
						    TRACE("lastSimModif %d\n", lastSimModif);
						    TRACE("status.m_mtime %d\n", status.m_mtime);
						    std::string messagetmp;
						    messagetmp.Format("file not valid in validation\n"
										    "simul no, %d, noLoc %d, noParam %d, iter %d\n"
										    "status.m_size %d\n"
										    "lastSimModif %d\n" 
										    "status.m_mtime %d\n", 
										    noLoc, noParam, iter,
										    status.m_size, lastSimModif, status.m_mtime);
                            std::string error;
                            msg.asgType(ERMsg::ERREUR);            
                            error.FormatMsg(IDS_SIM_VALID_OUT_OLDER, (LPCTSTR)ouputFileName);
                            msg.ajoute(error);
                            bRep = false;
						    break;
					    }
				    }
				    else
				    {
					    TRACE("file not found\n");
                        std::string error;
                        msg.asgType(ERMsg::ERREUR);            
                        error.FormatMsg(IDS_SIM_VALID_OUT_NOTFOUND, (LPCTSTR)ouputFileName);
                        msg.ajoute(error);
                        bRep = false;
				    }
			    }	// for(iter)

    }
	return msg;
}

ERMsg  CWeatherGeneration::ValidationOutputOLEDB(CFileManager& fileManager)const
{
    //CFileManager fileManager(fileManagerIn);


    ERMsg msg;
	CTime lastSimModif = GetLastModificationDate();
    CTime lastTime;

    if( m_lastWeatherTime == 0)
    {
		CWGInput WGInput;
		if( GetTGInParameter(fileManager, WGInput) )
		{
			if( fileManager.GetLastWeatherUpdate(WGInput.m_normalsDBName, WGInput.m_dailyDBName, lastTime) )
			{
				m_lastWeatherTime = lastTime;
			}
		}
    }        
    
    lastTime = m_lastWeatherTime;
    

    if(lastTime > lastSimModif )
    {
        msg.asgType(ERMsg::ERREUR);            
        msg.ajoute(UtilWin::GetCString(IDS_SIM_VALID_WEATHER_NEWER ));
    }
	
    if( msg )
    {
        std::string DBName = GetPath(fileManager) + GetDatabaseFileName();
        
	    std::string ouputFileName;
	    CFileStatus status;
	    
    
        if( CFile::GetStatus( DBName, status ) )   // static function
		{
			if( (status.m_size == 0) )
			{
				TRACE("database file empty\n");
				
                std::string error;
                msg.asgType(ERMsg::ERREUR);            
                error.FormatMsg(IDS_SIM_VALID_OUT_EMPTY, DBName);
                msg.ajoute(error);

			}
			else if( (lastSimModif > status.m_mtime) )
			{
				TRACE("file older than the simulation\n");
                
                std::string error;
                msg.asgType(ERMsg::ERREUR);            
                error.FormatMsg(IDS_SIM_VALID_OUT_OLDER, DBName);
                msg.ajoute(error);
			}
            else
            {
                msg = fileManager.OpenConnection(DBName);
                if( msg )
                {
                    short nbVariation = GetNbParVariation();
	                short nbIter = GetNbIteration();
	                short nbLoc = GetNbLocStation();
                
                    msg = fileManager.ValidationOutput(nbLoc, nbVariation, nbIter);
                    fileManager.CloseConnection();
                }
            }

		}
		else
		{
			TRACE("file not found\n");
            std::string error;
            msg.asgType(ERMsg::ERREUR);            
            error.FormatMsg(IDS_SIM_VALID_OUT_NOTFOUND, DBName);
            msg.ajoute(error);
		}
	    
    }
	return msg;
}
*/
ERMsg  CWeatherGeneration::ValidationOutputHomeDB(CFileManager& fileManager)const
{
    ERMsg msg;
	/*
	std::string DBFilePath = GetDBFilePath(GetPath(fileManager));
	__time64_t lastSimModif = GetLastModificationDate(DBFilePath);
    __time64_t lastSimModif = GetFileStamp(GetPath(fileManager));
	__time64_t modelInTime= fileManager.GetLastModelUpdate( m_parametersVariationsName, m_modelName, m_WGInputName);
	__time64_t locTime= fileManager.Loc().GetLastUpdate(m_locationsName);
	
	if(modelInTime>lastSimModif )
    {
		CStdString error;
        error.FormatMsg(IDS_SIM_VALID_MODEL_NEWER, m_parametersVariationsName);
		msg.ajoute(error);
    }

	if(locTime>lastSimModif )
    {
		CStdString error;
        error.FormatMsg(IDS_SIM_VALID_LOC_NEWER, m_locationsName);
        msg.ajoute(error);
    }
	

		CWGInput WGInput;
		if( LoadWGInput(fileManager, WGInput) )
		{
			if( fileManager.GetLastWeatherUpdate(WGInput.m_normalsDBName, WGInput.m_dailyDBName, WGInput.m_hourlyDBName) )
			{
				m_lastWeatherTime = lastTime;
			}
		}
    }        
    
    lastTime = m_lastWeatherTime;
    

    if(lastTime > lastSimModif )
    {
        msg.asgType(ERMsg::ERREUR);            
        msg.ajoute(UtilWin::GetCString(IDS_SIM_VALID_WEATHER_NEWER ));
    }
	
    if( msg )
    {
        std::string ouputFileName;
	    CFileStatus status;
	    
    
        if( CFile::GetStatus( DBFilePath, status ) )   // static function
		{
			if( (status.m_size == 0) )
			{
				TRACE("database file empty\n");
				
                std::string error;
                msg.asgType(ERMsg::ERREUR);            
                error.FormatMsg(IDS_SIM_VALID_OUT_EMPTY, DBFilePath);
                msg.ajoute(error);

			}
			else if( (lastSimModif > status.m_mtime) )
			{
				TRACE("file older than the simulation\n");
                
                std::string error;
                msg.asgType(ERMsg::ERREUR);            
                error.FormatMsg(IDS_SIM_VALID_OUT_OLDER, DBFilePath);
                msg.ajoute(error);
			}
            else
            {
				CResult db;
				msg = db.Open(DBFilePath, CFile::modeRead);
            }
		}
		else
		{
			TRACE("file not found\n");
            std::string error;
            error.FormatMsg(IDS_SIM_VALID_OUT_NOTFOUND, DBFilePath);
            msg.ajoute(error);
		}
    }
	*/
	return msg;
}

ERMsg CWeatherGeneration::GetLocations(const CFileManager& fileManager, CLocationVector& locations)const
{
    ERMsg msg;

	ASSERT( !m_locationsName.empty() );
	msg = fileManager.Loc().Get(m_locationsName, locations);

	
	if(msg)
	{
		ASSERT(locations.size() > 0);
		msg = locations.IsValid();
	}
	
    return msg;
}


ERMsg CWeatherGeneration::GetWGInput(const CFileManager& fileManager, CWGInput& WGInput)const
{
	ERMsg msg;

    if( m_WGInputName.empty() || m_WGInputName == STRDEFAULT)
    {
		WGInput.LoadDefaultParameter();
    }
    else
    {
		msg = fileManager.WGInput().Get(m_WGInputName, WGInput);
    }

	/*if (WGInput.m_variables[H_RELH] != WGInput.m_variables[H_TDEW])
	{
		msg.ajoute(GetString(IDS_WG_HUMIDITY_ERROR));
	}*/

    return msg;
}

ERMsg CWeatherGeneration::GetParametersVariations(const CFileManager& fileManager, CParametersVariationsDefinition& variations)const
{
	ERMsg msg;

	if (!m_parametersVariationsName.empty())
	{
		msg = fileManager.PVD(".WG").Get(m_parametersVariationsName, variations);
	}

	return msg;
}

ERMsg CWeatherGeneration::GetParentInfo(const CFileManager& fileManager, CParentInfo& info, CParentInfoFilter filter)const
{
	ERMsg msg;

	
	if (filter[LOCATION])
	{
		msg = fileManager.Loc().Get(m_locationsName, info.m_locations);
	}
    
	
	//verifier ici s'il y a des specialPath dans les types FILE
	if (filter[REPLICATION])
	{
		info.m_nbReplications = m_nbReplications;
	}

	if (filter[PARAMETER] || filter[TIME_REF] || filter[VARIABLE]) // || filter[CParentInfo::WG_INPUT]
	{
		CWGInput WGInput;
		msg = GetWGInput(fileManager, WGInput);
		if (msg)
		{
			if (filter[TIME_REF])
			{
				CTM TM = WGInput.GetTM();
				info.m_period = WGInput.GetTPeriod();
			}
			
			if (filter[VARIABLE])
			{
				info.m_variables = WGInput.GetOutputDefenition();
			}

			/*if (filter[CParentInfo::WG_INPUT])
			{
				info.m_WGInput = WGInput;
			}
*/
			//if (filter[PARAMETER])
			//{
			//	//if (!m_parametersVariationsName.empty())
			//	//{
			//	CParametersVariationsDefinition PVD;
			//	msg = fileManager.PVD(".WG").Get(m_parametersVariationsName, PVD);
			//	if (msg)
			//	{
			//		info.m_parameterset = PVD.GetModelInputVector(WGInput);
			//	}
			//	//}
			//}
		}
	}

	return msg;
}


ERMsg CWeatherGeneration::Execute(const CFileManager& fileManager, CCallback& callback)
{
	ASSERT( m_bExecute );

	ERMsg msg;

	//Load WGInput 
	CWGInput WGInput;
    if( msg ) 
		msg = GetWGInput(fileManager, WGInput);

	//find normal file path
	std::string NFilePath;
	if( msg )
		msg = fileManager.Normals().GetFilePath(WGInput.m_normalsDBName, NFilePath);

	//find daily file apth if any
	std::string DFilePath;
	if( msg && WGInput.UseDaily() )
		msg = fileManager.Daily().GetFilePath(WGInput.m_dailyDBName, DFilePath);

	std::string HFilePath;
	if (msg && WGInput.UseHourly() )
		msg = fileManager.Hourly().GetFilePath(WGInput.m_hourlyDBName, HFilePath);

	std::string GFilePath;
	if (msg && WGInput.UseGribs())
		msg = fileManager.Gribs().GetFilePath(WGInput.m_gribsDBName, GFilePath);

	
	//open normal database
	CNormalsDatabasePtr normalDB( new CNormalsDatabase );
	if (msg)
	{
		msg = normalDB->Open(NFilePath, CDailyDatabase::modeRead, callback);
		if (msg)
			msg = normalDB->OpenSearchOptimization(callback);//open here to be thread safe
	}
		
	
	//open daily databse
	CDailyDatabasePtr dailyDB;
	if (msg && WGInput.IsDaily())
	{
		dailyDB.reset( new CDailyDatabase );
		msg = dailyDB->Open(DFilePath, CDailyDatabase::modeRead, callback );
		if (msg)
			msg = dailyDB->OpenSearchOptimization(callback);//open here to be thread safe
	}
	

	CHourlyDatabasePtr hourlyDB;
	if (msg && WGInput.IsHourly())
	{
		hourlyDB.reset(new CHourlyDatabase);
		msg = hourlyDB->Open(HFilePath, CHourlyDatabase::modeRead, callback);
		if (msg)
			msg = hourlyDB->OpenSearchOptimization(callback);//open here to be thread safe
	}

	CGribsDatabasePtr pGribsDB;
	if (msg && WGInput.UseGribs())
	{
		pGribsDB = make_shared<CGribsDatabase>(WGInput.m_bAtSurfaceOnly);
		msg = pGribsDB->Open(GFilePath, callback);
	}

	//Load location list
    CLocationVector locations;
    if ( msg ) 
		msg = GetLocations(fileManager, locations);

	if( msg )
		msg = CheckLocationsInDatabase(normalDB, dailyDB, hourlyDB, locations, WGInput, callback);
	
	
	if(msg)
	{
		callback.AddMessage(FormatMsg(IDS_SIM_NORMAL_USED, NFilePath));
	
		if( WGInput.UseDaily() )
		{
			callback.AddMessage(FormatMsg(IDS_SIM_DAILY_USED, DFilePath));
		}
    
		if (WGInput.UseHourly())
		{
			callback.AddMessage(FormatMsg(IDS_SIM_HOURLY_USED, HFilePath));
		}

		if (WGInput.UseGribs())
		{
			callback.AddMessage(FormatMsg(IDS_SIM_GRIBS_USED, GFilePath));
		}

		//Generate weather
		msg = GenerateWeather(fileManager, normalDB, dailyDB, hourlyDB, pGribsDB, WGInput, locations, callback);
	}


	if( normalDB )
		normalDB->Close();
	
	if( dailyDB )
		dailyDB->Close();
	
	if (hourlyDB )
		hourlyDB->Close();
	
	if (pGribsDB)
		msg += pGribsDB->Close(callback);


	return msg;
}


ERMsg CWeatherGeneration::GenerateWeather(const CFileManager& fileManager, CNormalsDatabasePtr& normalDB, CDailyDatabasePtr& dailyDB, CHourlyDatabasePtr& hourlyDB,
						CGribsDatabasePtr& gribsDB, const CWGInput& WGInput, const CLocationVector& locations, CCallback& callback)
{
	ASSERT( !m_internalName.empty() );
  
	ERMsg msg;

	
	std::string outputPath = GetPath(fileManager);//Generate output path
	std::string DBFilePath = GetDBFilePath(outputPath);//Generate DB file path

	callback.AddMessage(FormatMsg(IDS_SIM_CREATE_DATABASE, m_name));
	callback.AddMessage(DBFilePath, 1);
	callback.PushTask(GetString(IDS_WG_CREATE_WEATHER), locations.size()*m_nbReplications, 1);


	CTimer timer(TRUE);

	bool bUseHxGrid = CTRL.m_bUseHxGrid && m_bUseHxGrid;
	short language = CRegistry().GetLanguage();

	
	CResult result;

	msg = result.Open(DBFilePath, std::fstream::binary | std::fstream::out | std::fstream::trunc);
	if (msg)
	{


		CModelInputVector modelInputVector(1);
		for (size_t i = 0; i < CWGInput::NB_MEMBERS; i++)
			modelInputVector[0].push_back(CModelInputParam(WGInput.GetMemberName(i), WGInput[i]));

		CDBMetadata& metadata = result.GetMetadata();
		metadata.SetLocations(locations);
		metadata.SetParameterSet(modelInputVector);
		metadata.SetNbReplications(m_nbReplications);
		metadata.SetModelName("WeatherGenerator");
		metadata.SetOutputDefinition(WGInput.GetOutputDefenition());

		vector<size_t> locPos;
		GetLocationIndexGrid(locations, locPos);

		CRandomGenerator rand(WGInput.m_seed);
		unsigned long seed = rand.Rand(1, CRandomGenerator::RAND_MAX_INT);

		int nested = omp_get_nested();
		int dynamic = omp_get_dynamic();
		omp_set_dynamic(0);
		omp_set_nested(0);

		int nbThreadsLoc = min(CTRL.m_nbMaxThreads, (int)locPos.size());//priority over location
		size_t memoryUsedByOneThread = sizeof(CWeatherYear)*m_nbReplications*WGInput.GetNbYears() / (1024 * 1024) * 120 / 100;
		size_t availableMemory = GetTotalSystemMemory() / (1024 * 1024) - 6000;

		//callback.AddMessage("Memory for one thres = " + ToString(memoryUsedByOneThread));
		//callback.AddMessage("Available memory = " + ToString(availableMemory));
		//callback.AddMessage("max threads = " + ToString(CTRL.m_nbMaxThreads));
		callback.AddMessage("Num loc threads = " + ToString(nbThreadsLoc));

		if (nbThreadsLoc > 1 && nbThreadsLoc*memoryUsedByOneThread > availableMemory)
		{
			nbThreadsLoc = max(1, int(availableMemory / memoryUsedByOneThread));
			callback.AddMessage("The number of threads was limited by available memory. " + ToString(CTRL.m_nbMaxThreads) + " --> " + ToString(nbThreadsLoc) + " threads");
		}


		bool bTestOK = true;

		//static,1 is to do simulation in the good other. This is best way to optimize weather cache
#pragma omp parallel for schedule(static, 1) shared(result, msg) num_threads(nbThreadsLoc) if ( !WGInput.UseGribs() ) 
		for (int ll = 0; ll < (int)locPos.size(); ll++)
		{
#pragma omp flush(msg)
			if (msg)
			{
				size_t l = locPos[ll];

				// init the loc part of WGInput
				CWeatherGenerator WG;
				WG.SetSeed(seed);
				WG.SetNbReplications(m_nbReplications);
				WG.SetWGInput(WGInput);
				WG.SetNormalDB(normalDB);
				WG.SetDailyDB(dailyDB);
				WG.SetHourlyDB(hourlyDB);
				WG.SetGribsDB(gribsDB);
				WG.SetTarget(locations[l]);

				ERMsg msgTmp = WG.Generate(callback);


#pragma omp flush(msg)
				if (msg && !msgTmp)
				{
					msg += msgTmp;
#pragma omp flush(msg)
				}

				for (size_t r = 0; r < m_nbReplications&& msg; r++)
				{
					//write info and weather to the stream
					const CSimulationPoint& weather = WG.GetWeather(r);
					ERMsg msgTmp = result.SetSection(metadata.GetSectionNo(l, 0, r), weather, callback);
#pragma omp flush(msg)
					if (msg && !msgTmp)
					{
						msg += msgTmp;
#pragma omp flush(msg)
					}

#pragma omp flush(msg)
					if (msg)
					{
						msg += callback.StepIt();
#pragma omp flush(msg)
					}


					CTPeriod p = weather.GetEntireTPeriod();
					for (CTRef TRef = p.Begin(); TRef <= p.End() && bTestOK; TRef++)
					{
						for (TVarH v = H_TAIR; v < NB_VAR_H; v++)
						{
							if (WGInput.m_variables[v])
							{
								if (weather[TRef][v][MEAN] < -100 || weather[TRef][v][MEAN]>100)
									bTestOK = false;
							}
						}
					}


					//wait when pause is activated								
					callback.WaitPause();
				}   // for replication
			}			// if (msg)
		}				// for(loc)


		timer.Stop();
		size_t nbGeneration = locations.size()*m_nbReplications*WGInput.GetNbYears();
		SetExecutionTime("WeatherGenerator", timer.Elapsed() / nbGeneration, WGInput.GetTM(), bUseHxGrid);


		omp_set_nested(nested);
		omp_set_dynamic(dynamic);

		result.Close();

		callback.AddMessage(string("Test Validation = ") + (bTestOK ? "OK" : "Failed"));
	}

	callback.PopTask();

	return msg;
}


void CWeatherGeneration::GetLocationIndexGrid(const CLocationVector& locArray, vector<size_t>& locPos)
{
	//for (size_t i = 0; i < locArray.size(); i++)
		//locPos[i] = (int)i;


	//boost::dynamic_bitset<size_t> done;
	//done.resize(locArray.size());
	//done.reset();
	//locPos.resize(locArray.size());
	
	KMterm	term(100, 0, 0, 0,		// run for 100 stages
		0.10,			// min consec RDL
		0.10,			// min accum RDL
		3,				// max run stages
		0.50,			// init. prob. of acceptance
		100,			// temp. run length
		0.95);			// temp. reduction factor

	//	term.setAbsMaxTotStage(stages);		// set number of stages


	
	size_t nbDim = 3;
	KMdata dataPts((int)nbDim, (int)locArray.size());		// allocate data storage
	for (int i = 0; i<(int)locArray.size(); i++)
	{
		double xx = Deg2Rad(locArray[i].m_x + 180);
		double yy = Deg2Rad(90 - locArray[i].m_y);

		dataPts[i][0] =  6371 * 1000 * cos(xx)*sin(yy); 
		dataPts[i][1] =  6371 * 1000 * sin(xx)*sin(yy); 
		dataPts[i][2] =  6371 * 1000 * cos(yy); 

	}

	////bool bStandardize= true;
	////if( bStandardize )
	////{
	//for (int i = 0; i<dataPts.getNPts(); i++)
	//{
	//	for (int j = 0; j<dataPts.getDim(); j++)
	//	{
	//		dataPts[i][j] = (dataPts[i][j] - stats[j][MEAN]) / stats[j][STD_DEV];
	//	}
	//}
	////}

	dataPts.buildKcTree();			// build filtering structure

	int nbCluster = (int)sqrt(locArray.size());
	KMfilterCenters ctrs(nbCluster, dataPts);		// allocate centers
	

	CTimer timer(true);
	// run each of the algorithms
	//cout << "\nExecuting Clustering Algorithm: Lloyd's\n";
	//KMlocalLloyds kmLloyds(ctrs, term);		// repeated Lloyd's
	//ctrs = kmLloyds.execute();			// execute
	//timer.Stop();   
	//printSummary("Lloyds",kmLloyds, dataPts, ctrs);	// print summary
	//cout << timer.Elapsed() << " s" << endl;

	timer.Start(true);
	cout << "\nExecuting Clustering Algorithm: Swap\n";
	KMlocalSwap kmSwap(ctrs, term);		// Swap heuristic
	ctrs = kmSwap.execute();
	
	timer.Stop();
	/*printSummary("Swap", kmSwap, dataPts, ctrs);
	cout << timer.Elapsed() << " s" << endl;

	timer.Start(true);
	cout << "\nExecuting Clustering Algorithm: EZ-Hybrid\n";
	KMlocalEZ_Hybrid kmEZ_Hybrid(ctrs, term);	// EZ-Hybrid heuristic
	ctrs = kmEZ_Hybrid.execute();
	timer.Stop();
	printSummary("EZ-Hybrid", kmEZ_Hybrid, dataPts, ctrs);
	cout << timer.Elapsed() << " s" << endl;

	timer.Start(true);
	cout << "\nExecuting Clustering Algorithm: Hybrid\n";
	KMlocalHybrid kmHybrid(ctrs, term);		// Hybrid heuristic
	ctrs = kmHybrid.execute();
	timer.Stop();
	printSummary("Hybrid", kmHybrid, dataPts, ctrs);
	cout << timer.Elapsed() << " s" << endl;
	*/

	int ts = kmSwap.getTotalStages();
	


	
	//pointOut.m_bGeographic = bGeographic;
	KMcenterArray clusterPts = ctrs.getCtrPts();

	CLocationVector test;
	// get point assignments
	//KMdataIdx
	KMctrIdxArray closeCtr = new KMctrIdx[locArray.size()];
	double		sqDist = 0;

	
	// closest center per point
	ctrs.getAssignments(closeCtr, NULL);		// sq'd dist to center
	//ctrs.getData().getKcTree()->getNeighbors()


	vector< pair<int, size_t>> pos(locArray.size());

	for (size_t i = 0; i < locArray.size(); i++)
		pos[i] = make_pair(closeCtr[i], i);
	
	std::sort(pos.begin(), pos.end());

	locPos.resize(locArray.size());
	for (size_t i = 0; i < locArray.size(); i++)
		locPos[i] = pos[i].second;


	//for (int i = 0; i < ctrs.getK(); i++)
	//{
	//	//const KMcenter& Kc = ctrs[i];
	//	CLocation loc;
	//	loc.m_x = Rad2Deg(atan2(ctrs[i][1], ctrs[i][0])) - 180;
	//	loc.m_y = 90 - Rad2Deg(acos(ctrs[i][2] / (6371 * 1000)));
	//	
	//	test.push_back(loc);
	//	//		
	//	//CLocation ptTmp; 
	//	//ptTmp.SetXY(ctrs[i][0], ctrs[i][1], ctrs[i][2]);
	//}

	//test.Save("D://KmeanCluster.csv");
	//CStatistic X;
	//CStatistic Y;

	////compute stats
	//for(int i=0; i<locArray.size(); i++)
	//{
	//	X+=locArray[i].m_lon+360;
	//	Y+=locArray[i].m_lat+360;
	//}

	//CGeoPoint pt1(X[LOWEST]-360, Y[LOWEST]-360, PRJ_WGS_84 );
	//CGeoPoint pt2(X[HIGHEST]-360, Y[HIGHEST]-360, PRJ_WGS_84);
	//CGeoDistance d = pt1.GetDistanceXY(pt2);

	//int divSizeX = (int)fabs(d.m_x/50000);
	//int divSizeY = (int)fabs(d.m_y/50000);

	//vector< vector< vector<int> > > locPos²(divSizeX+1);
	//for(size_t i=0; i<locPos².size(); i++)
	//	locPos²[i].resize(divSizeY+1);

	//for(int i=0; i<locArray.size(); i++)
	//{
	//	TNT::Subscript x=(int)Min(divSizeX, Max(0, (int)(((locArray[i].m_lon-(X[LOWEST]-360))/(X[HIGHEST]-X[LOWEST]))*(divSizeX+1))));
	//	TNT::Subscript y=(int)Min(divSizeY, Max(0, (int)(((locArray[i].m_lat-(Y[LOWEST]-360))/(Y[HIGHEST]-Y[LOWEST]))*(divSizeY+1))));

	//	locPos²[x][y].push_back(i);
	//}

	//
	//
	//
	//for(size_t x=0; x<locPos².size(); x++)
	//	for(size_t y=0; y<locPos²[x].size(); y++)
	//		locPos.insert(locPos.end(), locPos²[x][y].begin(), locPos²[x][y].end());

	//ASSERT( (int)locPos.size() == locArray.size() );
}




void CWeatherGeneration::writeStruc(zen::XmlElement& output)const
{
	CExecutable::writeStruc(output);

	zen::XmlOut out(output);

	out[CWeatherGeneration::GetMemberName(CWeatherGeneration::WG_INPUT_NAME)](m_WGInputName);
	out[CWeatherGeneration::GetMemberName(CWeatherGeneration::LOCATIONS_NAME)](m_locationsName);
	out[CWeatherGeneration::GetMemberName(CWeatherGeneration::PVARIATIONS_NAME)](m_parametersVariationsName);
	out[CWeatherGeneration::GetMemberName(CWeatherGeneration::NB_REPLICATIONS)](m_nbReplications);
	out[CWeatherGeneration::GetMemberName(CWeatherGeneration::USE_HXGRID)](m_bUseHxGrid);
}

bool CWeatherGeneration::readStruc(const zen::XmlElement& input)
{
	

	CExecutable::readStruc(input);

	zen::XmlIn in(input);
	in[CWeatherGeneration::GetMemberName(CWeatherGeneration::WG_INPUT_NAME)](m_WGInputName);
	in[CWeatherGeneration::GetMemberName(CWeatherGeneration::LOCATIONS_NAME)](m_locationsName);
	in[CWeatherGeneration::GetMemberName(CWeatherGeneration::PVARIATIONS_NAME)](m_parametersVariationsName);
	in[CWeatherGeneration::GetMemberName(CWeatherGeneration::NB_REPLICATIONS)](m_nbReplications);
	in[CWeatherGeneration::GetMemberName(CWeatherGeneration::USE_HXGRID)](m_bUseHxGrid);

	return true;
}
}