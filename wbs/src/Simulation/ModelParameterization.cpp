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
#include <math.h>
#include <sstream>

#include "Basic/OpenMP.h"
#include "Basic/UtilMath.h"
#include "Basic/ModelStat.h"
#include "Basic/FileStamp.h"
#include "Basic/UtilStd.h"
#include "Basic/HxGrid.h"
#include "FileManager/FileManager.h"
#include "ModelBase/CommunicationStream.h"
#include "ModelBase/WGInput-ModelInput.h"
#include "Simulation/ExecutableFactory.h"
#include "Simulation/WeatherGenerator.h"
#include "Simulation/LoadStaticData.h"
#include "Simulation/ModelParameterization.h"

#include "WeatherBasedSimulationString.h"


using namespace WBSF::WEATHER;
using namespace std;
using namespace VITALENGINE;



namespace WBSF
{




	static const char mutexName[] = "hxGridMutex";
	static const __int8 DIRECT_ACCESS = 0;
	static long NB_GET_DATA = 0;

	class TSimulatedAnnealingSessionData
	{
	public:

		TSimulatedAnnealingSessionData()
		{
			m_pGlobalDataStream = NULL;
			m_sessionId = 0;
			Reset();
		}

		~TSimulatedAnnealingSessionData()
		{
			ReleaseData();
		}

		void ReleaseData()
		{
			if (m_pGlobalDataStream)
			{
				m_pGlobalDataStream->Release();
				m_pGlobalDataStream = NULL;
			}
		}

		void InitSessionID()
		{
			Randomize();
			m_sessionId = Rand(1, 9999);
		}

		void Reset()
		{
			m_stats.clear();
			m_msg = ERMsg::OK;		//for error
			m_nbTaskCompleted = 0;//for progression
		}

		CStatisticXYVector m_stats;

		ERMsg m_msg;		//for error
		unsigned __int64 m_nbTaskCompleted;//for progression
		__int32 m_sessionId;
		//IGenericStream* m_pGlobalDataStream;//for global input data
		TGenericStream* m_pGlobalDataStream;
	};


	//the current session
	static TSimulatedAnnealingSessionData gSession;

	//*****************************************************************
	//CModelParameterization
	const char* CModelParameterization::DATA_DESCRIPTOR = "ParameterizationData";
	const char* CModelParameterization::XML_FLAG = "Parameterization";
	const char* CModelParameterization::MEMBERS_NAME[NB_MEMBERS_EX] = { "UseHxGrid", "ModelName", "ResultFileName", "LOCIDField", "ModelInputName", "ModelInputOptName", "NbReplications", "ParametersVariationsName", "Control", "FeedbackType" };
	const int CModelParameterization::CLASS_NUMBER = CExecutableFactory::RegisterClass(CModelParameterization::GetXMLFlag(), &CModelParameterization::CreateObject);

	CModelParameterization::CModelParameterization()
	{
		Reset();
	}

	CModelParameterization::~CModelParameterization()
	{}


	CModelParameterization::CModelParameterization(const CModelParameterization& in)
	{
		operator=(in);
	}


	void CModelParameterization::Reset()
	{
		CExecutable::Reset();
		m_name = "Parameterization";

		m_modelName.clear();
		m_resultFileName.clear();
		m_modelInputName = STRDEFAULT;
		m_modelInputOptName.empty();
		m_parametersVariationsName.clear();

		m_nbReplications = 1;
		m_feedbackType = LOOP;

		m_ctrl.Reset();
		m_parameters.clear();


		m_bUseHxGrid = false;
		m_locIDField = 0;

		m_pGridUser = NULL;
		m_hDLL = NULL;
		RunSimulatedAnnealing = NULL;
	}


	CModelParameterization& CModelParameterization::operator =(const CModelParameterization& in)
	{
		if (&in != this)
		{
			CExecutable::operator =(in);

			//reset loc size
			m_bUseHxGrid = in.m_bUseHxGrid;
			m_modelName = in.m_modelName;
			m_resultFileName = in.m_resultFileName;
			m_locIDField = in.m_locIDField;;
			m_modelInputName = in.m_modelInputName;
			m_modelInputOptName = in.m_modelInputOptName;
			m_parametersVariationsName = in.m_parametersVariationsName;
			m_nbReplications = in.m_nbReplications;
			m_feedbackType = in.m_feedbackType;
			//m_parametersVariations = in.m_parametersVariations;

			m_ctrl = in.m_ctrl;
			//m_parameters = in.m_parameters;
			m_parameters.clear();
			m_hDLL = NULL;
			RunSimulatedAnnealing = NULL;
		}

		return *this;
	}

	bool CModelParameterization::operator == (const CModelParameterization& in)const
	{
		bool bEqual = true;

		if (CExecutable::operator!=(in))bEqual = false;
		if (m_bUseHxGrid != in.m_bUseHxGrid) bEqual = false;
		if (m_modelName != in.m_modelName) bEqual = false;
		if (m_resultFileName != in.m_resultFileName) bEqual = false;
		if (m_locIDField != in.m_locIDField) bEqual = false;
		if (m_modelInputName != in.m_modelInputName)bEqual = false;
		if (m_modelInputOptName != in.m_modelInputOptName)bEqual = false;
		if (m_parametersVariationsName != in.m_parametersVariationsName)bEqual = false;
		if (m_nbReplications != in.m_nbReplications)bEqual = false;
		if (m_ctrl != in.m_ctrl)bEqual = false;
		//if (m_parametersVariations != in.m_parametersVariations)bEqual = false;
		if (m_feedbackType != in.m_feedbackType)bEqual = false;


		return bEqual;
	}


