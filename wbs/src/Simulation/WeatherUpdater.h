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
	class CWeatherUpdate : public CExecutable
	{
	public:

		enum TMember{ SCRIPT_TITLE = CExecutable::NB_MEMBERS, SHOW_APP, NB_MEMBERS, NB_MEMBERS_EX = NB_MEMBERS - CExecutable::NB_MEMBERS };

		static const char* GetMemberName(int i){ ASSERT(i >= 0 && i < NB_MEMBERS); return (i < CExecutable::NB_MEMBERS) ? CExecutable::GetMemberName(i) : MEMBERS_NAME[i - CExecutable::NB_MEMBERS]; }
		static const char* GetXMLFlag(){ return XML_FLAG; }
		static CExecutablePtr PASCAL CreateObject(){ return CExecutablePtr(new CWeatherUpdate()); }

		//public members
		std::string m_fileTitle;
		bool m_bShowApp;

		CWeatherUpdate();
		CWeatherUpdate(const CWeatherUpdate& in);
		virtual ~CWeatherUpdate();

		virtual const char* GetClassName()const{ return XML_FLAG; }
		virtual CExecutablePtr CopyObject()const{ return CExecutablePtr(new CWeatherUpdate(*this)); }
		virtual CExecutable& Assign(const CExecutable& in){ ASSERT(in.GetClassName() == XML_FLAG); return operator=(dynamic_cast<const CWeatherUpdate&>(in)); }
		virtual bool CompareObject(const CExecutable& in)const{ ASSERT(in.GetClassName() == XML_FLAG); return *this == dynamic_cast<const CWeatherUpdate&>(in); }
		virtual void writeStruc(zen::XmlElement& output)const;
		virtual bool readStruc(const zen::XmlElement& input);

		virtual ERMsg GetOutputDefinition(const CFileManager& fileManager, CModelOutputVariableDefVector& outputVar)const;
		void GetOutputDefinition(CModelOutputVariableDefVector& outputVar)const;

		void Reset();
		CWeatherUpdate& operator =(const CWeatherUpdate& in);
		bool operator == (const CWeatherUpdate& in)const;
		bool operator != (const CWeatherUpdate& in)const{ return !operator==(in); }

		virtual int GetDatabaseType()const{ return CBioSIMDatabase::DATA_STATISTIC; }
		virtual ERMsg Execute(const CFileManager& fileManager, CCallback& callBack = DEFAULT_CALLBACK);

		static ERMsg GenerateWUProject(const std::string& project_title, const std::string& server, const std::string& file_path);


	protected:

		void GetInputDBInfo(CResultPtr& pResult, CDBMetadata& info);
		void ClassReset();

		static const char* XML_FLAG;
		static const char* MEMBERS_NAME[NB_MEMBERS_EX];
		static const int CLASS_NUMBER;

	};

}