//***************************************************************************
// File:        Model.h
//
// class:		CTemporalRef: temporal reference using float format
//				CModel: represent a BioSIM model
//
// Abstract:    Definition for a model. Also run de exe or dll.
//
// Description: 
//
// Note:        
//***************************************************************************
#pragma once

#include "Basic/WeatherDefine.h"
#include "Basic/Registry.h"
#include "Basic/Statistic.h"
#include "ModelBase/ModelInputParameter.h"
#include "ModelBase/ModelOutputVariable.h"

namespace WBSF
{
	class CModelInput;
	class CWGInput;
	class CWCategory;
	class CTRef;



#define TG_NAME "Weather Generation" 


	//*****************************************
	class CModel
	{
	public:


		enum TTransferFileVersion{ VERSION_TEXT, VERSION_XML, VERSION_STREAM, NB_TRANSFER_VERSION };

		enum TTranslatedMember{ NB_TRANSLATED_MEMBER = 6 };
		static const short TRANSLATED_MEMBER[NB_TRANSLATED_MEMBER];


		enum TMember{ TITLE, VERSION, EXTENSION, DLL_NAME, BEHAVIOUR, DESCRIPTION, WINDOW_RECT, SIMULATED_CATEGORY, TRANSFER_FILE_VERSION, IO_FILE_VERSION, NB_YEAR_MIN, NB_YEAR_MAX, THREAD_SAFE, INPUT_VARIABLE, SSI, OUTPUT_TM, OUTPUT_JULIAN_DAY, MISSING_VALUE, OUTPUT_VARIABLE, CREDIT, COPYRIGHT, HELP_TYPE, HELP_FILENAME, HELP_TEXT, NB_MEMBER };
		static const char* GetMemberName(int i){ ASSERT(i >= 0 && i < NB_MEMBER); return MEMBER_NAME[i]; }
		static const char* GetXMLFlag(){ return XML_FLAG; }
		static const char* FILE_EXT;

		enum TBehariour { DETERMINISTIC, STOCHASTIC, NB_BEHAVIOUR };

		enum TType{
			UNKNOWN = CTRef::UNKNOWN,
			ANNUAL = CTRef::ANNUAL,
			MONTHLY = CTRef::MONTHLY,
			DAILY = CTRef::DAILY,
			HOURLY = CTRef::HOURLY,
			ATEMPORAL = CTRef::ATEMPORAL,
			NB_OUTPUT_TYPE
		};

		enum TMode {
			FOR_EACH_YEAR = CTRef::FOR_EACH_YEAR,
			OVERALL_YEARS = CTRef::OVERALL_YEARS,
			NB_MODE
		};

		//public member

		std::string m_title;
		std::string m_DLLName;
		std::string m_version;
		int m_behaviour;
		std::string m_description;

		CWVariables m_variables;
		int m_nbYearMin;
		int m_nbYearMax;
		bool m_bThreadSafe;
		int m_transferFileVersion;

		std::string m_SSI;

		tagRECT m_windowRect;
		CModelInputParameterDefVector m_inputList;
		std::string m_extension;

		CTM m_outputTM;
		CModelOutputVariableDefVector m_outputList;
		double m_missValue;

		//int m_IOFileVersion;

		std::string m_copyright;
		std::string m_credit;
		std::string m_helpFileName;


		CModel();
		CModel(const CModel& in);
		~CModel();

		CModel& operator = (const CModel& in);
		bool operator == (const CModel& in)const;
		bool operator != (const CModel& in)const;

		void Reset();
		ERMsg LoadDLL();
		void UnloadDLL();
		
		ERMsg Save(const std::string& filePath);
		ERMsg Load(const std::string& filePath, short language = CRegistry::ENGLISH);


		void GetDefaultParameter(CModelInput& modelInput)const;
		void SetDefaultParameter(const CModelInput& modelInput);

		ERMsg RunModel(const std::string & nameInputFile);
		ERMsg RunModel(std::istream& inStream, std::iostream& outStream);	// call dll
		ERMsg VerifyModelInput(const CModelInput& modelInput)const;
		ERMsg SetStaticData(std::istream& inStream);

		std::string GetName()const{ return GetFileTitle(m_filePath); }
		std::string GetTitle()const{ return m_title.empty() ? GetName() : m_title; }
		void SetTitle(const std::string& title){ m_title = title; }
		std::string GetExtension()const{ return m_extension; }
		void SetExtension(const std::string& ext);

		std::string GetDLLName()const{ return m_DLLName; }
		void SetDLLName(const std::string& name){ m_DLLName = name; }

		std::string GetVersion()const{ return m_version; }
		void SetVersion(const std::string& version){ m_version = version; }

		int GetBehaviour()const{ return m_behaviour; }
		void SetBehaviour(int in){ m_behaviour = in; }