	ERMsg CModelParameterization::Execute(const CFileManager& fileManager, CCallback& callback)
	{
		ERMsg msg;

		//randomize for session ID
		gSession.InitSessionID();
		gSession.Reset();


		if (m_ctrl.m_statisticType >= NB_STATXY_TYPE)
		{
			msg.ajoute(string("The statistic ") + CStatistic::GetTitle(m_ctrl.m_statisticType) +" is not supported by model parametrization" );
		}

		//set vMiss value
		m_ctrl.SetVMiss(m_ctrl.AdjustFValue(DBL_MAX));

		if (m_locIDField == UNKNOWN_POS)
			msg.ajoute("Error: The field of the location ID column is not set");


		//load model 
		if (msg)
			msg = fileManager.Model().Get(m_modelName, m_model);

		//load model input to prepare parameters
		if (msg)
			msg = LoadModelInput(fileManager, m_model, m_modelInputName, m_modelInput);

		//load parameters variations
		if (msg)
			msg = LoadParametersVariation(fileManager, m_model, m_parametersVariationsName, m_parametersVariations);

		//Load static data
		if (msg)
		{
			stringstream staticDataStream;
			msg = LoadStaticData(fileManager, m_model, m_modelInput, staticDataStream);
			if (msg)
				msg = m_model.SetStaticData(staticDataStream);

		}

		if (msg)
		{
			if (m_parametersVariations.empty())
			{
				msg.ajoute("Parameters variations file not selected or empty");
				return msg;
			}

			if (m_modelInput.size() != m_parametersVariations.size())
			{
				msg.ajoute("Model input parameters doesn't match the paramters variations. ");
				return msg;
			}

			m_parameters.clear();
			for (size_t i = 0; i < m_parametersVariations.size(); i++)
			{
				if (m_parametersVariations[i].m_bActive)
					m_parameters.push_back(CSAParameter(m_modelInput[i].m_name, m_modelInput[i].GetFloat(), m_parametersVariations[i].GetMin(), m_parametersVariations[i].GetMax()));
			}

			msg = CreateGlobalData(fileManager, (void**)&gSession.m_pGlobalDataStream, callback);
		}

		if (!msg)
			return msg;

		CResult result;



		if (m_bUseHxGrid)
		{
			CMultiAppSync appSync;
			if (!appSync.Enter(mutexName))
			{
				msg.ajoute(GetString(IDS_CMN_HXGRID_ALREADY_USED));
				return msg;
			}

			IGridUser* pGridUser = CreateGridUserObject(IGridUser::VERSION);

			if (pGridUser != NULL)
			{
				BYTE* pBase = gSession.m_pGlobalDataStream->GetBasePointer();
				DWORD length = gSession.m_pGlobalDataStream->GetLength();
				pGridUser->CompressStream(gSession.m_pGlobalDataStream);
				pBase = gSession.m_pGlobalDataStream->GetBasePointer();
				length = gSession.m_pGlobalDataStream->GetLength();


				typedef void(__cdecl *TGetDataCallbackProc)(const char* dataDesc, IGenericStream** stream);
				pGridUser->BindGetDataCallback((TGetDataCallbackProc)GetDataCallback);
				m_pGridUser = pGridUser;
			}
			else
			{
				msg.ajoute(GetString(IDS_CMN_UNABLE_CREATE_HXGRID));
			}
		}
		else
		{
			std::string dllName = "models\\" + m_model.GetDLLName();

			ASSERT(m_hDLL == NULL);
			m_hDLL = LoadLibraryW(convert(dllName).c_str());
			if (m_hDLL != NULL)
			{
				RunSimulatedAnnealing = (TRunSimulatedAnnealingProc)GetProcAddress(m_hDLL, "RunSimulatedAnnealingDirect");
				ASSERT(RunSimulatedAnnealing);

				typedef bool(__cdecl *TInitSimulatedAnnealing)(__int32 sessionId, IGenericStream* inStream, char*);
				TInitSimulatedAnnealing InitSimulatedAnnealing = (TInitSimulatedAnnealing)GetProcAddress(m_hDLL, "InitSimulatedAnnealing");
				ASSERT(InitSimulatedAnnealing);

				char errorMessage[1024] = { 0 };
				if (!InitSimulatedAnnealing(gSession.m_sessionId, gSession.m_pGlobalDataStream, errorMessage))
				{
					msg.ajoute(errorMessage);
				}
			}
			else
			{
				string error = FormatMsg(IDS_BSC_UNABLE_LOADDLL, dllName);
				msg.ajoute(error);
			}
		}

		if (msg)
			msg = Optimize(fileManager, result, callback);

		if (m_bUseHxGrid)
		{
			ASSERT(m_pGridUser);
			IGridUser* pGridUser = (IGridUser*)m_pGridUser;
			if (!msg)
				pGridUser->CancelTasks();

			pGridUser->Release();
			m_pGridUser = NULL;

			gSession.ReleaseData();
		}
		else
		{
			Sleep(1);//to avoid bug with open MP
			FreeLibrary(m_hDLL);
			m_hDLL = NULL;
			RunSimulatedAnnealing = NULL;
		}

		callback.AddMessage(GetCurrentTimeString());
		std::string logText = GetOutputString(msg, callback, true);

		std::string filePath = GetLogFilePath(GetPath(fileManager));
		msg += WriteOutputMessage(filePath, logText);

		
		if (/*msg && */!m_modelInputOptName.empty() && m_tmp.m_Xopt.size() == m_parameters.size())
		{
			CModelInput m_modelInputOpt = m_modelInput;
			
			for (size_t i = 0, j = 0; i < m_parametersVariations.size(); i++)
			{
				if (m_parametersVariations[i].m_bActive)
				{
					m_modelInputOpt[i].SetValue( Round(m_tmp.m_Xopt[j], 4) );
					j++;
				}
			}

			msg += SaveModelInput(fileManager, m_model, m_modelInputOptName, m_modelInputOpt);
		}

		m_model.UnloadDLL();

		return msg;
	}

	std::string CModelParameterization::GetWeatherPath(const CFileManager& fileManager)const
	{
		return GetPath(fileManager) + "Weather\\";
	}

	CTransferInfoIn FillTransferInfo(const CModel& model, const CLocationVector& locations, const CModelInput& modelInput, const CParametersVariationsDefinition& parametersVariations, size_t seed, size_t l)
	{
		ASSERT(modelInput.size() == parametersVariations.size());
		CTransferInfoIn info;



		info.m_transferTypeVersion = model.GetTransferFileVersion();
		info.m_locationsFilePath = locations.GetFilePath();
		info.m_modelName = model.GetName();
		//info.m_modelInputName = modelInput.GetName();

		info.m_locCounter = CCounter(l, locations.size());
		info.m_paramCounter = CCounter(0, 1);
		info.m_repCounter = CCounter(0, 1);

		info.m_loc = locations[l];
		info.m_inputParameters = modelInput.GetParametersVector();
		for (size_t i = 0; i < info.m_inputParameters.size(); i++)
			info.m_inputParameters[i].m_bIsVariable = parametersVariations[i].m_bActive;


		info.m_outputVariables = model.GetOutputDefinition().GetParametersVector();
		info.m_seed = seed;
		info.m_TM = model.m_outputTM;
		info.m_language = CRegistry().GetLanguage();

		return info;
	}

