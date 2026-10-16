//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
#pragma once


#include "basic/ERMsg.h"
#include "Basic/SimulatedAnnealing.h"
#include "ModelBase/Model.h"
#include "ModelBase/ParametersVariations.h"
#include "Simulation/Executable.h"


namespace WBSF
{


	class CWeatherGenerator;

	//*******************************************************************************
	//CModelParameterization

	class CModelParameterization : public CExecutable
	{
	public:

		static const char* DATA_DESCRIPTOR;
		enum TFeedback { LOOP, ITERATION, CYCLE };
		
		enum TMember{
			USE_HX_GRID = CExecutable::NB_MEMBERS, MODEL_NAME, RESULT_FILE_NAME, LOCID_FIELD, MODEL_INPUT_NAME, MODEL_INPUT_OPT_NAME, NB_REPLICATION, PARAMETERS_VARIATIONS_NAME, CONTROL, FEEDBACK_TYPE,
			NB_MEMBERS, NB_MEMBERS_EX = NB_MEMBERS - CExecutable::NB_MEMBERS
		};

		static const char* GetMemberName(int i){ ASSERT(i >= 0 && i < NB_MEMBERS); return (i < CExecutable::NB_MEMBERS) ? CExecutable::GetMemberName(i) : MEMBERS_NAME[i - CExecutable::NB_MEMBERS]; }
		static const char* GetXMLFlag(){ return XML_FLAG; }
		static CExecutablePtr PASCAL CreateObject(){ return CExecutablePtr(new CModelParameterization); }


		bool m_bUseHxGrid;
		std::string m_modelName;
		std::string m_resultFileName;
		size_t m_locIDField;
		std::string m_modelInputName;
		std::string m_modelInputOptName;
		std::string m_parametersVariationsName;
		size_t	m_nbReplications;
		int m_feedbackType;

		CSAControl m_ctrl;



		CModelParameterization();
		CModelParameterization(const CModelParameterization& in);
		virtual ~CModelParameterization();
		virtual const char* GetClassName()const{ return XML_FLAG; }
		virtual CExecutablePtr CopyObject()const{ return CExecutablePtr(new CModelParameterization(*this)); }
		virtual CExecutable& Assign(const CExecutable& in){ ASSERT(in.GetClassName() == XML_FLAG); return operator=(dynamic_cast<const CModelParameterization&>(in)); }
		virtual bool CompareObject(const CExecutable& in)const{ ASSERT(in.GetClassName() == XML_FLAG); return *this == dynamic_cast<const CModelParameterization&>(in); }
		virtual void writeStruc(zen::XmlElement& output)const;
		virtual bool readStruc(const zen::XmlElement& input);
		virtual std::string GetPath(const CFileManager& fileManager)const;

		void Reset();

		CModelParameterization& operator =(const CModelParameterization& in);
		bool operator == (const CModelParameterization& in)const;
		bool operator != (const CModelParameterization& in)const{ return !operator==(in); }

		virtual ERMsg Execute(const CFileManager& fileManager, CCallback& callBack = DEFAULT_CALLBACK);

		const CSAControl& GetControl()const { return m_ctrl; }
		void SetControl(CSAControl& control) { m_ctrl = control; }
		const std::vector<double>& GetOptimalValue() const{ return m_computation.m_opt.m_X; }

		const std::string& GetResultFileName()const{ return m_resultFileName; }
		void SetResultFileName(const std::string& in){ m_resultFileName = in; }
		const std::string& GetModelName()const{ return m_modelName; }
		void SetModelName(const std::string& in){ m_modelName = in; }
		const std::string& GetModelInputName()const{ return m_modelInputName; }
		void SetModelInputName(const std::string& in){ m_modelInputName = in; }
		int GetFeedbackType()const{ return m_feedbackType; }
		void SetFeedbackType(int in){ m_feedbackType = in; }

		bool GetUseHxGrid()const{ return m_bUseHxGrid; }
		void SetUseHxGrid(bool in){ m_bUseHxGrid = in; }



	protected:

		ERMsg LoadModelInput(const CFileManager& fileManager, const CModel& model, const std::string& name, CModelInput& modelInput)const;
		ERMsg SaveModelInput(const CFileManager& fileManager, const CModel& model, const std::string& name, CModelInput& modelInput)const;
		ERMsg LoadParametersVariation(const CFileManager& fileManager, const CModel& model, const std::string& name, CParametersVariationsDefinition& modelInput)const;
		ERMsg Optimize(const CFileManager& fileManager, CResult& result, CCallback& callback);
		ERMsg GetFValue(const std::vector<double>& X, double& F, CStatisticXY& stat, CCallback& callback);

		//size_t GetNbParameter()const{ return m_parameters.size(); }

		ERMsg InitialiseComputationVariable(CComputationVariable& computation, CCallback& callback);

		double Exprep(const double& RDUM);

		//feedBack method
		void OnStartSimulation(CCallback& callback);
		void OnStartLoop(int L, CCallback& callback);
		void OnStartIteration(int L, int I, CCallback& callback);
		void OnStartCycle(int L, int I, int C, CCallback& callback);

		void OnEndSimulation(CCallback& callback);
		void OnEndLoop(int L, CCallback& callback);
		void OnEndIteration(int L, int I, CCallback& callback);
		void OnEndCycle(int L, int I, int C, CCallback& callback);
		void WriteInfo(int L, int I, int C, CCallback& callback);

		//input


		std::string GetModelInputStampFilePath(const CFileManager& fileManager)const;
		std::string GetWeatherPath(const CFileManager& fileManager)const;
		size_t GetLocSize(const CFileManager& fileManager);
		//size_t m_locSize;

		//computation variable
		CComputationVariable m_computation;
		CModelInput m_modelInput;
		CParametersVariationsDefinition m_parametersVariations;
		CSAParameterVector m_parameters;

		CModel m_model;  //Load one model per location

		static void* CreateStream(__int32 sessionID, unsigned __int64 i, const std::vector<double>& X);
		ERMsg CreateGlobalData(const CFileManager& fileManager, void** pGlobalDataStreamIn, CCallback& callback)const;


		//HxGrid
		void* m_pGridUser;
		//Direct access
		typedef bool(__cdecl *TRunSimulatedAnnealingProc)(void* inStream, void* outStream);
		typedef bool(__cdecl *TInitSimulatedAnnealing)(DWORD sessionId, void* inStream);
		HINSTANCE m_hDLL;
		TRunSimulatedAnnealingProc RunSimulatedAnnealing;
		static void __cdecl GetDataCallback(const char* dataDesc, void** stream);


		static const char* XML_FLAG;
		static const char* MEMBERS_NAME[NB_MEMBERS_EX];
		static const int CLASS_NUMBER;
	};


	//*******************************************************************
	//CRandomizeNumber

	class CRandomizeNumber
	{
	public:
		CRandomizeNumber(long IJ = 0, long KL = 0);
		~CRandomizeNumber();

		void Rmarin(long IJ, long KL);
		double Ranmar();
	private:

		long mod(long a, long p){ return a - long(a / p)*p; }

		double m_U[97];
		double m_C;
		double m_CD;
		double m_CM;
		long m_I97;
		long m_J97;

	};

}