		std::string GetDescription()const{ return m_description; }
		void SetDescription(const std::string& description){ m_description = description; }

		void SetInputDefinition(const CModelInputParameterDefVector& varList){ m_inputList = varList; }
		CModelInputParameterDefVector GetInputDefinition(bool bRemoveNonVariable = false)const;

		const CModelOutputVariableDefVector& GetOutputDefinition()const{ return m_outputList; }
		void SetOutputDefinition(const CModelOutputVariableDefVector& varList){ m_outputList = varList; }

		const tagRECT& GetRect()const{ return m_windowRect; }
		void SetRect(const tagRECT& rect){ m_windowRect = rect; }

		std::string GetPath()const{ ASSERT(!m_filePath.empty()); return WBSF::GetPath(m_filePath); }
		std::string GetDLLFilePath()const{ return GetDLLFilePath(m_DLLName); }
		std::string GetDLLFilePath(std::string DLLName)const { return GetPath() + DLLName; }

		CWVariables GetVariables()const{ return m_variables; }
		void SetVariables(CWVariables in){ m_variables = in; }

		short GetTransferFileVersion()const{ return m_transferFileVersion; }
		void SetTransferFileVersion(short transferFileVersion){ m_transferFileVersion = transferFileVersion; }

		float GetMissValue()const{ return(float) m_missValue; }
		void SetMissValue(float value){ m_missValue = value; }

		int GetNbYearMin()const{ return m_nbYearMin; }
		void SetNbYearMin(int nbYear){ m_nbYearMin = nbYear; }
		int GetNbYearMax()const{ return m_nbYearMax; }
		void SetNbYearMax(int nbYear){ m_nbYearMax = nbYear; }
		bool GetThreadSafe()const{ return m_bThreadSafe; }
		void SetThreadSafe(bool in){ m_bThreadSafe = in; }

		const std::string& GetHelpFileName()const{ return m_helpFileName; }
		void SetHelpFileName(const std::string& helpFileName){ m_helpFileName = helpFileName; }
		const std::string& GetCopyright()const{ return m_copyright; }
		void SetCopyright(const std::string& copyright){ m_copyright = copyright; }
		const std::string& GetCredit()const{ return m_credit; }
		void SetCredit(const std::string& credit){ m_credit = credit; }

		static CTRef GetTRef(CTM TM, const std::vector<double>& in);
		static short GetFileVersion(const std::string& filePath);
		static std::string GetDLLVersion(const std::string& filePath);

		std::string GetDocumentation()const;
		ERMsg VerifyWGInput(const CWGInput& WGInput)const;
		ERMsg VerifyInputs(StringVector& SSIHeader, CWVariables m_variables)const;

		long GetSimulatedAnnealingSize(){ return m_GetSimulatedAnnealingSize(); }
		ERMsg SetSimulatedAnnealingSize(long size, std::string resultFilePath);
		ERMsg InitSimulatedAnnealing(long ID, const std::string& IDsFilePath);
		double GetFValue(const std::vector<double>& X, CStatisticXYVector& stat);


	protected:


		// run section
		ERMsg RunModelDLL(const std::string & nameInputFile);
		ERMsg RunModelEXE(const std::string & nameInputFile);
		ERMsg FormatModelError(int code);
		bool GetDiskSpaceOk(const std::string & nameInputFile);

		typedef int(*RunModelFileF)(const char*, char[1024]);
		typedef char* (*GetModelVersionF)();

		RunModelFileF m_RunModelFile;
		HINSTANCE m_hDll;

		static const char* MEMBER_NAME[NB_MEMBER];
		static const char* XML_FLAG;

		typedef bool(*RunModelStreamF)(std::istream& inStream, std::ostream& outStream);
		typedef void(*SetStaticDataF)(std::istream& inStream);
		RunModelStreamF m_RunModelStream;
		SetStaticDataF m_SetStaticData;



		//Simulated Annealing
		typedef long(*GetSimulatedAnnealingSizeF)();
		typedef void(*SetSimulatedAnnealingSizeF)(long size, const char* resultFilePath);
		typedef bool(*InitSimulatedAnnealingF)(long ID, const char*, char[1024]);
		typedef double(*GetFValueF)(long size, const double* paramArray, CStatisticXYVector& stat);


		GetSimulatedAnnealingSizeF m_GetSimulatedAnnealingSize;
		SetSimulatedAnnealingSizeF m_SetSimulatedAnnealingSize;
		InitSimulatedAnnealingF m_InitSimulatedAnnealing;
		GetFValueF m_GetFValue;


		std::string m_filePath;
		CCriticalSection m_CS;

	};
}


namespace zen
{