	//Initialise input parameter
	//user can overide theses methods
	ERMsg CModelParameterization::InitialiseComputationVariable(CComputationVariable& computation, CCallback& callback)
	{
		ERMsg msg;

		callback.PushTask("Initialize Computation Variables", 1);
		//callback.SetNbStep(1);


		computation.m_bounds.resize(m_parameters.size());
		computation.m_C.resize(m_parameters.size());
		computation.m_X.resize(m_parameters.size());
		computation.m_XP.resize(m_parameters.size());
		computation.m_XPstat.resize(m_parameters.size());
		computation.m_VM.resize(m_parameters.size());
		computation.m_VMstat.resize(m_parameters.size());


		for (int i = 0; i < m_parameters.size(); i++)
		{
			computation.m_bounds[i] = m_parameters[i].m_bounds;
			computation.m_XP[i] = m_parameters[i].m_initialValue;
			computation.m_C[i] = 2;
			computation.m_VM[i] = computation.m_bounds[i].GetExtent();

			//If the initial value is out of bounds, notify the user and return
			//to the calling routine.
			if (computation.m_bounds[i].IsOutOfBound(computation.m_XP[i]))
			{
				msg.ajoute("The starting value (" + ToString(computation.m_XP[i]) + ") is not inside the bounds [" + ToString(computation.m_bounds[i].GetLowerBound()) + "," + ToString(computation.m_bounds[i].GetUpperBound()) + "].");
				return msg;
			}
		}

		computation.Initialize(m_ctrl.T(), m_ctrl.NEPS(), m_ctrl.GetVMiss());
		//  Evaluate the function with input X and return value as F.
		msg += GetFValue(computation.m_XP, computation.m_FP, computation.m_SP, callback);

		computation.m_NFCNEV++;

		computation.m_X = computation.m_XP;
		computation.m_Xopt = computation.m_XP;

		if (computation.m_FP != m_ctrl.GetVMiss())
		{
			computation.m_S = computation.m_SP;
			computation.m_Sopt = m_tmp.m_SP;
			computation.m_F = computation.m_FP;
			computation.m_Fopt = computation.m_F;
			computation.m_FSTAR[0] = computation.m_FP;
		}


		callback.PopTask();

		return msg;
	}


	double CModelParameterization::Exprep(const double& RDUM)
	{
		//  This function replaces exp to avoid under- and overflows and is
		//  designed for IBM 370 type machines. It may be necessary to modify
		//  it for other machines. Note that the maximum and minimum values of
		//  EXPREP are such that they has no effect on the algorithm.

		double EXPREP = 0;

		if (RDUM > 174.)
		{
			EXPREP = 3.69E+75;
		}
		else if (RDUM < -180.)
		{
			EXPREP = 0.0;
		}
		else
		{
			EXPREP = exp(RDUM);
		}

		return EXPREP;
	}



	void CModelParameterization::OnStartSimulation(CCallback& callback)
	{
		WriteInfo(-1, -1, -1, callback);
	}
	void CModelParameterization::OnStartLoop(int L, CCallback& callback)
	{}
	void CModelParameterization::OnStartIteration(int L, int I, CCallback& callback)
	{}
	void CModelParameterization::OnStartCycle(int L, int I, int C, CCallback& callback)
	{}
	void CModelParameterization::OnEndSimulation(CCallback& callback)
	{
		//update R2 presdicted value with opt parameter
		WriteInfo(-1, -1, -1, callback);
	}

	void CModelParameterization::OnEndLoop(int L, CCallback& callback)
	{
		if (m_feedbackType == LOOP)
			WriteInfo(L, -1, -1, callback);

		//clean VM stats
		for (size_t i = 0; i < m_tmp.m_VMstat.size(); i++)
			m_tmp.m_VMstat[i].Reset();

	}
	void CModelParameterization::OnEndIteration(int L, int I, CCallback& callback)
	{
		if (m_feedbackType == ITERATION)
			WriteInfo(L, I, -1, callback);
	}

	void CModelParameterization::OnEndCycle(int L, int I, int C, CCallback& callback)
	{
		if (m_feedbackType == CYCLE)
			WriteInfo(L, I, C, callback);
	}

	void CModelParameterization::WriteInfo(int L, int I, int C, CCallback& callback)
	{
		string line;

		//if( m_tmp.m_S[NB_VALUE]>0 )
		//{
		//	//line.Format("", );
		//	//callback.AddMessage(line);
		//	line.Format("Report values:\tN=%10d\tT=%9.5f\tNbVal=%6.0lf\tBias=%8.5lf\tMAE=%8.5lf\tRMSE=%8.5lf\tCD=%8.5lf\tR²=%8.5lf", m_tmp.m_NFCNEV, m_tmp.m_T, m_tmp.m_S[NB_VALUE], m_tmp.m_S[BIAS], m_tmp.m_S[MAE], m_tmp.m_S[RMSE], m_tmp.m_S[COEF_D], m_tmp.m_S[STAT_R²] );
		//	callback.AddMessage(line);
		//}
		//else
		//{
		//	callback.AddMessage("Unable to find reasonable value since last report");
		//}


		if (m_tmp.m_Sopt[NB_VALUE] > 0)
		{
			//std::string bestD² = ToString( m_tmp.m_Sopt[COEF_D], 5 );
			//std::string bestR² = ToString( m_tmp.m_Sopt[STAT_R²], 5 );
			double F = m_ctrl.Max() ? m_tmp.m_Fopt : -m_tmp.m_Fopt;

			//line.Format("", F);
			//callback.AddMessage(line);

			line = FormatA("N=%10d\tT=%9.5f\tF=%8.5lf\nNbVal=%6.0lf\tBias=%8.5lf\tMAE=%8.5lf\tRMSE=%8.5lf\tCD=%8.5lf\tR²=%8.5lf", m_tmp.m_NFCNEV, m_tmp.m_T, F, m_tmp.m_Sopt[NB_VALUE], m_tmp.m_Sopt[BIAS], m_tmp.m_Sopt[MAE], m_tmp.m_Sopt[RMSE], m_tmp.m_Sopt[COEF_D], m_tmp.m_Sopt[STAT_R²]);
			callback.AddMessage(line);
		}
		else
		{
			callback.AddMessage("No optimum find yet...");
		}


		line.clear();
		ASSERT(m_tmp.m_Xopt.size() == m_parameters.size());

		bool bShowRange = !m_tmp.m_XPstat.empty() && m_tmp.m_XPstat[0][NB_VALUE] > 0;
		for (size_t i = 0, j = 0; i < m_parameters.size(); i++)
		{
			string name = m_parameters[i].m_name; Trim(name);
			string tmp;

			if (bShowRange)
			{
				tmp = FormatA("% -20.20s\t=%10.5lf {%10.5lf,%10.5lf}\tVM={%10.5lf,%10.5lf}\n", name.c_str(), m_tmp.m_Xopt[j], m_tmp.m_XPstat[j][LOWEST], m_tmp.m_XPstat[j][HIGHEST], m_tmp.m_VMstat[j][LOWEST], m_tmp.m_VMstat[j][HIGHEST]);
			}
			else
			{
				tmp = FormatA("% -20.20s\t=%10.5lf  ", name.c_str(), m_tmp.m_Xopt[j]);
			}

			line += tmp;
			j++;
		}

		callback.AddMessage(line);
		line.clear();


		double eps = fabs(m_tmp.m_Fopt - m_tmp.m_F);
		line = "Eps = [" + ToString(eps, -1) + "]{";

		for (int i = 0; i < m_tmp.m_FSTAR.size(); i++)
		{

			string tmp;
			double eps = fabs(m_tmp.m_F - m_tmp.m_FSTAR[i]);

			if (eps < 10e6)
				tmp = FormatA("%8.5lf", eps);
			else tmp = "-----";


			if (i > 0)
				line += ", ";

			line += tmp;
		}

		line += "}";

		callback.AddMessage(line);

		callback.AddMessage("***********************************");
		callback.StepIt(0);


		//clean X stats
		for (size_t i = 0; i < m_tmp.m_XPstat.size(); i++)
			m_tmp.m_XPstat[i].Reset();
	}

