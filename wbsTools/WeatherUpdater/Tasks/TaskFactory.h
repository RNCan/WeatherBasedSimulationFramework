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
	

	/*class CParamClassInfo
	{
	public:

		std::string m_className;
		CTaskParametersVector m_paramDefArray;

	};
*/

	//typedef std::map<std::string, CTaskParametersVector> CClassInfoMap;

	//typedef boost::function< CTaskPtr > a_factory;
	typedef std::map<std::string, createF> ClassMap;
	typedef ClassMap::const_iterator ClassIt;


	class CTaskFactory
	{
	public:
	
		static CTaskFactory& GetInstance()
		{
			static CTaskFactory INSTANCE; // Guaranteed to be destroyed.
			// Instantiated on first use.
			return INSTANCE;
		}

		static ClassIt begin(){ return GetInstance().m_classMap.begin(); }
		static ClassIt end(){ return GetInstance().m_classMap.end(); }
		//static size_t size(){ return GetInstance().m_classMap.size(); }
		//static string ClassName(size_t i){ ClassIt it = GetInstance().m_classMap.cbegin(); for (size_t ii = 0; ii != i; ii++, it++); return it->first; }
		static size_t RegisterClass(const std::string& className, createF createObjectFuntion);// { GetInstance().m_classMap[className] = createObjectFuntion; }
		static CTaskPtr CreateObject(const std::string& className);
		static bool IsRegistered(const std::string& className);

		static CTaskPtr CreateFromClipbord();
		//static void UpdateLanguage();
		//static CInfoParameters& GetInfoParameters(const std::string& sourceName){ return m_classMap[sourceName].m_paramDefArray; }
		//static CTaskParametersVector& GetParamClassInfo(const std::string& sourceName){ return m_classMap[sourceName]; }

	private:

		CTaskFactory()
		{
			//map["Aa"] = boost::bind(boost::factory< Aa::pointer >, _1);
			//map["Ab"] = boost::bind(boost::factory< Ab::create >, _1);
			//[…]
			//map["Ax"] = boost::bind(boost::factory< Ax::create >, _1);
		}

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