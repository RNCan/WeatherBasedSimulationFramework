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
	class CFileManager;
	class CCopyExportDlg;

	class CCopyExport : public CExecutable
	{
	public:

		///friend CCopyExportDlg;

		enum TMember{
			OUTPUT_FILE_PATH = CExecutable::NB_MEMBERS, USER_NAME, PASSWORD, NB_MEMBER,
			NB_MEMBER_EX = NB_MEMBER - CExecutable::NB_MEMBERS
		};

		static const char* GetMemberName(int i){ ASSERT(i >= 0 && i < NB_MEMBER); return (i < CExecutable::NB_MEMBERS) ? CExecutable::GetMemberName(i) : MEMBER_NAME[i - CExecutable::NB_MEMBERS]; }
		static const char* GetXMLFlag(){ return XML_FLAG; }
		static CExecutablePtr PASCAL CreateObject(){ return CExecutablePtr(new CCopyExport()); }


		CCopyExport();
		CCopyExport(const CCopyExport& in);
		virtual ~CCopyExport();
		virtual const char* GetClassName()const{ return XML_FLAG; }

		virtual CExecutablePtr CopyObject()const{ return CExecutablePtr(new CCopyExport(*this)); }
		virtual CExecutable& Assign(const CExecutable& in){ ASSERT(in.GetClassName() == XML_FLAG); return operator=(dynamic_cast<const CCopyExport&>(in)); }
		virtual bool CompareObject(const CExecutable& in)const{ ASSERT(in.GetClassName() == XML_FLAG); return *this == dynamic_cast<const CCopyExport&>(in); }


		void Reset();
		CCopyExport& operator =(const CCopyExport& in);
		bool operator == (const CCopyExport& in)const;
		bool operator != (const CCopyExport& in)const{ return !operator==(in); }

		//std::string GetMember(int i, LPXNode& pNode=NULL_ROOT)const;
		//void SetMember(int i, const std::string& str, const LPXNode pNode=NULL_ROOT);
		//virtual void GetXML(LPXNode& pRoot)const{XGetXML(*this, pRoot);}
		//virtual void SetXML(const LPXNode pRoot){XSetXML(*this, pRoot);}

		virtual void writeStruc(zen::XmlElement& output)const;
		virtual bool readStruc(const zen::XmlElement& input);
		virtual ERMsg Execute(const CFileManager& fileManager, CCallback& callback = CCallback::DEFAULT_CALLBACK);
		virtual ERMsg GetLocationList(const CFileManager& fileManager, CLocationVector& loc)const;
		virtual ERMsg GetParameterList(const CFileManager& fileManager, CModelInputVector& parameters)const;
		virtual ERMsg GetReplication(const CFileManager& fileManager, size_t& nbReplication)const;
		virtual ERMsg GetDefaultPeriod(const CFileManager& fileManager, CTPeriod& period)const;
		virtual ERMsg GetOutputDefinition(const CFileManager& fileManager, CModelOutputVariableDefVector& outputVar)const;
		virtual int GetDatabaseType()const{ return CBioSIMDatabase::DATA_STATISTIC; }

		std::string GetServerName(std::string path);
		std::string GetServerPath(std::string path);

		//protected:


		bool IsLocal()const;

		std::string m_outputFilePath;
		std::string m_userName;
		std::string m_password;
		bool m_bShowFTPTransfer;

		static const char* XML_FLAG;
		static const char* MEMBER_NAME[NB_MEMBER_EX];
		static const int CLASS_NUMBER;

	};
	//
	//
	//namespace zen
	//{
	//	
	//
	//	template <> inline
	//	void writeStruc(const CCopyExport& in, XmlElement& output)
	//	{
	//		writeStruc( (const CExecutable&)in, output );
	//		
	//		XmlOut out(output);
	//		out[CCopyExport::GetMemberName(CCopyExport::OUTPUT_FILE_PATH)](in.m_outputFilePath);
	//		out[CCopyExport::GetMemberName(CCopyExport::USER_NAME)](in.m_userName);
	//		out[CCopyExport::GetMemberName(CCopyExport::PASSWORD)](in.m_password);
	//	}
	//
	//	template <> inline
	//	bool readStruc(const XmlElement& input, CCopyExport& out)
	//	{
	//		XmlIn in(input);
	//		
	//		readStruc( input, (CExecutable&)out );
	//
	//		in[CCopyExport::GetMemberName(CCopyExport::OUTPUT_FILE_PATH)](out.m_outputFilePath);
	//		in[CCopyExport::GetMemberName(CCopyExport::USER_NAME)](out.m_userName);
	//		in[CCopyExport::GetMemberName(CCopyExport::PASSWORD)](out.m_password);
	//
	//		return true;
	//	}
	//}
}