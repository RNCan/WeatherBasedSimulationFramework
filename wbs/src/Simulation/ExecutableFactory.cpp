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
#include "StdAfx.h"
#include "Simulation/ExecutableFactory.h"

using namespace std;


namespace WBSF
{

	char CExecutableFactory::CLASS_NAME[20][100] = { 0 };
	CreateObjectF CExecutableFactory::CLASS_REGISTRED[20] = { 0 };
	short CExecutableFactory::NB_CLASS_REGISTRED = 0;

	short CExecutableFactory::Find(const string& className)
	{
		short index = -1;
		for (int i = 0; i < NB_CLASS_REGISTRED; i++)
		{
			if (strcmp(className.c_str(), CLASS_NAME[i]) == 0)
			{
				index = i;
				break;
			}
		}

		return index;
	}

	int CExecutableFactory::RegisterClass(const string& className, CreateObjectF createObjectFuntion)
	{
		ASSERT(NB_CLASS_REGISTRED >= 0 && NB_CLASS_REGISTRED < MAX_CLASS - 1);

		int pos = Find(className);
		if (pos == -1)
		{
			strcpy(CLASS_NAME[NB_CLASS_REGISTRED], className.c_str());
			CLASS_REGISTRED[NB_CLASS_REGISTRED] = createObjectFuntion;
			NB_CLASS_REGISTRED++;

			ASSERT(Find(className) == NB_CLASS_REGISTRED - 1);
		}


		return NB_CLASS_REGISTRED - 1;

	}

	CExecutablePtr CExecutableFactory::CreateObject(const string& className)
	{
		CExecutablePtr pItem;

		//	if( pCLASS_NAME && pCLASS_REGISTRED)
		//	{
		//		int pos = pCLASS_NAME->Find(className, false);
		short pos = Find(className);
		if (pos >= 0)
		{
			CreateObjectF CreateObject = CLASS_REGISTRED[pos];
			pItem = (*CreateObject)();

			//			pItem = (*pCLASS_REGISTRED)[pos]->CreateObject();
		}
		else
		{
			int i = 0;
			i = 5;
			//AfxThrowInvalidArgException();
		}
		//}

		return pItem;
	}

}