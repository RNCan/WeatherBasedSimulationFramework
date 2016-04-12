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

#include "TaskBase.h"

namespace WBSF
{
	typedef CTaskPtr(PASCAL *createF)();

	typedef std::map<std::string, createF> ClassMap;
	typedef ClassMap::const_iterator ClassIt;


	class CTaskFactory
	{
	public:
	
		static CTaskFactory& GetInstance()
		{
			static CTaskFactory INSTANCE; // Guaranteed to be destroyed.
			return INSTANCE;// Instantiated on first use.
		}

		static ClassIt begin(){ return GetInstance().m_classMap.begin(); }
		static ClassIt end(){ return GetInstance().m_classMap.end(); }
		static size_t RegisterTask(const std::string& className, createF createObjectFuntion);
		static CTaskPtr CreateObject(const std::string& className);
		static bool IsRegistered(const std::string& className);
		static CTaskPtr CreateFromClipbord();

	private:

		CTaskFactory()
		{}

		CTaskFactory(const CTaskFactory& )
		{}

		ClassMap m_classMap;
	};

	

/*
	class 
	{
	public:


		static int RegisterClass(const std::string& className, CreateObjectF createObjectFuntion);
		static CTaskPtr CreateObject(const std::string& className);

	protected:

		static short Find(const std::string& className);
		static const short MAX_CLASS = 50;
		static char CLASS_NAME[MAX_CLASS][100];
		static CreateObjectF CLASS_REGISTRED[MAX_CLASS];
		static short NB_CLASS_REGISTRED;

	};
*/


}