	//devrais faire partie de la class CModel
	ERMsg CModelParameterization::LoadModelInput(const CFileManager& fileManager, const CModel& model, const string& name, CModelInput& modelInput)const
	{
		ERMsg msg;

		if (name.empty() || name == STRDEFAULT)
		{
			model.GetDefaultParameter(modelInput);
		}
		else
		{
			if (msg)
				msg = fileManager.ModelInput(model.GetExtension()).Get(name, modelInput);

			if (msg)
				msg = model.VerifyModelInput(modelInput);
		}

		//verifier ici s'il y a des specialPath dans les types FILE
		modelInput.SpecialPath2FilePath(fileManager.GetAppPath(), fileManager.GetProjectPath());

		return msg;
	}

	ERMsg CModelParameterization::SaveModelInput(const CFileManager& fileManager, const CModel& model, const string& name, CModelInput& modelInput)const
	{
		ERMsg msg;
		
		msg = fileManager.ModelInput(model.GetExtension()).Set(name, modelInput);

		return msg;
	}

	ERMsg CModelParameterization::LoadParametersVariation(const CFileManager& fileManager, const CModel& model, const string& name, CParametersVariationsDefinition& paratersVariations)const
	{
		ERMsg msg;

		if (name.empty() || name == STRDEFAULT)
		{
			msg.ajoute("Un fichier de variation des paramètres doit être définie");
		}
		else
		{
			if (msg)
				msg = fileManager.PVD(model.GetExtension()).Get(name, paratersVariations);

			//if (msg)
				//msg = model.VerifyModelInput(paratersVariations);
		}

		//verifier ici s'il y a des specialPath dans les types FILE
		//paratersVariations.SpecialPath2FilePath(fileManager.GetAppPath(), fileManager.GetProjectPath());

		return msg;
	}



	std::string CModelParameterization::GetPath(const CFileManager& fileManager)const
	{
		if (m_pParent == NULL)
			return fileManager.GetTmpPath() + m_internalName + "\\";

		return m_pParent->GetPath(fileManager) + m_internalName + "\\";
	}


	//**********************************************************************
	//CRandomizeNumber

	CRandomizeNumber::CRandomizeNumber(long IJ, long KL)
	{
		if (IJ < 0)
			IJ = Rand(31328);

		if (KL < 0)
			KL = Rand(30081);


		ASSERT(IJ >= 0 && IJ <= 31328);
		ASSERT(KL >= 0 && KL <= 30081);

		for (int i = 0; i < 97; i++)
			m_U[97] = 0;

		m_C = 0;
		m_CD = 0;
		m_CM = 0;
		m_I97 = 0;
		m_J97 = 0;

		Rmarin(IJ, KL);
	}

	CRandomizeNumber::~CRandomizeNumber()
	{
	}

	void CRandomizeNumber::Rmarin(long IJ, long KL)
	{
		//  This subroutine and the next function generate random numbers. See
		//  the comments for SA for more information. The only changes from the
		//  orginal code is that (1) the test to make sure that RMARIN runs first
		//  was taken out since SA assures that this is done (this test didn't
		//  compile under IBM's VS Fortran) and (2) typing ivec as integer was
		//  taken out since ivec isn't used. With these exceptions, all following
		//  lines are original.

		// This is the initialization routine for the random number generator
		//     RANMAR()
		// NOTE: The seed variables can have values between:    0 <= IJ <= 31328
		//                                                      0 <= KL <= 30081
		_ASSERTE(IJ >= 0 && IJ <= 31328);
		_ASSERTE(KL >= 0 && KL <= 30081);

		long i = mod(IJ / 177, 177) + 2;
		long j = mod(IJ, 177) + 2;
		long k = mod(KL / 169, 178) + 1;
		long l = mod(KL, 169);

		for (int ii = 0; ii < 97; ii++)
		{
			double s = 0.0;
			double t = 0.5;
			for (int jj = 0; jj < 24; jj++)
			{
				long m = mod(mod(i*j, 179)*k, 179);
				i = j;
				j = k;
				k = m;
				l = mod(53 * l + 1, 169);
				if (mod(l*m, 64) >= 32)
				{
					s = s + t;
				}
				t = 0.5 * t;
			}

			m_U[ii] = s;
		}

		m_C = 362436.0 / 16777216.0;
		m_CD = 7654321.0 / 16777216.0;
		m_CM = 16777213.0 / 16777216.0;
		m_I97 = 96;
		m_J97 = 32;
	}

	double CRandomizeNumber::Ranmar()
	{
		double uni = m_U[m_I97] - m_U[m_J97];

		if (uni < 0.0)
			uni += 1.0;

		m_U[m_I97] = uni;
		m_I97--;

		if (m_I97 == -1)
			m_I97 = 96;

		m_J97--;
		if (m_J97 == -1)
			m_J97 = 96;

		m_C -= m_CD;
		if (m_C < 0.0)
			m_C += m_CM;

		uni -= m_C;
		if (uni < 0.0)
			uni += 1.0;

		return uni;
	}


	//******************************************************************************
	//HxGrid general section
	//******************************************************************************
	void __cdecl FinalizeSimulatedAnnealing(IGenericStream* outStream)
	{
		outStream->Seek(0);
		ASSERT(outStream->GetPos() == 0);

		try
		{
			DWORD test;
			outStream->Read(&test, sizeof(test));

			unsigned __int64 locNo = 0;
			outStream->Read(&locNo, sizeof(locNo));
			ASSERT(locNo < (long)gSession.m_stats.size());

			ERMsg msg;
			if (msg)
			{
				int sizeofStat = sizeof(gSession.m_stats[locNo]);
				outStream->Read(&(gSession.m_stats[locNo]), sizeof(gSession.m_stats[locNo]));
				gSession.m_nbTaskCompleted++;


			}
			else
			{
				gSession.m_msg = msg;
			}
		}
		catch (...)
		{
			ASSERT(false);
			//oups...
		}
	}


