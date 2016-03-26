//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//******************************************************************************
#include "stdafx.h"

#include "FileManager/FileManager.h"
#include "Simulation/ExecutableGroup.h"
#include "Simulation/ExecutableFactory.h"


using namespace std;


namespace WBSF
{
	//****************************************************************
	//CExecutableGroup
	const char* CExecutableGroup::XML_FLAG = "Group";
	const int CExecutableGroup::CLASS_NUMBER = CExecutableFactory::RegisterClass(CExecutableGroup::GetXMLFlag(), &CExecutableGroup::CreateObject);


	CExecutableGroup::CExecutableGroup()
	{
		m_name = "Group";
		m_bAddPath = true;
	}

	CExecutableGroup::CExecutableGroup(const CExecutableGroup& in)
	{
		operator=(in);
	}

	CExecutableGroup::~CExecutableGroup()
	{}

	string CExecutableGroup::GetPath(const CFileManager& fileManager)const
	{
		string internalName = m_bAddPath ? (m_internalName + "\\") : "";

		if (m_pParent == NULL)
			return fileManager.GetTmpPath() + internalName;

		return m_pParent->GetPath(fileManager) + internalName;
	}

	ERMsg CExecutableGroup::ExecuteBasic(const CFileManager& fileManager, CCallback& callback)
	{
		ERMsg msg;
		//Execution of group do nothing
		return msg;
	}

	//Because we don't count group,
	//we have to override GetNbExecute
	int CExecutableGroup::GetNbExecute(bool bTask)const
	{
		return m_executables.GetNbExecute(bTask);
	}

	CResultPtr CExecutableGroup::GetResult(const CFileManager& fileManager)const
	{
		if (!GetParent().get())
			return CExecutable::GetResult(fileManager);

		return GetParent()->GetResult(fileManager);
	}


	void CExecutableGroup::writeStruc(zen::XmlElement& output)const
	{
		CExecutable::writeStruc(output);
	}

	bool CExecutableGroup::readStruc(const zen::XmlElement& input)
	{
		CExecutable::readStruc(input);
		return true;
	}


}