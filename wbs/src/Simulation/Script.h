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

namespace WBSF
{

	//*******************************************************************************
	//
	class CScript : public CExecutable
	{
	public:

		enum TMember{ SCRIPT_NAME = CExecutable::NB_MEMBERS, INPUT, OUTPUT, ARGUMENTS, SHOW_PROGRESS, NB_MEMBERS, NB_MEMBERS_EX = NB_MEMBERS - CExecutable::NB_MEMBERS };

		static const char* GetMemberName(int i){ ASSERT(i >= 0 && i < NB_MEMBERS); return (i < CExecutable::NB_MEMBERS) ? CExecutable::GetMemberName(i) : MEMBERS_NAME[i - CExecutable::NB_MEMBERS]; }
		static const char* GetXMLFlag(){ return XML_FLAG; }
		static CExecutablePtr PASCAL CreateObject(){ return CExecutablePtr(new CScript()); }

		//public members
		std::string m_scriptFileName;
		std::string m_inputFileName;
		std::string m_outputFileName;
		std::string m_arguments;
		bool m_bShowProgress;

		CScript();
		CScript(const CScript& in);
		virtual ~CScript();

		virtual const char* GetClassName()const{ return XML_FLAG; }
		virtual CExecutablePtr CopyObject()const{ return CExecutablePtr(new CScript(*this)); }
		virtual CExecutable& Assign(const CExecutable& in){ ASSERT(in.GetClassName() == XML_FLAG); return operator=(dynamic_cast<const CScript&>(in)); }
		virtual bool CompareObject(const CExecutable& in)const{ ASSERT(in.GetClassName() == XML_FLAG); return *this == dynamic_cast<const CScript&>(in); }
		virtual void writeStruc(zen::XmlElement& output)const;
		virtual bool readStruc(const zen::XmlElement& input);

		virtual ERMsg GetOutputDefinition(const CFileManager& fileManager, CModelOutputVariableDefVector& outputVar)const;
		void GetOutputDefinition(CModelOutputVariableDefVector& outputVar)const;

		void Reset();
		CScript& operator =(const CScript& in);
		bool operator == (const CScript& in)const;
		bool operator != (const CScript& in)const{ return !operator==(in); }

		virtual int GetDatabaseType()const{ return CBioSIMDatabase::DATA_STATISTIC; }
		virtual ERMsg Execute(const CFileManager& fileManager, CCallback& callBack = DEFAULT_CALLBACK);



	protected:

		void GetInputDBInfo(CResultPtr& pResult, CDBMetadata& info);
		void ClassReset();

		static const char* XML_FLAG;
		static const char* MEMBERS_NAME[NB_MEMBERS_EX];
		static const int CLASS_NUMBER;

	};

}