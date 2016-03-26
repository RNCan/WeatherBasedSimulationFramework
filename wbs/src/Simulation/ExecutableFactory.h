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
	typedef CExecutablePtr(PASCAL *CreateObjectF)();

	class CExecutableFactory
	{
	public:


		static int RegisterClass(const std::string& className, CreateObjectF createObjectFuntion);
		static CExecutablePtr CreateObject(const std::string& className);

	protected:

		static short Find(const std::string& className);
		static const short MAX_CLASS = 20;
		static char CLASS_NAME[MAX_CLASS][100];
		static CreateObjectF CLASS_REGISTRED[MAX_CLASS];
		static short NB_CLASS_REGISTRED;

	};

}