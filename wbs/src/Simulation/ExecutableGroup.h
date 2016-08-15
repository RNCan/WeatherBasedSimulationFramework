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
//class CTimer;
namespace WBSF
{

	class CFileManager;
	class CModel;
	class CTGInput;
	class CModelInput;
	class CGeoRect;
	class CTempGen;
	class CNormalsDatabase;
	class CDailyDatabase;
	class CLocationVector;
	

	class CExecutableGroup : public CExecutable
	{
	public:

		using CExecutable::TMember;
		using CExecutable::GetMemberName;

		static const char* GetXMLFlag(){ return XML_FLAG; }
		static CExecutablePtr PASCAL CreateObject(){ return CExecutablePtr(new CExecutableGroup()); }


		CExecutableGroup();
		CExecutableGroup(const CExecutableGroup& in);
		virtual ~CExecutableGroup();

		virtual const char* GetClassName()const{ return XML_FLAG; }
		virtual CExecutablePtr CopyObject()const{ return CExecutablePtr(new CExecutableGroup(*this)); }
		virtual CExecutable& Assign(const CExecutable& in){ ASSERT(in.GetClassName() == XML_FLAG); return operator=(dynamic_cast<const CExecutableGroup&>(in)); }
		virtual bool CompareObject(const CExecutable& in)const{ ASSERT(in.GetClassName() == XML_FLAG); return *this == dynamic_cast<const CExecutableGroup&>(in); }
		virtual void writeStruc(zen::XmlElement& output)const;
		virtual bool readStruc(const zen::XmlElement& input);

		virtual std::string GetPath(const CFileManager& fileManager)const;
		virtual CResultPtr GetResult(const CFileManager& fileManager)const;

		virtual ERMsg ExecuteBasic(const CFileManager& fileManager, CCallback& callback = DEFAULT_CALLBACK);
		virtual int GetNbTask()const{ return 0; }
		virtual int GetNbExecute(bool bTask = false)const;

	protected:

		bool m_bAddPath;
		static const char* XML_FLAG;
		static const int CLASS_NUMBER;

	};

}