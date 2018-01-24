//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
//
// Class: CBioSIMModelBase: Base of BioSIM model 
//          CParameter: holds parameters info
//          CLocation: holds location info
//          CWeather:   holds climatic variable
//
// Notes: this file implements the global functions
//  int DoSimulation(char * infoFilePath)
//  int DoSimulation2(char * infoFilePath, char errorMessage[1024])
//  these functions must be exported with the .def file 
//
//*********************************************************************
// 01/12/2014	Rémi Saint-Amant	Add hourly and CWeatherStation support
// 05/07/2013	Rémi Saint-Amant	Add SetStaticData
// 03/02/2011   Rémi Saint-Amant	Add New SimulatedAnnealing functionality
// 09/02/2009	Rémi Saint-Amant	Add CModelFactory. An new way to create model
// 03/02/2009	Rémi Saint-Amant	Creation from existing code.
//*********************************************************************

#include "stdafx.h"
#include <map>

#include "basic/ERMsg.h"
#include "basic/hxGrid.h"
#include "ModelBase/EntryPoint.h"
#include "ModelBase/BioSIMModelBase.h"
#include "ModelBase/SimulatedAnnealingVector.h"

using namespace std;

#define DllImport   __declspec( dllimport )
#define DllExport   __declspec( dllexport )

static bool bUseHxGrid = false;
bool GetUseHxGrid(){ return bUseHxGrid; }


namespace WBSF
{
	//data 
	static CStaticDataStream STATIC_DATA_STREAM;
	const CStaticDataStream& GetStaticDataStream(){ return STATIC_DATA_STREAM; }


	CreateObjectM CModelFactory::_CreateObject = NULL;



	extern "C"
	{
		//****************************************************************
		//General function
		bool FormatErrorMessage(ERMsg msg, char errorMessage[1024])
		{
			if (msg)
				return true;

			string tmp;
			for (unsigned int i = 0; i < msg.dimension(); i++)
			{
				tmp += msg[i];
			}

			size_t length = min(tmp.length(), size_t(1023));
			strncpy(errorMessage, tmp.c_str(), length);
			errorMessage[length] = 0;

			return false;
		}

		//****************************************************************
		// Classic BioSIM DoSimulation
		DllExport int RunModelFile(char * infoFilePath, char errorMessage[1024])
		{
			ERMsg msg;

			CBioSIMModelBase* pModel = CModelFactory::CreateObject();
			ASSERT(pModel);

			try
			{
				msg = pModel->Init(infoFilePath);

				if (msg)
				{
					//msg = pModel->OpenOutputFile();

					//if( msg )
					//{
					msg = pModel->Execute();
					if (msg&&pModel->HaveOutput())
						pModel->SaveOutputFile();

					//pModel->CloseOutputFile();
					//}
				}
			}
			catch (ERMsg e)
			{
				msg = e;
			}
			catch (...)
			{
				msg = GetErrorMessage(0);
			}


			delete pModel; pModel = NULL;
			return FormatErrorMessage(msg, errorMessage);
		}


		DllExport const char* GetModelVersion()
		{
			return CBioSIMModelBase::GetVersion();
		}

		//****************************************************************
		// new BioSIM stream function


		bool RunModel(IAgent* agent, DWORD hxGridSessionId, istream& inStream, ostream& outStream)
		{
			ERMsg msg;

			CBioSIMModelBase* pModel = CModelFactory::CreateObject();
			_ASSERTE(pModel);

			pModel->m_pAgent = agent;
			pModel->m_hxGridSessionID = hxGridSessionId;

			try
			{
				msg = pModel->Init(inStream);

				if (msg)
					msg = pModel->Execute();
			}
			catch (ERMsg e)
			{
				msg = e;
			}
			catch (...)
			{
				msg = GetErrorMessage(0);
			}

			//Get output stream when ever they are error or not
			pModel->GetOutStream(msg, outStream);

			delete pModel; pModel = NULL;



			return msg ? true : false;
		}

		DllExport bool RunModelStream(istream& inStream, ostream& outStream)
		{
			return RunModel(NULL, 0, inStream, outStream);
		}

		DllExport void SetStaticData(std::istream& stream)
		{
			STATIC_DATA_STREAM.ReadStream(stream);
		}

	}



