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

	//char CExecutableFactory::CLASS_NAME[20][100] = { 0 };
	//CreateObjectF CExecutableFactory::CLASS_REGISTRED[20] = { 0 };
	//short CExecutableFactory::NB_CLASS_REGISTRED = 0;

	//short CExecutableFactory::Find(const string& className)
	//{
	//	short index = -1;
	//	for (int i = 0; i < NB_CLASS_REGISTRED; i++)
	//	{
	//		if (strcmp(className.c_str(), CLASS_NAME[i]) == 0)
	//		{
	//			index = i;
	//			break;
	//		}
	//	}

	//	return index;
	//}

	//int CExecutableFactory::RegisterClass(const string& className, CreateObjectF createObjectFuntion)
	//{
	//	ASSERT(NB_CLASS_REGISTRED >= 0 && NB_CLASS_REGISTRED < MAX_CLASS - 1);

	//	int pos = Find(className);
	//	if (pos == -1)
	//	{
	//		strcpy(CLASS_NAME[NB_CLASS_REGISTRED], className.c_str());
	//		CLASS_REGISTRED[NB_CLASS_REGISTRED] = createObjectFuntion;
	//		NB_CLASS_REGISTRED++;

	//		ASSERT(Find(className) == NB_CLASS_REGISTRED - 1);
	//	}


	//	return NB_CLASS_REGISTRED - 1;

	//}

	//CTaskPtr CExecutableFactory::CreateObject(const string& className)
	//{
	//	CTaskPtr pItem;

	//	//	if( pCLASS_NAME && pCLASS_REGISTRED)
	//	//	{
	//	//		int pos = pCLASS_NAME->Find(className, false);
	//	short pos = Find(className);
	//	if (pos >= 0)
	//	{
	//		CreateObjectF CreateObject = CLASS_REGISTRED[pos];
	//		pItem = (*CreateObject)();

	//		//			pItem = (*pCLASS_REGISTRED)[pos]->CreateObject();
	//	}
	//	else
	//	{
	//		int i = 0;
	//		i = 5;
	//		//AfxThrowInvalidArgException();
	//	}
	//	//}

	//	return pItem;
	//}

	
	size_t CTaskFactory::RegisterClass(const std::string& className, createF createObjectFuntion)
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
					//		if (pTask->PasteFromClipBoard())
							
					}
				}
					
			}
		}
		catch (...)
		{
		}
		

		return NULL;

	}

	/*void CTaskFactory::UpdateLanguage()
	{
		for (std::map<std::string, createF>::iterator it = GetInstance().m_classMap.begin(); it != GetInstance().m_classMap.end(); it++)
		{
			createF CreateObject = it->second;
			CTaskPtr pTask = (*CreateObject)();
			pTask->UpdateLanguage();
		}
	}*/
}