//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once


#include "FileManager/DirectoryManager.h"

namespace WBSF
{

	class CParametersVariationsDefinition;

	class CParametersVariationsDirectoryManager : public CDirectoryManager
	{
	public:

		static const char* SUB_DIR_NAME;


		CParametersVariationsDirectoryManager(const std::string& projectPath = "", const std::string& directoryListString = "");
		CParametersVariationsDirectoryManager(const CParametersVariationsDirectoryManager& NDM);
		virtual ~CParametersVariationsDirectoryManager(void);

		ERMsg Get(const std::string& fileName, CParametersVariationsDefinition& PV)const;
		ERMsg Set(const std::string& fileName, CParametersVariationsDefinition& PV)const;

		CParametersVariationsDirectoryManager& operator=(const CParametersVariationsDirectoryManager& NDM);


	};

}