	template <> inline
		void writeStruc(const WBSF::CModel& in, XmlElement& output)
	{
		XmlOut out(output);
		out[WBSF::CModel::GetMemberName(WBSF::CModel::TITLE)](in.m_title);
		out[WBSF::CModel::GetMemberName(WBSF::CModel::VERSION)](in.m_version);
		out[WBSF::CModel::GetMemberName(WBSF::CModel::EXTENSION)](in.m_extension);
		out[WBSF::CModel::GetMemberName(WBSF::CModel::DLL_NAME)](in.m_DLLName);
		out[WBSF::CModel::GetMemberName(WBSF::CModel::BEHAVIOUR)](in.m_behaviour);
		out[WBSF::CModel::GetMemberName(WBSF::CModel::DESCRIPTION)](in.m_description);
		out[WBSF::CModel::GetMemberName(WBSF::CModel::WINDOW_RECT)](in.m_windowRect);
		out[WBSF::CModel::GetMemberName(WBSF::CModel::SIMULATED_CATEGORY)](in.m_variables);
		out[WBSF::CModel::GetMemberName(WBSF::CModel::TRANSFER_FILE_VERSION)](in.m_transferFileVersion);
		out[WBSF::CModel::GetMemberName(WBSF::CModel::SSI)](in.m_SSI);
		out[WBSF::CModel::GetMemberName(WBSF::CModel::NB_YEAR_MIN)](in.m_nbYearMin);
		out[WBSF::CModel::GetMemberName(WBSF::CModel::NB_YEAR_MAX)](in.m_nbYearMax);
		out[WBSF::CModel::GetMemberName(WBSF::CModel::THREAD_SAFE)](in.m_bThreadSafe);
		out[WBSF::CModel::GetMemberName(WBSF::CModel::INPUT_VARIABLE)](in.m_inputList);
		out[WBSF::CModel::GetMemberName(WBSF::CModel::OUTPUT_TM)](in.m_outputTM);
		out[WBSF::CModel::GetMemberName(WBSF::CModel::MISSING_VALUE)](in.m_missValue);
		out[WBSF::CModel::GetMemberName(WBSF::CModel::OUTPUT_VARIABLE)](in.m_outputList);
		out[WBSF::CModel::GetMemberName(WBSF::CModel::COPYRIGHT)](in.m_copyright);
		out[WBSF::CModel::GetMemberName(WBSF::CModel::CREDIT)](in.m_credit);
		out[WBSF::CModel::GetMemberName(WBSF::CModel::HELP_FILENAME)](in.m_helpFileName);
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CModel& out)
	{
		XmlIn in(input);

		in[WBSF::CModel::GetMemberName(WBSF::CModel::TITLE)](out.m_title);
		in[WBSF::CModel::GetMemberName(WBSF::CModel::VERSION)](out.m_version);
		in[WBSF::CModel::GetMemberName(WBSF::CModel::EXTENSION)](out.m_extension);
		in[WBSF::CModel::GetMemberName(WBSF::CModel::DLL_NAME)](out.m_DLLName);
		in[WBSF::CModel::GetMemberName(WBSF::CModel::BEHAVIOUR)](out.m_behaviour);
		in[WBSF::CModel::GetMemberName(WBSF::CModel::DESCRIPTION)](out.m_description);
		in[WBSF::CModel::GetMemberName(WBSF::CModel::WINDOW_RECT)](out.m_windowRect);
		in[WBSF::CModel::GetMemberName(WBSF::CModel::SIMULATED_CATEGORY)](out.m_variables);
		in[WBSF::CModel::GetMemberName(WBSF::CModel::TRANSFER_FILE_VERSION)](out.m_transferFileVersion);
		in[WBSF::CModel::GetMemberName(WBSF::CModel::SSI)](out.m_SSI);
		in[WBSF::CModel::GetMemberName(WBSF::CModel::NB_YEAR_MIN)](out.m_nbYearMin);
		in[WBSF::CModel::GetMemberName(WBSF::CModel::NB_YEAR_MAX)](out.m_nbYearMax);
		in[WBSF::CModel::GetMemberName(WBSF::CModel::THREAD_SAFE)](out.m_bThreadSafe);
		in[WBSF::CModel::GetMemberName(WBSF::CModel::INPUT_VARIABLE)](out.m_inputList);
		in[WBSF::CModel::GetMemberName(WBSF::CModel::OUTPUT_TM)](out.m_outputTM);
		in[WBSF::CModel::GetMemberName(WBSF::CModel::MISSING_VALUE)](out.m_missValue);
		in[WBSF::CModel::GetMemberName(WBSF::CModel::OUTPUT_VARIABLE)](out.m_outputList);
		in[WBSF::CModel::GetMemberName(WBSF::CModel::COPYRIGHT)](out.m_copyright);
		in[WBSF::CModel::GetMemberName(WBSF::CModel::CREDIT)](out.m_credit);
		in[WBSF::CModel::GetMemberName(WBSF::CModel::HELP_FILENAME)](out.m_helpFileName);


		return true;
	}
}