	ERMsg CModelParameterization::CreateGlobalData(const CFileManager& fileManager, void** ppGlobalDataStream, CCallback& callback)const
	{
		ERMsg msg;

		CRandomGenerator rand;
		unsigned long seed = rand.Rand(1, CRandomGenerator::RAND_MAX_INT);

		//open parent data...
		CResultPtr pWeather = GetParent()->GetResult(fileManager);
		msg = pWeather->Open();
		if (msg)
		{
			const CDBMetadata& metadataIn = pWeather->GetMetadata();
			const CLocationVector& locations = metadataIn.GetLocations();
			//size_t nbReplications = m_nbReplications*metadataIn.GetNbReplications();

			msg = m_model.VerifyInputs(locations.GetSSIHeader(), metadataIn.GetOutputDefinition().GetWVariables());
			if (msg)
			{
				TGenericStream** pGlobalDataStream = (TGenericStream**)ppGlobalDataStream;
				//write model information, weather and model input
				stringstream inStream;
				//write basic info
				__int64 locIDField = m_locIDField;
				__int64 locSize = locations.size();
				write_value(inStream, locIDField);
				write_value(inStream, locSize);
				//inStream.write((char*)(&locIDField), sizeof(locIDField) );
				//inStream.write((char*)(&locSize), sizeof(locSize) );

				callback.PushTask("SaveData", locations.size());
				//callback.SetNbStep(locations.size());

				gSession.m_stats.resize(locations.size());
				for (size_t l = 0; l < locations.size() && msg; l++)
				{
					CSimulationPoint simulationPoint;
					pWeather->GetSection(pWeather->GetSectionNo(l, 0, 0), simulationPoint, m_model.m_variables);

					CTransferInfoIn info = FillTransferInfo(m_model, locations, m_modelInput, m_parametersVariations, seed, l);
					CCommunicationStream::WriteInputStream(info, simulationPoint, inStream);

					msg += callback.StepIt();
				}

				//write result info
				std::string inputFilePath = fileManager.Input().GetFilePath(m_resultFileName);


				ifStream inputFile;
				msg = inputFile.open(inputFilePath, ios::in | ios::binary);//open this file in benary mode to avoid some problem...
				if (msg)
				{
					WriteBuffer(inStream, inputFile.GetText(true));
					inputFile.close();

					string str = inStream.str();
					*pGlobalDataStream = new TGenericStream(10000000);//CreateGenericStream();
					(*pGlobalDataStream)->Write(str.c_str(), (DWORD)str.size());
				}

				callback.PopTask();
			}
		}

		return msg;
	}

	void __cdecl CModelParameterization::GetDataCallback(const char* dataDesc, void** streamIn)
	{
		ASSERT(strcmp(dataDesc, CModelParameterization::DATA_DESCRIPTOR) == 0);
		ASSERT(gSession.m_pGlobalDataStream);

		NB_GET_DATA++;
		IGenericStream** stream = (IGenericStream**)streamIn;
		(*stream) = *stream = new TGenericStream(10000000);//CreateGenericStream();

		BYTE* pBase = gSession.m_pGlobalDataStream->GetBasePointer();
		DWORD length = gSession.m_pGlobalDataStream->GetLength();

		(*stream)->Write(pBase, length);
	}

	void* CModelParameterization::CreateStream(__int32 sessionID, unsigned __int64 i, const vector<double>& X)
	{
		//Get info stream in XML
		IGenericStream* inStream = new TGenericStream(10000000); //CreateGenericStream();


		inStream->Write(&sessionID, sizeof(sessionID));
		inStream->Write(&DIRECT_ACCESS, sizeof(DIRECT_ACCESS));
		inStream->Write(&i, sizeof(i));

		unsigned __int64 size = X.size();
		inStream->Write(&size, sizeof(size));
		for (size_t i = 0; i < size; i++)
		{
			inStream->Write(&X[i], sizeof(X[i]));
		}

		inStream->Seek(0);

		return inStream;
	}

	ERMsg CModelParameterization::GetFValue(const vector<double>& X, double& F, CStatisticXY& stat, CCallback& callback)
	{
		ERMsg msg;

		for (size_t i = 0; i < gSession.m_stats.size(); i++)
			gSession.m_stats[i].clear();


#pragma omp parallel for num_threads(CTRL.m_nbMaxThreads) if(m_model.GetThreadSafe())
		for (__int64 i = 0; i < (__int64)gSession.m_stats.size(); i++)
		{
#pragma omp flush (msg)
			if (msg)
			{
				IGenericStream* inStream = NULL;
				IGenericStream* outStream = NULL;

#pragma omp critical  
				{
					inStream = (IGenericStream*)CreateStream(gSession.m_sessionId, i, X);
					outStream = new TGenericStream(10000000); //CreateGenericStream();
				}

				if (!RunSimulatedAnnealing(inStream, outStream))
				{
					msg.ajoute("Error in model when execute Simulated Annealing");
#pragma omp flush (msg)
				}

#pragma omp flush (msg)
				if (msg)
				{
#pragma omp critical  
					{
						FinalizeSimulatedAnnealing(outStream);
						inStream->Release(); inStream = NULL;
						outStream->Release(); outStream = NULL;
					}

#pragma omp critical(stepIt)
					{
#pragma omp flush(msg)
						if (msg)
							msg += callback.StepIt(0);
#pragma omp flush(msg)
					}
				}

				//wait when pause is activated
				callback.WaitPause();
			}
		}


		if (msg)
		{
			//stat.resize(1);
			//stat[0].Reset();
			stat.Reset();
			F = m_ctrl.GetVMiss();

			for (size_t i = 0; i < gSession.m_stats.size(); i++)
				stat += gSession.m_stats[i];

			//  If the function is to be minimized, switch the sign of the function.
			//  Note that all intermediate and final output switches the sign back
			//  to eliminate any possible confusion for the user.
			if (stat[NB_VALUE] > 0)
				F = m_ctrl.GetFinalFValue(stat);
		}

		return msg;
	}

