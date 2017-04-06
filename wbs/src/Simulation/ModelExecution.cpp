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
#include <boost/algorithm/string.hpp>

#include <sstream>
#include <omp.h>
#include <WTypes.h>
#include <psapi.h>

#include "Basic/WeatherStation.h"
#include "Basic/ModelStat.h"
#include "Basic/GeoBasic.h"
#include "Basic/Timer.h"
#include "Basic/hxGrid.h"
#include "ModelBase/Model.h"
#include "ModelBase/WGInput.h"
#include "ModelBase/CommunicationStream.h"
#include "Simulation/LoadStaticData.h"
#include "FileManager/FileManager.h"
#include "Simulation/ExecutableFactory.h"
#include "ModelBase/WGInput-ModelInput.h"
#include "Simulation/ModelExecution.h"

#include "WeatherBasedSimulationString.h"


using namespace std;
using namespace VITALENGINE;
using namespace WBSF::WEATHER;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::DIMENSION;

namespace WBSF
{

	static const char mutexName[] = "hxGridMutex";
	static const __int8 DIRECT_ACCESS = 0;


//**************************************************************
//CModelExecution

//CTime CModelExecution::m_lastWeatherTime = 0;
const char* CModelExecution::XML_FLAG = "ModelGeneration";
const char* CModelExecution::MEMBERS_NAME[NB_MEMBERS_EX] = {"ModelName", "ModelInputName", "LocationsName", "ParamVariationsName", "SeedType", "NbReplications", "UseHxGrid" };
const int CModelExecution::CLASS_NUMBER = CExecutableFactory::RegisterClass(CModelExecution::GetXMLFlag(), &CModelExecution::CreateObject );
    

CModelExecution::CModelExecution()
{
	Reset();
}

CModelExecution::CModelExecution(const CModelExecution& in)
{
	operator=(in);
}


CModelExecution::~CModelExecution()
{}

void CModelExecution::Reset()
{
	CExecutable::Reset();

	m_name = "ExecuteModel";
	m_modelName.clear();
	m_locationsName.clear();
	m_modelInputName = STRDEFAULT;
	m_paramVariationsName.clear();
	m_seedType = 0;
	m_nbReplications = 1;
	m_bUseHxGrid=false;
}


CModelExecution& CModelExecution::operator =(const CModelExecution& in)
{
    if( &in != this )
    {
		CExecutable::operator =(in);
	    m_modelName = in.m_modelName;
	    m_modelInputName = in.m_modelInputName;
	    m_locationsName = in.m_locationsName;
		m_paramVariationsName = in.m_paramVariationsName;
		m_seedType = in.m_seedType;
		m_nbReplications = in.m_nbReplications;
		m_bUseHxGrid = in.m_bUseHxGrid;
    }

	return *this;
}


bool CModelExecution::operator == (const CModelExecution& in)const
{
	bool bEqual = true;

	if( CExecutable::operator!=(in) )bEqual = false;
	if( m_modelName != in.m_modelName) bEqual = false;
	if( m_modelInputName != in.m_modelInputName)bEqual = false;
	if( m_locationsName != in.m_locationsName)bEqual = false;
	if (m_paramVariationsName != in.m_paramVariationsName)bEqual = false;
	if (m_seedType != in.m_seedType)bEqual = false;
	if( m_nbReplications != in.m_nbReplications)bEqual = false;
	if( m_bUseHxGrid != in.m_bUseHxGrid)bEqual = false;

	return bEqual;
}


std::string CModelExecution::GetPath(const CFileManager& fileManager)const
{
	if( m_pParent==NULL)
		return fileManager.GetTmpPath()+ m_internalName +"\\";
	
	return m_pParent->GetPath(fileManager)+ m_internalName +"\\";
}

std::string CModelExecution::GetModelOutputFilePath(const std::string& path, size_t l, size_t WGp, size_t WGr, size_t p, size_t r)const
{
    ASSERT( IsPathEndOk(path));

	return FormatA("%sO_%09d_%09d_%09d_%09d_%09d.csv", path.c_str(), l + 1, WGp+1, WGr+1, p + 1, r + 1);
}

std::string CModelExecution::GetWGFilePath(const std::string& path, size_t l, size_t WGp, size_t WGr)const
{
	ASSERT( IsPathEndOk(path));

	return FormatA("%sW_%09d_%09d_%09d.csv", path.c_str(), l + 1, WGp + 1, WGr + 1);
}

void CModelExecution::InitValidationOptimisation()
{
   // m_lastWeatherTime = 0;
}


	
ERMsg CModelExecution::Validation(CFileManager& fileManager)const
{
    ERMsg msg;
	msg = ValidationDB(fileManager);
	if( msg )
	{
		msg = ValidationOutputHomeDB(fileManager);
	}

	return msg;
}

ERMsg  CModelExecution::ValidationDB(const CFileManager& fileManager)const
{
    ASSERT(!m_modelInputName.empty());
    ASSERT(!m_locationsName.empty());
    
    ERMsg msg;
	

//	CModelExecutionDB db;
	//msg = db.Open( GetDBFilePath(GetPath(fileManager)), CFile::modeRead);

	__time64_t lastSimModif = GetFileStamp(GetPath(fileManager));
	__time64_t modelInTime= fileManager.GetLastModelUpdate( m_modelInputName, m_modelName, "");
	__time64_t locTime= fileManager.Loc().GetLastUpdate(m_locationsName);
	
	if(modelInTime>lastSimModif )
    {
		msg.ajoute(FormatMsg(IDS_SIM_VALID_MODEL_NEWER, m_modelInputName));
    }

	if(locTime>lastSimModif )
    {
		msg.ajoute(FormatMsg(IDS_SIM_VALID_LOC_NEWER, m_locationsName));
    }
	

	return msg;
}
ERMsg  CModelExecution::ValidationOutputHomeDB(CFileManager& fileManager)const
{
    ERMsg msg;
	/*
	std::string DBFilePath = GetDBFilePath(GetPath(fileManager));
	__time64_t lastSimModif = GetLastModificationDate(DBFilePath);
    __time64_t lastSimModif = GetFileStamp(GetPath(fileManager));
	__time64_t modelInTime= fileManager.GetLastModelUpdate( m_modelInputName, m_modelName, m_TGInputName);
	__time64_t locTime= fileManager.Loc().GetLastUpdate(m_locationsName);
	
	if(modelInTime>lastSimModif )
    {
		CStdString error;
        error.FormatMsg(IDS_SIM_VALID_MODEL_NEWER, m_modelInputName);
		msg.ajoute(error);
    }

	if(locTime>lastSimModif )
    {
		CStdString error;
        error.FormatMsg(IDS_SIM_VALID_LOC_NEWER, m_locationsName);
        msg.ajoute(error);
    }
	

		CTGInput TGInput;
		if( LoadTGInput(fileManager, TGInput) )
		{
			if( fileManager.GetLastWeatherUpdate(TGInput.m_normalsDBName, TGInput.m_dailyDBName, TGInput.m_hourlyDBName) )
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



ERMsg CModelExecution::GetModelInput(const CFileManager& fileManager, CModelInput& modelInput)const
{
    CModel model;
	
    ERMsg msg = fileManager.Model().Get( m_modelName, model);

	if( !msg)
		return msg;

    
    if( m_modelInputName.empty() || m_modelInputName == STRDEFAULT)
    {
	    model.GetDefaultParameter(modelInput);
    }
    else
    {
	    if( msg )
			msg = fileManager.ModelInput(model.GetExtension()).Get(	m_modelInputName, modelInput);

		if(msg)
            msg = model.VerifyModelInput(modelInput);
    }

	//verifier ici s'il y a des specialPath dans les types FILE
	modelInput.SpecialPath2FilePath(fileManager.GetAppPath(), fileManager.GetProjectPath() );
    
	return msg;
}


ERMsg CModelExecution::GetParamVariationsDefinetion(const CFileManager& fileManager, CParametersVariationsDefinition& paramVariationDef)const
{
	ERMsg msg;

	if (!m_paramVariationsName.empty() && m_paramVariationsName != STRDEFAULT)
	{
		CModel model;

		ERMsg msg = fileManager.Model().Get(m_modelName, model);

		if (!msg)
			return msg;

		
		if (msg)
			msg = fileManager.PVD(model.GetExtension()).Get(m_paramVariationsName, paramVariationDef);
	}

	return msg;
}

ERMsg CModelExecution::GetParentInfo(const CFileManager& fileManager, CParentInfo& info, CParentInfoFilter filter)const
{
	ERMsg msg;

	msg = m_pParent->GetParentInfo(fileManager, info, filter);
	if (msg)
	{
		if (filter[LOCATION])
		{
		}

		if (filter[PARAMETER])
		{
			//replace parent parameters variations by the combination of both
			CModelInput modelInput;
			CParametersVariationsDefinition paramVariations;
			msg += GetModelInput(fileManager, modelInput);
			msg += GetParamVariationsDefinetion(fileManager, paramVariations);
			if (msg)
			{
				info.m_parameterset = paramVariations.GetModelInputVector(modelInput);
			}
		}

		if (filter[REPLICATION])
		{
			info.m_nbReplications *= m_nbReplications;
		}

		
		if (filter[TIME_REF] || filter[VARIABLE])
		{
			CModel model;
			msg = fileManager.Model().Get(m_modelName, model);

			if (msg)
			{
				if (filter[TIME_REF])
					info.m_period.Transform(model.m_outputTM);

				if (filter[VARIABLE])
					info.m_variables = model.GetOutputDefinition();
			}
		}
	}
    
	

	return msg;
}
//
//ERMsg CModelExecution::LoadStaticData(const CFileManager& fileManager, const CModel& model, const CModelInput& modelInput, std::ostream& stream)
//{
//	ERMsg msg;
//
//
//	StringVector fileList;
//
//	CModelInputParameterDefVector inputDef=model.GetInputDefinition(true);
//
//	for (size_t i = 0; i<inputDef.size(); i++)
//	{
//		if (/*inputDef[i].IsExtendedList() || */inputDef[i].GetType() == CModelInputParameterDef::kMVFile)
//		{
//			string filePath;
//			/*if (inputDef[i].IsExtendedList())
//			{
//				int defPos = ToInt(inputDef[i].m_default);
//				StringVector listOfParam = inputDef[i].GetList(fileManager.GetAppPath(), fileManager.GetProjectPath());
//				filePath = SpecialPath2FilePath(listOfParam[1], );
//
//			}
//			else if (inputDef[i].GetType() == CModelInputParameterDef::kMVFile)
//			{*/
//				filePath = modelInput[i].GetFilePath();
//			//}
//
//			fileList.push_back(filePath);
//		}
//	}
//
//	if (msg && !fileList.empty())
//	{
//		CStaticDataStream staticData;
//		msg = staticData.m_files.Load(fileList);
//		if (msg)
//			msg = staticData.WriteStream(stream);
//	}
//
//
//	return msg;
//
//}

//const CWGInput& WGInput,

CTransferInfoIn CModelExecution::FillTransferInfo(CModel& model, const CLocationVector& locations, size_t WGReplication, const CModelInputVector& modelInputVector, size_t seed, size_t l, size_t WGr, size_t p, size_t r)
{
	CTransferInfoIn info;
	
	std::string path = GetPath(GetFileManager());

	info.m_transferTypeVersion = model.GetTransferFileVersion();
	info.m_inputInfoPath = path;
	info.m_inputWeatherFilePath = model.GetTransferFileVersion() != CModel::VERSION_STREAM ? GetWGFilePath(path, l, 0, WGr) : "";
	info.m_outputFilePath = model.GetTransferFileVersion() != CModel::VERSION_STREAM ? GetModelOutputFilePath(path, l, 0, WGr, p, r) : "";
	//info.m_normalDBFilePath = GetFileManager().Normal().GetFilePath(WGInput.m_normalsDBName);
	//info.m_dailyDBFilePath = WGInput.UseDaily()?GetFileManager().Daily().GetFilePath(WGInput.m_dailyDBName):"";
	//info.m_hourlyDBFilePath = WGInput.UseHourly() ? GetFileManager().Hourly().GetFilePath(WGInput.m_hourlyDBName) : "";

	info.m_locationsFilePath = locations.GetFilePath();
	info.m_modelName = model.GetName();
	//info.m_WGInputName = WGInput.GetName();
	info.m_modelInputName = modelInputVector[p].GetName();
	
	info.m_locCounter = CCounter(l, locations.size());
	info.m_paramCounter = CCounter(p, modelInputVector.size());
	info.m_repCounter = CCounter(WGr*m_nbReplications + r, m_nbReplications*WGReplication);

	info.m_loc = locations[l];
	//info.m_WG = WGInput;
	info.m_inputParameters = modelInputVector[p].GetParametersVector();
	info.m_outputVariables = model.GetOutputDefinition().GetParametersVector();
	info.m_seed = seed;
	info.m_TM = model.m_outputTM;
	info.m_language = CRegistry().GetLanguage();
	
	return info;
}



ERMsg CModelExecution::Execute(const CFileManager& fileManager, CCallback& callback)
{
	ASSERT( m_bExecute );
	
    ERMsg msg;

	//Load Model
	CModel model;
	if ( msg ) 
		msg = fileManager.Model().Get(m_modelName, model);
    
    //Load model input 
    CModelInput modelInput;    
    if( msg ) 
        msg = GetModelInput(fileManager, modelInput);

	//Generate model input parameter set
	CModelInputVector modelInputVector;
	if (msg)
	{
		//Load model parameters variations
		CParametersVariationsDefinition paramVariations;
		msg = GetParamVariationsDefinetion(fileManager, paramVariations);
		if (msg)
			modelInputVector = paramVariations.GetModelInputVector(modelInput);
	}

	//verify hxGrid
	bool bUseHxGrid = CTRL.m_bUseHxGrid && m_bUseHxGrid;
	if (msg && bUseHxGrid && (model.GetTransferFileVersion() != CModel::VERSION_STREAM || !model.GetThreadSafe()))
	{
		//can't use hxGrid if the model is not stream's communication or thread safe
		msg.ajoute(GetString(IDS_SIM_MODEL_NOT_HXGRID) );
	}

	
	if(msg)
	{
		
		CResultPtr pWeather = GetParent()->GetResult(fileManager);
		msg = pWeather->Open();
		if (msg)
		{
			const CDBMetadata& metadataIn = pWeather->GetMetadata();
			const CLocationVector& locations = metadataIn.GetLocations();
			size_t nbReplications = m_nbReplications*metadataIn.GetNbReplications();
			
			if (metadataIn.GetTPeriod().GetNbYears() < model.GetNbYearMin() || metadataIn.GetTPeriod().GetNbYears() > model.GetNbYearMax())
				msg.ajoute(FormatMsg(IDS_BSC_NB_YEAR_INVALID, ToString(metadataIn.GetTPeriod().GetNbYears()), model.GetName(), ToString(model.GetNbYearMin()), ToString(model.GetNbYearMax())));

			if (msg)
				msg = model.VerifyInputs(locations.GetSSIHeader(), metadataIn.GetOutputDefinition().GetWVariables());

			if (msg)
			{
				string filePath = GetDBFilePath(GetPath(fileManager));

				CResult result;
				msg = result.Open(filePath, std::fstream::out|std::fstream::binary);

				if (msg)
				{
					CTimer timerModel(true);
					CModelInputVector finalModelInputVector = modelInputVector*metadataIn.GetParameterSet();

					CDBMetadata& metadataOut = result.GetMetadata();
					metadataOut.SetModelName(model.GetName());
					metadataOut.SetLocations(locations);
					metadataOut.SetParameterSet(finalModelInputVector);
					metadataOut.SetNbReplications(nbReplications);
					metadataOut.SetOutputDefinition(model.GetOutputDefinition());

					callback.PushTask(FormatMsg(IDS_SIM_CREATE_DATABASE, m_name), locations.size()*finalModelInputVector.size()*nbReplications);
					callback.AddMessage(FormatMsg(IDS_SIM_CREATE_DATABASE, m_name));
					callback.AddMessage(model.GetCopyright());
					callback.AddMessage(filePath);
					


					if (model.GetTransferFileVersion() == CModel::VERSION_STREAM)
					{
						if (bUseHxGrid)
							msg = RunHxGridSimulation(fileManager, model, modelInputVector, *pWeather, result, callback);
						else
							msg = RunStreamSimulation(fileManager, model, modelInputVector, *pWeather, result, callback);
					}
					else
					{
						//run by files
						msg = RunFileSimulation(fileManager, model, modelInputVector, *pWeather, result, callback);
					}

					result.Close();
					timerModel.Stop();


					//update timer only on success
					size_t nbGeneration = locations.size()*nbReplications*finalModelInputVector.size()*metadataOut.GetTPeriod().GetNbYears();
					SetExecutionTime(model.GetName(), timerModel.Elapsed() / nbGeneration, metadataOut.GetTPeriod().GetTM(), bUseHxGrid);


					callback.PopTask();
				}
			}
		}
	}

	return msg;
}


ERMsg CModelExecution::RunStreamSimulation(const CFileManager& fileManager, CModel& model, const CModelInputVector& modelInputVector, const CResult& weather, CResult& result, CCallback& callback)
{
	ERMsg msg;

	//load files in memory for stream transfer
	stringstream staticDataStream;
	msg = LoadStaticData(fileManager, model, modelInputVector.m_pioneer, staticDataStream);
	if( msg )
		msg = model.SetStaticData(staticDataStream);

	staticDataStream.clear();//clear any bits set
	staticDataStream.str(std::string());
	
	if(!msg)
		return msg;

	
	const CLocationVector& locations = weather.GetMetadata().GetLocations();
	const CTPeriod period = weather.GetMetadata().GetTPeriod();
	size_t WGNbReplications = weather.GetMetadata().GetNbReplications();

	CRandomGenerator rand(m_seedType);
	vector<unsigned long> seeds(m_nbReplications);
	for (vector<unsigned long>::iterator it = seeds.begin(); it!=seeds.end(); it++)
		*it = rand.Rand(1, CRandomGenerator::RAND_MAX_INT);

	// for all loc station in the loc array
	int totalExec=0;

	int nested = omp_get_nested();
	int dynamic = omp_get_dynamic();
	
	
	bool bUseWGRep = WGNbReplications >= m_nbReplications;
	size_t nbReplications = bUseWGRep ? WGNbReplications : m_nbReplications;
	
	double repLocRatio = (double)nbReplications/locations.size();

	if (repLocRatio > 1)
		repLocRatio = 1 - 1.0 / repLocRatio;

	int nbThreadsRep = max(1, min(CTRL.m_nbMaxThreads-1, int(CTRL.m_nbMaxThreads*repLocRatio)));
	int nbThreadsLoc = max(1, min(int(ceil(CTRL.m_nbMaxThreads / nbThreadsRep)), int(CTRL.m_nbMaxThreads*(1 - repLocRatio))));
	
	size_t memoryUsedByOneThread = sizeof(CWeatherYear)*period.GetNbYears() / (1024 * 1024) * 120 / 100;
	size_t availableMemory = GetTotalSystemMemory() / (1024 * 1024) - 6000;

	if (nbThreadsLoc>1 && nbThreadsLoc*memoryUsedByOneThread > availableMemory)
	{
		nbThreadsLoc = max(1, int(availableMemory / memoryUsedByOneThread ));
		callback.AddMessage("The number of threads was limited by available memory. " + ToString(CTRL.m_nbMaxThreads) + " --> " + ToString(nbThreadsLoc) + " threads");
	}

	omp_set_dynamic(0);
	omp_set_nested(model.GetThreadSafe() && nbThreadsRep>1);

	size_t n = 0;
#pragma omp parallel for schedule(static, 1) shared(result, totalExec, msg) num_threads(nbThreadsLoc) if ( model.GetThreadSafe() ) 
	for (__int64 l = 0; l<(int)locations.size(); l++)
	{
		#pragma omp flush(msg)
		if (msg)
		{
#pragma omp parallel for shared(result, totalExec, msg)  num_threads(nbThreadsRep) if (bUseWGRep && model.GetThreadSafe() ) 
			for (__int64 WGr = 0; WGr<(__int64)WGNbReplications; WGr++)
			{
				#pragma omp flush(msg)
				if (msg)
				{

					//write info and weather to the stream
					CSimulationPoint simulationPoint;
						
					weather.GetSection(weather.GetSectionNo(l, 0, WGr), simulationPoint, model.m_variables);
					ASSERT(!simulationPoint.empty());
						

					for (size_t p = 0; p < modelInputVector.size() && msg; p++)
					{
#pragma omp parallel for shared(result, totalExec, msg)  num_threads(nbThreadsRep) if (!bUseWGRep && model.GetThreadSafe() ) 
						for (__int64 r = 0; r < (__int64)m_nbReplications; r++)
						{
#pragma omp flush(msg)
							if (msg)
							{
								stringstream inStream;
								stringstream outStream;

								//get transfer info
								CTransferInfoIn info = FillTransferInfo(model, locations, WGNbReplications, modelInputVector, seeds[r], l, WGr, p, r);
								CCommunicationStream::WriteInputStream(info, simulationPoint, inStream);

								ERMsg msgTmp = model.RunModel(inStream, outStream);	// call DLL
#pragma omp flush(msg)
								if (msg && !msgTmp)
								{
									msg += msgTmp;
									msg.ajoute(FormatMsg(IDS_SIM_SIMULATION_ERROR, ToString(l + 1), locations[l].m_name, locations[l].m_ID));
#pragma omp flush(msg)
								}

#pragma omp flush(msg)
								if (msg)
								{
									//get output from stream
									CTransferInfoOut infoOut;
									CModelStatVector section; 

									ERMsg msgTmp = CCommunicationStream::ReadOutputStream(outStream, infoOut, section);
#pragma omp flush(msg)
									if (msg && !msgTmp)
									{
										msg += msgTmp;
										msg.ajoute(FormatMsg(IDS_SIM_SIMULATION_ERROR, ToString(l + 1), locations[l].m_name, locations[l].m_ID));
#pragma omp flush(msg)
									}


#pragma omp flush(msg)
									if (msg)
									{
										section.ConvertValue(model.GetMissValue(), (float)VMISS);
										ERMsg msgTmp = result.SetSection(infoOut.GetSectionNo(), section, callback);
#pragma omp flush(msg)
										if (msg && !msgTmp)
										{
											msg += msgTmp;
#pragma omp flush(msg)
										}

#pragma omp flush(msg)
										if (msg)
										{
#pragma omp atomic
											n++;
											msg += callback.SetCurrentStepPos(n);
#pragma omp flush(msg)
										}

#pragma omp atomic
										totalExec++;

										//wait when pause is activated								
										callback.WaitPause();
									}// if (msg)
								}// if (msg)
							}// if (msg)
						}//for all model replications
					}//for all model parameters set
				}// if (msg)
			}   //for all weather replications
		}			//if (msg)
	}				//for all locations 


	omp_set_nested( nested );
	omp_set_dynamic(dynamic);

	return msg;
}


ERMsg CModelExecution::RunFileSimulation(const CFileManager& fileManager, CModel& model, 
                              const CModelInputVector& modelInputVector, 
							  const CResult& weather, CResult& result, CCallback& callback)
{
	ASSERT( !m_internalName.empty() );

	ERMsg msg;
	
    callback.AddMessage( model.GetCopyright());

	//language
	const short language = CRegistry().GetLanguage();
	
	//Generate output path
	std::string outputPath = GetPath(fileManager);
    
	//Generate DB file path
	std::string DBFilePath = GetDBFilePath(outputPath);

	
	const CDBMetadata& metadata = weather.GetMetadata();
	const CLocationVector& locations = metadata.GetLocations();
	size_t WGNbReplications = metadata.GetNbReplications();
	
	CRandomGenerator rand(m_seedType);
	vector<unsigned long> seeds(m_nbReplications);
	for (vector<unsigned long>::iterator it = seeds.begin(); it != seeds.end(); it++)
		*it = rand.Rand(1, CRandomGenerator::RAND_MAX_INT);


	// for all locations
	for (size_t l = 0; l<locations.size() && msg; l++)
	{
		//for all weather replications
		for (size_t WGr = 0; WGr < WGNbReplications&& msg; WGr++)
		{
			CSimulationPoint simulationPoint;
			weather.GetSection(weather.GetSectionNo(l, 0, WGr), simulationPoint, model.m_variables);
			std::string WGFilePath = GetWGFilePath(outputPath, l, 0, WGr);
			msg = simulationPoint.SaveData(WGFilePath);

			// for all model parameters set
			for (size_t p = 0; p < modelInputVector.size() && msg; p++)
			{
				// for all model replications
				for (size_t r = 0; r < m_nbReplications && msg; r++)
				{
					std::string modelOuputFP = GetModelOutputFilePath(outputPath, l, 0, WGr, p, r);

					CTransferInfoIn info = FillTransferInfo(model, locations, WGNbReplications, modelInputVector, seeds[r], l, WGr, p, r);
					std::string IDsFilePath = outputPath + "IDS.txt";
					msg = info.Save(IDsFilePath);

					if (msg)
					{

						msg = model.RunModel(IDsFilePath);	// run executable
						if (msg)
						{
							CModelStatVector section;
							msg = section.Load(modelOuputFP);
							if (msg)
							{
								section.ConvertValue(model.GetMissValue(), (float)VMISS);
								result.AddSection(section);
							}

							if (!CTRL.m_bKeepTmpFile)
								RemoveFile(modelOuputFP);
						}
					}

					msg += callback.StepIt();
				}   //for replications
			}		// for all parameters set
				
			if (!CTRL.m_bKeepTmpFile)
			{
				std::string WGFilePath = GetWGFilePath(outputPath, l, 0, WGr);
				RemoveFile(WGFilePath);
			}
		}	//for all weather replications
	}			//for all locations

	return msg;
}


//CTime CModelExecution::GetLastModificationDate(const std::string& filePath)
//{
//	//CFileStatus status;
//	//CFile::GetStatus(filePath, status);
//
//	return GetFileStamp(filePath);
//}

//ERMsg CModelExecution::TransferOutputToDB(const std::string& filePath, CResult& db, float missingValue)
//{
//	ASSERT( db.IsOpen() );
//	
//    ERMsg msg;
//	
//
//	//Read data and add a new section
//	CModelStatVector section;
//	msg = ReadOutputFile(filePath, section);
//	if( msg )
//	{
//		section.ConvertValue(missingValue, (float)VMISS);
//		db.AddSection(section);
//	}
//
//    return msg;
//}


//ERMsg CModelExecution::ReadOutputFile(const std::string& filePath, CModelStatVector& data, CTM TM, bool bOutputJulianDay, int nbOutputVar)
//{
//	ERMsg msg;	
//    CStdStdioFile file;
//
//	msg = file.Open( convert(filePath).c_str(), CStdFile::modeRead );
//	if ( !msg )
//		return msg;
//	
//
//	//short outputFormat = m_infoFile.GetOutputFormat();
//	int nbReference = TM.GetNbTimeReferences();
//	if( bOutputJulianDay )
//		nbReference--;
//
//	ASSERT( nbReference >= 0 && nbReference <= 4);
//    
//	vector<double> reference;
//	CModelStat outputVar;
//
//	reference.resize(nbReference);
//	outputVar.resize(nbOutputVar);
//
//    CStdString tmp;
//
//	file.ReadString(tmp);
//	tmp.MakeUpper(); 
//	string line = UTF8(tmp);
//	
//	//They have header or not????
//	if( line.find_first_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ") == string::npos)
//		file.SeekToBegin();
//
//    while( file.ReadString(tmp) && msg)
//    {
//		tmp.Trim();
//		line = UTF8(tmp);
//		int realOutputRead = 0;
//
//		if( !line.empty() )
//		{
//			string::size_type pos = 0;
//			for(int i=0; i<nbReference&&pos>=0; i++)
//			{
//				std::string elem = Tokenize(line, "\t ,;", pos);
//				if( !elem.empty() )
//				{
//					reference[i] = ToDouble(elem);
//					realOutputRead++;
//				}
//			}
//
//			for(int i=0; i<nbOutputVar&&pos>=0; i++)
//			{
//				std::string elem = Tokenize(line, "\t ,;", pos);
//				if( !elem.empty() )
//				{
//					double value = ToDouble(elem);
//					//if( value > m_model.GetMissValue() )
//						outputVar[i] = value;
//
//					realOutputRead++;
//				}
//				
//				//if( UtilWin::StringTokenizerReal(line, outputVar[i]) )
//				//	realOutputRead++;
//				//else break;
//			}
//		}
//
//	    if( realOutputRead==0) 
//			break;//end of the file
//
//        if( realOutputRead == nbReference + nbOutputVar)
//		{
//			CTRef TRef = CModel::GetTRef(TM, reference);
//			data.Insert(TRef, outputVar);
//		}
//		else
//	    {
//			msg.ajoute(FormatMsg(IDS_SIM_BAD_OUTPUTFILE_FORMAT, filePath, line));
//	    }
//    }
//
//    file.Close();
//
//	return msg;
//}

//******************************************************************************
//HxGrid general section
//===============================================================
//===============================================================
//***************************************************************
class TSimulationSessionData
{
public:

	TSimulationSessionData()
	{
		 Reset();
	}

	void Reset()
	{
		m_pResult=NULL;
		m_msg = ERMsg::OK;		//for error
		m_nbTaskCompleted=0;//for progression
		m_missingValue=MISSING;
	}

	CResult* m_pResult;
	
	
	ERMsg m_msg;		//for error
	int m_nbTaskCompleted;//for progression
	float m_missingValue;
	CCriticalSection m_CS;
}; 


//the current session
static TSimulationSessionData gSimulationSession;



//===============================================================
//===============================================================
static void __cdecl Finalize(IGenericStream* outStream)
{
	ASSERT(outStream->GetPos()==0);

	
	//ASSERT( pTest->AddRef() == 2);
	try
	{
		
		long simulationNo = 0;
		
		outStream->Read(&simulationNo, sizeof(simulationNo) );
		std::istringstream stream( string((char*)outStream->GetCurPointer(), outStream->GetLength()) );


		//if( success )
		//{
		ASSERT( gSimulationSession.m_pResult );

		if( gSimulationSession.m_pResult )
		{
			CTransferInfoOut infoOut;
			CModelStatVector section;

			CCommunicationStream::ReadOutputStream(stream, infoOut, section);
			gSimulationSession.m_CS.Enter();
			ASSERT(infoOut.m_msg);
			if( infoOut.m_msg )
			{
				section.ConvertValue(gSimulationSession.m_missingValue, (float)VMISS);
				gSimulationSession.m_pResult->SetSection(infoOut.GetSectionNo(), section);
				gSimulationSession.m_nbTaskCompleted++;
			}
			else
			{
				if(gSimulationSession.m_msg)
					gSimulationSession.m_msg = infoOut.m_msg;
			}
			gSimulationSession.m_CS.Leave();
		}
	}
	catch(...)
	{
		ASSERT(false);
		//oups...
	}
	
	
 }


static IGenericStream* CreateStream(long simulationNo, const CTransferInfoIn& info, const CWeatherStation& weather)
{
	//Get info stream in XML
	ostringstream stream;
	CCommunicationStream::WriteInputStream(info, weather, stream);
	string str = stream.str();


	IGenericStream* inStream = new TGenericStream(DWORD(sizeof( simulationNo ) + str.size()));
	
	//IGenericStream* inStream = CreateGenericStream();
	inStream->Write(&simulationNo, sizeof( simulationNo ) );
	inStream->Write(&DIRECT_ACCESS, sizeof(DIRECT_ACCESS));
	inStream->Write(str.c_str(), DWORD(str.size()) );

	return inStream;
}


// To ensure correct resolution of symbols, add Psapi.lib to TARGETLIBS
// and compile with -DPSAPI_VERSION=1
/*
int PrintModules( DWORD processID )
{
	HMODULE hMods[1024]={0};
    HANDLE hProcess=NULL;
    DWORD cbNeeded=0;
    


    // Print the process identifier.
    //printf( "\nProcess ID: %u\n", processID );

    // Get a handle to the process.

    hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
                            PROCESS_VM_READ,
                            FALSE, processID );
    if (NULL == hProcess)
        return 1;

	
   // Get a list of all the modules in this process.

    if( EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
    {
        for ( unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++ )
        {
            TCHAR szModName[MAX_PATH];

            // Get the full path to the module's file.
            if ( GetModuleFileNameEx( hProcess, hMods[i], szModName,sizeof(szModName) / sizeof(TCHAR)))
            {
				std::string filePath = szModName;
				std::string fileName = GetFileName(filePath);

				if( fileName.CompareNoCase("hxGridUserDLL.dll") == 0)
				{
					// Print the module name and handle value.

					//_tprintf( TEXT("\t%s (0x%08X)\n"), szModName, hMods[i] );
					std::string text;
					text.Format( TEXT("\t%s (0x%08X)\n"), szModName, hMods[i]);
				
					int h;
					h=0;
				}
            }
        }
    }
    
    // Release the handle to the process.

    CloseHandle( hProcess );

    return 0;
}

void EnumProc( void )
{

	
	DWORD aProcesses = GetCurrentProcessId();
	PrintModules( aProcesses );
}
*/

static bool IsDLLLoaded(char* DLLname)
{
	bool bRep=false;
	DWORD processID = GetCurrentProcessId();
	HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
                            PROCESS_VM_READ,
                            FALSE, processID );
    if (hProcess != NULL)
	{
	    // Get a list of all the modules in this process.
		HMODULE hMods[1024]={0};
	    DWORD cbNeeded=0;
		if( EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
		{
			for ( unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++ )
			{
				wchar_t szModName[MAX_PATH]={0};

				// Get the full path to the module's file.
				if (GetModuleFileNameExW(hProcess, hMods[i], szModName, sizeof(szModName) / sizeof(wchar_t)))
				{
					std::string filePath = UTF8(szModName);
					std::string fileName = GetFileName(filePath);

					if( boost::iequals(fileName, DLLname) )
					{
						bRep=true;
					}
				}
			}
		}
		
		// Release the handle to the process.
		CloseHandle( hProcess );
	}

	return bRep;
}

ERMsg CModelExecution::RunHxGridSimulation(const CFileManager& fileManager, CModel& model, const CModelInputVector& modelInputVector, const CResult& weather, CResult& result, CCallback& callback)
{
	ERMsg msg;

//	CMultiAppSync appSync;
//	if( !appSync.Enter(mutexName) )
//	{
//		msg.ajoute(GetString(IDS_CMN_HXGRID_ALREADY_USED));
//		return msg;
//	}
//	
//	//load files in memory for stream transfer
//	stringstream staticDataStream;
//	msg = LoadStaticData(fileManager, model, modelInputVector.m_pioneer, staticDataStream);
//	//a faire avec hxgrid
//
//	if(!msg)
//		return msg;
//
//
//
//	const std::string outputPath = GetPath(fileManager);
//	const short language = CRegistry().GetLanguage();
//	
//	//vector<int> locPos;
//	//GetLocationIndexGrid(locArray, locPos);
//
//	long sessionId = Rand();
//
//	CoFreeUnusedLibraries();
//	if( IsDLLLoaded("hxGridUserDLL.dll") )
//		callback.AddMessage("Before run: xhGrid is Loaded");
//
//	FreeGridUserLibrary();
//	
//
//	gSimulationSession.Reset();
//	gSimulationSession.m_pResult = &result;
//	gSimulationSession.m_missingValue = model.GetMissValue();
//	
//	CRandomGenerator rand(m_seedType);
//	unsigned long seed = rand.Rand();
//
//	double timerTG2=0;
//	double timerModel=0;
//	double timerWrite=0;
//
//
//if( DIRECT_ACCESS )
//{
//	std::string filePath = model.GetDLLFilePath();
//	HINSTANCE hDLL = LoadLibraryW( convert(filePath).c_str());
//	ASSERT(hDLL);
//
//			
//	typedef bool (__cdecl *TRunTaskProc)(IAgent* agent, DWORD sessionId, IGenericStream* inStream, IGenericStream* outStream);
//	typedef void (__cdecl *TEndSession)(IAgent* agent, DWORD sessionId);
//	TRunTaskProc RunTask = (TRunTaskProc)GetProcAddress( hDLL, "RunTask" );
//	TEndSession EndSession = (TEndSession)GetProcAddress( hDLL, "EndSession" );
//
//	ASSERT( RunTask );
//	ASSERT( EndSession );
//
//	
//	int loop=0;//to determine if hxGrid is alive
//	// for all loc station in the loc array
//	//static,1 is used to optimize weather CACHE
//	#pragma omp parallel for schedule(static, 1) shared(loop, msg) num_threads(CTRL.m_nbMaxThreads)  if ( model.GetThreadSafe() ) 
//	for(int l=0; l<(int)locations.size(); l++)
//	{
//		int l=locPos[ll];
//		if(msg)
//		{
//		
//			CTimer timer;
//			timer.Start();
//			double start = omp_get_wtime( );
//
//			//// init the loc part of TGInput
//			//// init the loc part of TGInput
//			//CWeatherGenerator WG;
//
//			//
//			//WG.SetSeed(seed);
//			//WG.SetErrorLevel(CTRL.m_errorLevel);
//			//WG.SetNbReplication(m_nbReplications);
//			//WG.SetTGInput(TGInput);
//			//WG.SetNormalDB( normalDB );
//			//WG.SetDailyDB( dailyDB );
//			//WG.SetTarget( locArray[l] );
//
//   //			ERMsg msgTmp = WG.Generate(callback);
//
//			//#pragma omp critical (AddMsg)
//			//{
//			//	#pragma omp flush(msg)
//			//	if( msg && !msgTmp )
//			//	{
//			//		msg += msgTmp;
//			//		#pragma omp flush(msg)
//			//	}
//			//}
//			//
//			//double end = omp_get_wtime( );
//			//timer.Stop();
//
//			/*if( omp_get_thread_num() == 0)
//			{
//				timerTG+=timer;
//				timerTG2 += end-start;
//			}*/
//
//		// for all parameter combination.
//			for(int p=0; p<modelInputVector.size()&&msg; p++)
//			{
//				// for all replication
//				for(short r=0; r<m_nbReplications&&msg; r++)
//				{
//					if(msg)
//					{
//						//std::string outputPath = GetPath(fileManager);
//
//						CTransferInfoIn info = FillTransferInfo( model, WGInput, modelInputVector, locations, l, p, r);
//						
//						CWeather weather;
//						WG.GetWeather(r, weather); 
//						
//						IGenericStream* inStream = CreateStream(sessionId, info, weather);
//						inStream->Seek(0);
//
//						//std::string dllName = "models\\" + model.GetDLLName();
//						
//						DWORD hxGridSessionId=0;
//						
//						//initialize with this parameters set
//				
//						double start = omp_get_wtime( );
//						IGenericStream* outStream = new TGenericStream(0);//CreateGenericStream();
//						if( RunTask(NULL, sessionId, inStream, outStream) )
//						{
//							double startW = omp_get_wtime( );
//							outStream->Seek(0);
//							Finalize(outStream); 
//
//							double endW = omp_get_wtime( );
//							if( omp_get_thread_num() == 0)
//								timerWrite+=endW-startW;
//						}
//
//						double end = omp_get_wtime( );
//						if( omp_get_thread_num() == 0)
//							timerModel+=end-start;
//
//
//						inStream->Release();
//						outStream->Release();
//
//						#pragma omp critical (StepIt)
//						{
//							#pragma omp flush(msg)
//							callback.SetCurrentStepPos( gSimulationSession.m_nbTaskCompleted );
//							if(msg)
//								msg+=callback.StepIt(0);
//							#pragma omp flush(msg)
//						}
//					}
//				}   //for replication
//			}   // if (msg)
//		}		// for(param)
//	}			// for(loc)
//
//	EndSession(NULL, sessionId);
//
//	
//	
//	callback.AddMessage(FormatA("Time TG=%.1g", timerTG2));
//	callback.AddMessage(FormatA("Time Model=%.1g", timerModel));
//	callback.AddMessage(FormatA("Time Write to disk=%.1g", timerWrite));
//
//	BOOL bFree = ::FreeLibrary(hDLL);
//	ASSERT( bFree );
//
//}
//else
//{
//
//
//	IGridUser* pGridUser = CreateGridUserObject(IGridUser::VERSION);
//	if( pGridUser == NULL)
//	{
//		msg.ajoute(GetString(IDS_CMN_UNABLE_CREATE_HXGRID_USER));
//		return msg;
//	}
//
//	DWORD refCount = pGridUser->AddRef();
//	refCount = pGridUser->Release();
//
//
//		
//
//	int loop=0;//to determine if hxGrid is operational
//	// for all loc station in the loc array
//	#pragma omp parallel for shared(msg) num_threads(CTRL.m_nbMaxThreads) if ( model.GetThreadSafe() ) 
//	for(int l=0; l<locArray.size(); l++)
//	{
//		if(msg)
//		{
//			// init the loc part of TGInput
//
//			// for all parameter combination.
//			for(int p=0; p<modelInputArray.size()&&msg; p++)
//			{
//			
//				// for all replication
//				//#pragma omp parallel for //if(m_bUseOpenMP)
//				for(short r=0; r<m_nbReplications&&msg; r++)
//				{
//					//std::string outputPath = GetPath(fileManager);
//
//					CTransferInfoIn info;
//					FillTransferInfo( info, model, WG, TGInput, modelInputArray, locArray, outputPath, l, p, r, m_nbReplications, language);
//
//					CWeather weather;
//					WG.GetWeather(r, weather); 
//
//					//int simulationNo = 12345;
//					IGenericStream* inStream = CreateStream(sessionId, info, weather);
//						
//					//std::string dllName = "models\\" + model.GetDLLName();
//					std::string dllName = model.GetDLLFilePath();
//						
//					DWORD hxGridSessionId=0;
//						 
//					//HRESULT hResult = user->RunTask(dllName,"RunTask",inStream,Finalize,&sessionId,true);
//					#pragma omp critical (RunhxGrid)
//					{
//						while( msg && pGridUser->RunTask(dllName.c_str(),"RunTask",inStream,Finalize,&hxGridSessionId,false) != S_OK )
//						{
//							//we can't add more task to the queue, then we wait
//							callback.SetCurrentStepPos( gSimulationSession.m_nbTaskCompleted );
//							msg+=callback.StepIt(0);
//
//							pGridUser->WaitForCompletionEvent(100);
//							msg+= GetConnectionStatus(pGridUser, loop);
//							
//							if( !gSimulationSession.m_msg )
//							{
//								gSimulationSession.m_CS.Enter();
//								msg+=gSimulationSession.m_msg;
//								gSimulationSession.m_CS.Leave();
//							}
//						}
//
//						callback.SetCurrentStepPos( gSimulationSession.m_nbTaskCompleted );
//						msg+=callback.StepIt(0);
//						#pragma omp flush(msg)
//					}
//				}   //for replication
//			}   // if (msg)
//		}		// for(param)
//	}			// for(loc)
//
//	bool bComplet=false;
//	while( msg && !bComplet )
//	{
//		pGridUser->IsComplete(&bComplet);
//			
//		callback.SetCurrentStepPos( gSimulationSession.m_nbTaskCompleted );
//		msg+=callback.StepIt(0);
//
//		if( !gSimulationSession.m_msg )
//		{
//			gSimulationSession.m_CS.Enter();
//			msg+=gSimulationSession.m_msg;
//			gSimulationSession.m_CS.Leave();
//		}
//
//		Sleep(100);
//		msg+= GetConnectionStatus(pGridUser, loop);
//	}
//
//	//EnumProc( );
//
//	if( !msg )
//		pGridUser->CancelTasks();
//
//	callback.AddMessage(FormatA("Time TG=%.1g", timerTG2));
//
//
//	refCount = pGridUser->AddRef();
//	refCount = pGridUser->Release();
//	callback.AddMessage(std::string("GridUser RefCount=") + ToString((int)refCount));
//
//	pGridUser->Release();
//
//
//	if( !FreeGridUserLibrary() )
//		callback.AddMessage("Unable to unload library hxGridUserDLL.dll");
//
//}
//	/*if( IsDLLLoaded("hxGridUserDLL.dll") )
//	{
//		callback.AddMessage("After run: xhGrid is Loaded");
//		HINSTANCE hInst = GetModuleHandle( "hxGridUserDLL.dll" );
//		if( hInst != NULL )
//		{
//			Sleep(100);
//			CoUninitialize();
//			CoFreeUnusedLibraries();
//			Sleep(100);
//			if( IsDLLLoaded("hxGridUserDLL.dll") )
//			{
//				callback.AddMessage("After CoFreeUnusedLibraries: xhGrid is Loaded");
//			}
//
//			if( !UnmapViewOfFile( hInst ) )
//			{ 
//				std::string error;
//				error.Format( "Couldn't unmap the file! Error Code: %02X\n", GetLastError( ) );
//				callback.AddMessage((LPCTSTR)error);
//			}
//		}
//	}
//	*/
//
//	appSync.Leave();

	return msg;
}




void CModelExecution::writeStruc(zen::XmlElement& output)const
{
	CExecutable::writeStruc(output);
	zen::XmlOut out(output);
	out[GetMemberName(MODEL_NAME)](m_modelName);
	out[GetMemberName(MODEL_INPUT_NAME)](m_modelInputName);
	out[GetMemberName(LOCATIONS_NAME)](m_locationsName);
	out[GetMemberName(PARAM_VARIATIONS_NAME)](m_paramVariationsName);
	out[GetMemberName(SEED_TYPE)](m_seedType);
	out[GetMemberName(NB_REPLICATIONS)](m_nbReplications);
	out[GetMemberName(USE_HXGRID)](m_bUseHxGrid);
}


bool CModelExecution::readStruc(const zen::XmlElement& input)
{
	CExecutable::readStruc(input);
	zen::XmlIn in(input);

	in[GetMemberName(MODEL_NAME)](m_modelName);
	in[GetMemberName(MODEL_INPUT_NAME)](m_modelInputName);
	in[GetMemberName(LOCATIONS_NAME)](m_locationsName);
	in[GetMemberName(PARAM_VARIATIONS_NAME)](m_paramVariationsName);
	in[GetMemberName(SEED_TYPE)](m_seedType);
	in[GetMemberName(NB_REPLICATIONS)](m_nbReplications);
	in[GetMemberName(USE_HXGRID)](m_bUseHxGrid);

	return true;
}


}