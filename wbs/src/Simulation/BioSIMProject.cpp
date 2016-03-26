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

#include "Simulation/BioSIMProject.h"
#include "Simulation/ExecutableFactory.h"

#include "WeatherBasedSimulationString.h"

using namespace std;
namespace WBSF
{

	//****************************************************************
	//CBioSIMProject

	const char* CBioSIMProject::XML_FLAG = "BioSIMProject";


	CBioSIMProject::CBioSIMProject()
	{
		m_pMyself = NULL;

		//CExecutableGroup* pGroup = new CExecutableGroup();

		SetName("Project");
		SetInternalName("Project");
		m_bAddPath = false;
	}

	CBioSIMProject::CBioSIMProject(const CBioSIMProject& in)
	{
		operator=(in);
	}

	CBioSIMProject::~CBioSIMProject()
	{}

	void CBioSIMProject::Reset()
	{
		m_filePath.clear();
		CExecutableGroup::Reset();
		SetName("Project");
		//m_graphArray.RemoveAll();
	}

	CBioSIMProject& CBioSIMProject::operator =(const CBioSIMProject& in)
	{
		ASSERT(!GetParent());

		if (&in != this)
		{
			CExecutableGroup::operator=(in);
			//m_pExecutable = in.m_pExecutable->CopyObject();
			//m_graphArray = in.m_graphArray;
		}

		ASSERT(operator==(in));

		return *this;
	}

	bool CBioSIMProject::operator == (const CBioSIMProject& in)const
	{
		return CExecutableGroup::operator==(in)/*&&m_graphArray==in.m_graphArray*/;
	}

	CExecutablePtr CBioSIMProject::FindItem(const string& iName)
	{
		ASSERT(m_pMyself);
		//we check if it's the project itself...
		if (GetInternalName() == iName)
			return *m_pMyself;

		return CExecutableGroup::FindItem(iName);
	}


	ERMsg CBioSIMProject::Load(const string& filePath)
	{
		ERMsg msg;

		zen::XmlDoc doc;

		msg = load(filePath, doc);
		if (msg)
		{
			string rootName = doc.root().getNameAs<string>();
			if (rootName.empty() || !IsEqualNoCase(rootName, XML_FLAG))
			{
				msg.ajoute(FormatMsg(IDS_INVALID_BIOSIM_DOCUMENT, filePath));
			}

			if (msg)
			{
				readStruc(doc.root());
				m_filePath = filePath;
			}
		}

		return msg;
	}

	ERMsg CBioSIMProject::Save(const string& filePath)
	{
		ERMsg msg;

		try
		{
			zen::XmlDoc doc(XML_FLAG);
			writeStruc(doc.root());
			doc.root().setAttribute("version", "11");
			doc.setEncoding("Windows-1252");

			std::string filePathU8 = ANSI_UTF8(filePath);
			zen::save(doc, filePathU8); //throw XmlFileError, XmlParsingError
		}
		catch (const zen::XmlFileError& e)
		{
			// handle error
			msg.ajoute(GetErrorDescription(e.lastError));
		}
		catch (const ERMsg& e)
		{
			// handle error
			msg = e;
		}

		//msg = zen::SaveXML(filePath, XML_FLAG, "11", *this);

		if (msg)
			m_filePath = filePath;

		return msg;
	}

	ERMsg CBioSIMProject::Execute(const CFileManager& fileManager, CCallback& callback)
	{
		ERMsg msg;

		callback.SetNbTask(GetNbExecute(true));
		msg += ExecuteChild(fileManager, callback);

		string logText = GetOutputString(msg, callback, true);
		string filePath = GetLogFilePath(GetPath(fileManager));

		msg += WriteOutputMessage(filePath, logText);

		return msg;
	}




}