	//  Version: 3.2
	//  Date: 1/22/94.
	//  Differences compared to Version 2.0:
	//     1. If a trial is out of bounds, a point is randomly selected
	//        from LB(i) to UB(i). Unlike in version 2.0, this trial is
	//        evaluated and is counted in acceptances and rejections.
	//        All corresponding documentation was changed as well.
	//  Differences compared to Version 3.0:
	//     1. If VM(i) > (UB(i) - LB(i)), VM is set to UB(i) - LB(i).
	//        The idea is that if T is high relative to LB & UB, most
	//        points will be accepted, causing VM to rise. But, in this
	//        situation, VM has little meaning; particularly if VM is
	//        larger than the acceptable region. Setting VM to this size
	//        still allows all parts of the allowable region to be selected.
	//  Differences compared to Version 3.1:
	//     1. Test made to see if the initial temperature is positive.
	//     2. WRITE statements prettied up.
	//     3. References to paper updated.
	//
	//  Synopsis:
	//  This routine implements the continuous simulated annealing global
	//  optimization algorithm described in Corana et al.'s article
	//  "Minimizing Multimodal Functions of Continuous Variables with the
	//  "Simulated Annealing" Algorithm" in the September 1987 (vol. 13,
	//  no. 3, pp. 262-280) issue of the ACM Transactions on Mathematical
	//  Software.
	//
	//  A very quick (perhaps too quick) overview of SA:
	//     SA tries to find the global optimum of an N dimensional function.
	//  It moves both up and downhill and as the optimization process
	//  proceeds, it focuses on the most promising area.
	//     To start, it randomly chooses a trial point within the step length
	//  VM (a vector of length N) of the user selected starting point. The
	//  function is evaluated at this trial point and its value is compared
	//  to its value at the initial point.
	//     In a maximization problem, all uphill moves are accepted and the
	//  algorithm continues from that trial point. Downhill moves may be
	//  accepted; the decision is made by the Metropolis criteria. It uses T
	//  (temperature) and the size of the downhill move in a probabilistic
	//  manner. The smaller T and the size of the downhill move are, the more
	//  likely that move will be accepted. If the trial is accepted, the
	//  algorithm moves on from that point. If it is rejected, another point
	//  is chosen instead for a trial evaluation.
	//     Each element of VM periodically adjusted so that half of all
	//  function evaluations in that direction are accepted.
	//     A fall in T is imposed upon the system with the RT variable by
	//  T(i+1) = RT*T(i) where i is the ith iteration. Thus, as T declines,
	//  downhill moves are less likely to be accepted and the percentage of
	//  rejections rise. Given the scheme for the selection for VM, VM falls.
	//  Thus, as T declines, VM falls and SA focuses upon the most promising
	//  area for optimization.
	//
	//  The importance of the parameter T:
	//     The parameter T is crucial in using SA successfully. It influences
	//  VM, the step length over which the algorithm searches for optima. For
	//  a small intial T, the step length may be too small; thus not enough
	//  of the function might be evaluated to find the global optima. The user
	//  should carefully examine VM in the intermediate output (set IPRINT =
	//  1) to make sure that VM is appropriate. The relationship between the
	//  initial temperature and the resulting step length is function
	//  dependent.
	//     To determine the starting temperature that is consistent with
	//  optimizing a function, it is worthwhile to run a trial run first. Set
	//  RT = 1.5 and T = 1.0. With RT > 1.0, the temperature increases and VM
	//  rises as well. Then select the T that produces a large enough VM.
	//
	//  For modifications to the algorithm and many details on its use,
	//  (particularly for econometric applications) see Goffe, Ferrier
	//  and Rogers, "Global Optimization of Statistical Functions with
	//  Simulated Annealing," Journal of Econometrics, vol. 60, no. 1/2, 
	//  Jan./Feb. 1994, pp. 65-100.
	//  For more information, contact 
	//              Bill Goffe
	//              Department of Economics and International Business
	//              University of Southern Mississippi 
	//              Hattiesburg, MS  39506-5072 
	//              (601) 266-4484 (office)
	//              (601) 266-4920 (fax)
	//              bgoffe@whale.st.usm.edu (Internet)
	//
	//  As far as possible, the parameters here have the same name as in
	//  the description of the algorithm on pp. 266-8 of Corana et al.
	//
	//  In this description, SP is single precision, DP is double precision,
	//  INT is integer, L is logical and (N) denotes an array of length n.
	//  Thus, DP(N) denotes a double precision array of length n.
	//
	//  Input Parameters:
	//    Note: The suggested values generally come from Corana et al. To
	//          drastically reduce runtime, see Goffe et al., pp. 90-1 for
	//          suggestions on choosing the appropriate RT and NT.
	//    N - Number of variables in the function to be optimized. (INT)
	//    X - The starting values for the variables of the function to be
	//        optimized. (DP(N))
	//    MAX - Denotes whether the function should be maximized or
	//          minimized. A true value denotes maximization while a false
	//          value denotes minimization. Intermediate output (see IPRINT)
	//          takes this into account. (L)
	//    RT - The temperature reduction factor. The value suggested by
	//         Corana et al. is .85. See Goffe et al. for more advice. (DP)
	//    EPS - Error tolerance for termination. If the final function
	//          values from the last neps temperatures differ from the
	//          corresponding value at the current temperature by less than
	//          EPS and the final function value at the current temperature
	//          differs from the current optimal function value by less than
	//          EPS, execution terminates and IER = 0 is returned. (EP)
	//    NS - Number of cycles. After NS*N function evaluations, each
	//         element of VM is adjusted so that approximately half of
	//         all function evaluations are accepted. The suggested value
	//         is 20. (INT)
	//    NT - Number of iterations before temperature reduction. After
	//         NT*NS*N function evaluations, temperature (T) is changed
	//         by the factor RT. Value suggested by Corana et al. is
	//         MAX(100, 5*N). See Goffe et al. for further advice. (INT)
	//    NEPS - Number of final function values used to decide upon termi-
	//           nation. See EPS. Suggested value is 4. (INT)
	//    MAXEVL - The maximum number of function evaluations. If it is
	//             exceeded, IER = 1. (INT)
	//    LB - The lower bound for the allowable solution variables. (DP(N))
	//    UB - The upper bound for the allowable solution variables. (DP(N))
	//         If the algorithm chooses X(I) .LT. LB(I) or X(I) .GT. UB(I),
	//         I = 1, N, a point is from inside is randomly selected. This
	//         This focuses the algorithm on the region inside UB and LB.
	//         Unless the user wishes to concentrate the search to a par-
	//         ticular region, UB and LB should be set to very large positive
	//         and negative values, respectively. Note that the starting
	//         vector X should be inside this region. Also note that LB and
	//         UB are fixed in position, while VM is centered on the last
	//         accepted trial set of variables that optimizes the function.
	//    C - Vector that controls the step length adjustment. The suggested
	//        value for all elements is 2.0. (DP(N))
	//    IPRINT - controls printing inside SA. (INT)
	//             Values: 0 - Nothing printed.
	//                     1 - Function value for the starting value and
	//                         summary results before each temperature
	//                         reduction. This includes the optimal
	//                         function value found so far, the total
	//                         number of moves (broken up into uphill,
	//                         downhill, accepted and rejected), the
	//                         number of out of bounds trials, the
	//                         number of new optima found at this
	//                         temperature, the current optimal X and
	//                         the step length VM. Note that there are
	//                         N*NS*NT function evalutations before each
	//                         temperature reduction. Finally, notice is
	//                         is also given upon achieveing the termination
	//                         criteria.
	//                     2 - Each new step length (VM), the current optimal
	//                         X (XOPT) and the current trial X (X). This
	//                         gives the user some idea about how far X
	//                         strays from XOPT as well as how VM is adapting
	//                         to the function.
	//                     3 - Each function evaluation, its acceptance or
	//                         rejection and new optima. For many problems,
	//                         this option will likely require a small tree
	//                         if hard copy is used. This option is best
	//                         used to learn about the algorithm. A small
	//                         value for MAXEVL is thus recommended when
	//                         using IPRINT = 3.
	//             Suggested value: 1
	//             Note: For a given value of IPRINT, the lower valued
	//                   options (other than 0) are utilized.
	//    ISEED1 - The first seed for the random number generator RANMAR.
	//             0 <= ISEED1 <= 31328. (INT)
	//    ISEED2 - The second seed for the random number generator RANMAR.
	//             0 <= ISEED2 <= 30081. Different values for ISEED1
	//             and ISEED2 will lead to an entirely different sequence
	//             of trial points and decisions on downhill moves (when
	//             maximizing). See Goffe et al. on how this can be used
	//             to test the results of SA. (INT)
	//
	//  Input/Output Parameters:
	//    T - On input, the initial temperature. See Goffe et al. for advice.
	//        On output, the final temperature. (DP)
	//    VM - The step length vector. On input it should encompass the
	//         region of interest given the starting value X. For point
	//         X(I), the next trial point is selected is from X(I) - VM(I)
	//         to  X(I) + VM(I). Since VM is adjusted so that about half
	//         of all points are accepted, the input value is not very
	//         important (i.e. is the value is off, SA adjusts VM to the
	//         correct value). (DP(N))
	//
	//  Output Parameters:
	//    XOPT - The variables that optimize the function. (DP(N))
	//    FOPT - The optimal value of the function. (DP)
	//    NACC - The number of accepted function evaluations. (INT)
	//    NFCNEV - The total number of function evaluations. In a minor
	//             point, note that the first evaluation is not used in the
	//             core of the algorithm; it simply initializes the
	//             algorithm. (INT).
	//    NOBDS - The total number of trial function evaluations that
	//            would have been out of bounds of LB and UB. Note that
	//            a trial point is randomly selected between LB and UB.
	//            (INT)
	//    IER - The error return number. (INT)
	//          Values: 0 - Normal return; termination criteria achieved.
	//                  1 - Number of function evaluations (NFCNEV) is
	//                      greater than the maximum number (MAXEVL).
	//                  2 - The starting value (X) is not inside the
	//                      bounds (LB and UB).
	//                  3 - The initial temperature is not positive.
	//                  99 - Should not be seen; only used internally.
	//
	//  Work arrays that must be dimensioned in the calling routine:
	//       RWK1 (DP(NEPS))  (FSTAR in SA)
	//       RWK2 (DP(N))     (XP    "  " )
	//       IWK  (INT(N))    (NACP  "  " )
	//
	//  Required Functions (included):
	//    EXPREP - Replaces the function EXP to avoid under- and overflows.
	//             It may have to be modified for non IBM-type main-
	//             frames. (DP)
	//    RMARIN - Initializes the random number generator RANMAR.
	//    RANMAR - The actual random number generator. Note that
	//             RMARIN must run first (SA does this). It produces uniform
	//             random numbers on [0,1]. These routines are from
	//             Usenet's comp.lang.fortran. For a reference, see
	//             "Toward a Universal Random Number Generator"
	//             by George Marsaglia and Arif Zaman, Florida State
	//             University Report: FSU-SCRI-87-50 (1987).
	//             It was later modified by F. James and published in
	//             "A Review of Pseudo-random Number Generators." For
	//             further information, contact stuart@ads.com. These
	//             routines are designed to be portable on any machine
	//             with a 24-bit or more mantissa. I have found it produces
	//             identical results on a IBM 3081 and a Cray Y-MP.
	//
	//  Required Subroutines (included):
	//    PRTVEC - Prints vectors.
	//    PRT1 ... PRT10 - Prints intermediate output.
	//    FCN - Function to be optimized. The form is
	//            SUBROUTINE FCN(N,X,F)
	//            INTEGER N
	//            DOUBLE PRECISION  X(N), F
	//            ...
	//            function code with F = F(X)
	//            ...
	//            RETURN
	//            END
	//          Note: This is the same form used in the multivariable
	//          minimization algorithms in the IMSL edition 10 library.
	//
	//  Machine Specific Features:
	//    1. EXPREP may have to be modified if used on non-IBM type main-
	//       frames. Watch for under- and overflows in EXPREP.
	//    2. Some FORMAT statements use G25.18; this may be excessive for
	//       some machines.
	//    3. RMARIN and RANMAR are designed to be protable; they should not
	//       cause any problems.
	ERMsg CModelParameterization::Optimize(const CFileManager& fileManager, CResult& result, CCallback& callback)
	{
		ERMsg msg;

		NB_GET_DATA = 0;
		//for(int i=0; i<m_nbReplication&&msg; i++)
		//replication are done in model
		{

			//Initialize computable parameter
			if (msg)
				msg = InitialiseComputationVariable(m_tmp, callback);

			if (!msg)
				return msg;

			//  Initialize the random number generator RANMAR.
			CRandomizeNumber random(m_ctrl.Seed1(), m_ctrl.Seed2());

			//feedback
			OnStartSimulation(callback);

			bool bQuit = false;

			//  Start the main loop. Note that it terminates if :
			//(i) the algorithm succesfully optimizes the function 
			//(ii) there are too many function evaluations (more than MAXEVL).
			int L = 0;
			do
			{
				L++;
				OnStartLoop(L, callback);



				callback.PushTask("Search optimum. Loop = " + ToString(L), m_ctrl.NT()*m_ctrl.NS()*m_tmp.m_X.size());
				//callback.SetNbStep(m_ctrl.NT()*m_ctrl.NS()*m_tmp.m_X.size());

				long NUP = 0;
				long NREJ = 0;
				long NNEW = 0;
				long NDOWN = 0;
				long LNOBDS = 0;

				//Do in write info
				//for(size_t i=0; i<m_tmp.m_VMstat.size(); i++)
					//m_tmp.m_VMstat[i].Reset();

				for (int M = 0; M < m_ctrl.NT() && msg; M++)
				{
					OnStartIteration(L, M, callback);

					vector<int> NACP;//(m_tmp.m_X.size());
					NACP.insert(NACP.begin(), m_tmp.m_X.size(), 0);

					for (int J = 0; J < m_ctrl.NS() && msg; J++)
					{
						OnStartCycle(L, M, J, callback);


						if ((m_feedbackType == LOOP && M == 0 && J == 0) ||
							(m_feedbackType == ITERATION && J == 0) ||
							(m_feedbackType == CYCLE))
							callback.AddMessage("Loop = " + ToString(L) + ", Iteration = " + ToString(M + 1) + ", Cycle = " + ToString(J + 1));


						for (int H = 0; H < NACP.size() && msg; H++)
						{
							//  If too many function evaluations occur, terminate the algorithm.
							if (m_tmp.m_NFCNEV >= m_ctrl.MAXEVL())
							{
								OnEndSimulation(callback);
								msg.ajoute("Number of function evaluations (NFCNEV) is greater than the maximum number (MAXEVL).");
								return msg;
							}

							m_tmp.m_XP.resize(m_tmp.m_X.size());


							do
							{
								//  Generate XP, the trial value of X. Note use of VM to choose XP.
								for (int I = 0; I < m_tmp.m_X.size(); I++)
								{
									if (I == H)
										m_tmp.m_XP[I] = m_tmp.m_X[I] + (random.Ranmar()*2.0 - 1.0) * m_tmp.m_VM[I];
									else m_tmp.m_XP[I] = m_tmp.m_X[I];


									//  If XP is out of bounds, select a point in bounds for the trial.
									if (m_tmp.m_bounds[I].IsOutOfBound(m_tmp.m_XP[I]))
									{
										m_tmp.m_XP[I] = m_tmp.m_bounds[I].GetLowerBound() + m_tmp.m_bounds[I].GetExtent()*random.Ranmar();

										LNOBDS++;
										m_tmp.m_NOBDS++;
									}
								}

								//  Evaluate the function with the trial point XP and return as FP.
								msg += GetFValue(m_tmp.m_XP, m_tmp.m_FP, m_tmp.m_SP, callback);
							} while (!m_tmp.m_SP.GetX().IsInit());//if no stat (bad parameters), find new vlaus

							//add X value to extreme XP statistic
							for (int i = 0; i < m_tmp.m_XP.size() && i < (int)m_tmp.m_XPstat.size(); i++)
								m_tmp.m_XPstat[i] += m_tmp.m_XP[i];

							for (size_t i = 0; i < m_tmp.m_VMstat.size(); i++)
								m_tmp.m_VMstat[i] += m_tmp.m_VM[i];

							

							//  Accept the new point if the function value increases.
							if (m_tmp.m_FP != m_ctrl.GetVMiss())
							{
								if (m_tmp.m_FP >= m_tmp.m_F)
								{
									m_tmp.m_X = m_tmp.m_XP;
									m_tmp.m_F = m_tmp.m_FP;
									m_tmp.m_S = m_tmp.m_SP;
									m_tmp.m_NACC++;
									NACP[H]++;
									NUP++;

									//  If greater than any other point, record as new optimum.
									if (m_tmp.m_FP > m_tmp.m_Fopt)
									{
										m_tmp.m_Xopt = m_tmp.m_XP;
										m_tmp.m_Fopt = m_tmp.m_FP;
										m_tmp.m_Sopt = m_tmp.m_SP;
										NNEW++;
									}
								}
								//  If the point is lower, use the Metropolis criteria to decide on
								//  acceptance or rejection.
								else
								{
									double P = Exprep((m_tmp.m_FP - m_tmp.m_F) / m_tmp.m_T);
									double PP = random.Ranmar();
									if (PP < P)
									{
										m_tmp.m_X = m_tmp.m_XP;
										m_tmp.m_F = m_tmp.m_FP;
										m_tmp.m_S = m_tmp.m_SP;
										m_tmp.m_NACC++;
										NACP[H]++;
										NDOWN++;
									}
									else
									{
										NREJ = NREJ + 1;
									}
								} //if
							}
							else
							{
								NREJ = NREJ + 1;
								//Eliminate this evaluation??
								H--;
							}

							m_tmp.m_NFCNEV++;


							if (msg)
								msg += callback.StepIt();
						} //H

						OnEndCycle(L, M, J, callback);
					} //J

					//  Adjust VM so that approximately half of all evaluations are accepted.
					ASSERT(m_tmp.m_VM.size() == NACP.size());
					for (int I = 0; I < m_tmp.m_VM.size(); I++)
					{
						double RATIO = double(NACP[I]) / double(m_ctrl.NS());
						if (RATIO > 0.6)
						{
							m_tmp.m_VM[I] = m_tmp.m_VM[I] * (1. + m_tmp.m_C[I] * (RATIO - .6) / .4);
						}
						else if (RATIO < 0.4)
						{
							m_tmp.m_VM[I] = m_tmp.m_VM[I] / (1. + m_tmp.m_C[I] * ((.4 - RATIO) / .4));
						}


						if (m_tmp.m_VM[I] > m_tmp.m_bounds[I].GetExtent())
						{
							m_tmp.m_VM[I] = m_tmp.m_bounds[I].GetExtent();
						}

						//m_tmp.m_VMstat[I] += m_tmp.m_VM[I];

					}//all VM

					OnEndIteration(L, M, callback);
				}//M

				OnEndLoop(L, callback);
				//  Loop again.


				bQuit = fabs(m_tmp.m_F - m_tmp.m_Fopt) <= m_ctrl.EPS();
				for (int I = 0; I < m_tmp.m_FSTAR.size() && bQuit; I++)
				{
					if (fabs(m_tmp.m_F - m_tmp.m_FSTAR[I]) > m_ctrl.EPS())
						bQuit = false;
				}

				//  If termination criteria is not met, prepare for another loop.
				m_tmp.PrepareForAnotherLoop(m_ctrl.RT());

				callback.PopTask();
			} while (!bQuit&&msg);


			OnEndSimulation(callback);
		}

		return msg;
	}



