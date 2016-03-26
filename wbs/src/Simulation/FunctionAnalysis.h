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

#include "Simulation/Executable.h"
#include "basic/ERMsg.h"
#include "Basic/UtilTime.h"
#include "Basic/Callback.h"
#include "Simulation/SectionData.h"

class MTParser;
class MTFunctionI;

namespace WBSF
{

	class CSimulation;
	class CFileManager;
	class CResult;
	class CAnalysisDB;
	
	MTFunctionI* CreateGetJDayFct();

	typedef CModelOutputVariableDef CFunctionDef;
	typedef CModelOutputVariableDefVector CFunctionDefArray;

	class CParserVariable
	{
	public:

		CParserVariable()
		{
			m_dim = -1;
			m_field = -1;
			m_value = 0;
		}

		int m_dim;
		int m_field;
		double m_value;
	};

	typedef std::vector<CParserVariable>CParserVariableVector;


	class CFunctionContext
	{
	public:

		CFunctionContext();
		~CFunctionContext();
		ERMsg Compile(const CModelOutputVariableDefVector& varDef, std::string equation);
		double Evaluate(CResultPtr& pResult, size_t s, size_t r);


	protected:

		MTParser* m_pParser;
		CParserVariableVector m_vars;

		static CParserVariable GetParserVariable(const CModelOutputVariableDefVector& varDef, std::string name);
	};

	typedef std::vector<CFunctionContext> CFunctionContextVector;

	//class CFunctionAnalysisDlg;
	class CFunctionAnalysis : public CExecutable
	{
	public:

		//friend CFunctionAnalysisDlg;
		enum TMember{
			FUNCTION_ARRAY = CExecutable::NB_MEMBERS, NB_MEMBER,
			NB_MEMBER_EX = NB_MEMBER - CExecutable::NB_MEMBERS
		};

		static const char* GetMemberName(int i){ ASSERT(i >= 0 && i < NB_MEMBER); return (i < CExecutable::NB_MEMBERS) ? CExecutable::GetMemberName(i) : MEMBER_NAME[i - CExecutable::NB_MEMBERS]; }
		static const char* GetXMLFlag(){ return XML_FLAG; }
		static CExecutablePtr PASCAL CreateObject(){ return CExecutablePtr(new CFunctionAnalysis()); }


		CFunctionAnalysis();
		CFunctionAnalysis(const CFunctionAnalysis& in);
		virtual ~CFunctionAnalysis();

		virtual const char* GetClassName()const{ return XML_FLAG; }
		virtual CExecutablePtr CopyObject()const{ return CExecutablePtr(new CFunctionAnalysis(*this)); }
		virtual CExecutable& Assign(const CExecutable& in){ ASSERT(in.GetClassName() == XML_FLAG); return operator=(dynamic_cast<const CFunctionAnalysis&>(in)); }
		virtual bool CompareObject(const CExecutable& in)const{ ASSERT(in.GetClassName() == XML_FLAG); return *this == dynamic_cast<const CFunctionAnalysis&>(in); }
		virtual void writeStruc(zen::XmlElement& output)const;
		virtual bool readStruc(const zen::XmlElement& input);

		virtual ERMsg GetOutputDefinition(const CFileManager& fileManager, CModelOutputVariableDefVector& outputVar)const;
		void GetOutputDefinition(CModelOutputVariableDefVector& outputVar)const;

		void Reset();
		CFunctionAnalysis& operator =(const CFunctionAnalysis& in);
		bool operator == (const CFunctionAnalysis& in)const;
		bool operator != (const CFunctionAnalysis& in)const{ return !operator==(in); }

		//std::string GetMember(int i, LPXNode& pNode=NULL_ROOT)const;
		//void SetMember(int i, const std::string& str, const LPXNode pNode=NULL_ROOT);
		//virtual void GetXML(LPXNode& pRoot)const{XGetXML(*this, pRoot);}
		//virtual void SetXML(const LPXNode pRoot){XSetXML(*this, pRoot);}


		virtual int GetDatabaseType()const{ return CBioSIMDatabase::DATA_STATISTIC; }
		virtual ERMsg Execute(const CFileManager& fileManager, CCallback& callBack = DEFAULT_CALLBACK);
		virtual ERMsg GetParentInfo(const CFileManager& fileManager, CParentInfo& info, CParentInfoFilter filter = CParentInfoFilter())const;



		CFunctionDefArray m_functionArray;


	protected:

		void ClassReset();
		void GetInputDBInfo(CResultPtr& pResult, CDBMetadata& info);



		static const char* XML_FLAG;
		static const char* MEMBER_NAME[NB_MEMBER_EX];
		static const int CLASS_NUMBER;


	};
}