	//****************************************************************
	//BioSIM HxGrid utility function



	class TSessionData
	{
	public:

		TSessionData()
		{}

		~TSessionData()
		{}

		CSimulatedAnnealingVector m_modelVector;
	};


	typedef std::map<unsigned __int64, TSessionData> CSessionDataCache;
	static CCriticalSection CS;

	static CSessionDataCache sessionDataCache;
	static const char* SIMULATED_ANNEALING_DATA_DESCRIPTOR = "ParameterizationData";

	const CSimulatedAnnealingVector& GetSimulatedAnnealingVector()
	{
		ASSERT(sessionDataCache.size() == 1);

		return sessionDataCache.begin()->second.m_modelVector;
	}


	ERMsg BeginSession(IAgent* agent, __int32 sessionId, DWORD hxGridSessionId, IGenericStream* globalDataStream = NULL)
	{
		ERMsg msg;
		//EnterCriticalSection(&sessionDataCache.m_CS);
		CS.Enter();

		CSessionDataCache::iterator i = sessionDataCache.find(sessionId);

		if (i == sessionDataCache.end())
		{
			//----------- read global data -------------
			//CStdString tmp;
			//tmp.Format("Before init\nsessionId = %d\nhxGridSessionId = %d",sessionId, hxGridSessionId);
			//MessageBox(NULL, (LPCTSTR)tmp, "BeginSession", MB_OK);   
			if (globalDataStream == NULL)
			{
				ASSERT(agent);
				HRESULT rz = agent->GetData(hxGridSessionId, SIMULATED_ANNEALING_DATA_DESCRIPTOR, &globalDataStream);

				if (rz != S_OK)
				{
					CS.Leave();

					msg.ajoute(string("ERROR: agent->GetData from ") + SIMULATED_ANNEALING_DATA_DESCRIPTOR + " failed");
					return msg;
				}
			}

			//insert a new session
			sessionDataCache.insert(std::make_pair(sessionId, TSessionData()));
			i = sessionDataCache.find(sessionId);
			ASSERT(i != sessionDataCache.end());


			TSessionData& sd = i->second;

			istringstream iStream(string((char*)globalDataStream->GetBasePointer(), globalDataStream->GetLength()));
			msg = sd.m_modelVector.ReadStream(iStream);
			if (!msg)
				return msg;

			//free global stream. No longer need for this session
			if (agent)
			{
				//release global stream 
				globalDataStream->Release();
				agent->FreeCachedData(hxGridSessionId, SIMULATED_ANNEALING_DATA_DESCRIPTOR);

				//set agent on model
				for (CSimulatedAnnealingVector::iterator it = sd.m_modelVector.begin(); it != sd.m_modelVector.end(); it++)
				{
					(*it)->m_pAgent = agent;
					(*it)->m_hxGridSessionID = hxGridSessionId;
				}
			}
		}

		CS.Leave();

		return msg;
	}

