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
#include "TaskFactory.h"

using namespace std;


namespace WBSF
{
	
	size_t CTaskFactory::RegisterTask(const std::string& className, createF createObjectFuntion)
	{
		GetInstance().m_classMap[className] = createObjectFuntion;
		return GetInstance().m_classMap.size();
	}

	CTaskPtr CTaskFactory::CreateObject(const std::string& className)
	{
		CTaskPtr pTask;

		std::map<std::string, createF>::iterator it = GetInstance().m_classMap.find(className);
		if (it != GetInstance().m_classMap.end())
		{
			createF CreateObject = it->second;
			pTask = (*CreateObject)();
		}
		else
		{
			int i = 0;
			i = 5;
		}

		return pTask;
	}

	//static function
	bool CTaskFactory::IsRegistered(const std::string& className)
	{ 
		return GetInstance().m_classMap.find(className) != GetInstance().m_classMap.end();
	}

	CTaskPtr CTaskFactory::CreateFromClipbord()
	{
		bool bRep = false;

		string className = "";

		try
		{
			string str = WBSF::GetClipboardText();
			zen::XmlDoc doc = zen::parse(str);
			if (doc.root().getNameAs<string>() == "WeatherUpdaterTask")
			{
				auto it = doc.root().getChild("Task");
				if (it != NULL)
				{
					it->getAttribute("type", className);
					if (IsRegistered(className))
					{
						CTaskPtr pTask = CreateObject(className);
						ASSERT(pTask);

						it->getAttribute("name", pTask->m_name);
						it->getAttribute("execute", pTask->m_bExecute);
						if (pTask->readStruc(*it))
						{
							
							return pTask;
						}
					}
				}
					
			}
		}
		catch (...)
		{
		}
		

		return NULL;

	}

	

	std::string CTaskFactory::ClassNameFromResourceID(UINT ID)
	{
		std::string class_name;
		for (ClassMap::const_iterator it = GetInstance().begin(); it != GetInstance().end() && class_name.empty(); it++)
		{
			CTaskPtr pTask = CTaskFactory::CreateObject(it->first);
			ASSERT(pTask.get());
			
			if (pTask->GetDescriptionStringID() == ID)
				class_name = it->first;
		}

		return class_name;
	}
}