//***************************************************************************
// File:        Simulation.h
//
// class:		CModelExecution
//				
//
// Abstract:    
//
// Description: 
//
// Note:        
//***************************************************************************
#pragma once

#include "Basic/NormalsDatabase.h"
#include "Basic/DailyDatabase.h"
#include "Basic/Timer.h"
#include "ModelBase/CommunicationStream.h"
#include "ModelBase/ModelInput.h"
#include "ModelBase/ParametersVariations.h"
#include "Simulation/Executable.h"


namespace WBSF
{


	class CFileManager;
	class CModel;
	class CModelInput;
	class CGeoRect;



	class CModelExecution : public CExecutable
	{
	public:

		enum TSeedType { RANDOM_FOR_ALL, FIXE_FOR_ALL, RANDOM_BY_REPLICATION, FIXE_BY_REPLICATION, NB_SEEDTYPE};
		enum TMember{
			MODEL_NAME = CExecutable::NB_MEMBERS, MODEL_INPUT_NAME, LOCATIONS_NAME, PARAM_VARIATIONS_NAME, SEED_TYPE, NB_REPLICATIONS, USE_HXGRID,
			NB_MEMBERS, NB_MEMBERS_EX = NB_MEMBERS - CExecutable::NB_MEMBERS
		};


		static const char* GetMemberName(int i){ ASSERT(i >= 0 && i < NB_MEMBERS); return (i < CExecutable::NB_MEMBERS) ? CExecutable::GetMemberName(i) : MEMBERS_NAME[i - CExecutable::NB_MEMBERS]; }
		static const char* GetXMLFlag(){ return XML_FLAG; }
		static CExecutablePtr PASCAL CreateObject(){ return CExecutablePtr(new CModelExecution()); }

		//*** public member ***
		std::string m_modelName;
		std::string m_modelInputName;
		std::string m_locationsName;
		std::string m_paramVariationsName;
		size_t	m_seedType;
		size_t	m_nbReplications;
		bool	m_bUseHxGrid;


		CModelExecution();
		CModelExecution(const CModelExecution& in);
		virtual ~CModelExecution();

		virtual const char* GetClassName()const{ return XML_FLAG; }
		virtual CExecutablePtr CopyObject()const{ return CExecutablePtr(new CModelExecution(*this)); }
		virtual CExecutable& Assign(const CExecutable& in){ ASSERT(in.GetClassName() == XML_FLAG); return operator=(dynamic_cast<const CModelExecution&>(in)); }
		virtual bool CompareObject(const CExecutable& in)const{ ASSERT(in.GetClassName() == XML_FLAG); return *this == dynamic_cast<const CModelExecution&>(in); }
		virtual void writeStruc(zen::XmlElement& output)const;
		virtual bool readStruc(const zen::XmlElement& input);


		void Reset();

		CModelExecution& operator =(const CModelExecution& in);
		bool operator == (const CModelExecution& in)const;
		bool operator != (const CModelExecution& in)const{ return !operator==(in); }

		virtual std::string GetPath(const CFileManager& fileManager)const;
		virtual ERMsg GetParentInfo(const CFileManager& fileManager, CParentInfo& info, CParentInfoFilter filter = CParentInfoFilter())const;
		virtual int GetDatabaseType()const{ return CBioSIMDatabase::DATA_FLOAT; }
		virtual int GetNbTask()const{ return 3; }
		virtual ERMsg Execute(const CFileManager& fileManager, CCallback& callback = DEFAULT_CALLBACK);

		ERMsg GetModelInput(const CFileManager& fileManager, CModelInput& ModelIn)const;
		ERMsg GetParamVariationsDefinetion(const CFileManager& fileManager, CParametersVariationsDefinition& paramVariationDef)const;


		static void InitValidationOptimisation();
		CWGInput GetWGInput(const CFileManager& fileManager)const;
		//static ERMsg LoadStaticData(const CFileManager& fileManager, const CModel& model, const CModelInput& modelInput, std::ostream& stream);


	protected:



		std::string GetWGFilePath(const std::string& path, size_t l, size_t WGp, size_t WGr)const;
		std::string GetModelOutputFilePath(const std::string& path, size_t l, size_t WGp, size_t WGr, size_t p, size_t r)const;

		ERMsg RunHxGridSimulation(const CFileManager& fileManager, CModel& model, const CModelInputVector& modelInputVector, const CResult& weather, CResult& result, CCallback& callback);
		ERMsg RunStreamSimulation(const CFileManager& fileManager, CModel& model, const CModelInputVector& modelInputVector, const CResult& weather, CResult& result, CCallback& callback);
		ERMsg RunFileSimulation(const CFileManager& fileManager, CModel& model, const CModelInputVector& modelInputVector, const CResult& weather, CResult& result, CCallback& callback);

		CTransferInfoIn FillTransferInfo(CModel& model, const CLocationVector& locations, size_t WGReplication, CWGInput WGInput, const CModelInputVector& modelInputVector, size_t seed, size_t l, size_t WGr, size_t p, size_t r);

		static const char* XML_FLAG;
		static const char* MEMBERS_NAME[NB_MEMBERS_EX];
		static const int CLASS_NUMBER;

	};


}