	void CModelParameterization::writeStruc(zen::XmlElement& output)const
	{
		CExecutable::writeStruc(output);
		zen::XmlOut out(output);
		out[GetMemberName(USE_HX_GRID)](m_bUseHxGrid);
		out[GetMemberName(MODEL_NAME)](m_modelName);
		out[GetMemberName(RESULT_FILE_NAME)](m_resultFileName);
		out[GetMemberName(LOCID_FIELD)](m_locIDField);
		out[GetMemberName(MODEL_INPUT_NAME)](m_modelInputName);
		out[GetMemberName(MODEL_INPUT_OPT_NAME)](m_modelInputOptName);
		out[GetMemberName(PARAMETERS_VARIATIONS_NAME)](m_parametersVariationsName);
		out[GetMemberName(NB_REPLICATION)](m_nbReplications);
		out[GetMemberName(FEEDBACK_TYPE)](m_feedbackType);
		out[GetMemberName(CONTROL)](m_ctrl);
	}

	bool CModelParameterization::readStruc(const zen::XmlElement& input)
	{
		CExecutable::readStruc(input);
		zen::XmlIn in(input);
		in[GetMemberName(USE_HX_GRID)](m_bUseHxGrid);
		in[GetMemberName(MODEL_NAME)](m_modelName);
		in[GetMemberName(RESULT_FILE_NAME)](m_resultFileName);
		in[GetMemberName(LOCID_FIELD)](m_locIDField);
		in[GetMemberName(MODEL_INPUT_NAME)](m_modelInputName);
		in[GetMemberName(MODEL_INPUT_OPT_NAME)](m_modelInputOptName);
		in[GetMemberName(PARAMETERS_VARIATIONS_NAME)](m_parametersVariationsName);
		in[GetMemberName(NB_REPLICATION)](m_nbReplications);
		in[GetMemberName(FEEDBACK_TYPE)](m_feedbackType);
		in[GetMemberName(CONTROL)](m_ctrl);

		return true;
	}
}