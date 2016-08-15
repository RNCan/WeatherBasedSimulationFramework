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

#include "Simulation/ExecutableGroup.h"
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
	



	class CBioSIMProject : public CExecutableGroup
	{
	public:

		static const char* GetXMLFlag(){ return XML_FLAG; }


		CBioSIMProject();
		CBioSIMProject(const CBioSIMProject& in);
		~CBioSIMProject();


		void Reset();
		CBioSIMProject& operator =(const CBioSIMProject& in);
		bool operator == (const CBioSIMProject& in)const;
		bool operator != (const CBioSIMProject& in)const{ return !operator==(in); }

		ERMsg Load(const std::string& filePath);
		ERMsg Save(const std::string& filePath);

		ERMsg Execute(const CFileManager& fileManager, CCallback& callback = DEFAULT_CALLBACK);
		CExecutablePtr FindItem(const std::string& iName);
		std::string GetFilePath()const{ return m_filePath; }


		void SetMyself(CExecutablePtr* pMyself) { m_pMyself = pMyself; }

		virtual CExecutablePtr GetExecutablePtr(){ ASSERT(m_pMyself); return *m_pMyself; }
		virtual const CExecutablePtr GetExecutablePtr()const{ ASSERT(m_pMyself); return *m_pMyself; }

	protected:

		std::string m_filePath;

		CExecutablePtr* m_pMyself;//project executeble: to be replaced by weakPtr
		static const char* XML_FLAG;
	};


}