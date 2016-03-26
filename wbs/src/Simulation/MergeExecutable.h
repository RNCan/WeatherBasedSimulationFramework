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
#include "Simulation/SectionData.h"

namespace WBSF
{
	//class CSimulation;
	class CFileManager;
	class CResult;


	class CMergeExecutable : public CExecutable
	{
	public:

		//friend CMergeExecutableDlg;

		enum TMember{
			APPEND_DIMENSION = CExecutable::NB_MEMBERS, MERGE_ARRAY, NB_MEMBERS,
			NB_MEMBERS_EX = NB_MEMBERS - CExecutable::NB_MEMBERS
		};

		static const char* GetMemberName(int i){ ASSERT(i >= 0 && i < NB_MEMBERS); return (i < CExecutable::NB_MEMBERS) ? CExecutable::GetMemberName(i) : MEMBERS_NAME[i - CExecutable::NB_MEMBERS]; }
		static const char* GetXMLFlag(){ return XML_FLAG; }
		static CExecutablePtr PASCAL CreateObject(){ return CExecutablePtr(new CMergeExecutable()); }

		//*** public member ***
		int m_dimensionAppend;//if append mode, on with dimension
		StringVector m_mergedArray;
		//*********************

		CMergeExecutable();
		CMergeExecutable(const CMergeExecutable& in);
		virtual ~CMergeExecutable();

		virtual const char* GetClassName()const{ return XML_FLAG; }
		virtual CExecutablePtr CopyObject()const{ return CExecutablePtr(new CMergeExecutable(*this)); }
		virtual CExecutable& Assign(const CExecutable& in){ ASSERT(in.GetClassName() == XML_FLAG); return operator=(dynamic_cast<const CMergeExecutable&>(in)); }
		virtual bool CompareObject(const CExecutable& in)const{ ASSERT(in.GetClassName() == XML_FLAG); return *this == dynamic_cast<const CMergeExecutable&>(in); }
		virtual void writeStruc(zen::XmlElement& output)const;
		virtual bool readStruc(const zen::XmlElement& input);

		virtual ERMsg GetParentInfo(const CFileManager& fileManager, CParentInfo& info, CParentInfoFilter filter = CParentInfoFilter())const;


		void Reset();
		CMergeExecutable& operator =(const CMergeExecutable& in);
		bool operator == (const CMergeExecutable& in)const;
		bool operator != (const CMergeExecutable& in)const{ return !operator==(in); }

		//std::string GetMember(int i, LPXNode& pNode=NULL_ROOT)const;
		//void SetMember(int i, const std::string& str, const LPXNode pNode=NULL_ROOT);
		//virtual void GetXML(LPXNode& pRoot)const{XGetXML(*this, pRoot);}
		//virtual void SetXML(const LPXNode pRoot){XSetXML(*this, pRoot);}


		virtual int GetDatabaseType()const{ return CBioSIMDatabase::DATA_STATISTIC; }
		virtual ERMsg Execute(const CFileManager& fileManager, CCallback& callBack = DEFAULT_CALLBACK);
		virtual int GetPriority(){ return 100; }

		const StringVector& GetMergedArray()const{ return m_mergedArray; }
		void SetMergedArray(const StringVector& in){ m_mergedArray = in; }

	protected:

		void ClassReset();
		void GetInputDBInfo(const CResultPtrVector& pResult, CDBMetadata& info);
		ERMsg GetExecutableArray(CExecutablePtrVector& executable)const;
		ERMsg VerifyValidity(const CResultPtrVector& resultArray);
	


		static const char* XML_FLAG;
		static const char* MEMBERS_NAME[NB_MEMBERS_EX];
		static const int CLASS_NUMBER;


	};
	
}