	extern "C"
	{


		//================================================================
		//================================================================
		DllExport void __cdecl EndSession(IAgent* agent, __int32 sessionId)
		{
			CSessionDataCache::iterator i = sessionDataCache.find(sessionId);

			if (i != sessionDataCache.end())
			{
				sessionDataCache.erase(i);
			}

			CoFreeUnusedLibrariesEx(0, 0);
		}




		//****************************************************************
		//BioSIM HxGrid function for running model

		DllExport  bool __cdecl RunTask(IAgent* agent,
			DWORD hxGridSessionId,
			IGenericStream* inStream,
			IGenericStream* outStream)
		{

			bool bRep = true;

			bUseHxGrid = true;
			__int32 sessionId = 0;
			inStream->Read(&sessionId, sizeof(sessionId));
			__int8 bDirectAccess = false;
			inStream->Read(&bDirectAccess, sizeof(bDirectAccess));

			istringstream iStream(string((char*)inStream->GetCurPointer(), inStream->GetLength()));
			ostringstream oStream;

			//RunModel(iStream, oStream);
			RunModel(agent, hxGridSessionId, iStream, oStream);

			const string& str = oStream.str();
			outStream->Write(&sessionId, sizeof(sessionId));
			outStream->Write(str.c_str(), (DWORD)str.length());

			if (agent && agent->TestConnection(hxGridSessionId) != S_OK)
				bRep = false;

			return bRep;
		}

		//TSessionData& GetSession(DWORD sessionId)
		//{
		//	//find the session
		//	EnterCriticalSection(&CS);

		//	TSessionDataCache::iterator i = sessionDataCache.find(sessionId);
		//	_ASSERTE(i!=sessionDataCache.end());

		//	TSessionData& sd = i->second;

		//	LeaveCriticalSection(&CS);
		//	
		//	return sd;
		//}

		//****************************************************************
		//BioSIM HxGrid function for simulated ennaling

		DllExport  bool __cdecl InitSimulatedAnnealing(__int32 sessionId, IGenericStream* pStream, char* errorMessage)
		{
			ERMsg msg = BeginSession(NULL, sessionId, -1, pStream);
			return FormatErrorMessage(msg, errorMessage);
		}



		bool __cdecl ExecuteSimulatedAnnealing(
			__int32 sessionId,
			IGenericStream* inStream,
			IGenericStream* outStream)
		{
			CS.Enter();
			CSessionDataCache::iterator i = sessionDataCache.find(sessionId);
			_ASSERTE(i != sessionDataCache.end());
			CS.Leave();

			TSessionData& sd = i->second;

			__int8 bDirectAccess = false;
			inStream->Read(&bDirectAccess, sizeof(bDirectAccess));


			//here we are not suppose to more than obne simulation in the same loc
			unsigned __int64 locNo = 0;
			inStream->Read(&locNo, sizeof(locNo));
			ASSERT(locNo < sd.m_modelVector.size());
			unsigned __int64 XSize = 0;
			inStream->Read(&XSize, sizeof(XSize));
			ASSERT(XSize < (long)100);

			vector<double> paramArray(XSize);
			for (size_t i = 0; i < XSize; i++)
				inStream->Read(&paramArray[i], sizeof(paramArray[i]));

			//LeaveCriticalSection(&CS);

			CStatisticXY stat;
			sd.m_modelVector[locNo]->GetFValue(paramArray, stat);

			//write outputStream
			outStream->Write(&sessionId, sizeof(sessionId));
			outStream->Write(&locNo, sizeof(locNo));
			outStream->Write(&stat, sizeof(stat));


			return true;
		}

		DllExport  bool __cdecl RunSimulatedAnnealingDirect(
			IGenericStream* inStream,
			IGenericStream* outStream)
		{
			__int32 sessionId = 0;
			inStream->Read(&sessionId, sizeof(sessionId));
			return ExecuteSimulatedAnnealing(sessionId, inStream, outStream);
		}

		DllExport  bool __cdecl RunSimulatedAnnealing(IAgent* agent,
			DWORD hxGridSessionId,
			IGenericStream* inStream,
			IGenericStream* outStream)
		{

			bUseHxGrid = true;
			__int32 sessionId = 0;
			inStream->Read(&sessionId, sizeof(sessionId));

			//initialise simulated Annealing 
			if (!BeginSession(agent, sessionId, hxGridSessionId, NULL))
				return false;

			//agent->TestConnection(sessionId)!=S_OK)
			bool bRep = ExecuteSimulatedAnnealing(sessionId, inStream, outStream);
			if (agent && agent->TestConnection(hxGridSessionId) != S_OK)
				bRep = false;

			return bRep;
		}
	}
}



BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
	)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		// Initialize once for each new process.
		// Return FALSE to fail DLL load.
		break;

	case DLL_THREAD_ATTACH:
		// Do thread-specific initialization.
		break;

	case DLL_THREAD_DETACH:
		// Do thread-specific cleanup.
		break;

	case DLL_PROCESS_DETACH:
		// Perform any necessary cleanup.
		break;
	}
	return TRUE;
}

